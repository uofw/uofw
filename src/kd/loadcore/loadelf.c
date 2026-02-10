/* Copyright (C) 2011, 2012, 2013 The uOFW team
   See the file COPYING for copying permission.
*/

/*
 * uofw/src/loadcore/loadelf.c
 * 
 * Loadelf represents most of the basic API of the File Loader.  It is
 * responsible for performing checks on loadable ELF files and loading 
 * them.  Loading them also implies taking care of any relocation of the 
 * files if needed.
 * 
 * The File Loader defines two possible object file types for modules:
 *    a) a static ELF
 *    b) a relocatable object file - named PRX (Playstation Relocatable Executable)
 * 
 * PRX comes with a few unique attributes compared to normal ELF's:
 *    a) Customized Program Headers
 *    b) Non-standard MIPS relocation sections
 *    c) a unique ELF type
 * 
 * 
 * Module segment order:
 * 
 * A single module needs to have at least three segments and max 
 * SCE_KERNEL_MAX_MODULE_SEGMENT segments.  These segments are typically 
 * located in consecutive order:
 *    Segment       Description
 * 
 *    TEXT          Includes instruction code and data structures used for
 *                  linking between modules.
 *    DATA          Contains initialized data values.
 *    BSS           Contains uninitialized data values.
 * 
 * Note: These segments can consist of several sections, i.e. the TEXT segment
 *       can contain a .text section (instruction code) and .sceStub.text.xxx
 *       sections (these sections contain the function stubs of the imported
 *       libraries a module is using - the "xxx" stand for the name of imported
 *       library).
 */

#include <sysmem_kdebug.h>
#include <sysmem_utils_kernel.h>
#include "loadelf.h"
#include "module.h"

#define ELF_MEMORY_BLOCK_NAME       "SceLoadElfBlock"

/* KL4E compressed file's magic number. */
#define KL4E_COMPRESSION_SIGNATURE  "KL4E"

static s32 sceKernelApplyPspRelSegment2(u32 *segmentAddr, u32 nSegments, u8 *relocData, u32 relocSize);
static s32 sceKernelApplyPspRelSection(u32 *segmentAddr, u32 nSegments, Elf32_Rel *relocInfo, u32 fileSize);
static s32 PspUncompress(u8 *modBuf, SceLoadCoreExecFileInfo *execInfo, u32 *arg2);
static void readElfSegmentInfo(PspHeader *header, SceLoadCoreExecFileInfo *execInfo);
static s32 CheckElfSection(Elf32_Ehdr *elfHeader, SceLoadCoreExecFileInfo *execInfo);
static s32 CheckElfSectionPRX(Elf32_Ehdr *elfHeader, SceLoadCoreExecFileInfo *execInfo);
static s32 CheckElfImage(Elf32_Ehdr *elfHeader1 __attribute((unused)), Elf32_Ehdr *elfHeader2, Elf32_Phdr *progHeader);
static s32 CheckTick(u8 *modBuf);

//LoadCoreForKernel_D3353EC4 - Address 0x00003FAC
s32 sceKernelCheckExecFile(u8 *buf, SceLoadCoreExecFileInfo *execInfo) 
{
    SceHeader *sceHeader = NULL;
    PspHeader *pspHeader = NULL;   
    Elf32_Ehdr *elfHeader = NULL;
    Elf32_Phdr *elfProgHeader = NULL;
    u32 lowAddr;
    u32 maxAddr;
    u32 maxOffset;
    u32 i;
    u32 size, newSize;
    u32 nSegments;    
    s32 status;
    
    if (execInfo == NULL)
        return SCE_ERROR_KERNEL_ERROR;

    size = 0;
    lowAddr = maxAddr = maxOffset = 0;
    
    sceHeader = (SceHeader *)buf;
    if (((sceHeader->magic[0] << 24) | (sceHeader->magic[1] << 16) | (sceHeader->magic[2] << 8) | sceHeader->magic[3]) == SCE_MAGIC) { //0x00003FE0
        if (sceHeader->hdrVersion == SCE_HEADER_BETA_VERSION)
            return SCE_ERROR_KERNEL_CANNOT_USE_BETA_VER_MODULE;
        
        size = sceHeader->size;
        if (sceHeader->size < SCE_HEADER_SIZE) //0x000043B4
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
         
        if ((s32)((sceHeader->size + (u32)buf) ^ (u32)buf) < 0) //0x000043C4
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE; 
    }
    /* skip SCE header if necessary */
    buf += size;
    
    if ((s32)size < 0) //0x00003FE8
        return size;
    
    if (!(execInfo->modeAttribute & SCE_EXEC_FILE_NO_COMPRESSION)) { //0x00003FF8        
        execInfo->isDecrypted = SCE_FALSE; //0x00004008
        if ((u32)buf & (SCE_HEADER_SIZE - 1)) { //0x00004004
            execInfo->elfType = SCE_EXEC_FILE_TYPE_INVALID_ELF;
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
        }
        
        pspHeader = (PspHeader *)buf;
        if (((pspHeader->magic[0] << 24) | (pspHeader->magic[1] << 16) | (pspHeader->magic[2] << 8) | pspHeader->magic[3]) == PSP_MAGIC) { //0x00004018
            execInfo->modInfoAttribute = pspHeader->modAttribute; //0x00004298
            if (pspHeader->decryptMode == 0) //0x000042A0
                execInfo->modInfoAttribute = pspHeader->modAttribute & ~SCE_MODULE_PRIVILEGE_LEVELS; //0x000042B0              

            execInfo->decSize = pspHeader->elfSize; //0x000042BC
            execInfo->execAttribute = pspHeader->compAttribute;
            execInfo->execSize = pspHeader->pspSize; //0x000042C8
            execInfo->isKernelMod = SCE_FALSE;
            execInfo->entryAddr = pspHeader->bootEntry; //0x000042D4
            if (IS_KERNEL_ADDR(pspHeader->modInfoOffset) && pspHeader->decryptMode) //0x000042DC & 0x00004378
                execInfo->isKernelMod = SCE_TRUE; //0x00004384

            execInfo->moduleInfoOffset = pspHeader->modInfoOffset & 0x1FFFFFFF; //0x000042E8
            execInfo->overlapSize = 0; //0x000042F4
            execInfo->bssSize = pspHeader->bssSize;
            if (execInfo->execAttribute & SCE_EXEC_FILE_GZIP_OVERLAP) //0x000042FC
                execInfo->overlapSize = pspHeader->overlapSize;

            if (!(execInfo->modeAttribute & SCE_EXEC_FILE_NO_HEADER_COMPRESSION)) { //0x00004310
                if (((execInfo->execAttribute & SCE_EXEC_FILE_COMPRESSED) == 0) || execInfo->topAddr != NULL) { //0x0000431C & 0x00004328
                    status = PspUncompress((u8 *)pspHeader, execInfo, &newSize); //0x00004360
                    if (status != SCE_ERROR_OK)
                        return status; //0x00004368
                }
            }
            execInfo->elfType = SCE_EXEC_FILE_TYPE_PRX; //0x0000433C
            if (execInfo->execAttribute & SCE_EXEC_FILE_ELF) //0x00004338
                execInfo->elfType = SCE_EXEC_FILE_TYPE_ELF; //0x00004344

            readElfSegmentInfo(pspHeader, execInfo); //0x00004354
        }
        else {
            //0x00004020
            execInfo->execSize = 0;
        }
        if (execInfo->execSize != 0 && !execInfo->isDecrypted && !execInfo->isDecompressed) { //0x00004028 & 0x00004034 & 0x00004040
            if (execInfo->modeAttribute != SCE_EXEC_FILE_NO_HEADER_COMPRESSION || execInfo->execSize > 0) //0x000040A4
                execInfo->execSize += size; //0x000040B8

            return SCE_ERROR_OK;
        }
        //0x0000404C
        if (execInfo->execAttribute & SCE_EXEC_FILE_COMPRESSED) { //0x00004050
            execInfo->isCompressed = SCE_TRUE; //0x00004060
            buf = execInfo->topAddr;
            execInfo->fileBase = execInfo->topAddr; //0x00004064
        }       
    }
    execInfo->topAddr = (void *)SCE_KERNEL_VALUE_UNITIALIZED; //0x0000406C
    execInfo->modCodeSize = 0; //0x00004078
    execInfo->maxSegAlign = 0; //0x0000407C
    
    elfHeader = (Elf32_Ehdr *)buf;
    if (elfHeader->e_ident[EI_CLASS] == ELFCLASS32 && elfHeader->e_ident[EI_DATA] == ELFDATA2LSB) { //0x00004084               
        if (elfHeader->e_machine == EM_MIPS_ALLEGREX) { //0x000040F0
            if ((s32)elfHeader->e_phoff <= 0 || (s32)((((u32)buf + elfHeader->e_phoff) ^ (u32)buf)) < 0 || 
             (s32)((u32)buf + elfHeader->e_phoff) < 0) { //0x000040FC & 0x0000410C
                execInfo->elfType = SCE_EXEC_FILE_TYPE_INVALID_ELF;
                return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE; 
            }
            if (elfHeader->e_type == ET_EXEC) //0x00004124
                execInfo->elfType = SCE_EXEC_FILE_ELF;
            else if (elfHeader->e_type == ET_SCE_PRX) //0x00004130
                execInfo->elfType = SCE_EXEC_FILE_TYPE_PRX;

            /* Don't load ELF's which don't have at least one program header. */
            if (elfHeader->e_phnum <= 0) { //0x0000413C
                execInfo->elfType = SCE_EXEC_FILE_TYPE_INVALID_ELF;
                return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE; 
            }
            nSegments = elfHeader->e_phnum;
            
            //0x00004140 - 0x00004164
            /* 
             * Search for the first loadable segment.  If there is no loadable
             * segment, don't load the ELF. 
             */
            elfProgHeader = (Elf32_Phdr *)(buf + elfHeader->e_phoff);           
            for (; elfProgHeader->p_type != PT_LOAD; elfProgHeader++) {
                 if (--nSegments <= 0) { //0x00004158
                     execInfo->elfType = SCE_EXEC_FILE_TYPE_INVALID_ELF;
                     return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
                 }
            }
            if (nSegments <= 0) { //0x0000416C
                execInfo->elfType = SCE_EXEC_FILE_TYPE_INVALID_ELF;
                return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
            }
            if (elfProgHeader->p_vaddr == elfProgHeader->p_paddr) { //0x0000417C
                execInfo->elfType = SCE_EXEC_FILE_TYPE_INVALID_ELF; //0x00004270
                execInfo->moduleInfoOffset = 0;
            }
            else {
                execInfo->moduleInfoOffset = elfProgHeader->p_paddr & 0x1FFFFFFF; //0x00004184
            }
            execInfo->topAddr = (void *)elfProgHeader->p_vaddr; //0x0000418C

            // TODO: Fix the loop condition: != PT_LOAD -> == PT_LOAD
            if (nSegments > 0 && elfProgHeader->p_type == PT_LOAD) { //0x00004190 & 0x000041A0
                //0x00004200 - (0x00004244) - 0x0000425C
                for (i = 0; i < nSegments && elfProgHeader->p_type != PT_LOAD; i++, elfProgHeader++) {
                     if (elfProgHeader->p_vaddr < lowAddr) { //0x000041A4 & 0x000041FC
                         execInfo->topAddr = (void *)elfProgHeader->p_vaddr; //0x00004204
                         lowAddr = elfProgHeader->p_vaddr;
                     }
                     maxAddr = (u32)pspMax((s32)maxAddr, elfProgHeader->p_vaddr + elfProgHeader->p_memsz); //0x00004234
                     maxOffset = pspMax(maxOffset, elfProgHeader->p_offset + elfProgHeader->p_filesz); //0x0000423C
                     if (execInfo->maxSegAlign < elfProgHeader->p_align) //0x00004238
                         execInfo->maxSegAlign = elfProgHeader->p_align; //0x00004240
                }
            }
            execInfo->modCodeSize = maxAddr - lowAddr; //0x000041B0
            if (execInfo->elfType == SCE_EXEC_FILE_ELF) { //0x000041B8
                execInfo->modCodeSize = maxAddr - (lowAddr & 0xFFFFFF00); //0x000041D8
                
                if (execInfo->execSize == 0) //0x000041D4
                    execInfo->execSize = maxOffset; //0x000041E8
            }
            if (((execInfo->modeAttribute & SCE_EXEC_FILE_NO_HEADER_COMPRESSION) == 0) || execInfo->execSize != 0)
                execInfo->execSize += size; //0x000040B8

            return SCE_ERROR_OK;
        }
    }   
    if (!(execInfo->modeAttribute & SCE_EXEC_FILE_NO_HEADER_COMPRESSION)) { //0x00004094
         execInfo->elfType = SCE_EXEC_FILE_TYPE_INVALID_ELF; //0x000040E4
         return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
    }
    if (execInfo->execSize != 0) //0x000040AC
        execInfo->execSize += size;

    return SCE_ERROR_OK; 
}

