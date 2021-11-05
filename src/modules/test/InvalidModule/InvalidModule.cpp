// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <thread>
#include <chrono>

class InvalidModule
{
public:
    void testMethod1()
    {
        std::this_thread::sleep_for(std::chrono::seconds(30));
    }
};