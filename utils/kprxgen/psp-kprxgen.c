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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../common/types.h"
#include "../common/elftypes.h"
#include "../common/prxtypes.h"

/* Arrangement of ELF file after stripping
 *
 * ELF Header - 52 bytes
 * Program Headers 
 * .text data
 * .data data
 * Section Headers
 * Relocation data
 * Section Header String Table
 *
 * When stripping the sections remove anything which isn't an allocated section or a relocation section.
 * The section string section we will rebuild.
 */

static const char *g_outfile;
static const char *g_infile;
static unsigned char *g_elfdata = NULL;
static struct ElfHeader g_elfhead;
static struct ElfSection *g_elfsections = NULL;
static struct ElfSection *g_modinfo = NULL;
static int g_alloc_size = 0;
static int g_reloc_size = 0;

/* Base addresses in the Elf */
static int g_phbase = 0;
static int g_allocbase = 0;
static int g_shbase = 0;
static int g_relocbase = 0;

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

			if(LH(pSymData[iSymbol].st_shndx) == 0)
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
	u32 i;

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

/* Reindex the sections we are keeping */
void reindex_sections(void)
{
	u32 i;
	int sect = 1;

	for(i = 0; i < g_elfhead.iShnum; i++)
	{
		if(g_elfsections[i].blOutput)
		{
			g_elfsections[i].iIndex = sect++;
		}
	}
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

		if(!load_sections())
		{
			break;
		}

		if(!process_relocs())
		{
			break;
		}

		reindex_sections();

		ret = 1;
	}
	while(0);

	return ret;
}

int calculate_outsize(void)
{
	u32 alloc_size = 0;
	int reloc_size = 0;
	u32 i;

	/* Calculate how big our output file needs to be */
	/* We have elf header + 3 PH + allocated data + section headers + relocation data */

	/* Note that the ELF should be based from 0, we use this to calculate the alloc and mem sizes */

	/* Skip null section */
	for(i = 1; i < g_elfhead.iShnum; i++)
	{
		if(g_elfsections[i].blOutput)
		{
			if(g_elfsections[i].iType == SHT_PROGBITS)
			{
                unsigned int top_addr = g_elfsections[i].iAddr + g_elfsections[i].iSize;
				if(top_addr > alloc_size)
				{
					alloc_size = top_addr;
				}
			}
			else if((g_elfsections[i].iType == SHT_REL) || (g_elfsections[i].iType == SHT_PRXRELOC))
			{
				/* Check this is a reloc for an allocated section */
				if(g_elfsections[g_elfsections[i].iInfo].iFlags & SHF_ALLOC)
				{
					reloc_size += g_elfsections[i].iSize;
				}
			}
		}
	}

	alloc_size = (alloc_size + 3) & ~3;

	/* Save them for future use */
	g_alloc_size = alloc_size;
	g_reloc_size = reloc_size;

	/* Lets build the offsets */
	g_phbase = sizeof(Elf32_Ehdr);
	/* The allocated data needs to be 64 byte aligned (probably; seen in kernel .prx files) */
	g_allocbase = (g_phbase + 3 * sizeof(Elf32_Phdr) + 0x3F) & ~0x3F;
	g_relocbase = g_allocbase + g_alloc_size;

	return g_relocbase + g_reloc_size;
}

/* Output the ELF header */
void output_header(unsigned char *data)
{
	Elf32_Ehdr *head;

	head = (Elf32_Ehdr*) data;

	SW(&head->e_magic, g_elfhead.iMagic);
	head->e_class = g_elfhead.iClass;
	head->e_data = g_elfhead.iData;
	head->e_idver = g_elfhead.iIdver;
	SH(&head->e_type, ELF_PRX_TYPE);
	SH(&head->e_machine, g_elfhead.iMachine);
	SW(&head->e_version, g_elfhead.iVersion);
	SW(&head->e_entry, g_elfhead.iEntry);
	SW(&head->e_phoff, g_phbase);
	SW(&head->e_shoff, g_shbase);
	SW(&head->e_flags, g_elfhead.iFlags);
	SH(&head->e_ehsize, sizeof(Elf32_Ehdr));
	SH(&head->e_phentsize, sizeof(Elf32_Phdr));
	SH(&head->e_phnum, 3);
	SH(&head->e_shentsize, 0);
	SH(&head->e_shnum, 0);
	SH(&head->e_shstrndx, 0);
}

struct ElfSection *get_sh(const char *name)
{
    u32 i;
    for(i = 0; i < g_elfhead.iShnum; i++)
        if (strcmp(name, g_elfsections[i].szName) == 0)
            return &g_elfsections[i];
    return NULL;
}

