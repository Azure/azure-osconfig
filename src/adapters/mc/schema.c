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

/*
**==============================================================================
**
** Schema Declaration
**
**==============================================================================
*/

MI_EXTERN_C MI_SchemaDecl schemaDecl;

/*
**==============================================================================
**
** Qualifier declarations
**
**==============================================================================
*/

static MI_CONST MI_Boolean Abstract_qual_decl_value = 0;

static MI_CONST MI_QualifierDecl Abstract_qual_decl =
{
    MI_T("Abstract"), /* name */
    MI_BOOLEAN, /* type */
    MI_FLAG_ASSOCIATION | MI_FLAG_CLASS | MI_FLAG_INDICATION, /* scope */
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_RESTRICTED, /* flavor */
    0, /* subscript */
    &Abstract_qual_decl_value, /* value */
};

static MI_CONST MI_Boolean Aggregate_qual_decl_value = 0;

static MI_CONST MI_QualifierDecl Aggregate_qual_decl =
{
    MI_T("Aggregate"), /* name */
    MI_BOOLEAN, /* type */
    MI_FLAG_REFERENCE, /* scope */
    MI_FLAG_DISABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    &Aggregate_qual_decl_value, /* value */
};

static MI_CONST MI_Boolean Aggregation_qual_decl_value = 0;

static MI_CONST MI_QualifierDecl Aggregation_qual_decl =
{
    MI_T("Aggregation"), /* name */
    MI_BOOLEAN, /* type */
    MI_FLAG_ASSOCIATION, /* scope */
    MI_FLAG_DISABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    &Aggregation_qual_decl_value, /* value */
};

static MI_CONST MI_QualifierDecl Alias_qual_decl =
{
    MI_T("Alias"), /* name */
    MI_STRING, /* type */
    MI_FLAG_METHOD | MI_FLAG_PROPERTY | MI_FLAG_REFERENCE, /* scope */
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS | MI_FLAG_TRANSLATABLE, /* flavor */
    0, /* subscript */
    NULL, /* value */
};

static MI_CONST MI_Char* ArrayType_qual_decl_value = MI_T("Bag");

static MI_CONST MI_QualifierDecl ArrayType_qual_decl =
{
    MI_T("ArrayType"), /* name */
    MI_STRING, /* type */
    MI_FLAG_PARAMETER | MI_FLAG_PROPERTY, /* scope */
    MI_FLAG_DISABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    &ArrayType_qual_decl_value, /* value */
};

static MI_CONST MI_Boolean Association_qual_decl_value = 0;

static MI_CONST MI_QualifierDecl Association_qual_decl =
{
    MI_T("Association"), /* name */
    MI_BOOLEAN, /* type */
    MI_FLAG_ASSOCIATION, /* scope */
    MI_FLAG_DISABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    &Association_qual_decl_value, /* value */
};

static MI_CONST MI_QualifierDecl BitMap_qual_decl =
{
    MI_T("BitMap"), /* name */
    MI_STRINGA, /* type */
    MI_FLAG_METHOD | MI_FLAG_PARAMETER | MI_FLAG_PROPERTY, /* scope */
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    NULL, /* value */
};

static MI_CONST MI_QualifierDecl BitValues_qual_decl =
{
    MI_T("BitValues"), /* name */
    MI_STRINGA, /* type */
    MI_FLAG_METHOD | MI_FLAG_PARAMETER | MI_FLAG_PROPERTY, /* scope */
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS | MI_FLAG_TRANSLATABLE, /* flavor */
    0, /* subscript */
    NULL, /* value */
};

static MI_CONST MI_QualifierDecl ClassConstraint_qual_decl =
{
    MI_T("ClassConstraint"), /* name */
    MI_STRINGA, /* type */
    MI_FLAG_ASSOCIATION | MI_FLAG_CLASS | MI_FLAG_INDICATION, /* scope */
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    NULL, /* value */
};

static MI_CONST MI_QualifierDecl ClassVersion_qual_decl =
{
    MI_T("ClassVersion"), /* name */
    MI_STRING, /* type */
    MI_FLAG_ASSOCIATION | MI_FLAG_CLASS | MI_FLAG_INDICATION, /* scope */
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_RESTRICTED, /* flavor */
    0, /* subscript */
    NULL, /* value */
};

static MI_CONST MI_Boolean Composition_qual_decl_value = 0;

static MI_CONST MI_QualifierDecl Composition_qual_decl =
{
    MI_T("Composition"), /* name */
    MI_BOOLEAN, /* type */
    MI_FLAG_ASSOCIATION, /* scope */
    MI_FLAG_DISABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    &Composition_qual_decl_value, /* value */
};

static MI_CONST MI_QualifierDecl Correlatable_qual_decl =
{
    MI_T("Correlatable"), /* name */
    MI_STRINGA, /* type */
    MI_FLAG_PROPERTY, /* scope */
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    NULL, /* value */
};

static MI_CONST MI_Boolean Counter_qual_decl_value = 0;

static MI_CONST MI_QualifierDecl Counter_qual_decl =
{
    MI_T("Counter"), /* name */
    MI_BOOLEAN, /* type */
    MI_FLAG_METHOD | MI_FLAG_PARAMETER | MI_FLAG_PROPERTY, /* scope */
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    &Counter_qual_decl_value, /* value */
};

static MI_CONST MI_Boolean Delete_qual_decl_value = 0;

static MI_CONST MI_QualifierDecl Delete_qual_decl =
{
    MI_T("Delete"), /* name */
    MI_BOOLEAN, /* type */
    MI_FLAG_ASSOCIATION | MI_FLAG_REFERENCE, /* scope */
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    &Delete_qual_decl_value, /* value */
};

static MI_CONST MI_QualifierDecl Deprecated_qual_decl =
{
    MI_T("Deprecated"), /* name */
    MI_STRINGA, /* type */
    MI_FLAG_ANY, /* scope */
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_RESTRICTED, /* flavor */
    0, /* subscript */
    NULL, /* value */
};

static MI_CONST MI_QualifierDecl Description_qual_decl =
{
    MI_T("Description"), /* name */
    MI_STRING, /* type */
    MI_FLAG_ANY, /* scope */
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS | MI_FLAG_TRANSLATABLE, /* flavor */
    0, /* subscript */
    NULL, /* value */
};

static MI_CONST MI_QualifierDecl DisplayDescription_qual_decl =
{
    MI_T("DisplayDescription"), /* name */
    MI_STRING, /* type */
    MI_FLAG_ANY, /* scope */
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS | MI_FLAG_TRANSLATABLE, /* flavor */
    0, /* subscript */
    NULL, /* value */
};

static MI_CONST MI_QualifierDecl DisplayName_qual_decl =
{
    MI_T("DisplayName"), /* name */
    MI_STRING, /* type */
    MI_FLAG_ANY, /* scope */
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS | MI_FLAG_TRANSLATABLE, /* flavor */
    0, /* subscript */
    NULL, /* value */
};

static MI_CONST MI_Boolean DN_qual_decl_value = 0;

static MI_CONST MI_QualifierDecl DN_qual_decl =
{
    MI_T("DN"), /* name */
    MI_BOOLEAN, /* type */
    MI_FLAG_METHOD | MI_FLAG_PARAMETER | MI_FLAG_PROPERTY, /* scope */
    MI_FLAG_DISABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    &DN_qual_decl_value, /* value */
};

static MI_CONST MI_QualifierDecl EmbeddedInstance_qual_decl =
{
    MI_T("EmbeddedInstance"), /* name */
    MI_STRING, /* type */
    MI_FLAG_METHOD | MI_FLAG_PARAMETER | MI_FLAG_PROPERTY, /* scope */
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    NULL, /* value */
};

static MI_CONST MI_Boolean EmbeddedObject_qual_decl_value = 0;

static MI_CONST MI_QualifierDecl EmbeddedObject_qual_decl =
{
    MI_T("EmbeddedObject"), /* name */
    MI_BOOLEAN, /* type */
    MI_FLAG_METHOD | MI_FLAG_PARAMETER | MI_FLAG_PROPERTY, /* scope */
    MI_FLAG_DISABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    &EmbeddedObject_qual_decl_value, /* value */
};

static MI_CONST MI_Boolean Exception_qual_decl_value = 0;

static MI_CONST MI_QualifierDecl Exception_qual_decl =
{
    MI_T("Exception"), /* name */
    MI_BOOLEAN, /* type */
    MI_FLAG_CLASS | MI_FLAG_INDICATION, /* scope */
    MI_FLAG_DISABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    &Exception_qual_decl_value, /* value */
};

static MI_CONST MI_Boolean Expensive_qual_decl_value = 0;

static MI_CONST MI_QualifierDecl Expensive_qual_decl =
{
    MI_T("Expensive"), /* name */
    MI_BOOLEAN, /* type */
    MI_FLAG_ANY, /* scope */
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    &Expensive_qual_decl_value, /* value */
};

static MI_CONST MI_Boolean Experimental_qual_decl_value = 0;

static MI_CONST MI_QualifierDecl Experimental_qual_decl =
{
    MI_T("Experimental"), /* name */
    MI_BOOLEAN, /* type */
    MI_FLAG_ANY, /* scope */
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_RESTRICTED, /* flavor */
    0, /* subscript */
    &Experimental_qual_decl_value, /* value */
};

static MI_CONST MI_Boolean Gauge_qual_decl_value = 0;

static MI_CONST MI_QualifierDecl Gauge_qual_decl =
{
    MI_T("Gauge"), /* name */
    MI_BOOLEAN, /* type */
    MI_FLAG_METHOD | MI_FLAG_PARAMETER | MI_FLAG_PROPERTY, /* scope */
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    &Gauge_qual_decl_value, /* value */
};

static MI_CONST MI_Boolean Ifdeleted_qual_decl_value = 0;

static MI_CONST MI_QualifierDecl Ifdeleted_qual_decl =
{
    MI_T("Ifdeleted"), /* name */
    MI_BOOLEAN, /* type */
    MI_FLAG_ASSOCIATION | MI_FLAG_REFERENCE, /* scope */
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    &Ifdeleted_qual_decl_value, /* value */
};

static MI_CONST MI_Boolean In_qual_decl_value = 1;

static MI_CONST MI_QualifierDecl In_qual_decl =
{
    MI_T("In"), /* name */
    MI_BOOLEAN, /* type */
    MI_FLAG_PARAMETER, /* scope */
    MI_FLAG_DISABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    &In_qual_decl_value, /* value */
};

static MI_CONST MI_Boolean Indication_qual_decl_value = 0;

static MI_CONST MI_QualifierDecl Indication_qual_decl =
{
    MI_T("Indication"), /* name */
    MI_BOOLEAN, /* type */
    MI_FLAG_CLASS | MI_FLAG_INDICATION, /* scope */
    MI_FLAG_DISABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    &Indication_qual_decl_value, /* value */
};

static MI_CONST MI_Boolean Invisible_qual_decl_value = 0;

static MI_CONST MI_QualifierDecl Invisible_qual_decl =
{
    MI_T("Invisible"), /* name */
    MI_BOOLEAN, /* type */
    MI_FLAG_ASSOCIATION | MI_FLAG_CLASS | MI_FLAG_METHOD | MI_FLAG_PROPERTY | MI_FLAG_REFERENCE, /* scope */
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    &Invisible_qual_decl_value, /* value */
};

static MI_CONST MI_Boolean IsPUnit_qual_decl_value = 0;

static MI_CONST MI_QualifierDecl IsPUnit_qual_decl =
{
    MI_T("IsPUnit"), /* name */
    MI_BOOLEAN, /* type */
    MI_FLAG_METHOD | MI_FLAG_PARAMETER | MI_FLAG_PROPERTY, /* scope */
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    &IsPUnit_qual_decl_value, /* value */
};

static MI_CONST MI_Boolean Key_qual_decl_value = 0;

static MI_CONST MI_QualifierDecl Key_qual_decl =
{
    MI_T("Key"), /* name */
    MI_BOOLEAN, /* type */
    MI_FLAG_PROPERTY | MI_FLAG_REFERENCE, /* scope */
    MI_FLAG_DISABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    &Key_qual_decl_value, /* value */
};

static MI_CONST MI_Boolean Large_qual_decl_value = 0;

static MI_CONST MI_QualifierDecl Large_qual_decl =
{
    MI_T("Large"), /* name */
    MI_BOOLEAN, /* type */
    MI_FLAG_CLASS | MI_FLAG_PROPERTY, /* scope */
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    &Large_qual_decl_value, /* value */
};

static MI_CONST MI_QualifierDecl MappingStrings_qual_decl =
{
    MI_T("MappingStrings"), /* name */
    MI_STRINGA, /* type */
    MI_FLAG_ANY, /* scope */
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    NULL, /* value */
};

static MI_CONST MI_QualifierDecl Max_qual_decl =
{
    MI_T("Max"), /* name */
    MI_UINT32, /* type */
    MI_FLAG_REFERENCE, /* scope */
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    NULL, /* value */
};

static MI_CONST MI_QualifierDecl MaxLen_qual_decl =
{
    MI_T("MaxLen"), /* name */
    MI_UINT32, /* type */
    MI_FLAG_METHOD | MI_FLAG_PARAMETER | MI_FLAG_PROPERTY, /* scope */
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    NULL, /* value */
};

static MI_CONST MI_QualifierDecl MaxValue_qual_decl =
{
    MI_T("MaxValue"), /* name */
    MI_SINT64, /* type */
    MI_FLAG_METHOD | MI_FLAG_PARAMETER | MI_FLAG_PROPERTY, /* scope */
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    NULL, /* value */
};

static MI_CONST MI_QualifierDecl MethodConstraint_qual_decl =
{
    MI_T("MethodConstraint"), /* name */
    MI_STRINGA, /* type */
    MI_FLAG_METHOD, /* scope */
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    NULL, /* value */
};

static MI_CONST MI_Uint32 Min_qual_decl_value = 0U;

static MI_CONST MI_QualifierDecl Min_qual_decl =
{
    MI_T("Min"), /* name */
    MI_UINT32, /* type */
    MI_FLAG_REFERENCE, /* scope */
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    &Min_qual_decl_value, /* value */
};

static MI_CONST MI_Uint32 MinLen_qual_decl_value = 0U;

