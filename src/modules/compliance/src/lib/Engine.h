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
    OsConfigLogHandle mLog = nullptr;
    unsigned int mMaxPayloadSize = 0;
    std::map<std::string, Procedure> mDatabase;

    Result<JsonWrapper> decodeB64JSON(const char* input) const;
    Optional<Error> setProcedure(const std::string& ruleName, const char* payload, const int payloadSizeBytes);
    Optional<Error> initAudit(const std::string& ruleName, const char* payload, const int payloadSizeBytes);
    Result<bool> executeRemediation(const std::string& ruleName, const char* payload, const int payloadSizeBytes);

public:
    explicit Engine(OsConfigLogHandle log) noexcept;
    ~Engine() = default;

    void setMaxPayloadSize(unsigned int value) noexcept;
    unsigned int getMaxPayloadSize() const noexcept;
    OsConfigLogHandle log() const noexcept;

    static const char* getModuleInfo() noexcept;

    Result<AuditResult> mmiGet(const char* objectName);
    Result<bool> mmiSet(const char* objectName, const char* payload, const int payloadSizeBytes);
};
} // namespace compliance

#endif // COMPLIANCE_ENGINE_H
