# Copyright (C) 2011 The uOFW team
# See the file COPYING for copying permission.

        .set noreorder

#include "pspstub.s"

        STUB_START "sceSuspendForKernel",0x00010011,0x00020005
        STUB_FUNC  0x91A77137,sceKernelRegisterSuspendHandler
        STUB_FUNC  0xB43D1A8C,sceKernelRegisterResumeHandler
        STUB_END

        STUB_START "LoadCoreForKernel",0x00010011,0x00010005
        STUB_FUNC  0x18CFDAA0,sceKernelGetModuleGPByAddressForKernel
        STUB_END

