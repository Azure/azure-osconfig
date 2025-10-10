#include <CommonContext.h>
#include <CompactListFormatter.hpp>
#include <DebugFormatter.hpp>
#include <Engine.h>
#include <JsonFormatter.hpp>
#include <Logging.h>
#include <Mof.hpp>
#include <NestedListFormatter.hpp>
#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <string>

using ComplianceEngine::Action;
using ComplianceEngine::CommonContext;
using ComplianceEngine::CompactListFormatter;
using ComplianceEngine::DebugFormatter;
using ComplianceEngine::Engine;
using ComplianceEngine::JsonFormatter;
using ComplianceEngine::NestedListFormatter;
using ComplianceEngine::Optional;
using ComplianceEngine::PayloadFormatter;
using ComplianceEngine::Status;
using ComplianceEngine::MOF::Resource;
using std::ifstream;
using std::istream;
using std::string;

namespace
{
enum class Command
{
    Help,
    Audit,
    Remediate,
    Undefined
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
    Command command = Command::Undefined;
    std::string input;
    Optional<string> section;
    bool invalidArguments = false;
};

Options ParseArgs(int argc, char* argv[])
{
    assert(argc > 0);

    Options options;
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--help")
        {
            options.command = Command::Help;
            continue;
        }

        if (arg == "--verbose")
        {
            options.verbose = true;
            continue;
        }

        if (arg == "--debug")
        {
            options.debug = true;
            continue;
        }

        if (arg == "--log-file")
        {
            if (i + 1 < argc)
            {
                options.logFile = argv[++i];
            }
            else
            {
                std::cerr << "Missing argument for --log-file\n";
                options.command = Command::Help;
                options.invalidArguments = true;
                break;
            }

            continue;
        }

        if (arg == "--section")
        {
            if (i + 1 < argc)
            {
                options.section = argv[++i];
            }
            else
            {
                std::cerr << "Missing argument for --section\n";
                options.command = Command::Help;
                options.invalidArguments = true;
                break;
            }

            continue;
        }

        if (arg == "--format")
        {
            if (i + 1 < argc)
            {
                std::string formatArg = argv[++i];
                std::transform(formatArg.begin(), formatArg.end(), formatArg.begin(), ::tolower);
                if (formatArg == "nested-list")
                {
                    options.format = Format::NestedList;
                }
                else if (formatArg == "compact-list")
                {
                    options.format = Format::CompactList;
                }
                else if (formatArg == "json")
                {
                    options.format = Format::Json;
                }
                else if (formatArg == "debug")
                {
                    options.format = Format::Debug;
                }
                else
                {
                    std::cerr << "Invalid format: " << formatArg << "\n";
                    options.command = Command::Help;
                    options.invalidArguments = true;
                    break;
                }
            }
            else
            {
                std::cerr << "Missing argument for --format\n";
                options.command = Command::Help;
                options.invalidArguments = true;
                break;
            }
            continue;
        }

        if (arg == "audit")
        {
            options.command = Command::Audit;
            continue;
        }

        if (arg == "remediate")
        {
            options.command = Command::Remediate;
            continue;
        }

        if (Command::Undefined == options.command)
        {
            std::cerr << "Invalid command: " << arg << "\n";
            options.command = Command::Help;
            options.invalidArguments = true;
            break;
        }

        if (options.input.empty())
        {
            options.input = arg;
            continue;
        }

        std::cerr << "Unexpected argument: " << arg << "\n";
        options.command = Command::Help;
        options.invalidArguments = true;
        break;
    }

    if (Command::Undefined == options.command)
    {
        std::cerr << "No command specified.\n";
        options.command = Command::Help;
        options.invalidArguments = true;
    }

    return options;
}

void PrintHelp()
{
    std::cout << "Usage: assessor [--help] [--verbose] [--debug] {audit|remediate} [<input>]\n"
              << "If <input> is absent, reads from stdin. The program reads a MOF file contents from the <input>.\n";
}

} // anonymous namespace

int main(int argc, char* argv[])
{
    const auto options = ParseArgs(argc, argv);
    assert(Command::Undefined != options.command);
    if (Command::Help == options.command)
    {
        PrintHelp();
        return 0;
    }

    std::unique_ptr<ComplianceEngine::BenchmarkFormatters::BenchmarkFormatter> benchmarkFormatter;
    std::unique_ptr<PayloadFormatter> payloadFormatter;
    if (options.format.HasValue())
    {
        switch (options.format.Value())
        {
            case Format::NestedList:
                benchmarkFormatter =
                    std::unique_ptr<ComplianceEngine::BenchmarkFormatters::BenchmarkFormatter>(new ComplianceEngine::BenchmarkFormatters::NestedListFormatter());
                payloadFormatter = std::unique_ptr<PayloadFormatter>(new NestedListFormatter());
                break;
            case Format::CompactList:
                benchmarkFormatter =
                    std::unique_ptr<ComplianceEngine::BenchmarkFormatters::BenchmarkFormatter>(new ComplianceEngine::BenchmarkFormatters::CompactListFormatter());
                payloadFormatter = std::unique_ptr<PayloadFormatter>(new CompactListFormatter());
                break;
            case Format::Json:
                benchmarkFormatter =
                    std::unique_ptr<ComplianceEngine::BenchmarkFormatters::BenchmarkFormatter>(new ComplianceEngine::BenchmarkFormatters::JsonFormatter());
                payloadFormatter = std::unique_ptr<PayloadFormatter>(new JsonFormatter());
                break;
            case Format::Debug:
                benchmarkFormatter =
                    std::unique_ptr<ComplianceEngine::BenchmarkFormatters::BenchmarkFormatter>(new ComplianceEngine::BenchmarkFormatters::DebugFormatter());
                payloadFormatter = std::unique_ptr<PayloadFormatter>(new DebugFormatter());
                break;
            default:
                std::cerr << "Invalid format specified.\n";
                return 1;
        }
    }

    if (!payloadFormatter)
    {
        payloadFormatter = std::unique_ptr<PayloadFormatter>(new JsonFormatter());
    }
    if (!benchmarkFormatter)
    {
        // Default benchmark formatter
        benchmarkFormatter =
            std::unique_ptr<ComplianceEngine::BenchmarkFormatters::BenchmarkFormatter>(new ComplianceEngine::BenchmarkFormatters::JsonFormatter());
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