//LoadCoreForKernel_41D10899
s32 sceKernelProbeExecutableObject(u8 *buf, SceLoadCoreExecFileInfo *execInfo)
{
    SceHeader *sceHeader = NULL;
    Elf32_Ehdr *elfHeader = NULL;
    Elf32_Phdr *elfProgHeader = NULL;
    SceModuleInfo *modInfo = NULL;
    s32 size;
    u32 tmpModAttr;
    s32 status;
    u8 isPrx;
    
    size = 0;
    execInfo->textSize = 0; //0x000043F0
    execInfo->dataSize = 0;
    execInfo->bssSize = 0;
    
    sceHeader = (SceHeader *)buf;
    if (((sceHeader->magic[0] << 24) | (sceHeader->magic[1] << 16) | (sceHeader->magic[2] << 8) | sceHeader->magic[3]) == SCE_MAGIC) { //0x00004408
        if (sceHeader->hdrVersion == SCE_HEADER_BETA_VERSION) { //0x000048D8
            execInfo->elfType = SCE_EXEC_FILE_TYPE_INVALID_ELF; //0x00004528
            return SCE_ERROR_KERNEL_CANNOT_USE_BETA_VER_MODULE;
        }            
        if (sceHeader->size < SCE_HEADER_SIZE) { //0x000048E8
            execInfo->elfType = SCE_EXEC_FILE_TYPE_INVALID_ELF; //0x00004528
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
        } 
        size = sceHeader->size; //0x000048FC
        
        if ((s32)((sceHeader->size + (u32)buf) ^ (u32)buf) < 0) { //0x000043C4
            execInfo->elfType = SCE_EXEC_FILE_TYPE_INVALID_ELF; //0x00004528
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;   
        }
    }   
    if (size < 0) { //0x00004410
        execInfo->elfType = SCE_EXEC_FILE_TYPE_INVALID_ELF; //0x00004528
        return size;      
    }
    /* skip SCE header if necessary */
    buf += size;
    
    if (!execInfo->isDecrypted && !execInfo->isDecompressed) { //0x0000441C
        status = sceKernelCheckExecFile(buf, execInfo); //0x000048B4
        if (status < SCE_ERROR_OK) { //0x000048C4
            execInfo->elfType = SCE_EXEC_FILE_TYPE_INVALID_ELF; //0x00004528
            return status;
        }
    }
    execInfo->textSize = 0; //0x00004434
    execInfo->dataSize = 0;
    execInfo->bssSize = 0;
    
    elfHeader = (Elf32_Ehdr *)buf;
    elfProgHeader = (Elf32_Phdr *)(elfHeader->e_phoff + buf); //0x00004444
    if (elfHeader->e_phoff <= 0 || ((s32)elfProgHeader ^ (s32)buf) < 0 || elfProgHeader == NULL) { //0x0000443C & 0x0000444C
        execInfo->elfType = SCE_EXEC_FILE_TYPE_INVALID_ELF; //0x00004528
        return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
    }    
    status = CheckElfImage(elfHeader, elfHeader, elfProgHeader); //0x0000445C
    if (status < SCE_ERROR_OK) { //0x00004464
        execInfo->elfType = SCE_EXEC_FILE_TYPE_INVALID_ELF; //0x00004528
        return status;
    }
    
    execInfo->entryAddr = elfHeader->e_entry; //0x00004480    
    //0x00004498
    switch (execInfo->elfType) {
    case SCE_EXEC_FILE_TYPE_PRX:
        isPrx = SCE_TRUE; //jumps to 0x000044A0
        break;
    case SCE_EXEC_FILE_TYPE_ELF:
        isPrx = SCE_FALSE; //jumps to 0x000046AC
        break;
    default: 
        execInfo->elfType = SCE_EXEC_FILE_TYPE_INVALID_ELF; //0x00004528
        return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
    }
    
    /* PRX */
    if (isPrx) { //0x00004498
        /* No moduleInfoOffset. */
        if (execInfo->moduleInfoOffset == 0) { //0x000044A4
            execInfo->elfType = SCE_EXEC_FILE_TYPE_INVALID_ELF; //0x00004528
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
        }
        status = CheckElfSectionPRX(elfHeader, execInfo); //0x000044B0
        if (status < SCE_ERROR_OK) { //0x000044B8
            execInfo->elfType = SCE_EXEC_FILE_TYPE_INVALID_ELF; //0x00004528
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
        }
        modInfo = (SceModuleInfo *)(buf + execInfo->moduleInfoOffset); //0x000044C8
        execInfo->moduleInfoOffset = (u32)modInfo;
        execInfo->exportsInfo = modInfo->entTop; //0x000044E0
        execInfo->exportsSize = modInfo->entEnd - modInfo->entTop;
        execInfo->importsSize = modInfo->stubEnd - modInfo->stubTop; //0x000044F0
        execInfo->importsInfo = modInfo->stubTop; //0x000044F8
        
        if (!execInfo->isDecrypted) { //0x000044F4
            if (modInfo->modAttribute & SCE_MODULE_PRIVILEGE_LEVELS) { //0x0000469C
                execInfo->elfType = SCE_EXEC_FILE_TYPE_INVALID_ELF; //0x00004528
                return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
            }
        }
        else {
            if ((modInfo->modAttribute & SCE_MODULE_PRIVILEGE_LEVELS) && (execInfo->modInfoAttribute & SCE_MODULE_PRIVILEGE_LEVELS)) { //0x00004504 & 0x00004514
                execInfo->elfType = SCE_EXEC_FILE_TYPE_INVALID_ELF; //0x00004528
                return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
            }
        }
        
        execInfo->modInfoAttribute |= (modInfo->modAttribute & ~SCE_MODULE_PRIVILEGE_LEVELS); //0x00004554
        if (execInfo->isDecrypted) //0x00004550
            return SCE_ERROR_OK;  
           
        if ((execInfo->modInfoAttribute & SCE_MODULE_PRIVILEGE_LEVELS) != SCE_MODULE_USER) { //0x00004558 - 0x00004590
            execInfo->elfType = SCE_EXEC_FILE_TYPE_INVALID_ELF; //0x00004528
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
        }
        
        /*
         * Accept the following API types (execInfo->unk12 & 0x10 > 0): 
         *    0x10, 0x110, 0x112, 0x114
         * Accept the following API types when a) PSP is in Tool mode
         * and b) execInfo->unk12 & 0x10 > 0:
         *    0x44 - 0x109, 0x122
         */
        
        /* check 0x11A - MAX_API_TYPE */
        if (execInfo->apiType >= 0x11A) { //0x000045A0
            if (execInfo->apiType == 0x122 && sceKernelIsToolMode() && execInfo->unk12 & 0x10)
                return SCE_ERROR_OK;              
        }        
        
        /* Check 0x44 - 0x119 */
        if (execInfo->apiType >= 0x44) { //0x000045B8
            if (execInfo->apiType == 0x110 || execInfo->apiType == 0x112 || execInfo->apiType == 0x114) { //0x00004620            
                if (execInfo->unk12 & 0x10) //0x000045FC
                    return SCE_ERROR_OK;         
            }          
            /* check 0x44 - 0x109 */
            if (execInfo->apiType < 0x110) { //0x00004614
                if (sceKernelIsToolMode() && execInfo->unk12 & 0x10)
                    return SCE_ERROR_OK;
            }
        }             
        if (execInfo->apiType == 0x10 && execInfo->unk12 & 0x10) //0x000045EC
            return SCE_ERROR_OK;
        
        execInfo->elfType = SCE_EXEC_FILE_TYPE_INVALID_ELF; //0x00004528
        return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;                    
    }
    
    /* ELF */
    status = CheckElfSection(elfHeader, execInfo); //0x000046B0
    if (status < SCE_ERROR_OK) { //0x000046B8
        execInfo->elfType = SCE_EXEC_FILE_TYPE_INVALID_ELF; //0x00004528
        return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE; 
    }
    /* Copy ModuleInfo. */
    modInfo = (SceModuleInfo *)(buf + execInfo->moduleInfoOffset); //0x000046C8
    execInfo->moduleInfo = modInfo; //0x000046CC
    execInfo->exportsInfo = modInfo->entTop; //0x000046E0
    execInfo->exportsSize = modInfo->entEnd - modInfo->entTop; //0x000046EC
    execInfo->importsSize = modInfo->stubEnd - modInfo->stubTop; //0x000046F0
    execInfo->importsInfo = modInfo->stubTop; //0x000046F8
    
    tmpModAttr = modInfo->modAttribute & SCE_MODULE_PRIVILEGE_LEVELS;
    if (!execInfo->isDecrypted) { //0x000046F4
        if (tmpModAttr) { //0x000048A4
            execInfo->elfType = SCE_EXEC_FILE_TYPE_INVALID_ELF; //0x00004528
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE; 
        }        
    }
    else { 
        /* Module privilege level mismatch! Don't load ELF. */
        if (tmpModAttr && tmpModAttr != execInfo->modInfoAttribute) { //0x00004704 & 0x00004714
            execInfo->elfType = SCE_EXEC_FILE_TYPE_INVALID_ELF; //0x00004528
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
        }
    }
    
    execInfo->modInfoAttribute |= (tmpModAttr & ~SCE_MODULE_PRIVILEGE_LEVELS); //0x00004730
    /* Don't load ELF into Kernel memory. */
    if (IS_KERNEL_ADDR(execInfo->topAddr)) { //0x0000472C
        execInfo->elfType = SCE_EXEC_FILE_TYPE_INVALID_ELF; //0x00004528
        return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
    }
    /* Don't load privileged ELF. */
    if (execInfo->modInfoAttribute & SCE_MODULE_PRIVILEGE_LEVELS) { //0x00004738
        execInfo->elfType = SCE_EXEC_FILE_TYPE_INVALID_ELF; //0x00004528
        return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
    }
    
    /*** TODO: Check if the "apiType" checks are correct and use a switch statement for them. ***/
    
    /*
     * This block allows the following APIs:
     *    0x120 -> when executable is decrypted
     */
    if (execInfo->apiType == 0x120) { //0x00004744
        if (execInfo->isDecrypted)  //0x000047F0
            return SCE_ERROR_OK;
        
        execInfo->elfType = SCE_EXEC_FILE_TYPE_INVALID_ELF; //0x00004528
        return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE; 
    }
    /*
     * This block allows the following API types between 0x121 and 0x172:
     *    0x123, 0x124, 0x125, 0x126, 0x157, 0x158, 0x159, 0x160, 0x161,
     *    0x170, 0x171 -> when executable is decrypted
     *    0x122 -> when PSP is in Tool mode and executable is decrypted or 
     *             execInfo->unk12 & 0x10 > 0
     */
    if (execInfo->apiType >= 0x121) { //0x0000474C                      
        if (execInfo->apiType == 0x122) { //0x00004848
            if (sceKernelIsToolMode() && (execInfo->unk12 & 0x10 || execInfo->isDecrypted)) //0x0000479C & 0x000047AC & 000045FC
                return SCE_ERROR_OK;
        }
        if ((execInfo->apiType >= 0x123 && execInfo->apiType < 0x127) ||
          (execInfo->apiType >= 0x157 && execInfo->apiType < 0x162) ||
          (execInfo->apiType >= 0x170 && execInfo->apiType < 0x172)) {
            if (execInfo->isDecrypted)  //0x000047F0
                return SCE_ERROR_OK;
        }
        execInfo->apiType = SCE_EXEC_FILE_TYPE_INVALID_ELF; //0x00004528
        return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;   
    }
    /*
     * This block makes the following API types disallowed:
     *    0x0 - 0x43
     */
    if (execInfo->apiType < 0x44) { //0x00004758
        execInfo->elfType = SCE_EXEC_FILE_TYPE_INVALID_ELF; //0x00004528
        return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
    }   
    /*
     * This block allows the following API types:
     *    0x110 -> when executable is decrypted or execInfo->unk12 & 0x10 > 0 
     *    0x112, 0x114 -> when PSP is in Tool mode and executable is decrypted or 
     */
    if (execInfo->apiType == 0x110) { //0x00004764
        if (execInfo->unk12 & 0x10 || execInfo->isDecrypted) //0x000047E8 &0x000047F0
            return SCE_ERROR_OK;
        
        execInfo->elfType = SCE_EXEC_FILE_TYPE_INVALID_ELF; //0x00004528
        return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
    }
    /*
     * This block allows the following API types between 0x111 and 0x120:
     *    0x111, 0x113, 0x115 -> when executable is decrypted
     *    0x112, 0x114 -> when PSP is in Tool mode and executable is decrypted or 
     *                    execInfo->unk12 & 0x10 > 0 
     */
    if (execInfo->apiType >= 0x111) { //0x0000476C            
        if (execInfo->apiType == 0x111 || execInfo->apiType == 0x113 || execInfo->apiType == 0x115) { //0x000047D0 & 0x000047BC & 0x00004808
            if (execInfo->isDecrypted) //0x000047F0
                return SCE_ERROR_OK;
        }
        if (execInfo->apiType == 0x112 || execInfo->apiType == 0x114) { //0x000047D8 & 0x000047C4
            if (execInfo->unk12 & 0x10 || execInfo->isDecrypted) //0x000047E8 &0x000047F0
                return SCE_ERROR_OK;
        }
        execInfo->elfType = SCE_EXEC_FILE_TYPE_INVALID_ELF; //0x00004528
        return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
    }
    /*
     * This block makes the following API types disallowed:
     *    0x44 - 0x52
     */
    if (execInfo->apiType < 0x53) { //0x00004778 & 0x00004780
        execInfo->elfType = SCE_EXEC_FILE_TYPE_INVALID_ELF; //0x00004528
        return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
    }
    /*
     * This block allows the following APIs:
     *    0x70 -> when PSP is in Tool mode AND execInfo->unk12 & 0x10 > 0 
     *            OR executable is decrypted
     */
    if (execInfo->apiType == 0x70) { //0x0000478C
        if (sceKernelIsToolMode() && (execInfo->unk12 & 0x10 || execInfo->isDecrypted)) //0x0000479C & 0x000047AC & 000045FC
            return SCE_ERROR_OK;
    }
    /*
     * This blocks disallows all other remaining APIs.
     */
    execInfo->elfType = SCE_EXEC_FILE_TYPE_INVALID_ELF; //0x00004528
    return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;   
}

