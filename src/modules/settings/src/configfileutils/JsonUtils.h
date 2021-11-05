// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "rapidjson/document.h"
#include "BaseUtils.h"

class JsonUtils : public BaseUtils
{
public:
    JsonUtils(const char* path) : m_path(path) {};
    ~JsonUtils() {};

    bool SetValueString(const std::string& name, const std::string& value) override;
    char* GetValueString(const std::string& name) override;
    bool SetValueInteger(const std::string& name, const int value) override;
    int GetValueInteger(const std::string& name) override;

private:
    bool SerializeToFile();
    bool DeserializeFromFile();

    template <typename T>
    bool SetValueInternal(const std::string& path, T value);

    const char* m_path;
    rapidjson::Document m_jsonDocumentObject;
};