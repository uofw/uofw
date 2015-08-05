#include "../loadcore/loadelf.c"

#define numberof(n) (sizeof(n)/sizeof(*(n)))


/** 
 * Rule for overriding the file descriptor of a module, to force it to load from
 * a desired location (usually flash0:).
 * Some modules were first only provided in UMD discs, but Sony decided at some
 * point that they should instead be loaded from the PSP (most likely to run the
 * latest version available).
 */
typedef struct {
    /** The path of the module that will be loaded instead */
    char *overridePath; // 0
    /** The number of module hashes (most likely used to match different module versions) */
    u32 nHash; // 4
    /** List containing nHash hashes, each of the size of 16 bytes */
    u32 *hashList; // 8
} OverrideRule; // Size: 12


// 0x0000949C
u32 g_HashListAudiocodec260 [][] = {
    {0x7876376E, 0x872AFEEB, 0x361DF963, 0xD8F098BB},
    {0xFB64AA34, 0x56645CF2, 0x1DAEAD98, 0xB00D3C0E},
    {0xDD5F48DA, 0x11783859, 0x2DC39F5B, 0x3F8D84ED},
    {0x20323A8F, 0x3B2C8CDB, 0x51B05818, 0x0FA89AA6},
    {0xA7B3D98B, 0x0464AA5C, 0x35E60616, 0x717B7262},
    {0x5539BBDB, 0x00C3B31B, 0x2467FC5D, 0x4579D766},
    {0xB1EBB191, 0xD325E72D, 0xAB99FA28, 0x8C0A52A8},
    {0xF42ECE92, 0xFA7C6927, 0xFB510A2F, 0xF783B5F4}
}

// 0x0000951C
u32 g_HashListAvcodec [][] = {
    {0xEB97A208, 0x98CCC35B, 0x56E55510, 0x63EEA61B}
}

// 0x0000952C
u32 g_HashListCertLoader[][] = {
    {0xEB97A208, 0x98CCC35B, 0x56E55510, 0x63EEA61B},
    {0xAA9F884F, 0x301F3F6C, 0x03EC68E1, 0x7AF22856},
    {0x658533EC, 0xEF9945FC, 0x51917DA6, 0x1DC51A72},
    {0xF01B5846, 0x65390520, 0x81F6DB31, 0x282021D2},
    {0x9EF2B510, 0x2CD6537A, 0x7019D1D6, 0x092AE96D}
}

// 0x0000957C
u32 g_HashListIfhandle[][] = {
    {0xE78C22A9, 0xD6D24E98, 0x0826F6F5, 0x85AF086B},
    {0xE03022E8, 0xE1AA30BF, 0x9C6A19DA, 0x98639D7F},
    {0x2BCA7A19, 0x0C7A8D82, 0x3537FCD6, 0x2F6D849B},
    {0x03C03988, 0x09517354, 0x440C0B6B, 0xBFEA08B0},
    {0x6DF2D214, 0x767A3E51, 0x2F887742, 0x7F0866A0}
}

// 0x00095DC
u32 g_HashListMemab[][] = {
    {0x5CEAA1D7, 0x74A363FF, 0x6DB369E0, 0x6BD19534},
    {0x978734BC, 0xE35F4DB4, 0x1D60F483, 0xA67774CA},
    {0x95B8EA41, 0xF39178DD, 0xF16840C0, 0x13001E64}
}

// 0x0x0000960C
u32 g_HashListMpegbase260[][] = {
    {0xFA82ED38, 0xCD2A8263, 0x3FC77BDE, 0xE9D61EDC},
    {0x1D758D52, 0x74FB358F, 0xA17BA41F, 0x86A29BD5},
    {0xDCDE4585, 0xDEC1EC4E, 0xDADA9CC0, 0x69700702},
    {0x10D80729, 0xBB96A47C, 0xF022301C, 0x44C24BD5},
    {0x5A84E1A8, 0x730B5E61, 0x6A66832A, 0x81921676},
    {0xE61A1541, 0x03AB87B1, 0xFB76D0A7, 0xA7306A7F},
    {0x4FE63CA5, 0xF05D8191, 0x762E9556, 0xCA96B5B8},
    {0x448D1EB8, 0x4B73E6DF, 0x2D4018A4, 0x858A7DC3},
    {0xD706B7E3, 0x12869525, 0x75013660, 0xD7D1804A}
}

