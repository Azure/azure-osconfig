#include <iostream>
#include <string>
#include <fstream>
#include <cassert>
#include "mof.hpp"
#include <Logging.h>
#include <Engine.h>
#include <CommonContext.h>

using compliance::CommonContext;
using compliance::CompactListFormatter;
using compliance::Engine;
using compliance::JsonFormatter;
using compliance::NestedListFormatter;
using compliance::Optional;
using compliance::PayloadFormatter;
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

    enum class Format {
        NestedList,
        CompactList,
        Json,
        Undefined
    };

    struct Options
    {
        bool verbose = false;
        bool debug = false;
        Optional<string> logFile;
        Optional<Format> format;
        Command command = Command::Undefined;
        std::string input;
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

            if (arg == "--format")
            {
                if (i + 1 < argc)
                {
                    std::string formatArg = argv[++i];
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
            << "If <input> is absent, reads from stdin. Valid file extensions: .json, .mof.\n";
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

    std::unique_ptr<PayloadFormatter> formatter;
    if (options.format.HasValue())
    {
        switch (options.format.Value())
        {
        case Format::NestedList:
            formatter = std::unique_ptr<PayloadFormatter>(new NestedListFormatter());
            break;
        case Format::CompactList:
            formatter = std::unique_ptr<PayloadFormatter>(new CompactListFormatter());
            break;
        case Format::Json:
            formatter = std::unique_ptr<PayloadFormatter>(new JsonFormatter());
            break;
        default:
            std::cerr << "Invalid format specified.\n";
            return 1;
        }
    }

    auto logHandle = options.logFile.HasValue() ? OpenLog(options.logFile->c_str(), nullptr) : nullptr;
    if (options.verbose)
    {
        std::cout << "Verbose logging enabled.\n";
        SetLoggingLevel(LoggingLevel::LoggingLevelInformational);
    }

    if(options.debug)
    {
        SetLoggingLevel(LoggingLevel::LoggingLevelDebug);
    }

    if(nullptr != logHandle)
    {
        SetConsoleLoggingEnabled(false);
    }

    auto context = std::unique_ptr<CommonContext>(new CommonContext(logHandle));
    Engine engine(std::move(context), std::move(formatter));

    ifstream file;
    if(!options.input.empty())
    {
        file.open(options.input);
        if (!file.is_open())
        {
            std::cerr << "Failed to open file: " << options.input << "\n";
            CloseLog(&logHandle);
            return 1;
        }
    }

    istream& inputStream = options.input.empty() ? std::cin : file;
    string line;
    compliance::Status status = compliance::Status::Compliant;
    while(std::getline(inputStream, line))
    {
        if (line.find("instance of OsConfigResource as") == std::string::npos)
        {
            continue;
        }

        auto result = mof::ParseSingleEntry(inputStream);
        if (!result.HasValue())
        {
            std::cerr << "Failed to parse MOF entry: " << result.Error().message << "\n";
            CloseLog(&logHandle);
            return 1;
        }

        auto mofEntry = result.Value();
        if(mofEntry.procedure.HasValue())
        {
            auto result = engine.MmiSet((string("procedure") + mofEntry.ruleName).c_str(), mofEntry.procedure.Value());
            if (!result.HasValue())
            {
                std::cerr << "Failed to set procedure: " << result.Error().message << "\n";
                status = compliance::Status::NonCompliant;
                continue;
            }
        }

        switch (options.command)
        {
        case Command::Audit:
        {
            if(mofEntry.hasInitAudit)
            {
                auto result = engine.MmiSet((string("init") + mofEntry.ruleName).c_str(), mofEntry.payload);
                if (!result.HasValue())
                {
                    std::cerr << "Failed to init audit: " << result.Error().message << "\n";
                    status = compliance::Status::NonCompliant;
                    continue;
                }
            }

            auto result = engine.MmiGet((string("audit") + mofEntry.ruleName).c_str());
            if (!result.HasValue())
            {
                std::cerr << "Failed to perform audit: " << result.Error().message << "\n";
                status = compliance::Status::NonCompliant;
                continue;
            }

            if(result.Value().status == compliance::Status::Compliant)
            {
            }
            else
            {
            }

            break;
        }

        case Command::Remediate:
        {
            auto result = engine.MmiSet((string("remediate") + mofEntry.ruleName).c_str(), mofEntry.payload);
            if (!result.HasValue())
            {
                std::cerr << "Failed to set remediation: " << result.Error().message << "\n";
                status = compliance::Status::NonCompliant;
                continue;
            }
            break;
        }

        default:
            break;
        }
    }

    CloseLog(&logHandle);
    return status == compliance::Status::Compliant ? 0 : 1;
}
