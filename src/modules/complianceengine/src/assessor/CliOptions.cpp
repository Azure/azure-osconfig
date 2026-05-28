// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <CliOptions.hpp>
#include <algorithm>
#include <cstring>
#include <getopt.h>
#include <ostream>
#include <string>

namespace ComplianceEngine
{
namespace Assessor
{

using std::string;

namespace
{
constexpr const char* kDefaultProgramName = "compliance-engine-assessor";

const char* ResolveProgramName(const char* programName)
{
    if ((nullptr == programName) || ('\0' == programName[0]))
    {
        return kDefaultProgramName;
    }
    return programName;
}

// Reset getopt's global parser state so ParseCommandLine is safe to call
// repeatedly (notably from unit tests). On glibc, assigning `optind = 0`
// is the documented full-reset (it also re-scans the environment). BSD libc
// uses `optreset`; we set both when available for portability.
void ResetGetopt()
{
    optind = 0;
#ifdef optreset
    optreset = 1;
    optind = 1;
#endif
    opterr = 0; // suppress getopt's own stderr diagnostics; we report ourselves
}
} // anonymous namespace

void PrintHelp(std::ostream& out, const char* programName)
{
    const char* name = ResolveProgramName(programName);
    out << "Usage: " << name << " [options] <audit|remediate> [filename]\n\n";
    out << "Available options:\n";
    out << "\t-h, --help\t\tShow help and exit.\n";
    out << "\t-V, --version\t\tShow software version and exit.\n";
    out << "\t-v, --verbose\t\tRun in verbose mode.\n";
    out << "\t-d, --debug\t\tRun in debug mode.\n";
    out << "\t-L, --log-file FILE\tWrite log entries to FILE. Default: stderr.\n";
    out << "\t-s, --section SECTION\tProcess only entries whose section starts with SECTION.\n";
    out << "\t-f, --format FORMAT\tOutput format: nested-list, compact-list, json, debug.\n";
    out << "\n";
    out << "Positional arguments:\n";
    out << "\tcommand\t\tRun in audit or remediation mode. Allowed values: {audit|remediate}.\n";
    out << "\tfilename\tProcess the specified MOF file. If skipped or set to '-', reads stdin.\n";
}

Result<Options> ParseCommandLine(const int argc, char* argv[])
{
    if (argc <= 0 || nullptr == argv)
    {
        return Error("Invalid command line: argc must be positive and argv non-null.", EINVAL);
    }

    ResetGetopt();

    static constexpr const char* kShortOpts = "hVvdL:s:f:";
    static const option kLongOpts[] = {
        {"help", no_argument, nullptr, 'h'},
        {"version", no_argument, nullptr, 'V'},
        {"verbose", no_argument, nullptr, 'v'},
        {"debug", no_argument, nullptr, 'd'},
        {"log-file", required_argument, nullptr, 'L'},
        {"section", required_argument, nullptr, 's'},
        {"format", required_argument, nullptr, 'f'},
        {nullptr, 0, nullptr, 0},
    };

    Options result;
    bool seenLogFile = false;
    bool seenSection = false;
    bool seenFormat = false;

    int opt = ::getopt_long(argc, argv, kShortOpts, kLongOpts, nullptr);
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

            case 'L':
                if (seenLogFile)
                {
                    return Error("Option --log-file specified more than once.", EINVAL);
                }
                if (nullptr == optarg || '\0' == optarg[0])
                {
                    return Error("Option --log-file requires a non-empty value.", EINVAL);
                }
                result.logFile = string(optarg);
                seenLogFile = true;
                break;

            case 's':
                if (seenSection)
                {
                    return Error("Option --section specified more than once.", EINVAL);
                }
                if (nullptr == optarg || '\0' == optarg[0])
                {
                    return Error("Option --section requires a non-empty value.", EINVAL);
                }
                result.section = string(optarg);
                seenSection = true;
                break;

            case 'f': {
                if (seenFormat)
                {
                    return Error("Option --format specified more than once.", EINVAL);
                }
                if (nullptr == optarg || '\0' == optarg[0])
                {
                    return Error("Option --format requires a non-empty value.", EINVAL);
                }
                string formatArg(optarg);
                std::transform(formatArg.begin(), formatArg.end(), formatArg.begin(), [](unsigned char c) { return static_cast<char>(::tolower(c)); });
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
                    return Error("Invalid format: '" + string(optarg) + "'. Allowed: nested-list, compact-list, json, debug.", EINVAL);
                }
                seenFormat = true;
                break;
            }

            case '?':
            case ':':
            default: {
                // getopt has consumed the offending token; optopt holds the short option byte
                // when known. Build a stable message without leaking attacker-controlled bytes
                // beyond a single character.
                string message = "Unknown or malformed option";
                if (optopt != 0 && optopt >= 0x20 && optopt < 0x7f)
                {
                    message += string(": -") + static_cast<char>(optopt);
                }
                return Error(message + ".", EINVAL);
            }
        }

        opt = ::getopt_long(argc, argv, kShortOpts, kLongOpts, nullptr);
    }

    // Positional: command
    if (optind >= argc)
    {
        return Error("Missing required command: 'audit' or 'remediate'.", EINVAL);
    }

    {
        const string arg = argv[optind];
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
            return Error("Invalid command: '" + arg + "'. Must be 'audit' or 'remediate'.", EINVAL);
        }
        ++optind;
    }

    // Positional: input filename (optional)
    if (optind < argc)
    {
        result.input = argv[optind];
        ++optind;
    }

    if (optind < argc)
    {
        return Error("Too many arguments provided.", EINVAL);
    }

    return result;
}

} // namespace Assessor
} // namespace ComplianceEngine