// 0x0000969C
u32 g_HashListPspnetAdhocAuth[][] = {
    {0x9A341D9A, 0x9181E91B, 0x6FDB531A, 0x04277939},
    {0x8BC2C3FD, 0x92447E47, 0x9D815AC7, 0x86D8059E},
    {0x956B25E3, 0xBAC6BB52, 0xBC0FD1DD, 0xE031A975},
    {0xA7861ADB, 0xB8FF4240, 0x41836328, 0x692D3927},
    {0xE57023CC, 0xADDF02AE, 0x02CFA543, 0xACD407E0},
    {0xF7D1C4A5, 0x52EFD710, 0x108DD8D2, 0x31A69D6E}
}

// 0x000096FC
u32 g_HashListScSascore[][] = {
    {0xD5378E43, 0xC35EEB6B, 0xB6E6BED6, 0x5D3C1B84},
    {0xF6E2A9F5, 0x0C8C6F5E, 0x6A00C84C, 0x133E38B4},
    {0x9A57DA9F, 0xA867593E, 0xF381A5D4, 0x301FBAC3},
    {0xE7422100, 0x8F810797, 0x9DE915CB, 0x95D82C99},
    {0x3AD38266, 0x69188BE9, 0xAF8A4094, 0x03EE89EC},
    {0xACF0E562, 0x91273C80, 0xB5567866, 0x8C7B168A},
    {0xEF4CDC26, 0x2BC68F9B, 0x817929F8, 0xF58910CA},
    {0x141D1D63, 0xD6FB060B, 0x3BBB2041, 0x0F5D3F93},
    {0x50906B1E, 0x8F4C2CD5, 0xBCF9B2C2, 0xBA768623}
}

// 0x0000978C
u32 g_HashListUsbacc[][] = {
    {0x38B71CE8, 0xD8A55E81, 0x6E316F97, 0x1D897CD4},
    {0x370B99BE, 0x4C39E7C1, 0x164EBB86, 0xA4902852},
    {0x8AE90692, 0xBA16A146, 0xC13F5F95, 0x20BBEA10}
}

// 0x000097BC
u32 g_HashListUsbgps[][] = {
    {0x02419066, 0x2F763CFC, 0x390E5248, 0x374D6E95}
}

// 0x000097CC
u32 g_HashListUsbmic[][] = {
    {0x23C19622, 0x8417EB95, 0x3D6FF272, 0x90CE5A63},
    {0xE1EB3B2E, 0x02D9D5EC, 0x235F070B, 0xAF34E772},
    {0xD24B82AB, 0xC5B9C50B, 0x71CBB494, 0x37E8752D}
}

// 0x000097FC
u32 g_HashListUsbpspcm[][] = {
    {0xECA51E6C, 0x2151DDE2, 0xE1C6C2FE, 0xA51EE0A6},
    {0xEB4211E4, 0x111BD3A8, 0x85F9DFC6, 0x754CEE44},
    {0x4A78E41A, 0x632E82C6, 0x5E17AA7A, 0x38F72BDB},
    {0x0740646F, 0x1261388E, 0x3732479E, 0x42A086F9},
    {0x4C5A9C76, 0xD4B69C2A, 0xC5F23FD1, 0x6ED2FCC8},
    {0xC8ECB894, 0xAAA64C73, 0xF4F48635, 0xCAB6BB25},
    {0xFA82E5F0, 0xA313B44B, 0xAED52C22, 0x9AF8C66F},
    {0x1EDFAB4C, 0xFCE9495D, 0xEEA594D1, 0xC800E61A}
}

// 0x0000987C
u32 g_HashListVideocodec260[][] = {
    {0x7B542C1F, 0x78B1BD53, 0x7E8DA5F9, 0x88764DB4},
    {0x6A5D777C, 0x05B78E83, 0xEF2A46B0, 0xF310B2C0},
    {0x030725A2, 0x203B2FAC, 0x9B870806, 0x438AA370},
    {0x0E026711, 0xB0D7767C, 0xBFE36838, 0x9D5DC16D},
    {0xA58B4A70, 0xCC49BF56, 0xC8945D42, 0x57B26C39},
    {0x44E2DD0F, 0x589BE5F0, 0xD99560F0, 0xA52E9BDE},
    {0xA96F1646, 0x7C294E21, 0xFCEF2951, 0x46B9DB61},
    {0x73345004, 0xAA9BD0E2, 0xF668F4F5, 0x1B681E70}
}

