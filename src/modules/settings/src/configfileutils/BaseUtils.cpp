// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <iostream>
#include <string>
#include <new>
#include "JsonUtils.h"
#include "TomlUtils.h"

#ifdef TEST_CODE
#include "TestingUtils.h"
#endif

#include "BaseUtils.h"
#include "ConfigFileUtils.h"

BaseUtils* BaseUtilsFactory::CreateInstance(const char* path, ConfigFileFormat format)
{
    if (nullptr == path)
    {
        return nullptr;
    }

    std::ifstream ifstream(path);

    if (!ifstream.is_open())
    {
        printf("BaseUtilsFactory::CreateInstance: %s does not exist\n", path);
        return nullptr;
    }

    switch (format)
    {
        case ConfigFileFormatJson:
            return new(std::nothrow) JsonUtils(path);

        case ConfigFileFormatToml:
            return new(std::nothrow) TomlUtils(path);

#ifdef TEST_CODE
        case Testing:
            return new TestingUtils();
#endif
        default :
            printf("BaseUtilsFactory::CreateInstance: Invalid argument\n");
            return nullptr;
    }
}