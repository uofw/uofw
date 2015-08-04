#include <sysmem_kdebug.h>
#include <sysmem_kernel.h>
#include <sysmem_sysclib.h>

#include "heap.h"
#include "intr.h"
#include "memory.h"
#include "partition.h"

typedef struct SceSysmemHoldElem {
    struct SceSysmemHoldElem *next; // 0
    struct SceSysmemHoldElem *prev; // 4
} SceSysmemHoldElem;

typedef struct {
    SceSysmemHoldElem hold0; // 0
    SceSysmemHoldElem hold1; // 8
    u32 count1; // 16
    u32 count2; // 20
} SceSysmemHoldHead; // size: 24

typedef struct {
    SceSysmemHoldElem hold0; // 0
    SceSysmemHoldElem hold1; // 8
    SceSysmemUidCB *uid1; // 16
    SceSysmemUidCB *uid0; // 20
} SceSysmemHoldElement;

s32 obj_no_op(SceSysmemUidCB *uid, SceSysmemUidCB *uidWithFunc, int funcId, va_list ap);
s32 obj_do_delete(SceSysmemUidCB *uid, SceSysmemUidCB *uidWithFunc, int funcId, va_list ap);
s32 obj_do_delete2(SceSysmemUidCB *uid, SceSysmemUidCB *uidWithFunc, int funcId, va_list ap);
s32 obj_do_name(SceSysmemUidCB *uid, SceSysmemUidCB *uidWithFunc, int funcId, va_list ap);
s32 obj_do_type(SceSysmemUidCB *uid, SceSysmemUidCB *uidWithFunc, int funcId, va_list ap);
s32 obj_do_isKindOf(SceSysmemUidCB *uid, SceSysmemUidCB *uidWithFunc, int funcId, va_list ap);
s32 obj_do_isMemberOf(SceSysmemUidCB *uid, SceSysmemUidCB *uidWithFunc, int funcId, va_list ap);
s32 obj_do_doseNotRecognize(SceSysmemUidCB *uid, SceSysmemUidCB *uidWithFunc, int funcId, va_list ap);
s32 obj_do_initialize(SceSysmemUidCB *uid, SceSysmemUidCB *uidWithFunc, int funcId, va_list ap);

void InitUidBasic(SceSysmemUidCB *root, char *rootName, SceSysmemUidCB *metaRoot, char *metaRootName, SceSysmemUidCB *basic, char *basicName, SceSysmemUidCB *metaBasic, char *metaBasicName);
SceSysmemUidCB *search_UidType_By_Name(const char *name);
SceSysmemUidCB *search_UIDObj_By_Name_With_Type(const char *name, SceSysmemUidCB *type);

// 140D8
SceSysmemHeap EHeap4UID;

// 140EC
SceSysmemUidCB *HeapCB;

// 145AC
SceUID HeapCBUid;

// 145B0
SceSysmemUidList g_uidTypeList;

void InitUid(void)
{
    SceSysmemHeapBlock *heapBlock = NULL;
    int oldIntr = suspendIntr();
    HeapCB = NULL;
    HeapCBUid = 0;
    _CreateHeap(MpidToCB(1), 0x2000, 1, &heapBlock);
    sceKernelProtectMemoryBlock(MpidToCB(1), heapBlock);
    EHeap4UID.partId = 1;
    EHeap4UID.firstBlock = heapBlock;
    EHeap4UID.size = 0x2000;
    SceSysmemPartitionInfo part;
    part.size = 16;
    sceKernelQueryMemoryPartitionInfo(1, &part);
    EHeap4UID.partAddr = part.startAddr;
    EHeap4UID.partSize = part.memSize;
    SceSysmemUidCB *root = _AllocHeapMemory(&EHeap4UID, sizeof(SceSysmemUidCB), 0);
    // A7FC
    memset(root, 0, sizeof(SceSysmemUidCB));
    SceSysmemUidCB *metaRoot = _AllocHeapMemory(&EHeap4UID, sizeof(SceSysmemUidCB), 0);
    // A820
    memset(metaRoot, 0, sizeof(SceSysmemUidCB));
    SceSysmemUidCB *basic = _AllocHeapMemory(&EHeap4UID, sizeof(SceSysmemUidCB), 0);
    // A844
    memset(basic, 0, sizeof(SceSysmemUidCB));
    SceSysmemUidCB *metaBasic = _AllocHeapMemory(&EHeap4UID, 36, 0);
    // A868
    memset(metaBasic, 0, sizeof(SceSysmemUidCB));
    char *rootName = _AllocHeapMemory(&EHeap4UID, strlen("Root") + 1, 0);
    char *metaRootName = _AllocHeapMemory(&EHeap4UID, strlen("MetaRoot") + 1, 0);
    char *basicName = _AllocHeapMemory(&EHeap4UID, strlen("Basic") + 1, 0);
    char *metaBasicName = _AllocHeapMemory(&EHeap4UID, strlen("MetaBasic") + 1, 0);
    InitUidBasic(root, rootName, metaRoot, metaRootName, basic, basicName, metaBasic, metaBasicName);
    HeapInit();
    if (sceKernelCreateUID(g_HeapType, "SceSystemMemoryManager", 0, &HeapCB) == 0)
    {
        SceSysmemHeap *heap = UID_CB_TO_DATA(HeapCB, g_HeapType, SceSysmemHeap);
        heap->size = EHeap4UID.size;
        heap->partId = EHeap4UID.partId;
        heap->partSize = EHeap4UID.partSize;
        heap->firstBlock = EHeap4UID.firstBlock;
        heap->partAddr = EHeap4UID.partAddr;
        HeapCBUid = HeapCB->uid;
    }
    // A968
    resumeIntr(oldIntr);
    return;
}

