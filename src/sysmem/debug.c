int sceKernelCheckDebugHandler()
{
    void *ptr = MpidToCB(3);
    return _CheckDebugHandler(*(int*)(ptr + 4), *(int*)(ptr + 8));
}

