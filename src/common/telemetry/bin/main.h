// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <Logging.h>
#include <chrono>
#include <string>

struct CommandLineArgs
{
    bool verbose;
    std::string filepath;
    std::chrono::seconds teardown_time;
};

void PrintUsage(const char* program_name);
bool ParseCommandLineArgs(int argc, char* argv[], CommandLineArgs& args, OsConfigLogHandle log);