static MI_CONST MI_QualifierDecl MinLen_qual_decl =
{
    MI_T("MinLen"), /* name */
    MI_UINT32, /* type */
    MI_FLAG_METHOD | MI_FLAG_PARAMETER | MI_FLAG_PROPERTY, /* scope */
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    &MinLen_qual_decl_value, /* value */
};

static MI_CONST MI_QualifierDecl MinValue_qual_decl =
{
    MI_T("MinValue"), /* name */
    MI_SINT64, /* type */
    MI_FLAG_METHOD | MI_FLAG_PARAMETER | MI_FLAG_PROPERTY, /* scope */
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    NULL, /* value */
};

static MI_CONST MI_QualifierDecl ModelCorrespondence_qual_decl =
{
    MI_T("ModelCorrespondence"), /* name */
    MI_STRINGA, /* type */
    MI_FLAG_ANY, /* scope */
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    NULL, /* value */
};

static MI_CONST MI_QualifierDecl Nonlocal_qual_decl =
{
    MI_T("Nonlocal"), /* name */
    MI_STRING, /* type */
    MI_FLAG_REFERENCE, /* scope */
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    NULL, /* value */
};

static MI_CONST MI_QualifierDecl NonlocalType_qual_decl =
{
    MI_T("NonlocalType"), /* name */
    MI_STRING, /* type */
    MI_FLAG_REFERENCE, /* scope */
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    NULL, /* value */
};

static MI_CONST MI_QualifierDecl NullValue_qual_decl =
{
    MI_T("NullValue"), /* name */
    MI_STRING, /* type */
    MI_FLAG_PROPERTY, /* scope */
    MI_FLAG_DISABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    NULL, /* value */
};

static MI_CONST MI_Boolean Octetstring_qual_decl_value = 0;

static MI_CONST MI_QualifierDecl Octetstring_qual_decl =
{
    MI_T("Octetstring"), /* name */
    MI_BOOLEAN, /* type */
    MI_FLAG_METHOD | MI_FLAG_PARAMETER | MI_FLAG_PROPERTY, /* scope */
    MI_FLAG_DISABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    &Octetstring_qual_decl_value, /* value */
};

static MI_CONST MI_Boolean Out_qual_decl_value = 0;

static MI_CONST MI_QualifierDecl Out_qual_decl =
{
    MI_T("Out"), /* name */
    MI_BOOLEAN, /* type */
    MI_FLAG_PARAMETER, /* scope */
    MI_FLAG_DISABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    &Out_qual_decl_value, /* value */
};

static MI_CONST MI_QualifierDecl Override_qual_decl =
{
    MI_T("Override"), /* name */
    MI_STRING, /* type */
    MI_FLAG_METHOD | MI_FLAG_PROPERTY | MI_FLAG_REFERENCE, /* scope */
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_RESTRICTED, /* flavor */
    0, /* subscript */
    NULL, /* value */
};

static MI_CONST MI_QualifierDecl Propagated_qual_decl =
{
    MI_T("Propagated"), /* name */
    MI_STRING, /* type */
    MI_FLAG_PROPERTY, /* scope */
    MI_FLAG_DISABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    NULL, /* value */
};

static MI_CONST MI_QualifierDecl PropertyConstraint_qual_decl =
{
    MI_T("PropertyConstraint"), /* name */
    MI_STRINGA, /* type */
    MI_FLAG_PROPERTY | MI_FLAG_REFERENCE, /* scope */
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    NULL, /* value */
};

static MI_CONST MI_Char* PropertyUsage_qual_decl_value = MI_T("CurrentContext");

static MI_CONST MI_QualifierDecl PropertyUsage_qual_decl =
{
    MI_T("PropertyUsage"), /* name */
    MI_STRING, /* type */
    MI_FLAG_PROPERTY, /* scope */
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    &PropertyUsage_qual_decl_value, /* value */
};

static MI_CONST MI_QualifierDecl Provider_qual_decl =
{
    MI_T("Provider"), /* name */
    MI_STRING, /* type */
    MI_FLAG_ANY, /* scope */
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    NULL, /* value */
};

static MI_CONST MI_QualifierDecl PUnit_qual_decl =
{
    MI_T("PUnit"), /* name */
    MI_STRING, /* type */
    MI_FLAG_METHOD | MI_FLAG_PARAMETER | MI_FLAG_PROPERTY, /* scope */
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    NULL, /* value */
};

static MI_CONST MI_Boolean Read_qual_decl_value = 1;

static MI_CONST MI_QualifierDecl Read_qual_decl =
{
    MI_T("Read"), /* name */
    MI_BOOLEAN, /* type */
    MI_FLAG_PROPERTY, /* scope */
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    &Read_qual_decl_value, /* value */
};

static MI_CONST MI_Boolean Required_qual_decl_value = 0;

static MI_CONST MI_QualifierDecl Required_qual_decl =
{
    MI_T("Required"), /* name */
    MI_BOOLEAN, /* type */
    MI_FLAG_METHOD | MI_FLAG_PARAMETER | MI_FLAG_PROPERTY | MI_FLAG_REFERENCE, /* scope */
    MI_FLAG_DISABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    &Required_qual_decl_value, /* value */
};

static MI_CONST MI_QualifierDecl Revision_qual_decl =
{
    MI_T("Revision"), /* name */
    MI_STRING, /* type */
    MI_FLAG_ASSOCIATION | MI_FLAG_CLASS | MI_FLAG_INDICATION, /* scope */
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS | MI_FLAG_TRANSLATABLE, /* flavor */
    0, /* subscript */
    NULL, /* value */
};

static MI_CONST MI_QualifierDecl Schema_qual_decl =
{
    MI_T("Schema"), /* name */
    MI_STRING, /* type */
    MI_FLAG_METHOD | MI_FLAG_PROPERTY, /* scope */
    MI_FLAG_DISABLEOVERRIDE | MI_FLAG_TOSUBCLASS | MI_FLAG_TRANSLATABLE, /* flavor */
    0, /* subscript */
    NULL, /* value */
};

static MI_CONST MI_QualifierDecl Source_qual_decl =
{
    MI_T("Source"), /* name */
    MI_STRING, /* type */
    MI_FLAG_ASSOCIATION | MI_FLAG_CLASS | MI_FLAG_INDICATION, /* scope */
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    NULL, /* value */
};

static MI_CONST MI_QualifierDecl SourceType_qual_decl =
{
    MI_T("SourceType"), /* name */
    MI_STRING, /* type */
    MI_FLAG_ASSOCIATION | MI_FLAG_CLASS | MI_FLAG_INDICATION | MI_FLAG_REFERENCE, /* scope */
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    NULL, /* value */
};

static MI_CONST MI_Boolean Static_qual_decl_value = 0;

static MI_CONST MI_QualifierDecl Static_qual_decl =
{
    MI_T("Static"), /* name */
    MI_BOOLEAN, /* type */
    MI_FLAG_METHOD | MI_FLAG_PROPERTY, /* scope */
    MI_FLAG_DISABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    &Static_qual_decl_value, /* value */
};

static MI_CONST MI_Boolean Stream_qual_decl_value = 0;

static MI_CONST MI_QualifierDecl Stream_qual_decl =
{
    MI_T("Stream"), /* name */
    MI_BOOLEAN, /* type */
    MI_FLAG_METHOD | MI_FLAG_PARAMETER, /* scope */
    MI_FLAG_DISABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    &Stream_qual_decl_value, /* value */
};

static MI_CONST MI_QualifierDecl Syntax_qual_decl =
{
    MI_T("Syntax"), /* name */
    MI_STRING, /* type */
    MI_FLAG_METHOD | MI_FLAG_PARAMETER | MI_FLAG_PROPERTY | MI_FLAG_REFERENCE, /* scope */
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    NULL, /* value */
};

static MI_CONST MI_QualifierDecl SyntaxType_qual_decl =
{
    MI_T("SyntaxType"), /* name */
    MI_STRING, /* type */
    MI_FLAG_METHOD | MI_FLAG_PARAMETER | MI_FLAG_PROPERTY | MI_FLAG_REFERENCE, /* scope */
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    NULL, /* value */
};

static MI_CONST MI_Boolean Terminal_qual_decl_value = 0;

static MI_CONST MI_QualifierDecl Terminal_qual_decl =
{
    MI_T("Terminal"), /* name */
    MI_BOOLEAN, /* type */
    MI_FLAG_ASSOCIATION | MI_FLAG_CLASS | MI_FLAG_INDICATION, /* scope */
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    &Terminal_qual_decl_value, /* value */
};

static MI_CONST MI_QualifierDecl TriggerType_qual_decl =
{
    MI_T("TriggerType"), /* name */
    MI_STRING, /* type */
    MI_FLAG_ASSOCIATION | MI_FLAG_CLASS | MI_FLAG_INDICATION | MI_FLAG_METHOD | MI_FLAG_PROPERTY | MI_FLAG_REFERENCE, /* scope */
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    NULL, /* value */
};

static MI_CONST MI_QualifierDecl UMLPackagePath_qual_decl =
{
    MI_T("UMLPackagePath"), /* name */
    MI_STRING, /* type */
    MI_FLAG_ASSOCIATION | MI_FLAG_CLASS | MI_FLAG_INDICATION, /* scope */
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    NULL, /* value */
};

static MI_CONST MI_QualifierDecl Units_qual_decl =
{
    MI_T("Units"), /* name */
    MI_STRING, /* type */
    MI_FLAG_METHOD | MI_FLAG_PARAMETER | MI_FLAG_PROPERTY, /* scope */
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS | MI_FLAG_TRANSLATABLE, /* flavor */
    0, /* subscript */
    NULL, /* value */
};

static MI_CONST MI_QualifierDecl UnknownValues_qual_decl =
{
    MI_T("UnknownValues"), /* name */
    MI_STRINGA, /* type */
    MI_FLAG_PROPERTY, /* scope */
    MI_FLAG_DISABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    NULL, /* value */
};

static MI_CONST MI_QualifierDecl UnsupportedValues_qual_decl =
{
    MI_T("UnsupportedValues"), /* name */
    MI_STRINGA, /* type */
    MI_FLAG_PROPERTY, /* scope */
    MI_FLAG_DISABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    NULL, /* value */
};

static MI_CONST MI_QualifierDecl ValueMap_qual_decl =
{
    MI_T("ValueMap"), /* name */
    MI_STRINGA, /* type */
    MI_FLAG_METHOD | MI_FLAG_PARAMETER | MI_FLAG_PROPERTY, /* scope */
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    NULL, /* value */
};

static MI_CONST MI_QualifierDecl Values_qual_decl =
{
    MI_T("Values"), /* name */
    MI_STRINGA, /* type */
    MI_FLAG_METHOD | MI_FLAG_PARAMETER | MI_FLAG_PROPERTY, /* scope */
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS | MI_FLAG_TRANSLATABLE, /* flavor */
    0, /* subscript */
    NULL, /* value */
};

static MI_CONST MI_QualifierDecl Version_qual_decl =
{
    MI_T("Version"), /* name */
    MI_STRING, /* type */
    MI_FLAG_ASSOCIATION | MI_FLAG_CLASS | MI_FLAG_INDICATION, /* scope */
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TRANSLATABLE | MI_FLAG_RESTRICTED, /* flavor */
    0, /* subscript */
    NULL, /* value */
};

static MI_CONST MI_Boolean Weak_qual_decl_value = 0;

static MI_CONST MI_QualifierDecl Weak_qual_decl =
{
    MI_T("Weak"), /* name */
    MI_BOOLEAN, /* type */
    MI_FLAG_REFERENCE, /* scope */
    MI_FLAG_DISABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    &Weak_qual_decl_value, /* value */
};

static MI_CONST MI_Boolean Write_qual_decl_value = 0;

static MI_CONST MI_QualifierDecl Write_qual_decl =
{
    MI_T("Write"), /* name */
    MI_BOOLEAN, /* type */
    MI_FLAG_PROPERTY, /* scope */
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS, /* flavor */
    0, /* subscript */
    &Write_qual_decl_value, /* value */
};

static MI_QualifierDecl MI_CONST* MI_CONST qualifierDecls[] =
{
    &Abstract_qual_decl,
    &Aggregate_qual_decl,
    &Aggregation_qual_decl,
    &Alias_qual_decl,
    &ArrayType_qual_decl,
    &Association_qual_decl,
    &BitMap_qual_decl,
    &BitValues_qual_decl,
    &ClassConstraint_qual_decl,
    &ClassVersion_qual_decl,
    &Composition_qual_decl,
    &Correlatable_qual_decl,
    &Counter_qual_decl,
    &Delete_qual_decl,
    &Deprecated_qual_decl,
    &Description_qual_decl,
    &DisplayDescription_qual_decl,
    &DisplayName_qual_decl,
    &DN_qual_decl,
    &EmbeddedInstance_qual_decl,
    &EmbeddedObject_qual_decl,
    &Exception_qual_decl,
    &Expensive_qual_decl,
    &Experimental_qual_decl,
    &Gauge_qual_decl,
    &Ifdeleted_qual_decl,
    &In_qual_decl,
    &Indication_qual_decl,
    &Invisible_qual_decl,
    &IsPUnit_qual_decl,
    &Key_qual_decl,
    &Large_qual_decl,
    &MappingStrings_qual_decl,
    &Max_qual_decl,
    &MaxLen_qual_decl,
    &MaxValue_qual_decl,
    &MethodConstraint_qual_decl,
    &Min_qual_decl,
    &MinLen_qual_decl,
    &MinValue_qual_decl,
    &ModelCorrespondence_qual_decl,
    &Nonlocal_qual_decl,
    &NonlocalType_qual_decl,
    &NullValue_qual_decl,
    &Octetstring_qual_decl,
    &Out_qual_decl,
    &Override_qual_decl,
    &Propagated_qual_decl,
    &PropertyConstraint_qual_decl,
    &PropertyUsage_qual_decl,
    &Provider_qual_decl,
    &PUnit_qual_decl,
    &Read_qual_decl,
    &Required_qual_decl,
    &Revision_qual_decl,
    &Schema_qual_decl,
    &Source_qual_decl,
    &SourceType_qual_decl,
    &Static_qual_decl,
    &Stream_qual_decl,
    &Syntax_qual_decl,
    &SyntaxType_qual_decl,
    &Terminal_qual_decl,
    &TriggerType_qual_decl,
    &UMLPackagePath_qual_decl,
    &Units_qual_decl,
    &UnknownValues_qual_decl,
    &UnsupportedValues_qual_decl,
    &ValueMap_qual_decl,
    &Values_qual_decl,
    &Version_qual_decl,
    &Weak_qual_decl,
    &Write_qual_decl,
};