//Subroutine LoadCoreForKernel_1C394885 - Address 0x00004908
s32 sceKernelLoadExecutableObject(u8 *buf, SceLoadCoreExecFileInfo *execInfo) 
{
    SceHeader *sceHeader = NULL;
    Elf32_Ehdr *elfHeader = NULL;
    Elf32_Phdr *elfProgHeader = NULL;
    Elf32_Shdr *elfSectionHdr = NULL;
    SceUID blockId;
    s32 segments;
    u32 backUpSegments;
    s32 size;
    s32 status;
    s32 nSegments;
    u32 *segmentPtr;
    u32 i;
    
    size = 0;
    backUpSegments = 0;
    segmentPtr = NULL;
    
    sceHeader = (SceHeader *)buf;
    if (((sceHeader->magic[0] << 24) | (sceHeader->magic[1] << 16) | (sceHeader->magic[2] << 8) | sceHeader->magic[3]) == SCE_MAGIC) { //0x00004948
        if (sceHeader->hdrVersion == SCE_HEADER_BETA_VERSION) //0x00004E3C
            return SCE_ERROR_KERNEL_CANNOT_USE_BETA_VER_MODULE;
            
        if (sceHeader->size < SCE_HEADER_SIZE) //0x00004E4C
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
 
        size = sceHeader->size; //0x00004E60
        
        if ((s32)((sceHeader->size + (u32)buf) ^ (u32)buf) < 0) //0x00004E5C
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;   

    }   
    if (size < 0) //0x00004950
        return size;      

    /* skip SCE header if necessary */
    buf += size; //0x0000495C
    
    if (execInfo->elfType != SCE_EXEC_FILE_TYPE_PRX && execInfo->elfType != SCE_EXEC_FILE_TYPE_ELF) //0x00004974
        return SCE_ERROR_KERNEL_ERROR;

    elfHeader = (Elf32_Ehdr *)buf;
    if (elfHeader->e_phoff <= 0)  //0x00004980
        return SCE_ERROR_KERNEL_ERROR;
    
    elfProgHeader = (Elf32_Phdr *)(buf + elfHeader->e_phoff); //0x00004984
    if (((s32)elfProgHeader ^ (s32)buf) < 0 || elfProgHeader == NULL) //0x0000498C & 0x00004994
        return SCE_ERROR_KERNEL_ERROR;  
    
    //0x000049A4 - 0x000049D0
    /* 
     * Verify if there are loadable segments. An ELF needs at least one
     * loadable segment in order to be loaded.
     */
    for (nSegments = elfHeader->e_phnum; elfProgHeader->p_type != PT_LOAD; elfProgHeader++, nSegments--) {
         if (nSegments <= 0) //0x000049C0
             return SCE_ERROR_KERNEL_ERROR;
    }
    if (nSegments <= 0) //0x000049D4
        return SCE_ERROR_KERNEL_ERROR;
    
    //0x000049F4 - 0x00004A68
    for (i = 0, segments = 0; elfProgHeader[i].p_type == PT_LOAD; i++) {
         if (segments >= SCE_KERNEL_MAX_MODULE_SEGMENT) //0x00004A08
             return SCE_ERROR_KERNEL_ERROR;       
        
         if ((s32)elfProgHeader[i].p_offset < 0 || (s32)elfProgHeader[i].p_vaddr < 0 || 
           (s32)elfProgHeader[i].p_filesz < 0 || (s32)elfProgHeader[i].p_memsz < 0 || 
           ((s32)(elfProgHeader[i].p_offset + elfHeader) >> 31) != ((s32)elfHeader >> 31)) {
             Kprintf("Cannot load kernel mode ELF\n");
             return SCE_ERROR_KERNEL_ERROR;
         }
         g_segmentStart[segments] = elfProgHeader[i].p_offset + elfHeader; //0x00004A24 & 0x00004A44
         g_segmentSize[segments] = elfProgHeader[i].p_filesz; //0x00004A18 & 0x00004A4C
         execInfo->segmentAlign[segments] = elfProgHeader[i].p_align; //0x00004A40 & 0x00004A50
         execInfo->segmentAddr[segments] = elfProgHeader[i].p_vaddr; //0x00004A14 & 0x00004A54
         execInfo->segmentSize[segments] = elfProgHeader[i].p_memsz; //0x00004A1C & 0x00004A5C
         
         if (++segments >= nSegments) //0x00004A58
             break;
    }
    execInfo->numSegments = segments; //0x00004A78
    //0x00004A6C - 0x00004A9C
    /* 
     * Fill every segment spot with no corresponding segment in the 
     * executable file with default values (0).
     */
    for(; segments < SCE_KERNEL_MAX_MODULE_SEGMENT; segments++) {
        execInfo->segmentAddr[segments] = 0; //0x00004A8C
        execInfo->segmentSize[segments] = 0; //0x00004A90
        execInfo->segmentAlign[segments] = 0; //0x00004A94
    }
    if (nSegments < 0) //0x00004AA4
        return nSegments;
    
    if (nSegments == 3) { //0x00004AB0       
        size = execInfo->segmentAddr[execInfo->numSegments - 1] + execInfo->segmentSize[execInfo->numSegments - 1]; //0x00004DC8
        if (!execInfo->isCompressed) { //0x00004DC4
            /* Ensure 256-Byte alignment of the ELF block. */         
            size = UPALIGN256(size) - (execInfo->segmentAddr[0] & 0xFFFFFF00); //0x00004DD8 & 0x00004DE0 & 0x00004DEC
            blockId = sceKernelAllocPartitionMemory(execInfo->partitionId, ELF_MEMORY_BLOCK_NAME, SCE_KERNEL_SMEM_Addr, 
                                                    size, execInfo->segmentAddr[0] & 0xFFFFFF00); //0x00004DF4
            
            execInfo->memBlockId = blockId; //0x00004DFC
            if (blockId < 0)
                return blockId;
        } 
    } 
    else {
        if (nSegments != execInfo->numSegments) { //0x00004AC4
            elfProgHeader = ((Elf32_Phdr *)(elfHeader->e_phoff + elfHeader)) + execInfo->numSegments; //0x00004AD0
            /* executable has to be relocated. */
            if (elfProgHeader->p_type == PT_PRX_RELOC || elfProgHeader->p_type == PT_PRX_RELOC2) { //0x00004AE8  
                segmentPtr = (u32 *)(elfProgHeader->p_offset + elfHeader); //0x00004AF8
            
                if ((s32)elfProgHeader->p_offset < 0 || 
                  (((s32)elfProgHeader->p_offset + (s32)elfHeader) >> 31) != ((s32)elfHeader >> 31)) { //0x00004B04
                    backUpSegments = execInfo->numSegments - 1; //0x00004BF8 -- $s4
                    return SCE_ERROR_KERNEL_ERROR; //0x00004C00
                }
            }
        }
        //0x00004B10 - 0x00004B48
        for (i = 0; i < execInfo->numSegments; i++) {
             execInfo->segmentAddr[i] += (u32)execInfo->topAddr; //0x00004B30          
             /* Reduce segments by one when the signs of execInfo->topAddr 
              * and (execInfo->segmentAddr[i] + execInfo->topAddr) differ.
              */            
             if ((((s32)execInfo->segmentAddr[i] + (s32)execInfo->topAddr) >> 31) != ((s32)execInfo->topAddr >> 31)) { //0x00004B3C
                 backUpSegments = execInfo->numSegments - 1;
                 return SCE_ERROR_KERNEL_ERROR;
             }
        }
        if ((s32)execInfo->entryAddr != SCE_KERNEL_VALUE_UNITIALIZED) //0x00004B54
            execInfo->entryAddr += (u32)execInfo->topAddr; //0x00004B5C
        
        if (execInfo->exportsInfo != (void *)SCE_KERNEL_VALUE_UNITIALIZED) //0x00004B74
            execInfo->exportsInfo += (u32)execInfo->topAddr; //0x00004B7C
        
        if (elfProgHeader->p_type == PT_PRX_RELOC) { //0x00004B88
            status = sceKernelApplyPspRelSection(execInfo->segmentAddr, execInfo->numSegments, (Elf32_Rel *)segmentPtr, 
                                                 elfProgHeader->p_filesz >> 3); //0x00004D94
            backUpSegments = execInfo->numSegments - 1; //0x00004BF4
            if (status < SCE_ERROR_OK) //0x00004D78                           
                return status; //0x00004BFC

        } else if (elfProgHeader->p_type == PT_PRX_RELOC2) { //0x00004B94
            status = sceKernelApplyPspRelSegment2(execInfo->segmentAddr, execInfo->numSegments, 
                                                  (u8 *)segmentPtr, elfProgHeader->p_filesz); //0x00004D6C
            backUpSegments = execInfo->numSegments - 1; //0x00004BF4
            if (status < SCE_ERROR_OK) //0x00004D78                             
                return status; //0x00004BFC

        } else if (elfHeader->e_shoff <= 0) { //0x00004BA4
            backUpSegments = execInfo->numSegments - 1; //0x00004BF4
            return SCE_ERROR_KERNEL_ERROR;
        } else if (elfHeader->e_shnum < 2) //0x00004BBC
            backUpSegments = execInfo->numSegments - 1; //0x00004BF4
        
        else {
        //0x00004BD0
            elfSectionHdr = ((Elf32_Shdr *)elfHeader + elfHeader->e_shoff); //0x00004BC0
            for (i = 1; i < elfHeader->e_shnum; i++) { //0x00004BB0 - 0x00004BE4
                 if (elfSectionHdr[i].sh_type == PT_PRX_RELOC && 
                     elfSectionHdr[elfSectionHdr[i].sh_info].sh_flags == SHF_ALLOC) { //0x00004BD4 & 0x00004D04
                        if (elfSectionHdr[i].sh_offset > 0 && elfSectionHdr[i].sh_entsize > 0) { //0x00004D20
                            status = sceKernelApplyPspRelSection(execInfo->segmentAddr, execInfo->numSegments, 
                                                                 (Elf32_Rel *)(elfHeader + elfSectionHdr[i].sh_offset), 
                                                                 elfSectionHdr[i].sh_size / elfSectionHdr[i].sh_entsize); //0x00004D38
                            if (status < SCE_ERROR_OK) { //0x00004D40                     
                                backUpSegments = execInfo->numSegments - 1; //0x00004BF4
                                return status;
                            }
                        }
                        else { //0x00004D60
                            backUpSegments = execInfo->numSegments - 1; //0x00004BF4
                            return SCE_ERROR_KERNEL_ERROR;
                        }                   
                    }
                }
            }
            backUpSegments = execInfo->numSegments - 1; //0x00004BF4
    }
    //0x00004C0C - 0x00004C74
    for (i = 0; i < execInfo->numSegments; i++) {
         sceKernelMemmove((void *)execInfo->segmentAddr[i], g_segmentStart[i], g_segmentSize[i]); //0x00004C3C
         if (i >= backUpSegments && g_segmentSize[i] < execInfo->segmentSize[i]) //0x00004C4C
             sceKernelMemset((void *)execInfo->segmentAddr[i] + g_segmentSize[i], 0, execInfo->segmentSize[i] - g_segmentSize[i]); //0x00004CD4
    }
    execInfo->moduleInfo = (SceModuleInfo *)((((u32)g_segmentStart[0] - (u32)elfHeader) - execInfo->segmentAddr[0]) + 
                             execInfo->moduleInfoOffset); //0x00004C94
    return SCE_ERROR_OK;
}

