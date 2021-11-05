// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <iostream>
#include <fstream>
#include <sstream>
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/pointer.h"
#include "rapidjson/error/en.h"
#include "JsonUtils.h"
#include "ConfigFileUtils.h"
#include "ScopeGuard.h"

bool JsonUtils::SetValueString(const std::string& name, const std::string& value)
{
    if (!DeserializeFromFile())
    {
        return false;
    }

    if (!SetValueInternal(name, value.c_str()))
    {
        printf("JsonUtils::SetValueString: Could not set value %s at %s", value.c_str(), name.c_str());
        return false;
    }

    return SerializeToFile();
}

char* JsonUtils::GetValueString(const std::string& name)
{
    if (!DeserializeFromFile())
    {
        return nullptr;
    }

    rapidjson::Pointer p(name.c_str());
    if (!p.IsValid())
    {
        printf("JsonUtils::GetValueString: invalid JSON pointer %s\n", name.c_str());
        return nullptr;
    }

    rapidjson::Value* jsonValue = rapidjson::GetValueByPointer(m_jsonDocumentObject, p);
    if (jsonValue == nullptr)
    {
        printf("JsonUtils::GetValueString: %s does not exist\n", name.c_str());
        return nullptr;
    }

    if (!(jsonValue->IsString()))
    {
        printf("JsonUtils::GetValueString: Value at %s is not of type string\n", name.c_str());
        return nullptr;
    }

    std::string valueString = jsonValue->GetString();
    char* value = new(std::nothrow) char[valueString.length() + 1];
    if (value == nullptr)
    {
        printf("JsonUtils::GetValueString: Allocation failed, issue with memory.\n");
        return nullptr;
    }
    strcpy(value, valueString.c_str());
    return value;
}

bool JsonUtils::SetValueInteger(const std::string& name, const int value)
{
    if (!DeserializeFromFile())
    {
        return false;
    }

    if (!SetValueInternal(name, value))
    {
        printf("JsonUtils::SetValueInteger: Could not set value %d at %s", value, name.c_str());
        return false;
    }

    return SerializeToFile();
}

int JsonUtils::GetValueInteger(const std::string& name)
{
    if (!DeserializeFromFile())
    {
        return READ_CONFIG_FAILURE;
    }

    rapidjson::Pointer p(name.c_str());
    if (!p.IsValid())
    {
        printf("JsonUtils::GetValueInteger: invalid JSON pointer %s\n", name.c_str());
        return READ_CONFIG_FAILURE;
    }

    rapidjson::Value* jsonValue = rapidjson::GetValueByPointer(m_jsonDocumentObject, p);
    if (jsonValue == nullptr)
    {
        printf("JsonUtils::GetValueInteger: %s does not exist\n", name.c_str());
        return READ_CONFIG_FAILURE;
    }

    if (!(jsonValue->IsInt()))
    {
        printf("JsonUtils::GetValueInteger: Value at %s is not of type int\n", name.c_str());
        return READ_CONFIG_FAILURE;
    }

    return jsonValue->GetInt();
}

template <typename T>
bool JsonUtils::SetValueInternal(const std::string& name, T value)
{
    rapidjson::Pointer p(name.c_str());
    if (!p.IsValid())
    {
        printf("JsonUtils::SetValueInternal: invalid JSON pointer %s\n", name.c_str());
        return false;
    }

    rapidjson::SetValueByPointer(m_jsonDocumentObject, p, value);
    return true;
}

bool JsonUtils::SerializeToFile()
{
    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);

    m_jsonDocumentObject.Accept(writer);
    const std::string& buffContents = buffer.GetString();
    std::ofstream ofs(m_path);
    if (ofs.fail())
    {
        printf("JsonUtils::SerializeToFile: iostream operation failed\n");
        return false;
    }

    ScopeGuard sg{[&]()
    {
        ofs.close();
    }};

    ofs << buffContents;
    return true;
}

bool JsonUtils::DeserializeFromFile()
{
    std::stringstream buffer;
    std::ifstream ifs(m_path);
    if (ifs.fail())
    {
        printf("JsonUtils::DeserializeFromFile: iostream operation failed\n");
        return false;
    }

    ScopeGuard sg{[&]()
    {
        ifs.close();
    }};

    buffer << ifs.rdbuf();

    const std::string& buffContents = buffer.str();
    std::string validJson(buffContents);
    m_jsonDocumentObject.Parse(validJson.c_str());

    if (m_jsonDocumentObject.HasParseError())
    {
        printf("JsonUtils::DeserializeFromFile: Parse operation failed with error: %s (offset: %u)\n",
            GetParseError_En(m_jsonDocumentObject.GetParseError()),
            (unsigned)m_jsonDocumentObject.GetErrorOffset());
        return false;
    }

    return true;
}