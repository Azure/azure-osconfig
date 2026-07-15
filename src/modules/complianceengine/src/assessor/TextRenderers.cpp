// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <TextRenderers.hpp>
#include <cerrno>
#include <parson.h>
#include <sstream>
#include <string>

namespace ComplianceEngine
{
namespace Assessor
{
using std::string;

namespace
{
string SafeString(const char* s)
{
    return (nullptr == s) ? string() : string(s);
}

// Recursively appends an indicator tree as indented "<label> [<status>]" lines,
// starting at `baseIndent` spaces and adding two per depth level. A node's label
// is its message (leaf) or its procedure (branch).
void AppendIndicators(const JSON_Array* indicators, size_t depth, size_t baseIndent, std::ostringstream& out)
{
    if (nullptr == indicators)
    {
        return;
    }
    const size_t count = json_array_get_count(indicators);
    for (size_t i = 0; i < count; ++i)
    {
        const JSON_Object* node = json_array_get_object(indicators, i);
        if (nullptr == node)
        {
            continue;
        }
        string label = SafeString(json_object_get_string(node, "message"));
        if (label.empty())
        {
            label = SafeString(json_object_get_string(node, "procedure"));
        }
        const string status = SafeString(json_object_get_string(node, "status"));
        out << string(baseIndent + depth * 2, ' ') << "- " << label;
        if (!status.empty())
        {
            out << " [" << status << "]";
        }
        out << "\n";
        AppendIndicators(json_object_get_array(node, "indicators"), depth + 1, baseIndent, out);
    }
}

string JoinParameters(const JSON_Object* rule)
{
    const JSON_Object* parameters = json_object_get_object(rule, "parameters");
    if (nullptr == parameters)
    {
        return string();
    }
    std::ostringstream out;
    const size_t count = json_object_get_count(parameters);
    for (size_t i = 0; i < count; ++i)
    {
        if (i != 0)
        {
            out << ", ";
        }
        out << SafeString(json_object_get_name(parameters, i)) << "=" << SafeString(json_value_get_string(json_object_get_value_at(parameters, i)));
    }
    return out.str();
}
} // anonymous namespace

Result<string> RenderText(const string& canonicalJson, const TextStyle style)
{
    JSON_Value* root = json_parse_string(canonicalJson.c_str());
    if (nullptr == root)
    {
        return Error("Failed to parse canonical result JSON", EINVAL);
    }
    struct RootGuard
    {
        JSON_Value* v;
        ~RootGuard()
        {
            json_value_free(v);
        }
    } guard{root};

    const JSON_Object* rootObject = json_value_get_object(root);
    if (nullptr == rootObject)
    {
        return Error("Canonical result JSON is not an object", EINVAL);
    }
    const JSON_Array* rules = json_object_get_array(rootObject, "rules");
    if (nullptr == rules)
    {
        return Error("Canonical result JSON has no 'rules' array", EINVAL);
    }

    std::ostringstream out;
    out << "Action: " << SafeString(json_object_get_string(rootObject, "action")) << "\n";
    out << "Timestamp: " << SafeString(json_object_get_string(rootObject, "timestamp")) << "\n";
    out << "Rules:\n";

    const size_t ruleCount = json_array_get_count(rules);
    for (size_t i = 0; i < ruleCount; ++i)
    {
        const JSON_Object* rule = json_array_get_object(rules, i);
        if (nullptr == rule)
        {
            return Error("Canonical result JSON 'rules' entry is not an object", EINVAL);
        }
        const string section = SafeString(json_object_get_string(rule, "section"));
        const string ruleName = SafeString(json_object_get_string(rule, "ruleName"));
        const string status = SafeString(json_object_get_string(rule, "status"));

        switch (style)
        {
            case TextStyle::CompactList:
                out << "  [" << status << "] " << section << " " << ruleName << "\n";
                break;

            case TextStyle::NestedList:
                out << "  " << section << " " << ruleName << " [" << status << "]\n";
                AppendIndicators(json_object_get_array(rule, "indicators"), 0, 4, out);
                break;

            case TextStyle::Debug: {
                out << "  " << section << " " << ruleName << " (ruleId=" << SafeString(json_object_get_string(rule, "ruleId")) << ") [" << status << "]\n";
                out << "    title: " << SafeString(json_object_get_string(rule, "title")) << "\n";
                const string parameters = JoinParameters(rule);
                if (!parameters.empty())
                {
                    out << "    parameters: " << parameters << "\n";
                }
                AppendIndicators(json_object_get_array(rule, "indicators"), 0, 4, out);
                break;
            }
        }
    }

    out << "Duration: " << static_cast<long long>(json_object_get_number(rootObject, "durationMs")) << " ms\n";
    out << "Status: " << SafeString(json_object_get_string(rootObject, "status")) << "\n";
    out << "End of Report\n";
    return out.str();
}

} // namespace Assessor
} // namespace ComplianceEngine
