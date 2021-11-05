// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <string>
#include "ConfigFileUtils.h"

class BaseUtils
{
public:
    virtual ~BaseUtils() {};
    virtual bool SetValueString(const std::string& name, const std::string& value) = 0;
    virtual char* GetValueString(const std::string& name) = 0;
    virtual bool SetValueInteger(const std::string& name, const int value) = 0;
    virtual int GetValueInteger(const std::string& name) = 0;
};

class BaseUtilsFactory
{
public:
    static BaseUtils* CreateInstance(const char* path, ConfigFileFormat format);
};