//Subroutine sub_00004E70 - Address 0x00004E70
static s32 sceKernelApplyPspRelSegment2(u32 *segmentAddr, u32 nSegments, u8 *relocData, u32 relocSize) 
{
    u32 type;
    u32 addr, addrBase, vAddr;
    u32 tmp, tmp2;
    u32 data;
    u8 *block1;
    u8 *block2;
    
    u8 type_table_size_1, type_table_size_2;
    u8 type_index;
    
    u8 *nData;
    u8 *pos;
    
    u32 cmd;    
    u8 part1s, part2s;
    u32 part1;   
    u32 offsetBase; 
    u8 lastPart2;
    
    u32 offset;
    u32 addend;
    u32 nBits; 
    
    offset = 0; //0x00004EAC
    addend = 0; //0x00004EB4
    offsetBase = nSegments; //0x00004E88
    nBits = (nSegments < 3) ? 1 : 2; //0x00004E90 & 0x00004E9C  
    
    if (*(u16 *)relocData != 0) //0x00004EC8
        return SCE_ERROR_KERNEL_ERROR;
        
    type_table_size_1 = relocData[4]; //0x00004ED0
    block1 = &relocData[4]; //0x00004ED4
    
    part1s = relocData[2]; //0x00004ED8
    part2s = relocData[3]; //0x00004EE4
    
    block2 = (block1 + type_table_size_1); //0x00004EDC
    type_table_size_2 = block2[0]; //0x00004EE0
    
    pos = block2 + type_table_size_2; //0x00004EE8 
    lastPart2 = type_table_size_2; //0x00004EF4
    
    //0x00004EF0 - 0x00005078
    for (nData = pos; nData < relocData + relocSize; nData = pos) {
         cmd = *(u16 *)nData; //0x00004EF8
         type_index = cmd & ((1 << part1s) - 1); //0x00004EF8 - 0x00004F08
         
         pos = nData + 2; //0x00004F14
         if (type_index >= type_table_size_1) //0x00004F18
             return SCE_ERROR_KERNEL_ERROR;
         
         part1 = block1[type_index]; //0x00004F24
         
         if ((part1 & 0x1) == 0) { //0x00004F2C
             offsetBase = ((1 << nBits) - 1) & (cmd >> part1s); //0x00004F30 & 0x00005234 & 0x0000523C
             /* Invalid offset base. */
             if (offsetBase >= nSegments) //0x00005248
                 return SCE_ERROR_KERNEL_ERROR;
             
             if ((part1 & 0x6) == 0) //0x00005258
                 continue; 
             
             if ((part1 & 0x4) != 4) //0x00005264
                 return SCE_ERROR_KERNEL_ERROR;
             
             pos = nData + 6; //0x0000526C
             offset = *(u32 *)(nData + 2); //0x00005270
         } else if ((part1 & 0x1) != 1)  //0x00004F34
             return SCE_ERROR_KERNEL_ERROR;
         else {
             type_index = (((part1 & 0x1) << part2s) - 1) & (cmd >> (nBits + part1s)); //0x00004F38 - 0x00004F48
             /* Invalid index for the second part? */
             if (type_index >= type_table_size_2) //0x00004F54
                 return SCE_ERROR_KERNEL_ERROR;
             
             addrBase = (((part1 & 0x1) << nBits) - 1) & (cmd >> part1s); //0x00004F5C & 0x00004F60 & 0x00004F68 & 0x00004F70
             if (offsetBase >= nSegments || addrBase < nSegments) //0x00004F74 & 0x00004F84
                 return SCE_ERROR_KERNEL_ERROR;
             
             type = ELF32_R_TYPE(block1[type_index]); //0x00004F8C & 0x00004F9C
             
             //0x00004F90 - 0x00004FC0
             switch (part1 & 0x6) {
             case 0: //0x00004FB4
                 offset += ((s16)cmd) >> (part2s + nBits + part1s); //0x00004FB0 - 0x00004FC0
                 break;                   
             case 2: //0x00004F98
                 cmd = (cmd << 16) >> (u32)(relocData + nBits + part1s); //0x0000520C - 0x0000521C
                 cmd = (cmd & 0xFFFF0000) | *(u16 *)(nData + 2); //0x00005220 & 0x00005224
                 offset += cmd; //0x00005228
                 pos = nData + 4; //0x00005230
                 break;
             case 4: //0x00004FA4 & 0x000051F0
                 pos = nData + 6; //0x000051F8
                 offset = *(u32 *)(nData + 2); //0x000051FC - 0x00005200
                 break;    
             /* Unknown OD: (part1 & 0x6)[part1 & 0x1] */    
             default: 
                 return SCE_ERROR_KERNEL_ERROR;
             }
             
             if (offset >= g_segmentSize[offsetBase]) //0x00004FDC
                 return SCE_ERROR_KERNEL_ERROR;
             
             //0x00004FE8 - 0x00005008
             switch (part1 & 0x38) {
             case 0:
                 addend = 0;
                 break;
             case 0x8: //0x00004FF0
                 if ((lastPart2 ^ 0x4) != 0) //0x00004FF4 & 0x000051E0
                     addend = 0; //0x000051E4
                 break;
             case 0x10: //0x00004FFC & 0x000051B8
                 addend = *(u16 *)pos; //0x000051C0
                 pos += 2; //0x000051C8
                 break;
             /* unknown ADDEND: (part1 & 0x38)[part1] */
             default:
                 return SCE_ERROR_KERNEL_ERROR;    
             }
             
             vAddr = (u32)g_segmentStart[offsetBase]; //0x00005018
             data = segmentAddr[addrBase]; //0x0000502C
             addr = offset + vAddr; //0x00005030
             
             //0x00005028 - 
             switch (type) {
             case R_MIPS_NONE: //0x00005050 & 0x0000506C
                 continue;
             case R_MIPS_16: case R_MIPS_HI16: //0x00005128
                 tmp = *(u32 *)addr; //0x00005128
                 tmp2 = (data + (s16)tmp) & 0xFFFF; //0x00005134 & 0x00005138
                 tmp &= 0xFFFF0000; //0x00005140
                 *(u32 *)addr = tmp | tmp2; //0x000050E8 & 0x00005064
                 break;
             case R_MIPS_32: //0x00005058
                 tmp = *(u32 *)addr; //0x00005058
                 *(u32 *)addr = tmp + data; //0x00005064
                 continue;
             case R_MIPS_REL32: //0x000050B0
                 tmp = *(u32 *)addr; //0x000050B8 & 0x000050BC
                 tmp2 = tmp & 0x3FFFFFFF; //0x000050C4
                 tmp2 = ((segmentAddr[offsetBase] + offset) & 0xF0000000) | (tmp2 << 2); //0x000050B4 & 0x000050C8 & 0x000050CC & 0x000050D4
                 tmp2 += data; //0x000050D8
                 *(u32 *)addr = (tmp2 & 0x0FFFFFFC) | (tmp & 0xFC000000); //0x000050DC - 0x000050E8 & 0x00005064
                 continue;
             case R_MIPS_26: //0x000050EC
                 tmp = *(u32 *)addr; //0x000050EC
                 tmp2 = (((s16)addend) + (tmp << 16)) + data; //0x000050F4 -- 0x00005100
                 tmp2 >>= 15; //0x00005104
                 tmp2 = ((tmp2 + 1) >> 1) & 0xFFFF; //0x00005108 & 0x0000510C
                 *(u32 *)data = (tmp2 | (tmp & 0xFFFF0000)); //0x00005110 & 0x00005114 &0x00005118 & 0x00005064
                 continue;
             case R_MIPS_LO16: //0x00005144
                 tmp = (*(u32 *)addr) & 0x3FFFFFFF; //0x0000514C - 0x00005154
                 tmp2 = (segmentAddr[offsetBase] + offset) & 0xF0000000; //0x00005144 & 0x00005148 & 0x00005158 & 0x00005160
                 tmp2 = (tmp2 | (tmp << 2)) + data; //0x00005168
                 *(u32 *)data = (tmp2 & 0x0FFFFFFC) | 0x08000000; //0x0000516C & 0x00005174 & 0x000050E8 & 0x00005064
                 continue;
             case R_MIPS_GPREL16: //0x00005178
                 tmp = (*(u32 *)addr) &  0x3FFFFFFF; //0x00005180 - 0x00005188
                 tmp2 = (segmentAddr[offsetBase] + offset) & 0xF0000000; //0x00005178 & 0x0000517C & 0x0000518C & 0x00005194
                 tmp2 = (tmp2 | (tmp << 2)) + data; //0x00005190 & 0x00005198 & 0x0000519C
                 *(u32 *)data = (tmp2 & 0x0FFFFFFC) | 0x0C000000; //0x000051A0 & 0x000051A8 & 0x000050E8 & 0x00005064
                 continue;
             default:
                 /* unacceptable relocation type. */
                 return SCE_ERROR_KERNEL_ERROR;    
             }      
         }   
    }
    return SCE_ERROR_OK;   
}

