#include <common_imp.h>
#include <crypto/kirk.h>
#include <threadman_kernel.h>

SCE_MODULE_INFO("sceChkreg", SCE_MODULE_KERNEL | SCE_MODULE_KIRK_SEMAPHORE_LIB | SCE_MODULE_ATTR_EXCLUSIVE_LOAD
                              | SCE_MODULE_ATTR_EXCLUSIVE_START, 1, 9);
SCE_SDK_VERSION(SDK_VERSION);

SceUID g_chkreg_sema;    // 0x00001538
u8 g_buf[KIRK_CERT_LEN]; // 0x00001480

typedef struct {
    u32 unk;     // DAT_00000040
    u32 unk1;    // DAT_00000044
    u32 unk2;    // DAT_00000048
    u32 unk3;    // DAT_00000b40
    void *buf;   // DAT_afbf0010
    u32 unk4;    // DAT_afbf0014
    u32 unk5;    // DAT_afbf0028
    u32 unk6;    // DAT_afbf0038
} g_chkreg_struct;

typedef struct {
    u8 *code;
} g_chkreg_pscode_struct;

g_chkreg_struct g_chkreg = { 0, 0, 0, 0, NULL, 0, 0, 0 };
g_chkreg_pscode_struct g_chkreg_pscode = { NULL };

// Declarations
s32 sceIdStorageReadLeaf(u16 key, void *buf);
s32 sceIdStorageLookup(u16, u32, void*, u32);
s32 sceUtilsBufferCopyWithRange(void* outBuf, s32 outsize, void* inBuf, s32 insize, s32 cmd);

// Subroutine sub_00000000 - Address 0x00000000 -- TODO
s32 sub_00000000(void)
{
    return 0;
}

// Subroutine sub_00000128 - Address 0x00000128 -- TODO
s32 sub_00000128(s32 arg0, s32 arg1)
{
    u32 unk1 = 0;
    s32 unk2 = 0;
    u32 unk3 = 0;
    s32 *pUnk = (s32 *)0x130;
    
    if (g_chkreg.unk != 0) {
        unk2 = 0x3c020000;
        while(1) {
            unk1 = unk3 << 3;
            unk3 = unk3 + 1;
            pUnk = pUnk + 2;
            
            if ((arg1 == unk2) && (arg0 == *(s32 *)((unk1 | 4) + 0x130)))
                return 1;
            
            if (g_chkreg.unk <= unk3)
                break;
                
            unk2 = *pUnk;
        }
    }

    return 0;
}

// Subroutine sub_00000190 - Address 0x00000190 -- Done
s32 sub_00000190(void)
{
    if (sceIdStorageLookup(0x100, 0x38, g_buf, KIRK_CERT_LEN) < 0) { // Read pscode into g_buf from key 0x100
        if (sceIdStorageLookup(0x120, 0x38, g_buf, KIRK_CERT_LEN) < 0) // Read (backup) pscode into g_buf from key 0x120 if key 0x100 fails
            return SCE_ERROR_NOT_FOUND;
    }
    
    g_chkreg.unk2 = 1; // Setting some sort of flag?
    return 0;
}

// Subroutine sub_0000020C - Address 0x0000020C -- Done
s32 sub_0000020C(void)
{
    if (sceUtilsBufferCopyWithRange(NULL, 0, g_buf, KIRK_CERT_LEN, KIRK_CMD_CERT_VER) != 0) // Validate g_buf
        return SCE_ERROR_INVALID_FORMAT;
        
    return 0;
}


// Subroutine module_start - Address 0x00000248 -- TODO
s32 module_start(SceSize args __attribute__((unused)), void *argp __attribute__((unused)))
{   
    g_chkreg.unk = 0;
    g_chkreg.unk1 = 0;
    g_chkreg.unk2 = 0;
    g_chkreg_sema = sceKernelCreateSema("SceChkreg", 0, 1, 1, NULL);
    return (g_chkreg_sema < 1);
}