/*
**==============================================================================
**
** MSFT_Credential
**
**==============================================================================
*/

static MI_CONST MI_Char* MSFT_Credential_UserName_Description_qual_value = MI_T("1");

static MI_CONST MI_Qualifier MSFT_Credential_UserName_Description_qual =
{
    MI_T("Description"),
    MI_STRING,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS | MI_FLAG_TRANSLATABLE,
    &MSFT_Credential_UserName_Description_qual_value
};

static MI_CONST MI_Uint32 MSFT_Credential_UserName_MaxLen_qual_value = 256U;

static MI_CONST MI_Qualifier MSFT_Credential_UserName_MaxLen_qual =
{
    MI_T("MaxLen"),
    MI_UINT32,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS,
    &MSFT_Credential_UserName_MaxLen_qual_value
};

static MI_Qualifier MI_CONST* MI_CONST MSFT_Credential_UserName_quals[] =
{
    &MSFT_Credential_UserName_Description_qual,
    &MSFT_Credential_UserName_MaxLen_qual,
};

/* property MSFT_Credential.UserName */
static MI_CONST MI_PropertyDecl MSFT_Credential_UserName_prop =
{
    MI_FLAG_PROPERTY | MI_FLAG_READONLY, /* flags */
    0x00756508, /* code */
    MI_T("UserName"), /* name */
    MSFT_Credential_UserName_quals, /* qualifiers */
    MI_COUNT(MSFT_Credential_UserName_quals), /* numQualifiers */
    MI_STRING, /* type */
    NULL, /* className */
    0, /* subscript */
    offsetof(MSFT_Credential, UserName), /* offset */
    MI_T("MSFT_Credential"), /* origin */
    MI_T("MSFT_Credential"), /* propagator */
    NULL,
};

static MI_CONST MI_Char* MSFT_Credential_Password_Description_qual_value = MI_T("2");

static MI_CONST MI_Qualifier MSFT_Credential_Password_Description_qual =
{
    MI_T("Description"),
    MI_STRING,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS | MI_FLAG_TRANSLATABLE,
    &MSFT_Credential_Password_Description_qual_value
};

static MI_Qualifier MI_CONST* MI_CONST MSFT_Credential_Password_quals[] =
{
    &MSFT_Credential_Password_Description_qual,
};

/* property MSFT_Credential.Password */
static MI_CONST MI_PropertyDecl MSFT_Credential_Password_prop =
{
    MI_FLAG_PROPERTY | MI_FLAG_READONLY, /* flags */
    0x00706408, /* code */
    MI_T("Password"), /* name */
    MSFT_Credential_Password_quals, /* qualifiers */
    MI_COUNT(MSFT_Credential_Password_quals), /* numQualifiers */
    MI_STRING, /* type */
    NULL, /* className */
    0, /* subscript */
    offsetof(MSFT_Credential, Password), /* offset */
    MI_T("MSFT_Credential"), /* origin */
    MI_T("MSFT_Credential"), /* propagator */
    NULL,
};

static MI_PropertyDecl MI_CONST* MI_CONST MSFT_Credential_props[] =
{
    &MSFT_Credential_UserName_prop,
    &MSFT_Credential_Password_prop,
};

static MI_CONST MI_Boolean MSFT_Credential_Abstract_qual_value = 1;

static MI_CONST MI_Qualifier MSFT_Credential_Abstract_qual =
{
    MI_T("Abstract"),
    MI_BOOLEAN,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_RESTRICTED,
    &MSFT_Credential_Abstract_qual_value
};

static MI_CONST MI_Char* MSFT_Credential_ClassVersion_qual_value = MI_T("1.0.0");

static MI_CONST MI_Qualifier MSFT_Credential_ClassVersion_qual =
{
    MI_T("ClassVersion"),
    MI_STRING,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_RESTRICTED,
    &MSFT_Credential_ClassVersion_qual_value
};

static MI_CONST MI_Char* MSFT_Credential_Description_qual_value = MI_T("3");

static MI_CONST MI_Qualifier MSFT_Credential_Description_qual =
{
    MI_T("Description"),
    MI_STRING,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS | MI_FLAG_TRANSLATABLE,
    &MSFT_Credential_Description_qual_value
};

static MI_Qualifier MI_CONST* MI_CONST MSFT_Credential_quals[] =
{
    &MSFT_Credential_Abstract_qual,
    &MSFT_Credential_ClassVersion_qual,
    &MSFT_Credential_Description_qual,
};

/* class MSFT_Credential */
MI_CONST MI_ClassDecl MSFT_Credential_rtti =
{
    MI_FLAG_CLASS | MI_FLAG_ABSTRACT, /* flags */
    0x006D6C0F, /* code */
    MI_T("MSFT_Credential"), /* name */
    MSFT_Credential_quals, /* qualifiers */
    MI_COUNT(MSFT_Credential_quals), /* numQualifiers */
    MSFT_Credential_props, /* properties */
    MI_COUNT(MSFT_Credential_props), /* numProperties */
    sizeof(MSFT_Credential), /* size */
    NULL, /* superClass */
    NULL, /* superClassDecl */
    NULL, /* methods */
    0, /* numMethods */
    &schemaDecl, /* schema */
    NULL, /* functions */
    NULL /* owningClass */
};

/*
**==============================================================================
**
** OMI_BaseResource
**
**==============================================================================
*/

static MI_CONST MI_Char* OMI_BaseResource_ResourceId_Description_qual_value = MI_T("4");

static MI_CONST MI_Qualifier OMI_BaseResource_ResourceId_Description_qual =
{
    MI_T("Description"),
    MI_STRING,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS | MI_FLAG_TRANSLATABLE,
    &OMI_BaseResource_ResourceId_Description_qual_value
};

static MI_CONST MI_Boolean OMI_BaseResource_ResourceId_Required_qual_value = 1;

static MI_CONST MI_Qualifier OMI_BaseResource_ResourceId_Required_qual =
{
    MI_T("Required"),
    MI_BOOLEAN,
    MI_FLAG_DISABLEOVERRIDE | MI_FLAG_TOSUBCLASS,
    &OMI_BaseResource_ResourceId_Required_qual_value
};

static MI_Qualifier MI_CONST* MI_CONST OMI_BaseResource_ResourceId_quals[] =
{
    &OMI_BaseResource_ResourceId_Description_qual,
    &OMI_BaseResource_ResourceId_Required_qual,
};

/* property OMI_BaseResource.ResourceId */
static MI_CONST MI_PropertyDecl OMI_BaseResource_ResourceId_prop =
{
    MI_FLAG_PROPERTY | MI_FLAG_REQUIRED | MI_FLAG_READONLY, /* flags */
    0x0072640A, /* code */
    MI_T("ResourceId"), /* name */
    OMI_BaseResource_ResourceId_quals, /* qualifiers */
    MI_COUNT(OMI_BaseResource_ResourceId_quals), /* numQualifiers */
    MI_STRING, /* type */
    NULL, /* className */
    0, /* subscript */
    offsetof(OMI_BaseResource, ResourceId), /* offset */
    MI_T("OMI_BaseResource"), /* origin */
    MI_T("OMI_BaseResource"), /* propagator */
    NULL,
};

static MI_CONST MI_Char* OMI_BaseResource_SourceInfo_Description_qual_value = MI_T("5");

static MI_CONST MI_Qualifier OMI_BaseResource_SourceInfo_Description_qual =
{
    MI_T("Description"),
    MI_STRING,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS | MI_FLAG_TRANSLATABLE,
    &OMI_BaseResource_SourceInfo_Description_qual_value
};

static MI_CONST MI_Boolean OMI_BaseResource_SourceInfo_Write_qual_value = 1;

static MI_CONST MI_Qualifier OMI_BaseResource_SourceInfo_Write_qual =
{
    MI_T("Write"),
    MI_BOOLEAN,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS,
    &OMI_BaseResource_SourceInfo_Write_qual_value
};

static MI_Qualifier MI_CONST* MI_CONST OMI_BaseResource_SourceInfo_quals[] =
{
    &OMI_BaseResource_SourceInfo_Description_qual,
    &OMI_BaseResource_SourceInfo_Write_qual,
};

/* property OMI_BaseResource.SourceInfo */
static MI_CONST MI_PropertyDecl OMI_BaseResource_SourceInfo_prop =
{
    MI_FLAG_PROPERTY, /* flags */
    0x00736F0A, /* code */
    MI_T("SourceInfo"), /* name */
    OMI_BaseResource_SourceInfo_quals, /* qualifiers */
    MI_COUNT(OMI_BaseResource_SourceInfo_quals), /* numQualifiers */
    MI_STRING, /* type */
    NULL, /* className */
    0, /* subscript */
    offsetof(OMI_BaseResource, SourceInfo), /* offset */
    MI_T("OMI_BaseResource"), /* origin */
    MI_T("OMI_BaseResource"), /* propagator */
    NULL,
};

static MI_CONST MI_Char* OMI_BaseResource_DependsOn_Description_qual_value = MI_T("6");

static MI_CONST MI_Qualifier OMI_BaseResource_DependsOn_Description_qual =
{
    MI_T("Description"),
    MI_STRING,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS | MI_FLAG_TRANSLATABLE,
    &OMI_BaseResource_DependsOn_Description_qual_value
};

static MI_CONST MI_Boolean OMI_BaseResource_DependsOn_Write_qual_value = 1;

static MI_CONST MI_Qualifier OMI_BaseResource_DependsOn_Write_qual =
{
    MI_T("Write"),
    MI_BOOLEAN,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS,
    &OMI_BaseResource_DependsOn_Write_qual_value
};

static MI_Qualifier MI_CONST* MI_CONST OMI_BaseResource_DependsOn_quals[] =
{
    &OMI_BaseResource_DependsOn_Description_qual,
    &OMI_BaseResource_DependsOn_Write_qual,
};

/* property OMI_BaseResource.DependsOn */
static MI_CONST MI_PropertyDecl OMI_BaseResource_DependsOn_prop =
{
    MI_FLAG_PROPERTY, /* flags */
    0x00646E09, /* code */
    MI_T("DependsOn"), /* name */
    OMI_BaseResource_DependsOn_quals, /* qualifiers */
    MI_COUNT(OMI_BaseResource_DependsOn_quals), /* numQualifiers */
    MI_STRINGA, /* type */
    NULL, /* className */
    0, /* subscript */
    offsetof(OMI_BaseResource, DependsOn), /* offset */
    MI_T("OMI_BaseResource"), /* origin */
    MI_T("OMI_BaseResource"), /* propagator */
    NULL,
};

static MI_CONST MI_Char* OMI_BaseResource_ModuleName_Description_qual_value = MI_T("7");

static MI_CONST MI_Qualifier OMI_BaseResource_ModuleName_Description_qual =
{
    MI_T("Description"),
    MI_STRING,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS | MI_FLAG_TRANSLATABLE,
    &OMI_BaseResource_ModuleName_Description_qual_value
};

static MI_CONST MI_Boolean OMI_BaseResource_ModuleName_Required_qual_value = 1;

static MI_CONST MI_Qualifier OMI_BaseResource_ModuleName_Required_qual =
{
    MI_T("Required"),
    MI_BOOLEAN,
    MI_FLAG_DISABLEOVERRIDE | MI_FLAG_TOSUBCLASS,
    &OMI_BaseResource_ModuleName_Required_qual_value
};

static MI_Qualifier MI_CONST* MI_CONST OMI_BaseResource_ModuleName_quals[] =
{
    &OMI_BaseResource_ModuleName_Description_qual,
    &OMI_BaseResource_ModuleName_Required_qual,
};

/* property OMI_BaseResource.ModuleName */
static MI_CONST MI_PropertyDecl OMI_BaseResource_ModuleName_prop =
{
    MI_FLAG_PROPERTY | MI_FLAG_REQUIRED | MI_FLAG_READONLY, /* flags */
    0x006D650A, /* code */
    MI_T("ModuleName"), /* name */
    OMI_BaseResource_ModuleName_quals, /* qualifiers */
    MI_COUNT(OMI_BaseResource_ModuleName_quals), /* numQualifiers */
    MI_STRING, /* type */
    NULL, /* className */
    0, /* subscript */
    offsetof(OMI_BaseResource, ModuleName), /* offset */
    MI_T("OMI_BaseResource"), /* origin */
    MI_T("OMI_BaseResource"), /* propagator */
    NULL,
};

static MI_CONST MI_Char* OMI_BaseResource_ModuleVersion_Description_qual_value = MI_T("8");

static MI_CONST MI_Qualifier OMI_BaseResource_ModuleVersion_Description_qual =
{
    MI_T("Description"),
    MI_STRING,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS | MI_FLAG_TRANSLATABLE,
    &OMI_BaseResource_ModuleVersion_Description_qual_value
};

static MI_CONST MI_Boolean OMI_BaseResource_ModuleVersion_Required_qual_value = 1;

static MI_CONST MI_Qualifier OMI_BaseResource_ModuleVersion_Required_qual =
{
    MI_T("Required"),
    MI_BOOLEAN,
    MI_FLAG_DISABLEOVERRIDE | MI_FLAG_TOSUBCLASS,
    &OMI_BaseResource_ModuleVersion_Required_qual_value
};

static MI_Qualifier MI_CONST* MI_CONST OMI_BaseResource_ModuleVersion_quals[] =
{
    &OMI_BaseResource_ModuleVersion_Description_qual,
    &OMI_BaseResource_ModuleVersion_Required_qual,
};

