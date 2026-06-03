#include <AssessorContext.h>
#include <CliOptions.hpp>
#include <CompactListFormatter.hpp>
#include <DebugFormatter.hpp>
#include <Engine.h>
#include <JsonFormatter.hpp>
#include <Logging.h>
#include <Mof.hpp>
#include <NestedListFormatter.hpp>
#include <Optional.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <version.h>

using ComplianceEngine::Action;
using ComplianceEngine::AssessorContext;
using ComplianceEngine::Engine;
using ComplianceEngine::Error;
using ComplianceEngine::Optional;
using ComplianceEngine::PayloadFormatter;
using ComplianceEngine::Result;
using ComplianceEngine::Status;
using ComplianceEngine::Assessor::Command;
using ComplianceEngine::Assessor::Format;
using ComplianceEngine::Assessor::Options;
using ComplianceEngine::Assessor::ParseCommandLine;
using ComplianceEngine::Assessor::PrintHelp;
using ComplianceEngine::BenchmarkFormatters::BenchmarkFormatter;
using ComplianceEngine::BenchmarkFormatters::CompactListFormatter;
using ComplianceEngine::BenchmarkFormatters::DebugFormatter;
using ComplianceEngine::BenchmarkFormatters::JsonFormatter;
using ComplianceEngine::BenchmarkFormatters::NestedListFormatter;
using ComplianceEngine::MOF::Resource;
using std::ifstream;
using std::istream;
using std::string;

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

    std::unique_ptr<OsConfigLog, void (*)(OsConfigLog*)> logHandle(options.logFile.HasValue() ? OpenLog(options.logFile->c_str(), nullptr) : nullptr,
        [](OsConfigLog* h) {
            OsConfigLogHandle tmp = h;
            CloseLog(&tmp);
        });
    if (logHandle)
    {
        SetConsoleLoggingEnabled(false);
    }

    if (options.verbose)
    {
        SetLoggingLevel(LoggingLevel::LoggingLevelInformational);
        OsConfigLogInfo(logHandle.get(), "Verbose logging enabled");
    }

    if (options.debug)
    {
        SetLoggingLevel(LoggingLevel::LoggingLevelDebug);
        OsConfigLogInfo(logHandle.get(), "Debug logging enabled");
    }

    auto context = std::unique_ptr<AssessorContext>(new AssessorContext(logHandle.get()));
    Engine engine(std::move(context), std::move(payloadFormatter));

    auto error = benchmarkFormatter->Begin(options.command == Command::Audit ? Action::Audit : Action::Remediate);
    if (error)
    {
        OsConfigLogError(logHandle.get(), "Failed to begin formatted output: %s", error.Value().message.c_str());
        return 1;
    }

    ifstream file;
    if (!options.input.empty())
    {
        file.open(options.input);
        if (!file.is_open())
        {
            OsConfigLogError(logHandle.get(), "Failed to open input file: %s", options.input.c_str());
            return 1;
        }
    }

    istream& inputStream = options.input.empty() ? std::cin : file;
    string line;
    auto status = Status::Compliant;
    bool hasError = false;
    while (std::getline(inputStream, line))
    {
        if (line.find("instance of OsConfigResource as") == std::string::npos)
        {
            continue;
        }

        auto mofParsingResult = Resource::ParseSingleEntry(inputStream);
        if (!mofParsingResult.HasValue())
        {
            OsConfigLogError(logHandle.get(), "Failed to parse MOF entry: %s", mofParsingResult.Error().message.c_str());
            return 1;
        }

        auto mofEntry = std::move(mofParsingResult.Value());
        if (options.section.HasValue())
        {
            if (mofEntry.benchmarkInfo.section.find(options.section.Value()) != 0)
            {
                OsConfigLogDebug(logHandle.get(), "Skipping entry %s as it does not match section %s", mofEntry.resourceID.c_str(),
                    options.section.Value().c_str());
                continue;
            }
        }

        auto procedureResult = engine.MmiSet((string("procedure") + mofEntry.ruleName).c_str(), mofEntry.procedure);
        if (!procedureResult.HasValue())
        {
            OsConfigLogError(logHandle.get(), "Failed to set procedure: %s", procedureResult.Error().message.c_str());
            if (!options.continueOnError)
            {
                return 1;
            }
            hasError = true;
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
                        OsConfigLogError(logHandle.get(), "Failed to init audit: %s", result.Error().message.c_str());
                        if (!options.continueOnError)
                        {
                            return 1;
                        }
                        hasError = true;
                        continue;
                    }
                }

                auto ruleName = string("audit") + mofEntry.ruleName;
                auto result = engine.MmiGet(ruleName.c_str());
                if (!result.HasValue())
                {
                    OsConfigLogError(logHandle.get(), "Failed to perform audit: %s", result.Error().message.c_str());
                    if (!options.continueOnError)
                    {
                        return 1;
                    }
                    hasError = true;
                    continue;
                }

                error = benchmarkFormatter->AddEntry(mofEntry, result.Value().status, result.Value().payload);
                if (error)
                {
                    OsConfigLogError(logHandle.get(), "Failed to add entry to JSON formatter: %s", error.Value().message.c_str());
                    if (!options.continueOnError)
                    {
                        return 1;
                    }
                    hasError = true;
                    continue;
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
                    OsConfigLogError(logHandle.get(), "Failed to remediate: %s", result.Error().message.c_str());
                    if (!options.continueOnError)
                    {
                        return 1;
                    }
                    hasError = true;
                    continue;
                }

                error = benchmarkFormatter->AddEntry(mofEntry, result.Value(), "[]");
                if (error)
                {
                    OsConfigLogError(logHandle.get(), "Failed to add entry to JSON formatter: %s", error.Value().message.c_str());
                    if (!options.continueOnError)
                    {
                        return 1;
                    }
                    hasError = true;
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
    if (!result.HasValue())
    {
        OsConfigLogError(logHandle.get(), "Failed to finish formatted output: %s", result.Error().message.c_str());
        return 1;
    }

    std::cout << result.Value() << "\n";
    return hasError ? 1 : 0;
}