void *AllocSceUIDtypeCB(void)
{
    int *buf = _AllocHeapMemory(&EHeap4UID, 36, 0);
    if (buf != NULL)
    {
        // A9B8
        int i;
        for (i = 0; i < 36; i++)
            buf[i] = 0;
    }
    return buf;
}

int FreeSceUIDtypeCB(void *uidType)
{
    _FreeHeapMemory(&EHeap4UID, uidType);
    return 0;
}

SceSysmemUidCB *AllocSceUIDobjectCB(s32 size)
{
    SceSysmemUidCB *uid = _AllocHeapMemory(&EHeap4UID, size, 0);
    if (uid != NULL)
        sceKernelMemset32(uid, 0, size);
    return uid;
}

s32 FreeSceUIDobjectCB(SceSysmemUidCB *uid)
{
    _FreeHeapMemory(&EHeap4UID, uid);
    return 0;
}

char *AllocSceUIDnamestr(const char *str1, const char *str2)
{
    u32 len1 = strnlen(str1, 31);
    u32 len2 = strnlen(str2, 31);
    if (len1 + len2 >= 32)
        return NULL;
    char *buf = _AllocHeapMemory(&EHeap4UID, len1 + len2 + 1, 0);
    if (buf == NULL)
        return buf;
    memcpy(buf, str1, len1);
    memcpy(buf + len1, str2, len2);
    buf[len1 + len2] = '\0';
    return buf;
}

int FreeSceUIDnamestr(char *name)
{
    _FreeHeapMemory(&EHeap4UID, name);
    return 0;
}

SceSysmemHoldHead *AllocSceUIDHoldHead(void)
{
    SceSysmemHoldHead *head = _AllocHeapMemory(&EHeap4UID, sizeof(SceSysmemHoldHead), 0);
    if (head != NULL)
        memset(head, 0, sizeof(*head));
    return head;
}

s32 FreeSceUIDHoldHead(SceSysmemHoldHead *head)
{
    _FreeHeapMemory(&EHeap4UID, head);
    return 0;
}

SceSysmemHoldElement *AllocSceUIDHoldElement(void)
{
    SceSysmemHoldElement *elem = _AllocHeapMemory(&EHeap4UID, sizeof(SceSysmemHoldElement), 0);
    if (elem != NULL)
        memset(elem, 0, sizeof(*elem));
    return elem;
}

s32 FreeSceUIDHoldElement(SceSysmemHoldElement *elem)
{
    _FreeHeapMemory(&EHeap4UID, elem);
    return 0;
}

s32 sceKernelCallUIDFunction(SceUID id, int funcId, ...)
{
    va_list ap;
    va_start(ap, funcId);
    s32 oldIntr = suspendIntr();
    SceSysmemUidCB *uid = (SceSysmemUidCB*)(0x88000000 | (((u32)id >> 7) << 2));
    if (id < 0) {
        // ACAC (dup)
        resumeIntr(oldIntr);
        return 0x800200CB;
    }
    if (uid->uid != id) {
        // ACAC (dup)
        resumeIntr(oldIntr);
        return 0x800200CB;
    }
    // ACD8
    SceSysmemUidFunc func = NULL;
    SceSysmemUidCB *parentUid;
    sceKernelLookupUIDFunction(uid->meta, funcId, &func, &parentUid);
    s32 ret;
    if (func == NULL) {
        // AD1C
        ret = sceKernelCallUIDObjFunction(uid, 0xB9970352, funcId);
    } else
        ret = func(uid, parentUid, funcId, ap);
    // AD0C
    resumeIntr(oldIntr);
    return ret;
}

s32 sceKernelCallUIDObjFunction(SceSysmemUidCB *uid, s32 funcId, ...)
{
    va_list ap;
    va_start(ap, funcId);
    SceSysmemUidFunc func = NULL;
    SceSysmemUidCB *parentUid;
    s32 oldIntr = suspendIntr();
    sceKernelLookupUIDFunction(uid->meta, funcId, &func, &parentUid);
    s32 ret;
    if (func == NULL) {
        // ADCC
        ret = sceKernelCallUIDObjFunction(uid, 0xB9970352, funcId);
    } else
        ret = func(uid, parentUid, funcId, ap);
    // ADA8
    resumeIntr(oldIntr);
    va_end(ap);
    return ret;
}

int sceKernelLookupUIDFunction(SceSysmemUidCB *uid, int id, SceSysmemUidFunc *func, SceSysmemUidCB **parentUidWithFunc)
{
    int oldIntr = suspendIntr();
    *func = NULL;
    // AE18
    while (uid != NULL) {
        if (uid->funcTable != NULL) {
            SceSysmemUidLookupFunc *cur = uid->funcTable;
            // AE30
            while (cur->func != NULL) {
                if (cur->id == id) {
                    // AE80
                    *parentUidWithFunc = uid;
                    *func = cur->func;
                    resumeIntr(oldIntr);
                    return 0;
                }
                cur++;
            }
        }
        // (AE48)
        uid = uid->PARENT1;
        // AE4C
    }
    // AE54
    resumeIntr(oldIntr);
    return 0x800200CE;
}

