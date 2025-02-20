// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCE_ENGINE_H
#define COMPLIANCE_ENGINE_H

#include <Mmi.h>
#include <cstdio>
#include "Logging.h"

#include "Result.h"
#include "Optional.h"
#include "Procedure.h"
#include "JsonWrapper.h"

#include <memory>
#include <string>
#include <map>

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

        Result<JsonWrapper> decodeB64JSON(const char* input) const;
        Optional<Error> setProcedure(const std::string& ruleName, const char* payload, const int payloadSizeBytes);
        Optional<Error> initAudit(const std::string& ruleName, const char* payload, const int payloadSizeBytes);
        Result<bool> executeRemediation(const std::string& ruleName, const char* payload, const int payloadSizeBytes);
    public:
        // Create engine with external log file
        Engine(void* log) noexcept;
        // Create engine with locally initialized log file
        Engine() noexcept;
        ~Engine();

        void setMaxPayloadSize(unsigned int value) noexcept;
        unsigned int getMaxPayloadSize() const noexcept;
        OSCONFIG_LOG_HANDLE log() const noexcept;

        static const char* getModuleInfo() noexcept;

        Result<AuditResult> mmiGet(const char* objectName);
        Result<bool> mmiSet(const char* objectName, const char* payload, const int payloadSizeBytes);
    };
}

#endif // COMPLIANCE_ENGINE_H
