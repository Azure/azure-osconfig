// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "CeTelemetry.h"
#include "MmiResults.h"
#include "Result.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <map>

using ComplianceEngine::AuditResult;
using ComplianceEngine::Error;
using ComplianceEngine::Result;
using ComplianceEngine::Status;
using ComplianceEngine::TelemetryEvent;
using ComplianceEngine::TelemetryEventType;

struct CapturedEvent
{
    TelemetryEventType type;
    int resultCode;
    int64_t durationUs;
    std::chrono::system_clock::time_point createdAt;
};

class MockTelemetry : public ComplianceEngine::TelemetryInterface
{
public:
    explicit MockTelemetry() = default;

    ~MockTelemetry() noexcept = default;

    MockTelemetry(const MockTelemetry&) = delete;
    MockTelemetry& operator=(const MockTelemetry&) = delete;
    MockTelemetry(MockTelemetry&&) = delete;
    MockTelemetry& operator=(MockTelemetry&&) = delete;

    void LogEvent(const TelemetryEvent& event, int resultCode, int64_t durationUs, const std::chrono::system_clock::time_point& createdAt) noexcept
    {
        mCapturedEvents[event] = CapturedEvent{event.Type(), resultCode, durationUs, createdAt};
    }

    std::map<TelemetryEvent, CapturedEvent> mCapturedEvents;
};

class TelemetryTest : public ::testing::Test
{
};

TEST_F(TelemetryTest, RunInTelemetry_WithIntSuccess_NoEventLogged)
{
    MockTelemetry mockTelemetry;
    EXPECT_EQ(mockTelemetry.mCapturedEvents.size(), 0);
}

TEST_F(TelemetryTest, TelemetryEvent_RunWithTelemetry)
{
    MockTelemetry mockTelemetry;
    auto mockEvent = TelemetryEvent(TelemetryEventType::Audit, "FooBar");
    auto result = RunWithTelemetry(mockEvent, mockTelemetry, [&]() { return Result<AuditResult>(Error("System is wrong")); });
    EXPECT_EQ(result.HasValue(), false);
    EXPECT_EQ(mockTelemetry.mCapturedEvents.size(), 1);
    EXPECT_EQ(mockTelemetry.mCapturedEvents[mockEvent].type, TelemetryEventType::Audit);
    // The default resultCode for Error if only Error(message) is given
    EXPECT_EQ(mockTelemetry.mCapturedEvents[mockEvent].resultCode, -1);
    EXPECT_EQ(result.Error().message, std::string("System is wrong"));
}
TEST_F(TelemetryTest, TelemetryEvent_RunWithTelemetryNoEvents)
{
    MockTelemetry mockTelemetry;
    auto mockEvent = TelemetryEvent(TelemetryEventType::Audit, "FooBar");
    auto result = RunWithTelemetry(mockEvent, mockTelemetry, [&]() { return Result<AuditResult>(AuditResult{Status::Compliant, "Horay"}); });
    EXPECT_EQ(result.HasValue(), true);
    EXPECT_EQ(mockTelemetry.mCapturedEvents.size(), 0);
    EXPECT_EQ(result.Value().payload, std::string("Horay"));
}
