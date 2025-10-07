// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_ENGINE_H
#define COMPLIANCEENGINE_ENGINE_H

#include "ContextInterface.h"
#include "DistributionInfo.h"
#include "JsonWrapper.h"
#include "Logging.h"
#include "Mmi.h"
#include "MmiResults.h"
#include "Optional.h"
#include "Procedure.h"
#include "Result.h"

#include <Evaluator.h>
#include <map>
#include <memory>
#include <string>

struct json_object_t;

namespace ComplianceEngine
{
class Engine
{
private:
    unsigned int mMaxPayloadSize = 0;
    std::map<std::string, Procedure> mDatabase;
    std::unique_ptr<ContextInterface> mContext;
    std::unique_ptr<PayloadFormatter> mFormatter;
    Optional<DistributionInfo> mDistributionInfo;

    Optional<Error> SetProcedure(const std::string& ruleName, const std::string& payload);
    Optional<Error> InitAudit(const std::string& ruleName, const std::string& payload);
    Result<Status> ExecuteRemediation(const std::string& ruleName, const std::string& payload);

public:
    explicit Engine(std::unique_ptr<ContextInterface> context,
        std::unique_ptr<PayloadFormatter> payloadFormatter = std::unique_ptr<PayloadFormatter>(new DebugFormatter())) noexcept;
    ~Engine() = default;
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
    Engine(Engine&&) = delete;
    Engine& operator=(Engine&&) = delete;

    void SetMaxPayloadSize(unsigned int value) noexcept;
    unsigned int GetMaxPayloadSize() const noexcept;
    OsConfigLogHandle Log() const noexcept;

    Optional<Error> LoadDistributionInfo();
    const Optional<DistributionInfo>& GetDistributionInfo() const noexcept;

    static const char* GetModuleInfo() noexcept;

    Result<AuditResult> MmiGet(const char* objectName);
    Result<Status> MmiSet(const char* objectName, const std::string& payload);
};
} // namespace ComplianceEngine

#endif // COMPLIANCEENGINE_ENGINE_H
