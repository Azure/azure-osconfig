// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef RECIPEINVOKER_H
#define RECIPEINVOKER_H

static const std::string g_defaultClient = "RecipeInvoker";

static const std::string g_modulePath = "ModulePath";
static const std::string g_mimPath = "MimPath";
static const std::string g_testRecipesPath = "TestRecipesPath";

class RecipeFixture : public ::testing::Test {};

class RecipeInvoker : public RecipeFixture
{
public:
    explicit RecipeInvoker(const TestRecipe &recipe) : m_recipe(recipe) {}
    void TestBody() override;

private:
    TestRecipe m_recipe;
};

class BasicModuleTester : public RecipeFixture
{
public:
    explicit BasicModuleTester(std::shared_ptr<ManagementModule> module) : m_module(module) {}
    void TestBody() override;

private:
    std::shared_ptr<ManagementModule> m_module;
};

#endif // RECIPEINVOKER_H