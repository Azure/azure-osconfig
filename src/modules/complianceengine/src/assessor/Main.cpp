#include <CommonContext.h>
#include <CompactListFormatter.hpp>
#include <DebugFormatter.hpp>
#include <Engine.h>
#include <JsonFormatter.hpp>
#include <Logging.h>
#include <Mof.hpp>
#include <NestedListFormatter.hpp>
#include <Optional.h>
#include <algorithm>
#include <cassert>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <memory>
#include <string>
#include <version.h>

using ComplianceEngine::Action;
using ComplianceEngine::CommonContext;
using ComplianceEngine::Engine;
using ComplianceEngine::Error;
using ComplianceEngine::Optional;
using ComplianceEngine::PayloadFormatter;
using ComplianceEngine::Result;
using ComplianceEngine::Status;
using ComplianceEngine::BenchmarkFormatters::BenchmarkFormatter;
using ComplianceEngine::BenchmarkFormatters::CompactListFormatter;
using ComplianceEngine::BenchmarkFormatters::DebugFormatter;
using ComplianceEngine::BenchmarkFormatters::JsonFormatter;
using ComplianceEngine::BenchmarkFormatters::NestedListFormatter;
using ComplianceEngine::MOF::Resource;
using std::ifstream;
using std::istream;
using std::string;

namespace
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
    Optional<string> logFile;
    Optional<Format> format;
    Command command = Command::Help;
    std::string input;
    Optional<string> section;
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
    std::cout << "\t-s, --section\tProcess only specific sections. Default: process all available rules.\n";
    std::cout << "\n";
    std::cout << "Positional arguments:\n";
    std::cout << "\tcommand\t\tDetermine whether to run in audit or remediation mode. Allowed values: {audit|remediate}.\n";
    std::cout << "\tfilename\tProcess the specified MOF file. Optional: if skipped or the value is -, the program reads standard input\n";
}

