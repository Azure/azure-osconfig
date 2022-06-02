// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef MIMPARSER_H
#define MIMPARSER_H

#include "Common.h"

struct MimField
{
    std::string name;
    std::string type;
    std::string subType1;    // For arrays: interger or string subtypes (subType1)
    std::string subType2;    // For maps: key (subType1), value (subType2)
    std::shared_ptr<std::vector<std::string>> allowedValues;
};

struct MimObject
{
    std::string m_name;
    std::string m_type;
    bool m_desired;
    std::shared_ptr<std::map<std::string, MimField>> m_fields;
};

typedef std::map<std::string, std::shared_ptr<std::map<std::string, MimObject>>> MimObjects;
typedef std::shared_ptr<MimObjects> pMimObjects;

class MimParser
{
private:
    static void ParseMimField(JSON_Object* jsonField, MimObject& mimObject);

public:
    static pMimObjects ParseMim(std::string path);
};

#endif // MIMPARSER_H