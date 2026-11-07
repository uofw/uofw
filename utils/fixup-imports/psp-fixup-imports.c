/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.

   Based on the original file from PSPSDK, written by tyranid; many thanks to him!
*/

#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>

#include "../common/types.h"
#include "../common/elftypes.h"
#include "../common/prxtypes.h"
#include "../common/sha1.h"

#define PRX_LIBSTUB_SECT  ".lib.stub"
#define PRX_STUBTEXT_SECT ".sceStub.text"
#define PRX_NID_SECT      ".rodata.sceNid"
#define PRX_VSTUB_SECT    ".rodata.sceVstub"
#define PRX_SYMTAB_SECT   ".symtab"

struct NidMap
{
	unsigned int oldnid;
	unsigned int newnid;
};

#define MAX_MAPNIDS 1024

struct ImportMap
{
	struct ImportMap *next;
	char name[32];
	int  count;
	/* Could fail on things like PAF but who is going to want to remap 1000+ nids ? */
	struct NidMap nids[MAX_MAPNIDS];
};

static const char *g_outfile;
static const char *g_infile;
static const char *g_mapfile;
static unsigned char *g_elfdata = NULL;
static unsigned int  g_elfsize;
static struct ElfHeader g_elfhead;
static struct ElfSection *g_elfsections = NULL;
static struct ElfSection *g_modinfo = NULL;
static struct ElfSection *g_libstub = NULL;
static struct ElfSection *g_stubtext = NULL;
static struct ElfSection *g_vstub = NULL;
static struct ElfSection *g_nid = NULL;
static struct ElfSection *g_symtab = NULL;
static struct ImportMap  *g_map = NULL;
static int g_reversemap = 0;

/* Specifies that the current usage is to the print the pspsdk path */
static int g_verbose = 0;

static struct option arg_opts[] =
{
	{"output", required_argument, NULL, 'o'},
	{"reverse", no_argument, NULL, 'r' },
	{"map", required_argument, NULL, 'm'},
	{"verbose", no_argument, NULL, 'v'},
	{ NULL, 0, NULL, 0 }
};

/* Process the arguments */
int process_args(int argc, char **argv)
{
	int ch;

	g_outfile = NULL;
	g_infile = NULL;
	g_mapfile = NULL;

	ch = getopt_long(argc, argv, "vro:m:", arg_opts, NULL);
	while(ch != -1)
	{
		switch(ch)
		{
			case 'v' : g_verbose = 1;
					   break;
			case 'o' : g_outfile = optarg;
					   break;
			case 'm' : g_mapfile = optarg;
					   break;
			case 'r' : g_reversemap = 1;
					   break;
			default  : break;
		};

		ch = getopt_long(argc, argv, "vro:m:", arg_opts, NULL);
	}

	argc -= optind;
	argv += optind;

	if(argc < 1)
	{
		return 0;
	}

	g_infile = argv[0];

	if(g_outfile == NULL)
	{
		g_outfile = argv[0];
	}

	if(g_verbose)
	{
		fprintf(stderr, "Loading %s, outputting to %s\n", g_infile, g_outfile);
	}

	return 1;
}

void print_help(void)
{
	fprintf(stderr, "Usage: psp-fixup-imports [-v] [-o outfile.elf] infile.elf\n");
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "-o, --output outfile    : Output to a different file\n");
	fprintf(stderr, "-m, --map    mapfile    : Specify a firmware NID mapfile\n");
	fprintf(stderr, "-r, --reverse           : Reverse the mapping\n");
	fprintf(stderr, "-v, --verbose           : Verbose output\n");
}

/* Scan through the sections trying to find this address */
const unsigned char *find_data(unsigned int iAddr)
{
	u32 i;

	for(i = 0; i < g_elfhead.iShnum; i++)
	{
		if((g_elfsections[i].iAddr <= iAddr) && ((g_elfsections[i].iAddr + g_elfsections[i].iSize) > iAddr))
		{
			return g_elfsections[i].pData + (iAddr - g_elfsections[i].iAddr);
		}
	}

	return NULL;
}

struct ImportMap *find_map_by_name(const char *name)
{
	struct ImportMap *currmap = g_map;

	while(currmap)
	{
		if(strcmp(name, currmap->name) == 0)
		{
			break;
		}
		currmap = currmap->next;
	}
	return currmap;
}

unsigned char *load_file(const char *file, unsigned int *size)
{
	FILE *fp;
	unsigned char *data = NULL;

	do
	{
		fp = fopen(file, "rb");
		if(fp != NULL)
		{
			(void) fseek(fp, 0, SEEK_END);
			*size = ftell(fp);
			rewind(fp);

			if(*size < sizeof(Elf32_Ehdr))
			{
				fprintf(stderr, "Error, invalid file size\n");
				break;
			}

			data = (unsigned char *) malloc(*size);
			if(data == NULL)
			{
				fprintf(stderr, "Error, could not allocate memory for ELF\n");
				break;
			}

			(void) fread(data, 1, *size, fp);
			fclose(fp);
		}
		else
		{
			fprintf(stderr, "Error, could not find file %s\n", file);
		}
	}
	while(0);

	return data;
}

