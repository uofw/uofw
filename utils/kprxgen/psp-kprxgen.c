/*
 * PSP Software Development Kit - http://www.pspdev.org
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in PSPSDK root for details.
 *
 * psp-prxgen.c - Simple program to build a PRX file (and strip at the same time)
 *
 * Copyright (c) 2005 James Forshaw <tyranid@gmail.com>
 *
 * $Id: psp-prxgen.c 1520 2005-12-04 20:09:36Z tyranid $
 */

#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <ctype.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../common/types.h"
#include "../common/elftypes.h"
#include "../common/prxtypes.h"

static const char *g_outfile;
static const char *g_infile;
static unsigned char *g_elfdata = NULL;
static struct ElfHeader g_elfhead = {0};
static struct ElfSection *g_elfsections = NULL;
static struct ElfSection *g_modinfo = NULL;

/* Specifies that the current usage is to the print the pspsdk path */
static int g_verbose = 0;

static struct option arg_opts[] = 
{
	{"verbose", no_argument, NULL, 'v'},
	{ NULL, 0, NULL, 0 }
};

/* Process the arguments */
int process_args(int argc, char **argv)
{
	int ch;

	g_outfile = NULL;
	g_infile = NULL;

	ch = getopt_long(argc, argv, "v", arg_opts, NULL);
	while(ch != -1)
	{
		switch(ch)
		{
			case 'v' : g_verbose = 1;
					   break;
			default  : break;
		};

		ch = getopt_long(argc, argv, "v", arg_opts, NULL);
	}

	argc -= optind;
	argv += optind;

	if(argc < 2)
	{
		return 0;
	}

	g_infile = argv[0];
	g_outfile = argv[1];

	if(g_verbose)
	{
		fprintf(stderr, "Loading %s, outputting to %s\n", g_infile, g_outfile);
	}

	return 1;
}

void print_help(void)
{
	fprintf(stderr, "Usage: psp-prxgen [-v] infile.elf outfile.prx\n");
	fprintf(stderr, "Outputs the layout of the original modules (no section headers, one program header\n");
	fprintf(stderr, "per segment, 0x700000A1 relocations).\n");
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "-v, --verbose           : Verbose output\n");
}

