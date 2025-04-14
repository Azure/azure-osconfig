// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCE_ENGINE_H
#define COMPLIANCE_ENGINE_H

#include "ContextInterface.h"
#include "JsonWrapper.h"
#include "Logging.h"
#include "Mmi.h"
#include "MmiResults.h"
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
private:
    unsigned int mMaxPayloadSize = 0;
    std::map<std::string, Procedure> mDatabase;
    std::unique_ptr<ContextInterface> mContext;

    Result<JsonWrapper> DecodeB64Json(const std::string& input) const;
    Optional<Error> SetProcedure(const std::string& ruleName, const std::string& payload);
    Optional<Error> InitAudit(const std::string& ruleName, const std::string& payload);
    Result<Status> ExecuteRemediation(const std::string& ruleName, const std::string& payload);

public:
    explicit Engine(std::unique_ptr<ContextInterface> context) noexcept;
    ~Engine() = default;
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
    Engine(Engine&&) = delete;
    Engine& operator=(Engine&&) = delete;

    void SetMaxPayloadSize(unsigned int value) noexcept;
    unsigned int GetMaxPayloadSize() const noexcept;
    OsConfigLogHandle Log() const noexcept;

    static const char* GetModuleInfo() noexcept;

    Result<AuditResult> MmiGet(const char* objectName);
    Result<Status> MmiSet(const char* objectName, const std::string& payload);
};
} // namespace compliance

#endif // COMPLIANCE_ENGINE_H
