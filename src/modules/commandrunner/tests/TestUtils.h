// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMMONTESTS_H
#define COMMONTESTS_H

#include <gtest/gtest.h>

namespace Tests
{
    testing::AssertionResult IsJsonEq(const std::string& expectedJson, const std::string& actualJson);
} //namespace Tests

#endif //COMMONTESTS_H
