/* Copyright (C) 2011, 2012, 2013 The uOFW team    
   See the file COPYING for copying permission.
*/

#include "usersystemlib_int.h"

s32 sub_00000208(s32 dlId  __attribute__((unused)), void *stall  __attribute__((unused)))
{
    /* The syscall instruction is patched at runtime by ge.prx to run sceGeListUpdateStallAddr */
    asm __volatile__ (
        ".set noat\n"
        ".set noreorder\n"
        "jr          $ra\n"
        "syscall     0x0\n"
    );

    /* Never reached */
    return 0;
}

// This function replaces sceGeListUpdateStallAddr() in a patched game (Gensou Suikoden I&II)
s32 sceGe_lazy_31129B95(s32 dlId, void *stall)
{
    s32 ret;

    // Updating the same display list stall address again: update g_lazy.stall and skip if count < max (= 100)
    if (dlId == g_lazy.dlId) {
        g_lazy.stall = stall;

        if (stall != NULL) {
            g_lazy.count++;

            if (g_lazy.count < g_lazy.max) {
                return 0;
            }
        }
    }

    // Update the stall address of the last display list and reset g_lazy.dlId
    if (g_lazy.dlId >= 0) {
        SceBool idErr = 0;

        do {
            if (pspLl(&g_lazy.dlId) != g_lazy.dlId) {
                idErr = 1;
                break;
            }
        } while (!pspSc(-1, &g_lazy.dlId));

        if (!idErr) {
            sub_00000208(g_lazy.dlId, g_lazy.stall);
        }
    }

    // Update the specified display list stall address
    ret = sub_00000208(dlId, stall);

    if (ret < 0) {
        return ret;
    }

    // Set the 'lazy' status to the display list we updated the stall address of
    do {
        if (pspLl(&g_lazy.dlId) >= 0) {
            return ret;
        }
    } while (!pspSc(dlId, &g_lazy.dlId));

    g_lazy.stall = stall;
    g_lazy.count = 0;

    return ret;
}