/* property OMI_BaseResource.ModuleVersion */
static MI_CONST MI_PropertyDecl OMI_BaseResource_ModuleVersion_prop =
{
    MI_FLAG_PROPERTY | MI_FLAG_REQUIRED | MI_FLAG_READONLY, /* flags */
    0x006D6E0D, /* code */
    MI_T("ModuleVersion"), /* name */
    OMI_BaseResource_ModuleVersion_quals, /* qualifiers */
    MI_COUNT(OMI_BaseResource_ModuleVersion_quals), /* numQualifiers */
    MI_STRING, /* type */
    NULL, /* className */
    0, /* subscript */
    offsetof(OMI_BaseResource, ModuleVersion), /* offset */
    MI_T("OMI_BaseResource"), /* origin */
    MI_T("OMI_BaseResource"), /* propagator */
    NULL,
};

static MI_CONST MI_Char* OMI_BaseResource_ConfigurationName_Description_qual_value = MI_T("9");

static MI_CONST MI_Qualifier OMI_BaseResource_ConfigurationName_Description_qual =
{
    MI_T("Description"),
    MI_STRING,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS | MI_FLAG_TRANSLATABLE,
    &OMI_BaseResource_ConfigurationName_Description_qual_value
};

static MI_CONST MI_Boolean OMI_BaseResource_ConfigurationName_Write_qual_value = 1;

static MI_CONST MI_Qualifier OMI_BaseResource_ConfigurationName_Write_qual =
{
    MI_T("Write"),
    MI_BOOLEAN,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS,
    &OMI_BaseResource_ConfigurationName_Write_qual_value
};

static MI_Qualifier MI_CONST* MI_CONST OMI_BaseResource_ConfigurationName_quals[] =
{
    &OMI_BaseResource_ConfigurationName_Description_qual,
    &OMI_BaseResource_ConfigurationName_Write_qual,
};

/* property OMI_BaseResource.ConfigurationName */
static MI_CONST MI_PropertyDecl OMI_BaseResource_ConfigurationName_prop =
{
    MI_FLAG_PROPERTY, /* flags */
    0x00636511, /* code */
    MI_T("ConfigurationName"), /* name */
    OMI_BaseResource_ConfigurationName_quals, /* qualifiers */
    MI_COUNT(OMI_BaseResource_ConfigurationName_quals), /* numQualifiers */
    MI_STRING, /* type */
    NULL, /* className */
    0, /* subscript */
    offsetof(OMI_BaseResource, ConfigurationName), /* offset */
    MI_T("OMI_BaseResource"), /* origin */
    MI_T("OMI_BaseResource"), /* propagator */
    NULL,
};

static MI_CONST MI_Char* OMI_BaseResource_PsDscRunAsCredential_Description_qual_value = MI_T("10");

static MI_CONST MI_Qualifier OMI_BaseResource_PsDscRunAsCredential_Description_qual =
{
    MI_T("Description"),
    MI_STRING,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS | MI_FLAG_TRANSLATABLE,
    &OMI_BaseResource_PsDscRunAsCredential_Description_qual_value
};

static MI_CONST MI_Char* OMI_BaseResource_PsDscRunAsCredential_EmbeddedInstance_qual_value = MI_T("MSFT_Credential");

static MI_CONST MI_Qualifier OMI_BaseResource_PsDscRunAsCredential_EmbeddedInstance_qual =
{
    MI_T("EmbeddedInstance"),
    MI_STRING,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS,
    &OMI_BaseResource_PsDscRunAsCredential_EmbeddedInstance_qual_value
};

static MI_Qualifier MI_CONST* MI_CONST OMI_BaseResource_PsDscRunAsCredential_quals[] =
{
    &OMI_BaseResource_PsDscRunAsCredential_Description_qual,
    &OMI_BaseResource_PsDscRunAsCredential_EmbeddedInstance_qual,
};

/* property OMI_BaseResource.PsDscRunAsCredential */
static MI_CONST MI_PropertyDecl OMI_BaseResource_PsDscRunAsCredential_prop =
{
    MI_FLAG_PROPERTY | MI_FLAG_READONLY, /* flags */
    0x00706C14, /* code */
    MI_T("PsDscRunAsCredential"), /* name */
    OMI_BaseResource_PsDscRunAsCredential_quals, /* qualifiers */
    MI_COUNT(OMI_BaseResource_PsDscRunAsCredential_quals), /* numQualifiers */
    MI_INSTANCE, /* type */
    MI_T("MSFT_Credential"), /* className */
    0, /* subscript */
    offsetof(OMI_BaseResource, PsDscRunAsCredential), /* offset */
    MI_T("OMI_BaseResource"), /* origin */
    MI_T("OMI_BaseResource"), /* propagator */
    NULL,
};

static MI_PropertyDecl MI_CONST* MI_CONST OMI_BaseResource_props[] =
{
    &OMI_BaseResource_ResourceId_prop,
    &OMI_BaseResource_SourceInfo_prop,
    &OMI_BaseResource_DependsOn_prop,
    &OMI_BaseResource_ModuleName_prop,
    &OMI_BaseResource_ModuleVersion_prop,
    &OMI_BaseResource_ConfigurationName_prop,
    &OMI_BaseResource_PsDscRunAsCredential_prop,
};

static MI_CONST MI_Boolean OMI_BaseResource_Abstract_qual_value = 1;

static MI_CONST MI_Qualifier OMI_BaseResource_Abstract_qual =
{
    MI_T("Abstract"),
    MI_BOOLEAN,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_RESTRICTED,
    &OMI_BaseResource_Abstract_qual_value
};

static MI_CONST MI_Char* OMI_BaseResource_ClassVersion_qual_value = MI_T("1.0.0");

static MI_CONST MI_Qualifier OMI_BaseResource_ClassVersion_qual =
{
    MI_T("ClassVersion"),
    MI_STRING,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_RESTRICTED,
    &OMI_BaseResource_ClassVersion_qual_value
};

static MI_CONST MI_Char* OMI_BaseResource_Description_qual_value = MI_T("11");

static MI_CONST MI_Qualifier OMI_BaseResource_Description_qual =
{
    MI_T("Description"),
    MI_STRING,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS | MI_FLAG_TRANSLATABLE,
    &OMI_BaseResource_Description_qual_value
};

static MI_Qualifier MI_CONST* MI_CONST OMI_BaseResource_quals[] =
{
    &OMI_BaseResource_Abstract_qual,
    &OMI_BaseResource_ClassVersion_qual,
    &OMI_BaseResource_Description_qual,
};

/* class OMI_BaseResource */
MI_CONST MI_ClassDecl OMI_BaseResource_rtti =
{
    MI_FLAG_CLASS | MI_FLAG_ABSTRACT, /* flags */
    0x006F6510, /* code */
    MI_T("OMI_BaseResource"), /* name */
    OMI_BaseResource_quals, /* qualifiers */
    MI_COUNT(OMI_BaseResource_quals), /* numQualifiers */
    OMI_BaseResource_props, /* properties */
    MI_COUNT(OMI_BaseResource_props), /* numProperties */
    sizeof(OMI_BaseResource), /* size */
    NULL, /* superClass */
    NULL, /* superClassDecl */
    NULL, /* methods */
    0, /* numMethods */
    &schemaDecl, /* schema */
    NULL, /* functions */
    NULL /* owningClass */
};

/*
**==============================================================================
**
** ReasonClass
**
**==============================================================================
*/

static MI_CONST MI_Boolean ReasonClass_Phrase_Read_qual_value = 1;

static MI_CONST MI_Qualifier ReasonClass_Phrase_Read_qual =
{
    MI_T("Read"),
    MI_BOOLEAN,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS,
    &ReasonClass_Phrase_Read_qual_value
};

static MI_Qualifier MI_CONST* MI_CONST ReasonClass_Phrase_quals[] =
{
    &ReasonClass_Phrase_Read_qual,
};

/* property ReasonClass.Phrase */
static MI_CONST MI_PropertyDecl ReasonClass_Phrase_prop =
{
    MI_FLAG_PROPERTY | MI_FLAG_READONLY, /* flags */
    0x00706506, /* code */
    MI_T("Phrase"), /* name */
    ReasonClass_Phrase_quals, /* qualifiers */
    MI_COUNT(ReasonClass_Phrase_quals), /* numQualifiers */
    MI_STRING, /* type */
    NULL, /* className */
    0, /* subscript */
    offsetof(ReasonClass, Phrase), /* offset */
    MI_T("ReasonClass"), /* origin */
    MI_T("ReasonClass"), /* propagator */
    NULL,
};

static MI_CONST MI_Boolean ReasonClass_Code_Read_qual_value = 1;

static MI_CONST MI_Qualifier ReasonClass_Code_Read_qual =
{
    MI_T("Read"),
    MI_BOOLEAN,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS,
    &ReasonClass_Code_Read_qual_value
};

static MI_Qualifier MI_CONST* MI_CONST ReasonClass_Code_quals[] =
{
    &ReasonClass_Code_Read_qual,
};

/* property ReasonClass.Code */
static MI_CONST MI_PropertyDecl ReasonClass_Code_prop =
{
    MI_FLAG_PROPERTY | MI_FLAG_READONLY, /* flags */
    0x00636504, /* code */
    MI_T("Code"), /* name */
    ReasonClass_Code_quals, /* qualifiers */
    MI_COUNT(ReasonClass_Code_quals), /* numQualifiers */
    MI_STRING, /* type */
    NULL, /* className */
    0, /* subscript */
    offsetof(ReasonClass, Code), /* offset */
    MI_T("ReasonClass"), /* origin */
    MI_T("ReasonClass"), /* propagator */
    NULL,
};

static MI_PropertyDecl MI_CONST* MI_CONST ReasonClass_props[] =
{
    &ReasonClass_Phrase_prop,
    &ReasonClass_Code_prop,
};

static MI_CONST MI_Char* ReasonClass_ClassVersion_qual_value = MI_T("1.0.0");

static MI_CONST MI_Qualifier ReasonClass_ClassVersion_qual =
{
    MI_T("ClassVersion"),
    MI_STRING,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_RESTRICTED,
    &ReasonClass_ClassVersion_qual_value
};

static MI_Qualifier MI_CONST* MI_CONST ReasonClass_quals[] =
{
    &ReasonClass_ClassVersion_qual,
};

/* class ReasonClass */
MI_CONST MI_ClassDecl ReasonClass_rtti =
{
    MI_FLAG_CLASS, /* flags */
    0x0072730B, /* code */
    MI_T("ReasonClass"), /* name */
    ReasonClass_quals, /* qualifiers */
    MI_COUNT(ReasonClass_quals), /* numQualifiers */
    ReasonClass_props, /* properties */
    MI_COUNT(ReasonClass_props), /* numProperties */
    sizeof(ReasonClass), /* size */
    NULL, /* superClass */
    NULL, /* superClassDecl */
    NULL, /* methods */
    0, /* numMethods */
    &schemaDecl, /* schema */
    NULL, /* functions */
    NULL /* owningClass */
};

/*
**==============================================================================
**
** OsConfigResource
**
**==============================================================================
*/

static MI_CONST MI_Boolean OsConfigResource_RuleId_Key_qual_value = 1;

static MI_CONST MI_Qualifier OsConfigResource_RuleId_Key_qual =
{
    MI_T("Key"),
    MI_BOOLEAN,
    MI_FLAG_DISABLEOVERRIDE | MI_FLAG_TOSUBCLASS,
    &OsConfigResource_RuleId_Key_qual_value
};

static MI_Qualifier MI_CONST* MI_CONST OsConfigResource_RuleId_quals[] =
{
    &OsConfigResource_RuleId_Key_qual,
};

/* property OsConfigResource.RuleId */
static MI_CONST MI_PropertyDecl OsConfigResource_RuleId_prop =
{
    MI_FLAG_PROPERTY | MI_FLAG_KEY | MI_FLAG_READONLY, /* flags */
    0x00726406, /* code */
    MI_T("RuleId"), /* name */
    OsConfigResource_RuleId_quals, /* qualifiers */
    MI_COUNT(OsConfigResource_RuleId_quals), /* numQualifiers */
    MI_STRING, /* type */
    NULL, /* className */
    0, /* subscript */
    offsetof(OsConfigResource, RuleId), /* offset */
    MI_T("OsConfigResource"), /* origin */
    MI_T("OsConfigResource"), /* propagator */
    NULL,
};

static MI_CONST MI_Boolean OsConfigResource_PayloadKey_Write_qual_value = 1;

static MI_CONST MI_Qualifier OsConfigResource_PayloadKey_Write_qual =
{
    MI_T("Write"),
    MI_BOOLEAN,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS,
    &OsConfigResource_PayloadKey_Write_qual_value
};

static MI_Qualifier MI_CONST* MI_CONST OsConfigResource_PayloadKey_quals[] =
{
    &OsConfigResource_PayloadKey_Write_qual,
};

/* property OsConfigResource.PayloadKey */
static MI_CONST MI_PropertyDecl OsConfigResource_PayloadKey_prop =
{
    MI_FLAG_PROPERTY, /* flags */
    0x0070790A, /* code */
    MI_T("PayloadKey"), /* name */
    OsConfigResource_PayloadKey_quals, /* qualifiers */
    MI_COUNT(OsConfigResource_PayloadKey_quals), /* numQualifiers */
    MI_STRING, /* type */
    NULL, /* className */
    0, /* subscript */
    offsetof(OsConfigResource, PayloadKey), /* offset */
    MI_T("OsConfigResource"), /* origin */
    MI_T("OsConfigResource"), /* propagator */
    NULL,
};

static MI_CONST MI_Boolean OsConfigResource_ComponentName_Write_qual_value = 1;

static MI_CONST MI_Qualifier OsConfigResource_ComponentName_Write_qual =
{
    MI_T("Write"),
    MI_BOOLEAN,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS,
    &OsConfigResource_ComponentName_Write_qual_value
};

static MI_CONST MI_Char* OsConfigResource_ComponentName_Description_qual_value = MI_T("12");

static MI_CONST MI_Qualifier OsConfigResource_ComponentName_Description_qual =
{
    MI_T("Description"),
    MI_STRING,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS | MI_FLAG_TRANSLATABLE,
    &OsConfigResource_ComponentName_Description_qual_value
};

static MI_Qualifier MI_CONST* MI_CONST OsConfigResource_ComponentName_quals[] =
{
    &OsConfigResource_ComponentName_Write_qual,
    &OsConfigResource_ComponentName_Description_qual,
};