// This is the list containing every override rule
OverrideRule g_OverrideList[] = { // 0x000098FC
    [0] = {
        .overridePath = "flash0:/kd/audiocodec_260.prx";
        .nHash = numberof(g_HashListAudiocodec260); // = 8
        .hashList = g_HashListAudiocodec260;
    },
    [1] = {
        .overridePath = "flash0:/kd/avcodec.prx";
        .nHash = numberof(g_HashListAvcodec); // = 1
        .hashList = g_HashListAvcodec;
    },
    [2] = {
        .overridePath = "flash0:/kd/cert_loader.prx";
        .nHash = numberof(g_HashListCertLoader); // = 5
        .hashList = g_HashListCertLoader;
    },
    [3] = {
        .overridePath = "flash0:/kd/ifhandle.prx";
        .nHash = numberof(g_HashListIfhandle); // = 6
        .hashList = g_HashListIfhandle;
    },
    [4] = {
        .overridePath = "flash0:/kd/memab.prx";
        .nHash = numberof(g_HashListMemab); // = 3
        .hashList = g_HashListMemab;
    },
    [5] = {
        .overridePath = "flash0:/kd/mpegbase_260.prx";
        .nHash = numberof(g_HashListMpegbase260); // = 9
        .hashList = g_HashListMpegbase260;
    },
    [6] = {
        .overridePath = "flash0:/kd/pspnet_adhoc_auth.prx";
        .nHash = numberof(g_HashListPspnetAdhocAuth); // = 6
        .hashList = g_HashListPspnetAdhocAuth;
    },
    [7] = {
        .overridePath = "flash0:/kd/sc_sascore.prx";
        .nHash = numberof(g_HashListScSascore); // = 9
        .hashList = g_HashListScSascore;
    },
    [8] = {
        .overridePath = "flash0:/kd/usbacc.prx";
        .nHash = numberof(g_HashListUsbacc); // = 3
        .hashList = g_HashListUsbacc;
    },
    [8] = {
        .overridePath = "flash0:/kd/usbgps.prx";
        .nHash = numberof(g_HashListUsbgps); // = 1
        .hashList = g_HashListUsbgps;
    },
    [9] = {
        .overridePath = "flash0:/kd/usbmic.prx";
        .nHash = numberof(g_HashListUsbmic); // = 3
        .hashList = g_HashListUsbmic;
    },
    [10] = {
        .overridePath = "flash0:/kd/usbpspcm.prx";
        .nHash = numberof(g_HashListUsbpspcm); // = 8
        .hashList = g_HashListUsbpspcm;
    },
    [11] = {
        .overridePath = "flash0:/kd/videocodec_260.prx";
        .nHash = numberof(g_HashListVideocodec260); // = 8
        .hashList = g_HashListVideocodec260;
    }
};

// This function checks if the module provided in pBuffer is known (using its hash),
// and return the file descriptor from which the module will be loaded instead
s32 _CheckOverride(s32 apiType, void *pBuffer, SceUID *fd) // 0x00008568
{
    
    if (apiType != 0x30 && apiType != 0x10) { // 0x000085B0
        return SCE_ERROR_OK;
    }

    // Let's assume we were provided a PspHeader
    PspHeader* pspHeader = pBuffer;

    // If this is not a PspHeader but a SceHeader
    if ((u32)pspHeader.magic == SCE_MAGIC) { // 0x000085C4, from loadelf.h
        // Skip the SCE header
        pspHeader += (SceHeader *)pspHeader.size;
    }

    // If this is still not a PspHeader, we can't fix anything
    if((u32)pspHeader.magic != PSP_MAGIC) { // 0x000085DC, from loadcore_int.h
        return SCE_ERROR_OK;
    }


    u32 i;
    // Try to find if the hash of our module matches the one of our rules
    for (i=0; i <= 13; i++) { // 0x00008630, 0x00008684, 0x0000868C

        // Get the current rule
        OverrideRule curRule = g_OverrideList[i*12];

        // Get the number of hashes of the current rule
        u32 curNHash = curRule.nHash;

        // No hash available? Try the next rule
        if (curNHash <= 0) {
            continue;
        }

        u32 j;
        // Check the hash of our module against the list of hashes of the current rule
        for (j=0; j < curNHash; j++) {

            // Current hash to check against
            u32 curHash = curRule.hashList[j];

            // Compare the hash of the module, with the current hash
            int sameHash = !memcmp(curHash, curHash, KEY_DATA_SIZE); // KEY_DATA_SIZE is from loadelf.h

            // Phew! A hash from the list of the current rule is the same as the one from the module
            if (sameHash) {
                // Override the file descriptor of the module with our own
                fd = sceIoOpen(entry_offset.0, 0x04000001, 0);

                return SCE_ERROR_OK;
            }
        }
    }
}