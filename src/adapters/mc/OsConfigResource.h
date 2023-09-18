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
**    ClassKey
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
    /*KEY*/ MI_ConstStringField ClassKey;
    MI_ConstStringField ComponentName;
    MI_ConstStringField ReportedObjectName;
    MI_ConstStringField ReportedObjectValue;
    MI_ConstStringField DesiredObjectName;
    MI_ConstStringField DesiredObjectValue;
    MI_ConstUint32Field ReportedMpiResult;
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
    _Out_ OsConfigResource* self,
    _In_ MI_Context* context)
{
    return MI_Context_ConstructInstance(context, &OsConfigResource_rtti,
        (MI_Instance*)&self->__instance);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Clone(
    _In_ const OsConfigResource* self,
    _Outptr_ OsConfigResource** newInstance)
{
    return MI_Instance_Clone(
        &self->__instance, (MI_Instance**)newInstance);
}

MI_INLINE MI_Boolean MI_CALL OsConfigResource_IsA(
    _In_ const MI_Instance* self)
{
    MI_Boolean res = MI_FALSE;
    return MI_Instance_IsA(self, &OsConfigResource_rtti, &res) == MI_RESULT_OK && res;
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Destruct(_Inout_ OsConfigResource* self)
{
    return MI_Instance_Destruct(&self->__instance);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Delete(_Inout_ OsConfigResource* self)
{
    return MI_Instance_Delete(&self->__instance);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Post(
    _In_ const OsConfigResource* self,
    _In_ MI_Context* context)
{
    return MI_Context_PostInstance(context, &self->__instance);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Set_ResourceId(
    _Inout_ OsConfigResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        0,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetPtr_ResourceId(
    _Inout_ OsConfigResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        0,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Clear_ResourceId(
    _Inout_ OsConfigResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        0);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Set_SourceInfo(
    _Inout_ OsConfigResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        1,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetPtr_SourceInfo(
    _Inout_ OsConfigResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        1,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Clear_SourceInfo(
    _Inout_ OsConfigResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        1);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Set_DependsOn(
    _Inout_ OsConfigResource* self,
    _In_reads_opt_(size) const MI_Char** data,
    _In_ MI_Uint32 size)
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
    _Inout_ OsConfigResource* self,
    _In_reads_opt_(size) const MI_Char** data,
    _In_ MI_Uint32 size)
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
    _Inout_ OsConfigResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        2);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Set_ModuleName(
    _Inout_ OsConfigResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        3,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetPtr_ModuleName(
    _Inout_ OsConfigResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        3,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Clear_ModuleName(
    _Inout_ OsConfigResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        3);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Set_ModuleVersion(
    _Inout_ OsConfigResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        4,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetPtr_ModuleVersion(
    _Inout_ OsConfigResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        4,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Clear_ModuleVersion(
    _Inout_ OsConfigResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        4);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Set_ConfigurationName(
    _Inout_ OsConfigResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        5,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetPtr_ConfigurationName(
    _Inout_ OsConfigResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        5,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Clear_ConfigurationName(
    _Inout_ OsConfigResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        5);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Set_PsDscRunAsCredential(
    _Inout_ OsConfigResource* self,
    _In_ const MSFT_Credential* x)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        6,
        (MI_Value*)&x,
        MI_INSTANCE,
        0);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetPtr_PsDscRunAsCredential(
    _Inout_ OsConfigResource* self,
    _In_ const MSFT_Credential* x)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        6,
        (MI_Value*)&x,
        MI_INSTANCE,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Clear_PsDscRunAsCredential(
    _Inout_ OsConfigResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        6);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Set_ClassKey(
    _Inout_ OsConfigResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        7,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetPtr_ClassKey(
    _Inout_ OsConfigResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        7,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Clear_ClassKey(
    _Inout_ OsConfigResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        7);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Set_ComponentName(
    _Inout_ OsConfigResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        8,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetPtr_ComponentName(
    _Inout_ OsConfigResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        8,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Clear_ComponentName(
    _Inout_ OsConfigResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        8);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Set_ReportedObjectName(
    _Inout_ OsConfigResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        9,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetPtr_ReportedObjectName(
    _Inout_ OsConfigResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        9,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Clear_ReportedObjectName(
    _Inout_ OsConfigResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        9);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Set_ReportedObjectValue(
    _Inout_ OsConfigResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        10,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetPtr_ReportedObjectValue(
    _Inout_ OsConfigResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        10,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Clear_ReportedObjectValue(
    _Inout_ OsConfigResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        10);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Set_DesiredObjectName(
    _Inout_ OsConfigResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        11,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetPtr_DesiredObjectName(
    _Inout_ OsConfigResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        11,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Clear_DesiredObjectName(
    _Inout_ OsConfigResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        11);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Set_DesiredObjectValue(
    _Inout_ OsConfigResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        12,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetPtr_DesiredObjectValue(
    _Inout_ OsConfigResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        12,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Clear_DesiredObjectValue(
    _Inout_ OsConfigResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        12);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Set_ReportedMpiResult(
    _Inout_ OsConfigResource* self,
    _In_ MI_Uint32 x)
{
    ((MI_Uint32Field*)&self->ReportedMpiResult)->value = x;
    ((MI_Uint32Field*)&self->ReportedMpiResult)->exists = 1;
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OsConfigResource_Clear_ReportedMpiResult(
    _Inout_ OsConfigResource* self)
{
    memset((void*)&self->ReportedMpiResult, 0, sizeof(self->ReportedMpiResult));
    return MI_RESULT_OK;
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
    _Out_ OsConfigResource_GetTargetResource* self,
    _In_ MI_Context* context)
{
    return MI_Context_ConstructParameters(context, &OsConfigResource_GetTargetResource_rtti,
        (MI_Instance*)&self->__instance);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_GetTargetResource_Clone(
    _In_ const OsConfigResource_GetTargetResource* self,
    _Outptr_ OsConfigResource_GetTargetResource** newInstance)
{
    return MI_Instance_Clone(
        &self->__instance, (MI_Instance**)newInstance);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_GetTargetResource_Destruct(
    _Inout_ OsConfigResource_GetTargetResource* self)
{
    return MI_Instance_Destruct(&self->__instance);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_GetTargetResource_Delete(
    _Inout_ OsConfigResource_GetTargetResource* self)
{
    return MI_Instance_Delete(&self->__instance);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_GetTargetResource_Post(
    _In_ const OsConfigResource_GetTargetResource* self,
    _In_ MI_Context* context)
{
    return MI_Context_PostInstance(context, &self->__instance);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_GetTargetResource_Set_MIReturn(
    _Inout_ OsConfigResource_GetTargetResource* self,
    _In_ MI_Uint32 x)
{
    ((MI_Uint32Field*)&self->MIReturn)->value = x;
    ((MI_Uint32Field*)&self->MIReturn)->exists = 1;
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OsConfigResource_GetTargetResource_Clear_MIReturn(
    _Inout_ OsConfigResource_GetTargetResource* self)
{
    memset((void*)&self->MIReturn, 0, sizeof(self->MIReturn));
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OsConfigResource_GetTargetResource_Set_InputResource(
    _Inout_ OsConfigResource_GetTargetResource* self,
    _In_ const OsConfigResource* x)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        1,
        (MI_Value*)&x,
        MI_INSTANCE,
        0);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_GetTargetResource_SetPtr_InputResource(
    _Inout_ OsConfigResource_GetTargetResource* self,
    _In_ const OsConfigResource* x)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        1,
        (MI_Value*)&x,
        MI_INSTANCE,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_GetTargetResource_Clear_InputResource(
    _Inout_ OsConfigResource_GetTargetResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        1);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_GetTargetResource_Set_Flags(
    _Inout_ OsConfigResource_GetTargetResource* self,
    _In_ MI_Uint32 x)
{
    ((MI_Uint32Field*)&self->Flags)->value = x;
    ((MI_Uint32Field*)&self->Flags)->exists = 1;
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OsConfigResource_GetTargetResource_Clear_Flags(
    _Inout_ OsConfigResource_GetTargetResource* self)
{
    memset((void*)&self->Flags, 0, sizeof(self->Flags));
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OsConfigResource_GetTargetResource_Set_OutputResource(
    _Inout_ OsConfigResource_GetTargetResource* self,
    _In_ const OsConfigResource* x)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        3,
        (MI_Value*)&x,
        MI_INSTANCE,
        0);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_GetTargetResource_SetPtr_OutputResource(
    _Inout_ OsConfigResource_GetTargetResource* self,
    _In_ const OsConfigResource* x)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        3,
        (MI_Value*)&x,
        MI_INSTANCE,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_GetTargetResource_Clear_OutputResource(
    _Inout_ OsConfigResource_GetTargetResource* self)
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
    _Out_ OsConfigResource_TestTargetResource* self,
    _In_ MI_Context* context)
{
    return MI_Context_ConstructParameters(context, &OsConfigResource_TestTargetResource_rtti,
        (MI_Instance*)&self->__instance);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_TestTargetResource_Clone(
    _In_ const OsConfigResource_TestTargetResource* self,
    _Outptr_ OsConfigResource_TestTargetResource** newInstance)
{
    return MI_Instance_Clone(
        &self->__instance, (MI_Instance**)newInstance);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_TestTargetResource_Destruct(
    _Inout_ OsConfigResource_TestTargetResource* self)
{
    return MI_Instance_Destruct(&self->__instance);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_TestTargetResource_Delete(
    _Inout_ OsConfigResource_TestTargetResource* self)
{
    return MI_Instance_Delete(&self->__instance);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_TestTargetResource_Post(
    _In_ const OsConfigResource_TestTargetResource* self,
    _In_ MI_Context* context)
{
    return MI_Context_PostInstance(context, &self->__instance);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_TestTargetResource_Set_MIReturn(
    _Inout_ OsConfigResource_TestTargetResource* self,
    _In_ MI_Uint32 x)
{
    ((MI_Uint32Field*)&self->MIReturn)->value = x;
    ((MI_Uint32Field*)&self->MIReturn)->exists = 1;
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OsConfigResource_TestTargetResource_Clear_MIReturn(
    _Inout_ OsConfigResource_TestTargetResource* self)
{
    memset((void*)&self->MIReturn, 0, sizeof(self->MIReturn));
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OsConfigResource_TestTargetResource_Set_InputResource(
    _Inout_ OsConfigResource_TestTargetResource* self,
    _In_ const OsConfigResource* x)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        1,
        (MI_Value*)&x,
        MI_INSTANCE,
        0);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_TestTargetResource_SetPtr_InputResource(
    _Inout_ OsConfigResource_TestTargetResource* self,
    _In_ const OsConfigResource* x)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        1,
        (MI_Value*)&x,
        MI_INSTANCE,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_TestTargetResource_Clear_InputResource(
    _Inout_ OsConfigResource_TestTargetResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        1);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_TestTargetResource_Set_Flags(
    _Inout_ OsConfigResource_TestTargetResource* self,
    _In_ MI_Uint32 x)
{
    ((MI_Uint32Field*)&self->Flags)->value = x;
    ((MI_Uint32Field*)&self->Flags)->exists = 1;
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OsConfigResource_TestTargetResource_Clear_Flags(
    _Inout_ OsConfigResource_TestTargetResource* self)
{
    memset((void*)&self->Flags, 0, sizeof(self->Flags));
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OsConfigResource_TestTargetResource_Set_Result(
    _Inout_ OsConfigResource_TestTargetResource* self,
    _In_ MI_Boolean x)
{
    ((MI_BooleanField*)&self->Result)->value = x;
    ((MI_BooleanField*)&self->Result)->exists = 1;
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OsConfigResource_TestTargetResource_Clear_Result(
    _Inout_ OsConfigResource_TestTargetResource* self)
{
    memset((void*)&self->Result, 0, sizeof(self->Result));
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OsConfigResource_TestTargetResource_Set_ProviderContext(
    _Inout_ OsConfigResource_TestTargetResource* self,
    _In_ MI_Uint64 x)
{
    ((MI_Uint64Field*)&self->ProviderContext)->value = x;
    ((MI_Uint64Field*)&self->ProviderContext)->exists = 1;
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OsConfigResource_TestTargetResource_Clear_ProviderContext(
    _Inout_ OsConfigResource_TestTargetResource* self)
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
    _Out_ OsConfigResource_SetTargetResource* self,
    _In_ MI_Context* context)
{
    return MI_Context_ConstructParameters(context, &OsConfigResource_SetTargetResource_rtti,
        (MI_Instance*)&self->__instance);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetTargetResource_Clone(
    _In_ const OsConfigResource_SetTargetResource* self,
    _Outptr_ OsConfigResource_SetTargetResource** newInstance)
{
    return MI_Instance_Clone(
        &self->__instance, (MI_Instance**)newInstance);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetTargetResource_Destruct(
    _Inout_ OsConfigResource_SetTargetResource* self)
{
    return MI_Instance_Destruct(&self->__instance);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetTargetResource_Delete(
    _Inout_ OsConfigResource_SetTargetResource* self)
{
    return MI_Instance_Delete(&self->__instance);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetTargetResource_Post(
    _In_ const OsConfigResource_SetTargetResource* self,
    _In_ MI_Context* context)
{
    return MI_Context_PostInstance(context, &self->__instance);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetTargetResource_Set_MIReturn(
    _Inout_ OsConfigResource_SetTargetResource* self,
    _In_ MI_Uint32 x)
{
    ((MI_Uint32Field*)&self->MIReturn)->value = x;
    ((MI_Uint32Field*)&self->MIReturn)->exists = 1;
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetTargetResource_Clear_MIReturn(
    _Inout_ OsConfigResource_SetTargetResource* self)
{
    memset((void*)&self->MIReturn, 0, sizeof(self->MIReturn));
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetTargetResource_Set_InputResource(
    _Inout_ OsConfigResource_SetTargetResource* self,
    _In_ const OsConfigResource* x)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        1,
        (MI_Value*)&x,
        MI_INSTANCE,
        0);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetTargetResource_SetPtr_InputResource(
    _Inout_ OsConfigResource_SetTargetResource* self,
    _In_ const OsConfigResource* x)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        1,
        (MI_Value*)&x,
        MI_INSTANCE,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetTargetResource_Clear_InputResource(
    _Inout_ OsConfigResource_SetTargetResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        1);
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetTargetResource_Set_ProviderContext(
    _Inout_ OsConfigResource_SetTargetResource* self,
    _In_ MI_Uint64 x)
{
    ((MI_Uint64Field*)&self->ProviderContext)->value = x;
    ((MI_Uint64Field*)&self->ProviderContext)->exists = 1;
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetTargetResource_Clear_ProviderContext(
    _Inout_ OsConfigResource_SetTargetResource* self)
{
    memset((void*)&self->ProviderContext, 0, sizeof(self->ProviderContext));
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetTargetResource_Set_Flags(
    _Inout_ OsConfigResource_SetTargetResource* self,
    _In_ MI_Uint32 x)
{
    ((MI_Uint32Field*)&self->Flags)->value = x;
    ((MI_Uint32Field*)&self->Flags)->exists = 1;
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OsConfigResource_SetTargetResource_Clear_Flags(
    _Inout_ OsConfigResource_SetTargetResource* self)
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
    _Outptr_result_maybenull_ OsConfigResource_Self** self,
    _In_opt_ MI_Module_Self* selfModule,
    _In_ MI_Context* context);

MI_EXTERN_C void MI_CALL OsConfigResource_Unload(
    _In_opt_ OsConfigResource_Self* self,
    _In_ MI_Context* context);

MI_EXTERN_C void MI_CALL OsConfigResource_EnumerateInstances(
    _In_opt_ OsConfigResource_Self* self,
    _In_ MI_Context* context,
    _In_opt_z_ const MI_Char* nameSpace,
    _In_opt_z_ const MI_Char* className,
    _In_opt_ const MI_PropertySet* propertySet,
    _In_ MI_Boolean keysOnly,
    _In_opt_ const MI_Filter* filter);

MI_EXTERN_C void MI_CALL OsConfigResource_GetInstance(
    _In_opt_ OsConfigResource_Self* self,
    _In_ MI_Context* context,
    _In_opt_z_ const MI_Char* nameSpace,
    _In_opt_z_ const MI_Char* className,
    _In_ const OsConfigResource* instanceName,
    _In_opt_ const MI_PropertySet* propertySet);

MI_EXTERN_C void MI_CALL OsConfigResource_CreateInstance(
    _In_opt_ OsConfigResource_Self* self,
    _In_ MI_Context* context,
    _In_opt_z_ const MI_Char* nameSpace,
    _In_opt_z_ const MI_Char* className,
    _In_ const OsConfigResource* newInstance);

MI_EXTERN_C void MI_CALL OsConfigResource_ModifyInstance(
    _In_opt_ OsConfigResource_Self* self,
    _In_ MI_Context* context,
    _In_opt_z_ const MI_Char* nameSpace,
    _In_opt_z_ const MI_Char* className,
    _In_ const OsConfigResource* modifiedInstance,
    _In_opt_ const MI_PropertySet* propertySet);

MI_EXTERN_C void MI_CALL OsConfigResource_DeleteInstance(
    _In_opt_ OsConfigResource_Self* self,
    _In_ MI_Context* context,
    _In_opt_z_ const MI_Char* nameSpace,
    _In_opt_z_ const MI_Char* className,
    _In_ const OsConfigResource* instanceName);

MI_EXTERN_C void MI_CALL OsConfigResource_Invoke_GetTargetResource(
    _In_opt_ OsConfigResource_Self* self,
    _In_ MI_Context* context,
    _In_opt_z_ const MI_Char* nameSpace,
    _In_opt_z_ const MI_Char* className,
    _In_opt_z_ const MI_Char* methodName,
    _In_ const OsConfigResource* instanceName,
    _In_opt_ const OsConfigResource_GetTargetResource* in);

MI_EXTERN_C void MI_CALL OsConfigResource_Invoke_TestTargetResource(
    _In_opt_ OsConfigResource_Self* self,
    _In_ MI_Context* context,
    _In_opt_z_ const MI_Char* nameSpace,
    _In_opt_z_ const MI_Char* className,
    _In_opt_z_ const MI_Char* methodName,
    _In_ const OsConfigResource* instanceName,
    _In_opt_ const OsConfigResource_TestTargetResource* in);

MI_EXTERN_C void MI_CALL OsConfigResource_Invoke_SetTargetResource(
    _In_opt_ OsConfigResource_Self* self,
    _In_ MI_Context* context,
    _In_opt_z_ const MI_Char* nameSpace,
    _In_opt_z_ const MI_Char* className,
    _In_opt_z_ const MI_Char* methodName,
    _In_ const OsConfigResource* instanceName,
    _In_opt_ const OsConfigResource_SetTargetResource* in);


#endif /* _OsConfigResource_h */
