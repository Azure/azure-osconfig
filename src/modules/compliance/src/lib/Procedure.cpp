// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Procedure.h"

#include "Logging.h"

#include <cassert>
#include <parson.h>

namespace compliance
{
namespace
{
std::size_t SkipSpaces(const std::string& input, std::size_t pos)
{
    while ((pos < input.size()) && (isspace(input[pos])))
    {
        ++pos;
    }
    return pos;
}

Result<std::size_t> ParseKey(const std::string& input, std::size_t pos)
{
    bool first = true;
    while ((pos < input.size()) && (!isspace(input[pos])) && (input[pos] != '='))
    {
        if ((!isalnum(input[pos])) && (input[pos] != '_'))
        {
            return Error("Invalid key: only alphanumeric and underscore characters are allowed");
        }

        if (first && isdigit(input[pos]))
        {
            return Error("Invalid key: first character must not be a digit");
        }
        first = false;
        ++pos;
    }

    return pos;
}

std::size_t ParseQuotedValue(const std::string& input, std::size_t pos, std::string& value)
{
    // Assuming the first character is a quote
    // pos is always > 1 as we are passing value which must follow a valid key, assignment character and a quote
    assert(pos > 1);
    assert(input[pos] == '"');

    while (++pos < input.size())
    {
        if (input[pos] == '\\')
        {
            if (++pos >= input.size())
            {
                // Found a backslash at the end of the string, the input is invalid
                return pos;
            }

            // Found a backslash, check if it is escaped
            if (input[pos] == '\\')
            {
                // Escaped backslash, add it to the value
                value += '\\';
                continue;
            }
            else if (input[pos] == '"')
            {
                // Escaped quote, add it to the value
                value += '"';
                continue;
            }

            // Found a backslash with invalid escaped character, treat this as error
            OsConfigLogInfo(nullptr, "Invalid escape sequence: %c", input[pos]);
            break;
        }

        if (input[pos] == '"')
        {
            return pos + 1;
        }

        // Found an escaped quote, add it to the value, but in place of the last backslash
        value += input[pos];
    }

    return pos;
}
} // namespace

void Procedure::SetParameter(const std::string& key, std::string value)
{
    mParameters[key] = std::move(value);
}

Optional<Error> Procedure::UpdateUserParameters(const std::string& input)
{
    size_t pos = 0;
    while (pos < input.size())
    {
        // Skip spaces
        pos = SkipSpaces(input, pos);
        if (pos >= input.size())
        {
            break;
        }

        // Parse key
        const size_t keyStart = pos;
        auto result = ParseKey(input, pos);
        if (!result.HasValue())
        {
            return result.Error();
        }

        auto key = input.substr(keyStart, result.Value() - keyStart);
        pos = SkipSpaces(input, result.Value());
        if ((pos >= input.size()) || (input[pos] != '='))
        {
            return Error("Invalid key-value pair: '=' expected");
        }

        // Skip space after assignment character
        pos = SkipSpaces(input, pos + 1);
        if (pos >= input.size())
        {
            return Error("Invalid key-value pair: value expected");
        }

        // Parse value
        std::string value;
        if (input[pos] == '"')
        {
            const size_t valueStart = pos;
            pos = ParseQuotedValue(input, pos, value);
            if (input[pos - 1] != '"')
            {
                return Error("Invalid key-value pair: missing closing quote or invalid escape sequence");
            }
            if ((pos >= input.size()) && (pos - valueStart == 1))
            {
                return Error("Invalid key-value pair: missing closing quote at the end of the input");
            }
        }
        else
        {
            // Unquoted value
            const size_t valueStart = pos;
            while (pos < input.size() && !isspace(input[pos]))
            {
                pos++;
            }
            value = input.substr(valueStart, pos - valueStart);
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

Optional<Error> Procedure::SetAudit(const json_value_t* rule)
{
    if (mAuditRule != nullptr)
    {
        return Error("Audit rule already set");
    }

    mAuditRule.reset(json_value_deep_copy(rule));
    return Optional<Error>();
}

const JSON_Object* Procedure::Audit() const noexcept
{
    if (mAuditRule == nullptr)
    {
        return nullptr;
    }
    return json_value_get_object(mAuditRule.get());
}

Optional<Error> Procedure::SetRemediation(const json_value_t* rule)
{
    if (mRemediationRule != nullptr)
    {
        return Error("Remediation rule already set");
    }

    mRemediationRule.reset(json_value_deep_copy(rule));
    return Optional<Error>();
}

const JSON_Object* Procedure::Remediation() const noexcept
{
    if (mRemediationRule == nullptr)
    {
        return nullptr;
    }
    return json_value_get_object(mRemediationRule.get());
}
} // namespace compliance