unsigned char *load_file(const char *file)
{
	FILE *fp;
	unsigned int size;
	unsigned char *data = NULL;

	do
	{
		fp = fopen(file, "rb");
		if(fp != NULL)
		{
			(void) fseek(fp, 0, SEEK_END);
			size = ftell(fp);
			rewind(fp);

			if(size < sizeof(Elf32_Ehdr))
			{
				fprintf(stderr, "Error, invalid file size\n");
				break;
			}

			data = (unsigned char *) malloc(size);
			if(data == NULL)
			{
				fprintf(stderr, "Error, could not allocate memory for ELF\n");
				break;
			}

			(void) fread(data, 1, size, fp);
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
int load_sections(unsigned char *data)
{
	int ret = 0;
	int found_rel = 0;
	unsigned int load_addr = 0xFFFFFFFF;

	if(g_elfhead.iShnum > 0)
	{
		do
		{
			Elf32_Shdr *sect;
			int i;

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
					g_elfsections[i].blOutput = 1;
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
				else if(strcmp(g_elfsections[i].szName, PSP_MODULE_REMOVE_REL) == 0)
				{
					/* Don't output .rel.lib.stub relocations */
					g_elfsections[i].blOutput = 0;
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

			if(!found_rel)
			{
				fprintf(stderr, "Error, found no relocation sections\n");
				break;
			}

			if(load_addr != 0)
			{
				fprintf(stderr, "Error, ELF not loaded to address 0 (%08X)\n", load_addr);
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

int remove_weak_relocs(struct ElfSection *pReloc, struct ElfSection *pSymbol, struct ElfSection *pString)
{
	int iCount;
	int iMaxSymbol;
	void *pNewRel = NULL;
	Elf32_Rel *pInRel;
	Elf32_Rel *pOutRel;
	Elf32_Sym *pSymData = (Elf32_Sym *) pSymbol->pData;
	char *pStrData = NULL;
	int iOutput;
	int i;

	if(pString != NULL)
	{
		pStrData = (char *) pString->pData;
	}

	iMaxSymbol = pSymbol->iSize / sizeof(Elf32_Sym);
	iCount = pReloc->iSize / sizeof(Elf32_Rel);

	pNewRel = malloc(pReloc->iSize);
	if(pNewRel == NULL)
	{
		return 0;
	}
	pOutRel = (Elf32_Rel *) pNewRel;
	pInRel = (Elf32_Rel *) pReloc->pData;
	iOutput = 0;

	if(g_verbose)
	{
		fprintf(stderr, "[%s] Processing %d relocations, %d symbols\n", pReloc->szName, iCount, iMaxSymbol);
	}

	for(i = 0; i < iCount; i++)
	{
		int iSymbol;

		iSymbol = ELF32_R_SYM(LW(pInRel->r_info));
		if(g_verbose)
		{
			fprintf(stderr, "Relocation %d - Symbol %x\n", iOutput, iSymbol);
		}

		if(iSymbol >= iMaxSymbol)
		{
			fprintf(stderr, "Warning: Ignoring relocation as cannot find matching symbol\n");
		}
		else
		{
			if(g_verbose)
			{
				if(pStrData != NULL)
				{
					fprintf(stderr, "Symbol %d - Name %s info %x ndx %x\n", iSymbol, &pStrData[pSymData[iSymbol].st_name], 
							pSymData[iSymbol].st_info, pSymData[iSymbol].st_shndx);
				}
				else
				{
					fprintf(stderr, "Symbol %d - Name %d info %x ndx %x\n", iSymbol, pSymData[iSymbol].st_name, 
							pSymData[iSymbol].st_info, pSymData[iSymbol].st_shndx);
				}
			}

			/* Remove PC16 relocations (unsupported by PSP, and useless) */
			if(LH(pSymData[iSymbol].st_shndx) == 0 || ELF32_R_TYPE(LW(pInRel->r_info)) == R_MIPS_PC16)
			{
				if(g_verbose)
				{
					fprintf(stderr, "Deleting relocation\n");
				}
			}
			else
			{
				/* We are keeping this relocation, copy it across */
				*pOutRel = *pInRel;
				pOutRel++;
				iOutput++;
			}
		}

		pInRel++;
	}

	/* If we deleted some relocations */
	if(iOutput < iCount)
	{
		int iSize;

		iSize = iOutput * sizeof(Elf32_Rel);
		if(g_verbose)
		{
			fprintf(stderr, "Old relocation size %d, new %d\n", pReloc->iSize, iSize);
		}
		pReloc->iSize = iSize;
		/* If size is zero then delete this section */
		if(iSize == 0)
		{
			pReloc->blOutput = 0;
		}
		else
		{
			/* Copy across the new relocation data */
			memcpy(pReloc->pData, pNewRel, pReloc->iSize);
		}
	}

	free(pNewRel);

	return 1;
}

/* Let's remove the weak relocations from the list */
int process_relocs(void)
{
	int i;

	for(i = 0; i < g_elfhead.iShnum; i++)
	{
		if((g_elfsections[i].blOutput) && (g_elfsections[i].iType == SHT_REL))
		{
			struct ElfSection *pReloc;

			pReloc = &g_elfsections[i];
			if((pReloc->iLink < g_elfhead.iShnum) && (g_elfsections[pReloc->iLink].iType == SHT_SYMTAB))
			{
				struct ElfSection *pStrings = NULL;
				struct ElfSection *pSymbols;

				pSymbols = &g_elfsections[pReloc->iLink];
				if((pSymbols->iLink < g_elfhead.iShnum) && (g_elfsections[pSymbols->iLink].iType == SHT_STRTAB))
				{
					pStrings = &g_elfsections[pSymbols->iLink];
				}

				if(!remove_weak_relocs(pReloc, pSymbols, pStrings))
				{
					return 0;
				}
			}
			else
			{
				if(g_verbose)
				{
					fprintf(stderr, "Ignoring relocation section %d, invalid link number\n", i);
				}
			}
		}
	}

	return 1;
}

/* Load an ELF file */
int load_elf(const char *elf)
{
	int ret = 0;

	do
	{
		g_elfdata = load_file(elf);
		if(g_elfdata == NULL)
		{
			break;
		}

		if(!validate_header(g_elfdata))
		{
			break;
		}

		if(!load_sections(g_elfdata))
		{
			break;
		}

		if(!process_relocs())
		{
			break;
		}

		ret = 1;
	}
	while(0);

	return ret;
}

/*
 * Output the layout of the original PRX files, decoded from the firmware modules:
 *
 *  - one program header per PT_LOAD segment of the ELF (linked with a script with PHDRS, see
 *    lib/linkfile.prx), then the relocations (type 0x700000A1), and no section headers;
 *  - the first segment at the size of the headers aligned on its alignment (0xA0 or 0xC0), each following
 *    segment at the end of the previous one aligned on its alignment, the relocations at the end of the last
 *    segment aligned on 16 bytes;
 *  - the relocations in the compressed 0x700000A1 format (below).
 */

#define SCE_MAX_SEGS     (4)

struct SceReloc
{
	u32 offseg;   /* segment of the relocated word */
	u32 addr;     /* address of the relocated word, relative to its segment */
	u32 type;     /* 2: 32-bit, 3: 26-bit (data), 4: HI16, 5: LO16, 6: J, 7: JAL */
	u32 addrseg;  /* segment of the target */
	s16 lo16;     /* HI16: the low 16 bits of the target (with the sign of a LO16) */
	int patch;    /* the relocated word is rewritten (target in a segment other than the first) */
	u32 word;     /* the rewritten word */
};

static Elf32_Phdr g_segs[SCE_MAX_SEGS];
static int g_nsegs = 0;
static struct SceReloc *g_scerels = NULL;
static int g_nscerels = 0;

static int sce_seg_of_addr(u32 addr)
{
	int i;
	int seg = 0;

	for(i = 0; i < g_nsegs; i++)
	{
		if(addr >= g_segs[i].p_vaddr)
		{
			seg = i;
		}
	}

	return seg;
}

/* The word at address addr in the segments of the ELF (0 outside of the file data) */
static u32 sce_word(u32 addr)
{
	int i;

	for(i = 0; i < g_nsegs; i++)
	{
		if(addr >= g_segs[i].p_vaddr && addr + 4 <= g_segs[i].p_vaddr + g_segs[i].p_filesz)
		{
			return LW(*(u32 *) (g_elfdata + g_segs[i].p_offset + addr - g_segs[i].p_vaddr));
		}
	}

	return 0;
}

/* Add the relocation rel of section pReloc (whose relocated section is sect) to g_scerels */
static int sce_add_reloc(struct ElfSection *pReloc, Elf32_Rel *rels, int count, int i)
{
	Elf32_Sym *syms = (Elf32_Sym *) g_elfsections[pReloc->iLink].pData;
	u32 info = LW(rels[i].r_info);
	u32 addr = LW(rels[i].r_offset);
	struct SceReloc *r = &g_scerels[g_nscerels];
	u32 symval;
	int j;

	symval = LW(syms[ELF32_R_SYM(info)].st_value);
	r->offseg = sce_seg_of_addr(addr);
	r->addr = addr - g_segs[r->offseg].p_vaddr;
	r->addrseg = sce_seg_of_addr(symval);
	r->lo16 = 0;

	switch(ELF32_R_TYPE(info))
	{
		case R_MIPS_32:
			r->type = 2;
			break;
		case R_MIPS_26:
			/* the type of the jumps is the one of the instruction */
			switch(sce_word(addr) >> 26)
			{
				case 2:  r->type = 6; break;
				case 3:  r->type = 7; break;
				default: r->type = 3; break;
			}
			break;
		case R_MIPS_HI16:
			r->type = 4;
			/* the low part of the target is the immediate of the matching LO16 (same symbol) */
			for(j = i + 1; j < count; j++)
			{
				if((ELF32_R_TYPE(LW(rels[j].r_info)) == R_MIPS_LO16) && (ELF32_R_SYM(LW(rels[j].r_info)) == ELF32_R_SYM(info)))
				{
					r->lo16 = (s16) (sce_word(LW(rels[j].r_offset)) & 0xFFFF);
					break;
				}
			}
			if(j == count)
			{
				fprintf(stderr, "Error, HI16 relocation at %08X without LO16\n", addr);
				return 0;
			}
			break;
		case R_MIPS_LO16:
			r->type = 5;
			break;
		default:
			fprintf(stderr, "Error, unsupported relocation type %d at %08X\n", ELF32_R_TYPE(info), addr);
			return 0;
	}

	g_nscerels++;
	return 1;
}

static u32 g_libstub_addr;
static u8 *g_libstub_data;
static u32 g_libstub_size;

/*
 * Order of the .lib.stub relocations: the library names in the order of the entries, then the stub and NID
 * table pointers by address of the stub table, then the variable stub table pointers by address.
 */
static u32 sce_stub_key(u32 addr)
{
	u32 ent = 0;
	u32 ofs;
	u32 size;

	for(;;)
	{
		size = g_libstub_data[ent + 8] * 4;
		if((addr - g_libstub_addr) < ent + size || size == 0)
		{
			break;
		}
		ent += size;
	}
	ofs = addr - g_libstub_addr - ent;

	if(ofs == 0)
	{
		return ent;
	}
	if(ofs == 12 || ofs == 16)
	{
		/* stub table first, then NID table */
		return 0x40000000 + LW(*(u32 *) (g_libstub_data + ent + 16)) * 2 + (ofs == 12);
	}
	return 0x80000000 + LW(*(u32 *) (g_libstub_data + ent + ofs));
}

static int sce_cmp_stub(const void *a, const void *b)
{
	u32 ka = sce_stub_key(((const struct SceReloc *) a)->addr + g_segs[((const struct SceReloc *) a)->offseg].p_vaddr);
	u32 kb = sce_stub_key(((const struct SceReloc *) b)->addr + g_segs[((const struct SceReloc *) b)->offseg].p_vaddr);

	return (ka > kb) - (ka < kb);
}

/*
 * The relocations of the variable stub table (.rodata.sceVstub) are in the order of the import entries in
 * .lib.stub owning the variables (and of the variables in each entry), like in all the 6.60 modules.
 */
static u32 sce_vstub_key(u32 addr)
{
	u32 ent;

	for(ent = 0; ent + 24 <= g_libstub_size && g_libstub_data[ent + 8] != 0; ent += g_libstub_data[ent + 8] * 4)
	{
		u32 vars;
		u32 count;

		if(g_libstub_data[ent + 8] < 6)
		{
			continue;
		}
		vars = LW(*(u32 *) (g_libstub_data + ent + 20));
		count = g_libstub_data[ent + 9];
		if(addr >= vars && addr < vars + count * 8)
		{
			return (ent << 16) + (addr - vars) / 8;
		}
	}

	return 0xFFFFFFFF;
}

static int sce_cmp_vstub(const void *a, const void *b)
{
	u32 ka = sce_vstub_key(((const struct SceReloc *) a)->addr + g_segs[((const struct SceReloc *) a)->offseg].p_vaddr);
	u32 kb = sce_vstub_key(((const struct SceReloc *) b)->addr + g_segs[((const struct SceReloc *) b)->offseg].p_vaddr);

	return (ka > kb) - (ka < kb);
}

/* Whether the relocation j of the relocation section pReloc must be output (not a removed or a null one) */
static int sce_keep_reloc(struct ElfSection *pReloc, int j)
{
	Elf32_Rel *rels = (Elf32_Rel *) pReloc->pData;
	Elf32_Sym *syms = (Elf32_Sym *) g_elfsections[pReloc->iLink].pData;
	u32 info = LW(rels[j].r_info);

	return ELF32_R_TYPE(info) != R_MIPS_NONE && ELF32_R_TYPE(info) != R_MIPS_PC16
		&& LH(syms[ELF32_R_SYM(info)].st_shndx) != 0;
}

/*
 * The variable table pointer (word 5) of a stub library entry which imports no variable: 0 and not relocated
 * in the original modules (psp-fixup-imports leaves it relocated against the start of the module)
 */
static int sce_null_varptr(u32 addr)
{
	u32 ofs = 0;

	if(g_libstub_data == NULL || addr < g_libstub_addr || addr >= g_libstub_addr + g_libstub_size)
	{
		return 0;
	}
	while(ofs + 12 <= g_libstub_size)
	{
		u8 len = g_libstub_data[ofs + 8];

		if(len == 0)
		{
			break;
		}
		if(addr == g_libstub_addr + ofs + 20)
		{
			return len >= 6 && g_libstub_data[ofs + 9] == 0 && sce_word(addr) == 0;
		}
		ofs += len * 4;
	}

	return 0;
}

static int sce_in_section(struct ElfSection *sect, u32 addr)
{
	return addr >= sect->iAddr && addr < sect->iAddr + sect->iSize;
}

/*
 * Collect the relocations of the ELF: section by section in the order of the relocation sections, each in
 * the order of the relocations. The relocations which psp-fixup-imports moved to the relocation tables of the
 * imported variables (in .rodata.sceVstub) are output with the relocations of that section, after them.
 */
static int sce_collect_relocs(void)
{
	int total = 0;
	int i, j, k;

	for(i = 0; i < g_elfhead.iShnum; i++)
	{
		if(g_elfsections[i].iType == SHT_REL && g_elfsections[i].pRef != NULL)
		{
			total += g_elfsections[i].iSize / sizeof(Elf32_Rel);
		}
	}

	for(i = 0; i < g_elfhead.iShnum; i++)
	{
		if(strcmp(g_elfsections[i].szName, ".lib.stub") == 0)
		{
			g_libstub_addr = g_elfsections[i].iAddr;
			g_libstub_data = g_elfsections[i].pData;
			g_libstub_size = g_elfsections[i].iSize;
		}
	}

	g_scerels = (struct SceReloc *) malloc(sizeof(struct SceReloc) * (total + 1));
	if(g_scerels == NULL)
	{
		return 0;
	}

	for(i = 0; i < g_elfhead.iShnum; i++)
	{
		struct ElfSection *pReloc = &g_elfsections[i];
		Elf32_Rel *rels;
		int count;
		int first = g_nscerels;

		/* the sections not output: the relocations of the stubs (.rel.sceStub.text) and empty ones */
		if(pReloc->iType != SHT_REL || pReloc->pRef == NULL || pReloc->blOutput == 0)
		{
			continue;
		}

		rels = (Elf32_Rel *) pReloc->pData;
		count = pReloc->iSize / sizeof(Elf32_Rel);
		for(j = 0; j < count; j++)
		{
			if(sce_keep_reloc(pReloc, j) && sce_in_section(pReloc->pRef, LW(rels[j].r_offset))
					&& !sce_null_varptr(LW(rels[j].r_offset)))
			{
				if(!sce_add_reloc(pReloc, rels, count, j))
				{
					return 0;
				}
			}
		}

		if(strcmp(pReloc->szName, ".rel.rodata.sceVstub") == 0 && g_libstub_data != NULL)
		{
			qsort(&g_scerels[first], g_nscerels - first, sizeof(struct SceReloc), sce_cmp_vstub);
		}

		/* the relocations of other relocation sections in this section */
		for(k = 0; k < g_elfhead.iShnum; k++)
		{
			struct ElfSection *pOther = &g_elfsections[k];
			Elf32_Rel *orels;
			int ocount;

			if(k == i || pOther->iType != SHT_REL || pOther->pRef == NULL || pOther->blOutput == 0)
			{
				continue;
			}
			orels = (Elf32_Rel *) pOther->pData;
			ocount = pOther->iSize / sizeof(Elf32_Rel);
			for(j = 0; j < ocount; j++)
			{
				u32 addr = LW(orels[j].r_offset);

				if(sce_keep_reloc(pOther, j) && !sce_in_section(pOther->pRef, addr) && sce_in_section(pReloc->pRef, addr))
				{
					if(!sce_add_reloc(pOther, orels, ocount, j))
					{
						return 0;
					}
				}
			}
		}

		if(strcmp(pReloc->szName, ".rel.lib.stub") == 0 && g_libstub_data != NULL)
		{
			qsort(&g_scerels[first], g_nscerels - first, sizeof(struct SceReloc), sce_cmp_stub);
		}
	}

	return 1;
}

/*
 * The 0x700000A1 relocation format:
 *
 *  u8 0, u8 0, u8 flagBits, u8 typeBits
 *  u8 flagTableSize, u8 flags[]: the command kinds, in the order of their first use, padded to an even size
 *  u8 typeTableSize, u8 types[]: the relocation types, in the order of their first use, padded to an even size
 *  u16 commands: [flag index (flagBits) | segment (segBits) | type index (typeBits) | payload], index 0 unused
 *
 * Flags: bit 0 set: a relocation (else a base command: sets the segment of the relocated words and the
 * address of the first relocation, 0x00 if it fits in the payload, else 0x04 + u32); bit 1: the address
 * delta is in a following u16 (the payload is its high part) instead of the payload; bits 3-4: 0x00 no
 * low part (J/JAL), 0x08 the low part of the previous HI16 (or none), 0x10 the low part follows as s16.
 */
struct SceCmd
{
	u8 flag;
	u8 seg;
	u8 type;      /* 0xFF: base command */
	u32 payload;
	u8 extra[8];
	int nextra;
};

static int sce_bits(int n)
{
	int bits = 1;

	while((1 << bits) < n)
	{
		bits++;
	}

	return bits;
}

static int sce_index(u8 *table, int *size, u8 v)
{
	int i;

	for(i = 0; i < *size; i++)
	{
		if(table[i] == v)
		{
			return i;
		}
	}
	table[(*size)++] = v;
	return i;
}

static int sce_encode(unsigned char *out, int flagBits, int typeBits, int *outFlagBits, int *outTypeBits)
{
	int segBits = sce_bits(g_nsegs);
	int sh = flagBits + segBits + typeBits;
	u8 flags[256], types[256];
	int nflags = 0, ntypes = 0;
	struct SceCmd *cmds;
	int ncmds = 0;
	u32 offseg = 0xFFFFFFFF;
	u32 base = 0;
	u32 lastType = 0xFFFFFFFF;
	s16 lastLo = 0;
	int pos;
	int i;

	cmds = (struct SceCmd *) malloc(sizeof(struct SceCmd) * (g_nscerels * 2 + 1));
	for(i = 0; i < g_nscerels; i++)
	{
		struct SceReloc *r = &g_scerels[i];
		struct SceCmd *c;
		s32 delta;
		u8 lof;
		int ext = 0;

		if(r->offseg != offseg)
		{
			c = &cmds[ncmds++];
			memset(c, 0, sizeof(*c));
			c->seg = r->offseg;
			c->type = 0xFF;
			if(r->addr < (1u << (16 - segBits - flagBits)))
			{
				c->flag = 0x00;
				c->payload = r->addr;
			}
			else
			{
				c->flag = 0x04;
				SW((u32 *) c->extra, r->addr);
				c->nextra = 4;
			}
			offseg = r->offseg;
			base = r->addr;
		}

		c = &cmds[ncmds++];
		memset(c, 0, sizeof(*c));
		delta = (s32) (r->addr - base);
		if(r->type == 4)
		{
			if(lastType == 4 && r->lo16 == lastLo)
			{
				lof = 0x08;
			}
			else
			{
				lof = 0x10;
				ext = 1;
			}
		}
		else if(r->type == 6 || r->type == 7)
		{
			lof = 0x00;
		}
		else
		{
			lof = 0x08;
		}

		c->seg = r->addrseg;
		c->type = r->type;
		if(delta >= -(1 << (16 - sh - 1)) && delta < (1 << (16 - sh - 1)))
		{
			c->flag = 0x01 | lof;
			c->payload = (u32) delta;
		}
		else
		{
			c->flag = 0x03 | lof;
			c->payload = (u32) delta >> 16;
			SH((u16 *) c->extra, (u32) delta & 0xFFFF);
			c->nextra = 2;
		}
		if(ext)
		{
			SH((u16 *) (c->extra + c->nextra), (u16) r->lo16);
			c->nextra += 2;
		}

		base = r->addr;
		lastType = r->type;
		if(r->type == 4)
		{
			lastLo = r->lo16;
		}
	}

	for(i = 0; i < ncmds; i++)
	{
		sce_index(flags, &nflags, cmds[i].flag);
		if(cmds[i].type != 0xFF)
		{
			sce_index(types, &ntypes, cmds[i].type);
		}
	}

	*outFlagBits = sce_bits(nflags + 1);
	*outTypeBits = sce_bits(ntypes + 1);

	pos = 0;
	if(out != NULL)
	{
		out[0] = 0;
		out[1] = 0;
		out[2] = flagBits;
		out[3] = typeBits;
	}
	pos = 4;
	/* the tables (size byte included) have an even size */
	if(out != NULL)
	{
		out[pos] = (nflags + 1 + 1) & ~1;
		memcpy(&out[pos + 1], flags, nflags);
		memset(&out[pos + 1 + nflags], 0, out[pos] - 1 - nflags);
	}
	pos += (nflags + 1 + 1) & ~1;
	if(out != NULL)
	{
		out[pos] = (ntypes + 1 + 1) & ~1;
		memcpy(&out[pos + 1], types, ntypes);
		memset(&out[pos + 1 + ntypes], 0, out[pos] - 1 - ntypes);
	}
	pos += (ntypes + 1 + 1) & ~1;

	for(i = 0; i < ncmds; i++)
	{
		struct SceCmd *c = &cmds[i];
		u32 cmd;

		cmd = (sce_index(flags, &nflags, c->flag) + 1) | (c->seg << flagBits);
		if(c->type == 0xFF)
		{
			cmd |= (c->payload & ((1 << (16 - segBits - flagBits)) - 1)) << (segBits + flagBits);
		}
		else
		{
			cmd |= (sce_index(types, &ntypes, c->type) + 1) << (flagBits + segBits);
			cmd |= (c->payload & ((1 << (16 - sh)) - 1)) << sh;
		}
		if(out != NULL)
		{
			SH((u16 *) &out[pos], cmd);
			memcpy(&out[pos + 2], c->extra, c->nextra);
		}
		pos += 2 + c->nextra;
	}

	free(cmds);
	return pos;
}

/* Output a PRX file in the layout of the original modules */
int output_prx(const char *prxfile)
{
	Elf32_Ehdr *head;
	Elf32_Phdr *phdr;
	struct PspModuleInfo *pModinfo;
	unsigned char *data;
	u32 offsets[SCE_MAX_SEGS];
	u32 ofs, relofs, size;
	int flagBits = 3, typeBits = 3;
	int newFlagBits, newTypeBits;
	int relsize;
	int i, j;
	FILE *fp;

	/* The PT_LOAD segments of the ELF */
	for(i = 0; i < g_elfhead.iPhnum; i++)
	{
		Elf32_Phdr *in = (Elf32_Phdr *) (g_elfdata + g_elfhead.iPhoff + i * g_elfhead.iPhentsize);

		if(LW(in->p_type) != 1)
		{
			continue;
		}
		if(g_nsegs == SCE_MAX_SEGS)
		{
			fprintf(stderr, "Error, too many segments\n");
			return 1;
		}
		g_segs[g_nsegs].p_type = 1;
		g_segs[g_nsegs].p_offset = LW(in->p_offset);
		g_segs[g_nsegs].p_vaddr = LW(in->p_vaddr);
		g_segs[g_nsegs].p_filesz = LW(in->p_filesz);
		g_segs[g_nsegs].p_memsz = LW(in->p_memsz);
		g_segs[g_nsegs].p_flags = LW(in->p_flags);
		/* the largest alignment of its sections (at least 16 bytes), not the page size of the linker */
		g_segs[g_nsegs].p_align = 0x10;
		for(j = 0; j < g_elfhead.iShnum; j++)
		{
			struct ElfSection *sect = &g_elfsections[j];

			if((sect->iFlags & SHF_ALLOC) && sect->iSize != 0 && sect->iAddr >= g_segs[g_nsegs].p_vaddr
					&& sect->iAddr < g_segs[g_nsegs].p_vaddr + g_segs[g_nsegs].p_memsz
					&& sect->iAddralign > g_segs[g_nsegs].p_align)
			{
				g_segs[g_nsegs].p_align = sect->iAddralign;
			}
		}
		g_nsegs++;
	}

	if(!sce_collect_relocs())
	{
		return 1;
	}

	/*
	 * The relocated words address their target relative to the start of the target's segment (the
	 * original modules: e.g. lui 0 / addiu 0x40 for the .bss variable at the start of segment 1 + 0x40),
	 * the ELF has the absolute addresses: rewrite the words (and the low parts of the HI16 relocations)
	 * which target a segment other than the first.
	 */
	for(i = 0; i < g_nscerels; i++)
	{
		struct SceReloc *r = &g_scerels[i];
		u32 base = g_segs[r->addrseg].p_vaddr;
		u32 w = sce_word(r->addr + g_segs[r->offseg].p_vaddr);
		u32 target;

		r->patch = 0;
		if(r->addrseg == 0 || base == 0)
		{
			continue;
		}
		switch(r->type)
		{
			case 2:
				r->word = w - base;
				break;
			case 4:
				target = (w << 16) + (s32) r->lo16 - base;
				r->word = (w & 0xFFFF0000) | (((target + 0x8000) >> 16) & 0xFFFF);
				r->lo16 = (s16) (target & 0xFFFF);
				break;
			case 5:
				r->word = (w & 0xFFFF0000) | ((w - base) & 0xFFFF);
				break;
			default:
				continue;
		}
		r->patch = 1;
	}

	/* The sizes of the flag and type indexes depend on the number of used flags and types */
	do
	{
		relsize = sce_encode(NULL, flagBits, typeBits, &newFlagBits, &newTypeBits);
		if(newFlagBits == flagBits && newTypeBits == typeBits)
		{
			break;
		}
		flagBits = newFlagBits;
		typeBits = newTypeBits;
	}
	while(1);

	ofs = sizeof(Elf32_Ehdr) + (g_nsegs + 1) * sizeof(Elf32_Phdr);
	for(i = 0; i < g_nsegs; i++)
	{
		ofs = (ofs + g_segs[i].p_align - 1) & ~(g_segs[i].p_align - 1);
		offsets[i] = ofs;
		ofs += g_segs[i].p_filesz;
	}
	relofs = (ofs + 0xF) & ~0xF;
	size = relofs + relsize;

	data = (unsigned char *) malloc(size);
	if(data == NULL)
	{
		fprintf(stderr, "Error, couldn't allocate output data\n");
		return 1;
	}
	memset(data, 0, size);

	head = (Elf32_Ehdr *) data;
	SW(&head->e_magic, g_elfhead.iMagic);
	head->e_class = g_elfhead.iClass;
	head->e_data = g_elfhead.iData;
	head->e_idver = g_elfhead.iIdver;
	SH(&head->e_type, ELF_PRX_TYPE);
	SH(&head->e_machine, g_elfhead.iMachine);
	SW(&head->e_version, g_elfhead.iVersion);
	SW(&head->e_entry, g_elfhead.iEntry);
	SW(&head->e_phoff, sizeof(Elf32_Ehdr));
	SW(&head->e_shoff, 0);
	SW(&head->e_flags, g_elfhead.iFlags);
	SH(&head->e_ehsize, sizeof(Elf32_Ehdr));
	SH(&head->e_phentsize, sizeof(Elf32_Phdr));
	SH(&head->e_phnum, g_nsegs + 1);
	SH(&head->e_shentsize, 0);
	SH(&head->e_shnum, 0);
	SH(&head->e_shstrndx, 0);

	pModinfo = (struct PspModuleInfo *) (g_modinfo->pData);
	phdr = (Elf32_Phdr *) (data + sizeof(Elf32_Ehdr));
	for(i = 0; i < g_nsegs; i++, phdr++)
	{
		SW(&phdr->p_type, 1);
		SW(&phdr->p_offset, offsets[i]);
		SW(&phdr->p_vaddr, g_segs[i].p_vaddr);
		/* the first segment: the file offset of the module info (kernel modules: with the top bit set) */
		if(i == 0)
		{
			SW(&phdr->p_paddr, ((LW(pModinfo->flags) & 0x1000) ? 0x80000000 : 0) | (g_modinfo->iAddr + offsets[0]));
		}
		else
		{
			SW(&phdr->p_paddr, 0);
		}
		SW(&phdr->p_filesz, g_segs[i].p_filesz);
		SW(&phdr->p_memsz, g_segs[i].p_memsz);
		SW(&phdr->p_flags, g_segs[i].p_flags);
		SW(&phdr->p_align, g_segs[i].p_align);
		memcpy(data + offsets[i], g_elfdata + g_segs[i].p_offset, g_segs[i].p_filesz);
	}
	for(i = 0; i < g_nscerels; i++)
	{
		if(g_scerels[i].patch)
		{
			SW((u32 *) (data + offsets[g_scerels[i].offseg] + g_scerels[i].addr), g_scerels[i].word);
		}
	}
	SW(&phdr->p_type, 0x700000A1);
	SW(&phdr->p_offset, relofs);
	SW(&phdr->p_vaddr, 0);
	SW(&phdr->p_paddr, 0);
	SW(&phdr->p_filesz, relsize);
	SW(&phdr->p_memsz, 0);
	SW(&phdr->p_flags, 0);
	SW(&phdr->p_align, 0x10);

	(void) sce_encode(data + relofs, flagBits, typeBits, &newFlagBits, &newTypeBits);

	fp = fopen(prxfile, "wb");
	if(fp == NULL)
	{
		fprintf(stderr, "Error, could not open output file %s\n", prxfile);
		free(data);
		return 1;
	}
	fwrite(data, 1, size, fp);
	fclose(fp);
	free(data);
	free(g_scerels);

	return 0;
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

int main(int argc, char **argv)
{
	if(process_args(argc, argv))
	{
		if(load_elf(g_infile))
		{
			(void) output_prx(g_outfile);
			free_data();
		}
	}
	else
	{
		print_help();
	}

	return 0;
}
