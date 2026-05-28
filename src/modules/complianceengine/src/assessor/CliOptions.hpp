// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCE_ENGINE_ASSESSOR_CLI_OPTIONS_HPP
#define COMPLIANCE_ENGINE_ASSESSOR_CLI_OPTIONS_HPP

#include <Optional.h>
#include <Result.h>
#include <iosfwd>
#include <string>

namespace ComplianceEngine
{
namespace Assessor
{

enum class Command
{
    Help,
    Version,
    Audit,
    Remediate
};

enum class Format
{
    NestedList,
    CompactList,
    Json,
    Debug
};

struct Options
{
    bool verbose = false;
    bool debug = false;
    Optional<std::string> logFile;
    Optional<Format> format;
    Command command = Command::Help;
    std::string input;
    Optional<std::string> section;
};

// Parses command-line arguments for the assessor.
//
// Safe to call multiple times: resets getopt's global state on entry. Suppresses
// getopt's own diagnostics (`opterr = 0`) so the only error output comes from
// the returned Error. Returns an Error on any malformed input, unknown option,
// unknown command, missing command, duplicate flag, or empty optarg.
Result<Options> ParseCommandLine(int argc, char* argv[]);

// Prints usage text. Writes to `out`. `programName` may be null or empty;
// a sensible default is used in that case.
void PrintHelp(std::ostream& out, const char* programName);

} // namespace Assessor
} // namespace ComplianceEngine

#endif // COMPLIANCE_ENGINE_ASSESSOR_CLI_OPTIONS_HPP
