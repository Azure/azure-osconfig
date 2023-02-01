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
#ifndef _LinuxOsConfigResource_h
#define _LinuxOsConfigResource_h

/*
**==============================================================================
**
** LinuxOsConfigResource [LinuxOsConfigResource]
**
** Keys:
**    LinuxOsConfigClassKey
**
**==============================================================================
*/

typedef struct _LinuxOsConfigResource /* extends OMI_BaseResource */
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
    /* LinuxOsConfigResource properties */
    /*KEY*/ MI_ConstStringField LinuxOsConfigClassKey;
    MI_ConstStringField ComponentName;
    MI_ConstStringField ReportedObjectName;
    MI_ConstStringField ReportedObjectValue;
    MI_ConstStringField DesiredObjectName;
    MI_ConstStringField DesiredObjectValue;
    MI_ConstUint32Field ReportedMpiResult;
}
LinuxOsConfigResource;

typedef struct _LinuxOsConfigResource_Ref
{
    LinuxOsConfigResource* value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
LinuxOsConfigResource_Ref;

typedef struct _LinuxOsConfigResource_ConstRef
{
    MI_CONST LinuxOsConfigResource* value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
LinuxOsConfigResource_ConstRef;

typedef struct _LinuxOsConfigResource_Array
{
    struct _LinuxOsConfigResource** data;
    MI_Uint32 size;
}
LinuxOsConfigResource_Array;

typedef struct _LinuxOsConfigResource_ConstArray
{
    struct _LinuxOsConfigResource MI_CONST* MI_CONST* data;
    MI_Uint32 size;
}
LinuxOsConfigResource_ConstArray;

typedef struct _LinuxOsConfigResource_ArrayRef
{
    LinuxOsConfigResource_Array value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
LinuxOsConfigResource_ArrayRef;

typedef struct _LinuxOsConfigResource_ConstArrayRef
{
    LinuxOsConfigResource_ConstArray value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
LinuxOsConfigResource_ConstArrayRef;

MI_EXTERN_C MI_CONST MI_ClassDecl LinuxOsConfigResource_rtti;

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_Construct(
    _Out_ LinuxOsConfigResource* self,
    _In_ MI_Context* context)
{
    return MI_Context_ConstructInstance(context, &LinuxOsConfigResource_rtti,
        (MI_Instance*)&self->__instance);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_Clone(
    _In_ const LinuxOsConfigResource* self,
    _Outptr_ LinuxOsConfigResource** newInstance)
{
    return MI_Instance_Clone(
        &self->__instance, (MI_Instance**)newInstance);
}

MI_INLINE MI_Boolean MI_CALL LinuxOsConfigResource_IsA(
    _In_ const MI_Instance* self)
{
    MI_Boolean res = MI_FALSE;
    return MI_Instance_IsA(self, &LinuxOsConfigResource_rtti, &res) == MI_RESULT_OK && res;
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_Destruct(_Inout_ LinuxOsConfigResource* self)
{
    return MI_Instance_Destruct(&self->__instance);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_Delete(_Inout_ LinuxOsConfigResource* self)
{
    return MI_Instance_Delete(&self->__instance);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_Post(
    _In_ const LinuxOsConfigResource* self,
    _In_ MI_Context* context)
{
    return MI_Context_PostInstance(context, &self->__instance);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_Set_ResourceId(
    _Inout_ LinuxOsConfigResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        0,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_SetPtr_ResourceId(
    _Inout_ LinuxOsConfigResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        0,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_Clear_ResourceId(
    _Inout_ LinuxOsConfigResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        0);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_Set_SourceInfo(
    _Inout_ LinuxOsConfigResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        1,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_SetPtr_SourceInfo(
    _Inout_ LinuxOsConfigResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        1,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_Clear_SourceInfo(
    _Inout_ LinuxOsConfigResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        1);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_Set_DependsOn(
    _Inout_ LinuxOsConfigResource* self,
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

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_SetPtr_DependsOn(
    _Inout_ LinuxOsConfigResource* self,
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

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_Clear_DependsOn(
    _Inout_ LinuxOsConfigResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        2);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_Set_ModuleName(
    _Inout_ LinuxOsConfigResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        3,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_SetPtr_ModuleName(
    _Inout_ LinuxOsConfigResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        3,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_Clear_ModuleName(
    _Inout_ LinuxOsConfigResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        3);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_Set_ModuleVersion(
    _Inout_ LinuxOsConfigResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        4,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_SetPtr_ModuleVersion(
    _Inout_ LinuxOsConfigResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        4,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_Clear_ModuleVersion(
    _Inout_ LinuxOsConfigResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        4);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_Set_ConfigurationName(
    _Inout_ LinuxOsConfigResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        5,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_SetPtr_ConfigurationName(
    _Inout_ LinuxOsConfigResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        5,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_Clear_ConfigurationName(
    _Inout_ LinuxOsConfigResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        5);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_Set_PsDscRunAsCredential(
    _Inout_ LinuxOsConfigResource* self,
    _In_ const MSFT_Credential* x)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        6,
        (MI_Value*)&x,
        MI_INSTANCE,
        0);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_SetPtr_PsDscRunAsCredential(
    _Inout_ LinuxOsConfigResource* self,
    _In_ const MSFT_Credential* x)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        6,
        (MI_Value*)&x,
        MI_INSTANCE,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_Clear_PsDscRunAsCredential(
    _Inout_ LinuxOsConfigResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        6);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_Set_LinuxOsConfigClassKey(
    _Inout_ LinuxOsConfigResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        7,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_SetPtr_LinuxOsConfigClassKey(
    _Inout_ LinuxOsConfigResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        7,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_Clear_LinuxOsConfigClassKey(
    _Inout_ LinuxOsConfigResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        7);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_Set_ComponentName(
    _Inout_ LinuxOsConfigResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        8,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_SetPtr_ComponentName(
    _Inout_ LinuxOsConfigResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        8,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_Clear_ComponentName(
    _Inout_ LinuxOsConfigResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        8);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_Set_ReportedObjectName(
    _Inout_ LinuxOsConfigResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        9,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_SetPtr_ReportedObjectName(
    _Inout_ LinuxOsConfigResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        9,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_Clear_ReportedObjectName(
    _Inout_ LinuxOsConfigResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        9);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_Set_ReportedObjectValue(
    _Inout_ LinuxOsConfigResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        10,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_SetPtr_ReportedObjectValue(
    _Inout_ LinuxOsConfigResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        10,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_Clear_ReportedObjectValue(
    _Inout_ LinuxOsConfigResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        10);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_Set_DesiredObjectName(
    _Inout_ LinuxOsConfigResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        11,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_SetPtr_DesiredObjectName(
    _Inout_ LinuxOsConfigResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        11,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_Clear_DesiredObjectName(
    _Inout_ LinuxOsConfigResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        11);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_Set_DesiredObjectValue(
    _Inout_ LinuxOsConfigResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        12,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_SetPtr_DesiredObjectValue(
    _Inout_ LinuxOsConfigResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        12,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_Clear_DesiredObjectValue(
    _Inout_ LinuxOsConfigResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        12);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_Set_ReportedMpiResult(
    _Inout_ LinuxOsConfigResource* self,
    _In_ MI_Uint32 x)
{
    ((MI_Uint32Field*)&self->ReportedMpiResult)->value = x;
    ((MI_Uint32Field*)&self->ReportedMpiResult)->exists = 1;
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_Clear_ReportedMpiResult(
    _Inout_ LinuxOsConfigResource* self)
{
    memset((void*)&self->ReportedMpiResult, 0, sizeof(self->ReportedMpiResult));
    return MI_RESULT_OK;
}

/*
**==============================================================================
**
** LinuxOsConfigResource.GetTargetResource()
**
**==============================================================================
*/

typedef struct _LinuxOsConfigResource_GetTargetResource
{
    MI_Instance __instance;
    /*OUT*/ MI_ConstUint32Field MIReturn;
    /*IN*/ LinuxOsConfigResource_ConstRef InputResource;
    /*IN*/ MI_ConstUint32Field Flags;
    /*OUT*/ LinuxOsConfigResource_ConstRef OutputResource;
}
LinuxOsConfigResource_GetTargetResource;

MI_EXTERN_C MI_CONST MI_MethodDecl LinuxOsConfigResource_GetTargetResource_rtti;

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_GetTargetResource_Construct(
    _Out_ LinuxOsConfigResource_GetTargetResource* self,
    _In_ MI_Context* context)
{
    return MI_Context_ConstructParameters(context, &LinuxOsConfigResource_GetTargetResource_rtti,
        (MI_Instance*)&self->__instance);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_GetTargetResource_Clone(
    _In_ const LinuxOsConfigResource_GetTargetResource* self,
    _Outptr_ LinuxOsConfigResource_GetTargetResource** newInstance)
{
    return MI_Instance_Clone(
        &self->__instance, (MI_Instance**)newInstance);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_GetTargetResource_Destruct(
    _Inout_ LinuxOsConfigResource_GetTargetResource* self)
{
    return MI_Instance_Destruct(&self->__instance);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_GetTargetResource_Delete(
    _Inout_ LinuxOsConfigResource_GetTargetResource* self)
{
    return MI_Instance_Delete(&self->__instance);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_GetTargetResource_Post(
    _In_ const LinuxOsConfigResource_GetTargetResource* self,
    _In_ MI_Context* context)
{
    return MI_Context_PostInstance(context, &self->__instance);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_GetTargetResource_Set_MIReturn(
    _Inout_ LinuxOsConfigResource_GetTargetResource* self,
    _In_ MI_Uint32 x)
{
    ((MI_Uint32Field*)&self->MIReturn)->value = x;
    ((MI_Uint32Field*)&self->MIReturn)->exists = 1;
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_GetTargetResource_Clear_MIReturn(
    _Inout_ LinuxOsConfigResource_GetTargetResource* self)
{
    memset((void*)&self->MIReturn, 0, sizeof(self->MIReturn));
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_GetTargetResource_Set_InputResource(
    _Inout_ LinuxOsConfigResource_GetTargetResource* self,
    _In_ const LinuxOsConfigResource* x)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        1,
        (MI_Value*)&x,
        MI_INSTANCE,
        0);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_GetTargetResource_SetPtr_InputResource(
    _Inout_ LinuxOsConfigResource_GetTargetResource* self,
    _In_ const LinuxOsConfigResource* x)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        1,
        (MI_Value*)&x,
        MI_INSTANCE,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_GetTargetResource_Clear_InputResource(
    _Inout_ LinuxOsConfigResource_GetTargetResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        1);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_GetTargetResource_Set_Flags(
    _Inout_ LinuxOsConfigResource_GetTargetResource* self,
    _In_ MI_Uint32 x)
{
    ((MI_Uint32Field*)&self->Flags)->value = x;
    ((MI_Uint32Field*)&self->Flags)->exists = 1;
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_GetTargetResource_Clear_Flags(
    _Inout_ LinuxOsConfigResource_GetTargetResource* self)
{
    memset((void*)&self->Flags, 0, sizeof(self->Flags));
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_GetTargetResource_Set_OutputResource(
    _Inout_ LinuxOsConfigResource_GetTargetResource* self,
    _In_ const LinuxOsConfigResource* x)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        3,
        (MI_Value*)&x,
        MI_INSTANCE,
        0);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_GetTargetResource_SetPtr_OutputResource(
    _Inout_ LinuxOsConfigResource_GetTargetResource* self,
    _In_ const LinuxOsConfigResource* x)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        3,
        (MI_Value*)&x,
        MI_INSTANCE,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_GetTargetResource_Clear_OutputResource(
    _Inout_ LinuxOsConfigResource_GetTargetResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        3);
}

/*
**==============================================================================
**
** LinuxOsConfigResource.TestTargetResource()
**
**==============================================================================
*/

typedef struct _LinuxOsConfigResource_TestTargetResource
{
    MI_Instance __instance;
    /*OUT*/ MI_ConstUint32Field MIReturn;
    /*IN*/ LinuxOsConfigResource_ConstRef InputResource;
    /*IN*/ MI_ConstUint32Field Flags;
    /*OUT*/ MI_ConstBooleanField Result;
    /*OUT*/ MI_ConstUint64Field ProviderContext;
}
LinuxOsConfigResource_TestTargetResource;

MI_EXTERN_C MI_CONST MI_MethodDecl LinuxOsConfigResource_TestTargetResource_rtti;

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_TestTargetResource_Construct(
    _Out_ LinuxOsConfigResource_TestTargetResource* self,
    _In_ MI_Context* context)
{
    return MI_Context_ConstructParameters(context, &LinuxOsConfigResource_TestTargetResource_rtti,
        (MI_Instance*)&self->__instance);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_TestTargetResource_Clone(
    _In_ const LinuxOsConfigResource_TestTargetResource* self,
    _Outptr_ LinuxOsConfigResource_TestTargetResource** newInstance)
{
    return MI_Instance_Clone(
        &self->__instance, (MI_Instance**)newInstance);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_TestTargetResource_Destruct(
    _Inout_ LinuxOsConfigResource_TestTargetResource* self)
{
    return MI_Instance_Destruct(&self->__instance);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_TestTargetResource_Delete(
    _Inout_ LinuxOsConfigResource_TestTargetResource* self)
{
    return MI_Instance_Delete(&self->__instance);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_TestTargetResource_Post(
    _In_ const LinuxOsConfigResource_TestTargetResource* self,
    _In_ MI_Context* context)
{
    return MI_Context_PostInstance(context, &self->__instance);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_TestTargetResource_Set_MIReturn(
    _Inout_ LinuxOsConfigResource_TestTargetResource* self,
    _In_ MI_Uint32 x)
{
    ((MI_Uint32Field*)&self->MIReturn)->value = x;
    ((MI_Uint32Field*)&self->MIReturn)->exists = 1;
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_TestTargetResource_Clear_MIReturn(
    _Inout_ LinuxOsConfigResource_TestTargetResource* self)
{
    memset((void*)&self->MIReturn, 0, sizeof(self->MIReturn));
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_TestTargetResource_Set_InputResource(
    _Inout_ LinuxOsConfigResource_TestTargetResource* self,
    _In_ const LinuxOsConfigResource* x)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        1,
        (MI_Value*)&x,
        MI_INSTANCE,
        0);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_TestTargetResource_SetPtr_InputResource(
    _Inout_ LinuxOsConfigResource_TestTargetResource* self,
    _In_ const LinuxOsConfigResource* x)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        1,
        (MI_Value*)&x,
        MI_INSTANCE,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_TestTargetResource_Clear_InputResource(
    _Inout_ LinuxOsConfigResource_TestTargetResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        1);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_TestTargetResource_Set_Flags(
    _Inout_ LinuxOsConfigResource_TestTargetResource* self,
    _In_ MI_Uint32 x)
{
    ((MI_Uint32Field*)&self->Flags)->value = x;
    ((MI_Uint32Field*)&self->Flags)->exists = 1;
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_TestTargetResource_Clear_Flags(
    _Inout_ LinuxOsConfigResource_TestTargetResource* self)
{
    memset((void*)&self->Flags, 0, sizeof(self->Flags));
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_TestTargetResource_Set_Result(
    _Inout_ LinuxOsConfigResource_TestTargetResource* self,
    _In_ MI_Boolean x)
{
    ((MI_BooleanField*)&self->Result)->value = x;
    ((MI_BooleanField*)&self->Result)->exists = 1;
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_TestTargetResource_Clear_Result(
    _Inout_ LinuxOsConfigResource_TestTargetResource* self)
{
    memset((void*)&self->Result, 0, sizeof(self->Result));
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_TestTargetResource_Set_ProviderContext(
    _Inout_ LinuxOsConfigResource_TestTargetResource* self,
    _In_ MI_Uint64 x)
{
    ((MI_Uint64Field*)&self->ProviderContext)->value = x;
    ((MI_Uint64Field*)&self->ProviderContext)->exists = 1;
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_TestTargetResource_Clear_ProviderContext(
    _Inout_ LinuxOsConfigResource_TestTargetResource* self)
{
    memset((void*)&self->ProviderContext, 0, sizeof(self->ProviderContext));
    return MI_RESULT_OK;
}

/*
**==============================================================================
**
** LinuxOsConfigResource.SetTargetResource()
**
**==============================================================================
*/

typedef struct _LinuxOsConfigResource_SetTargetResource
{
    MI_Instance __instance;
    /*OUT*/ MI_ConstUint32Field MIReturn;
    /*IN*/ LinuxOsConfigResource_ConstRef InputResource;
    /*IN*/ MI_ConstUint64Field ProviderContext;
    /*IN*/ MI_ConstUint32Field Flags;
}
LinuxOsConfigResource_SetTargetResource;

MI_EXTERN_C MI_CONST MI_MethodDecl LinuxOsConfigResource_SetTargetResource_rtti;

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_SetTargetResource_Construct(
    _Out_ LinuxOsConfigResource_SetTargetResource* self,
    _In_ MI_Context* context)
{
    return MI_Context_ConstructParameters(context, &LinuxOsConfigResource_SetTargetResource_rtti,
        (MI_Instance*)&self->__instance);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_SetTargetResource_Clone(
    _In_ const LinuxOsConfigResource_SetTargetResource* self,
    _Outptr_ LinuxOsConfigResource_SetTargetResource** newInstance)
{
    return MI_Instance_Clone(
        &self->__instance, (MI_Instance**)newInstance);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_SetTargetResource_Destruct(
    _Inout_ LinuxOsConfigResource_SetTargetResource* self)
{
    return MI_Instance_Destruct(&self->__instance);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_SetTargetResource_Delete(
    _Inout_ LinuxOsConfigResource_SetTargetResource* self)
{
    return MI_Instance_Delete(&self->__instance);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_SetTargetResource_Post(
    _In_ const LinuxOsConfigResource_SetTargetResource* self,
    _In_ MI_Context* context)
{
    return MI_Context_PostInstance(context, &self->__instance);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_SetTargetResource_Set_MIReturn(
    _Inout_ LinuxOsConfigResource_SetTargetResource* self,
    _In_ MI_Uint32 x)
{
    ((MI_Uint32Field*)&self->MIReturn)->value = x;
    ((MI_Uint32Field*)&self->MIReturn)->exists = 1;
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_SetTargetResource_Clear_MIReturn(
    _Inout_ LinuxOsConfigResource_SetTargetResource* self)
{
    memset((void*)&self->MIReturn, 0, sizeof(self->MIReturn));
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_SetTargetResource_Set_InputResource(
    _Inout_ LinuxOsConfigResource_SetTargetResource* self,
    _In_ const LinuxOsConfigResource* x)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        1,
        (MI_Value*)&x,
        MI_INSTANCE,
        0);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_SetTargetResource_SetPtr_InputResource(
    _Inout_ LinuxOsConfigResource_SetTargetResource* self,
    _In_ const LinuxOsConfigResource* x)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        1,
        (MI_Value*)&x,
        MI_INSTANCE,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_SetTargetResource_Clear_InputResource(
    _Inout_ LinuxOsConfigResource_SetTargetResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        1);
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_SetTargetResource_Set_ProviderContext(
    _Inout_ LinuxOsConfigResource_SetTargetResource* self,
    _In_ MI_Uint64 x)
{
    ((MI_Uint64Field*)&self->ProviderContext)->value = x;
    ((MI_Uint64Field*)&self->ProviderContext)->exists = 1;
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_SetTargetResource_Clear_ProviderContext(
    _Inout_ LinuxOsConfigResource_SetTargetResource* self)
{
    memset((void*)&self->ProviderContext, 0, sizeof(self->ProviderContext));
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_SetTargetResource_Set_Flags(
    _Inout_ LinuxOsConfigResource_SetTargetResource* self,
    _In_ MI_Uint32 x)
{
    ((MI_Uint32Field*)&self->Flags)->value = x;
    ((MI_Uint32Field*)&self->Flags)->exists = 1;
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL LinuxOsConfigResource_SetTargetResource_Clear_Flags(
    _Inout_ LinuxOsConfigResource_SetTargetResource* self)
{
    memset((void*)&self->Flags, 0, sizeof(self->Flags));
    return MI_RESULT_OK;
}

/*
**==============================================================================
**
** LinuxOsConfigResource provider function prototypes
**
**==============================================================================
*/

/* The developer may optionally define this structure */
typedef struct _LinuxOsConfigResource_Self LinuxOsConfigResource_Self;

MI_EXTERN_C void MI_CALL LinuxOsConfigResource_Load(
    _Outptr_result_maybenull_ LinuxOsConfigResource_Self** self,
    _In_opt_ MI_Module_Self* selfModule,
    _In_ MI_Context* context);

MI_EXTERN_C void MI_CALL LinuxOsConfigResource_Unload(
    _In_opt_ LinuxOsConfigResource_Self* self,
    _In_ MI_Context* context);

MI_EXTERN_C void MI_CALL LinuxOsConfigResource_EnumerateInstances(
    _In_opt_ LinuxOsConfigResource_Self* self,
    _In_ MI_Context* context,
    _In_opt_z_ const MI_Char* nameSpace,
    _In_opt_z_ const MI_Char* className,
    _In_opt_ const MI_PropertySet* propertySet,
    _In_ MI_Boolean keysOnly,
    _In_opt_ const MI_Filter* filter);

MI_EXTERN_C void MI_CALL LinuxOsConfigResource_GetInstance(
    _In_opt_ LinuxOsConfigResource_Self* self,
    _In_ MI_Context* context,
    _In_opt_z_ const MI_Char* nameSpace,
    _In_opt_z_ const MI_Char* className,
    _In_ const LinuxOsConfigResource* instanceName,
    _In_opt_ const MI_PropertySet* propertySet);

MI_EXTERN_C void MI_CALL LinuxOsConfigResource_CreateInstance(
    _In_opt_ LinuxOsConfigResource_Self* self,
    _In_ MI_Context* context,
    _In_opt_z_ const MI_Char* nameSpace,
    _In_opt_z_ const MI_Char* className,
    _In_ const LinuxOsConfigResource* newInstance);

MI_EXTERN_C void MI_CALL LinuxOsConfigResource_ModifyInstance(
    _In_opt_ LinuxOsConfigResource_Self* self,
    _In_ MI_Context* context,
    _In_opt_z_ const MI_Char* nameSpace,
    _In_opt_z_ const MI_Char* className,
    _In_ const LinuxOsConfigResource* modifiedInstance,
    _In_opt_ const MI_PropertySet* propertySet);

MI_EXTERN_C void MI_CALL LinuxOsConfigResource_DeleteInstance(
    _In_opt_ LinuxOsConfigResource_Self* self,
    _In_ MI_Context* context,
    _In_opt_z_ const MI_Char* nameSpace,
    _In_opt_z_ const MI_Char* className,
    _In_ const LinuxOsConfigResource* instanceName);

MI_EXTERN_C void MI_CALL LinuxOsConfigResource_Invoke_GetTargetResource(
    _In_opt_ LinuxOsConfigResource_Self* self,
    _In_ MI_Context* context,
    _In_opt_z_ const MI_Char* nameSpace,
    _In_opt_z_ const MI_Char* className,
    _In_opt_z_ const MI_Char* methodName,
    _In_ const LinuxOsConfigResource* instanceName,
    _In_opt_ const LinuxOsConfigResource_GetTargetResource* in);

MI_EXTERN_C void MI_CALL LinuxOsConfigResource_Invoke_TestTargetResource(
    _In_opt_ LinuxOsConfigResource_Self* self,
    _In_ MI_Context* context,
    _In_opt_z_ const MI_Char* nameSpace,
    _In_opt_z_ const MI_Char* className,
    _In_opt_z_ const MI_Char* methodName,
    _In_ const LinuxOsConfigResource* instanceName,
    _In_opt_ const LinuxOsConfigResource_TestTargetResource* in);

MI_EXTERN_C void MI_CALL LinuxOsConfigResource_Invoke_SetTargetResource(
    _In_opt_ LinuxOsConfigResource_Self* self,
    _In_ MI_Context* context,
    _In_opt_z_ const MI_Char* nameSpace,
    _In_opt_z_ const MI_Char* className,
    _In_opt_z_ const MI_Char* methodName,
    _In_ const LinuxOsConfigResource* instanceName,
    _In_opt_ const LinuxOsConfigResource_SetTargetResource* in);


#endif /* _LinuxOsConfigResource_h */