/* property OsConfigResource.ComponentName */
static MI_CONST MI_PropertyDecl OsConfigResource_ComponentName_prop =
{
    MI_FLAG_PROPERTY, /* flags */
    0x0063650D, /* code */
    MI_T("ComponentName"), /* name */
    OsConfigResource_ComponentName_quals, /* qualifiers */
    MI_COUNT(OsConfigResource_ComponentName_quals), /* numQualifiers */
    MI_STRING, /* type */
    NULL, /* className */
    0, /* subscript */
    offsetof(OsConfigResource, ComponentName), /* offset */
    MI_T("OsConfigResource"), /* origin */
    MI_T("OsConfigResource"), /* propagator */
    NULL,
};

static MI_CONST MI_Boolean OsConfigResource_InitObjectName_Write_qual_value = 1;

static MI_CONST MI_Qualifier OsConfigResource_InitObjectName_Write_qual =
{
    MI_T("Write"),
    MI_BOOLEAN,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS,
    &OsConfigResource_InitObjectName_Write_qual_value
};

static MI_CONST MI_Char* OsConfigResource_InitObjectName_Description_qual_value = MI_T("13");

static MI_CONST MI_Qualifier OsConfigResource_InitObjectName_Description_qual =
{
    MI_T("Description"),
    MI_STRING,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS | MI_FLAG_TRANSLATABLE,
    &OsConfigResource_InitObjectName_Description_qual_value
};

static MI_Qualifier MI_CONST* MI_CONST OsConfigResource_InitObjectName_quals[] =
{
    &OsConfigResource_InitObjectName_Write_qual,
    &OsConfigResource_InitObjectName_Description_qual,
};

/* property OsConfigResource.InitObjectName */
static MI_CONST MI_PropertyDecl OsConfigResource_InitObjectName_prop =
{
    MI_FLAG_PROPERTY, /* flags */
    0x0069650E, /* code */
    MI_T("InitObjectName"), /* name */
    OsConfigResource_InitObjectName_quals, /* qualifiers */
    MI_COUNT(OsConfigResource_InitObjectName_quals), /* numQualifiers */
    MI_STRING, /* type */
    NULL, /* className */
    0, /* subscript */
    offsetof(OsConfigResource, InitObjectName), /* offset */
    MI_T("OsConfigResource"), /* origin */
    MI_T("OsConfigResource"), /* propagator */
    NULL,
};

static MI_CONST MI_Boolean OsConfigResource_ProcedureObjectName_Write_qual_value = 1;

static MI_CONST MI_Qualifier OsConfigResource_ProcedureObjectName_Write_qual =
{
    MI_T("Write"),
    MI_BOOLEAN,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS,
    &OsConfigResource_ProcedureObjectName_Write_qual_value
};

static MI_CONST MI_Char* OsConfigResource_ProcedureObjectName_Description_qual_value = MI_T("28");

static MI_CONST MI_Qualifier OsConfigResource_ProcedureObjectName_Description_qual =
{
    MI_T("Description"),
    MI_STRING,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS | MI_FLAG_TRANSLATABLE,
    &OsConfigResource_ProcedureObjectName_Description_qual_value
};

static MI_Qualifier MI_CONST* MI_CONST OsConfigResource_ProcedureObjectName_quals[] =
{
    &OsConfigResource_ProcedureObjectName_Write_qual,
    &OsConfigResource_ProcedureObjectName_Description_qual,
};

/* property OsConfigResource.ProcedureObjectName */
static MI_CONST MI_PropertyDecl OsConfigResource_ProcedureObjectName_prop =
{
    MI_FLAG_PROPERTY, /* flags */
    0x00706513, /* code */
    MI_T("ProcedureObjectName"), /* name */
    OsConfigResource_ProcedureObjectName_quals, /* qualifiers */
    MI_COUNT(OsConfigResource_ProcedureObjectName_quals), /* numQualifiers */
    MI_STRING, /* type */
    NULL, /* className */
    0, /* subscript */
    offsetof(OsConfigResource, ProcedureObjectName), /* offset */
    MI_T("OsConfigResource"), /* origin */
    MI_T("OsConfigResource"), /* propagator */
    NULL,
};

static MI_CONST MI_Boolean OsConfigResource_ProcedureObjectValue_Write_qual_value = 1;

static MI_CONST MI_Qualifier OsConfigResource_ProcedureObjectValue_Write_qual =
{
    MI_T("Write"),
    MI_BOOLEAN,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS,
    &OsConfigResource_ProcedureObjectValue_Write_qual_value
};

static MI_CONST MI_Char* OsConfigResource_ProcedureObjectValue_Description_qual_value = MI_T("29");

static MI_CONST MI_Qualifier OsConfigResource_ProcedureObjectValue_Description_qual =
{
    MI_T("Description"),
    MI_STRING,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS | MI_FLAG_TRANSLATABLE,
    &OsConfigResource_ProcedureObjectValue_Description_qual_value
};

static MI_Qualifier MI_CONST* MI_CONST OsConfigResource_ProcedureObjectValue_quals[] =
{
    &OsConfigResource_ProcedureObjectValue_Write_qual,
    &OsConfigResource_ProcedureObjectValue_Description_qual,
};

/* property OsConfigResource.ProcedureObjectValue */
static MI_CONST MI_PropertyDecl OsConfigResource_ProcedureObjectValue_prop =
{
    MI_FLAG_PROPERTY, /* flags */
    0x00706514, /* code */
    MI_T("ProcedureObjectValue"), /* name */
    OsConfigResource_ProcedureObjectValue_quals, /* qualifiers */
    MI_COUNT(OsConfigResource_ProcedureObjectValue_quals), /* numQualifiers */
    MI_STRING, /* type */
    NULL, /* className */
    0, /* subscript */
    offsetof(OsConfigResource, ProcedureObjectValue), /* offset */
    MI_T("OsConfigResource"), /* origin */
    MI_T("OsConfigResource"), /* propagator */
    NULL,
};

static MI_CONST MI_Boolean OsConfigResource_ReportedObjectName_Write_qual_value = 1;

static MI_CONST MI_Qualifier OsConfigResource_ReportedObjectName_Write_qual =
{
    MI_T("Write"),
    MI_BOOLEAN,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS,
    &OsConfigResource_ReportedObjectName_Write_qual_value
};

static MI_CONST MI_Char* OsConfigResource_ReportedObjectName_Description_qual_value = MI_T("14");

static MI_CONST MI_Qualifier OsConfigResource_ReportedObjectName_Description_qual =
{
    MI_T("Description"),
    MI_STRING,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS | MI_FLAG_TRANSLATABLE,
    &OsConfigResource_ReportedObjectName_Description_qual_value
};

static MI_Qualifier MI_CONST* MI_CONST OsConfigResource_ReportedObjectName_quals[] =
{
    &OsConfigResource_ReportedObjectName_Write_qual,
    &OsConfigResource_ReportedObjectName_Description_qual,
};

/* property OsConfigResource.ReportedObjectName */
static MI_CONST MI_PropertyDecl OsConfigResource_ReportedObjectName_prop =
{
    MI_FLAG_PROPERTY, /* flags */
    0x00726512, /* code */
    MI_T("ReportedObjectName"), /* name */
    OsConfigResource_ReportedObjectName_quals, /* qualifiers */
    MI_COUNT(OsConfigResource_ReportedObjectName_quals), /* numQualifiers */
    MI_STRING, /* type */
    NULL, /* className */
    0, /* subscript */
    offsetof(OsConfigResource, ReportedObjectName), /* offset */
    MI_T("OsConfigResource"), /* origin */
    MI_T("OsConfigResource"), /* propagator */
    NULL,
};

static MI_CONST MI_Boolean OsConfigResource_ReportedObjectValue_Read_qual_value = 1;

static MI_CONST MI_Qualifier OsConfigResource_ReportedObjectValue_Read_qual =
{
    MI_T("Read"),
    MI_BOOLEAN,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS,
    &OsConfigResource_ReportedObjectValue_Read_qual_value
};

static MI_CONST MI_Char* OsConfigResource_ReportedObjectValue_Description_qual_value = MI_T("15");

static MI_CONST MI_Qualifier OsConfigResource_ReportedObjectValue_Description_qual =
{
    MI_T("Description"),
    MI_STRING,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS | MI_FLAG_TRANSLATABLE,
    &OsConfigResource_ReportedObjectValue_Description_qual_value
};

static MI_Qualifier MI_CONST* MI_CONST OsConfigResource_ReportedObjectValue_quals[] =
{
    &OsConfigResource_ReportedObjectValue_Read_qual,
    &OsConfigResource_ReportedObjectValue_Description_qual,
};

/* property OsConfigResource.ReportedObjectValue */
static MI_CONST MI_PropertyDecl OsConfigResource_ReportedObjectValue_prop =
{
    MI_FLAG_PROPERTY | MI_FLAG_READONLY, /* flags */
    0x00726513, /* code */
    MI_T("ReportedObjectValue"), /* name */
    OsConfigResource_ReportedObjectValue_quals, /* qualifiers */
    MI_COUNT(OsConfigResource_ReportedObjectValue_quals), /* numQualifiers */
    MI_STRING, /* type */
    NULL, /* className */
    0, /* subscript */
    offsetof(OsConfigResource, ReportedObjectValue), /* offset */
    MI_T("OsConfigResource"), /* origin */
    MI_T("OsConfigResource"), /* propagator */
    NULL,
};

static MI_CONST MI_Boolean OsConfigResource_ExpectedObjectValue_Write_qual_value = 1;

static MI_CONST MI_Qualifier OsConfigResource_ExpectedObjectValue_Write_qual =
{
    MI_T("Write"),
    MI_BOOLEAN,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS,
    &OsConfigResource_ExpectedObjectValue_Write_qual_value
};

static MI_CONST MI_Char* OsConfigResource_ExpectedObjectValue_Description_qual_value = MI_T("16");

static MI_CONST MI_Qualifier OsConfigResource_ExpectedObjectValue_Description_qual =
{
    MI_T("Description"),
    MI_STRING,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS | MI_FLAG_TRANSLATABLE,
    &OsConfigResource_ExpectedObjectValue_Description_qual_value
};

static MI_Qualifier MI_CONST* MI_CONST OsConfigResource_ExpectedObjectValue_quals[] =
{
    &OsConfigResource_ExpectedObjectValue_Write_qual,
    &OsConfigResource_ExpectedObjectValue_Description_qual,
};

/* property OsConfigResource.ExpectedObjectValue */
static MI_CONST MI_PropertyDecl OsConfigResource_ExpectedObjectValue_prop =
{
    MI_FLAG_PROPERTY, /* flags */
    0x00656513, /* code */
    MI_T("ExpectedObjectValue"), /* name */
    OsConfigResource_ExpectedObjectValue_quals, /* qualifiers */
    MI_COUNT(OsConfigResource_ExpectedObjectValue_quals), /* numQualifiers */
    MI_STRING, /* type */
    NULL, /* className */
    0, /* subscript */
    offsetof(OsConfigResource, ExpectedObjectValue), /* offset */
    MI_T("OsConfigResource"), /* origin */
    MI_T("OsConfigResource"), /* propagator */
    NULL,
};

static MI_CONST MI_Boolean OsConfigResource_DesiredObjectName_Write_qual_value = 1;

static MI_CONST MI_Qualifier OsConfigResource_DesiredObjectName_Write_qual =
{
    MI_T("Write"),
    MI_BOOLEAN,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS,
    &OsConfigResource_DesiredObjectName_Write_qual_value
};

static MI_CONST MI_Char* OsConfigResource_DesiredObjectName_Description_qual_value = MI_T("17");

static MI_CONST MI_Qualifier OsConfigResource_DesiredObjectName_Description_qual =
{
    MI_T("Description"),
    MI_STRING,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS | MI_FLAG_TRANSLATABLE,
    &OsConfigResource_DesiredObjectName_Description_qual_value
};

static MI_Qualifier MI_CONST* MI_CONST OsConfigResource_DesiredObjectName_quals[] =
{
    &OsConfigResource_DesiredObjectName_Write_qual,
    &OsConfigResource_DesiredObjectName_Description_qual,
};

/* property OsConfigResource.DesiredObjectName */
static MI_CONST MI_PropertyDecl OsConfigResource_DesiredObjectName_prop =
{
    MI_FLAG_PROPERTY, /* flags */
    0x00646511, /* code */
    MI_T("DesiredObjectName"), /* name */
    OsConfigResource_DesiredObjectName_quals, /* qualifiers */
    MI_COUNT(OsConfigResource_DesiredObjectName_quals), /* numQualifiers */
    MI_STRING, /* type */
    NULL, /* className */
    0, /* subscript */
    offsetof(OsConfigResource, DesiredObjectName), /* offset */
    MI_T("OsConfigResource"), /* origin */
    MI_T("OsConfigResource"), /* propagator */
    NULL,
};

static MI_CONST MI_Boolean OsConfigResource_DesiredObjectValue_Write_qual_value = 1;

static MI_CONST MI_Qualifier OsConfigResource_DesiredObjectValue_Write_qual =
{
    MI_T("Write"),
    MI_BOOLEAN,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS,
    &OsConfigResource_DesiredObjectValue_Write_qual_value
};

static MI_CONST MI_Char* OsConfigResource_DesiredObjectValue_Description_qual_value = MI_T("18");

static MI_CONST MI_Qualifier OsConfigResource_DesiredObjectValue_Description_qual =
{
    MI_T("Description"),
    MI_STRING,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS | MI_FLAG_TRANSLATABLE,
    &OsConfigResource_DesiredObjectValue_Description_qual_value
};

static MI_Qualifier MI_CONST* MI_CONST OsConfigResource_DesiredObjectValue_quals[] =
{
    &OsConfigResource_DesiredObjectValue_Write_qual,
    &OsConfigResource_DesiredObjectValue_Description_qual,
};

/* property OsConfigResource.DesiredObjectValue */
static MI_CONST MI_PropertyDecl OsConfigResource_DesiredObjectValue_prop =
{
    MI_FLAG_PROPERTY, /* flags */
    0x00646512, /* code */
    MI_T("DesiredObjectValue"), /* name */
    OsConfigResource_DesiredObjectValue_quals, /* qualifiers */
    MI_COUNT(OsConfigResource_DesiredObjectValue_quals), /* numQualifiers */
    MI_STRING, /* type */
    NULL, /* className */
    0, /* subscript */
    offsetof(OsConfigResource, DesiredObjectValue), /* offset */
    MI_T("OsConfigResource"), /* origin */
    MI_T("OsConfigResource"), /* propagator */
    NULL,
};

