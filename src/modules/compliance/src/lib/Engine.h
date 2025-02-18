// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCE_ENGINE_H
#define COMPLIANCE_ENGINE_H

#include "JsonWrapper.h"
#include "Logging.h"
#include "Mmi.h"
#include "Optional.h"
#include "Procedure.h"
#include "Result.h"

#include <map>
#include <memory>
#include <string>

struct json_object_t;

namespace compliance
{
class Engine
{
public:
    struct AuditResult
    {
        bool result = false;
        char* payload = nullptr;
        int payloadSize = 0;

        AuditResult() = default;
        ~AuditResult()
        {
            free(payload);
        }

        AuditResult(const AuditResult&) = delete;
        AuditResult(AuditResult&& other) noexcept
        {
            result = other.result;
            payload = other.payload;
            payloadSize = other.payloadSize;
            other.payload = nullptr;
        }

        AuditResult& operator=(const AuditResult&) = delete;
        AuditResult& operator=(AuditResult&& other) noexcept
        {
            if (this == &other)
            {
                return *this;
            }

            result = other.result;
            free(payload);
            payload = other.payload;
            payloadSize = other.payloadSize;
            other.payload = nullptr;
            return *this;
        }
    };

private:
    OSCONFIG_LOG_HANDLE mLog = nullptr;
    bool mLocalLog = false;
    unsigned int mMaxPayloadSize = 0;
    std::map<std::string, Procedure> mDatabase;

    Result<JsonWrapper> DecodeB64Json(const char* input) const;
    Optional<Error> SetProcedure(const std::string& ruleName, const char* payload, const int payloadSizeBytes);
    Optional<Error> InitAudit(const std::string& ruleName, const char* payload, const int payloadSizeBytes);
    Result<bool> ExecuteRemediation(const std::string& ruleName, const char* payload, const int payloadSizeBytes);

public:
    // Create engine with external log file
    Engine(void* log) noexcept;
    // Create engine with locally initialized log file
    Engine() noexcept;
    ~Engine();

    void SetMaxPayloadSize(unsigned int value) noexcept;
    unsigned int GetMaxPayloadSize() const noexcept;
    OSCONFIG_LOG_HANDLE Log() const noexcept;

    static const char* GetModuleInfo() noexcept;

    Result<AuditResult> MmiGet(const char* objectName);
    Result<bool> MmiSet(const char* objectName, const char* payload, const int payloadSizeBytes);
};
} // namespace compliance

#endif // COMPLIANCE_ENGINE_H
