#include "telemetry.h"
#include "manager.h"

#define _CRT_SECURE_LOG_S
#include "mat.h"

static const char* config =
"{"
	"\"config\":{\"host\": \"*\"},"               // Attach as guest to any host
	"\"name\":\"C-API-Client-0\","                // Module ID
	"\"version\":\"1.0.0\","                      // Module semver
	"\"primaryToken\":\"" API_KEY "\","           // Primary Token
	"\"maxTeardownUploadTimeInSec\":5,"           // Allow up to 5 seconds for upload
	"\"hostMode\":false,"                         // Explicitly declare yourself as guest
	"\"minimumTraceLevel\":0,"                    // Debug printout level
	"\"sdkmode\":0"                               // 1DS direct-upload mode
"}";

void InitializeTelemetry()
{
	evt_prop event[] = TELEMETRY_EVENT
	(
		_STR("name",      "osconfig.test.c")  // Represents the uniquely qualified name for the event
	);

	evt_handle_t handle = evt_open(config);

	if (handle == 0)
	{
		fprintf(stderr, "Failed to open telemetry handle\n");
		return;
	}

	evt_log(handle, event);
	evt_flush(handle);
	evt_upload(handle);
	evt_close(handle);
}

void TelemetryEventWrite_CompletedBaseline(const char *targetName, const char *baselineName, const char *mode, double seconds)
{
	evt_prop event[] = TELEMETRY_EVENT
	(
		_STR("name", "osconfig.test.c.completedbaseline"),
		_STR("targetName", targetName),
		_STR("baselineName", baselineName),
		_STR("mode", mode),
		_DBL("seconds", seconds)
	);

	evt_handle_t handle = evt_open(config);

	if (handle == 0)
	{
		fprintf(stderr, "Failed to open telemetry handle\n");
		return;
	}

	evt_log(handle, event);
	evt_flush(handle);
	evt_upload(handle);
	evt_close(handle);
}

void ShutdownTelemetry()
{

}
