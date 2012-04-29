typedef struct SceSysEventHandler
{
    int size;
    char* name;
    int type_mask;
    int (*handler)(int ev_id, char* ev_name, void* param, int* result);
    int gp;
    int busy;
    struct SceSysEventHandler *next;
    int reserved[9];
} SceSysEventHandler;

int sceKernelUnregisterSysEventHandler(SceSysEventHandler *handler);
int sceKernelSysEventDispatch(int ev_type_mask, int ev_id, char* ev_name, void* param, int* result, int break_nonzero, SceSysEventHandler **break_handler);
int sceKernelSysEventInit(void);
int sceKernelIsRegisterSysEventHandler(SceSysEventHandler* handler);
int sceKernelRegisterSysEventHandler(SceSysEventHandler* handler);
SceSysEventHandler *sceKernelReferSysEventHandler(void);

int sceKernelRegisterResumeHandler(int reg, int (*handler)(int unk, void *param), void *param);
int sceKernelRegisterSuspendHandler(int reg, int (*handler)(int unk, void *param), void *param);

