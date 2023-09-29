// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

/* @migen@ */
/*
**==============================================================================
**
** WARNING: THIS FILE WAS AUTOMATICALLY GENERATED. PLEASE DO NOT EDIT.
**
**==============================================================================
*/
#ifndef _ReasonClass_h
#define _ReasonClass_h

/*
**==============================================================================
**
** ReasonClass [ReasonClass]
**
** Keys:
**
**==============================================================================
*/

typedef struct _ReasonClass
{
    MI_Instance __instance;
    /* ReasonClass properties */
    MI_ConstStringField Phrase;
    MI_ConstStringField Code;
}
ReasonClass;

typedef struct _ReasonClass_Ref
{
    ReasonClass* value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
ReasonClass_Ref;

typedef struct _ReasonClass_ConstRef
{
    MI_CONST ReasonClass* value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
ReasonClass_ConstRef;

typedef struct _ReasonClass_Array
{
    struct _ReasonClass** data;
    MI_Uint32 size;
}
ReasonClass_Array;

typedef struct _ReasonClass_ConstArray
{
    struct _ReasonClass MI_CONST* MI_CONST* data;
    MI_Uint32 size;
}
ReasonClass_ConstArray;

typedef struct _ReasonClass_ArrayRef
{
    ReasonClass_Array value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
ReasonClass_ArrayRef;

typedef struct _ReasonClass_ConstArrayRef
{
    ReasonClass_ConstArray value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
ReasonClass_ConstArrayRef;

MI_EXTERN_C MI_CONST MI_ClassDecl ReasonClass_rtti;

MI_INLINE MI_Result MI_CALL ReasonClass_Construct(
    ReasonClass* self,
    MI_Context* context)
{
    return MI_Context_ConstructInstance(context, &ReasonClass_rtti,
        (MI_Instance*)&self->__instance);
}

MI_INLINE MI_Result MI_CALL ReasonClass_Clone(
    const ReasonClass* self,
    ReasonClass** newInstance)
{
    return MI_Instance_Clone(
        &self->__instance, (MI_Instance**)newInstance);
}

MI_INLINE MI_Boolean MI_CALL ReasonClass_IsA(
    const MI_Instance* self)
{
    MI_Boolean res = MI_FALSE;
    return MI_Instance_IsA(self, &ReasonClass_rtti, &res) == MI_RESULT_OK && res;
}

MI_INLINE MI_Result MI_CALL ReasonClass_Destruct(ReasonClass* self)
{
    return MI_Instance_Destruct(&self->__instance);
}

MI_INLINE MI_Result MI_CALL ReasonClass_Delete(ReasonClass* self)
{
    return MI_Instance_Delete(&self->__instance);
}

MI_INLINE MI_Result MI_CALL ReasonClass_Post(
    const ReasonClass* self,
    MI_Context* context)
{
    return MI_Context_PostInstance(context, &self->__instance);
}

MI_INLINE MI_Result MI_CALL ReasonClass_Set_Phrase(
    ReasonClass* self,
    const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        0,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL ReasonClass_SetPtr_Phrase(
    ReasonClass* self,
    const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        0,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL ReasonClass_Clear_Phrase(
    ReasonClass* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        0);
}

MI_INLINE MI_Result MI_CALL ReasonClass_Set_Code(
    ReasonClass* self,
    const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        1,
        (MI_Value*)&str,
        MI_STRING,
        0);
}

MI_INLINE MI_Result MI_CALL ReasonClass_SetPtr_Code(
    ReasonClass* self,
    const MI_Char* str)
{
    return self->__instance.ft->SetElementAt(
        (MI_Instance*)&self->__instance,
        1,
        (MI_Value*)&str,
        MI_STRING,
        MI_FLAG_BORROW);
}

MI_INLINE MI_Result MI_CALL ReasonClass_Clear_Code(
    ReasonClass* self)
{
    return self->__instance.ft->ClearElementAt(
        (MI_Instance*)&self->__instance,
        1);
}

#endif /* _ReasonClass_h */
