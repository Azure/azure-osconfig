// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMMONTESTS_H
#define COMMONTESTS_H

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <rapidjson/document.h>

namespace Tests
{
    testing::AssertionResult JSON_EQ(std::string const &leftString, std::string const &rightString);
} //namespace Tests

#endif //COMMONTESTS_H