// Subroutine module_stop - Address 0x000002E0 -- TODO
s32 module_stop(SceSize args __attribute__((unused)), void *argp __attribute__((unused)))
{
    s32 ret = 0;
    SceUInt timeout = 1000000;
    
    g_chkreg.unk = 0;
    g_chkreg.unk1 = 0;
    g_chkreg.unk2 = 0;

    if ((ret = sceKernelWaitSema(g_chkreg_sema, 1, &timeout)) == 0)
        sceKernelDeleteSema(g_chkreg_sema);
    
    return (ret != 0);
}

// Subroutine sceChkreg_driver_59F8491D - Address 0x00000438 (sceChkregGetPsCode) -- Done
s32 sceChkreg_driver_59F8491D(u8 *code)
{
    s32 ret = SCE_ERROR_SEMAPHORE;

    if ((sceKernelWaitSema(g_chkreg_sema, 1, 0)) == 0) {
        if (((g_chkreg.unk2 != 0) || ((ret = sub_00000190()) == 0)) && ((ret = sub_0000020C()) == 0)) {
            code[0] = g_chkreg_pscode.code[1];
            code[1] = g_chkreg_pscode.code[0];
            code[2] = g_chkreg_pscode.code[3];
            code[3] = g_chkreg_pscode.code[2];
            code[4] = g_chkreg_pscode.code[5];
            code[5] = g_chkreg_pscode.code[4];
            code[6] = g_chkreg_pscode.code[6] >> 2;
            code[7] = 0;
        }
        
        if ((sceKernelSignalSema(g_chkreg_sema, 1)) != 0)
            ret = SCE_ERROR_SEMAPHORE;
    }
    
    return ret;
}

// Subroutine sceChkreg_driver_54495B19 - Address 0x00000390 (sceChkregCheckRegion) -- Done
s32 sceChkreg_driver_54495B19(u32 arg0, u32 arg1)
{
    s32 ret = 0;
    
    if (sceKernelWaitSema(g_chkreg_sema, 1, NULL) == 0) {
        if ((g_chkreg.unk1 != 0) || ((ret = sub_00000000()) == 0))
            ret = sub_00000128(0x80000000, (arg0 | arg1));
        
        if (sceKernelSignalSema(g_chkreg_sema, 1) != 0)
            ret = SCE_ERROR_SEMAPHORE;
    }

    return ret;
}

// Subroutine sceChkreg_driver_6894A027 - Address 0x000006B8 -- TODO
s32 sceChkreg_driver_6894A027(u8 *arg0, s32 arg1) {
    s32 ret = SCE_ERROR_INVALID_INDEX;

    if (arg1 == 0) {
        ret = SCE_ERROR_SEMAPHORE;

        if ((sceKernelWaitSema(g_chkreg_sema, 1, 0)) == 0) {
            if (((g_chkreg.unk2 != 0) || ((ret = sub_00000190()) == 0)) && ((ret = sub_0000020C()) == 0)) {
                if (((u32)g_chkreg_pscode.code[6] << 0x18) >> 0x1a == 0x23)
                    *arg0 = g_chkreg_pscode.code[6] << 6 | g_chkreg_pscode.code[7] >> 2;
                else
                    ret = SCE_ERROR_INVALID_VALUE;
            }
            
            if ((sceKernelSignalSema(g_chkreg_sema, 1)) != 0)
                ret = SCE_ERROR_SEMAPHORE;
        }
    }
    
    return ret;
}

// Subroutine sceChkreg_driver_7939C851 - Address 0x0000079C
s32 sceChkreg_driver_7939C851(void) {
    s32 ret = 0;
    u8 code[4];
    u32 unk = 0;
    
    // TODO: Fix this, it doesn't seem right
    ret = sceChkreg_driver_59F8491D(code);
    if (ret == 0) {
        ret = 0;
        
        switch(unk) {
            case 0:
                ret = 0;
                break;
            
            case 1:
            case 2:
                ret = 1;
                break;
                
            case 3:
                ret = 2;
                break;
                
            case 4:
            case 6:
            case 8:
                ret = 3;
                break;
                
            case 5:
            case 7:
            case 9:
                ret = 5;
        }
    }
    
    return ret;
}

// Subroutine sceChkreg_driver_9C6E1D34 - Address 0x0000051C -- TODO
s32 sceChkreg_driver_9C6E1D34(void) {
    return 0;
}
