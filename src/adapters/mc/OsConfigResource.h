// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Common.h"

/* @migen@ */
/*
**==============================================================================
**
** WARNING: THIS FILE WAS AUTOMATICALLY GENERATED. PLEASE DO NOT EDIT.
**
**==============================================================================
*/
#ifndef _OsConfigResource_h
#define _OsConfigResource_h

/*
**==============================================================================
**
** OsConfigResource [OsConfigResource]
**
** Keys:
**    RuleId
**
**==============================================================================
*/

typedef struct _OsConfigResource /* extends OMI_BaseResource */
{
    MI_Instance __instance;
    /* OMI_BaseResource properties */
    MI_ConstStringField ResourceId;
    MI_ConstStringField SourceInfo;
    MI_ConstStringAField DependsOn;
    MI_ConstStringField ModuleName;
    MI_ConstStringField ModuleVersion;
    MI_ConstStringField ConfigurationName;
    MSFT_Credential_ConstRef PsDscRunAsCredential;
    /* OsConfigResource properties */
    /*KEY*/ MI_ConstStringField RuleId;
    MI_ConstStringField PayloadKey;
    MI_ConstStringField ComponentName;
    MI_ConstStringField InitObjectName;
    MI_ConstStringField ProcedureObjectName;
    MI_ConstStringField ProcedureObjectValue;
    MI_ConstStringField ReportedObjectName;
    MI_ConstStringField ReportedObjectValue;
    MI_ConstStringField ExpectedObjectValue;
    MI_ConstStringField DesiredObjectName;
    MI_ConstStringField DesiredObjectValue;
    MI_ConstUint32Field ReportedMpiResult;
    ReasonClass_ConstArrayRef Reasons;
}
OsConfigResource;