/* Validate the ELF header */
int validate_header(unsigned char *data)
{
	Elf32_Ehdr *head;
	int ret = 0;

	head = (Elf32_Ehdr*) data;

	do
	{
		/* Read in the header structure */
		g_elfhead.iMagic = LW(head->e_magic);
		g_elfhead.iClass = head->e_class;
		g_elfhead.iData = head->e_data;
		g_elfhead.iIdver = head->e_idver;
		g_elfhead.iType = LH(head->e_type);
		g_elfhead.iMachine = LH(head->e_machine);
		g_elfhead.iVersion = LW(head->e_version);
		g_elfhead.iEntry = LW(head->e_entry);
		g_elfhead.iPhoff = LW(head->e_phoff);
		g_elfhead.iShoff = LW(head->e_shoff);
		g_elfhead.iFlags = LW(head->e_flags);
		g_elfhead.iEhsize = LH(head->e_ehsize);
		g_elfhead.iPhentsize = LH(head->e_phentsize);
		g_elfhead.iPhnum = LH(head->e_phnum);
		g_elfhead.iShentsize = LH(head->e_shentsize);
		g_elfhead.iShnum = LH(head->e_shnum);
		g_elfhead.iShstrndx = LH(head->e_shstrndx);

		if(g_verbose)
		{
			fprintf(stderr, "Magic %08X, Class %02X, Data %02X, Idver %02X\n", g_elfhead.iMagic,
					g_elfhead.iClass, g_elfhead.iData, g_elfhead.iIdver);
			fprintf(stderr, "Type %04X, Machine %04X, Version %08X, Entry %08X\n", g_elfhead.iType,
					g_elfhead.iMachine, g_elfhead.iVersion, g_elfhead.iEntry);
			fprintf(stderr, "Phoff %08X, Shoff %08X, Flags %08X, Ehsize %08X\n", g_elfhead.iPhoff,
					g_elfhead.iShoff, g_elfhead.iFlags, g_elfhead.iEhsize);
			fprintf(stderr, "Phentsize %04X, Phnum %04X\n", g_elfhead.iPhentsize, g_elfhead.iPhnum);
			fprintf(stderr, "Shentsize %04X, Shnum %08X, Shstrndx %04X\n", g_elfhead.iShentsize,
					g_elfhead.iShnum, g_elfhead.iShstrndx);
		}

		if(g_elfhead.iMagic != ELF_MAGIC)
		{
			fprintf(stderr, "Error, invalid magic in the header\n");
			break;
		}

		if((g_elfhead.iType != ELF_EXEC_TYPE) && (g_elfhead.iType != ELF_PRX_TYPE))
		{
			fprintf(stderr, "Error, not EXEC type elf\n");
			break;
		}

		if(g_elfhead.iMachine != ELF_MACHINE_MIPS)
		{
			fprintf(stderr, "Error, not MIPS type ELF\n");
			break;
		}

		if(g_elfhead.iShnum < g_elfhead.iShstrndx)
		{
			fprintf(stderr, "Error, number of headers is less than section string index\n");
			break;
		}

		ret = 1;
	}
	while(0);

	return ret;
}

/* Load sections into ram */
int load_sections()
{
	int ret = 0;
	unsigned int load_addr = 0xFFFFFFFF;

	if(g_elfhead.iShnum > 0)
	{
		do
		{
			Elf32_Shdr *sect;
			u32 i;

			g_elfsections = (struct ElfSection *) malloc(sizeof(struct ElfSection) * g_elfhead.iShnum);
			if(g_elfsections == NULL)
			{
				fprintf(stderr, "Error, could not allocate memory for sections\n");
				break;
			}

			memset(g_elfsections, 0, sizeof(struct ElfSection) * g_elfhead.iShnum);

			for(i = 0; i < g_elfhead.iShnum; i++)
			{
				sect = (Elf32_Shdr *) (g_elfdata + g_elfhead.iShoff + (i * g_elfhead.iShentsize));

				g_elfsections[i].iName = LW(sect->sh_name);
				g_elfsections[i].iType = LW(sect->sh_type);
				g_elfsections[i].iAddr = LW(sect->sh_addr);
				g_elfsections[i].iFlags = LW(sect->sh_flags);
				g_elfsections[i].iOffset = LW(sect->sh_offset);
				g_elfsections[i].iSize = LW(sect->sh_size);
				g_elfsections[i].iLink = LW(sect->sh_link);
				g_elfsections[i].iInfo = LW(sect->sh_info);
				g_elfsections[i].iAddralign = LW(sect->sh_addralign);
				g_elfsections[i].iEntsize = LW(sect->sh_entsize);
				g_elfsections[i].iIndex = i;

				if(g_elfsections[i].iOffset != 0)
				{
					g_elfsections[i].pData = g_elfdata + g_elfsections[i].iOffset;
				}

				if(g_elfsections[i].iFlags & SHF_ALLOC)
				{
					g_elfsections[i].blOutput = 1;
					if(g_elfsections[i].iAddr < load_addr)
					{
						load_addr = g_elfsections[i].iAddr;
					}
				}

				if(((g_elfsections[i].iType == SHT_REL) || (g_elfsections[i].iType == SHT_PRXRELOC))
						&& (g_elfsections[g_elfsections[i].iInfo].iFlags & SHF_ALLOC))
				{
					g_elfsections[i].pRef = &g_elfsections[g_elfsections[i].iInfo];
				}
			}

			/* Okay so we have loaded all the sections, lets fix up the names */
			for(i = 0; i < g_elfhead.iShnum; i++)
			{
				strcpy(g_elfsections[i].szName, (char *) (g_elfsections[g_elfhead.iShstrndx].pData + g_elfsections[i].iName));
				if(strcmp(g_elfsections[i].szName, PSP_MODULE_INFO_NAME) == 0)
				{
					g_modinfo = &g_elfsections[i];
				}
				else if(strcmp(g_elfsections[i].szName, PRX_LIBSTUB_SECT) == 0)
				{
					g_libstub = &g_elfsections[i];
				}
				else if(strcmp(g_elfsections[i].szName, PRX_STUBTEXT_SECT) == 0)
				{
					g_stubtext = &g_elfsections[i];
				}
				else if(strcmp(g_elfsections[i].szName, PRX_VSTUB_SECT) == 0)
				{
					g_vstub = &g_elfsections[i];
				}
				else if(strcmp(g_elfsections[i].szName, PRX_NID_SECT) == 0)
				{
					g_nid = &g_elfsections[i];
				}
				else if(strcmp(g_elfsections[i].szName, PRX_SYMTAB_SECT) == 0)
				{
					g_symtab = &g_elfsections[i];
				}
			}

			if(g_verbose)
			{
				for(i = 0; i < g_elfhead.iShnum; i++)
				{
					fprintf(stderr, "\nSection %d: %s\n", i, g_elfsections[i].szName);
					fprintf(stderr, "Name %08X, Type %08X, Flags %08X, Addr %08X\n",
							g_elfsections[i].iName, g_elfsections[i].iType,
							g_elfsections[i].iFlags, g_elfsections[i].iAddr);
					fprintf(stderr, "Offset %08X, Size %08X, Link %08X, Info %08X\n",
							g_elfsections[i].iOffset, g_elfsections[i].iSize,
							g_elfsections[i].iLink, g_elfsections[i].iInfo);
					fprintf(stderr, "Addralign %08X, Entsize %08X pData %p\n",
							g_elfsections[i].iAddralign, g_elfsections[i].iEntsize,
							g_elfsections[i].pData);
				}

				fprintf(stderr, "ELF Load Base address %08X\n", load_addr);
			}

			if(g_modinfo == NULL)
			{
				fprintf(stderr, "Error, no sceModuleInfo section found\n");
				break;
			}

			ret = 1;
		}
		while(0);
	}
	else
	{
		fprintf(stderr, "Error, no sections in the ELF\n");
	}

	return ret;
}

