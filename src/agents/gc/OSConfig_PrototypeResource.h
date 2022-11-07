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
#ifndef _OSConfig_PrototypeResource_h
#define _OSConfig_PrototypeResource_h

/*
**==============================================================================
**
** OSConfig_PrototypeResource [OSConfig_PrototypeResource]
**
** Keys:
**    PrototypeClassKey
**
**==============================================================================
*/

typedef struct _OSConfig_PrototypeResource /* extends OMI_BaseResource */
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
    /* OSConfig_PrototypeResource properties */
    /*KEY*/ MI_ConstStringField PrototypeClassKey;
    MI_ConstStringField Ensure;
    MI_ConstStringField ReportedString;
    MI_ConstStringField DesiredString;
    MI_ConstBooleanField ReportedBoolean;
    MI_ConstBooleanField DesiredBoolean;
    MI_ConstBooleanField ReportedInteger;
    MI_ConstUint32Field DesiredInteger;
    MI_ConstUint32Field ReportedIntegerStatus;
    MI_ConstStringField ReportedStringResult;
}
OSConfig_PrototypeResource;

typedef struct _OSConfig_PrototypeResource_Ref
{
    OSConfig_PrototypeResource* value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
OSConfig_PrototypeResource_Ref;

typedef struct _OSConfig_PrototypeResource_ConstRef
{
    MI_CONST OSConfig_PrototypeResource* value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
OSConfig_PrototypeResource_ConstRef;

typedef struct _OSConfig_PrototypeResource_Array
{
    struct _OSConfig_PrototypeResource** data;
    MI_Uint32 size;
}
OSConfig_PrototypeResource_Array;

typedef struct _OSConfig_PrototypeResource_ConstArray
{
    struct _OSConfig_PrototypeResource MI_CONST* MI_CONST* data;
    MI_Uint32 size;
}
OSConfig_PrototypeResource_ConstArray;

typedef struct _OSConfig_PrototypeResource_ArrayRef
{
    OSConfig_PrototypeResource_Array value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
OSConfig_PrototypeResource_ArrayRef;

typedef struct _OSConfig_PrototypeResource_ConstArrayRef
{
    OSConfig_PrototypeResource_ConstArray value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
OSConfig_PrototypeResource_ConstArrayRef;

MI_EXTERN_C MI_CONST MI_ClassDecl OSConfig_PrototypeResource_rtti;

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_Construct(
    _Out_ OSConfig_PrototypeResource* self,
    _In_ MI_Context* context)
{
    return MI_Context_ConstructInstance(context, &OSConfig_PrototypeResource_rtti,
        (MI_Instance*)&self->__instance);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_Clone(
    _In_ const OSConfig_PrototypeResource* self,
    _Outptr_ OSConfig_PrototypeResource** newInstance)
{
    return MI_Instance_Clone(
        &self->__instance, (MI_Instance**)newInstance);
}

MI_INLINE MI_Boolean MI_CALL OSConfig_PrototypeResource_IsA(
    _In_ const MI_Instance* self)
{
    MI_Boolean res = MI_FALSE;
    return MI_Instance_IsA(self, &OSConfig_PrototypeResource_rtti, &res) == MI_RESULT_OK && res;
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_Destruct(_Inout_ OSConfig_PrototypeResource* self)
{
    return MI_Instance_Destruct(&self->__instance);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_Delete(_Inout_ OSConfig_PrototypeResource* self)
{
    return MI_Instance_Delete(&self->__instance);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_Post(
    _In_ const OSConfig_PrototypeResource* self,
    _In_ MI_Context* context)
{
    return MI_Context_PostInstance(context, &self->__instance);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_Set_ResourceId(
    _Inout_ OSConfig_PrototypeResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        0,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_SetPtr_ResourceId(
    _Inout_ OSConfig_PrototypeResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        0,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_Clear_ResourceId(
    _Inout_ OSConfig_PrototypeResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        0);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_Set_SourceInfo(
    _Inout_ OSConfig_PrototypeResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        1,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_SetPtr_SourceInfo(
    _Inout_ OSConfig_PrototypeResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        1,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_Clear_SourceInfo(
    _Inout_ OSConfig_PrototypeResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        1);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_Set_DependsOn(
    _Inout_ OSConfig_PrototypeResource* self,
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

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_SetPtr_DependsOn(
    _Inout_ OSConfig_PrototypeResource* self,
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

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_Clear_DependsOn(
    _Inout_ OSConfig_PrototypeResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        2);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_Set_ModuleName(
    _Inout_ OSConfig_PrototypeResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        3,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_SetPtr_ModuleName(
    _Inout_ OSConfig_PrototypeResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        3,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_Clear_ModuleName(
    _Inout_ OSConfig_PrototypeResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        3);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_Set_ModuleVersion(
    _Inout_ OSConfig_PrototypeResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        4,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_SetPtr_ModuleVersion(
    _Inout_ OSConfig_PrototypeResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        4,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_Clear_ModuleVersion(
    _Inout_ OSConfig_PrototypeResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        4);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_Set_ConfigurationName(
    _Inout_ OSConfig_PrototypeResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        5,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_SetPtr_ConfigurationName(
    _Inout_ OSConfig_PrototypeResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        5,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_Clear_ConfigurationName(
    _Inout_ OSConfig_PrototypeResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        5);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_Set_PsDscRunAsCredential(
    _Inout_ OSConfig_PrototypeResource* self,
    _In_ const MSFT_Credential* x)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        6,
        (MI_Value*)&x,
        MI_INSTANCE,
        0);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_SetPtr_PsDscRunAsCredential(
    _Inout_ OSConfig_PrototypeResource* self,
    _In_ const MSFT_Credential* x)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        6,
        (MI_Value*)&x,
        MI_INSTANCE,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_Clear_PsDscRunAsCredential(
    _Inout_ OSConfig_PrototypeResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        6);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_Set_PrototypeClassKey(
    _Inout_ OSConfig_PrototypeResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        7,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_SetPtr_PrototypeClassKey(
    _Inout_ OSConfig_PrototypeResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        7,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_Clear_PrototypeClassKey(
    _Inout_ OSConfig_PrototypeResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        7);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_Set_Ensure(
    _Inout_ OSConfig_PrototypeResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        8,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_SetPtr_Ensure(
    _Inout_ OSConfig_PrototypeResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        8,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_Clear_Ensure(
    _Inout_ OSConfig_PrototypeResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        8);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_Set_ReportedString(
    _Inout_ OSConfig_PrototypeResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        9,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_SetPtr_ReportedString(
    _Inout_ OSConfig_PrototypeResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        9,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_Clear_ReportedString(
    _Inout_ OSConfig_PrototypeResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        9);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_Set_DesiredString(
    _Inout_ OSConfig_PrototypeResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        10,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_SetPtr_DesiredString(
    _Inout_ OSConfig_PrototypeResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        10,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_Clear_DesiredString(
    _Inout_ OSConfig_PrototypeResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        10);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_Set_ReportedBoolean(
    _Inout_ OSConfig_PrototypeResource* self,
    _In_ MI_Boolean x)
{
    ((MI_BooleanField*)&self->ReportedBoolean)->value = x;
    ((MI_BooleanField*)&self->ReportedBoolean)->exists = 1;
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_Clear_ReportedBoolean(
    _Inout_ OSConfig_PrototypeResource* self)
{
    memset((void*)&self->ReportedBoolean, 0, sizeof(self->ReportedBoolean));
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_Set_DesiredBoolean(
    _Inout_ OSConfig_PrototypeResource* self,
    _In_ MI_Boolean x)
{
    ((MI_BooleanField*)&self->DesiredBoolean)->value = x;
    ((MI_BooleanField*)&self->DesiredBoolean)->exists = 1;
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_Clear_DesiredBoolean(
    _Inout_ OSConfig_PrototypeResource* self)
{
    memset((void*)&self->DesiredBoolean, 0, sizeof(self->DesiredBoolean));
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_Set_ReportedInteger(
    _Inout_ OSConfig_PrototypeResource* self,
    _In_ MI_Boolean x)
{
    ((MI_BooleanField*)&self->ReportedInteger)->value = x;
    ((MI_BooleanField*)&self->ReportedInteger)->exists = 1;
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_Clear_ReportedInteger(
    _Inout_ OSConfig_PrototypeResource* self)
{
    memset((void*)&self->ReportedInteger, 0, sizeof(self->ReportedInteger));
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_Set_DesiredInteger(
    _Inout_ OSConfig_PrototypeResource* self,
    _In_ MI_Uint32 x)
{
    ((MI_Uint32Field*)&self->DesiredInteger)->value = x;
    ((MI_Uint32Field*)&self->DesiredInteger)->exists = 1;
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_Clear_DesiredInteger(
    _Inout_ OSConfig_PrototypeResource* self)
{
    memset((void*)&self->DesiredInteger, 0, sizeof(self->DesiredInteger));
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_Set_ReportedIntegerStatus(
    _Inout_ OSConfig_PrototypeResource* self,
    _In_ MI_Uint32 x)
{
    ((MI_Uint32Field*)&self->ReportedIntegerStatus)->value = x;
    ((MI_Uint32Field*)&self->ReportedIntegerStatus)->exists = 1;
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_Clear_ReportedIntegerStatus(
    _Inout_ OSConfig_PrototypeResource* self)
{
    memset((void*)&self->ReportedIntegerStatus, 0, sizeof(self->ReportedIntegerStatus));
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_Set_ReportedStringResult(
    _Inout_ OSConfig_PrototypeResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        16,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_SetPtr_ReportedStringResult(
    _Inout_ OSConfig_PrototypeResource* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        16,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_Clear_ReportedStringResult(
    _Inout_ OSConfig_PrototypeResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        16);
}

/*
**==============================================================================
**
** OSConfig_PrototypeResource.GetTargetResource()
**
**==============================================================================
*/

typedef struct _OSConfig_PrototypeResource_GetTargetResource
{
    MI_Instance __instance;
    /*OUT*/ MI_ConstUint32Field MIReturn;
    /*IN*/ OSConfig_PrototypeResource_ConstRef InputResource;
    /*IN*/ MI_ConstUint32Field Flags;
    /*OUT*/ OSConfig_PrototypeResource_ConstRef OutputResource;
}
OSConfig_PrototypeResource_GetTargetResource;

MI_EXTERN_C MI_CONST MI_MethodDecl OSConfig_PrototypeResource_GetTargetResource_rtti;

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_GetTargetResource_Construct(
    _Out_ OSConfig_PrototypeResource_GetTargetResource* self,
    _In_ MI_Context* context)
{
    return MI_Context_ConstructParameters(context, &OSConfig_PrototypeResource_GetTargetResource_rtti,
        (MI_Instance*)&self->__instance);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_GetTargetResource_Clone(
    _In_ const OSConfig_PrototypeResource_GetTargetResource* self,
    _Outptr_ OSConfig_PrototypeResource_GetTargetResource** newInstance)
{
    return MI_Instance_Clone(
        &self->__instance, (MI_Instance**)newInstance);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_GetTargetResource_Destruct(
    _Inout_ OSConfig_PrototypeResource_GetTargetResource* self)
{
    return MI_Instance_Destruct(&self->__instance);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_GetTargetResource_Delete(
    _Inout_ OSConfig_PrototypeResource_GetTargetResource* self)
{
    return MI_Instance_Delete(&self->__instance);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_GetTargetResource_Post(
    _In_ const OSConfig_PrototypeResource_GetTargetResource* self,
    _In_ MI_Context* context)
{
    return MI_Context_PostInstance(context, &self->__instance);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_GetTargetResource_Set_MIReturn(
    _Inout_ OSConfig_PrototypeResource_GetTargetResource* self,
    _In_ MI_Uint32 x)
{
    ((MI_Uint32Field*)&self->MIReturn)->value = x;
    ((MI_Uint32Field*)&self->MIReturn)->exists = 1;
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_GetTargetResource_Clear_MIReturn(
    _Inout_ OSConfig_PrototypeResource_GetTargetResource* self)
{
    memset((void*)&self->MIReturn, 0, sizeof(self->MIReturn));
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_GetTargetResource_Set_InputResource(
    _Inout_ OSConfig_PrototypeResource_GetTargetResource* self,
    _In_ const OSConfig_PrototypeResource* x)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        1,
        (MI_Value*)&x,
        MI_INSTANCE,
        0);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_GetTargetResource_SetPtr_InputResource(
    _Inout_ OSConfig_PrototypeResource_GetTargetResource* self,
    _In_ const OSConfig_PrototypeResource* x)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        1,
        (MI_Value*)&x,
        MI_INSTANCE,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_GetTargetResource_Clear_InputResource(
    _Inout_ OSConfig_PrototypeResource_GetTargetResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        1);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_GetTargetResource_Set_Flags(
    _Inout_ OSConfig_PrototypeResource_GetTargetResource* self,
    _In_ MI_Uint32 x)
{
    ((MI_Uint32Field*)&self->Flags)->value = x;
    ((MI_Uint32Field*)&self->Flags)->exists = 1;
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_GetTargetResource_Clear_Flags(
    _Inout_ OSConfig_PrototypeResource_GetTargetResource* self)
{
    memset((void*)&self->Flags, 0, sizeof(self->Flags));
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_GetTargetResource_Set_OutputResource(
    _Inout_ OSConfig_PrototypeResource_GetTargetResource* self,
    _In_ const OSConfig_PrototypeResource* x)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        3,
        (MI_Value*)&x,
        MI_INSTANCE,
        0);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_GetTargetResource_SetPtr_OutputResource(
    _Inout_ OSConfig_PrototypeResource_GetTargetResource* self,
    _In_ const OSConfig_PrototypeResource* x)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        3,
        (MI_Value*)&x,
        MI_INSTANCE,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_GetTargetResource_Clear_OutputResource(
    _Inout_ OSConfig_PrototypeResource_GetTargetResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        3);
}

/*
**==============================================================================
**
** OSConfig_PrototypeResource.TestTargetResource()
**
**==============================================================================
*/

typedef struct _OSConfig_PrototypeResource_TestTargetResource
{
    MI_Instance __instance;
    /*OUT*/ MI_ConstUint32Field MIReturn;
    /*IN*/ OSConfig_PrototypeResource_ConstRef InputResource;
    /*IN*/ MI_ConstUint32Field Flags;
    /*OUT*/ MI_ConstBooleanField Result;
    /*OUT*/ MI_ConstUint64Field ProviderContext;
}
OSConfig_PrototypeResource_TestTargetResource;

MI_EXTERN_C MI_CONST MI_MethodDecl OSConfig_PrototypeResource_TestTargetResource_rtti;

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_TestTargetResource_Construct(
    _Out_ OSConfig_PrototypeResource_TestTargetResource* self,
    _In_ MI_Context* context)
{
    return MI_Context_ConstructParameters(context, &OSConfig_PrototypeResource_TestTargetResource_rtti,
        (MI_Instance*)&self->__instance);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_TestTargetResource_Clone(
    _In_ const OSConfig_PrototypeResource_TestTargetResource* self,
    _Outptr_ OSConfig_PrototypeResource_TestTargetResource** newInstance)
{
    return MI_Instance_Clone(
        &self->__instance, (MI_Instance**)newInstance);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_TestTargetResource_Destruct(
    _Inout_ OSConfig_PrototypeResource_TestTargetResource* self)
{
    return MI_Instance_Destruct(&self->__instance);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_TestTargetResource_Delete(
    _Inout_ OSConfig_PrototypeResource_TestTargetResource* self)
{
    return MI_Instance_Delete(&self->__instance);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_TestTargetResource_Post(
    _In_ const OSConfig_PrototypeResource_TestTargetResource* self,
    _In_ MI_Context* context)
{
    return MI_Context_PostInstance(context, &self->__instance);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_TestTargetResource_Set_MIReturn(
    _Inout_ OSConfig_PrototypeResource_TestTargetResource* self,
    _In_ MI_Uint32 x)
{
    ((MI_Uint32Field*)&self->MIReturn)->value = x;
    ((MI_Uint32Field*)&self->MIReturn)->exists = 1;
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_TestTargetResource_Clear_MIReturn(
    _Inout_ OSConfig_PrototypeResource_TestTargetResource* self)
{
    memset((void*)&self->MIReturn, 0, sizeof(self->MIReturn));
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_TestTargetResource_Set_InputResource(
    _Inout_ OSConfig_PrototypeResource_TestTargetResource* self,
    _In_ const OSConfig_PrototypeResource* x)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        1,
        (MI_Value*)&x,
        MI_INSTANCE,
        0);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_TestTargetResource_SetPtr_InputResource(
    _Inout_ OSConfig_PrototypeResource_TestTargetResource* self,
    _In_ const OSConfig_PrototypeResource* x)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        1,
        (MI_Value*)&x,
        MI_INSTANCE,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_TestTargetResource_Clear_InputResource(
    _Inout_ OSConfig_PrototypeResource_TestTargetResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        1);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_TestTargetResource_Set_Flags(
    _Inout_ OSConfig_PrototypeResource_TestTargetResource* self,
    _In_ MI_Uint32 x)
{
    ((MI_Uint32Field*)&self->Flags)->value = x;
    ((MI_Uint32Field*)&self->Flags)->exists = 1;
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_TestTargetResource_Clear_Flags(
    _Inout_ OSConfig_PrototypeResource_TestTargetResource* self)
{
    memset((void*)&self->Flags, 0, sizeof(self->Flags));
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_TestTargetResource_Set_Result(
    _Inout_ OSConfig_PrototypeResource_TestTargetResource* self,
    _In_ MI_Boolean x)
{
    ((MI_BooleanField*)&self->Result)->value = x;
    ((MI_BooleanField*)&self->Result)->exists = 1;
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_TestTargetResource_Clear_Result(
    _Inout_ OSConfig_PrototypeResource_TestTargetResource* self)
{
    memset((void*)&self->Result, 0, sizeof(self->Result));
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_TestTargetResource_Set_ProviderContext(
    _Inout_ OSConfig_PrototypeResource_TestTargetResource* self,
    _In_ MI_Uint64 x)
{
    ((MI_Uint64Field*)&self->ProviderContext)->value = x;
    ((MI_Uint64Field*)&self->ProviderContext)->exists = 1;
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_TestTargetResource_Clear_ProviderContext(
    _Inout_ OSConfig_PrototypeResource_TestTargetResource* self)
{
    memset((void*)&self->ProviderContext, 0, sizeof(self->ProviderContext));
    return MI_RESULT_OK;
}

/*
**==============================================================================
**
** OSConfig_PrototypeResource.SetTargetResource()
**
**==============================================================================
*/

typedef struct _OSConfig_PrototypeResource_SetTargetResource
{
    MI_Instance __instance;
    /*OUT*/ MI_ConstUint32Field MIReturn;
    /*IN*/ OSConfig_PrototypeResource_ConstRef InputResource;
    /*IN*/ MI_ConstUint64Field ProviderContext;
    /*IN*/ MI_ConstUint32Field Flags;
}
OSConfig_PrototypeResource_SetTargetResource;

MI_EXTERN_C MI_CONST MI_MethodDecl OSConfig_PrototypeResource_SetTargetResource_rtti;

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_SetTargetResource_Construct(
    _Out_ OSConfig_PrototypeResource_SetTargetResource* self,
    _In_ MI_Context* context)
{
    return MI_Context_ConstructParameters(context, &OSConfig_PrototypeResource_SetTargetResource_rtti,
        (MI_Instance*)&self->__instance);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_SetTargetResource_Clone(
    _In_ const OSConfig_PrototypeResource_SetTargetResource* self,
    _Outptr_ OSConfig_PrototypeResource_SetTargetResource** newInstance)
{
    return MI_Instance_Clone(
        &self->__instance, (MI_Instance**)newInstance);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_SetTargetResource_Destruct(
    _Inout_ OSConfig_PrototypeResource_SetTargetResource* self)
{
    return MI_Instance_Destruct(&self->__instance);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_SetTargetResource_Delete(
    _Inout_ OSConfig_PrototypeResource_SetTargetResource* self)
{
    return MI_Instance_Delete(&self->__instance);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_SetTargetResource_Post(
    _In_ const OSConfig_PrototypeResource_SetTargetResource* self,
    _In_ MI_Context* context)
{
    return MI_Context_PostInstance(context, &self->__instance);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_SetTargetResource_Set_MIReturn(
    _Inout_ OSConfig_PrototypeResource_SetTargetResource* self,
    _In_ MI_Uint32 x)
{
    ((MI_Uint32Field*)&self->MIReturn)->value = x;
    ((MI_Uint32Field*)&self->MIReturn)->exists = 1;
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_SetTargetResource_Clear_MIReturn(
    _Inout_ OSConfig_PrototypeResource_SetTargetResource* self)
{
    memset((void*)&self->MIReturn, 0, sizeof(self->MIReturn));
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_SetTargetResource_Set_InputResource(
    _Inout_ OSConfig_PrototypeResource_SetTargetResource* self,
    _In_ const OSConfig_PrototypeResource* x)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        1,
        (MI_Value*)&x,
        MI_INSTANCE,
        0);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_SetTargetResource_SetPtr_InputResource(
    _Inout_ OSConfig_PrototypeResource_SetTargetResource* self,
    _In_ const OSConfig_PrototypeResource* x)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        1,
        (MI_Value*)&x,
        MI_INSTANCE,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_SetTargetResource_Clear_InputResource(
    _Inout_ OSConfig_PrototypeResource_SetTargetResource* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        1);
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_SetTargetResource_Set_ProviderContext(
    _Inout_ OSConfig_PrototypeResource_SetTargetResource* self,
    _In_ MI_Uint64 x)
{
    ((MI_Uint64Field*)&self->ProviderContext)->value = x;
    ((MI_Uint64Field*)&self->ProviderContext)->exists = 1;
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_SetTargetResource_Clear_ProviderContext(
    _Inout_ OSConfig_PrototypeResource_SetTargetResource* self)
{
    memset((void*)&self->ProviderContext, 0, sizeof(self->ProviderContext));
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_SetTargetResource_Set_Flags(
    _Inout_ OSConfig_PrototypeResource_SetTargetResource* self,
    _In_ MI_Uint32 x)
{
    ((MI_Uint32Field*)&self->Flags)->value = x;
    ((MI_Uint32Field*)&self->Flags)->exists = 1;
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL OSConfig_PrototypeResource_SetTargetResource_Clear_Flags(
    _Inout_ OSConfig_PrototypeResource_SetTargetResource* self)
{
    memset((void*)&self->Flags, 0, sizeof(self->Flags));
    return MI_RESULT_OK;
}

/*
**==============================================================================
**
** OSConfig_PrototypeResource provider function prototypes
**
**==============================================================================
*/

/* The developer may optionally define this structure */
typedef struct _OSConfig_PrototypeResource_Self OSConfig_PrototypeResource_Self;

MI_EXTERN_C void MI_CALL OSConfig_PrototypeResource_Load(
    _Outptr_result_maybenull_ OSConfig_PrototypeResource_Self** self,
    _In_opt_ MI_Module_Self* selfModule,
    _In_ MI_Context* context);

MI_EXTERN_C void MI_CALL OSConfig_PrototypeResource_Unload(
    _In_opt_ OSConfig_PrototypeResource_Self* self,
    _In_ MI_Context* context);

MI_EXTERN_C void MI_CALL OSConfig_PrototypeResource_EnumerateInstances(
    _In_opt_ OSConfig_PrototypeResource_Self* self,
    _In_ MI_Context* context,
    _In_opt_z_ const MI_Char* nameSpace,
    _In_opt_z_ const MI_Char* className,
    _In_opt_ const MI_PropertySet* propertySet,
    _In_ MI_Boolean keysOnly,
    _In_opt_ const MI_Filter* filter);

MI_EXTERN_C void MI_CALL OSConfig_PrototypeResource_GetInstance(
    _In_opt_ OSConfig_PrototypeResource_Self* self,
    _In_ MI_Context* context,
    _In_opt_z_ const MI_Char* nameSpace,
    _In_opt_z_ const MI_Char* className,
    _In_ const OSConfig_PrototypeResource* instanceName,
    _In_opt_ const MI_PropertySet* propertySet);

MI_EXTERN_C void MI_CALL OSConfig_PrototypeResource_CreateInstance(
    _In_opt_ OSConfig_PrototypeResource_Self* self,
    _In_ MI_Context* context,
    _In_opt_z_ const MI_Char* nameSpace,
    _In_opt_z_ const MI_Char* className,
    _In_ const OSConfig_PrototypeResource* newInstance);

MI_EXTERN_C void MI_CALL OSConfig_PrototypeResource_ModifyInstance(
    _In_opt_ OSConfig_PrototypeResource_Self* self,
    _In_ MI_Context* context,
    _In_opt_z_ const MI_Char* nameSpace,
    _In_opt_z_ const MI_Char* className,
    _In_ const OSConfig_PrototypeResource* modifiedInstance,
    _In_opt_ const MI_PropertySet* propertySet);

MI_EXTERN_C void MI_CALL OSConfig_PrototypeResource_DeleteInstance(
    _In_opt_ OSConfig_PrototypeResource_Self* self,
    _In_ MI_Context* context,
    _In_opt_z_ const MI_Char* nameSpace,
    _In_opt_z_ const MI_Char* className,
    _In_ const OSConfig_PrototypeResource* instanceName);

MI_EXTERN_C void MI_CALL OSConfig_PrototypeResource_Invoke_GetTargetResource(
    _In_opt_ OSConfig_PrototypeResource_Self* self,
    _In_ MI_Context* context,
    _In_opt_z_ const MI_Char* nameSpace,
    _In_opt_z_ const MI_Char* className,
    _In_opt_z_ const MI_Char* methodName,
    _In_ const OSConfig_PrototypeResource* resourceClass,
    _In_opt_ const OSConfig_PrototypeResource_GetTargetResource* in);

MI_EXTERN_C void MI_CALL OSConfig_PrototypeResource_Invoke_TestTargetResource(
    _In_opt_ OSConfig_PrototypeResource_Self* self,
    _In_ MI_Context* context,
    _In_opt_z_ const MI_Char* nameSpace,
    _In_opt_z_ const MI_Char* className,
    _In_opt_z_ const MI_Char* methodName,
    _In_ const OSConfig_PrototypeResource* resourceClass,
    _In_opt_ const OSConfig_PrototypeResource_TestTargetResource* in);

MI_EXTERN_C void MI_CALL OSConfig_PrototypeResource_Invoke_SetTargetResource(
    _In_opt_ OSConfig_PrototypeResource_Self* self,
    _In_ MI_Context* context,
    _In_opt_z_ const MI_Char* nameSpace,
    _In_opt_z_ const MI_Char* className,
    _In_opt_z_ const MI_Char* methodName,
    _In_ const OSConfig_PrototypeResource* resourceClass,
    _In_opt_ const OSConfig_PrototypeResource_SetTargetResource* in);


#endif /* _OSConfig_PrototypeResource_h */