/* Output the program header */
void output_ph(unsigned char *data)
{
	Elf32_Phdr *phdr;
	struct PspModuleInfo *pModinfo;
	int mod_flags;
    int dataAddr, dataSize;

	phdr = (Elf32_Phdr*) data;
	pModinfo = (struct PspModuleInfo *) (g_modinfo->pData);
	mod_flags = LW(pModinfo->flags);

	SW(&phdr->p_type, 1);
	/* Starts after the program header */
	SW(&phdr->p_offset, g_allocbase);
	SW(&phdr->p_vaddr, 0);

	/* Check if this is a kernel module */
	if(mod_flags & 0x1000)
	{
		SW(&phdr->p_paddr, 0x80000000 | (g_modinfo->iAddr + g_allocbase));
	}
	else
	{
		SW(&phdr->p_paddr, (g_modinfo->iAddr + g_allocbase));
	}
	SW(&phdr->p_filesz, g_alloc_size);
	SW(&phdr->p_memsz, g_alloc_size);
	SW(&phdr->p_flags, 5);
	SW(&phdr->p_align, 0x40);

    /* Second program header */
    phdr++;

    if (get_sh(".data") != NULL) {
        dataAddr = get_sh(".data")->iAddr;
        dataSize = get_sh(".data")->iSize;
    }
    else {
        dataAddr = g_relocbase - g_allocbase;
        dataSize = 0;
    }
    SW(&phdr->p_type,   1);
    SW(&phdr->p_offset, dataAddr + g_allocbase);
    SW(&phdr->p_vaddr,  dataAddr);
    SW(&phdr->p_paddr,  0);
    SW(&phdr->p_filesz, dataSize);
    SW(&phdr->p_memsz,  dataSize + get_sh(".bss")->iSize);
    SW(&phdr->p_flags,  6);
    SW(&phdr->p_align,  0x40);

    /* Third program header */
    phdr++;

    SW(&phdr->p_type,   0x700000A1);
    SW(&phdr->p_offset, g_relocbase);
    SW(&phdr->p_vaddr,  0);
    SW(&phdr->p_paddr,  0);
    SW(&phdr->p_filesz, g_reloc_size);
    SW(&phdr->p_memsz,  0);
    SW(&phdr->p_flags,  0);
    SW(&phdr->p_align,  0x10);
}

/* Output the allocated sections */
void output_alloc(unsigned char *data)
{
	u32 i;

	for(i = 0; i < g_elfhead.iShnum; i++)
	{
		if((g_elfsections[i].blOutput) && (g_elfsections[i].iType == SHT_PROGBITS))
		{
			memcpy(&data[g_elfsections[i].iAddr], g_elfsections[i].pData, g_elfsections[i].iSize);
		}
	}
}

/* Output relocations */
void output_relocs(unsigned char *data)
{
	u32 i;
	unsigned char *pReloc;

	pReloc = data;

	for(i = 0; i < g_elfhead.iShnum; i++)
	{
		if((g_elfsections[i].blOutput) && 
				((g_elfsections[i].iType == SHT_REL) || (g_elfsections[i].iType == SHT_PRXRELOC)))
		{
			Elf32_Rel *rel;
			int j, count;

			memcpy(pReloc, g_elfsections[i].pData, g_elfsections[i].iSize);
			rel = (Elf32_Rel*) pReloc;
			count = g_elfsections[i].iSize / sizeof(Elf32_Rel);
			for(j = 0; j < count; j++)
			{
				unsigned int sym;

				/* Clear the top 24bits of the info */
				/* Kind of a dirty trick but hey :P */
				sym = LW(rel->r_info);
				sym &= 0xFF;
				SW(&rel->r_info, sym);
				rel++;
			}
			pReloc += g_elfsections[i].iSize;
		}
	}
}

/* Output a stripped prx file */
int output_prx(const char *prxfile)
{
	int size;
	unsigned char *data;
	FILE *fp;

	do
	{
		size = calculate_outsize();
		data = malloc(size);
		if(data == NULL)
		{
			fprintf(stderr, "Error, couldn't allocate output data\n");
			break;
		}

		memset(data, 0, size);

		output_header(data);
		output_ph(data + g_phbase);
		output_alloc(data + g_allocbase);
		output_relocs(data + g_relocbase);

		fp = fopen(prxfile, "wb");
		if(fp != NULL)
		{
			fwrite(data, 1, size, fp);
			fclose(fp);
		}
		else
		{
			fprintf(stderr, "Error, could not open output file %s\n", prxfile);
		}

		free(data);
	}
	while(0);

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
