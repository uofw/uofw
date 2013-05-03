/* Copyright (C) 2011, 2012, 2013 The uOFW team
   See the file COPYING for copying permission.
*/

#include <common_imp.h>

/* The total number of available chunks. */
#define INIT_NUM_CHUNKS                 (16)
/* The slot of the highest available chunk. */
#define INIT_MAX_CHUNK                  (INIT_NUM_CHUNKS - 1)

/* Indicates a chunk is currently not used by the system. */
#define INIT_CHUNK_NOT_USED             (-1)

/* The group of chunks to be used by the system. */
s32 chunks[INIT_NUM_CHUNKS]; //0x00009A48

//sub_000086C0
static void ChunkInit(void)
{
    u32 i;
    for (i = 0; i < INIT_NUM_CHUNKS; i++)
        chunks[i] = INIT_CHUNK_NOT_USED;
}

SceUID sceKernelGetChunk(SceUID chunkId)
{
    if (chunkId < 0 || chunkId > INIT_MAX_CHUNK)
        return SCE_ERROR_KERNEL_ILLEGAL_CHUNK_ID;
    return chunks[chunkId];
}

SceUID sceKernelRegisterChunk(SceUID chunkId, SceUID blockId)
{
    if (chunkId < 0 || chunkId > INIT_MAX_CHUNK)
        return SCE_ERROR_KERNEL_ILLEGAL_CHUNK_ID;
    
    chunks[chunkId] = blockId;
    return blockId;
}

SceUID sceKernelReleaseChunk(SceUID chunkId)
{
    if (chunkId < 0 || chunkId > INIT_MAX_CHUNK)
        return SCE_ERROR_KERNEL_ILLEGAL_CHUNK_ID;
    
    chunks[chunkId] = INIT_CHUNK_NOT_USED;
    return INIT_CHUNK_NOT_USED;
}