s32 sceKernelCallUIDObjCommonFunction(SceSysmemUidCB *uid, SceSysmemUidCB *uidWithFunc, s32 funcId, va_list ap)
{
    SceSysmemUidCB *parentWithFunc = uidWithFunc;
    s32 oldIntr = suspendIntr();
    SceSysmemUidFunc func;
    sceKernelLookupUIDFunction(uidWithFunc->PARENT1, funcId, &func, &parentWithFunc);
    s32 ret;
    if (func == NULL) {
        // 0AF2C
        ret = sceKernelCallUIDObjFunction(uid, 0xB9970352, funcId);
    } else
        ret = func(uid, parentWithFunc, funcId, ap);
    // AF04
    resumeIntr(oldIntr);
    return ret;
}

int sceKernelCreateUIDtypeInherit(const char *parentName, const char *name, int size,
                                 SceSysmemUidLookupFunc *funcTable,
                                 SceSysmemUidLookupFunc *metaFuncTable,
                                 SceSysmemUidCB **uidTypeOut)
{
    if (parentName == NULL || name == NULL)
        return 0x80020001;
    int oldIntr = suspendIntr();
    if (search_UidType_By_Name(name) != 0)
    {
        // B1A8
        resumeIntr(oldIntr);
        return 0x800200C8;
    }
    SceSysmemUidCB *parentUidType = search_UidType_By_Name(parentName);
    if (parentUidType == NULL)
    {
        // B194
        resumeIntr(oldIntr);
        return 0x800200C9;
    }
    SceSysmemUidCB *uidType = AllocSceUIDtypeCB();
    SceSysmemUidCB *metaUidType = AllocSceUIDtypeCB();
    char *namePtr = AllocSceUIDnamestr(name, "");
    char *metaName = AllocSceUIDnamestr("Meta", name);
    if (namePtr == NULL || metaName == NULL || uidType == NULL || metaUidType == NULL)
    {
        // B024
        if (uidType != NULL)
            FreeSceUIDtypeCB(uidType);
        // B034
        if (metaUidType != NULL)
            FreeSceUIDtypeCB(metaUidType);
        // B044
        if (namePtr != NULL)
            FreeSceUIDnamestr(namePtr);
        // B054
        if (metaName != NULL)
            FreeSceUIDnamestr(metaName);
        // B064
        resumeIntr(oldIntr);
        return 0x80020190;
    }
    // B0A4
    parentUidType->meta->next.numChild++;
    uidType->PARENT0 = uidType;
    uidType->uid = ((int)uidType << 5) | ((g_uidTypeList.count & 0x3F) << 1) | 1;
    uidType->nextChild = uidType;
    g_uidTypeList.count++;
    metaUidType->uid = ((int)metaUidType << 5) | ((g_uidTypeList.count & 0x3F) << 1) | 1;
    metaUidType->PARENT0 = metaUidType;
    g_uidTypeList.count++;
    metaUidType->nextChild = metaUidType;
    uidType->meta = metaUidType;
    uidType->childSize = parentUidType->childSize + ((u32)(size + 3) >> 2);
    metaUidType->meta = g_uidTypeList.metaRoot;
    uidType->size = parentUidType->childSize;
    metaUidType->childSize = 6;
    metaUidType->size = 0;
    uidType->name = namePtr;
    metaUidType->name = metaName;
    uidType->next.next = g_uidTypeList.root->next.next;
    uidType->PARENT1 = parentUidType;
    g_uidTypeList.root->next.next = uidType;
    metaUidType->next.numChild = 0;
    metaUidType->PARENT1 = parentUidType->meta;
    uidType->funcTable = funcTable;
    *uidTypeOut = uidType;
    metaUidType->funcTable = metaFuncTable;
    resumeIntr(oldIntr);
    return 0;
}

int sceKernelCreateUID(SceSysmemUidCB *type, const char *name, char k1, SceSysmemUidCB **outUid)
{
    if (type == NULL || name == NULL)
        return 0x80020001;
    int oldIntr = suspendIntr();
    *outUid = NULL;
    SceSysmemUidCB *uid = AllocSceUIDobjectCB(type->childSize * 4);
    if (uid == NULL) {
        // B2F8
        resumeIntr(oldIntr);
        return 0x80020190;
    }
    char *nameBuf = AllocSceUIDnamestr(name, "");
    if (nameBuf == NULL) {
        // B2EC
        FreeSceUIDobjectCB(uid);
        resumeIntr(oldIntr);
        return 0x80020190;
    }
    uid->attr = k1;
    *outUid = uid;
    uid->uid = ((int)uid << 5) | ((g_uidTypeList.count & 0x3F) << 1) | 1;
    uid->nextChild = type->nextChild;
    type->nextChild = uid;
    uid->PARENT0 = type;
    g_uidTypeList.count++;
    uid->nextChild->PARENT0 = uid;
    uid->name = nameBuf;
    uid->size = type->size;
    uid->childSize = type->childSize;
    uid->meta = type;
    sceKernelCallUIDObjFunction(uid, 0xD310D2D9);
    resumeIntr(oldIntr);
    return 0;
}

SceUID sceKernelSearchUIDbyName(const char *name, SceUID typeId)
{
    SceSysmemMemoryPartition *part = g_MemInfo.kernel;
    if (name == NULL)
        return 0x80020001;
    s32 oldIntr = suspendIntr();
    SceSysmemUidCB *uid;
    if (typeId == 0) {
        // B438
        SceSysmemUidCB *cur = g_uidTypeList.root;
        // B444
        do {
            uid = search_UIDObj_By_Name_With_Type(name, cur);
            if (uid != NULL)
                break;
            cur = cur->next.next;
        } while (cur != g_uidTypeList.root);
    } else {
        SceSysmemUidCB *typeUid = (SceSysmemUidCB*)(0x88000000 | (((u32)typeId >> 7) * 4));
        if ((typeId & 0x80000001) != 1 || (u32)typeUid < part->addr ||
            (u32)typeUid >= part->addr + part->size || typeUid->uid != typeId) {
            // B3A4
            resumeIntr(oldIntr);
            return 0x800200CB;
        }
        // B3D8
        if (typeUid->meta->meta != g_uidTypeList.metaRoot || typeUid->meta == g_uidTypeList.root) { // B404
            // B3F4
            resumeIntr(oldIntr);
            return 0x800200C9;
        }
        uid = search_UIDObj_By_Name_With_Type(name, typeUid);
    }
    // (B41C)
    // B420
    resumeIntr(oldIntr);
    if (uid == NULL)
        return 0x800200CD;
    return uid->uid;
}

