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
#define PRX_TEXT_SECT     ".text"
#define PRX_TEXTREL_SECT  ".rel.text"

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
static struct ElfSection *g_text = NULL;
static struct ElfSection *g_textrel = NULL;
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
	int found_rel = 0;
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
					found_rel = 1;
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
                else if(strcmp(g_elfsections[i].szName, PRX_TEXT_SECT) == 0)
                {
                    g_text = &g_elfsections[i];
                }
                else if(strcmp(g_elfsections[i].szName, PRX_TEXTREL_SECT) == 0)
                {
                    g_textrel = &g_elfsections[i];
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
    int addr;
    int numRelocs;
    int *relocs;
} Reloc;

int fixup_variables(void)
{
    unsigned int *pText;
    struct PspModuleImport *pLastImport = NULL;
    int numStub;
    Reloc *relocs;
            
    if(g_vstub == NULL || g_textrel == NULL) // no imported variables
        return 1;

    numStub = g_vstub->iSize / 8;
    relocs = malloc(sizeof(Reloc) * numStub);
    int i;
    for (i = 0; i < numStub; i++) {
        relocs[i].addr = 0;
        relocs[i].numRelocs = 0;
        relocs[i].relocs = NULL;
    }
    
    int relCount = g_textrel->iSize / sizeof(Elf32_Rel);

    // check the relocations pointing to the imported variable
    for(i = 0; i < relCount; i++)
    {
        Elf32_Rel *rel = &((Elf32_Rel*)g_textrel->pData)[i];
        u32 dwRealOfs;
        u32 dwCurrBase;
        int iOfsPH;
        int iValPH;
        int type = ELF32_R_TYPE(LW(rel->r_info));
        int symbol = ELF32_R_SYM(LW(rel->r_info));
        int offset = rel->r_offset;
        Elf32_Phdr *seg = (Elf32_Phdr*)(g_elfdata + g_elfhead.iPhoff);
        u32 *pData;
        char added = 0;
        int out = 0;
        u32 addr;

        iOfsPH = symbol & 0xFF;
        iValPH = (symbol >> 8) & 0xFF;
        dwRealOfs = offset + LW(seg[iOfsPH].p_vaddr);
        dwCurrBase = LW(seg[iValPH].p_vaddr);
        pData = (u32*)(g_elfdata + g_text->iOffset + dwRealOfs);
        switch (type)
        {
        case R_MIPS_HI16: {
            int inst = LW(*pData);
            addr = ((inst & 0xFFFF) << 16) + dwCurrBase;
            int loinst;
            int oldcount = i;
            while (++i < relCount) {
                if (ELF32_R_TYPE(LW(((Elf32_Rel*)g_textrel->pData)[i].r_info)) != R_MIPS_HI16) break;
            }
            if (i < relCount) {
                loinst = LW(*(u32*)(g_elfdata + g_text->iOffset + ((Elf32_Rel*)g_textrel->pData)[i].r_offset + LW(seg[iOfsPH].p_vaddr)));
            } else {
                loinst = 0;
            }
                
            addr = (s32) addr + (s16) (loinst & 0xFFFF);
            if (addr >= g_vstub->iAddr && addr < g_vstub->iAddr + g_vstub->iSize) {
                added = 1;
                out = 0x14000000 | (dwRealOfs >> 2);
            }
            i = oldcount;
        }
            break;
        case R_MIPS_LO16: {
            addr = ((s16)(LW(*pData) & 0xFFFF)) + dwCurrBase;
            if (addr >= g_vstub->iAddr && addr < g_vstub->iAddr + g_vstub->iSize) {
                added = 1;
                out = 0x18000000 | (dwRealOfs >> 2);
            }
        }
            break;
        }

        if (added)
        {
            int off = (addr - g_vstub->iAddr) / 8;
            relocs[off].numRelocs++;
            if (relocs[off].numRelocs == 1)
                relocs[off].relocs = malloc(sizeof(int) * 2);
            else
                relocs[off].relocs = realloc(relocs[off].relocs, sizeof(int) * (relocs[off].numRelocs + 1));
            relocs[off].relocs[relocs[off].numRelocs - 1] = out;
        }
    }

    for (i = 0; i < numStub; i++)
    {
        relocs[i].numRelocs++;
        if (relocs[i].numRelocs == 1)
            relocs[i].relocs = malloc(sizeof(int) * 2);
        else
            relocs[i].relocs = realloc(relocs[i].relocs, sizeof(int) * (relocs[i].numRelocs + 1));
        relocs[i].relocs[relocs[i].numRelocs - 1] = 0;
    }

    // add the special relocations data
    int startAddr = g_vstub->iAddr + g_vstub->iSize;
    int addr = startAddr;
    for (i = 0; i < numStub; i++)
    {
        relocs[i].addr = addr;
        addr += relocs[i].numRelocs * 4;
    }
    int shift = addr - startAddr;
    g_elfsize += shift;
    g_elfdata = realloc(g_elfdata, g_elfsize);
    u8 *start = g_elfdata + g_vstub->iOffset + g_vstub->iSize;
    memmove(start + shift, start, g_elfsize - (g_vstub->iOffset + g_vstub->iSize + shift));
    int *cur = (int*)(g_elfdata + g_vstub->iOffset + g_vstub->iSize);
    for (i = 0; i < numStub; i++) {
        memcpy(cur, relocs[i].relocs, relocs[i].numRelocs * 4);
        cur += relocs[i].numRelocs;
    }

    if (((Elf32_Ehdr *)g_elfdata)->e_shoff > g_vstub->iOffset + g_vstub->iSize)
        ((Elf32_Ehdr *)g_elfdata)->e_shoff += shift;
    if (((Elf32_Ehdr *)g_elfdata)->e_phoff > g_vstub->iOffset + g_vstub->iSize)
        ((Elf32_Ehdr *)g_elfdata)->e_phoff += shift;
    for (i = 0; i < (int)g_elfhead.iShnum; i++)
    {
        Elf32_Shdr *sect = (Elf32_Shdr *) (g_elfdata + ((Elf32_Ehdr *)g_elfdata)->e_shoff + (i * g_elfhead.iShentsize));
        if (sect->sh_addr > g_vstub->iAddr)
            sect->sh_addr += shift;
        if (sect->sh_offset > g_vstub->iOffset)
            sect->sh_offset += shift;
        if (sect->sh_offset == g_vstub->iOffset)
            sect->sh_size += shift;
        if (sect->sh_type == SHT_REL)
        {
            int relCount = sect->sh_size / sizeof(Elf32_Rel);
            int j;
            printf("Loading relocations %s\n", g_elfsections[i].szName);
   
            // check the relocations pointing to the imported variable
            for(j = 0; j < relCount; j++)
            {  
                Elf32_Rel *rel = &((Elf32_Rel*)(g_elfdata + sect->sh_offset))[j];
                if (rel->r_offset > g_vstub->iOffset)
                    rel->r_offset += shift;

                u32 dwRealOfs;
                u32 dwCurrBase;
                int iOfsPH;
                int iValPH;
                int type = ELF32_R_TYPE(LW(rel->r_info));
                int symbol = ELF32_R_SYM(LW(rel->r_info));
                int offset = rel->r_offset;
                Elf32_Phdr *seg = (Elf32_Phdr*)(g_elfdata + g_elfhead.iPhoff);
                u32 *pData;
                int lastJ = j;

                iOfsPH = symbol & 0xFF;
                iValPH = (symbol >> 8) & 0xFF;
                dwRealOfs = offset + LW(seg[iOfsPH].p_vaddr);
                dwCurrBase = LW(seg[iValPH].p_vaddr);
                pData = (u32*)(g_elfdata + g_text->iOffset + dwRealOfs);
                if (strcmp(g_elfsections[i].szName, ".rel.rodata.sceModuleInfo") == 0 && j == 0) // disable _gp relocation
                    continue;

                switch (type)
                {
                case R_MIPS_HI16: {
                    int addr = ((LW(*pData) & 0xFFFF) << 16) + dwCurrBase;
                    int loinst;
                    while (++j < relCount) {
                        if (ELF32_R_TYPE(LW(((Elf32_Rel*)g_textrel->pData)[j].r_info)) != R_MIPS_HI16) break;
                    }
                    if (j < relCount) {
                        loinst = LW(*(u32*)(g_elfdata + g_text->iOffset + ((Elf32_Rel*)g_textrel->pData)[j].r_offset + LW(seg[iOfsPH].p_vaddr)));
                    } else {
                        loinst = 0;
                    }
                    addr = (s32) addr + (s16) (loinst & 0xFFFF);
                    if ((u32)addr > g_vstub->iAddr)
                        SW(pData, (LW(*pData) & 0xFFFF0000) | ((addr - dwCurrBase + shift) >> 16));
                    }
                    break;
                case R_MIPS_16:
                case R_MIPS_LO16: {
                    u32 addr = (s16)(LW(*pData) & 0xFFFF) + dwCurrBase;
                    if (addr > g_vstub->iAddr)
                        SW(pData, (LW(*pData) & 0xFFFF0000) | ((addr - dwCurrBase + shift) & 0xFFFF));
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
                    if (addr > g_vstub->iAddr)
                        SW(pData, (LW(*pData) & 0xFC000000) | (((addr - dwCurrBase) >> 2) + shift));
                    }
                    break;
                case R_MIPS_REL32:
                case R_MIPS_32: {
                    u32 addr = ((LW(*pData) + (dwCurrBase & 0x03FFFFFF)) & 0x03FFFFFF);
                    if (addr > g_vstub->iAddr)
                        SW(pData, (LW(*pData) & 0xFC000000) | ((LW(*pData) & 0x03FFFFFF) + shift));
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
    }
    for (i = 0; i < (int)g_elfhead.iPhnum; i++)
    {
        Elf32_Phdr *seg = (Elf32_Phdr *) (g_elfdata + ((Elf32_Ehdr *)g_elfdata)->e_phoff + (i * g_elfhead.iPhentsize));
        if (seg->p_offset > g_vstub->iOffset + g_vstub->iSize)
            seg->p_offset += shift;
        if (seg->p_vaddr > g_vstub->iAddr + g_vstub->iSize)
            seg->p_vaddr += shift;
        if (seg->p_paddr > g_vstub->iAddr + g_vstub->iSize)
            seg->p_paddr += shift;
    }

    free(g_elfsections);
    validate_header(g_elfdata);
    load_sections();
    pText = (unsigned int *) g_vstub->pData;
    for (i = 0; i < numStub; i++)
    {       
        unsigned int stub_addr;
        unsigned int stub_nid;

        stub_addr = LW(pText[0]);
        stub_nid  = LW(pText[1]);
            
        struct PspModuleImport *pImport;
        u8     var_count;
            
        pImport = (struct PspModuleImport *) (g_libstub->pData + (stub_addr - g_libstub->iAddr));
        var_count = LH(pImport->var_count);
           
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
        SH(&pImport->var_count, var_count);
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

	while(size >= sizeof(struct PspModuleImport))
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

		pImport++;
		size -= sizeof(struct PspModuleImport);
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
