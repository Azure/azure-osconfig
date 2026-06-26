// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <AssessorContext.h>
#include <CliOptions.hpp>
#include <CommonContext.h>
#include <CompactListFormatter.hpp>
#include <DebugFormatter.hpp>
#include <Engine.h>
#include <InputSecurity.hpp>
#include <JsonFormatter.hpp>
#include <Logging.h>
#include <Mof.hpp>
#include <NestedListFormatter.hpp>
#include <Optional.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
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
using ComplianceEngine::Assessor::OpenVerifiedInput;
using ComplianceEngine::Assessor::Options;
using ComplianceEngine::Assessor::ParseCommandLine;
using ComplianceEngine::Assessor::PrintHelp;
using ComplianceEngine::Assessor::RefusePathTraversal;
using ComplianceEngine::Assessor::RefuseUnsafeLogFile;
using ComplianceEngine::Assessor::RefuseWritableParentDir;
using ComplianceEngine::BenchmarkFormatters::BenchmarkFormatter;
using ComplianceEngine::BenchmarkFormatters::CompactListFormatter;
using ComplianceEngine::BenchmarkFormatters::DebugFormatter;
using ComplianceEngine::BenchmarkFormatters::JsonFormatter;
using ComplianceEngine::BenchmarkFormatters::NestedListFormatter;
using ComplianceEngine::MOF::Resource;
using std::istream;
using std::string;

// Upper bound on the total bytes accepted from either --input or stdin. A
// real benchmark MOF is far smaller; the cap prevents a malformed or hostile
// input (or a pipe that never ends) from exhausting memory while we run as
// root. NOTE: when MOF parsing is reworked to stream both file and stdin
// inputs, this byte-accounting moves into the streaming parser.
static constexpr size_t kMaxInputBytes = static_cast<size_t>(8) * 1024 * 1024;

// Upper bound on the number of MOF entries processed from a single input. A
// real benchmark has a few hundred rules; a vastly larger count indicates a
// malformed or hostile input.
static constexpr size_t kMaxMofEntries = 100000;

int main(int argc, char* argv[])
{
    // Ensure file-creation permissions are at least as restrictive as 0077
    // without overriding a stricter inherited mask.
    ::umask(::umask(0) | S_IRWXG | S_IRWXO);

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
        std::cout << "Compliance Engine Assessor\nVersion: " << KOMPLI_VERSION << "\n";
        return 0;
    }

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

    // Validate the log-file path before opening it. The shared logging code
    // opens the log with a symlink-following append and chmod's it while we run
    // as root, so an attacker-controlled symlink or writable parent directory
    // could redirect those writes. No log handle exists yet, so failures are
    // reported to stderr.
    if (options.logFile.HasValue())
    {
        if (options.logFile->empty() || RefuseUnsafeLogFile(options.logFile.Value(), nullptr))
        {
            std::cerr << "Error: refusing to use unsafe log file path." << std::endl;
            return 1;
        }
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

    std::istringstream fileStream;
    if (!options.input.empty())
    {
        if (RefusePathTraversal(options.input, logHandle.get()))
        {
            return 1;
        }
        if (RefuseWritableParentDir(options.input, logHandle.get()))
        {
            return 1;
        }
        const auto inputFdResult = OpenVerifiedInput(options.input, logHandle.get());
        if (!inputFdResult.HasValue())
        {
            return 1;
        }
        const int inputFd = inputFdResult.Value();
        std::string content;
        char buf[4096];
        ssize_t n = 0;
        bool tooLarge = false;
        while (true)
        {
            n = ::read(inputFd, buf, sizeof(buf));
            if (n > 0)
            {
                if (content.size() + static_cast<size_t>(n) > kMaxInputBytes)
                {
                    tooLarge = true;
                    break;
                }
                content.append(buf, static_cast<size_t>(n));
            }
            else if (n == 0)
            {
                break; // EOF
            }
            else if (errno != EINTR)
            {
                break; // real error
            }
            // EINTR: signal interrupted the syscall; retry
        }
        const int savedErrno = errno;
        ::close(inputFd);
        if (tooLarge)
        {
            OsConfigLogError(logHandle.get(), "Refusing to read input file '%s': exceeds maximum size of %zu bytes.", options.input.c_str(), kMaxInputBytes);
            return 1;
        }
        if (n < 0)
        {
            OsConfigLogError(logHandle.get(), "Failed to read input file '%s': %s", options.input.c_str(), std::strerror(savedErrno));
            return 1;
        }
        fileStream.str(content);
    }

    istream& inputStream = options.input.empty() ? std::cin : fileStream;
    string line;
    auto status = Status::Compliant;
    bool hasError = false;
    // Total bytes consumed from the input stream (outer scan loop + all lines
    // read inside Resource::ParseSingleEntry). The --input path is already
    // bounded by kMaxInputBytes above; this shared counter applies the same
    // ceiling to the stdin path without buffering all of stdin (per the
    // planned MOF streaming rework). Per-entry line-length and entry-count
    // limits are also enforced inside Resource::ParseSingleEntry.
    size_t bytesConsumed = 0;
    size_t entryCount = 0;
    while (std::getline(inputStream, line))
    {
        // Include one byte for the newline that std::getline() discards.
        bytesConsumed += line.size() + 1;
        if (bytesConsumed > kMaxInputBytes)
        {
            OsConfigLogError(logHandle.get(), "Refusing to process input: exceeds maximum size of %zu bytes.", kMaxInputBytes);
            return 1;
        }

        if (line.find("instance of OsConfigResource as") == std::string::npos)
        {
            continue;
        }

        if (++entryCount > kMaxMofEntries)
        {
            OsConfigLogError(logHandle.get(), "Refusing to process input: exceeds maximum of %zu MOF entries.", kMaxMofEntries);
            return 1;
        }

        auto mofParsingResult = Resource::ParseSingleEntry(inputStream, bytesConsumed, kMaxInputBytes);
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
                    // If the producer flagged InitObject support but supplied no
                    // DesiredObjectValue, fall back to an empty JSON object so
                    // we don't deref an empty Optional.
                    const string initPayload = mofEntry.payload.HasValue() ? mofEntry.payload.Value() : string("{}");
                    auto result = engine.MmiSet((string("init") + mofEntry.ruleName).c_str(), initPayload);
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
                if (!mofEntry.payload.HasValue())
                {
                    OsConfigLogError(logHandle.get(), "Cannot remediate '%s': missing DesiredObjectValue.", mofEntry.resourceID.c_str());
                    status = Status::NonCompliant;
                    if (!options.continueOnError)
                    {
                        return 1;
                    }
                    hasError = true;
                    continue;
                }
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
