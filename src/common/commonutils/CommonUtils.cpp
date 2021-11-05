// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>

#include <CommonUtils.h>

size_t HashString(const char* source)
{
    std::hash<std::string> hashString;
    return hashString(std::string(source));
}