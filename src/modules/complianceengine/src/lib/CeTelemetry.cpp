// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "CeTelemetry.h"

#ifdef BUILD_TELEMETRY
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <utility>
#endif // BUILD_TELEMETRY

namespace ComplianceEngine
{
TelemetryInterface::~TelemetryInterface() = default;

const char* ToString(const TelemetryEventType type) noexcept
{
    switch (type)
    {
        case TelemetryEventType::Audit:
            return "audit";
        case TelemetryEventType::Remediation:
            return "remediation";
    }
    return "unknown";
}

#ifdef BUILD_TELEMETRY

namespace
{
static int64_t ToEpochMicroseconds(const std::chrono::system_clock::time_point& timestamp) noexcept
{
    return std::chrono::duration_cast<std::chrono::microseconds>(timestamp.time_since_epoch()).count();
}

} // namespace

Telemetry::Telemetry(const int fd) noexcept
    : fd(fd)
{
}

Telemetry::~Telemetry() noexcept
{
    if (0 <= fd)
    {
        close(fd);
    }
}

void Telemetry::LogEvent(const TelemetryEvent& event, int resultCode, int64_t durationUs, const std::chrono::system_clock::time_point& createdAt) noexcept
{
    if (0 > fd)
    {
        return;
    }

    const int64_t createdAtUs = ToEpochMicroseconds(createdAt);
    const int64_t completedAtUs = ToEpochMicroseconds(std::chrono::system_clock::now());

    dprintf(fd, "{\"EventName\":\"%s\",\"name\":\"%s\",\"resultCode\":%d,\"createdAtUs\":%lld,\"completedAtUs\":%lld,\"durationUs\":%lld}\n",
        ToString(event.Type()), event.Name().c_str(), resultCode, static_cast<long long>(createdAtUs), static_cast<long long>(completedAtUs),
        static_cast<long long>(durationUs));
}

#endif // BUILD_TELEMETRY
} // namespace ComplianceEngine
