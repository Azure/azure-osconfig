// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <CliOptions.hpp>
#include <CommonContext.h>
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
using ComplianceEngine::CommonContext;
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
using ComplianceEngine::MOF::ParseAll;
using ComplianceEngine::MOF::Resource;
using std::ifstream;
using std::istream;
using std::string;

namespace
{
std::unique_ptr<BenchmarkFormatter> MakeBenchmarkFormatter(Format f)
{
    switch (f)
    {
        case Format::NestedList:
            return std::unique_ptr<BenchmarkFormatter>(new NestedListFormatter());
        case Format::CompactList:
            return std::unique_ptr<BenchmarkFormatter>(new CompactListFormatter());
        case Format::Json:
            return std::unique_ptr<BenchmarkFormatter>(new JsonFormatter());
        case Format::Debug:
            return std::unique_ptr<BenchmarkFormatter>(new DebugFormatter());
    }
    return std::unique_ptr<BenchmarkFormatter>(new JsonFormatter());
}

std::unique_ptr<PayloadFormatter> MakePayloadFormatter(Format f)
{
    switch (f)
    {
        case Format::NestedList:
            return std::unique_ptr<PayloadFormatter>(new ComplianceEngine::NestedListFormatter());
        case Format::CompactList:
            return std::unique_ptr<PayloadFormatter>(new ComplianceEngine::CompactListFormatter());
        case Format::Json:
            return std::unique_ptr<PayloadFormatter>(new ComplianceEngine::JsonFormatter());
        case Format::Debug:
            return std::unique_ptr<PayloadFormatter>(new ComplianceEngine::DebugFormatter());
    }
    return std::unique_ptr<PayloadFormatter>(new ComplianceEngine::JsonFormatter());
}
} // anonymous namespace

int main(int argc, char* argv[])
{
    const auto optionsResult = ParseCommandLine(argc, argv);
    const char* programName = (argc > 0 && argv != nullptr) ? argv[0] : nullptr;
    if (!optionsResult.HasValue())
    {
        std::cerr << "Error: " << optionsResult.Error().message << std::endl;
        PrintHelp(std::cerr, programName);
        return 1;
    }

    const auto& options = optionsResult.Value();
    if (Command::Help == options.command)
    {
        PrintHelp(std::cout, programName);
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
        benchmarkFormatter = MakeBenchmarkFormatter(options.format.Value());
        payloadFormatter = MakePayloadFormatter(options.format.Value());
    }
    else
    {
        benchmarkFormatter = std::unique_ptr<BenchmarkFormatter>(new JsonFormatter());
        payloadFormatter = std::unique_ptr<PayloadFormatter>(new ComplianceEngine::JsonFormatter());
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
    if (!options.input.empty() && options.input != "-")
    {
        file.open(options.input);
        if (!file.is_open())
        {
            OsConfigLogError(logHandle, "Failed to open input file: %s", options.input.c_str());
            CloseLog(&logHandle);
            return 1;
        }
    }

    istream& inputStream = (!options.input.empty() && options.input != "-") ? static_cast<istream&>(file) : std::cin;
    auto status = Status::Compliant;

    auto handleEntry = [&](Resource&& mofEntry) -> Optional<Error> {
        if (options.section.HasValue())
        {
            if (mofEntry.benchmarkInfo.section.find(options.section.Value()) != 0)
            {
                OsConfigLogDebug(logHandle, "Skipping entry %s as it does not match section %s", mofEntry.resourceID.c_str(), options.section.Value().c_str());
                return Optional<Error>();
            }
        }

        auto procedureResult = engine.MmiSet((string("procedure") + mofEntry.ruleName).c_str(), mofEntry.procedure);
        if (!procedureResult.HasValue())
        {
            OsConfigLogError(logHandle, "Failed to set procedure: %s", procedureResult.Error().message.c_str());
            status = Status::NonCompliant;
            return Optional<Error>();
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
                        return Optional<Error>();
                    }
                }

                auto ruleName = string("audit") + mofEntry.ruleName;
                auto result = engine.MmiGet(ruleName.c_str());
                if (!result.HasValue())
                {
                    OsConfigLogError(logHandle, "Failed to perform audit: %s", result.Error().message.c_str());
                    status = Status::NonCompliant;
                    return Optional<Error>();
                }

                auto addErr = benchmarkFormatter->AddEntry(mofEntry, result.Value().status, result.Value().payload);
                if (addErr)
                {
                    OsConfigLogError(logHandle, "Failed to add entry to formatter: %s", addErr.Value().message.c_str());
                    status = Status::NonCompliant;
                    return Optional<Error>();
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
                    return Optional<Error>();
                }

                auto addErr = benchmarkFormatter->AddEntry(mofEntry, result.Value(), "[]");
                if (addErr)
                {
                    OsConfigLogError(logHandle, "Failed to add entry to formatter: %s", addErr.Value().message.c_str());
                    status = Status::NonCompliant;
                    return Optional<Error>();
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
        return Optional<Error>();
    };

    auto parseErr = ParseAll(inputStream, handleEntry);
    if (parseErr.HasValue())
    {
        OsConfigLogError(logHandle, "Failed to parse MOF input: %s", parseErr.Value().message.c_str());
        CloseLog(&logHandle);
        return 1;
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
