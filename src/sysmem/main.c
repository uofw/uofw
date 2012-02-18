// 14548
void *g_deci2p;

int sceKernelDeci2pRegisterOperations(void *op)
{
    g_deci2p = op;
    return 0;
}

void *sceKernelDeci2pReferOperations(void)
{
    return g_deci2p;
}

