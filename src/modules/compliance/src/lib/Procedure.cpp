// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Procedure.h"
#include "Logging.h"

#include <parson.h>
#include <sstream>

namespace compliance
{
    void Procedure::setParameter(const std::string& key, std::string value)
    {
        mParameters[key] = std::move(value);
    }

    Optional<Error> Procedure::updateUserParameters(const std::string &input)
    {
        std::istringstream stream(input);
        std::string token;

        while (std::getline(stream, token, ' '))
        {
            // In the non-NRP scenario the token is delimited by double quotes
            if (token.size() >= 2 && token.front() == '"' && token.back() == '"')
            {
                token = token.substr(1, token.size() - 2);
            }

            size_t pos = token.find('=');
            if (pos == std::string::npos)
            {
                continue;
            }

            std::string key = token.substr(0, pos);
            std::string value = token.substr(pos + 1);

            if (!value.empty() && value[0] == '"')
            {
                value.erase(0, 1);
                while (!value.empty() && value.back() == '\\')
                {
                    std::string nextToken;
                    if (std::getline(stream, nextToken, ' '))
                    {
                        value.pop_back();
                        value += ' ' + nextToken;
                    }
                    else
                    {
                        break;
                    }
                }
                if (!value.empty() && value.back() == '"')
                {
                    value.pop_back();
                }
            }

            auto it = mParameters.find(key);
            if (it == mParameters.end())
            {
                return Error(std::string("User parameter '") + key + std::string("' not found"));
            }

            it->second = value;
        }

        return Optional<Error>();
    }

    Optional<Error> Procedure::setAudit(const json_value_t* rule)
    {
        if(mAuditRule != nullptr)
        {
            return Error("Audit rule already set");
        }

        mAuditRule.reset(json_value_deep_copy(rule));
        return Optional<Error>();
    }

    const JSON_Object* Procedure::audit() const noexcept {
        if( mAuditRule == nullptr )
        {
            return nullptr;
        }
        return json_value_get_object(mAuditRule.get());
    }

    Optional<Error> Procedure::setRemediation(const json_value_t* rule)
    {
        if (mRemediationRule != nullptr)
        {
            return Error("Remediation rule already set");
        }

        mRemediationRule.reset(json_value_deep_copy(rule));
        return Optional<Error>();
    }

    const JSON_Object* Procedure::remediation() const noexcept {
        if (mRemediationRule == nullptr)
        {
            return nullptr;
        }
        return json_value_get_object(mRemediationRule.get());
    }
} // namespace compliance