/* Load an ELF file */
int load_elf(const char *elf)
{
	int ret = 0;

	do
	{
		g_elfdata = load_file(elf, &g_elfsize);
		if(g_elfdata == NULL)
		{
			break;
		}

		if(!validate_header(g_elfdata))
		{
			break;
		}

		if(!load_sections())
		{
			break;
		}

		ret = 1;
	}
	while(0);

	return ret;
}

/* Free allocated memory */
void free_data(void)
{
	if(g_elfdata != NULL)
	{
		free(g_elfdata);
		g_elfdata = NULL;
	}

	if(g_elfsections != NULL)
	{
		free(g_elfsections);
		g_elfsections = NULL;
	}
}

void strip_wsp(char *str)
{
	int len;
	char *p;

	len = strlen(str);
	while((len > 0) && (isspace(str[len-1])))
	{
		str[--len] = 0;
	}

	p = str;
	while(isspace(*p))
	{
		p++;
		len--;
	}

	memmove(str, p, len);
	str[len] = 0;
}

/* Load map file in
 * File format is :-
 * @LibraryName followed by 0 or more
 * OLDNID=NEWNID [ Comment ]
 *
 * Read in badly :P
 */
int load_mapfile(const char *mapfile)
{
	int ret = 1;
	char buf[1024];
	struct ImportMap *currmap = NULL;
	int line = 0;

	if(mapfile != NULL)
	{
		do
		{
			FILE *fp;

			fp = fopen(mapfile, "r");
			if(fp != NULL)
			{
				while(fgets(buf, sizeof(buf), fp))
				{
					line++;
					strip_wsp(buf);
					if((buf[0]) && (buf[0] != '#'))
					{
						if(buf[0] == '@')
						{
							struct ImportMap *temp;

							temp = (struct ImportMap *) malloc(sizeof(struct ImportMap));
							if(temp == NULL)
							{
								printf("Error allocating memory for import map\n");
								ret = 0;
								break;
							}

							memset(temp, 0, sizeof(struct ImportMap));
							if(currmap == NULL)
							{
								g_map = temp;
							}
							else
							{
								temp->next = currmap;
								g_map = temp;
							}

							currmap = temp;
							if(buf[1])
							{
								strncpy(currmap->name, &buf[1], 32);
								currmap->name[31] = 0;
							}
							else
							{
								printf("Invalid library name at line %d\n", line);
								break;
							}

							if(g_verbose)
							{
								printf("Mapping library %s\n", currmap->name);
							}
						}
						else
						{
							unsigned int oldnid;
							unsigned int newnid;
							char *endp;

							if(currmap->count == MAX_MAPNIDS)
							{
								printf("Error, number of defined nids exceed maximum\n");
								break;
							}

							/* Hex data should be prefixed with 0 */
							if(buf[0] == '0')
							{
								errno = 0;
								oldnid = strtoul(buf, &endp, 16);
								if((errno != 0) || (*endp != ':'))
								{
									printf("Invalid NID entry on line %d\n", line);
									continue;
								}
							}
							else
							{
								unsigned char hash[SHA1_DIGEST_SIZE];

								endp = strchr(buf, ':');
								if(endp == NULL)
								{
									printf("Invalid NID entry on line %d\n", line);
									continue;
								}

								sha1(hash, (unsigned char *) buf, endp-buf);
								oldnid = hash[0] | (hash[1] << 8) | (hash[2] << 16) | (hash[3] << 24);
							}

							newnid = strtoul(endp+1, &endp, 16);
							if(g_verbose)
							{
								fprintf(stderr, "NID Mapping 0x%08X to 0x%08X\n", oldnid, newnid);
							}

							currmap->nids[currmap->count].oldnid = oldnid;
							currmap->nids[currmap->count].newnid = newnid;
							currmap->count++;
						}
					}
				}
				fclose(fp);
			}
		}
		while(0);
	}

	return ret;
}

