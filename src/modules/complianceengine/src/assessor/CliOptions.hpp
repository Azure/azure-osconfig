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
    bool continueOnError = false;
    Optional<std::string> logFile;
    Optional<Format> format;
    Command command = Command::Help;
    std::string input;
    Optional<std::string> section;
};

void PrintHelp(const std::string& programName);

Result<Options> ParseCommandLine(int argc, char* argv[]);

} // namespace Assessor
} // namespace ComplianceEngine

#endif // COMPLIANCE_ENGINE_ASSESSOR_CLI_OPTIONS_HPP