//Subroutine sub_00005280 - Address 0x00005280
static s32 sceKernelApplyPspRelSection(u32 *segmentAddr, u32 nSegments, Elf32_Rel *relocInfo, u32 fileSize)
{
    u32 daddr, daddr2;
    u32 segStart, segSize;
    u32 ofsSegIndex, ofsSegIndex2, addrSegIndex;
    s32 type;
    u32 data, dataLow;
    u32 i, j, k;
    
    //0x000052B4 & 0x000052BC - 0x00005380
    for (i = 0; i < fileSize; i += k) {
         type = ELF32_R_TYPE(relocInfo[i].r_info); //0x000052E4
         ofsSegIndex = ELF32_R_OFS_BASE(relocInfo[i].r_info); //0x000052C0
         addrSegIndex = ELF32_R_ADDR_BASE(relocInfo[i].r_info); //0x000052C8
         
         k = 1; //0x000052C4
                 
         if (addrSegIndex >= nSegments || ofsSegIndex >= nSegments) //0x000052E8
             return SCE_ERROR_KERNEL_ERROR; 
         
         if (relocInfo[i].r_offset >= g_segmentSize[ofsSegIndex])  //0x00005308
             return SCE_ERROR_KERNEL_ERROR;       
         
         segStart = segmentAddr[addrSegIndex]; //0x000052BC & 0x000052C8 & 0x0000531C & 0x00005320 & 0x00005330
         daddr = (u32)(relocInfo[i].r_offset + g_segmentStart[ofsSegIndex]); //0x00005328 - 0x000052C0
         
         switch (type) {
         case R_MIPS_NONE: //0x00005374
             break;
         case R_MIPS_16: //0x00005350 
             data = segStart + (u32)(((*(s32 *)daddr) << 16) >> 16); //0x00005350 - 0x0000535C
             *(u32 *)daddr &= 0xFFFF0000; //0x00005364
             *(u32 *)daddr |= data & 0xFFFF; //0x00005360 & 0x00005368
             break;
         case R_MIPS_32: //0x000053B8
                 *(u32 *)(daddr) += segStart; //0x000053C4
                 break;
         case R_MIPS_26: //0x000053C8
             data = *(u32 *)daddr; //0x000053D4
             data = (data & 0x03FFFFFF) << 2; //0x000053DC & 0x000053E4
             daddr2 = ((u32)segmentAddr[ofsSegIndex]) + relocInfo[i].r_offset; //0x000053C8 & 0x000053CC & 0x000053E0
             data |= (daddr2 & 0xF0000000); //0x000053E8 & 0x000053EC
             data += segStart; //0x000053F0
             *(u32 *)daddr &= 0xFC000000; //0x000053FC
             *(u32 *)daddr |= (data >> 2) & 0x03FFFFFF; //0x000053F4 & 0x00005368
             break;  
         case R_MIPS_HI16: //0x00005400
             data = (u32)((*(s32 *)daddr) << 16); //0x0000540C & 0x00005414
             
             //0x00005418 & 0x00005538 - 0x00005580
             for (j = i + 1; j < fileSize && (relocInfo[j].r_info == R_MIPS_HI16); j++, k++) {
                  ofsSegIndex2 = ELF32_R_OFS_BASE(relocInfo[j].r_info); //0x00005538
                  if (relocInfo[j].r_offset >= g_segmentSize[ofsSegIndex] || ofsSegIndex2 >= nSegments) //0x00005558
                      return SCE_ERROR_KERNEL_ERROR;
             }
             ofsSegIndex2 = ELF32_R_OFS_BASE(relocInfo[j].r_info); //0x00005438
             segSize = g_segmentSize[j]; //0x00005440
             if (ofsSegIndex2 >= nSegments || relocInfo[j].r_offset >= segSize) //0x0000545C
                 return SCE_ERROR_KERNEL_ERROR;
             
             daddr2 = (u32)(g_segmentStart[ofsSegIndex2] + relocInfo[j].r_offset); //0x00005464 - 0x00005478
             dataLow = (u32)(((*(s32 *)daddr2) << 16) >> 16); //0x0000547C - 0x00005484
             data = data + dataLow + segStart; //0x00005488 & 0x0000548C
             *(u32 *)daddr |= ((data >> 15) >> 1) & 0xFFFF; //0x00005490 & 0x00005494 & 0x0000549C
             
             //0x00005498 & 0x000054AC - 0x00005508
             for (j = 0; j < k; j++) {
                  ofsSegIndex2 = ELF32_R_OFS_BASE(relocInfo[j].r_info); //0x000054B0
                  if (ofsSegIndex2 >= nSegments || relocInfo[j].r_offset >= segSize) //0x000054D0
                      return SCE_ERROR_KERNEL_ERROR; 
                  
                  daddr2 = (u32)(g_segmentStart[ofsSegIndex2] + relocInfo[j].r_offset); //0x000054D8 - 0x000054E4
                  *(u32 *)daddr2 |= 0xFFFF0000; //0x000054F8
                  *(u32 *)daddr2 |= daddr; //0x000054F4
             }
             break;    
         case R_MIPS_LO16: //0x0000559C
             data = (u32) (((*(s32 *)daddr) << 16) >> 16) + segStart; //0x0000559C - 0x000055A8
             *(u32 *)daddr &= 0xFFFF0000; //0x000055AC & 0x000055B0
             *(u32 *)daddr |= data & 0xFFFF; //0x000055B8
             break;
         case R_MIPS_LITERAL: //0x000055F0
             Kprintf("********************\n"); //0x000055C4
             Kprintf("PSP cannot load this image\n"); //0x000055D0
             Kprintf("unacceptable relocation type: 0x%x\n", type); //0x000055E0
             break;
         default:
             Kprintf("********************\n"); //0x000055C4
             Kprintf("PSP cannot load this image\n"); //0x000055D0
             Kprintf("unacceptable relocation type: 0x%x\n", type); //0x000055E0
             return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
         }
    }
    return SCE_ERROR_OK;
}