#define MIPS_JR_31 0x03e00008
#define MIPS_NOP   0x0

int fixup_functions(void)
{
	unsigned int *pText;
	unsigned int *pNid;
	struct PspModuleImport *pLastImport = NULL;
	int count;

	if(g_stubtext == NULL || g_nid == NULL) // no imported functions
		return 1;

	count = g_stubtext->iSize / 8;
	pText = (unsigned int *) g_stubtext->pData;
	pNid = (unsigned int *) g_nid->pData;

	if(g_verbose)
	{
		fprintf(stderr, "Import count %d\n", count);
	}

	while(count > 0)
	{
		unsigned int stub_addr;
		unsigned int stub_nid;
		unsigned int sect_nid;

		stub_addr = LW(pText[0]);
		stub_nid  = LW(pText[1]);
		sect_nid  = LW(pNid[0]);

		/* Check if this is an original format NID */
		if((stub_addr != MIPS_JR_31) || (stub_nid != MIPS_NOP))
		{
			struct PspModuleImport *pImport;
			u16    func_count;

			if(g_verbose)
			{
				fprintf(stderr, "Found import to fixup. pStub %08X, Nid %08X, NidInSect %08X\n", stub_addr, stub_nid, sect_nid);
			}

			if(stub_nid != sect_nid)
			{
				fprintf(stderr, "Error, unmatched NIDs\n");
				return 0;
			}

			if((stub_addr < g_libstub->iAddr) || (stub_addr > (g_libstub->iAddr + g_libstub->iSize)) || (stub_addr & 3))
			{
				fprintf(stderr, "Error, invalid stub address\n");
				return 0;
			}

			pImport = (struct PspModuleImport *) (g_libstub->pData + (stub_addr - g_libstub->iAddr));
			if(g_verbose)
			{
				fprintf(stderr, "Import Stub %p, %08X, %08X, %02X, %02X, %04X, %08X, %08X\n", pImport,
						LW(pImport->name), LW(pImport->flags), pImport->entry_size, pImport->var_count,
						LH(pImport->func_count), LW(pImport->nids), LW(pImport->funcs));
			}

			func_count = LH(pImport->func_count);

			if(func_count == 0)
			{
				/* Setup the stub */
				SW(&pImport->nids, ((unsigned char *) pNid - g_nid->pData) + g_nid->iAddr);
				SW(&pImport->funcs, ((unsigned char *) pText - g_stubtext->pData) + g_stubtext->iAddr);
			}
			else
			{
				if((pLastImport) && (pImport != pLastImport))
				{
					fprintf(stderr, "Error, could not fixup imports, stubs out of order.\n");
					fprintf(stderr, "Ensure the SDK libraries are linked in last to correct this error\n");
					return 0;
				}
			}

			pLastImport = pImport;
			func_count++;
			SH(&pImport->func_count, func_count);
			SW(&pText[0], MIPS_JR_31);
			SW(&pText[1], MIPS_NOP);
		}
		else
		{
			/* Set last import to some value so we know if we have out of order stubs over a fixed stub table */
			pLastImport = (struct PspModuleImport *) pText;
		}

		pText += 2;
		pNid++;
		count--;
	}

	/* Should indicate no error occurred */
	if(count == 0)
	{
		if(g_verbose)
		{
			fprintf(stderr, "No libraries to fixup\n");
		}
	}

	return 1;
}

typedef struct
{
	int out;
	int sourceSect;
	int sourceReloc;
} RelocEntry;

typedef struct
{
	int addr;
	int numRelocs;
	RelocEntry *relocs;
} Reloc;

/*
 * The ELF can have several PT_LOAD segments (e.g. the code and data, then .bss, as in the original modules). The
 * variable stubs must be in the first one, whose file data holds all the relocated words; the addresses after
 * the variable stubs in the first segment move by the size of the inserted data, and the following segments
 * move to the next address with the alignment of their first section, like the linker would place them.
 */
#define MAX_SEGS 4
static int g_nsegs;
static u32 g_segVaddr[MAX_SEGS];
static u32 g_segDelta[MAX_SEGS];
static u32 g_vstubEnd;
static u32 g_shift;

static u32 new_addr(u32 addr)
{
	int i;

	if (addr < g_vstubEnd)
		return addr;
	for (i = g_nsegs - 1; i > 0; i--)
		if (addr >= g_segVaddr[i])
			return addr + g_segDelta[i];
	return addr + g_shift;
}