static MI_CONST MI_Boolean OsConfigResource_ReportedMpiResult_Read_qual_value = 1;

static MI_CONST MI_Qualifier OsConfigResource_ReportedMpiResult_Read_qual =
{
    MI_T("Read"),
    MI_BOOLEAN,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS,
    &OsConfigResource_ReportedMpiResult_Read_qual_value
};

static MI_CONST MI_Char* OsConfigResource_ReportedMpiResult_Description_qual_value = MI_T("19");

static MI_CONST MI_Qualifier OsConfigResource_ReportedMpiResult_Description_qual =
{
    MI_T("Description"),
    MI_STRING,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS | MI_FLAG_TRANSLATABLE,
    &OsConfigResource_ReportedMpiResult_Description_qual_value
};

static MI_Qualifier MI_CONST* MI_CONST OsConfigResource_ReportedMpiResult_quals[] =
{
    &OsConfigResource_ReportedMpiResult_Read_qual,
    &OsConfigResource_ReportedMpiResult_Description_qual,
};

/* property OsConfigResource.ReportedMpiResult */
static MI_CONST MI_PropertyDecl OsConfigResource_ReportedMpiResult_prop =
{
    MI_FLAG_PROPERTY | MI_FLAG_READONLY, /* flags */
    0x00727411, /* code */
    MI_T("ReportedMpiResult"), /* name */
    OsConfigResource_ReportedMpiResult_quals, /* qualifiers */
    MI_COUNT(OsConfigResource_ReportedMpiResult_quals), /* numQualifiers */
    MI_UINT32, /* type */
    NULL, /* className */
    0, /* subscript */
    offsetof(OsConfigResource, ReportedMpiResult), /* offset */
    MI_T("OsConfigResource"), /* origin */
    MI_T("OsConfigResource"), /* propagator */
    NULL,
};

static MI_CONST MI_Boolean OsConfigResource_Reasons_Read_qual_value = 1;

static MI_CONST MI_Qualifier OsConfigResource_Reasons_Read_qual =
{
    MI_T("Read"),
    MI_BOOLEAN,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS,
    &OsConfigResource_Reasons_Read_qual_value
};

static MI_CONST MI_Char* OsConfigResource_Reasons_EmbeddedInstance_qual_value = MI_T("ReasonClass");

static MI_CONST MI_Qualifier OsConfigResource_Reasons_EmbeddedInstance_qual =
{
    MI_T("EmbeddedInstance"),
    MI_STRING,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS,
    &OsConfigResource_Reasons_EmbeddedInstance_qual_value
};

static MI_Qualifier MI_CONST* MI_CONST OsConfigResource_Reasons_quals[] =
{
    &OsConfigResource_Reasons_Read_qual,
    &OsConfigResource_Reasons_EmbeddedInstance_qual,
};

/* property OsConfigResource.Reasons */
static MI_CONST MI_PropertyDecl OsConfigResource_Reasons_prop =
{
    MI_FLAG_PROPERTY | MI_FLAG_READONLY, /* flags */
    0x00727307, /* code */
    MI_T("Reasons"), /* name */
    OsConfigResource_Reasons_quals, /* qualifiers */
    MI_COUNT(OsConfigResource_Reasons_quals), /* numQualifiers */
    MI_INSTANCEA, /* type */
    MI_T("ReasonClass"), /* className */
    0, /* subscript */
    offsetof(OsConfigResource, Reasons), /* offset */
    MI_T("OsConfigResource"), /* origin */
    MI_T("OsConfigResource"), /* propagator */
    NULL,
};

static MI_PropertyDecl MI_CONST* MI_CONST OsConfigResource_props[] =
{
    &OMI_BaseResource_ResourceId_prop,
    &OMI_BaseResource_SourceInfo_prop,
    &OMI_BaseResource_DependsOn_prop,
    &OMI_BaseResource_ModuleName_prop,
    &OMI_BaseResource_ModuleVersion_prop,
    &OMI_BaseResource_ConfigurationName_prop,
    &OMI_BaseResource_PsDscRunAsCredential_prop,
    &OsConfigResource_RuleId_prop,
    &OsConfigResource_PayloadKey_prop,
    &OsConfigResource_ComponentName_prop,
    &OsConfigResource_InitObjectName_prop,
    &OsConfigResource_ProcedureObjectName_prop,
    &OsConfigResource_ProcedureObjectValue_prop,
    &OsConfigResource_ReportedObjectName_prop,
    &OsConfigResource_ReportedObjectValue_prop,
    &OsConfigResource_ExpectedObjectValue_prop,
    &OsConfigResource_DesiredObjectName_prop,
    &OsConfigResource_DesiredObjectValue_prop,
    &OsConfigResource_ReportedMpiResult_prop,
    &OsConfigResource_Reasons_prop,
};

static MI_CONST MI_Boolean OsConfigResource_GetTargetResource_Static_qual_value = 1;

static MI_CONST MI_Qualifier OsConfigResource_GetTargetResource_Static_qual =
{
    MI_T("Static"),
    MI_BOOLEAN,
    MI_FLAG_DISABLEOVERRIDE | MI_FLAG_TOSUBCLASS,
    &OsConfigResource_GetTargetResource_Static_qual_value
};

static MI_CONST MI_Char* OsConfigResource_GetTargetResource_Description_qual_value = MI_T("20");

static MI_CONST MI_Qualifier OsConfigResource_GetTargetResource_Description_qual =
{
    MI_T("Description"),
    MI_STRING,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS | MI_FLAG_TRANSLATABLE,
    &OsConfigResource_GetTargetResource_Description_qual_value
};

static MI_Qualifier MI_CONST* MI_CONST OsConfigResource_GetTargetResource_quals[] =
{
    &OsConfigResource_GetTargetResource_Static_qual,
    &OsConfigResource_GetTargetResource_Description_qual,
};

static MI_CONST MI_Boolean OsConfigResource_GetTargetResource_InputResource_In_qual_value = 1;

static MI_CONST MI_Qualifier OsConfigResource_GetTargetResource_InputResource_In_qual =
{
    MI_T("In"),
    MI_BOOLEAN,
    MI_FLAG_DISABLEOVERRIDE | MI_FLAG_TOSUBCLASS,
    &OsConfigResource_GetTargetResource_InputResource_In_qual_value
};

static MI_CONST MI_Char* OsConfigResource_GetTargetResource_InputResource_EmbeddedInstance_qual_value = MI_T("OsConfigResource");

static MI_CONST MI_Qualifier OsConfigResource_GetTargetResource_InputResource_EmbeddedInstance_qual =
{
    MI_T("EmbeddedInstance"),
    MI_STRING,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS,
    &OsConfigResource_GetTargetResource_InputResource_EmbeddedInstance_qual_value
};

static MI_CONST MI_Char* OsConfigResource_GetTargetResource_InputResource_Description_qual_value = MI_T("21");

static MI_CONST MI_Qualifier OsConfigResource_GetTargetResource_InputResource_Description_qual =
{
    MI_T("Description"),
    MI_STRING,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS | MI_FLAG_TRANSLATABLE,
    &OsConfigResource_GetTargetResource_InputResource_Description_qual_value
};

static MI_Qualifier MI_CONST* MI_CONST OsConfigResource_GetTargetResource_InputResource_quals[] =
{
    &OsConfigResource_GetTargetResource_InputResource_In_qual,
    &OsConfigResource_GetTargetResource_InputResource_EmbeddedInstance_qual,
    &OsConfigResource_GetTargetResource_InputResource_Description_qual,
};

/* parameter OsConfigResource.GetTargetResource(): InputResource */
static MI_CONST MI_ParameterDecl OsConfigResource_GetTargetResource_InputResource_param =
{
    MI_FLAG_PARAMETER | MI_FLAG_IN, /* flags */
    0x0069650D, /* code */
    MI_T("InputResource"), /* name */
    OsConfigResource_GetTargetResource_InputResource_quals, /* qualifiers */
    MI_COUNT(OsConfigResource_GetTargetResource_InputResource_quals), /* numQualifiers */
    MI_INSTANCE, /* type */
    MI_T("OsConfigResource"), /* className */
    0, /* subscript */
    offsetof(OsConfigResource_GetTargetResource, InputResource), /* offset */
};

static MI_CONST MI_Boolean OsConfigResource_GetTargetResource_Flags_In_qual_value = 1;

static MI_CONST MI_Qualifier OsConfigResource_GetTargetResource_Flags_In_qual =
{
    MI_T("In"),
    MI_BOOLEAN,
    MI_FLAG_DISABLEOVERRIDE | MI_FLAG_TOSUBCLASS,
    &OsConfigResource_GetTargetResource_Flags_In_qual_value
};

static MI_CONST MI_Char* OsConfigResource_GetTargetResource_Flags_Description_qual_value = MI_T("22");

static MI_CONST MI_Qualifier OsConfigResource_GetTargetResource_Flags_Description_qual =
{
    MI_T("Description"),
    MI_STRING,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS | MI_FLAG_TRANSLATABLE,
    &OsConfigResource_GetTargetResource_Flags_Description_qual_value
};

static MI_Qualifier MI_CONST* MI_CONST OsConfigResource_GetTargetResource_Flags_quals[] =
{
    &OsConfigResource_GetTargetResource_Flags_In_qual,
    &OsConfigResource_GetTargetResource_Flags_Description_qual,
};

/* parameter OsConfigResource.GetTargetResource(): Flags */
static MI_CONST MI_ParameterDecl OsConfigResource_GetTargetResource_Flags_param =
{
    MI_FLAG_PARAMETER | MI_FLAG_IN, /* flags */
    0x00667305, /* code */
    MI_T("Flags"), /* name */
    OsConfigResource_GetTargetResource_Flags_quals, /* qualifiers */
    MI_COUNT(OsConfigResource_GetTargetResource_Flags_quals), /* numQualifiers */
    MI_UINT32, /* type */
    NULL, /* className */
    0, /* subscript */
    offsetof(OsConfigResource_GetTargetResource, Flags), /* offset */
};

static MI_CONST MI_Boolean OsConfigResource_GetTargetResource_OutputResource_Out_qual_value = 1;

static MI_CONST MI_Qualifier OsConfigResource_GetTargetResource_OutputResource_Out_qual =
{
    MI_T("Out"),
    MI_BOOLEAN,
    MI_FLAG_DISABLEOVERRIDE | MI_FLAG_TOSUBCLASS,
    &OsConfigResource_GetTargetResource_OutputResource_Out_qual_value
};

static MI_CONST MI_Char* OsConfigResource_GetTargetResource_OutputResource_EmbeddedInstance_qual_value = MI_T("OsConfigResource");

static MI_CONST MI_Qualifier OsConfigResource_GetTargetResource_OutputResource_EmbeddedInstance_qual =
{
    MI_T("EmbeddedInstance"),
    MI_STRING,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS,
    &OsConfigResource_GetTargetResource_OutputResource_EmbeddedInstance_qual_value
};

static MI_CONST MI_Char* OsConfigResource_GetTargetResource_OutputResource_Description_qual_value = MI_T("23");

static MI_CONST MI_Qualifier OsConfigResource_GetTargetResource_OutputResource_Description_qual =
{
    MI_T("Description"),
    MI_STRING,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS | MI_FLAG_TRANSLATABLE,
    &OsConfigResource_GetTargetResource_OutputResource_Description_qual_value
};

static MI_Qualifier MI_CONST* MI_CONST OsConfigResource_GetTargetResource_OutputResource_quals[] =
{
    &OsConfigResource_GetTargetResource_OutputResource_Out_qual,
    &OsConfigResource_GetTargetResource_OutputResource_EmbeddedInstance_qual,
    &OsConfigResource_GetTargetResource_OutputResource_Description_qual,
};

/* parameter OsConfigResource.GetTargetResource(): OutputResource */
static MI_CONST MI_ParameterDecl OsConfigResource_GetTargetResource_OutputResource_param =
{
    MI_FLAG_PARAMETER | MI_FLAG_OUT, /* flags */
    0x006F650E, /* code */
    MI_T("OutputResource"), /* name */
    OsConfigResource_GetTargetResource_OutputResource_quals, /* qualifiers */
    MI_COUNT(OsConfigResource_GetTargetResource_OutputResource_quals), /* numQualifiers */
    MI_INSTANCE, /* type */
    MI_T("OsConfigResource"), /* className */
    0, /* subscript */
    offsetof(OsConfigResource_GetTargetResource, OutputResource), /* offset */
};

static MI_CONST MI_Boolean OsConfigResource_GetTargetResource_MIReturn_Static_qual_value = 1;

static MI_CONST MI_Qualifier OsConfigResource_GetTargetResource_MIReturn_Static_qual =
{
    MI_T("Static"),
    MI_BOOLEAN,
    MI_FLAG_DISABLEOVERRIDE | MI_FLAG_TOSUBCLASS,
    &OsConfigResource_GetTargetResource_MIReturn_Static_qual_value
};

static MI_CONST MI_Char* OsConfigResource_GetTargetResource_MIReturn_Description_qual_value = MI_T("20");

static MI_CONST MI_Qualifier OsConfigResource_GetTargetResource_MIReturn_Description_qual =
{
    MI_T("Description"),
    MI_STRING,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS | MI_FLAG_TRANSLATABLE,
    &OsConfigResource_GetTargetResource_MIReturn_Description_qual_value
};

static MI_Qualifier MI_CONST* MI_CONST OsConfigResource_GetTargetResource_MIReturn_quals[] =
{
    &OsConfigResource_GetTargetResource_MIReturn_Static_qual,
    &OsConfigResource_GetTargetResource_MIReturn_Description_qual,
};

/* parameter OsConfigResource.GetTargetResource(): MIReturn */
static MI_CONST MI_ParameterDecl OsConfigResource_GetTargetResource_MIReturn_param =
{
    MI_FLAG_PARAMETER | MI_FLAG_OUT, /* flags */
    0x006D6E08, /* code */
    MI_T("MIReturn"), /* name */
    OsConfigResource_GetTargetResource_MIReturn_quals, /* qualifiers */
    MI_COUNT(OsConfigResource_GetTargetResource_MIReturn_quals), /* numQualifiers */
    MI_UINT32, /* type */
    NULL, /* className */
    0, /* subscript */
    offsetof(OsConfigResource_GetTargetResource, MIReturn), /* offset */
};