SceSysmemUidCB *search_UidType_By_Name(const char *name)
{
    SceSysmemUidCB *cur = g_uidTypeList.root;
    // B47C
    do
    {
        char *curStr = cur->name;
        const char *curName = name;
        if (name == curStr)
            return cur;
        if (name != NULL && curStr != NULL) {
            // B4BC, B4C0
            while (*(curStr++) == *(curName))
                if (*(curName++) == '\0')
                    return cur;
        }
        cur = cur->next.next;
        // B4AC
    } while (cur != g_uidTypeList.root);
    return 0;
}

SceSysmemUidCB *search_UIDObj_By_Name_With_Type(const char *name, SceSysmemUidCB *type)
{
    SceSysmemUidCB *parent = type->PARENT0;
    // B4FC
    while (parent != type) {
        if (name == parent->name)
            return parent;
        if (name != NULL && parent->name != NULL) {
            const char *curName = name;
            char *curParentName = parent->name;
            char lastChar = *curParentName;
            // B53C, B540
            while (lastChar == *(curName++)) {
                if (lastChar == '\0')
                    return parent;
                lastChar = *(curParentName++);
            }
        }
        type = type->PARENT0;
        // B52C
    }
    return 0;
}

// 13570
SceSysmemUidLookupFunc RootFuncs[] = {
    { 0xD310D2D9, obj_no_op },
    { 0x973A5367, obj_no_op },
    { 0x285422D4, obj_no_op },
    { 0x87089863, obj_do_delete },
    { 0x86D94883, obj_no_op },
    { 0xF0ADE1B6, obj_do_name },
    { 0x58D965CE, obj_do_type },
    { 0x9AFB14E2, obj_do_isKindOf },
    { 0xE19A43D1, obj_do_isMemberOf },
    { 0xB9970352, obj_do_doseNotRecognize },
    { 0, NULL }
};

// 135C8
SceSysmemUidLookupFunc MetaRootFuncs[] = {
    { 0x87089863, obj_no_op },
    { 0, NULL }
};

void InitUidRoot(SceSysmemUidCB *root, char *rootName, SceSysmemUidCB *metaRoot, char *metaRootName)
{
    int oldIntr = suspendIntr();
    g_uidTypeList.metaRoot = metaRoot;
    g_uidTypeList.root = root;
    root->meta = metaRoot;
    g_uidTypeList.count = 1;
    g_uidTypeList.metaRoot->meta = g_uidTypeList.metaRoot;
    root->PARENT0 = root;
    g_uidTypeList.root->uid = ((int)g_uidTypeList.root << 5) | 3;
    root->nextChild = root;
    g_uidTypeList.count++;
    g_uidTypeList.metaRoot->uid = ((int)g_uidTypeList.metaRoot << 5) | ((g_uidTypeList.count & 0x3F) << 1) | 1;
    metaRoot->PARENT0 = metaRoot;
    metaRoot->nextChild = metaRoot;
    g_uidTypeList.count++;
    strcpy(rootName, "Root");
    strcpy(metaRootName, "MetaRoot");
    g_uidTypeList.metaRoot->childSize = 6;
    g_uidTypeList.root->childSize = 6;
    g_uidTypeList.root->name = rootName;
    g_uidTypeList.metaRoot->size = 0;
    g_uidTypeList.root->size = 0;
    g_uidTypeList.metaRoot->name = metaRootName;
    g_uidTypeList.root->next.next = g_uidTypeList.root;
    g_uidTypeList.metaRoot->next.numChild = 0;
    g_uidTypeList.root->PARENT1 = NULL;
    g_uidTypeList.metaRoot->PARENT1 = g_uidTypeList.root;
    g_uidTypeList.root->funcTable = RootFuncs;
    g_uidTypeList.metaRoot->funcTable = MetaRootFuncs;
    resumeIntr(oldIntr);
}

int _sceKernelCreateUIDtypeInherit_sub(SceSysmemUidCB *root, SceSysmemUidCB *basic, SceSysmemUidCB *metaBasic,
                                    char *basicName, char *metaBasicName, int size, SceSysmemUidLookupFunc *funcTable,
                                    SceSysmemUidLookupFunc *metaFuncTable, SceSysmemUidCB **outUidType)
{
    basic->meta = metaBasic;
    root->meta->next.numChild++;
    basic->PARENT0 = basic;
    basic->uid = ((int)basic << 5) | ((g_uidTypeList.count & 0x3F) << 1) | 1;
    basic->nextChild = basic;
    g_uidTypeList.count++;
    metaBasic->uid = ((int)metaBasic << 5) | ((g_uidTypeList.count & 0x3F) << 1) | 1;
    metaBasic->PARENT0 = metaBasic;
    g_uidTypeList.count++;
    metaBasic->nextChild = metaBasic;
    basic->name = basicName;
    basic->childSize += (u32)(size + 3) >> 2;
    metaBasic->meta = g_uidTypeList.metaRoot;
    basic->size = root->childSize;
    metaBasic->childSize = 6;
    metaBasic->size = 0;
    metaBasic->name = metaBasicName;
    basic->next.next = g_uidTypeList.root->next.next;
    basic->PARENT1 = root;
    g_uidTypeList.root->next.next = basic;
    metaBasic->next.numChild = 0;
    metaBasic->PARENT1 = root->meta;
    basic->funcTable = funcTable;
    *outUidType = basic;
    metaBasic->funcTable = metaFuncTable;
    return 0;
}