//sub_000056B8
static s32 PspUncompress(u8 *modBuf, SceLoadCoreExecFileInfo *execInfo, 
                         u32 *newSize) 
{   
    PspHeader *header = (PspHeader *)modBuf;
    u8 checkCompAttr;
    u8 decryptMode;
    s32 status;
    SceSize decSize;
    u32 subType;
    u8 *tag;
    u32 i;
    
    checkCompAttr = SCE_FALSE;
    /* We have to save this because the ~PSP header will be overwritten by 
       the plain ELF data when the module is successfully decrypted. */
    decryptMode = header->decryptMode;

    /* Permit decryption modes dependent on the module/API type. */
    switch (execInfo->apiType) {
    case 0: case 2: case 32: case 33: case 81:
        if (((header->modAttribute & SCE_MODULE_PRIVILEGE_LEVELS) != SCE_MODULE_VSH || header->decryptMode != 3) && //0x00005D2C
          ((header->modAttribute & SCE_MODULE_PRIVILEGE_LEVELS) != SCE_MODULE_KERNEL || header->decryptMode - 1 >= 2) && //0x00005D08
          (header->modAttribute > SCE_MODULE_VSH || header->decryptMode != 4)) //0x0000578C   
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;     
        checkCompAttr = SCE_TRUE;
        break; //0x0000579C
    case 3:
        if ((header->modAttribute & SCE_MODULE_PRIVILEGE_LEVELS) != SCE_MODULE_USER || header->decryptMode != 4) //0x00005DA8 & 0x0000578C
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE; 
        checkCompAttr = SCE_TRUE;
        break; //0x0000579C
    case 16:
        if (((header->modAttribute & SCE_MODULE_PRIVILEGE_LEVELS) != SCE_MODULE_KERNEL || header->decryptMode - 1 >= 2) && //0x00005D6C
          ((header->modAttribute & SCE_MODULE_PRIVILEGE_LEVELS) != SCE_MODULE_USER || header->decryptMode != 4)) //0x00005D9C
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
        checkCompAttr = SCE_TRUE;
        break; //0x0000579C
    case 19:
        if ((header->modAttribute & SCE_MODULE_PRIVILEGE_LEVELS) != SCE_MODULE_USER || (header->decryptMode != 18 && //0x00005E30
          (header->decryptMode != 19 || sceKernelIsToolMode() == 0))) //0x00005E50 & 0x00005E38
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;  
        checkCompAttr = SCE_TRUE;
        break; //0x0000579C
    case 20:
        if ((header->modAttribute & SCE_MODULE_PRIVILEGE_LEVELS) != SCE_MODULE_USER || header->decryptMode != 23) //0x00005DEC
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
        checkCompAttr = SCE_TRUE;
        break; //0x0000579C
    case 17: case 48: case 66:
        if (((header->modAttribute & SCE_MODULE_PRIVILEGE_LEVELS) == SCE_MODULE_USER && header->decryptMode != 4) && //0x00005D60 & 0x00005790
          (((header->modAttribute & SCE_MODULE_PRIVILEGE_LEVELS) != SCE_MODULE_KERNEL) || header->decryptMode - 1 >= 2)) //0x00005D6C & 0x00005D14
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
        checkCompAttr = SCE_TRUE;
        break; //0x0000579C
    case 67:
        if ((header->modAttribute & SCE_MODULE_PRIVILEGE_LEVELS) != SCE_MODULE_APP || header->decryptMode != 22) //0x00005E1C & 0x00005E28
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
        checkCompAttr = SCE_TRUE;
        break; //0x0000579C
    case 80:
        if ((header->modAttribute & SCE_MODULE_PRIVILEGE_LEVELS) != SCE_MODULE_KERNEL || header->decryptMode - 1 >= 2) //0x00005D6C & 0x00005D1C
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
        checkCompAttr = SCE_TRUE;
        break; //0x0000579C
    case 112:
        //TODO: Check this.
        if ((sceKernelIsToolMode() == 0 || (header->modAttribute & SCE_MODULE_PRIVILEGE_LEVELS) == SCE_MODULE_APP) && //0x00005F98 & 0x00005FB4
          (header->modAttribute != SCE_MODULE_KERNEL || header->decryptMode - 1 >= 2) && //0x00005D08 & 0x00005D1C
          (header->modAttribute != SCE_MODULE_VSH || header->decryptMode != 3) && (header->modAttribute != 0 || 
           header->decryptMode != 4)) //0x00005D30
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
        checkCompAttr = SCE_TRUE;
        break; //0x0000579C
    case 272: case 274:
        if ((header->modAttribute & SCE_MODULE_PRIVILEGE_LEVELS) != SCE_MODULE_USER || header->decryptMode != 0) //0x00005F7C & 0x00005F88
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
        break; //0x000057B4
    case 275: case 277:
        if ((header->modAttribute & SCE_MODULE_PRIVILEGE_LEVELS) != SCE_MODULE_USER || header->decryptMode != 9) //0x00005F48
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
        break; //0x000057B4
    case 276: 
        if ((header->modAttribute & SCE_MODULE_PRIVILEGE_LEVELS) != SCE_MODULE_USER || header->decryptMode != 0) //0x00005F7C & 0x00005F88
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
        break; //0x000057B4
    case 278: case 280:
        if ((header->modAttribute & SCE_MODULE_PRIVILEGE_LEVELS) != SCE_MODULE_USER || header->decryptMode != 23) //0x00006044 & 0x00005F64
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
        break; //0x000057B4
    case 273: case 288:
        if ((header->modAttribute & SCE_MODULE_PRIVILEGE_LEVELS) != SCE_MODULE_USER || header->decryptMode != 9) //0x00005F50 & 0x00005F64
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
        break; //0x000057B4
    case 289:
        if ((header->modAttribute & SCE_MODULE_PRIVILEGE_LEVELS) != SCE_MODULE_VSH || header->decryptMode != 3) //0x00006308 & 0x00005790
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
        checkCompAttr = SCE_TRUE;
        break; //0x0000579C
    case 290:
        if (sceKernelIsToolMode() == 0 || (header->modAttribute & SCE_MODULE_PRIVILEGE_LEVELS) != SCE_MODULE_USER || //0x000060C0 & 0x000060D8
         ((header->decryptMode & 0xFF) != 0 && (header->decryptMode & 0xFF) != 9)) //0x000057B4 & 0x00005F64
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
        break; //0x000057B4
    case 305: case 307:
        if (sceKernelIsToolMode() == 0 || (header->modAttribute & SCE_MODULE_PRIVILEGE_LEVELS) != SCE_MODULE_USB_WLAN || //0x0000617C & 0x00006190
         (header->decryptMode - 10) >= 2) //0x00005D18
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
        checkCompAttr = SCE_TRUE;
        break; //0x0000579C
    case 304: case 306:
        if ((header->modAttribute & SCE_MODULE_PRIVILEGE_LEVELS) != SCE_MODULE_USB_WLAN || header->decryptMode != 10) //0x00006120
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
        checkCompAttr = SCE_TRUE;
        break; //0x00005798
    case 320:
        if ((header->modAttribute & SCE_MODULE_PRIVILEGE_LEVELS) != SCE_MODULE_VSH || header->decryptMode != 12) //0x00006244
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
        checkCompAttr = SCE_TRUE;
        break; //0x00005798
    case 325:
        if ((header->modAttribute & SCE_MODULE_PRIVILEGE_LEVELS) != SCE_MODULE_MS || header->decryptMode != 21) //0x000062A8 & 0x000062C0/0x00005790
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
        checkCompAttr = SCE_TRUE;
        break; //0x00005798
    case 323: case 340:
        if ((header->modAttribute & SCE_MODULE_PRIVILEGE_LEVELS) != SCE_MODULE_APP || header->decryptMode != 14) //0x00006224 & 0x000061F8
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
        
        status = CheckTick(modBuf); //0x00006200
        if (status < SCE_ERROR_OK) //0x00006208
            return status; 
        checkCompAttr = SCE_TRUE;
        break; //0x00005798
    case 324: case 341:
        if ((header->modAttribute & SCE_MODULE_PRIVILEGE_LEVELS) != SCE_MODULE_MS || header->decryptMode != 20) //0x000061C0
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
        checkCompAttr = SCE_TRUE;
        break; //0x00005790
    case 342:
        if ((header->modAttribute & SCE_MODULE_PRIVILEGE_LEVELS) != SCE_MODULE_MS || header->decryptMode != 21) //0x000062B0
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
        checkCompAttr = SCE_TRUE;
        break; //00005790

    //TODO: Check these
    case 291: case 292: case 294: case 353:
        if ((header->modAttribute & SCE_MODULE_PRIVILEGE_LEVELS) != SCE_MODULE_USER || header->decryptMode != 25) //0x0000613C & 0x00005F64
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
        break; //0x000057B4
    case 512: case 528: case 544: case 768:
        if ((header->modAttribute & SCE_MODULE_PRIVILEGE_LEVELS) != SCE_MODULE_VSH || header->decryptMode != 3) //0x00006314 & 0x00005D30 & 0x00005790
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
        checkCompAttr = SCE_TRUE;
        break; //0x00005798

    default:
        return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
    }

    if (checkCompAttr == SCE_TRUE && header->compAttribute & SCE_EXEC_FILE_ELF) //0x0000579C
        return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;

    /* Decrypt the executable file. */
    switch (header->decryptMode) { //0x000057D0
    case DECRYPT_MODE_NO_EXEC: 
        execInfo->isDecrypted = SCE_FALSE; //0x000057D8
        break;
    case DECRYPT_MODE_BOGUS_MODULE: case DECRYPT_MODE_KERNEL_MODULE:
        if ((header->modAttribute & SCE_MODULE_PRIVILEGE_LEVELS) != SCE_MODULE_KERNEL) //0x000058C8
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
        
        if (execInfo->isSignChecked == SCE_TRUE) { //0x000058D4
            if (g_prepareGetLengthFunc(modBuf, execInfo->execSize) != 0) //0x000058F0
                return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
            
            if (CheckLatestSubType(modBuf) == 0) //0x00005900
                return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
        }
        if (CheckLatestSubType(modBuf) != 0) { //0x00005908
            if (g_setMaskFunc == NULL) { //0x0000591C
                if (memlmd_F26A33C3(0, HWPTR(0xBFC00200)) != SCE_ERROR_OK) //0x0000598C
                    return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
            } else if (g_setMaskFunc(0, HWPTR(0xBFC00200)) != 0) //0x00005938
                    return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
        }
        if (g_getLengthFunc == NULL) { //0x00005944
            if (memlmd_EF73E85B(modBuf, execInfo->execSize, newSize) != SCE_ERROR_OK) //0x00005958
                return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;           
        } else if (g_getLengthFunc(modBuf, execInfo->execSize, newSize) != 0) //0x00005958
                return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
        
        execInfo->isDecrypted = SCE_TRUE;
        break; //0x000057DC
    case DECRYPT_MODE_VSH_MODULE:
        if ((header->modAttribute & SCE_MODULE_PRIVILEGE_LEVELS) != SCE_MODULE_VSH) //0x000059B0
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;

        if (execInfo->isSignChecked) { //0x000059BC
            if (memlmd_6192F715(modBuf, execInfo->execSize) != SCE_ERROR_OK) //0x000059CC
                return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
                       
            tag = (u8 *)&header->tag;
            subType = tag[0] << 24 | tag[1] << 16 | tag[2] << 8 | tag[3];
            if (g_compareSubType != NULL) { //0x0005A00
                if (g_compareSubType(subType) == 0) //0x00005A10
                    return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
            } else if (memlmd_2AE425D2(subType) == 0) //0x00005A30 & 0x00005A10
                    return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
        }
        if (sceUtilsGetLoadModuleCLength(modBuf, execInfo->execSize, newSize) != SCE_ERROR_OK) //0x00005958
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;

        execInfo->isDecrypted = SCE_TRUE;
        break; //0x000057DC
    case DECRYPT_MODE_USER_MODULE:
        if ((header->modAttribute & SCE_MODULE_PRIVILEGE_LEVELS) != SCE_MODULE_USER) //0x00005A48
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;

        if (execInfo->isSignChecked) { //0x00005A54
            if (memlmd_6192F715(modBuf, execInfo->execSize) != SCE_ERROR_OK) //0x00005A64
                return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
                   
            tag = (u8 *)&header->tag;
            subType = tag[0] << 24 | tag[1] << 16 | tag[2] << 8 | tag[3];
            if (g_compareSubType != NULL) { //0x00005A98
                if (g_compareSubType(subType) == 0) //0x00005AA8
                    return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
            } else if (memlmd_2AE425D2(subType) == 0) //0x00005AA8
                    return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
        }
        if (sceUtilsGetLoadModuleDLength(modBuf, execInfo->execSize, newSize) != SCE_ERROR_OK) //0x00005958
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
        
        execInfo->isDecrypted = SCE_TRUE;
        break; //0x57DC    
    case DECRYPT_MODE_UMD_GAME_EXEC:
        if ((header->modAttribute & SCE_MODULE_PRIVILEGE_LEVELS) != SCE_MODULE_USER) //0x00005AE0
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;        
                
        if (sceUtilsGetLoadModuleILength(modBuf, execInfo->execSize, newSize) != SCE_ERROR_OK) //0x00005958
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;

        execInfo->isDecrypted = SCE_TRUE;
        break; //0x000057DC
    case DECRYPT_MODE_GAMESHARING_EXEC:
        if ((header->modAttribute & SCE_MODULE_PRIVILEGE_LEVELS) != SCE_MODULE_USB_WLAN) //0x00005B0C
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
        
        if (sceUtilsGetLoadModuleJLength(modBuf, execInfo->execSize, newSize) != SCE_ERROR_OK) //0x00005958
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;

        execInfo->isDecrypted = SCE_TRUE;
        break; //0x000057DC
    case DECRYPT_MODE_GAMESHARING_EXEC_DEVTOOL:
        if (sceKernelIsToolMode() != SCE_ERROR_OK) //0x00005B34
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
                
        if ((header->modAttribute & SCE_MODULE_PRIVILEGE_LEVELS) != SCE_MODULE_USB_WLAN) //0x00005B48
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
                
        if (sceUtilsGetLoadModuleKLength(modBuf, execInfo->execSize, newSize) != SCE_ERROR_OK) //0x00005958
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
        
        execInfo->isDecrypted = SCE_TRUE;
        break; //0x000057DC
    case DECRYPT_MODE_MS_UPDATER:
        if ((header->modAttribute & SCE_MODULE_PRIVILEGE_LEVELS) != SCE_MODULE_VSH) //0x00005B74
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;

        if (sceUtilsGetLoadModuleLLength(modBuf, execInfo->execSize, newSize) != SCE_ERROR_OK) //0x00005958
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
        
        execInfo->isDecrypted = SCE_TRUE;
        break; //0x000057DC           
    case DECRYPT_MODE_DEMO_EXEC:
        if ((header->modAttribute & SCE_MODULE_PRIVILEGE_LEVELS) != SCE_MODULE_MS) //0x00005BA0
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;

        if (sceUtilsGetLoadModuleMLength(modBuf, execInfo->execSize, newSize) != SCE_ERROR_OK) //0x00005958
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;

        execInfo->isDecrypted = SCE_TRUE;
        break; //0x000057DC
    case DECRYPT_MODE_APP_MODULE:
        if ((header->modAttribute & SCE_MODULE_PRIVILEGE_LEVELS) != SCE_MODULE_APP) //0x00005BCC
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;

        if (sceUtilsGetLoadModuleNLength(modBuf, execInfo->execSize, newSize) != SCE_ERROR_OK) //0x00005958
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;

        execInfo->isDecrypted = SCE_TRUE;
        break; //0x000057DC
    case DECRYPT_MODE_MS_GAME_PATCH:
        if (sceUtilsGetLoadModuleRLength(modBuf, execInfo->execSize, newSize, execInfo->secureInstallId) != SCE_ERROR_OK) //0x00005958
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;

        execInfo->isDecrypted = SCE_TRUE;
        break; //0x000057DC
    case DECRYPT_MODE_MS_GAME_PATCH_DEVTOOL:
        if (sceUtilsGetLoadModuleSLength(modBuf, execInfo->execSize, newSize, execInfo->secureInstallId) != SCE_ERROR_OK) //0x00005958
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
        
        execInfo->isDecrypted = SCE_TRUE;
        break; //0x000057DC
    case DECRYPT_MODE_POPS_EXEC:
        if ((header->modAttribute & SCE_MODULE_PRIVILEGE_LEVELS) != SCE_MODULE_MS) //0x00005C30
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
                
        if (sceUtilsGetLoadModuleTLength(modBuf, execInfo->execSize, newSize, execInfo->secureInstallId) != SCE_ERROR_OK) //0x00005958
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;

        execInfo->isDecrypted = SCE_TRUE;
        break; //0x000057DC
    case DECRYPT_MODE_UNKNOWN_21:             
        if ((header->modAttribute & SCE_MODULE_PRIVILEGE_LEVELS) != SCE_MODULE_MS) //0x00005C60
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
                
        if (sceUtilsGetLoadModuleULength(modBuf, execInfo->execSize, newSize, execInfo->secureInstallId) != SCE_ERROR_OK) //0x00005958
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;

        execInfo->isDecrypted = SCE_TRUE;
        break; //0x000057DC
    case DECRYPT_MODE_UNKNOWN_22:
        if ((header->modAttribute & SCE_MODULE_PRIVILEGE_LEVELS) != SCE_MODULE_APP) //0x00005C90
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
                
        if (sceUtilsGetLoadModuleVLength(modBuf, execInfo->execSize, newSize) != SCE_ERROR_OK) //0x00005958
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;

        execInfo->isDecrypted = SCE_TRUE;
        break; //0x000057DC
    case DECRYPT_MODE_USER_NPDRM:
        if ((header->modAttribute & SCE_MODULE_PRIVILEGE_LEVELS) != SCE_MODULE_USER) //0x00005CB8
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;

        if (sceUtilsGetLoadModuleWLength(modBuf, execInfo->execSize, newSize, execInfo->secureInstallId) != SCE_ERROR_OK) //0x00005958
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;

        execInfo->isDecrypted = SCE_TRUE;
        break; //0x000057DC
    case DECRYPT_MODE_MS_GAME_PBOOT:
        if ((header->modAttribute & SCE_MODULE_PRIVILEGE_LEVELS) != SCE_MODULE_USER) //0x00005CE4
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
                
        if (sceUtilsGetLoadModuleYLength(modBuf, execInfo->execSize, newSize, execInfo->secureInstallId) != SCE_ERROR_OK) //0x00005958
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;

        execInfo->isDecrypted = SCE_TRUE;
        break; //0x000057DC
    default:
        return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;            
    }

    if ((execInfo->execAttribute & SCE_EXEC_FILE_COMPRESSED) == 0) { //0x000057E4
        if (decryptMode != DECRYPT_MODE_NO_EXEC) //0x000058AC
            return SCE_ERROR_OK; //0x5884
        else
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE; //0x000058B4
    }
    if (execInfo->topAddr == NULL) { //0x000057F0
        return SCE_ERROR_OK; 
    }
    if (execInfo->topAddr == (void *)SCE_KERNEL_VALUE_UNITIALIZED) {//0x00005800
        return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
    }
    
    if (decryptMode == DECRYPT_MODE_NO_EXEC) { // 0x00005810
        modBuf += 0x80;
    }
    /* Check decompression method of the executable. */
    if ((execInfo->execAttribute & 0xF00) == 0) { //0x0000580C
        /* GZIP compressed. */
        decSize = sceKernelGzipDecompress(execInfo->topAddr, execInfo->decSize, modBuf, NULL); //0x00005894
    } 
    else {
        /* KL4E compressed. */
        if ((execInfo->execAttribute & 0xF00) != SCE_EXEC_FILE_KL4E_COMPRESSED) //0x00005820
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
        
        /* Verify the signature of the header, must be "KL4E". */
        const char *ident = KL4E_COMPRESSION_SIGNATURE; //0x0000581C
        for (i = 0; i < 4; i++) //0x0000582C - 0x00005850
            if (modBuf[i] != ident[i])
                return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;

        decSize = UtilsForKernel_6C6887EE(execInfo->topAddr, execInfo->decSize, modBuf + 4, NULL); //0x00005860
    }

    if ((int)decSize < SCE_ERROR_OK) { //0x00005868
        return decSize; //0x00005884
    } else if (decSize != execInfo->decSize) { //0x0005874
        return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE; //0x587c
    }

    execInfo->isDecompressed = SCE_TRUE; //0x00005890
    return 0; //0x00005884
}