/* Compute the new addresses of the segments after the first one (g_vstubEnd and g_shift set) */
static int compute_segments(void)
{
	Elf32_Phdr *seg = (Elf32_Phdr*) (g_elfdata + g_elfhead.iPhoff);
	u32 end;
	int i, j;

	g_nsegs = g_elfhead.iPhnum;
	if (g_nsegs > MAX_SEGS) {
		fprintf(stderr, "Error, too many program headers\n");
		return 0;
	}
	for (i = 0; i < g_nsegs; i++) {
		if (LW(seg[i].p_type) != 1) {
			fprintf(stderr, "Error, could not fixup imported variables, the program headers must be PT_LOAD.\n");
			return 0;
		}
		g_segVaddr[i] = LW(seg[i].p_vaddr);
	}
	if (g_vstub->iAddr + g_vstub->iSize > LW(seg[0].p_vaddr) + LW(seg[0].p_filesz)) {
		fprintf(stderr, "Error, could not fixup imported variables, the variable stubs must be in the first segment.\n");
		return 0;
	}
	g_segDelta[0] = g_shift;
	end = LW(seg[0].p_vaddr) + LW(seg[0].p_memsz) + g_shift;
	for (i = 1; i < g_nsegs; i++) {
		u32 align = 1;
		u32 vaddr;
		/* the alignment of the first section of the segment */
		for (j = 0; j < (int) g_elfhead.iShnum; j++)
			if ((g_elfsections[j].iFlags & SHF_ALLOC) && g_elfsections[j].iAddr == g_segVaddr[i] && g_elfsections[j].iSize != 0)
				if (g_elfsections[j].iAddralign > align)
					align = g_elfsections[j].iAddralign;
		vaddr = (end + align - 1) & ~(align - 1);
		if (vaddr < g_segVaddr[i])
			vaddr = g_segVaddr[i];
		g_segDelta[i] = vaddr - g_segVaddr[i];
		end = vaddr + LW(seg[i].p_memsz);
	}
	return 1;
}

