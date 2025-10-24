// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "main.h"
#include "Telemetry.h"

#include <Logging.h>
#include <Telemetry.hpp>

#include <string>

static OsConfigLogHandle g_log;
static const char* g_logFile = "/var/log/osconfig_telemetry.log";
static const char* g_rolledLogFile = "/var/log/osconfig_telemetry.bak";

void __attribute__((constructor)) Init(void)
{
    g_log = OpenLog(g_logFile, g_rolledLogFile);
#if defined(DEBUG) || defined(_DEBUG) || !defined(NDEBUG)
    SetLoggingLevel(LoggingLevelDebug);
#endif
    assert(NULL != g_log);
}

void __attribute__((destructor)) Destroy(void)
{
    CloseLog(&g_log);
}

int main(int argc, char* argv[])
{
    try
    {
        CommandLineArgs args;
        if (!ParseCommandLineArgs(argc, argv, args, g_log))
        {
            OsConfigLogError(g_log, "Error: Failed to parse command line arguments.");
            return 1;
        }

        std::string init_message = "Initializing telemetry with verbose=" + std::string(args.verbose ? "true" : "false");
        if (args.teardown_time != TELEMETRY_TIMEOUT_SECONDS) // Default teardown time
        {
            init_message += " and teardown_time=" + std::to_string(args.teardown_time) + "s";
        }
        OsConfigLogInfo(g_log, "%s", init_message.c_str());

        OsConfigLogInfo(g_log, "Telemetry initializing...");
        Telemetry::TelemetryManager telemetryManager(args.verbose, args.teardown_time, g_log);

        telemetryManager.ProcessJsonFile(args.filepath);
        OsConfigLogInfo(g_log, "Processed telemetry JSON file: %s", args.filepath.c_str());

        if (std::remove(args.filepath.c_str()) != 0)
        {
            OsConfigLogError(g_log, "Warning: Failed to delete JSON file: %s", args.filepath.c_str());
        }
    }
    catch (const std::exception& e)
    {
        OsConfigLogError(g_log, "Error: Telemetry operation failed: %s", e.what());
        return 1;
    }
    catch (...)
    {
        OsConfigLogError(g_log, "Error: Telemetry operation failed with unknown exception");
        return 1;
    }

    return 0;
}
