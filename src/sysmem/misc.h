#ifndef MISC_H
#define MISC_H

extern s32 gSysmemRandomValue;
extern u32 g_1453C;
extern u32 gSysmemModel;

void sub_A1E8(void);

void InitGameInfo(void);
void CopyGameInfo(SceKernelGameInfo *info);

#endif

