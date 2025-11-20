#include <CommonContext.h>
#include <Logging.h>
#include <LuaEvaluator.h>
#include <Optional.h>
#include <Telemetry.h>
#include <cassert>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>
#include <version.h>

using ComplianceEngine::Action;
using ComplianceEngine::CommonContext;
using ComplianceEngine::Error;
using ComplianceEngine::IndicatorsTree;
using ComplianceEngine::LuaEvaluator;
using ComplianceEngine::NestedListFormatter;
using ComplianceEngine::Optional;
using ComplianceEngine::PayloadFormatter;
using ComplianceEngine::Result;
using ComplianceEngine::Status;
using std::ifstream;
using std::istream;
using std::string;

namespace
{
enum class Command
{
    Help,
    Version,
    Evaluate,
};

struct Options
{
    bool verbose = false;
    bool debug = false;
    Optional<string> logFile;
    Command command = Command::Help;
    Optional<std::string> input;
};

void PrintHelp(const std::string& programName)
{
    std::cout << "Usage: " + programName + "\n\n";
    std::cout << "Available optinos:\n";
    std::cout << "\t-h, --help\tShow help and exit.\n";
    std::cout << "\t-V, --version\tShow software version and exit.\n";
    std::cout << "\t-v, --verbose\tRun in verbose mode.\n";
    std::cout << "\t-d, --debug\tRun in debug mode.\n";
    std::cout << "\t-l, --log-file\tSpecify a log file. Default: print log entries to standard output.\n";
    std::cout << "\n";
    std::cout << "Positional arguments:\n";
    std::cout << "\tfilename\tProcess the specified Lua source file. Optional: if skipped or the value is -, the program reads standard input\n";
}

// Command line parser using getopt_long
Result<Options> ParseCommandLine(const int argc, char* argv[])
{
    const auto* short_opts = "hVvdl";
    const option long_opts[] = {{"help", no_argument, nullptr, 'h'}, {"version", no_argument, nullptr, 'V'}, {"verbose", no_argument, nullptr, 'v'},
        {"debug", no_argument, nullptr, 'd'}, {"log-file", required_argument, nullptr, 'l'}, {nullptr, 0, nullptr, 0}};

    auto result = Options{};
    int opt = getopt_long(argc, argv, short_opts, long_opts, nullptr);
    while (opt != -1)
    {
        switch (opt)
        {
            case 'h':
                result.command = Command::Help;
                return result;
            case 'V':
                result.command = Command::Version;
                return result;
            case 'v':
                result.verbose = true;
                break;
            case 'd':
                result.debug = true;
                break;
            case 'l':
                result.logFile = std::string(optarg);
                break;
            default:
                return Error("Unknown option.");
        }

        opt = getopt_long(argc, argv, short_opts, long_opts, nullptr);
    }

    result.command = Command::Evaluate;

    // Input filename
    if (optind < argc)
    {
        const std::string arg = argv[optind];
        if (arg != "-")
        {
            result.input = arg;
        }
        ++optind;
    }

    // End of positional arguments
    if (optind < argc)
    {
        return Error("Too many arguments provided.");
    }

    return result;
}
} // anonymous namespace

int main(int argc, char* argv[])
{
    const auto optionsResult = ParseCommandLine(argc, argv);
    if (!optionsResult.HasValue())
    {
        std::cerr << "Error: " << optionsResult.Error().message << std::endl;
        PrintHelp(argv[0]);
        return 1;
    }

    const auto& options = optionsResult.Value();
    if (Command::Help == options.command)
    {
        PrintHelp(argv[0]);
        return 0;
    }

    if (Command::Version == options.command)
    {
        std::cout << "Compliance Engine Assessor\nVersion: " << OSCONFIG_VERSION << "\n";
        return 0;
    }

    auto logHandle = options.logFile.HasValue() ? OpenLog(options.logFile->c_str(), nullptr) : nullptr;
    SetConsoleLoggingEnabled(nullptr == logHandle);
    auto logGuard = std::unique_ptr<OsConfigLogHandle, void (*)(OsConfigLogHandle*)>(&logHandle, CloseLog);

    if (options.verbose)
    {
        SetLoggingLevel(LoggingLevel::LoggingLevelInformational);
        OsConfigLogInfo(logHandle, "Verbose logging enabled");
    }

    if (options.debug)
    {
        SetLoggingLevel(LoggingLevel::LoggingLevelDebug);
        OsConfigLogInfo(logHandle, "Debug logging enabled");
    }

    auto context = std::unique_ptr<CommonContext>(new CommonContext(logHandle));
    LuaEvaluator evaluator;

    ifstream file;
    if (options.input.HasValue())
    {
        std::cerr << "Loading input file " << options.input.Value() << std::endl;
        file.open(options.input.Value());
        if (!file.is_open())
        {
            OsConfigLogError(logHandle, "Failed to open input file: %s", options.input->c_str());
            OSConfigTelemetryStatusTrace("fopen", errno);
            return 1;
        }
    }

    istream& inputStream = options.input.HasValue() ? file : std::cin;

    auto script = string(std::istreambuf_iterator<char>{inputStream}, {});
    auto indicators = IndicatorsTree();
    indicators.Push("Lua");
    auto result = evaluator.Evaluate(script, indicators, *context, Action::Audit);
    if (!result.HasValue())
    {
        OsConfigLogError(logHandle, "Failed to evaluate script: %s", result.Error().message.c_str());
        OSConfigTelemetryStatusTrace("Evaluate", result.Error().code);
        std::cerr << "Error: " << result.Error().message << std::endl;
        return 1;
    }

    std::cout << "Result: " << (result.Value() == Status::Compliant ? "Compliant" : "Non-Compliant") << std::endl;
    NestedListFormatter formatter;
    auto formattingResult = formatter.Format(indicators);
    if (!formattingResult.HasValue())
    {
        OsConfigLogError(logHandle, "Failed to format indicators: %s", formattingResult.Error().message.c_str());
        OSConfigTelemetryStatusTrace("Format", formattingResult.Error().code);
        std::cerr << "Error: " << formattingResult.Error().message << std::endl;
        return 1;
    }

    std::cout << formattingResult.Value() << std::endl;
    return 0;
}
