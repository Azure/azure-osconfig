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
#ifndef _MSFT_Credential_h
#define _MSFT_Credential_h

/*
**==============================================================================
**
** MSFT_Credential [MSFT_Credential]
**
** Keys:
**
**==============================================================================
*/

typedef struct _MSFT_Credential
{
    MI_Instance __instance;
    /* MSFT_Credential properties */
    MI_ConstStringField UserName;
    MI_ConstStringField Password;
}
MSFT_Credential;

typedef struct _MSFT_Credential_Ref
{
    MSFT_Credential* value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MSFT_Credential_Ref;

typedef struct _MSFT_Credential_ConstRef
{
    MI_CONST MSFT_Credential* value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MSFT_Credential_ConstRef;

typedef struct _MSFT_Credential_Array
{
    struct _MSFT_Credential** data;
    MI_Uint32 size;
}
MSFT_Credential_Array;

typedef struct _MSFT_Credential_ConstArray
{
    struct _MSFT_Credential MI_CONST* MI_CONST* data;
    MI_Uint32 size;
}
MSFT_Credential_ConstArray;

typedef struct _MSFT_Credential_ArrayRef
{
    MSFT_Credential_Array value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MSFT_Credential_ArrayRef;

typedef struct _MSFT_Credential_ConstArrayRef
{
    MSFT_Credential_ConstArray value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MSFT_Credential_ConstArrayRef;

MI_EXTERN_C MI_CONST MI_ClassDecl MSFT_Credential_rtti;

MI_INLINE MI_Result MI_CALL MSFT_Credential_Construct(
    MSFT_Credential* self,
    MI_Context* context)
{
    return MI_Context_ConstructInstance(context, &MSFT_Credential_rtti,
        (MI_Instance*)&self->__instance);
}

MI_INLINE MI_Result MI_CALL MSFT_Credential_Clone(
    const MSFT_Credential* self,
    MSFT_Credential** newInstance)
{
    return MI_Instance_Clone(
        &self->__instance, (MI_Instance**)newInstance);
}

MI_INLINE MI_Boolean MI_CALL MSFT_Credential_IsA(
    const MI_Instance* self)
{
    MI_Boolean res = MI_FALSE;
    return MI_Instance_IsA(self, &MSFT_Credential_rtti, &res) == MI_RESULT_OK && res;
}

MI_INLINE MI_Result MI_CALL MSFT_Credential_Destruct(MSFT_Credential* self)
{
    return MI_Instance_Destruct(&self->__instance);
}

MI_INLINE MI_Result MI_CALL MSFT_Credential_Delete(MSFT_Credential* self)
{
    return MI_Instance_Delete(&self->__instance);
}

MI_INLINE MI_Result MI_CALL MSFT_Credential_Post(
    const MSFT_Credential* self,
    MI_Context* context)
{
    return MI_Context_PostInstance(context, &self->__instance);
}

MI_INLINE MI_Result MI_CALL MSFT_Credential_Set_UserName(
    MSFT_Credential* self,
    const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        0,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL MSFT_Credential_SetPtr_UserName(
    MSFT_Credential* self,
    const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        0,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL MSFT_Credential_Clear_UserName(
    MSFT_Credential* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        0);
}

MI_INLINE MI_Result MI_CALL MSFT_Credential_Set_Password(
    MSFT_Credential* self,
    const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        1,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL MSFT_Credential_SetPtr_Password(
    MSFT_Credential* self,
    const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        1,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL MSFT_Credential_Clear_Password(
    MSFT_Credential* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        1);
}


#endif /* _MSFT_Credential_h */