//sub_00006324
/*
 * Get the lowest start address of the file's segments (it will be used 
 * to set the entry address for the executable).  Also compute the largest
 * segment size and the maximal segment align info.
 */
static void readElfSegmentInfo(PspHeader *header, SceLoadCoreExecFileInfo *execInfo)
{
    u32 i;
    u32 maxSegEnd = 0;
    
    execInfo->modCodeSize = 0; //0x00006328
    execInfo->topAddr = (void *)header->segAddress[0]; //0x00006334
    execInfo->maxSegAlign = 0; //0x00006338
    
    for (i = 0; i < header->nSegments; i++) {
         if (header->segAddress[i] < (u32)execInfo->topAddr) {
             execInfo->topAddr = (void *)header->segAddress[i];
             
             if (execInfo->maxSegAlign < header->segAlign[i]) 
                 execInfo->maxSegAlign = header->segAlign[i];
             
             if (maxSegEnd < (header->segAddress[i] + header->segSize[i])) 
                 maxSegEnd = header->segAddress[i] + header->segSize[i];
        }
    }
    execInfo->modCodeSize = maxSegEnd - (u32)execInfo->topAddr;
}

//sub_000063C0
/*
 * Verify loadable program segments and update the object file's 
 * section data.  A plain ELF file on the PSP does not contain any 
 * special sections (other than PRX), so a loadable segment contains 
 * either a .text section or a .data section.
 * 
 * Returns 0 on success.
 */
