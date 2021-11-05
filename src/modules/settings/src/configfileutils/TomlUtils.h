// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "tinytoml/toml.h"
#include "BaseUtils.h"

class TomlUtils : public BaseUtils
{
public:
    TomlUtils(const char* path) : m_path(path) {};
    ~TomlUtils() {};

    bool SetValueString(const std::string& name, const std::string& value) override;
    char* GetValueString(const std::string& name) override;
    bool SetValueInteger(const std::string& name, const int value) override;
    int GetValueInteger(const std::string& name) override;

private:
    bool SerializeToFile();
    bool DeserializeFromFile();

    const char* m_path;
    toml::Value m_tomlDocumentObject;
};