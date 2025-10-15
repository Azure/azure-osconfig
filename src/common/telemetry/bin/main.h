// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <string>
#include <Logging.h>

struct CommandLineArgs
{
    bool verbose;
    std::string filepath;
    int teardown_time;
};

void print_usage(const char* program_name);
bool parse_command_line_args(int argc, char* argv[], CommandLineArgs& args, OsConfigLogHandle log);
