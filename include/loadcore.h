/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

typedef struct {
    s8   *libName; //0
    u8   version[2]; //4
    u16  attribute; //6
    u8   len; //8
    u8   vstubcount; //9
    u8   stubcount; //10
    void *entryTable; //12
    u16  unk16; //16
    u8   unk18; //18
    u8   unk19; //19
} SceLibraryEntryTable;

int sceKernelRegisterLibrary(SceLibraryEntryTable *lib);
int sceKernelGetModuleGPByAddressForKernel(void *addr);

