// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_CETELEMETRY_H
#define COMPLIANCEENGINE_CETELEMETRY_H

#include "Result.h"

#include <chrono>
#include <cstdint>
#include <string>
#include <utility>

namespace ComplianceEngine
{
enum class TelemetryEventType
{
    Audit,
    Remediation
};

const char* ToString(const TelemetryEventType type) noexcept;

class TelemetryEvent
{
public:
    TelemetryEvent(TelemetryEventType type, std::string name)
        : mType(type),
          mName(std::move(name))
    {
    }

    TelemetryEventType Type() const noexcept
    {
        return mType;
    }

    const std::string& Name() const noexcept
    {
        return mName;
    }

    bool operator<(const TelemetryEvent& other) const noexcept
    {
        if (mType != other.mType)
        {
            return mType < other.mType;
        }

        return mName < other.mName;
    }

private:
    TelemetryEventType mType;
    std::string mName;
};

class TelemetryInterface
{
public:
    virtual ~TelemetryInterface() = 0;

private:
    virtual void LogEvent(const TelemetryEvent& event, int resultCode, int64_t durationUs, const std::chrono::system_clock::time_point& createdAt) noexcept = 0;
    template <typename F>
    friend auto RunWithTelemetry(const TelemetryEvent& event, TelemetryInterface& telemetry, F&& function) noexcept(noexcept(std::forward<F>(function)()))
        -> decltype(std::forward<F>(function)());
};

#ifdef BUILD_TELEMETRY

class Telemetry : public TelemetryInterface
{
public:
    explicit Telemetry(const int fd) noexcept;
    virtual ~Telemetry() noexcept;

    Telemetry(const Telemetry&) = delete;
    Telemetry& operator=(const Telemetry&) = delete;
    Telemetry(Telemetry&&) = delete;
    Telemetry& operator=(Telemetry&&) = delete;

private:
    void LogEvent(const TelemetryEvent& event, int resultCode, int64_t durationUs, const std::chrono::system_clock::time_point& createdAt) noexcept;
    template <typename F>
    friend auto RunWithTelemetry(const TelemetryEvent& event, TelemetryInterface& telemetry, F&& function) noexcept(noexcept(std::forward<F>(function)()))
        -> decltype(std::forward<F>(function)());
    int fd = -1;
};

template <typename F>
auto RunWithTelemetry(const TelemetryEvent& event, TelemetryInterface& telemetry, F&& function) noexcept(noexcept(std::forward<F>(function)()))
    -> decltype(std::forward<F>(function)())
{
    const auto createdAt = std::chrono::system_clock::now();
    const auto begin = std::chrono::steady_clock::now();

    auto&& result = std::forward<F>(function)();
    if (result.HasValue())
    {
        return std::forward<decltype(result)>(result);
    }

    const auto end = std::chrono::steady_clock::now();
    const auto durationUs = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
    telemetry.LogEvent(event, result.Error().code, durationUs, createdAt);
    return std::forward<decltype(result)>(result);
}
#else  // BUILD_TELEMETRY

class Telemetry : public TelemetryInterface
{
public:
    explicit Telemetry(const int fd) noexcept
    {
        (void)fd;
    }

    ~Telemetry() noexcept override = default;

    Telemetry(const Telemetry&) = delete;
    Telemetry& operator=(const Telemetry&) = delete;
    Telemetry(Telemetry&&) = delete;
    Telemetry& operator=(Telemetry&&) = delete;

private:
    void LogEvent(const TelemetryEvent& event, int resultCode, int64_t durationUs, const std::chrono::system_clock::time_point& createdAt) noexcept override
    {
        (void)event;
        (void)resultCode;
        (void)durationUs;
        (void)createdAt;
    }
    template <typename F>
    friend auto RunWithTelemetry(const TelemetryEvent& event, TelemetryInterface& telemetry, F&& function) noexcept(noexcept(std::forward<F>(function)()))
        -> decltype(std::forward<F>(function)());
};

template <typename F>
auto RunWithTelemetry(const TelemetryEvent& event, TelemetryInterface& telemetry, F&& function) noexcept(noexcept(std::forward<F>(function)()))
    -> decltype(std::forward<F>(function)())
{
    (void)event;
    (void)telemetry;
    return std::forward<F>(function)();
}
#endif // BUILD_TELEMETRY
} // namespace ComplianceEngine

#endif // COMPLIANCEENGINE_CETELEMETRY_H
