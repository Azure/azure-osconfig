// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCE_ENGINE_H
#define COMPLIANCE_ENGINE_H

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
    OsConfigLogHandle mLog = nullptr;
    unsigned int mMaxPayloadSize = 0;
    std::map<std::string, Procedure> mDatabase;

    Result<JsonWrapper> decodeB64JSON(const std::string& input) const;
    Optional<Error> setProcedure(const std::string& ruleName, const std::string& payload);
    Optional<Error> initAudit(const std::string& ruleName, const std::string& payload);
    Result<Status> executeRemediation(const std::string& ruleName, const std::string& payload);

public:
    explicit Engine(OsConfigLogHandle log) noexcept;
    ~Engine() = default;

    void setMaxPayloadSize(unsigned int value) noexcept;
    unsigned int getMaxPayloadSize() const noexcept;
    OsConfigLogHandle log() const noexcept;

    static const char* getModuleInfo() noexcept;

    Result<AuditResult> mmiGet(const char* objectName);
    Result<Status> mmiSet(const char* objectName, const std::string& payload);
};
} // namespace compliance

#endif // COMPLIANCE_ENGINE_H
