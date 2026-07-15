#include <CliOptions.hpp>
#include <algorithm>
#include <getopt.h>
#include <iostream>
#include <string>

namespace ComplianceEngine
{
namespace Assessor
{

using std::string;

void PrintHelp(const std::string& programName)
{
    std::cout << "Usage: " + programName + " [options] <command> [filename]\n\n";
    std::cout << "Commands:\n";
    std::cout << "\taudit\t\tEvaluate a benchmark and emit the canonical result JSON.\n";
    std::cout << "\tremediate\tRemediate a benchmark and emit the canonical result JSON.\n";
    std::cout << "\trender\t\tRender a canonical result JSON into a presentation format.\n";
    std::cout << "\n";
    std::cout << "Common options:\n";
    std::cout << "\t-h, --help\tShow help and exit.\n";
    std::cout << "\t-V, --version\tShow software version and exit.\n";
    std::cout << "\t-v, --verbose\tRun in verbose mode.\n";
    std::cout << "\t-d, --debug\tRun in debug mode.\n";
    std::cout << "\n";
    std::cout << "audit / remediate options:\n";
    std::cout << "\t-e, --continue-on-error\tSkip rules that fail due to engine errors and continue processing. Returns 1 if any error occurred.\n";
    std::cout << "\t-l, --log-file\tSpecify a log file. Default: print log entries to standard output.\n";
    std::cout << "\t-s, --section\tProcess only specific sections. Default: process all available rules.\n";
    std::cout << "\tfilename\tProcess the specified MOF file. Optional: if skipped or the value is '-', the program reads standard input.\n";
    std::cout << "\n";
    std::cout << "render options:\n";
    std::cout << "\t-f, --format\tPresentation format. Allowed values: {junit, nested-list, compact-list, debug}. Default: junit.\n";
    std::cout << "\t    --suite-name\tName for the JUnit <testsuite>. Default: compliance.\n";
    std::cout << "\tfilename\tRead the canonical result JSON from this file. Optional: if skipped or '-', reads standard input.\n";
}

// Long-only option identifiers (no short equivalent). Values start above the
// ASCII range so they never collide with a short-option character.
enum
{
    kSuiteNameOpt = 256
};

// Command line parser using getopt_long.
//
// Resets getopt's global parser state on entry so this function can be safely
// called more than once per process (notably from unit tests). The shipping
// binary calls it exactly once, so the reset is a no-op there.
Result<Options> ParseCommandLine(const int argc, char* argv[])
{
    optind = 0;
#ifdef optreset
    optreset = 1;
    optind = 1;
#endif

    const auto* short_opts = "hVvdel:s:f:";
    const option long_opts[] = {{"help", no_argument, nullptr, 'h'}, {"version", no_argument, nullptr, 'V'}, {"verbose", no_argument, nullptr, 'v'},
        {"debug", no_argument, nullptr, 'd'}, {"continue-on-error", no_argument, nullptr, 'e'}, {"log-file", required_argument, nullptr, 'l'},
        {"section", required_argument, nullptr, 's'}, {"format", required_argument, nullptr, 'f'},
        {"suite-name", required_argument, nullptr, kSuiteNameOpt}, {nullptr, 0, nullptr, 0}};

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
            case 'e':
                result.continueOnError = true;
                break;
            case 'l':
                if (optarg[0] == '\0')
                {
                    return Error("Log file path must not be empty.");
                }
                result.logFile = std::string(optarg);
                break;
            case 's':
                if (optarg[0] == '\0')
                {
                    return Error("Section must not be empty.");
                }
                result.section = std::string(optarg);
                break;
            case 'f': {
                if (optarg[0] == '\0')
                {
                    return Error("Format must not be empty.");
                }
                auto formatArg = std::string(optarg);
                std::transform(formatArg.begin(), formatArg.end(), formatArg.begin(), ::tolower);
                if (formatArg == "junit")
                {
                    result.format = Format::Junit;
                }
                else if (formatArg == "nested-list")
                {
                    result.format = Format::NestedList;
                }
                else if (formatArg == "compact-list")
                {
                    result.format = Format::CompactList;
                }
                else if (formatArg == "debug")
                {
                    result.format = Format::Debug;
                }
                else
                {
                    return Error("Invalid format: " + formatArg + ". Allowed values: {junit, nested-list, compact-list, debug}.");
                }
                break;
            }
            case kSuiteNameOpt:
                if (optarg[0] == '\0')
                {
                    return Error("Suite name must not be empty.");
                }
                result.suiteName = std::string(optarg);
                break;
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
        else if (arg == "render")
        {
            result.command = Command::Render;
        }
        else
        {
            return Error("Invalid command: '" + arg + "'. Must be 'audit', 'remediate' or 'render'.");
        }
        ++optind;
    }
    else
    {
        return Error("Missing required command: 'audit', 'remediate' or 'render'.");
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

    // Cross-option validation: keep the audit/remediate surface (which always
    // emits canonical JSON) free of presentation flags, and keep render free of
    // scan flags.
    if (Command::Render == result.command)
    {
        if (result.section.HasValue())
        {
            return Error("--section is not valid for the 'render' subcommand.");
        }
        // Default the renderer when none was supplied.
        if (!result.format.HasValue())
        {
            result.format = Format::Junit;
        }
    }
    else
    {
        if (result.format.HasValue())
        {
            return Error("--format is only valid for the 'render' subcommand; 'audit' and 'remediate' always emit the canonical JSON.");
        }
        if (result.suiteName.HasValue())
        {
            return Error("--suite-name is only valid for the 'render' subcommand.");
        }
    }

    return result;
}

} // namespace Assessor
} // namespace ComplianceEngine