typedef struct _OsConfigResource_Ref
{
    OsConfigResource* value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
OsConfigResource_Ref;

typedef struct _OsConfigResource_ConstRef
{
    MI_CONST OsConfigResource* value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
OsConfigResource_ConstRef;

typedef struct _OsConfigResource_Array
{
    struct _OsConfigResource** data;
    MI_Uint32 size;
}
OsConfigResource_Array;

typedef struct _OsConfigResource_ConstArray
{
    struct _OsConfigResource MI_CONST* MI_CONST* data;
    MI_Uint32 size;
}
OsConfigResource_ConstArray;

typedef struct _OsConfigResource_ArrayRef
{
    OsConfigResource_Array value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
OsConfigResource_ArrayRef;

typedef struct _OsConfigResource_ConstArrayRef
{
    OsConfigResource_ConstArray value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
OsConfigResource_ConstArrayRef;

MI_EXTERN_C MI_CONST MI_ClassDecl OsConfigResource_rtti;

MI_INLINE MI_Result MI_CALL OsConfigResource_Construct(
    OsConfigResource* self,
    MI_Context* context)
{
    return MI_Context_ConstructInstance(context, &OsConfigResource_rtti,
        (MI_Instance*)&self->__instance);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Clone(
    const OsConfigResource* self,
    OsConfigResource** newInstance)
{
    return MI_Instance_Clone(
        &self->__instance, (MI_Instance**)newInstance);
}

MI_INLINE MI_Boolean MI_CALL OsConfigResource_IsA(
    const MI_Instance* self)
{
    MI_Boolean res = MI_FALSE;
    return MI_Instance_IsA(self, &OsConfigResource_rtti, &res) == MI_RESULT_OK && res;
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Destruct(OsConfigResource* self)
{
    return MI_Instance_Destruct(&self->__instance);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Delete(OsConfigResource* self)
{
    return MI_Instance_Delete(&self->__instance);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Post(
    const OsConfigResource* self,
    MI_Context* context)
{
    return MI_Context_PostInstance(context, &self->__instance);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Set_ResourceId(
    OsConfigResource* self,
    const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        0,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetPtr_ResourceId(
    OsConfigResource* self,
    const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        0,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Clear_ResourceId(
    OsConfigResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        0);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Set_SourceInfo(
    OsConfigResource* self,
    const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        1,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetPtr_SourceInfo(
    OsConfigResource* self,
    const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        1,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Clear_SourceInfo(
    OsConfigResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        1);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Set_DependsOn(
    OsConfigResource* self,
    const MI_Char** data,
    MI_Uint32 size)
{
    MI_Array arr;
    arr.data = (void*)data;
    arr.size = size;
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        2,
        (MI_Value*)&arr,
        MI_STRINGA,
        0);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetPtr_DependsOn(
    OsConfigResource* self,
    const MI_Char** data,
    MI_Uint32 size)
{
    MI_Array arr;
    arr.data = (void*)data;
    arr.size = size;
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        2,
        (MI_Value*)&arr,
        MI_STRINGA,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Clear_DependsOn(
    OsConfigResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        2);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Set_ModuleName(
    OsConfigResource* self,
    const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        3,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetPtr_ModuleName(
    OsConfigResource* self,
    const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        3,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Clear_ModuleName(
    OsConfigResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        3);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Set_ModuleVersion(
    OsConfigResource* self,
    const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        4,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetPtr_ModuleVersion(
    OsConfigResource* self,
    const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        4,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Clear_ModuleVersion(
    OsConfigResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        4);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Set_ConfigurationName(
    OsConfigResource* self,
    const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        5,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetPtr_ConfigurationName(
    OsConfigResource* self,
    const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        5,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Clear_ConfigurationName(
    OsConfigResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        5);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Set_PsDscRunAsCredential(
    OsConfigResource* self,
    const MSFT_Credential* x)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        6,
        (MI_Value*)&x,
        MI_INSTANCE,
        0);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetPtr_PsDscRunAsCredential(
    OsConfigResource* self,
    const MSFT_Credential* x)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        6,
        (MI_Value*)&x,
        MI_INSTANCE,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Clear_PsDscRunAsCredential(
    OsConfigResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        6);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Set_RuleId(
    OsConfigResource* self,
    const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        7,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetPtr_RuleId(
    OsConfigResource* self,
    const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        7,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Clear_RuleId(
    OsConfigResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        7);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Set_PayloadKey(
    OsConfigResource* self,
    const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        8,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetPtr_PayloadKey(
    OsConfigResource* self,
    const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        8,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Clear_PayloadKey(
    OsConfigResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        8);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Set_ComponentName(
    OsConfigResource* self,
    const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        9,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetPtr_ComponentName(
    OsConfigResource* self,
    const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        9,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Clear_ComponentName(
    OsConfigResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        9);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Set_InitObjectName(
    OsConfigResource* self,
    const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        10,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetPtr_InitObjectName(
    OsConfigResource* self,
    const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        10,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Clear_InitObjectName(
    OsConfigResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        10);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Set_ProcedureObjectName(
    OsConfigResource* self,
    const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        11,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetPtr_ProcedureObjectName(
    OsConfigResource* self,
    const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        11,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Clear_ProcedureObjectName(
    OsConfigResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        11);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Set_ProcedureObjectValue(
    OsConfigResource* self,
    const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        12,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetPtr_ProcedureObjectValue(
    OsConfigResource* self,
    const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        12,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Clear_ProcedureObjectValue(
    OsConfigResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        12);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Set_ReportedObjectName(
    OsConfigResource* self,
    const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        13,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetPtr_ReportedObjectName(
    OsConfigResource* self,
    const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        13,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Clear_ReportedObjectName(
    OsConfigResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        13);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Set_ReportedObjectValue(
    OsConfigResource* self,
    const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        14,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetPtr_ReportedObjectValue(
    OsConfigResource* self,
    const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        14,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Clear_ReportedObjectValue(
    OsConfigResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        14);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Set_ExpectedObjectValue(
    OsConfigResource* self,
    const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        15,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetPtr_ExpectedObjectValue(
    OsConfigResource* self,
    const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        15,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Clear_ExpectedObjectValue(
    OsConfigResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        15);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Set_DesiredObjectName(
    OsConfigResource* self,
    const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        16,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetPtr_DesiredObjectName(
    OsConfigResource* self,
    const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        16,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Clear_DesiredObjectName(
    OsConfigResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        16);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Set_DesiredObjectValue(
    OsConfigResource* self,
    const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        17,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetPtr_DesiredObjectValue(
    OsConfigResource* self,
    const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        17,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Clear_DesiredObjectValue(
    OsConfigResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        17);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Set_ReportedMpiResult(
    OsConfigResource* self,
    MI_Uint32 x)
{
    ((MI_Uint32Field*)&self->ReportedMpiResult)->value = x;
    ((MI_Uint32Field*)&self->ReportedMpiResult)->exists = 1;
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Clear_ReportedMpiResult(
    OsConfigResource* self)
{
    memset((void*)&self->ReportedMpiResult, 0, sizeof(self->ReportedMpiResult));
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Set_Reasons(
    OsConfigResource* self,
    const ReasonClass * const * data,
    MI_Uint32 size)
{
    MI_Array arr;
    arr.data = (void*)data;
    arr.size = size;
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        19,
        (MI_Value*)&arr,
        MI_INSTANCEA,
        0);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetPtr_Reasons(
    OsConfigResource* self,
    const ReasonClass * const * data,
    MI_Uint32 size)
{
    MI_Array arr;
    arr.data = (void*)data;
    arr.size = size;
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        19,
        (MI_Value*)&arr,
        MI_INSTANCEA,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Clear_Reasons(
    OsConfigResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        19);
}

/*
**==============================================================================
**
** OsConfigResource.GetTargetResource()
**
**==============================================================================
*/

typedef struct _OsConfigResource_GetTargetResource
{
    MI_Instance __instance;
    /*OUT*/ MI_ConstUint32Field MIReturn;
    /*IN*/ OsConfigResource_ConstRef InputResource;
    /*IN*/ MI_ConstUint32Field Flags;
    /*OUT*/ OsConfigResource_ConstRef OutputResource;
}
OsConfigResource_GetTargetResource;

MI_EXTERN_C MI_CONST MI_MethodDecl OsConfigResource_GetTargetResource_rtti;

MI_INLINE MI_Result MI_CALL OsConfigResource_GetTargetResource_Construct(
    OsConfigResource_GetTargetResource* self,
    MI_Context* context)
{
    return MI_Context_ConstructParameters(context, &OsConfigResource_GetTargetResource_rtti,
        (MI_Instance*)&self->__instance);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_GetTargetResource_Clone(
    const OsConfigResource_GetTargetResource* self,
    OsConfigResource_GetTargetResource** newInstance)
{
    return MI_Instance_Clone(
        &self->__instance, (MI_Instance**)newInstance);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_GetTargetResource_Destruct(
    OsConfigResource_GetTargetResource* self)
{
    return MI_Instance_Destruct(&self->__instance);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_GetTargetResource_Delete(
    OsConfigResource_GetTargetResource* self)
{
    return MI_Instance_Delete(&self->__instance);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_GetTargetResource_Post(
    const OsConfigResource_GetTargetResource* self,
    MI_Context* context)
{
    return MI_Context_PostInstance(context, &self->__instance);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_GetTargetResource_Set_MIReturn(
    OsConfigResource_GetTargetResource* self,
    MI_Uint32 x)
{
    ((MI_Uint32Field*)&self->MIReturn)->value = x;
    ((MI_Uint32Field*)&self->MIReturn)->exists = 1;
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OsConfigResource_GetTargetResource_Clear_MIReturn(
    OsConfigResource_GetTargetResource* self)
{
    memset((void*)&self->MIReturn, 0, sizeof(self->MIReturn));
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OsConfigResource_GetTargetResource_Set_InputResource(
    OsConfigResource_GetTargetResource* self,
    const OsConfigResource* x)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        1,
        (MI_Value*)&x,
        MI_INSTANCE,
        0);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_GetTargetResource_SetPtr_InputResource(
    OsConfigResource_GetTargetResource* self,
    const OsConfigResource* x)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        1,
        (MI_Value*)&x,
        MI_INSTANCE,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_GetTargetResource_Clear_InputResource(
    OsConfigResource_GetTargetResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        1);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_GetTargetResource_Set_Flags(
    OsConfigResource_GetTargetResource* self,
    MI_Uint32 x)
{
    ((MI_Uint32Field*)&self->Flags)->value = x;
    ((MI_Uint32Field*)&self->Flags)->exists = 1;
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OsConfigResource_GetTargetResource_Clear_Flags(
    OsConfigResource_GetTargetResource* self)
{
    memset((void*)&self->Flags, 0, sizeof(self->Flags));
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OsConfigResource_GetTargetResource_Set_OutputResource(
    OsConfigResource_GetTargetResource* self,
    const OsConfigResource* x)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        3,
        (MI_Value*)&x,
        MI_INSTANCE,
        0);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_GetTargetResource_SetPtr_OutputResource(
    OsConfigResource_GetTargetResource* self,
    const OsConfigResource* x)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        3,
        (MI_Value*)&x,
        MI_INSTANCE,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_GetTargetResource_Clear_OutputResource(
    OsConfigResource_GetTargetResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        3);
}

/*
**==============================================================================
**
** OsConfigResource.TestTargetResource()
**
**==============================================================================
*/

typedef struct _OsConfigResource_TestTargetResource
{
    MI_Instance __instance;
    /*OUT*/ MI_ConstUint32Field MIReturn;
    /*IN*/ OsConfigResource_ConstRef InputResource;
    /*IN*/ MI_ConstUint32Field Flags;
    /*OUT*/ MI_ConstBooleanField Result;
    /*OUT*/ MI_ConstUint64Field ProviderContext;
}
OsConfigResource_TestTargetResource;

MI_EXTERN_C MI_CONST MI_MethodDecl OsConfigResource_TestTargetResource_rtti;

MI_INLINE MI_Result MI_CALL OsConfigResource_TestTargetResource_Construct(
    OsConfigResource_TestTargetResource* self,
    MI_Context* context)
{
    return MI_Context_ConstructParameters(context, &OsConfigResource_TestTargetResource_rtti,
        (MI_Instance*)&self->__instance);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_TestTargetResource_Clone(
    const OsConfigResource_TestTargetResource* self,
    OsConfigResource_TestTargetResource** newInstance)
{
    return MI_Instance_Clone(
        &self->__instance, (MI_Instance**)newInstance);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_TestTargetResource_Destruct(
    OsConfigResource_TestTargetResource* self)
{
    return MI_Instance_Destruct(&self->__instance);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_TestTargetResource_Delete(
    OsConfigResource_TestTargetResource* self)
{
    return MI_Instance_Delete(&self->__instance);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_TestTargetResource_Post(
    const OsConfigResource_TestTargetResource* self,
    MI_Context* context)
{
    return MI_Context_PostInstance(context, &self->__instance);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_TestTargetResource_Set_MIReturn(
    OsConfigResource_TestTargetResource* self,
    MI_Uint32 x)
{
    ((MI_Uint32Field*)&self->MIReturn)->value = x;
    ((MI_Uint32Field*)&self->MIReturn)->exists = 1;
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OsConfigResource_TestTargetResource_Clear_MIReturn(
    OsConfigResource_TestTargetResource* self)
{
    memset((void*)&self->MIReturn, 0, sizeof(self->MIReturn));
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OsConfigResource_TestTargetResource_Set_InputResource(
    OsConfigResource_TestTargetResource* self,
    const OsConfigResource* x)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        1,
        (MI_Value*)&x,
        MI_INSTANCE,
        0);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_TestTargetResource_SetPtr_InputResource(
    OsConfigResource_TestTargetResource* self,
    const OsConfigResource* x)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        1,
        (MI_Value*)&x,
        MI_INSTANCE,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_TestTargetResource_Clear_InputResource(
    OsConfigResource_TestTargetResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        1);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_TestTargetResource_Set_Flags(
    OsConfigResource_TestTargetResource* self,
    MI_Uint32 x)
{
    ((MI_Uint32Field*)&self->Flags)->value = x;
    ((MI_Uint32Field*)&self->Flags)->exists = 1;
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OsConfigResource_TestTargetResource_Clear_Flags(
    OsConfigResource_TestTargetResource* self)
{
    memset((void*)&self->Flags, 0, sizeof(self->Flags));
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OsConfigResource_TestTargetResource_Set_Result(
    OsConfigResource_TestTargetResource* self,
    MI_Boolean x)
{
    ((MI_BooleanField*)&self->Result)->value = x;
    ((MI_BooleanField*)&self->Result)->exists = 1;
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OsConfigResource_TestTargetResource_Clear_Result(
    OsConfigResource_TestTargetResource* self)
{
    memset((void*)&self->Result, 0, sizeof(self->Result));
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OsConfigResource_TestTargetResource_Set_ProviderContext(
    OsConfigResource_TestTargetResource* self,
    MI_Uint64 x)
{
    ((MI_Uint64Field*)&self->ProviderContext)->value = x;
    ((MI_Uint64Field*)&self->ProviderContext)->exists = 1;
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OsConfigResource_TestTargetResource_Clear_ProviderContext(
    OsConfigResource_TestTargetResource* self)
{
    memset((void*)&self->ProviderContext, 0, sizeof(self->ProviderContext));
    return MI_RESULT_OK;
}

/*
**==============================================================================
**
** OsConfigResource.SetTargetResource()
**
**==============================================================================
*/

typedef struct _OsConfigResource_SetTargetResource
{
    MI_Instance __instance;
    /*OUT*/ MI_ConstUint32Field MIReturn;
    /*IN*/ OsConfigResource_ConstRef InputResource;
    /*IN*/ MI_ConstUint64Field ProviderContext;
    /*IN*/ MI_ConstUint32Field Flags;
}
OsConfigResource_SetTargetResource;

MI_EXTERN_C MI_CONST MI_MethodDecl OsConfigResource_SetTargetResource_rtti;

MI_INLINE MI_Result MI_CALL OsConfigResource_SetTargetResource_Construct(
    OsConfigResource_SetTargetResource* self,
    MI_Context* context)
{
    return MI_Context_ConstructParameters(context, &OsConfigResource_SetTargetResource_rtti,
        (MI_Instance*)&self->__instance);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetTargetResource_Clone(
    const OsConfigResource_SetTargetResource* self,
    OsConfigResource_SetTargetResource** newInstance)
{
    return MI_Instance_Clone(
        &self->__instance, (MI_Instance**)newInstance);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetTargetResource_Destruct(
    OsConfigResource_SetTargetResource* self)
{
    return MI_Instance_Destruct(&self->__instance);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetTargetResource_Delete(
    OsConfigResource_SetTargetResource* self)
{
    return MI_Instance_Delete(&self->__instance);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetTargetResource_Post(
    const OsConfigResource_SetTargetResource* self,
    MI_Context* context)
{
    return MI_Context_PostInstance(context, &self->__instance);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetTargetResource_Set_MIReturn(
    OsConfigResource_SetTargetResource* self,
    MI_Uint32 x)
{
    ((MI_Uint32Field*)&self->MIReturn)->value = x;
    ((MI_Uint32Field*)&self->MIReturn)->exists = 1;
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetTargetResource_Clear_MIReturn(
    OsConfigResource_SetTargetResource* self)
{
    memset((void*)&self->MIReturn, 0, sizeof(self->MIReturn));
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetTargetResource_Set_InputResource(
    OsConfigResource_SetTargetResource* self,
    const OsConfigResource* x)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        1,
        (MI_Value*)&x,
        MI_INSTANCE,
        0);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetTargetResource_SetPtr_InputResource(
    OsConfigResource_SetTargetResource* self,
    const OsConfigResource* x)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        1,
        (MI_Value*)&x,
        MI_INSTANCE,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetTargetResource_Clear_InputResource(
    OsConfigResource_SetTargetResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        1);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetTargetResource_Set_ProviderContext(
    OsConfigResource_SetTargetResource* self,
    MI_Uint64 x)
{
    ((MI_Uint64Field*)&self->ProviderContext)->value = x;
    ((MI_Uint64Field*)&self->ProviderContext)->exists = 1;
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetTargetResource_Clear_ProviderContext(
    OsConfigResource_SetTargetResource* self)
{
    memset((void*)&self->ProviderContext, 0, sizeof(self->ProviderContext));
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetTargetResource_Set_Flags(
    OsConfigResource_SetTargetResource* self,
    MI_Uint32 x)
{
    ((MI_Uint32Field*)&self->Flags)->value = x;
    ((MI_Uint32Field*)&self->Flags)->exists = 1;
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetTargetResource_Clear_Flags(
    OsConfigResource_SetTargetResource* self)
{
    memset((void*)&self->Flags, 0, sizeof(self->Flags));
    return MI_RESULT_OK;
}

/*
**==============================================================================
**
** OsConfigResource provider function prototypes
**
**==============================================================================
*/

/* The developer may optionally define this structure */
typedef struct _OsConfigResource_Self OsConfigResource_Self;

MI_EXTERN_C void MI_CALL OsConfigResource_Load(
    OsConfigResource_Self** self,
    MI_Module_Self* selfModule,
    MI_Context* context);

MI_EXTERN_C void MI_CALL OsConfigResource_Unload(
    OsConfigResource_Self* self,
    MI_Context* context);

MI_EXTERN_C void MI_CALL OsConfigResource_EnumerateInstances(
    OsConfigResource_Self* self,
    MI_Context* context,
    const MI_Char* nameSpace,
    const MI_Char* className,
    const MI_PropertySet* propertySet,
    MI_Boolean keysOnly,
    const MI_Filter* filter);

MI_EXTERN_C void MI_CALL OsConfigResource_GetInstance(
    OsConfigResource_Self* self,
    MI_Context* context,
    const MI_Char* nameSpace,
    const MI_Char* className,
    const OsConfigResource* instanceName,
    const MI_PropertySet* propertySet);

MI_EXTERN_C void MI_CALL OsConfigResource_CreateInstance(
    OsConfigResource_Self* self,
    MI_Context* context,
    const MI_Char* nameSpace,
    const MI_Char* className,
    const OsConfigResource* newInstance);

MI_EXTERN_C void MI_CALL OsConfigResource_ModifyInstance(
    OsConfigResource_Self* self,
    MI_Context* context,
    const MI_Char* nameSpace,
    const MI_Char* className,
    const OsConfigResource* modifiedInstance,
    const MI_PropertySet* propertySet);

MI_EXTERN_C void MI_CALL OsConfigResource_DeleteInstance(
    OsConfigResource_Self* self,
    MI_Context* context,
    const MI_Char* nameSpace,
    const MI_Char* className,
    const OsConfigResource* instanceName);

MI_EXTERN_C void MI_CALL OsConfigResource_Invoke_GetTargetResource(
    OsConfigResource_Self* self,
    MI_Context* context,
    const MI_Char* nameSpace,
    const MI_Char* className,
    const MI_Char* methodName,
    const OsConfigResource* instanceName,
    const OsConfigResource_GetTargetResource* in);

MI_EXTERN_C void MI_CALL OsConfigResource_Invoke_TestTargetResource(
    OsConfigResource_Self* self,
    MI_Context* context,
    const MI_Char* nameSpace,
    const MI_Char* className,
    const MI_Char* methodName,
    const OsConfigResource* instanceName,
    const OsConfigResource_TestTargetResource* in);

MI_EXTERN_C void MI_CALL OsConfigResource_Invoke_SetTargetResource(
    OsConfigResource_Self* self,
    MI_Context* context,
    const MI_Char* nameSpace,
    const MI_Char* className,
    const MI_Char* methodName,
    const OsConfigResource* instanceName,
    const OsConfigResource_SetTargetResource* in);

#endif /* _OsConfigResource_h */