static MI_ParameterDecl MI_CONST* MI_CONST OsConfigResource_GetTargetResource_params[] =
{
    &OsConfigResource_GetTargetResource_MIReturn_param,
    &OsConfigResource_GetTargetResource_InputResource_param,
    &OsConfigResource_GetTargetResource_Flags_param,
    &OsConfigResource_GetTargetResource_OutputResource_param,
};

/* method OsConfigResource.GetTargetResource() */
MI_CONST MI_MethodDecl OsConfigResource_GetTargetResource_rtti =
{
    MI_FLAG_METHOD | MI_FLAG_STATIC, /* flags */
    0x00676511, /* code */
    MI_T("GetTargetResource"), /* name */
    OsConfigResource_GetTargetResource_quals, /* qualifiers */
    MI_COUNT(OsConfigResource_GetTargetResource_quals), /* numQualifiers */
    OsConfigResource_GetTargetResource_params, /* parameters */
    MI_COUNT(OsConfigResource_GetTargetResource_params), /* numParameters */
    sizeof(OsConfigResource_GetTargetResource), /* size */
    MI_UINT32, /* returnType */
    MI_T("OsConfigResource"), /* origin */
    MI_T("OsConfigResource"), /* propagator */
    &schemaDecl, /* schema */
    (MI_ProviderFT_Invoke)OsConfigResource_Invoke_GetTargetResource, /* method */
};

static MI_CONST MI_Boolean OsConfigResource_TestTargetResource_Static_qual_value = 1;

static MI_CONST MI_Qualifier OsConfigResource_TestTargetResource_Static_qual =
{
    MI_T("Static"),
    MI_BOOLEAN,
    MI_FLAG_DISABLEOVERRIDE | MI_FLAG_TOSUBCLASS,
    &OsConfigResource_TestTargetResource_Static_qual_value
};

static MI_CONST MI_Char* OsConfigResource_TestTargetResource_Description_qual_value = MI_T("24");

static MI_CONST MI_Qualifier OsConfigResource_TestTargetResource_Description_qual =
{
    MI_T("Description"),
    MI_STRING,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS | MI_FLAG_TRANSLATABLE,
    &OsConfigResource_TestTargetResource_Description_qual_value
};

static MI_Qualifier MI_CONST* MI_CONST OsConfigResource_TestTargetResource_quals[] =
{
    &OsConfigResource_TestTargetResource_Static_qual,
    &OsConfigResource_TestTargetResource_Description_qual,
};

static MI_CONST MI_Boolean OsConfigResource_TestTargetResource_InputResource_In_qual_value = 1;

static MI_CONST MI_Qualifier OsConfigResource_TestTargetResource_InputResource_In_qual =
{
    MI_T("In"),
    MI_BOOLEAN,
    MI_FLAG_DISABLEOVERRIDE | MI_FLAG_TOSUBCLASS,
    &OsConfigResource_TestTargetResource_InputResource_In_qual_value
};

static MI_CONST MI_Char* OsConfigResource_TestTargetResource_InputResource_EmbeddedInstance_qual_value = MI_T("OsConfigResource");

static MI_CONST MI_Qualifier OsConfigResource_TestTargetResource_InputResource_EmbeddedInstance_qual =
{
    MI_T("EmbeddedInstance"),
    MI_STRING,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS,
    &OsConfigResource_TestTargetResource_InputResource_EmbeddedInstance_qual_value
};

static MI_CONST MI_Char* OsConfigResource_TestTargetResource_InputResource_Description_qual_value = MI_T("21");

static MI_CONST MI_Qualifier OsConfigResource_TestTargetResource_InputResource_Description_qual =
{
    MI_T("Description"),
    MI_STRING,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS | MI_FLAG_TRANSLATABLE,
    &OsConfigResource_TestTargetResource_InputResource_Description_qual_value
};

static MI_Qualifier MI_CONST* MI_CONST OsConfigResource_TestTargetResource_InputResource_quals[] =
{
    &OsConfigResource_TestTargetResource_InputResource_In_qual,
    &OsConfigResource_TestTargetResource_InputResource_EmbeddedInstance_qual,
    &OsConfigResource_TestTargetResource_InputResource_Description_qual,
};

/* parameter OsConfigResource.TestTargetResource(): InputResource */
static MI_CONST MI_ParameterDecl OsConfigResource_TestTargetResource_InputResource_param =
{
    MI_FLAG_PARAMETER | MI_FLAG_IN, /* flags */
    0x0069650D, /* code */
    MI_T("InputResource"), /* name */
    OsConfigResource_TestTargetResource_InputResource_quals, /* qualifiers */
    MI_COUNT(OsConfigResource_TestTargetResource_InputResource_quals), /* numQualifiers */
    MI_INSTANCE, /* type */
    MI_T("OsConfigResource"), /* className */
    0, /* subscript */
    offsetof(OsConfigResource_TestTargetResource, InputResource), /* offset */
};

static MI_CONST MI_Boolean OsConfigResource_TestTargetResource_Flags_In_qual_value = 1;

static MI_CONST MI_Qualifier OsConfigResource_TestTargetResource_Flags_In_qual =
{
    MI_T("In"),
    MI_BOOLEAN,
    MI_FLAG_DISABLEOVERRIDE | MI_FLAG_TOSUBCLASS,
    &OsConfigResource_TestTargetResource_Flags_In_qual_value
};

static MI_CONST MI_Char* OsConfigResource_TestTargetResource_Flags_Description_qual_value = MI_T("22");

static MI_CONST MI_Qualifier OsConfigResource_TestTargetResource_Flags_Description_qual =
{
    MI_T("Description"),
    MI_STRING,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS | MI_FLAG_TRANSLATABLE,
    &OsConfigResource_TestTargetResource_Flags_Description_qual_value
};

static MI_Qualifier MI_CONST* MI_CONST OsConfigResource_TestTargetResource_Flags_quals[] =
{
    &OsConfigResource_TestTargetResource_Flags_In_qual,
    &OsConfigResource_TestTargetResource_Flags_Description_qual,
};

/* parameter OsConfigResource.TestTargetResource(): Flags */
static MI_CONST MI_ParameterDecl OsConfigResource_TestTargetResource_Flags_param =
{
    MI_FLAG_PARAMETER | MI_FLAG_IN, /* flags */
    0x00667305, /* code */
    MI_T("Flags"), /* name */
    OsConfigResource_TestTargetResource_Flags_quals, /* qualifiers */
    MI_COUNT(OsConfigResource_TestTargetResource_Flags_quals), /* numQualifiers */
    MI_UINT32, /* type */
    NULL, /* className */
    0, /* subscript */
    offsetof(OsConfigResource_TestTargetResource, Flags), /* offset */
};

static MI_CONST MI_Boolean OsConfigResource_TestTargetResource_Result_Out_qual_value = 1;

static MI_CONST MI_Qualifier OsConfigResource_TestTargetResource_Result_Out_qual =
{
    MI_T("Out"),
    MI_BOOLEAN,
    MI_FLAG_DISABLEOVERRIDE | MI_FLAG_TOSUBCLASS,
    &OsConfigResource_TestTargetResource_Result_Out_qual_value
};

static MI_CONST MI_Char* OsConfigResource_TestTargetResource_Result_Description_qual_value = MI_T("25");

static MI_CONST MI_Qualifier OsConfigResource_TestTargetResource_Result_Description_qual =
{
    MI_T("Description"),
    MI_STRING,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS | MI_FLAG_TRANSLATABLE,
    &OsConfigResource_TestTargetResource_Result_Description_qual_value
};

static MI_Qualifier MI_CONST* MI_CONST OsConfigResource_TestTargetResource_Result_quals[] =
{
    &OsConfigResource_TestTargetResource_Result_Out_qual,
    &OsConfigResource_TestTargetResource_Result_Description_qual,
};

/* parameter OsConfigResource.TestTargetResource(): Result */
static MI_CONST MI_ParameterDecl OsConfigResource_TestTargetResource_Result_param =
{
    MI_FLAG_PARAMETER | MI_FLAG_OUT, /* flags */
    0x00727406, /* code */
    MI_T("Result"), /* name */
    OsConfigResource_TestTargetResource_Result_quals, /* qualifiers */
    MI_COUNT(OsConfigResource_TestTargetResource_Result_quals), /* numQualifiers */
    MI_BOOLEAN, /* type */
    NULL, /* className */
    0, /* subscript */
    offsetof(OsConfigResource_TestTargetResource, Result), /* offset */
};

static MI_CONST MI_Boolean OsConfigResource_TestTargetResource_ProviderContext_Out_qual_value = 1;

static MI_CONST MI_Qualifier OsConfigResource_TestTargetResource_ProviderContext_Out_qual =
{
    MI_T("Out"),
    MI_BOOLEAN,
    MI_FLAG_DISABLEOVERRIDE | MI_FLAG_TOSUBCLASS,
    &OsConfigResource_TestTargetResource_ProviderContext_Out_qual_value
};

static MI_CONST MI_Char* OsConfigResource_TestTargetResource_ProviderContext_Description_qual_value = MI_T("26");

static MI_CONST MI_Qualifier OsConfigResource_TestTargetResource_ProviderContext_Description_qual =
{
    MI_T("Description"),
    MI_STRING,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS | MI_FLAG_TRANSLATABLE,
    &OsConfigResource_TestTargetResource_ProviderContext_Description_qual_value
};

static MI_Qualifier MI_CONST* MI_CONST OsConfigResource_TestTargetResource_ProviderContext_quals[] =
{
    &OsConfigResource_TestTargetResource_ProviderContext_Out_qual,
    &OsConfigResource_TestTargetResource_ProviderContext_Description_qual,
};

/* parameter OsConfigResource.TestTargetResource(): ProviderContext */
static MI_CONST MI_ParameterDecl OsConfigResource_TestTargetResource_ProviderContext_param =
{
    MI_FLAG_PARAMETER | MI_FLAG_OUT, /* flags */
    0x0070740F, /* code */
    MI_T("ProviderContext"), /* name */
    OsConfigResource_TestTargetResource_ProviderContext_quals, /* qualifiers */
    MI_COUNT(OsConfigResource_TestTargetResource_ProviderContext_quals), /* numQualifiers */
    MI_UINT64, /* type */
    NULL, /* className */
    0, /* subscript */
    offsetof(OsConfigResource_TestTargetResource, ProviderContext), /* offset */
};

static MI_CONST MI_Boolean OsConfigResource_TestTargetResource_MIReturn_Static_qual_value = 1;

static MI_CONST MI_Qualifier OsConfigResource_TestTargetResource_MIReturn_Static_qual =
{
    MI_T("Static"),
    MI_BOOLEAN,
    MI_FLAG_DISABLEOVERRIDE | MI_FLAG_TOSUBCLASS,
    &OsConfigResource_TestTargetResource_MIReturn_Static_qual_value
};

static MI_CONST MI_Char* OsConfigResource_TestTargetResource_MIReturn_Description_qual_value = MI_T("24");

static MI_CONST MI_Qualifier OsConfigResource_TestTargetResource_MIReturn_Description_qual =
{
    MI_T("Description"),
    MI_STRING,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS | MI_FLAG_TRANSLATABLE,
    &OsConfigResource_TestTargetResource_MIReturn_Description_qual_value
};

static MI_Qualifier MI_CONST* MI_CONST OsConfigResource_TestTargetResource_MIReturn_quals[] =
{
    &OsConfigResource_TestTargetResource_MIReturn_Static_qual,
    &OsConfigResource_TestTargetResource_MIReturn_Description_qual,
};

/* parameter OsConfigResource.TestTargetResource(): MIReturn */
static MI_CONST MI_ParameterDecl OsConfigResource_TestTargetResource_MIReturn_param =
{
    MI_FLAG_PARAMETER | MI_FLAG_OUT, /* flags */
    0x006D6E08, /* code */
    MI_T("MIReturn"), /* name */
    OsConfigResource_TestTargetResource_MIReturn_quals, /* qualifiers */
    MI_COUNT(OsConfigResource_TestTargetResource_MIReturn_quals), /* numQualifiers */
    MI_UINT32, /* type */
    NULL, /* className */
    0, /* subscript */
    offsetof(OsConfigResource_TestTargetResource, MIReturn), /* offset */
};

static MI_ParameterDecl MI_CONST* MI_CONST OsConfigResource_TestTargetResource_params[] =
{
    &OsConfigResource_TestTargetResource_MIReturn_param,
    &OsConfigResource_TestTargetResource_InputResource_param,
    &OsConfigResource_TestTargetResource_Flags_param,
    &OsConfigResource_TestTargetResource_Result_param,
    &OsConfigResource_TestTargetResource_ProviderContext_param,
};

/* method OsConfigResource.TestTargetResource() */
MI_CONST MI_MethodDecl OsConfigResource_TestTargetResource_rtti =
{
    MI_FLAG_METHOD | MI_FLAG_STATIC, /* flags */
    0x00746512, /* code */
    MI_T("TestTargetResource"), /* name */
    OsConfigResource_TestTargetResource_quals, /* qualifiers */
    MI_COUNT(OsConfigResource_TestTargetResource_quals), /* numQualifiers */
    OsConfigResource_TestTargetResource_params, /* parameters */
    MI_COUNT(OsConfigResource_TestTargetResource_params), /* numParameters */
    sizeof(OsConfigResource_TestTargetResource), /* size */
    MI_UINT32, /* returnType */
    MI_T("OsConfigResource"), /* origin */
    MI_T("OsConfigResource"), /* propagator */
    &schemaDecl, /* schema */
    (MI_ProviderFT_Invoke)OsConfigResource_Invoke_TestTargetResource, /* method */
};

static MI_CONST MI_Boolean OsConfigResource_SetTargetResource_Static_qual_value = 1;

static MI_CONST MI_Qualifier OsConfigResource_SetTargetResource_Static_qual =
{
    MI_T("Static"),
    MI_BOOLEAN,
    MI_FLAG_DISABLEOVERRIDE | MI_FLAG_TOSUBCLASS,
    &OsConfigResource_SetTargetResource_Static_qual_value
};

