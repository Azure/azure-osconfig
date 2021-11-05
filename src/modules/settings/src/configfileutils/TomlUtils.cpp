// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <iostream>
#include <fstream>
#include <string.h>
#include "TomlUtils.h"
#include "ConfigFileUtils.h"
#include "ScopeGuard.h"

bool TomlUtils::SetValueString(const std::string& name, const std::string& value)
{
    if (!DeserializeFromFile())
    {
        return false;
    }

    if (!(m_tomlDocumentObject.has(name)))
    {
        printf("TomlUtils::SetValueString: %s does not exist\n", name.c_str());
        return false;
    }

    toml::Value valueToSet = value;
    std::string settingName = name;
    m_tomlDocumentObject.set(settingName, valueToSet);
    return SerializeToFile();
}

char* TomlUtils::GetValueString(const std::string& name)
{
    if (!DeserializeFromFile())
    {
        return nullptr;
    }

    const toml::Value* valueToml = m_tomlDocumentObject.find(name);
    if (nullptr != valueToml)
    {
        std::string valueString = valueToml->as<std::string>();
        char* value = new(std::nothrow) char[valueString.length() + 1];
        if (value == nullptr)
        {
            printf("TomlUtils::GetValueString: Allocation failed, issue with memory.\n");
            return nullptr;
        }
        strcpy(value, valueString.c_str());
        return value;
    }

    return nullptr;
}

bool TomlUtils::SetValueInteger(const std::string& name, const int value)
{
    if (!DeserializeFromFile())
    {
        return false;
    }

    if (!(m_tomlDocumentObject.has(name)))
    {
        printf("TomlUtils::SetValueInteger: %s does not exist\n", name.c_str());
        return false;
    }

    toml::Value valueToSet = value;
    std::string settingName = name;
    m_tomlDocumentObject.set(settingName, valueToSet);
    return SerializeToFile();
}

int TomlUtils::GetValueInteger(const std::string& name)
{
    if (!DeserializeFromFile())
    {
        return READ_CONFIG_FAILURE;
    }

    return m_tomlDocumentObject.find(name)->as<int>();
}

bool TomlUtils::SerializeToFile()
{
    std::ofstream ofs;
    ofs.open(m_path);
    if (ofs.fail())
    {
        printf("TomlUtils::SerializeToFile: iostream operation failed\n");
        return false;
    }

    ScopeGuard sg{[&]()
    {
        ofs.close();
    }};

    ofs << m_tomlDocumentObject;
    return true;
}

bool TomlUtils::DeserializeFromFile()
{
    std::ifstream ifs(m_path);
    ScopeGuard sg{[&]()
    {
        ifs.close();
    }};

    toml::ParseResult parseResult = toml::parse(ifs);
    if (!parseResult.valid())
    {
        return false;
    }

    m_tomlDocumentObject = parseResult.value;
    return true;
}