int KDebugForKernel_61B6A1CE(void);

typedef struct
{
    int size;
    int opt1;
    int opt2;
    char *typeName; /* vsh, umdemu.. */
    int opt4;
    int opt5;
    int opt6;
    int opt7;
    int loadPspbtcnf; // Always set to 0x10000
    int opt9;
    int opt10;
    int opt11;
} RebootArgs2;

typedef struct
{
    int opt0;
    int opt1;
    char *fileName;
    RebootArgs2 *args2;
    int opt4;
    void *opt5;
    int opt6;
    int opt7;
} RebootArgs;

int func_282C(RebootArgs2 *arg);
int LoadExecForKernel_0DBC783B(int arg0, int arg1);
int LoadExecForKernel_3D805DE6(int arg0, int arg1);
int LoadExecForKernel_4A9446E7(int arg0, int arg1);
int LoadExecForKernel_5AA1A6D2(RebootArgs2 *opt);
int LoadExecForKernel_6C00E642(int arg0, int arg1);
int LoadExecForKernel_7A206082(int arg0, int arg1);
int LoadExecForKernel_7C0ADE1F();
int LoadExecForKernel_8CE2AB36();
int LoadExecForKernel_8EF38192(int arg0, int arg1);
int LoadExecForKernel_9D39758A();
int LoadExecForKernel_10E94E41(int arg0, int arg1);
int LoadExecForKernel_45C6125B(int arg0, int arg1);
int LoadExecForKernel_59A2F67F(int arg0, int arg1);
int LoadExecForKernel_106ABDB8(int arg0, int arg1, int arg2);
int LoadExecForKernel_179D905A(int arg0, int arg1);
int LoadExecForKernel_778E333F(int arg0, int arg1);
int LoadExecForKernel_818E14A4(int arg0, int arg1, int arg2);
int LoadExecForKernel_905FDDB6(int arg0, int arg1, int arg2);
int LoadExecForKernel_2752CD13(int arg0, int arg1);
int LoadExecForKernel_7286CF0B(int arg0, int arg1);
int LoadExecForKernel_9212E475(int arg0, int arg1);
int LoadExecForKernel_9828D1D9(int arg0, int arg1);
int LoadExecForKernel_54303E86(RebootArgs2 *opt, int arg1);
int LoadExecForKernel_78912B54(int arg0, int arg1);
int LoadExecForKernel_BAEB4B89(int arg0, int arg1);
int LoadExecForKernel_C42F65FA(int arg0, int arg1);
int LoadExecForKernel_CA86DDD9(int arg0, int arg1);
int LoadExecForKernel_CEFE1100(int arg0, int arg1);
int LoadExecForKernel_D35D6403(int arg0, int arg1);
int LoadExecForKernel_DDED4433(int arg0, int arg1);
int LoadExecForKernel_DEA6A7FC(int arg0, int arg1);
int LoadExecForUser_4AC57943(int arg);
int LoadExecForKernel_E35220AC(int arg0, int arg1, int arg2);
SceUID LoadExecForKernel_EF9C9627();
int LoadExecForKernel_FCD765C9(RebootArgs2 *arg);
int LoadExecForKernel_FE8E1A30(int arg0, int arg1, int arg2);
int LoadExecForUser_2AC9954B();
int LoadExecForKernel_E9B45481(int arg);
int LoadExecForUser_8ADA38D3(char *fileName, int arg1);
int LoadExecForUser_362A956B();
int LoadExecForUser_05572A5F();
int LoadExecForUser_BD2F1094(char *arg0, int arg1);
int module_bootstart();
void decodeKL4E(char *dst, int size, char *src, int arg3);
void sub_0BBC(int *arg0);
void sub_2A4C(RebootArgs *opt);
void sub_09D8(int *struct1, int *struct2);
int runExec(RebootArgs *args); // 20E4
int sub_21C8(char *name, int devcmd, int iocmd);
int sub_22F0(RebootArgs2 *opt);
void sub_32E4();
int sub_236C(int arg0, int arg1, int arg2, int arg3);
int runExecFromThread(int unk, RebootArgs *opt); // 284C
void sub_298C(int *dst, int arg1, int *src);
int sub_2568(int arg0, int arg1, int arg2, int arg3, int arg4);
int sub_2698(int arg0, RebootArgs2 *opt);
int LoadExecForUser_D1FB50DC(int arg);
