// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef TESTRECIPEPARSER_H
#define TESTRECIPEPARSER_H

#include "MimParser.h"

struct TestRecipeMetadata
{
    std::string m_modulePath;
    std::string m_mimPath;
    std::string m_testRecipesPath;
};
struct TestRecipe
{
    std::string m_componentName;
    std::string m_objectName;
    bool m_desired;
    std::string m_payload;
    size_t m_payloadSizeBytes;
    int m_expectedResult;
    int m_waitSeconds;
    TestRecipeMetadata m_metadata;
    pMimObjects m_mimObjects;
};

typedef std::shared_ptr<std::vector<TestRecipe>> TestRecipes;

class TestRecipeParser
{
public:
    static TestRecipes ParseTestRecipe(std::string path);
};

#endif // TESTRECIPEPARSER_H