int fixup_variables(void)
{
	unsigned int *pText;
	struct PspModuleImport *pLastImport = NULL;
	int numStub;
	Reloc *relocs;

	if (g_vstub == NULL) // no imported variables
		return 1;

	if (g_elfhead.iPhnum < 1) {
		fprintf(stderr, "Error, could not fixup imported variables. ELF must have a program header.\n");
		return 0;
	}

	numStub = g_vstub->iSize / 8;
	relocs = malloc(sizeof(Reloc) * numStub);
	int i;
	for (i = 0; i < numStub; i++) {
		relocs[i].addr = 0;
		relocs[i].numRelocs = 0;
		relocs[i].relocs = NULL;
	}

	// check the relocations pointing to the imported variables
	int sectIdx;
	for (sectIdx = 0; sectIdx < (int) g_elfhead.iShnum; sectIdx++) {
		if (g_elfsections[sectIdx].iType != SHT_REL) {
			continue;
		}
		int relCount = g_elfsections[sectIdx].iSize / sizeof(Elf32_Rel);
		for (i = 0; i < relCount; i++) {
			Elf32_Rel *rel = &((Elf32_Rel*) g_elfsections[sectIdx].pData)[i];
			int type = ELF32_R_TYPE(LW(rel->r_info));
			int symbol = ELF32_R_SYM(LW(rel->r_info));
			int offset = LW(rel->r_offset);
			Elf32_Sym *sym = &((Elf32_Sym*) g_symtab->pData)[symbol];
			u32 symValue = LW(sym->st_value);

			int importedVar = symValue >= g_vstub->iAddr && symValue < g_vstub->iAddr + g_vstub->iSize;
			if (!importedVar) {
				continue;
			}

			// input must have one program header so iOfsPH/iValPH will be always zero
			int iOfsPH = 0;
			int iValPH = 0;
			Elf32_Phdr *seg = (Elf32_Phdr*) (g_elfdata + g_elfhead.iPhoff);
			u32 dwRealOfs = offset + LW(seg[iOfsPH].p_vaddr);
			u32 dwCurrBase = LW(seg[iValPH].p_vaddr);
			u32 *pData = (u32*) (g_elfdata + LW(seg->p_offset) + dwRealOfs);

			u32 addr;
			int out = 0;
			switch (type) {
			case R_MIPS_32: {
				addr = LW(*pData);
				out = 0x08000000 | (dwRealOfs >> 2);
				SW(pData, addr - symValue);
			}
				break;
			case R_MIPS_HI16: {
				int inst = LW(*pData);
				addr = ((inst & 0xFFFF) << 16) + dwCurrBase;
				int loinst;
				int oldcount = i;
				while (++i < relCount) {
					if (ELF32_R_TYPE(LW(((Elf32_Rel*)g_elfsections[sectIdx].pData)[i].r_info)) != R_MIPS_HI16) break;
				}
				if (i < relCount) {
					loinst = LW(*(u32*)(g_elfdata + LW(seg->p_offset) + (LW((Elf32_Rel*)g_elfsections[sectIdx].pData)[i].r_offset) + LW(seg[iOfsPH].p_vaddr)));
				} else {
					loinst = 0;
				}

				addr = (s32) addr + (s16) (loinst & 0xFFFF);
				u32 symbolOffset = addr - symValue;
				out = 0x14000000 | (dwRealOfs >> 2);
				SW(pData, (inst & 0xFFFF0000) | ((symbolOffset - (s32)(s16)symbolOffset) >> 16));
				i = oldcount;
			}
				break;
			case R_MIPS_LO16: {
				int inst = LW(*pData);
				addr = ((s16) (inst & 0xFFFF)) + dwCurrBase;
				u32 symbolOffset = addr - symValue;
				out = 0x18000000 | (dwRealOfs >> 2);
				SW(pData, (inst & 0xFFFF0000) | ((s32)(s16)(symbolOffset) & 0xFFFF));
			}
				break;
			default: {
				fprintf(stderr, "Error, use of imported variable in an unsupported relocation type\n");
				return 0;
			}
			}

			SW(&rel->r_offset, 0);
			SW(&rel->r_info, 0);

			int stubIdx = (symValue - g_vstub->iAddr) / 8;
			relocs[stubIdx].numRelocs++;
			if (relocs[stubIdx].numRelocs == 1)
				relocs[stubIdx].relocs = malloc(sizeof(RelocEntry) * 2);
			else
				relocs[stubIdx].relocs = realloc(relocs[stubIdx].relocs, sizeof(RelocEntry) * (relocs[stubIdx].numRelocs + 1));
			RelocEntry *entry = &relocs[stubIdx].relocs[relocs[stubIdx].numRelocs - 1];
			entry->out = out;
			entry->sourceSect = sectIdx;
			entry->sourceReloc = i;
		}
	}

	// add final zero entry
	for (i = 0; i < numStub; i++)
	{
		relocs[i].numRelocs++;
		if (relocs[i].numRelocs == 1)
			relocs[i].relocs = malloc(sizeof(RelocEntry) * 2);
		else
			relocs[i].relocs = realloc(relocs[i].relocs, sizeof(RelocEntry) * (relocs[i].numRelocs + 1));
		RelocEntry* entry = &relocs[i].relocs[relocs[i].numRelocs - 1];
		entry->out = 0;
		entry->sourceSect = -1;
		entry->sourceReloc = -1;
	}

	// add the special relocations data
	int stubRelocStartAddr = g_vstub->iAddr + g_vstub->iSize;
	int addr = stubRelocStartAddr;
	for (i = 0; i < numStub; i++)
	{
		relocs[i].addr = addr;
		addr += relocs[i].numRelocs * 4;
	}
	int shift = addr - stubRelocStartAddr;
	g_shift = shift;
	g_vstubEnd = stubRelocStartAddr;
	if (!compute_segments())
		return 0;
	g_elfsize += shift;
	g_elfdata = realloc(g_elfdata, g_elfsize);
	u8 *start = g_elfdata + g_vstub->iOffset + g_vstub->iSize;
	memmove(start + shift, start, g_elfsize - (g_vstub->iOffset + g_vstub->iSize + shift));
	int *cur = (int*)(g_elfdata + g_vstub->iOffset + g_vstub->iSize);
	for (i = 0; i < numStub; i++) {
		for (int j = 0; j < relocs[i].numRelocs; j++) {
			RelocEntry *entry = &relocs[i].relocs[j];
			*cur++ = entry->out;
		}
	}

	// shift section and program header offsets
	if (LW(((Elf32_Ehdr *)g_elfdata)->e_shoff) > g_vstub->iOffset + g_vstub->iSize)
		SW(&((Elf32_Ehdr *)g_elfdata)->e_shoff, LW(((Elf32_Ehdr *)g_elfdata)->e_shoff) + shift);
	if (LW(((Elf32_Ehdr *)g_elfdata)->e_phoff) > g_vstub->iOffset + g_vstub->iSize)
		SW(&((Elf32_Ehdr *)g_elfdata)->e_phoff, LW(((Elf32_Ehdr *)g_elfdata)->e_phoff) + shift);

	// shift program header sizes
	Elf32_Phdr *seg = (Elf32_Phdr*)(g_elfdata + g_elfhead.iPhoff);
	SW(&seg->p_filesz, LW(seg->p_filesz) + shift);
	SW(&seg->p_memsz, LW(seg->p_memsz) + shift);
	/* the following segments: moved in the file by the inserted data, in memory by their new address */
	for (i = 1; i < g_nsegs; i++) {
		SW(&seg[i].p_offset, LW(seg[i].p_offset) + shift);
		SW(&seg[i].p_vaddr, LW(seg[i].p_vaddr) + g_segDelta[i]);
		SW(&seg[i].p_paddr, LW(seg[i].p_paddr) + g_segDelta[i]);
	}

	// shift sections
	for (i = 0; i < (int)g_elfhead.iShnum; i++) {
		Elf32_Shdr *sect = (Elf32_Shdr *) (g_elfdata + ((Elf32_Ehdr *)g_elfdata)->e_shoff + (i * g_elfhead.iShentsize));
		if (LW(sect->sh_addr) > g_vstub->iAddr)
			SW(&sect->sh_addr, new_addr(LW(sect->sh_addr)));
		if (LW(sect->sh_offset) > g_vstub->iOffset)
			SW(&sect->sh_offset, LW(sect->sh_offset) + shift);
		if (LW(sect->sh_offset) == g_vstub->iOffset)
			SW(&sect->sh_size, LW(sect->sh_size) + shift);
	}

	// reload data
	free(g_elfsections);
	validate_header(g_elfdata);
	load_sections();

	// shift symbol table
	int symCount = g_symtab->iSize / sizeof(Elf32_Sym);
	/*
	 * How much each symbol moves, for the R_MIPS_HI16/R_MIPS_LO16 against it: their target is the symbol
	 * (plus a small addend) so it moves like the symbol. The target cannot be rebuilt from the
	 * instructions alone: an R_MIPS_LO16 does not always follow its R_MIPS_HI16 in the relocation table
	 * (one `lui` for several accesses, delay slots), so pairing it with a previous R_MIPS_HI16 gave a wrong
	 * target for some of them, which then did not move with their segment.
	 */
	s32 *symDelta = calloc(symCount, sizeof(s32));
	char *symDefined = calloc(symCount, 1);
	for (i = 0; i < symCount; i++)
	{
		Elf32_Sym *sym = &((Elf32_Sym*)g_symtab->pData)[i];
		u32 value = LW(sym->st_value);
		symDefined[i] = LH(sym->st_shndx) != 0 && LH(sym->st_shndx) < 0xFF00; /* not undefined, absolute or common */
		/* only the symbols after the variable stubs move (not the variable stubs themselves) */
		if (LH(sym->st_shndx) != 0 && value >= g_vstub->iAddr + g_vstub->iSize - shift) {
			SW(&sym->st_value, new_addr(value));
			symDelta[i] = new_addr(value) - value;
		}
	}

	// shift all relocations after vstub
	/* the relocated addresses which move: those after the original variable stubs, inside the image */
	u32 vstubEnd = g_vstub->iAddr + g_vstub->iSize - shift;
	u32 imageEnd = g_segVaddr[g_nsegs - 1] + LW(seg[g_nsegs - 1].p_memsz);
	for (i = 0; i < (int)g_elfhead.iShnum; i++)
	{
		if (g_elfsections[i].iType != SHT_REL) {
			continue;
		}
		int relCount = g_elfsections[i].iSize / sizeof(Elf32_Rel);
		int j;
		/* the high part of the last R_MIPS_HI16, for the relocations without a defined symbol */
		u32 lastHi = 0;
		if (g_verbose)
			fprintf(stderr, "Loading relocations %s\n", g_elfsections[i].szName);

		for(j = 0; j < relCount; j++)
		{
			int iOfsPH = 0;
			int iValPH = 0;
			Elf32_Phdr *seg = (Elf32_Phdr*)(g_elfdata + g_elfhead.iPhoff);
			Elf32_Rel *rel = &((Elf32_Rel*)g_elfsections[i].pData)[j];
			if (LW(seg[iOfsPH].p_vaddr) + LW(rel->r_offset) > g_vstub->iAddr + g_vstub->iSize - shift)
				SW(&rel->r_offset, new_addr(LW(rel->r_offset)));
			int type = ELF32_R_TYPE(LW(rel->r_info));
			int offset = LW(rel->r_offset);

			int lastJ = j;

			u32 dwRealOfs = offset + LW(seg[iOfsPH].p_vaddr);
			u32 dwCurrBase = LW(seg[iValPH].p_vaddr);
			u32 *pData = (u32*)(g_elfdata + LW(seg->p_offset) + dwRealOfs);
			if (strcmp(g_elfsections[i].szName, ".rel.rodata.sceModuleInfo") == 0 && j == 0) { // disable _gp relocation
				/* with several segments, $gp is relative to the following segments: it moves with them */
				if (g_nsegs > 1)
					SW(pData, new_addr(LW(*pData)));
				continue;
			}

			switch (type)
			{
			case R_MIPS_HI16: {
				int addr = ((LW(*pData) & 0xFFFF) << 16) + dwCurrBase;
				u32 sym = ELF32_R_SYM(LW(rel->r_info));
				lastHi = (LW(*pData) & 0xFFFF) << 16;
				if (sym < (u32)symCount && symDefined[sym]) {
					/* the low part (for the carry) is in the next R_MIPS_LO16 of the same symbol */
					int k;
					int lo = 0;
					for (k = j + 1; k < relCount; k++) {
						Elf32_Rel *lorel = &((Elf32_Rel*)g_elfsections[i].pData)[k];
						if (ELF32_R_TYPE(LW(lorel->r_info)) == R_MIPS_LO16 && ELF32_R_SYM(LW(lorel->r_info)) == sym) {
							lo = (s16)(LW(*(u32*)(g_elfdata + LW(seg->p_offset) + LW(lorel->r_offset) + LW(seg[iOfsPH].p_vaddr))) & 0xFFFF);
							break;
						}
					}
					addr += lo + symDelta[sym];
					SW(pData, (LW(*pData) & 0xFFFF0000) | ((((u32)addr - dwCurrBase) + 0x8000) >> 16));
					break;
				}
				int loinst;
				while (++j < relCount) {
					if (ELF32_R_TYPE(LW(((Elf32_Rel*)g_elfsections[i].pData)[j].r_info)) != R_MIPS_HI16) break;
				}
				if (j < relCount) {
					loinst = LW(*(u32*)(g_elfdata + LW(seg->p_offset) + (LW((Elf32_Rel*)g_elfsections[i].pData)[j].r_offset) + LW(seg[iOfsPH].p_vaddr)));
				} else {
					loinst = 0;
				}
				addr = (s32) addr + (s16) (loinst & 0xFFFF);
				if ((u32)addr >= vstubEnd && (u32)addr < imageEnd) {
					u32 naddr = new_addr(addr);
					SW(pData, (LW(*pData) & 0xFFFF0000) | (((naddr - dwCurrBase) + 0x8000) >> 16));
				}
			}
			break;
			case R_MIPS_16:
			case R_MIPS_LO16: {
				u32 sym = ELF32_R_SYM(LW(rel->r_info));
				if (type == R_MIPS_LO16 && sym < (u32)symCount && symDefined[sym]) {
					SW(pData, (LW(*pData) & 0xFFFF0000) | ((LW(*pData) + symDelta[sym]) & 0xFFFF));
					break;
				}
				u32 addr = (type == R_MIPS_LO16 ? lastHi : 0) + (s16)(LW(*pData) & 0xFFFF) + dwCurrBase;
				if (addr >= vstubEnd && addr < imageEnd)
					SW(pData, (LW(*pData) & 0xFFFF0000) | ((new_addr(addr) - dwCurrBase) & 0xFFFF));
			}
			break;
			/*
			case R_MIPS_X_HI16: {
				u32 addr = ((LW(*pData) & 0xFFFF) << 16) + rel->base + dwCurrBase;
				if (addr > g_vstub->iAddr)
					SW(pData, (LW(*pData) & 0xFFFF0000) | ((addr - rel->base - dwCurrBase + shift) >> 16));
				}
				break;
				*/
			case R_MIPS_26: {
				u32 addr = (LW(*pData) & 0x03FFFFFF) << 2;
				addr += dwCurrBase;
				if (addr >= vstubEnd && addr < imageEnd)
					SW(pData, (LW(*pData) & 0xFC000000) | (((new_addr(addr) - dwCurrBase) >> 2)));
			}
			break;
			case R_MIPS_REL32:
			case R_MIPS_32: {
				/* a whole word address (the rewritten function stubs, "jr $ra", do not move) */
				u32 addr = LW(*pData) + dwCurrBase;
				if (addr >= vstubEnd && addr < imageEnd)
					SW(pData, new_addr(addr) - dwCurrBase);
			}
			break;
			case R_MIPS_NONE:
				break;
			default:
				printf("Error: can't patch unhandled relocation type %d!\n", type);
				return 0;
			}

			j = lastJ;
		}
	}
	free(symDelta);
	free(symDefined);

	// add special relocations to stub entries
	int pos = 0;
	for (i = 0; i < numStub; i++) {
		for (int j = 0; j < relocs[i].numRelocs; j++) {
			RelocEntry* entry = &relocs[i].relocs[j];
			if (entry->sourceSect != -1 && entry->sourceReloc != -1) {
				Elf32_Rel *rel = &((Elf32_Rel*) g_elfsections[entry->sourceSect].pData)[entry->sourceReloc];
				// hacky but we have only single program header here
				SW(&rel->r_info, R_MIPS_26 | (1 << 8));
				SW(&rel->r_offset, stubRelocStartAddr + pos);
			}
			pos += 4;
		}
	}

	pText = (unsigned int *) g_vstub->pData;
	for (i = 0; i < numStub; i++)
	{
		unsigned int stub_addr;

		stub_addr = LW(pText[0]);

		struct PspModuleImport *pImport;
		u8     var_count;

		pImport = (struct PspModuleImport *) (g_libstub->pData + (stub_addr - g_libstub->iAddr));
		var_count = pImport->var_count;

		if(var_count == 0)
		{
			/* Setup the stub */
			SW(&pImport->vars, ((unsigned char *) pText - g_vstub->pData) + g_vstub->iAddr);
		}
		else
		{
			if((pLastImport) && (pImport != pLastImport))
			{
				fprintf(stderr, "Error, could not fixup imports, stubs out of order.\n");
				fprintf(stderr, "Ensure the SDK libraries are linked in last to correct this error\n");
				return 0;
			}
		}

		pLastImport = pImport;
		var_count++;
		pImport->var_count = var_count;
		SW(&pText[0], relocs[i].addr);

		pText += 2;
	}

	for (i = 0; i < numStub; i++)
		if (relocs[i].relocs != NULL)
			free(relocs[i].relocs);
	free(relocs);

	return 1;
}