int sceKernelCreateUIDtype(const char *name, int size, SceSysmemUidLookupFunc *funcTable,
                           SceSysmemUidLookupFunc *metaFuncTable, SceSysmemUidCB **uidTypeOut)
{
    return sceKernelCreateUIDtypeInherit("Basic", name, size, funcTable, metaFuncTable, uidTypeOut);
}

s32 sceKernelDeleteUIDtype(SceSysmemUidCB *uid)
{
    s32 oldIntr = suspendIntr();
    if (uid->PARENT0 != uid || uid->meta->next.next != NULL) {
        // B8B0
        resumeIntr(oldIntr);
        return 0x800200CA;
    }
    SceSysmemUidCB *prev = g_uidTypeList.root;
    SceSysmemUidCB *cur = prev->next.next;
    while (cur != uid) { // B854
        prev = cur;
        cur = cur->next.next;
    }
    // B868
    prev->next.next = uid->next.next;
    FreeSceUIDnamestr(uid->name);
    FreeSceUIDnamestr(uid->meta->name);
    FreeSceUIDtypeCB(uid->meta);
    FreeSceUIDtypeCB(uid);
    resumeIntr(oldIntr);
    return 0;
}

s32 sceKernelGetUIDname(SceUID id, s32 len, char *out)
{
    SceSysmemMemoryPartition *part = g_MemInfo.kernel;
    s32 oldIntr = suspendIntr();
    SceSysmemUidCB *uidType = (void*)(0x88000000 | (((u32)id >> 7) * 4));
    if ((id & 0x80000001) != 1 || (u32)uidType < part->addr ||
        (u32)uidType >= part->addr + part->size || uidType->uid != id) {
        // B948
        resumeIntr(oldIntr);
        return 0x800200CB;
    }
    // B974
    char *name = uidType->name;
    if (len >= 2) {
        char lastChar = *(name++);
        while (lastChar != '\0') { // B98C
            len--;
            *(out++) = lastChar;
            if (len < 2)
                break;
            lastChar = *(name++);
        }
    }
    // B9AC
    *out = '\0';
    // B9B0
    resumeIntr(oldIntr);
    return 0;
}

s32 sceKernelRenameUID(SceUID id, const char *name)
{
    SceSysmemMemoryPartition *part = g_MemInfo.kernel;
    s32 oldIntr = suspendIntr();
    SceSysmemUidCB *uid = (SceSysmemUidCB*)(0x88000000 | (((u32)id >> 7) * 4));
    if ((id & 0x80000001) != 1 || (u32)uid < part->addr ||
        (u32)uid >= part->addr + part->size || uid->uid != id) {
        // BA48
        resumeIntr(oldIntr);
        return 0x800200CB;
    }
    // BA78
    FreeSceUIDnamestr(uid->name);
    char *newName = AllocSceUIDnamestr(name, "");
    if (newName == NULL) {
        // BAAC
        resumeIntr(oldIntr);
        return 0x80020190;
    }
    uid->name = newName;
    resumeIntr(oldIntr);
    return 0;
}

s32 sceKernelGetUIDtype(SceUID id)
{
    SceSysmemUidCB *uid = (SceSysmemUidCB *)(0x88000000 | (((u32)id >> 7) * 4));
    SceSysmemMemoryPartition *part = g_MemInfo.kernel;
    if ((id & 0x80000001) != 1 || (u32)uid < part->addr ||
        (u32)uid >= part->addr + part->size || uid->uid != id)
        return 0x800200CB;
    // BB2C
    return uid->meta->uid;
}

s32 sceKernelDeleteUID(SceUID id)
{
    return sceKernelCallUIDFunction(id, 0x87089863);
}

s32 sceKernelGetUIDcontrolBlock(SceUID id, SceSysmemUidCB **uidOut)
{
    SceSysmemUidCB *uid = (SceSysmemUidCB *)(0x88000000 | (((u32)id >> 7) << 2));
    SceSysmemMemoryPartition *part = g_MemInfo.kernel;
    if ((id & 0x80000001) != 1 || (u32)uid < part->addr ||
        (u32)uid >= part->addr + part->size || uid->uid != id)
        return 0x800200CB;
    *uidOut = uid;
    return 0;
}

s32 sceKernelGetUIDcontrolBlockWithType(SceUID id, SceSysmemUidCB *type, SceSysmemUidCB **outUid)
{
    s32 oldIntr = suspendIntr();
    SceSysmemUidCB *uid = (SceSysmemUidCB*)(0x88000000 | (((u32)id >> 7) * 4));
    SceSysmemMemoryPartition *part = g_MemInfo.kernel;
    if ((id & 0x80000001) != 1 || (u32)uid < part->addr ||
        (u32)uid >= part->addr + part->size || uid->uid != id) {
        // BC50
        resumeIntr(oldIntr);
        return 0x800200CB;
    }
    // BC7C
    if (uid->meta != type) {
        // BC98
        resumeIntr(oldIntr);
        return 0x800200CC;
    }
    *outUid = uid;
    resumeIntr(oldIntr);
    return 0;
}

