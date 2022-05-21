/* Copyright (C) 2011 - 2015 The uOFW team
   See the file COPYING for copying permission.
*/

#include <common_imp.h>

/* The total number of available chunks. */
#define SCE_KERNEL_NUM_CHUNKS       (16)
/* The slot of the highest available chunk. */
#define SCE_KERNEL_MAX_CHUNK        (SCE_KERNEL_NUM_CHUNKS - 1)

/* The group of chunks to be used by the system. */
SceUID chunks[SCE_KERNEL_NUM_CHUNKS]; //0x00009A48

// sub_000086C0
void ChunkInit(void)
{
    u32 i;
    for (i = 0; i < SCE_KERNEL_NUM_CHUNKS; i++)
        chunks[i] = SCE_KERNEL_VALUE_UNITIALIZED;
}

SceUID sceKernelGetChunk(s32 chunkId)
{
    if (chunkId < 0 || chunkId > SCE_KERNEL_MAX_CHUNK)
        return SCE_ERROR_KERNEL_ILLEGAL_CHUNK_ID;
    return chunks[chunkId];
}

SceUID sceKernelRegisterChunk(s32 chunkId, SceUID blockId)
{
    if (chunkId < 0 || chunkId > SCE_KERNEL_MAX_CHUNK)
        return SCE_ERROR_KERNEL_ILLEGAL_CHUNK_ID;
    
    chunks[chunkId] = blockId;
    return blockId;
}

s32 sceKernelReleaseChunk(s32 chunkId)
{
    if (chunkId < 0 || chunkId > SCE_KERNEL_MAX_CHUNK)
        return SCE_ERROR_KERNEL_ILLEGAL_CHUNK_ID;
    
    chunks[chunkId] = SCE_KERNEL_VALUE_UNITIALIZED;
    return SCE_KERNEL_VALUE_UNITIALIZED;
}