int fixup_nidmap(void)
{
	struct PspModuleImport *pImport;
	u32 size;

	pImport = (struct PspModuleImport *) g_libstub->pData;
	size = g_libstub->iSize;

	while(size >= 5 * 4 && pImport->entry_size >= 5)
	{
		const char *str;

		str = (const char*) find_data(LW(pImport->name));
		if(str)
		{
			struct ImportMap *pMap;

			pMap = find_map_by_name(str);
			if(pMap)
			{
				int count;
				unsigned int *pNid;

				pNid = (unsigned int*) find_data(LW(pImport->nids));
				count = LH(pImport->func_count) + pImport->var_count;

				if(pNid && (count > 0))
				{
					if(g_verbose)
					{
						fprintf(stderr, "Mapping library %s\n", str);
					}

					while(count > 0)
					{
						int i;

						for(i = 0; i < pMap->count; i++)
						{
							unsigned oldnid, newnid;

							if(g_reversemap)
							{
								oldnid = pMap->nids[i].newnid;
								newnid = pMap->nids[i].oldnid;
							}
							else
							{
								newnid = pMap->nids[i].newnid;
								oldnid = pMap->nids[i].oldnid;
							}

							if(oldnid == *pNid)
							{
								if(g_verbose)
								{
									fprintf(stderr, "Mapping 0x%08X to 0x%08X\n", oldnid, newnid);
								}

								*pNid = newnid;
								break;
							}
						}

						pNid++;
						count--;
					}
				}
			}
		}

		/* the entries are entry_size words long (5 or 6) */
		size -= pImport->entry_size * 4 < size ? pImport->entry_size * 4 : size;
		pImport = (struct PspModuleImport *) ((unsigned char *) pImport + pImport->entry_size * 4);
	}

	return 1;
}

int main(int argc, char **argv)
{
	int ret = 0;

	if(process_args(argc, argv))
	{
		if(load_mapfile(g_mapfile) && load_elf(g_infile))
		{
			if(g_libstub == NULL || (fixup_functions() && fixup_variables() && fixup_nidmap()))
			{
				FILE *fp;

				fp = fopen(g_outfile, "wb");
				if(fp != NULL)
				{
					(void) fwrite(g_elfdata, 1, g_elfsize, fp);
					fclose(fp);
				}
				else
				{
					fprintf(stderr, "Error, couldn't open %s for writing\n", g_outfile);
					return 0;
				}
			}
			else
			{
				ret = 1;
			}
			free_data();
		}
		else
		{
			ret = 1;
		}
	}
	else
	{
		print_help();
		ret = 1;
	}

	return ret;
}