s32 sceKernelIsKindOf(SceSysmemUidCB *uid, SceSysmemUidCB *type)
{
    SceSysmemUidCB *cur = uid->meta;
    while (cur != NULL && cur != type) { // BCCC
        cur = cur->next.next;
        // BCE4
    }
    return (cur == type);
}

s32 sceKernelPrintUidListAll(void)
{
    s32 oldIntr = suspendIntr();
    Kprintf("<< UID list >>\n");
    SceSysmemUidCB *uid = g_uidTypeList.root->next.next;
    while (uid != g_uidTypeList.basic) { // BD4C
        Kprintf("\n[%s]   UID 0x%08x (attribute 0x%x)\n", uid->name, uid->uid, uid->attr);
        if (uid->nextChild == uid) {
            // BDE4
            Kprintf("    <No UID objects>\n");
        }
        // BD68
        SceSysmemUidCB *curChild = uid->nextChild;
        while (curChild != uid) { // BD78
            Kprintf("  --  (Name): %31s, (UID): 0x%08x, (attr): 0x%x\n", curChild->name, curChild->uid, curChild->attr);
            curChild = curChild->nextChild;
        }
        // BD94
        uid = uid->next.next;
    }
    // BDA4
    Kprintf("\n  *** end of list ***\n");
    resumeIntr(oldIntr);
    return 0;
}

SceSysmemUidList *sceKernelGetUidmanCB(void)
{
    return &g_uidTypeList;
}

s32 obj_no_op(SceSysmemUidCB *uid, SceSysmemUidCB *uidWithFunc __attribute__((unused)), int funcId __attribute__((unused)), va_list ap __attribute__((unused)))
{
    return uid->uid;
}

s32 obj_do_delete(SceSysmemUidCB *uid, SceSysmemUidCB *uidWithFunc __attribute__((unused)), int funcId __attribute__((unused)), va_list ap __attribute__((unused)))
{
    uid->meta = NULL;
    uid->PARENT0->nextChild = uid->nextChild;
    uid->uid = 0;
    uid->nextChild->PARENT0 = uid->PARENT0;
    uid->nextChild = uid;
    uid->PARENT0 = uid;
    FreeSceUIDnamestr(uid->name);
    FreeSceUIDobjectCB(uid);
    return 0;
}

s32 obj_do_delete2(SceSysmemUidCB *uid, SceSysmemUidCB *uidWithFunc, int funcId, va_list ap)
{
    SceSysmemHoldHead *head = *UID_CB_TO_DATA(uid, g_uidTypeList.basic, SceSysmemHoldHead *);
    if (head != NULL) {
        if (head->count1 > 0) {
            SceSysmemHoldElem *cur = head->hold0.next;
            while (cur != &head->hold0) {
                // BEC8
                SceSysmemHoldElement *elem = (SceSysmemHoldElement*)cur;
                SceSysmemHoldElem *formerNext8 = elem->hold1.next;
                SceSysmemHoldElem *formerPrev8 = elem->hold1.prev;
                formerNext8->prev = formerPrev8;
                elem->hold1.prev = &elem->hold1;
                formerPrev8->next = formerNext8;
                elem->hold1.next = &elem->hold1;

                SceSysmemHoldElem *formerNext0 = elem->hold0.next;
                SceSysmemHoldElem *formerPrev0 = elem->hold0.prev;
                formerNext0->prev = formerPrev0;
                elem->hold0.prev = &elem->hold0;
                formerPrev0->next = formerNext0;
                elem->hold0.next = &elem->hold0;
                FreeSceUIDHoldElement(elem);

                cur = formerNext0;
            }
        }
        // BF1C
        if (head->count2 > 0) {
            SceSysmemHoldElem *cur = head->hold1.next;
            while (cur != &head->hold1) {
                // BF30
                SceSysmemHoldElement *elem = (SceSysmemHoldElement*)((void*)cur - 8);
                SceSysmemHoldElem *formerNext8 = elem->hold1.next;
                SceSysmemHoldElem *formerPrev8 = elem->hold1.prev;
                formerNext8->prev = formerPrev8;
                elem->hold1.prev = &elem->hold1;
                formerPrev8->next = formerNext8;
                elem->hold1.next = &elem->hold1;

                SceSysmemHoldElem *formerNext0 = elem->hold0.next;
                SceSysmemHoldElem *formerPrev0 = elem->hold0.prev;
                formerNext0->prev = formerPrev0;
                elem->hold0.prev = &elem->hold0;
                formerPrev0->next = formerNext0;
                elem->hold0.next = &elem->hold0;
                FreeSceUIDHoldElement(elem);

                cur = formerNext8;
            }
        }
        // BF7C
        FreeSceUIDHoldHead(head);
    }
    // BF84
    return sceKernelCallUIDObjCommonFunction(uid, uidWithFunc, funcId, ap);
}

s32 obj_do_name(SceSysmemUidCB *uid, SceSysmemUidCB *uidWithFunc __attribute__((unused)), int funcId __attribute__((unused)), va_list ap)
{
    s32 length = va_arg(ap, s32);
    char *str = va_arg(ap, char*);
    char *outStr = uid->name;
    if (length >= 2) {
        char c = *(outStr++);
        // BFE4
        while (c != 0) {
            length--;
            *(str++) = c;
            if (length < 2)
                break;
            c = *(outStr++);
        }
    }
    // C008
    *str = '\0';
    return 0;
}

