// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "BaseUtils.h"

class TestingUtils : public BaseUtils
{
public:
    TestingUtils() {};
    ~TestingUtils() {};

    bool SetValueString(const std::string& name, const std::string& value) override;
    char* GetValueString(const std::string& name) override;
    bool SetValueInteger(const std::string& name, const int value) override;
    int GetValueInteger(const std::string& name) override;
};