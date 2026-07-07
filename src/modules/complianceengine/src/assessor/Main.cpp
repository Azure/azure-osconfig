// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

//
// compliance-engine-assessor
//
// Threat model
// ------------
// This tool is intended to run as root on Linux endpoints to perform CIS
// benchmark audit and remediation. The trust boundary is the invoking
// operator: the input MOF file, the log-file path, and command-line arguments
// are treated as operator-supplied (trusted to be benign in intent, but not
// to be free of bugs or accidental hostile content).
//
//  - The input MOF parser is strict and streaming (see MofResourceRange): it
//    validates the fixed field set the augmentation engine emits, rejects
//    unknown fields, and bounds line length, total size, and entry count. It
//    owns the input file and encapsulates the integrity checks below. A fuzzer
//    target exercises it; extend the corpus when changing the format.
//
//  - Input file integrity (when --input is used; stdin bypasses all checks):
//
//    1. Parent directory (stat): must be root-owned and not writable by
//       group or others. A writable directory enables a rename-swap attack:
//       an attacker can unlink the validated file and place a hostile one
//       before the process reads it.
//
//    2. open(O_RDONLY|O_NOFOLLOW|O_CLOEXEC): the kernel refuses symlinks in
//       the final path component atomically (ELOOP), eliminating the
//       lstat-then-open TOCTOU window. Symlinks are intentionally rejected
//       rather than accepted-with-a-warning; callers that stage input via a
//       symlink must resolve the link before passing the path. Note: symlinks
//       in intermediate path components are not checked; the operator is
//       trusted to supply a straightforward path.
//
//    3. fstat on the open fd: ownership and mode are verified against the
//       inode we actually hold, not a potentially-swapped path entry. The
//       file must be a regular file (FIFOs, devices, sockets are refused so
//       they cannot block the read or stream unbounded data), root-owned, and
//       not group/world-writable.
//
//    4. The verified fd is wrapped in a stream and read incrementally (never
//       slurped whole). The fd keeps the inode reachable across the read even
//       if the directory entry is concurrently renamed or unlinked. The total
//       bytes read, per-line length, and entry count are all bounded inside the
//       streaming parser to keep memory use bounded.
//
//  - stdin (--input not supplied): all file integrity checks are bypassed.
//    Streaming inputs (pipes, process substitution) must use stdin. The bytes
//    consumed, per-line length, and total entry count are still bounded inside
//    the streaming MOF parser. Callers in automated pipelines should always use
//    --input with a root-owned, non-world-writable file.
//
//  - umask is tightened to at least S_IRWXG|S_IRWXO (preserving any stricter
//    inherited mask). The log file when --log-file is supplied is the primary
//    case.
//
//  - The --log-file path is validated before opening (RefuseUnsafeLogFile):
//    the shared logging code opens it with a symlink-following append and
//    chmod's it while we run as root, so a symlink, non-root-owned target, or
//    writable parent directory is refused to prevent redirecting root's writes
//    onto a sensitive file.
//
//    Residual TOCTOU (known limitation): unlike --input, the log file is NOT
//    verified via fstat() on a held fd. The shared OpenLog() API is path-only
//    (no fd-accepting entry point) and TrimLog() re-opens the path with
//    fopen() on every log rotation, so a pinned, pre-verified fd cannot be
//    handed to the logging layer; both the initial open and each rotation
//    re-resolve the path with symlink-following fopen(). RefuseUnsafeLogFile()
//    therefore checks the path with lstat() shortly before OpenLog() resolves
//    it again, leaving a small check-to-use window. That window is closed in
//    practice by the parent-directory check: requiring the parent to be
//    root-owned and not group/world-writable prevents an attacker from
//    creating, renaming, or swapping the entry at all, so the path cannot be
//    pointed at a new target between the check and any (re-)open. Fully
//    eliminating the window (fd-based open with O_NOFOLLOW handed to the
//    logger) would require changing the shared logging library, which is
//    out of scope here as it affects every azure-osconfig binary.
//
//  - The PATH/IFS environment is inherited and used by procedure scripts the
//    engine spawns. Sanitizing the environment is the engine's
//    responsibility, not the assessor's.

#include "CliOptions.hpp"
#include "CompactListFormatter.hpp"
#include "DebugFormatter.hpp"
#include "InputSecurity.hpp"
#include "JsonFormatter.hpp"
#include "Mof.hpp"
#include "NestedListFormatter.hpp"

#include <AssessorContext.h>
#include <CommonContext.h>
#include <Engine.h>
#include <Logging.h>
#include <Optional.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <memory>
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
using ComplianceEngine::Assessor::Options;
using ComplianceEngine::Assessor::ParseCommandLine;
using ComplianceEngine::Assessor::PrintHelp;
using ComplianceEngine::Assessor::RefuseUnsafeLogFile;
using ComplianceEngine::BenchmarkFormatters::BenchmarkFormatter;
using ComplianceEngine::BenchmarkFormatters::CompactListFormatter;
using ComplianceEngine::BenchmarkFormatters::DebugFormatter;
using ComplianceEngine::BenchmarkFormatters::JsonFormatter;
using ComplianceEngine::BenchmarkFormatters::NestedListFormatter;
using ComplianceEngine::MOF::MofResourceRange;
using std::string;

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

    // Open the input as a strictly-validated, streaming MOF range. For --input
    // the range encapsulates the full input-hardening posture (path-traversal
    // rejection, root-owned non-writable parent directory, O_NOFOLLOW open, and
    // regular-file/ownership/mode checks) and owns the file; for stdin it
    // streams without those on-disk checks. Size, line, and entry caps are
    // enforced inside the range.
    auto rangeResult = options.input.empty() ? MofResourceRange::Make(std::cin, logHandle.get()) : MofResourceRange::Make(options.input, logHandle.get());
    if (!rangeResult.HasValue())
    {
        OsConfigLogError(logHandle.get(), "Failed to open MOF input: %s", rangeResult.Error().message.c_str());
        return 1;
    }
    auto& mofRange = rangeResult.Value();

    auto status = Status::Compliant;
    bool hasError = false;
    for (const auto& entryResult : mofRange)
    {
        if (!entryResult.HasValue())
        {
            OsConfigLogError(logHandle.get(), "Failed to parse MOF entry: %s", entryResult.Error().message.c_str());
            return 1;
        }

        const auto& mofEntry = entryResult.Value();
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
                // The augmentation engine emits an empty DesiredObjectValue for
                // every rule (modelled here as an absent payload); fall back to
                // an empty JSON object so remediation can still run, mirroring
                // the audit-init path above.
                const string remediatePayload = mofEntry.payload.HasValue() ? mofEntry.payload.Value() : string("{}");
                auto ruleName = string("remediate") + mofEntry.ruleName;
                auto result = engine.MmiSet(ruleName.c_str(), remediatePayload);
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