s32 obj_do_type(SceSysmemUidCB *uid, SceSysmemUidCB *uidWithFunc __attribute__((unused)), int funcId __attribute__((unused)), va_list ap __attribute__((unused)))
{
    return uid->meta->uid;
}

s32 obj_do_isKindOf(SceSysmemUidCB *uid, SceSysmemUidCB *uidWithFunc __attribute__((unused)), int funcId __attribute__((unused)), va_list ap)
{
    SceUID typeId = va_arg(ap, SceUID);
    SceSysmemUidCB *typeUid = (SceSysmemUidCB*)(0x88000000 | (((u32)typeId >> 7) * 4));
    SceSysmemMemoryPartition *part = g_MemInfo.kernel;
    if ((typeId & 0x80000001) != 1 || (u32)typeUid < part->addr ||
        (u32)typeUid >= part->addr + part->size || typeUid->uid != typeId)
        return 0;
    SceSysmemUidCB *cur = uid->meta;
    while (cur != NULL && cur != typeUid) // C0A0
        cur = cur->PARENT1;
    // C0B8
    return (cur == typeUid);
}

s32 obj_do_isMemberOf(SceSysmemUidCB *uid, SceSysmemUidCB *uidWithFunc __attribute__((unused)), int funcId __attribute__((unused)), va_list ap)
{
    SceUID id = va_arg(ap, SceUID);
    SceSysmemUidCB *uid2 = (SceSysmemUidCB*)(0x88000000 | (((u32)id >> 7) << 2));
    SceSysmemMemoryPartition *part = g_MemInfo.kernel;
    if ((id & 0x80000001) != 1 || (u32)uid2 < part->addr ||
        (u32)uid2 >= part->addr + part->size || uid2->uid != id)
        return 0;
    return (uid->meta == uid2);
}

s32 obj_do_doseNotRecognize(SceSysmemUidCB *uid, SceSysmemUidCB *uidWithFunc __attribute__((unused)), int funcId __attribute__((unused)), va_list ap)
{
    Kprintf("WARNING: %s of %s not support 0x%x key\n", uid->name, uid->meta->name, va_arg(ap, s32));
    return 0x800200CE;
}

s32 sceKernelIsHold(SceSysmemUidCB *uid0, SceSysmemUidCB *uid1)
{
    s32 oldIntr = suspendIntr();
    if (sceKernelIsKindOf(uid1, g_uidTypeList.basic) != 0 &&
        sceKernelIsKindOf(uid0, g_uidTypeList.basic) != 0) {
        SceSysmemHoldHead *head2 = *UID_CB_TO_DATA(uid1, g_uidTypeList.basic, SceSysmemHoldHead *);
        SceSysmemHoldHead *head1 = *UID_CB_TO_DATA(uid0, g_uidTypeList.basic, SceSysmemHoldHead *);
        SceSysmemHoldElem *cur;
        if (head2->count2 >= head1->count1) {
            cur = head1->hold0.next;
            // C258
            // C264
            while (cur != &head1->hold0) {
                SceSysmemHoldElement *elem = (SceSysmemHoldElement*)cur;
                if (elem->uid1 == uid1) {
                    // C248 dup
                    resumeIntr(oldIntr);
                    return 1;
                }
                cur = cur->next;
            }
        } else {
            cur = head2->hold1.next;
            while (cur != &head1->hold1) {
                SceSysmemHoldElement *elem = (SceSysmemHoldElement*)((void*)cur - 8);
                if (elem->uid0 == uid0) {
                    // C248 dup
                    resumeIntr(oldIntr);
                    return 1;
                }
                cur = cur->next;
            }
        }
    }
    // C220
    resumeIntr(oldIntr);
    return 0;
}

s32 sceKernelHoldUID(SceUID id0, SceUID id1)
{
    SceSysmemHoldElement *elem = NULL;
    SceSysmemHoldHead *head0 = NULL;
    SceSysmemHoldHead *head1 = NULL;
    s32 oldIntr = suspendIntr();
    SceSysmemUidCB *uid0 = (SceSysmemUidCB*)(0x88000000 | (((u32)id0 >> 7) * 4));
    SceSysmemUidCB *uid1 = (SceSysmemUidCB*)(0x88000000 | (((u32)id1 >> 7) * 4));
    if (id1 < 1 || id0 < 1 || uid1->uid != id1 || uid0->uid != id0) {
        // C2FC
        resumeIntr(oldIntr);
        return 0x800200CB;
    }
    // C338
    if (sceKernelIsKindOf(uid1, g_uidTypeList.basic) == 0 || sceKernelIsKindOf(uid0, g_uidTypeList.basic) == 0) {
        // C36C
        resumeIntr(oldIntr);
        return 0x800200CE;
    }
    // C380
    if (sceKernelIsHold(uid0, uid1) != 0) {
        // C4DC
        resumeIntr(oldIntr);
        return 0x800200CF;
    }
    SceSysmemHoldHead **headPtr0 = UID_CB_TO_DATA(uid0, g_uidTypeList.basic, SceSysmemHoldHead *);
    SceSysmemHoldHead **headPtr1 = UID_CB_TO_DATA(uid1, g_uidTypeList.basic, SceSysmemHoldHead *);
    if (*headPtr0 == NULL) {
        // C4B4
        head0 = AllocSceUIDHoldHead();
        if (head0 == NULL)
            goto error;
        head0->hold0.prev = &head0->hold0;
        head0->hold0.next = &head0->hold0;
        head0->hold1.prev = &head0->hold1;
        head0->hold1.next = &head0->hold1;
    }
    // C3AC
    if (*headPtr1 == NULL) {
        head1 = AllocSceUIDHoldHead();
        if (head1 == NULL)
            goto error;
        head1->hold0.prev = &head1->hold0;
        head1->hold0.next = &head1->hold0;
        head1->hold1.prev = &head1->hold1;
        head1->hold1.next = &head1->hold1;
    }
    // C3DC
    elem = AllocSceUIDHoldElement();
    if (elem == NULL)
        goto error;
    elem->uid0 = uid0;
    elem->uid1 = uid1;
    if (head1 == NULL) {
        // C468
        head1 = *headPtr1;
    } else
        *headPtr1 = head1;
    // C3FC
    if (head0 == NULL)
        head0 = *headPtr0;
    else
        *headPtr0 = head0;
    // C408
    elem->hold1.prev = head1->hold1.prev;
    head1->hold1.prev = &elem->hold1;
    elem->hold0.prev = head0->hold0.prev;
    head0->hold0.prev = &elem->hold0;
    elem->hold1.next = &head1->hold1;
    elem->hold1.prev->next = &elem->hold1;
    head1->count2++;
    elem->hold0.next = &head0->hold0;
    head0->count1++;
    elem->hold0.next->prev = &elem->hold0;
    resumeIntr(oldIntr);
    return 0;

    error:
    if (head0 != NULL)
        FreeSceUIDHoldHead(head0);
    // C480
    if (head1 != 0)
        FreeSceUIDHoldHead(head1);
    // C490
    if (elem != 0)
        FreeSceUIDHoldElement(elem);
    // C4A4
    resumeIntr(oldIntr);
    return 0x80020190;
}

