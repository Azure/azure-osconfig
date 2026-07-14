#ifndef COMPLIANCE_ENGINE_ASSESSOR_CLI_OPTIONS_HPP
#define COMPLIANCE_ENGINE_ASSESSOR_CLI_OPTIONS_HPP

#include <Optional.h>
#include <Result.h>
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
    Remediate,
    Format
};

// Presentation formats produced by the `format` subcommand. `audit` / `remediate`
// no longer select a format: they always emit the canonical JSON. The list/debug
// renderers are retained for a later port under `format`; only Junit is wired
// today.
enum class Format
{
    NestedList,
    CompactList,
    Json,
    Debug,
    Junit
};

struct Options
{
    bool verbose = false;
    bool debug = false;
    bool continueOnError = false;
    Optional<std::string> logFile;
    Optional<Format> format;
    Command command = Command::Help;
    std::string input;
    Optional<std::string> section;
    // `format` only: the JUnit <testsuite name>. The assessor does not know which
    // benchmark package it came from, so the caller supplies this.
    Optional<std::string> suiteName;
};

void PrintHelp(const std::string& programName);

Result<Options> ParseCommandLine(int argc, char* argv[]);

} // namespace Assessor
} // namespace ComplianceEngine

#endif // COMPLIANCE_ENGINE_ASSESSOR_CLI_OPTIONS_HPP
