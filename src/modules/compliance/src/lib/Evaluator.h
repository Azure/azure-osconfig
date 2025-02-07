// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCE_EVALUATOR_H
#define COMPLIANCE_EVALUATOR_H

#include <map>
#include <string>
#include <sstream>
#include "Result.h"
#include "Logging.h"

struct json_object_t;

#define AUDIT_FN(fn_name) \
  ::compliance::Result<bool> Audit_##fn_name(std::map<std::string, std::string> args, std::ostringstream &logstream)

#define REMEDIATE_FN(fn_name) \
  ::compliance::Result<bool> Remediate_##fn_name(std::map<std::string, std::string> args, std::ostringstream &logstream)

namespace compliance
{
    typedef Result<bool> (*action_func_t)(std::map<std::string, std::string> args, std::ostringstream &logstream);
    class Evaluator
    {
    public:
        Evaluator(const struct json_object_t *json, const std::map<std::string, std::string> &parameters, OSCONFIG_LOG_HANDLE log) : mJson(json), mParameters(parameters), mLog(log) {}
        Result<bool> ExecuteAudit(char **payload, int *payloadSizeBytes);
        Result<bool> ExecuteRemediation();

        void setProcedureMap(std::map<std::string, std::pair<action_func_t, action_func_t>> procedureMap);
    private:
        enum class Action
        {
            Audit,
            Remediate
        };
        Result<bool> EvaluateProcedure(const struct json_object_t *json, const Action action);
        const struct json_object_t *mJson;
        const std::map<std::string, std::string> &mParameters;
        std::ostringstream mLogstream;
        void *mLog;
        // autogenerated, instantiated in ProcedureMap.cpp
        static std::map<std::string, std::pair<action_func_t, action_func_t>> mProcedureMap;
        static const size_t cLogstreamMaxSize = 4096;
    };
}

#endif // COMPLIANCE_EVALUATOR_H