// Command line parser using getopt_long
Result<Options> ParseCommandLine(const int argc, char* argv[])
{
    const auto* short_opts = "hVvdl:s:f:";
    const option long_opts[] = {{"help", no_argument, nullptr, 'h'}, {"version", no_argument, nullptr, 'V'}, {"verbose", no_argument, nullptr, 'v'},
        {"debug", no_argument, nullptr, 'd'}, {"log-file", required_argument, nullptr, 'l'}, {"section", required_argument, nullptr, 's'},
        {"format", required_argument, nullptr, 'f'}, {nullptr, 0, nullptr, 0}};

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
            case 's':
                result.section = std::string(optarg);
                break;
            case 'f': {
                auto formatArg = std::string(optarg);
                std::transform(formatArg.begin(), formatArg.end(), formatArg.begin(), ::tolower);
                if (formatArg == "nested-list")
                {
                    result.format = Format::NestedList;
                }
                else if (formatArg == "compact-list")
                {
                    result.format = Format::CompactList;
                }
                else if (formatArg == "json")
                {
                    result.format = Format::Json;
                }
                else if (formatArg == "debug")
                {
                    result.format = Format::Debug;
                }
                else
                {
                    return Error("Invalid format: " + formatArg);
                }
                break;
            }
            default:
                return Error("Unknown option.");
        }

        opt = getopt_long(argc, argv, short_opts, long_opts, nullptr);
    }

    // After options, parse the positional arguments
    if (optind < argc)
    {
        const std::string arg = argv[optind];
        if (arg == "audit")
        {
            result.command = Command::Audit;
        }
        else if (arg == "remediate")
        {
            result.command = Command::Remediate;
        }
        else
        {
            return Error("Invalid command: '" + arg + "'. Must be 'audit' or 'remediate'.");
        }
        ++optind;
    }
    else
    {
        return Error("Missing required command: 'audit' or 'remediate'.");
    }

    // Input filename
    if (optind < argc)
    {
        const std::string arg = argv[optind];
        result.input = arg;
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

    std::cerr << "Compliance Engine Assessor\n";
    std::unique_ptr<BenchmarkFormatter> benchmarkFormatter;
    std::unique_ptr<PayloadFormatter> payloadFormatter;
    if (options.format.HasValue())
    {
        switch (options.format.Value())
        {
            case Format::NestedList:
                benchmarkFormatter = std::unique_ptr<BenchmarkFormatter>(new NestedListFormatter());
                payloadFormatter = std::unique_ptr<PayloadFormatter>(new ComplianceEngine::NestedListFormatter());
                break;
            case Format::CompactList:
                benchmarkFormatter = std::unique_ptr<BenchmarkFormatter>(new CompactListFormatter());
                payloadFormatter = std::unique_ptr<PayloadFormatter>(new ComplianceEngine::CompactListFormatter());
                break;
            case Format::Json:
                benchmarkFormatter = std::unique_ptr<BenchmarkFormatter>(new JsonFormatter());
                payloadFormatter = std::unique_ptr<PayloadFormatter>(new ComplianceEngine::JsonFormatter());
                break;
            case Format::Debug:
                benchmarkFormatter = std::unique_ptr<BenchmarkFormatter>(new DebugFormatter());
                payloadFormatter = std::unique_ptr<PayloadFormatter>(new ComplianceEngine::DebugFormatter());
                break;
            default:
                std::cerr << "Invalid format specified.\n";
                return 1;
        }
    }
    if (!payloadFormatter)
    {
        payloadFormatter = std::unique_ptr<PayloadFormatter>(new ComplianceEngine::JsonFormatter());
    }
    if (!benchmarkFormatter)
    {
        benchmarkFormatter = std::unique_ptr<BenchmarkFormatter>(new JsonFormatter());
    }

    auto logHandle = options.logFile.HasValue() ? OpenLog(options.logFile->c_str(), nullptr) : nullptr;
    if (nullptr != logHandle)
    {
        SetConsoleLoggingEnabled(false);
    }

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
    Engine engine(std::move(context), std::move(payloadFormatter));

    auto error = benchmarkFormatter->Begin(options.command == Command::Audit ? Action::Audit : Action::Remediate);
    if (error)
    {
        OsConfigLogError(logHandle, "Failed to begin formatted output: %s", error.Value().message.c_str());
        CloseLog(&logHandle);
        return 1;
    }

    ifstream file;
    if (!options.input.empty())
    {
        file.open(options.input);
        if (!file.is_open())
        {
            OsConfigLogError(logHandle, "Failed to open input file: %s", options.input.c_str());
            CloseLog(&logHandle);
            return 1;
        }
    }

    istream& inputStream = options.input.empty() ? std::cin : file;
    string line;
    auto status = Status::Compliant;
    while (std::getline(inputStream, line))
    {
        if (line.find("instance of OsConfigResource as") == std::string::npos)
        {
            continue;
        }

        auto mofParsingResult = Resource::ParseSingleEntry(inputStream);
        if (!mofParsingResult.HasValue())
        {
            OsConfigLogError(logHandle, "Failed to parse MOF entry: %s", mofParsingResult.Error().message.c_str());
            CloseLog(&logHandle);
            return 1;
        }

        auto mofEntry = std::move(mofParsingResult.Value());
        if (options.section.HasValue())
        {
            if (mofEntry.benchmarkInfo.section.find(options.section.Value()) != 0)
            {
                OsConfigLogDebug(logHandle, "Skipping entry %s as it does not match section %s", mofEntry.resourceID.c_str(), options.section.Value().c_str());
                continue;
            }
        }

        auto procedureResult = engine.MmiSet((string("procedure") + mofEntry.ruleName).c_str(), mofEntry.procedure);
        if (!procedureResult.HasValue())
        {
            OsConfigLogError(logHandle, "Failed to set procedure: %s", procedureResult.Error().message.c_str());
            status = Status::NonCompliant;
            continue;
        }

        switch (options.command)
        {
            case Command::Audit: {
                if (mofEntry.hasInitAudit)
                {
                    auto result = engine.MmiSet((string("init") + mofEntry.ruleName).c_str(), mofEntry.payload.Value());
                    if (!result.HasValue())
                    {
                        OsConfigLogError(logHandle, "Failed to init audit: %s", result.Error().message.c_str());
                        status = Status::NonCompliant;
                        continue;
                    }
                }

                auto ruleName = string("audit") + mofEntry.ruleName;
                auto result = engine.MmiGet(ruleName.c_str());
                if (!result.HasValue())
                {
                    OsConfigLogError(logHandle, "Failed to perform audit: %s", result.Error().message.c_str());
                    status = Status::NonCompliant;
                    continue;
                }

                error = benchmarkFormatter->AddEntry(mofEntry, result.Value().status, result.Value().payload);
                if (error)
                {
                    OsConfigLogError(logHandle, "Failed to add entry to JSON formatter: %s", error.Value().message.c_str());
                    status = Status::NonCompliant;
                    break;
                }

                if (result.Value().status != Status::Compliant)
                {
                    status = Status::NonCompliant;
                }

                break;
            }

            case Command::Remediate: {
                auto ruleName = string("remediate") + mofEntry.ruleName;
                auto result = engine.MmiSet(ruleName.c_str(), mofEntry.payload.Value());
                if (!result.HasValue())
                {
                    OsConfigLogError(logHandle, "Failed to remediate: %s", result.Error().message.c_str());
                    status = Status::NonCompliant;
                    continue;
                }

                error = benchmarkFormatter->AddEntry(mofEntry, result.Value(), "[]");
                if (error)
                {
                    OsConfigLogError(logHandle, "Failed to add entry to JSON formatter: %s", error.Value().message.c_str());
                    status = Status::NonCompliant;
                    continue;
                }

                if (result.Value() != Status::Compliant)
                {
                    status = Status::NonCompliant;
                }

                break;
            }

            default:
                break;
        }
    }

    auto result = benchmarkFormatter->Finish(status);
    CloseLog(&logHandle);
    if (!result.HasValue())
    {
        OsConfigLogError(logHandle, "Failed to finish formatted output: %s", result.Error().message.c_str());
        return 1;
    }

    std::cout << result.Value() << "\n";
    return status == Status::Compliant ? 0 : 1;
}
