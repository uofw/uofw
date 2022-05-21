/* Copyright (C) 2011, 2012, 2013 The uOFW team
   See the file COPYING for copying permission.
*/

#include <common_imp.h>

#include <sysmem_kernel.h>
#include <sysmem_sysclib.h>
#include <sysmem_utils_kernel.h>

#include "gamePatching.h"
#include "libcUtils.h"

#define IS_JUMP(op) (((op) & 0xFC000000) == 0x0C000000)
#define JUMP(ref, addr) (((ref) & 0xF0000000) | (((addr) & 0x03FFFFFFF) << 2))

#define MEDAL_OF_HONORS_HEROES_GAME_ID_US               "ULUS10141"
#define MEDAL_OF_HONORS_HEROES_GAME_ID_UK               "ULAS42082"
#define MEDAL_OF_HONORS_HEROES_GAME_ID_EU               "ULES00557"
#define MEDAL_OF_HONORS_HEROES_GAME_ID_FR               "ULES00558"
#define MEDAL_OF_HONORS_HEROES_GAME_ID_DE               "ULES00559"
#define MEDAL_OF_HONORS_HEROES_GAME_ID_EU_PLATIN        "ULES00560"
#define MEDAL_OF_HONORS_HEROES_GAME_ID_SP               "ULES00561"
#define MEDAL_OF_HONORS_HEROES_GAME_ID_NL               "ULES00562"
#define MEDAL_OF_HONORS_HEROES_GAME_ID_KOR              "ULKS46066"
#define MEDAL_OF_HONORS_HEROES_GAME_ID_JAP              "ULJM05213"

typedef struct {
    char *gameId;
    s32 **strcpy256;
    s32 numStrcpy256;
    s32 **wcscpy256;
    s32 numWcscpy256;
} SceJumpFixup;

s32 *g_2840[] = { (s32 *)0x089C07AC };
s32 *g_2844[] = { 
    (s32 *)0x0888EA48, (s32 *)0x0888EA84, (s32 *)0x0888EAB4, (s32 *)0x0888EAF0, (s32 *)0x0888EB20, (s32 *)0x0888EB88, 
    (s32 *)0x0888EBB8
};
s32 *g_2860[] = { (s32 *)0x089C08F0 };
s32 *g_2864[] = { 
    (s32 *)0x0888EA40, (s32 *)0x0888EA7C, (s32 *)0x0888EAAC, (s32 *)0x0888EAE8, (s32 *)0x0888EB18, (s32 *)0x0888EB80, 
    (s32*)0x0888EBB0
};
s32 *g_2880[] = { (s32 *)0x089C0928 };
s32 *g_2884[] = { 
    (s32 *)0x0888EA48, (s32 *)0x0888EA84, (s32 *)0x0888EAB4, (s32 *)0x0888EAF0, (s32 *)0x0888EB20, (s32 *)0x0888EB88, 
    (s32 *)0x0888EBB8
};

#define INIT_PATCH_GAMES_NUM        (10)
// 22E4
SceJumpFixup jumpFixups[INIT_PATCH_GAMES_NUM] = {
    {MEDAL_OF_HONORS_HEROES_GAME_ID_US, g_2840, 1, g_2844, 7},
    {MEDAL_OF_HONORS_HEROES_GAME_ID_EU, g_2840, 1, g_2844, 1},
    {MEDAL_OF_HONORS_HEROES_GAME_ID_FR, g_2840, 1, g_2844, 1},
    {MEDAL_OF_HONORS_HEROES_GAME_ID_DE, g_2840, 1, g_2844, 1},
    {MEDAL_OF_HONORS_HEROES_GAME_ID_EU_PLATIN, g_2840, 1, g_2844, 1},
    {MEDAL_OF_HONORS_HEROES_GAME_ID_SP, g_2840, 1, g_2844, 1},
    {MEDAL_OF_HONORS_HEROES_GAME_ID_NL, g_2840, 1, g_2844, 1},
    {MEDAL_OF_HONORS_HEROES_GAME_ID_UK, g_2880, 1, g_2884, 1},
    {MEDAL_OF_HONORS_HEROES_GAME_ID_KOR, g_2880, 1, g_2884, 1},
    {MEDAL_OF_HONORS_HEROES_GAME_ID_JAP, g_2860, 1, g_2864, 1}
};

void patchGames(void)
{   
    SceKernelGameInfo *gameInfo;
    s32 lastStrcpyOp, lastWcscpyOp;
    
    gameInfo = sceKernelGetGameInfo();
    lastStrcpyOp = 0;
    lastWcscpyOp = 0;
    
    if (gameInfo == NULL)
        return;
    
    s32 j;
    for (j = 0; j < INIT_PATCH_GAMES_NUM; j++) {
        if (strcmp(gameInfo->gameId, jumpFixups[j].gameId) == 0) {
            s32 i;
            for (i = 0; i < jumpFixups[j].numStrcpy256; i++) {
                s32 *ptr = jumpFixups[j].strcpy256[i];
                if (IS_JUMP(*ptr)) {
                    lastStrcpyOp = JUMP((s32)ptr, *ptr);
                    *ptr = JUMP(0, SCE_USERSPACE_ADDR_KU0);
                }
            }
            for (i = 0; i < jumpFixups[j].numWcscpy256; i++) {
                s32 *ptr = jumpFixups[j].wcscpy256[i];
                if (IS_JUMP(*ptr)) {
                    lastWcscpyOp = JUMP((s32)ptr, *ptr);
                    *ptr = JUMP(0, SCE_USERSPACE_ADDR_KU0 + (u32)wcscpy256 - (u32)strcpy256);
                }
            }
        }
    }
    if (lastWcscpyOp != 0 || lastStrcpyOp != 0) {
        memcpy((void *)SCE_USERSPACE_ADDR_KU0, strcpy256, (u32)&patchGames - (u32)strcpy256);
    }
    sceKernelDcacheWritebackAll();
}