static s32 CheckElfSection(Elf32_Ehdr *elfHeader, SceLoadCoreExecFileInfo *execInfo)
{
    Elf32_Phdr *progHeader;
    u32 i;
    
    //0x000063C4 - 0x000063D8
    if ((s32)elfHeader->e_phoff <= 0 || (s32)(((u32)elfHeader + elfHeader->e_phoff) ^ (u32)elfHeader) <= 0 || 
      ((s32)elfHeader + elfHeader->e_phoff) <= 0)    
        return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
    
    progHeader = (Elf32_Phdr *)(elfHeader + elfHeader->e_phoff);
    for (i = elfHeader->e_phnum; i != 0; i--) {
         if (progHeader[i].p_type != PT_LOAD) //0x000063F4
             continue;
            
         /* 
          * If the segment is executable, it contains the .text section.  Otherwise,
          * it contains the .data section. 
          */
         if (progHeader[i].p_flags & PF_X) //0x00006414              
             execInfo->textSize += progHeader[i].p_filesz; //0x00006418-0x00006428
         else
             execInfo->dataSize += progHeader[i].p_filesz; //0x0000644C - 0x00006458      
         
         /* Add the "extra" segment bytes to the .bss section. */
         if (progHeader[i].p_filesz < progHeader[i].p_memsz)
             execInfo->bssSize += progHeader[i].p_memsz - progHeader[i].p_filesz;
    } 
    return SCE_ERROR_OK;
}


//Subroutine sub_00006468 - Address 0x00006468
/*
 * Check the sections of a PRX module and add their sizes to the 
 * module segments which are TEXT, DATA and BSS.  Each of these 
 * three segments can be made up with multiple sections.
 * 
 * Returns 0 on success.
 */
static s32 CheckElfSectionPRX(Elf32_Ehdr *elfHeader, SceLoadCoreExecFileInfo *execInfo) 
{
    Elf32_Shdr *sectionHeader = NULL;
    Elf32_Shdr *sectionHeader2 = NULL;
    u32 i;
    u32 offset;   
    u32 sectHeaderEntries;
    
    if (elfHeader->e_shoff <= 0) //0x00006494
        return CheckElfSection(elfHeader, execInfo); //0x000066D4
    
    if (elfHeader->e_shnum == 0 || elfHeader->e_shstrndx >= elfHeader->e_shnum)
        return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
    
    sectionHeader = (Elf32_Shdr *)elfHeader + elfHeader->e_shoff; //0x00006498
    sectionHeader2 = sectionHeader + elfHeader->e_shstrndx;
    
    if (sectionHeader2->sh_offset <= 0) //0x000064D8
        return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
    
    offset = (sceKernelDipsw(10) != 0) ? 0x08000000 : 0x02000000; //0x000064EC
        
    sectHeaderEntries = elfHeader->e_shnum; //0x00006508
    //0x0000650C - 0x00006570
    for (i = 0; i < sectHeaderEntries; sectionHeader++, i++) {       
         if (sectionHeader2->sh_size < sectionHeader->sh_name) { //0x00006518
             Kprintf("%4d/%4d: max 0x%08x < sh_name 0x%08x\n", i, sectHeaderEntries, sectionHeader2->sh_size, sectionHeader->sh_name); //0x000066C4
             return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
         }   
         if (sectionHeader->sh_offset >= offset || sectionHeader->sh_size >= offset || 
          (sectionHeader->sh_offset + sectionHeader->sh_size) >= offset) { //0x00006534 & 0x00006540
             Kprintf("Invalid shdr\n"); //0x00006600
             Kprintf("ehdr->e_shoff     0x%08x\n", elfHeader->e_shoff); //0x00006610
             Kprintf("%4d/%4d: sh_name      0x%08x\n", i, sectHeaderEntries, sectionHeader->sh_name); //0x00006628
             Kprintf("%4d: sh_type      0x%08x\n", i, sectionHeader->sh_type); //0x0000663C
             Kprintf("%4d: sh_flags     0x%08x\n", i, sectionHeader->sh_flags); //0x00006654
             Kprintf("%4d: sh_offset    0x%08x\n", i, sectionHeader->sh_offset); //0x00006664
             Kprintf("%4d: sh_size    0x%08x\n", i, sectionHeader->sh_size); //0x00006678
             Kprintf("%4d: sh_addralign 0x%08x\n", i, sectionHeader->sh_addralign); //0x0000668C
             Kprintf("%4d: sh_entsize   0x%08x\n", i, sectionHeader->sh_entsize); //0x0000668C
        
             return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
         }
         
         /*
          * Add section sizes to the three module segments: 
          *    TEXT segment: Any section containing program data, occupying memory
          *                  and, optional, containing executable code.
          *    DATA segment: Any section containing program data, occupying memory
          *                  and containing writable data. 
          *    BSS segment:  Any section occupying no space in the file, occupying 
          *                  memory and containing writable data. 
          */
         if (sectionHeader->sh_type == SHT_PROGBITS) { //0x0000654C
             if ((sectionHeader->sh_flags == (SHF_EXECINSTR | SHF_ALLOC)) || (sectionHeader->sh_flags == SHF_ALLOC)) //0x000065C4 & 0x000065CC
                 execInfo->textSize += sectionHeader->sh_size; //0x000065F8
             else if (sectionHeader->sh_flags == (SHF_WRITE | SHF_ALLOC)) //0x000065D4
                 execInfo->dataSize += sectionHeader->sh_size; //0x000065E8
         } 
         else if ((sectionHeader->sh_type == SHT_NOBITS) && (sectionHeader->sh_flags == (SHF_WRITE | SHF_ALLOC))) { //0x0000655C
             execInfo->bssSize += sectionHeader->sh_size; //0x000065BC
         }
    }
    return SCE_ERROR_OK;  
}

//sub_000066E4
/*
 * Count the loadable segments of an object file.  A segment can 
 * contain several sections (such as .text, .data, .rodata, .bss).  
 * The object file cannot have more than SCE_KERNEL_MAX_MODULE_SEGMENT 
 * loadable segments.
 */
static s32 CheckElfImage(Elf32_Ehdr *elfHeader1 __attribute((unused)), Elf32_Ehdr *elfHeader2, Elf32_Phdr *progHeader) 
{ 
    u32 i;
    u32 offset;
    u32 segCount;
    
    offset = (sceKernelDipsw(10) == 0) ? 0x02000000 : 0x08000000; //0x000066FC 
    segCount = 0;           
    
    //0x00006718
    for (i = 0; i < elfHeader2->e_phnum; i++, progHeader++) {
         if (progHeader->p_type == PT_LOAD) { //0x00006738              
             if (offset < progHeader->p_offset || offset < progHeader->p_filesz || 
               offset < (progHeader->p_offset + progHeader->p_filesz)) //0x00006780
                 return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
             
             segCount++;    
        }
    }
    if (segCount >= SCE_KERNEL_MAX_MODULE_SEGMENT)
        return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
    
    return SCE_ERROR_OK;
}

//Subroutine sub_000067B8 - Address 0x000067B8
static s32 CheckTick(u8 *modBuf)
{
    s32 status;
    u64 currentTick;
    u64 expireTick;
    
    status = sceUtilsGetModuleExpireTick(modBuf, 0x160, &expireTick); //0x000067C4
    //getModuleParam error
    if (status < SCE_ERROR_OK) //0x000067E0
        return (status == (s32)SCE_ERROR_NOT_SUPPORTED) ? SCE_ERROR_OK : status; //0x000067D4
    
    status = sceKernelRtcGetTick(&currentTick); //0x000067E8
    if (status < SCE_ERROR_OK)
        return status;
    
    if (expireTick < currentTick) //0x00006808 - 0x00006810
        return 0x8001003E;

    return SCE_ERROR_OK;
}
