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
    const option long_opts[] = { {"help", no_argument, nullptr, 'h'}, {"version", no_argument, nullptr, 'V'},
        {"verbose", no_argument, nullptr, 'v'}, {"debug", no_argument, nullptr, 'd'}, {"continue-on-error", no_argument, nullptr, 'e'},
        {"log-file", required_argument, nullptr, 'l'}, {"section", required_argument, nullptr, 's'}, {"format", required_argument, nullptr, 'f'},
        {nullptr, 0, nullptr, 0} };

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

} // namespace Assessor
} // namespace ComplianceEngine
