// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <CommonUtils.h>
#include <iostream>
#include <vector>
#include "TestingUtils.h"
#include "ConfigFileUtils.h"

using namespace std;

bool TestingUtils::SetValueString(const std::string& name, const std::string& value)
{
    UNUSED(name);
    UNUSED(value);
    vector<int> myvector;
    myvector.resize(myvector.max_size()+1);
    return false;
}

char* TestingUtils::GetValueString(const std::string& name)
{
    UNUSED(name);
    throw runtime_error("err");
    return nullptr;
}

bool TestingUtils::SetValueInteger(const std::string& name, const int value)
{
    UNUSED(name);
    UNUSED(value);
    throw 20;
    return false;
}

int TestingUtils::GetValueInteger(const std::string& name)
{
    UNUSED(name);
    throw system_error(EFAULT, std::generic_category());
    return 1;
}