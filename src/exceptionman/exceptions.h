#ifndef EXCEPTIONMAN_H
#define EXCEPTIONMAN_H

typedef struct SceExceptionHandler
{
    struct SceExceptionHandler *next;
    void *cb;
} SceExceptionHandler;

void build_exectbl(void);
SceExceptionHandler *newExcepCB(void);
void FreeExcepCB(SceExceptionHandler *ex);
void Allocexceppool(void);

#endif

