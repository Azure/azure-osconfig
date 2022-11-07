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
#ifndef _ObjectResult_h
#define _ObjectResult_h

/*
**==============================================================================
**
** ObjectResult [ObjectResult]
**
** Keys:
**
**==============================================================================
*/

typedef struct _ObjectResult
{
    MI_Instance __instance;
    /* ObjectResult properties */
    MI_ConstStringField Result;
    MI_ConstUint32Field StatusCode;
    MI_ConstStringField Description;
}
ObjectResult;

typedef struct _ObjectResult_Ref
{
    ObjectResult* value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
ObjectResult_Ref;

typedef struct _ObjectResult_ConstRef
{
    MI_CONST ObjectResult* value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
ObjectResult_ConstRef;

typedef struct _ObjectResult_Array
{
    struct _ObjectResult** data;
    MI_Uint32 size;
}
ObjectResult_Array;

typedef struct _ObjectResult_ConstArray
{
    struct _ObjectResult MI_CONST* MI_CONST* data;
    MI_Uint32 size;
}
ObjectResult_ConstArray;

typedef struct _ObjectResult_ArrayRef
{
    ObjectResult_Array value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
ObjectResult_ArrayRef;

typedef struct _ObjectResult_ConstArrayRef
{
    ObjectResult_ConstArray value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
ObjectResult_ConstArrayRef;

MI_EXTERN_C MI_CONST MI_ClassDecl ObjectResult_rtti;

MI_INLINE MI_Result MI_CALL ObjectResult_Construct(
    _Out_ ObjectResult* self,
    _In_ MI_Context* context)
{
    return MI_Context_ConstructInstance(context, &ObjectResult_rtti,
        (MI_Instance*)&self->__instance);
}

MI_INLINE MI_Result MI_CALL ObjectResult_Clone(
    _In_ const ObjectResult* self,
    _Outptr_ ObjectResult** newInstance)
{
    return MI_Instance_Clone(
        &self->__instance, (MI_Instance**)newInstance);
}

MI_INLINE MI_Boolean MI_CALL ObjectResult_IsA(
    _In_ const MI_Instance* self)
{
    MI_Boolean res = MI_FALSE;
    return MI_Instance_IsA(self, &ObjectResult_rtti, &res) == MI_RESULT_OK && res;
}

MI_INLINE MI_Result MI_CALL ObjectResult_Destruct(_Inout_ ObjectResult* self)
{
    return MI_Instance_Destruct(&self->__instance);
}

MI_INLINE MI_Result MI_CALL ObjectResult_Delete(_Inout_ ObjectResult* self)
{
    return MI_Instance_Delete(&self->__instance);
}

MI_INLINE MI_Result MI_CALL ObjectResult_Post(
    _In_ const ObjectResult* self,
    _In_ MI_Context* context)
{
    return MI_Context_PostInstance(context, &self->__instance);
}

MI_INLINE MI_Result MI_CALL ObjectResult_Set_Result(
    _Inout_ ObjectResult* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        0,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL ObjectResult_SetPtr_Result(
    _Inout_ ObjectResult* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        0,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL ObjectResult_Clear_Result(
    _Inout_ ObjectResult* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        0);
}

MI_INLINE MI_Result MI_CALL ObjectResult_Set_StatusCode(
    _Inout_ ObjectResult* self,
    _In_ MI_Uint32 x)
{
    ((MI_Uint32Field*)&self->StatusCode)->value = x;
    ((MI_Uint32Field*)&self->StatusCode)->exists = 1;
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL ObjectResult_Clear_StatusCode(
    _Inout_ ObjectResult* self)
{
    memset((void*)&self->StatusCode, 0, sizeof(self->StatusCode));
    return MI_RESULT_OK;
}

MI_INLINE MI_Result MI_CALL ObjectResult_Set_Description(
    _Inout_ ObjectResult* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        2,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL ObjectResult_SetPtr_Description(
    _Inout_ ObjectResult* self,
    _In_z_ const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        2,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL ObjectResult_Clear_Description(
    _Inout_ ObjectResult* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        2);
}


#endif /* _ObjectResult_h */