static MI_CONST MI_Char* OsConfigResource_SetTargetResource_Description_qual_value = MI_T("27");

static MI_CONST MI_Qualifier OsConfigResource_SetTargetResource_Description_qual =
{
    MI_T("Description"),
    MI_STRING,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS | MI_FLAG_TRANSLATABLE,
    &OsConfigResource_SetTargetResource_Description_qual_value
};

static MI_Qualifier MI_CONST* MI_CONST OsConfigResource_SetTargetResource_quals[] =
{
    &OsConfigResource_SetTargetResource_Static_qual,
    &OsConfigResource_SetTargetResource_Description_qual,
};

static MI_CONST MI_Boolean OsConfigResource_SetTargetResource_InputResource_In_qual_value = 1;

static MI_CONST MI_Qualifier OsConfigResource_SetTargetResource_InputResource_In_qual =
{
    MI_T("In"),
    MI_BOOLEAN,
    MI_FLAG_DISABLEOVERRIDE | MI_FLAG_TOSUBCLASS,
    &OsConfigResource_SetTargetResource_InputResource_In_qual_value
};

static MI_CONST MI_Char* OsConfigResource_SetTargetResource_InputResource_EmbeddedInstance_qual_value = MI_T("OsConfigResource");

static MI_CONST MI_Qualifier OsConfigResource_SetTargetResource_InputResource_EmbeddedInstance_qual =
{
    MI_T("EmbeddedInstance"),
    MI_STRING,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS,
    &OsConfigResource_SetTargetResource_InputResource_EmbeddedInstance_qual_value
};

static MI_CONST MI_Char* OsConfigResource_SetTargetResource_InputResource_Description_qual_value = MI_T("21");

static MI_CONST MI_Qualifier OsConfigResource_SetTargetResource_InputResource_Description_qual =
{
    MI_T("Description"),
    MI_STRING,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS | MI_FLAG_TRANSLATABLE,
    &OsConfigResource_SetTargetResource_InputResource_Description_qual_value
};

static MI_Qualifier MI_CONST* MI_CONST OsConfigResource_SetTargetResource_InputResource_quals[] =
{
    &OsConfigResource_SetTargetResource_InputResource_In_qual,
    &OsConfigResource_SetTargetResource_InputResource_EmbeddedInstance_qual,
    &OsConfigResource_SetTargetResource_InputResource_Description_qual,
};

/* parameter OsConfigResource.SetTargetResource(): InputResource */
static MI_CONST MI_ParameterDecl OsConfigResource_SetTargetResource_InputResource_param =
{
    MI_FLAG_PARAMETER | MI_FLAG_IN, /* flags */
    0x0069650D, /* code */
    MI_T("InputResource"), /* name */
    OsConfigResource_SetTargetResource_InputResource_quals, /* qualifiers */
    MI_COUNT(OsConfigResource_SetTargetResource_InputResource_quals), /* numQualifiers */
    MI_INSTANCE, /* type */
    MI_T("OsConfigResource"), /* className */
    0, /* subscript */
    offsetof(OsConfigResource_SetTargetResource, InputResource), /* offset */
};

static MI_CONST MI_Boolean OsConfigResource_SetTargetResource_ProviderContext_In_qual_value = 1;

static MI_CONST MI_Qualifier OsConfigResource_SetTargetResource_ProviderContext_In_qual =
{
    MI_T("In"),
    MI_BOOLEAN,
    MI_FLAG_DISABLEOVERRIDE | MI_FLAG_TOSUBCLASS,
    &OsConfigResource_SetTargetResource_ProviderContext_In_qual_value
};

static MI_CONST MI_Char* OsConfigResource_SetTargetResource_ProviderContext_Description_qual_value = MI_T("26");

static MI_CONST MI_Qualifier OsConfigResource_SetTargetResource_ProviderContext_Description_qual =
{
    MI_T("Description"),
    MI_STRING,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS | MI_FLAG_TRANSLATABLE,
    &OsConfigResource_SetTargetResource_ProviderContext_Description_qual_value
};

static MI_Qualifier MI_CONST* MI_CONST OsConfigResource_SetTargetResource_ProviderContext_quals[] =
{
    &OsConfigResource_SetTargetResource_ProviderContext_In_qual,
    &OsConfigResource_SetTargetResource_ProviderContext_Description_qual,
};

/* parameter OsConfigResource.SetTargetResource(): ProviderContext */
static MI_CONST MI_ParameterDecl OsConfigResource_SetTargetResource_ProviderContext_param =
{
    MI_FLAG_PARAMETER | MI_FLAG_IN, /* flags */
    0x0070740F, /* code */
    MI_T("ProviderContext"), /* name */
    OsConfigResource_SetTargetResource_ProviderContext_quals, /* qualifiers */
    MI_COUNT(OsConfigResource_SetTargetResource_ProviderContext_quals), /* numQualifiers */
    MI_UINT64, /* type */
    NULL, /* className */
    0, /* subscript */
    offsetof(OsConfigResource_SetTargetResource, ProviderContext), /* offset */
};

static MI_CONST MI_Boolean OsConfigResource_SetTargetResource_Flags_In_qual_value = 1;

static MI_CONST MI_Qualifier OsConfigResource_SetTargetResource_Flags_In_qual =
{
    MI_T("In"),
    MI_BOOLEAN,
    MI_FLAG_DISABLEOVERRIDE | MI_FLAG_TOSUBCLASS,
    &OsConfigResource_SetTargetResource_Flags_In_qual_value
};

static MI_CONST MI_Char* OsConfigResource_SetTargetResource_Flags_Description_qual_value = MI_T("22");

static MI_CONST MI_Qualifier OsConfigResource_SetTargetResource_Flags_Description_qual =
{
    MI_T("Description"),
    MI_STRING,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS | MI_FLAG_TRANSLATABLE,
    &OsConfigResource_SetTargetResource_Flags_Description_qual_value
};

static MI_Qualifier MI_CONST* MI_CONST OsConfigResource_SetTargetResource_Flags_quals[] =
{
    &OsConfigResource_SetTargetResource_Flags_In_qual,
    &OsConfigResource_SetTargetResource_Flags_Description_qual,
};

/* parameter OsConfigResource.SetTargetResource(): Flags */
static MI_CONST MI_ParameterDecl OsConfigResource_SetTargetResource_Flags_param =
{
    MI_FLAG_PARAMETER | MI_FLAG_IN, /* flags */
    0x00667305, /* code */
    MI_T("Flags"), /* name */
    OsConfigResource_SetTargetResource_Flags_quals, /* qualifiers */
    MI_COUNT(OsConfigResource_SetTargetResource_Flags_quals), /* numQualifiers */
    MI_UINT32, /* type */
    NULL, /* className */
    0, /* subscript */
    offsetof(OsConfigResource_SetTargetResource, Flags), /* offset */
};

static MI_CONST MI_Boolean OsConfigResource_SetTargetResource_MIReturn_Static_qual_value = 1;

static MI_CONST MI_Qualifier OsConfigResource_SetTargetResource_MIReturn_Static_qual =
{
    MI_T("Static"),
    MI_BOOLEAN,
    MI_FLAG_DISABLEOVERRIDE | MI_FLAG_TOSUBCLASS,
    &OsConfigResource_SetTargetResource_MIReturn_Static_qual_value
};

static MI_CONST MI_Char* OsConfigResource_SetTargetResource_MIReturn_Description_qual_value = MI_T("27");

static MI_CONST MI_Qualifier OsConfigResource_SetTargetResource_MIReturn_Description_qual =
{
    MI_T("Description"),
    MI_STRING,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS | MI_FLAG_TRANSLATABLE,
    &OsConfigResource_SetTargetResource_MIReturn_Description_qual_value
};

static MI_Qualifier MI_CONST* MI_CONST OsConfigResource_SetTargetResource_MIReturn_quals[] =
{
    &OsConfigResource_SetTargetResource_MIReturn_Static_qual,
    &OsConfigResource_SetTargetResource_MIReturn_Description_qual,
};

/* parameter OsConfigResource.SetTargetResource(): MIReturn */
static MI_CONST MI_ParameterDecl OsConfigResource_SetTargetResource_MIReturn_param =
{
    MI_FLAG_PARAMETER | MI_FLAG_OUT, /* flags */
    0x006D6E08, /* code */
    MI_T("MIReturn"), /* name */
    OsConfigResource_SetTargetResource_MIReturn_quals, /* qualifiers */
    MI_COUNT(OsConfigResource_SetTargetResource_MIReturn_quals), /* numQualifiers */
    MI_UINT32, /* type */
    NULL, /* className */
    0, /* subscript */
    offsetof(OsConfigResource_SetTargetResource, MIReturn), /* offset */
};

static MI_ParameterDecl MI_CONST* MI_CONST OsConfigResource_SetTargetResource_params[] =
{
    &OsConfigResource_SetTargetResource_MIReturn_param,
    &OsConfigResource_SetTargetResource_InputResource_param,
    &OsConfigResource_SetTargetResource_ProviderContext_param,
    &OsConfigResource_SetTargetResource_Flags_param,
};

/* method OsConfigResource.SetTargetResource() */
MI_CONST MI_MethodDecl OsConfigResource_SetTargetResource_rtti =
{
    MI_FLAG_METHOD | MI_FLAG_STATIC, /* flags */
    0x00736511, /* code */
    MI_T("SetTargetResource"), /* name */
    OsConfigResource_SetTargetResource_quals, /* qualifiers */
    MI_COUNT(OsConfigResource_SetTargetResource_quals), /* numQualifiers */
    OsConfigResource_SetTargetResource_params, /* parameters */
    MI_COUNT(OsConfigResource_SetTargetResource_params), /* numParameters */
    sizeof(OsConfigResource_SetTargetResource), /* size */
    MI_UINT32, /* returnType */
    MI_T("OsConfigResource"), /* origin */
    MI_T("OsConfigResource"), /* propagator */
    &schemaDecl, /* schema */
    (MI_ProviderFT_Invoke)OsConfigResource_Invoke_SetTargetResource, /* method */
};

static MI_MethodDecl MI_CONST* MI_CONST OsConfigResource_meths[] =
{
    &OsConfigResource_GetTargetResource_rtti,
    &OsConfigResource_TestTargetResource_rtti,
    &OsConfigResource_SetTargetResource_rtti,
};

static MI_CONST MI_ProviderFT OsConfigResource_funcs =
{
    (MI_ProviderFT_Load)OsConfigResource_Load,
    (MI_ProviderFT_Unload)OsConfigResource_Unload,
    (MI_ProviderFT_GetInstance)OsConfigResource_GetInstance,
    (MI_ProviderFT_EnumerateInstances)OsConfigResource_EnumerateInstances,
    (MI_ProviderFT_CreateInstance)OsConfigResource_CreateInstance,
    (MI_ProviderFT_ModifyInstance)OsConfigResource_ModifyInstance,
    (MI_ProviderFT_DeleteInstance)OsConfigResource_DeleteInstance,
    (MI_ProviderFT_AssociatorInstances)NULL,
    (MI_ProviderFT_ReferenceInstances)NULL,
    (MI_ProviderFT_EnableIndications)NULL,
    (MI_ProviderFT_DisableIndications)NULL,
    (MI_ProviderFT_Subscribe)NULL,
    (MI_ProviderFT_Unsubscribe)NULL,
    (MI_ProviderFT_Invoke)NULL,
};

static MI_CONST MI_Char* OsConfigResource_Description_qual_value = MI_T("11");

static MI_CONST MI_Qualifier OsConfigResource_Description_qual =
{
    MI_T("Description"),
    MI_STRING,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_TOSUBCLASS | MI_FLAG_TRANSLATABLE,
    &OsConfigResource_Description_qual_value
};

static MI_CONST MI_Char* OsConfigResource_ClassVersion_qual_value = MI_T("4.0.0");

static MI_CONST MI_Qualifier OsConfigResource_ClassVersion_qual =
{
    MI_T("ClassVersion"),
    MI_STRING,
    MI_FLAG_ENABLEOVERRIDE | MI_FLAG_RESTRICTED,
    &OsConfigResource_ClassVersion_qual_value
};

static MI_Qualifier MI_CONST* MI_CONST OsConfigResource_quals[] =
{
    &OsConfigResource_Description_qual,
    &OsConfigResource_ClassVersion_qual,
};

/* class OsConfigResource */
MI_CONST MI_ClassDecl OsConfigResource_rtti =
{
    MI_FLAG_CLASS, /* flags */
    0x006F6510, /* code */
    MI_T("OsConfigResource"), /* name */
    OsConfigResource_quals, /* qualifiers */
    MI_COUNT(OsConfigResource_quals), /* numQualifiers */
    OsConfigResource_props, /* properties */
    MI_COUNT(OsConfigResource_props), /* numProperties */
    sizeof(OsConfigResource), /* size */
    MI_T("OMI_BaseResource"), /* superClass */
    &OMI_BaseResource_rtti, /* superClassDecl */
    OsConfigResource_meths, /* methods */
    MI_COUNT(OsConfigResource_meths), /* numMethods */
    &schemaDecl, /* schema */
    &OsConfigResource_funcs, /* functions */
    NULL /* owningClass */
};

/*
**==============================================================================
**
** __mi_server
**
**==============================================================================
*/

MI_Server* __mi_server;
/*
**==============================================================================
**
** Schema
**
**==============================================================================
*/

static MI_ClassDecl MI_CONST* MI_CONST classes[] =
{
    &MSFT_Credential_rtti,
    &OMI_BaseResource_rtti,
    &OsConfigResource_rtti,
    &ReasonClass_rtti,
};

MI_SchemaDecl schemaDecl =
{
    qualifierDecls, /* qualifierDecls */
    MI_COUNT(qualifierDecls), /* numQualifierDecls */
    classes, /* classDecls */
    MI_COUNT(classes), /* classDecls */
};

/*
**==============================================================================
**
** MI_Server Methods
**
**==============================================================================
*/

MI_Result MI_CALL MI_Server_GetVersion(
    MI_Uint32* version){
    return __mi_server->serverFT->GetVersion(version);
}

MI_Result MI_CALL MI_Server_GetSystemName(
    const MI_Char** systemName)
{
    return __mi_server->serverFT->GetSystemName(systemName);
}
