// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Common.h"

MI_EXTERN_C MI_SchemaDecl schemaDecl;

void MI_CALL Load(MI_Module_Self** self, struct _MI_Context* context)
{
    OsConfigLogInfo(GetLog(), "[OsConfigResource] MI module load (PID: %d)", getpid());

    *self = NULL;
    MI_Context_PostResult(context, MI_RESULT_OK);
}

void MI_CALL Unload(MI_Module_Self* self, struct _MI_Context* context)
{
    OsConfigLogInfo(GetLog(), "[OsConfigResource] MI module unload (PID: %d)", getpid());

    MI_UNREFERENCED_PARAMETER(self);
    MI_UNREFERENCED_PARAMETER(context);

    MI_Context_PostResult(context, MI_RESULT_OK);
}

MI_EXTERN_C MI_EXPORT MI_Module* MI_MAIN_CALL MI_Main(MI_Server* server)
{
    OsConfigLogInfo(GetLog(), "[OsConfigResource] MI module main (PID: %d)", getpid());

    static MI_Module module;
    MI_EXTERN_C MI_Server* __mi_server;

    __mi_server = server;

    module.flags |= MI_MODULE_FLAG_DESCRIPTIONS;
    module.flags |= MI_MODULE_FLAG_VALUES;
    module.flags |= MI_MODULE_FLAG_BOOLEANS;
    module.flags |= MI_MODULE_FLAG_LOCALIZED;
    module.charSize = sizeof(MI_Char);
    module.version = MI_VERSION;
    module.generatorVersion = MI_MAKE_VERSION(1, 0, 0);
    module.schemaDecl = &schemaDecl;
    module.Load = Load;
    module.Unload = Unload;

    return &module;
}