s32 sceKernelReleaseUID(SceUID id0, SceUID id1)
{
    s32 oldIntr = suspendIntr();
    SceSysmemUidCB *uid0 = (SceSysmemUidCB*)(0x88000000 | (((u32)id0 >> 7) * 4));
    SceSysmemUidCB *uid1 = (SceSysmemUidCB*)(0x88000000 | (((u32)id1 >> 7) * 4));
    if (id1 < 1 || id0 < 1 || uid1->uid != id1 || uid0->uid != id0) {
        // C554
        resumeIntr(oldIntr);
        return 0x800200CB;
    }
    // C584
    if (sceKernelIsKindOf(uid1, g_uidTypeList.basic) == 0 || sceKernelIsKindOf(uid0, g_uidTypeList.basic) == 0) {
        // C5B8
        resumeIntr(oldIntr);
        return 0x800200CE;
    }
    // C5CC
    SceSysmemHoldHead **headPtr1 = UID_CB_TO_DATA(uid1, g_uidTypeList.basic, SceSysmemHoldHead *);
    SceSysmemHoldHead *head1 = *headPtr1;
    SceSysmemHoldHead **headPtr0 = UID_CB_TO_DATA(uid0, g_uidTypeList.basic, SceSysmemHoldHead *);
    SceSysmemHoldHead *head0 = *headPtr0;

    SceSysmemHoldElem *cur = head1->hold1.next;
    SceSysmemHoldElement *elem = (void*)cur - 8;
    // C5F0
    while (cur != &head1->hold1) {
        if (elem->uid0 == uid0)
            break;
        cur = cur->next;
        elem = (void*)cur - 8;
    }

    if (cur == &head1->hold1) {
        // C608
        resumeIntr(oldIntr);
        return 0x800200D0;
    }
    // C61C
    elem->hold1.next->prev = elem->hold1.prev;
    elem->hold1.prev = &elem->hold1;
    elem->hold1.prev->next = elem->hold1.next;
    elem->hold1.next = &elem->hold1;
    elem->hold0.next->prev = elem->hold0.prev;
    head1->count2--;
    elem->hold0.prev->next = elem->hold0.next;
    head0->count1--;
    elem->hold0.prev = &elem->hold0;
    elem->hold0.next = &elem->hold0;
    FreeSceUIDHoldElement(elem);
    if (head1->count2 + head1->count1 == 0) {
        // C6D0
        FreeSceUIDHoldHead(head1);
        *headPtr1 = NULL;
    }
    // C698
    if (head0->count2 + head0->count1 == 0) {
        // C6C0
        FreeSceUIDHoldHead(head0);
        *headPtr0 = NULL;
    }
    // C6B0
    resumeIntr(oldIntr);
    return 0;
}

// 135D8
SceSysmemUidLookupFunc BasicFuncs[] = {
    { 0xD310D2D9, obj_do_initialize },
    { 0x87089863, obj_do_delete2 },
    { 0, NULL }
};

void InitUidBasic(SceSysmemUidCB *root, char *rootName, SceSysmemUidCB *metaRoot, char *metaRootName, SceSysmemUidCB *basic, char *basicName, SceSysmemUidCB *metaBasic, char *metaBasicName)
{
    int oldIntr = suspendIntr();
    InitUidRoot(root, rootName, metaRoot, metaRootName);
    strcpy(basicName, "Basic");
    strcpy(metaBasicName, "MetaBasic");
    _sceKernelCreateUIDtypeInherit_sub(g_uidTypeList.root, basic, metaBasic, basicName, metaBasicName, 4, BasicFuncs, 0, &g_uidTypeList.basic);
    resumeIntr(oldIntr);
}

s32 obj_do_initialize(SceSysmemUidCB *uid, SceSysmemUidCB *uidWithFunc, int funcId, va_list ap)
{
    sceKernelCallUIDObjCommonFunction(uid, uidWithFunc, funcId, ap);
    *UID_CB_TO_DATA(uid, g_uidTypeList.basic, SceSysmemHoldHead *) = NULL;
    return uid->uid;
}

