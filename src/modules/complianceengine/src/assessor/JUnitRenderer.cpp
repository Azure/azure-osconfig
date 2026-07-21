// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <JUnitRenderer.hpp>
#include <StringTools.h>
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
// Escapes the five XML entities and neutralises control characters that are
// illegal in XML 1.0 text (everything below 0x20 except tab/newline/carriage
// return), so arbitrary indicator/parameter text cannot produce malformed XML.
string EscapeXml(const string& in)
{
    string out;
    out.reserve(in.size());
    for (const char ch : in)
    {
        switch (ch)
        {
            case '&':
                out += "&amp;";
                break;
            case '<':
                out += "&lt;";
                break;
            case '>':
                out += "&gt;";
                break;
            case '"':
                out += "&quot;";
                break;
            case '\'':
                out += "&apos;";
                break;
            default:
                if (static_cast<unsigned char>(ch) < 0x20 && ch != '\t' && ch != '\n' && ch != '\r')
                {
                    out += ' ';
                }
                else
                {
                    out += ch;
                }
                break;
        }
    }
    return out;
}

// Recursively renders an indicators array into the readable indented style used
// by tests/reporting/junit.py: "{indent*depth}  - {label} [{status}]". A node's
// label is its message (leaf) or its procedure (branch); children are rendered
// one level deeper.
//
// Recursion is bounded to guard against pathologically deep (or maliciously
// crafted) indicator trees causing stack exhaustion; nodes below the limit are
// silently dropped from the rendered output.
constexpr size_t cMaxIndicatorDepth = 16;

void AppendIndicators(const JSON_Array* indicators, size_t depth, std::ostringstream& body)
{
    if (nullptr == indicators || depth >= cMaxIndicatorDepth)
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
        string label = StringOrEmpty(json_object_get_string(node, "message"));
        if (label.empty())
        {
            label = StringOrEmpty(json_object_get_string(node, "procedure"));
        }
        const string status = StringOrEmpty(json_object_get_string(node, "status"));
        body << string(depth * 2, ' ') << "  - " << label;
        if (!status.empty())
        {
            body << " [" << status << "]";
        }
        body << "\n";
        AppendIndicators(json_object_get_array(node, "indicators"), depth + 1, body);
    }
}

// Builds the human-readable failure body for a rule: a Parameters section
// (present when the canonical JSON carries per-rule parameters) followed by an
// indented Indicators tree.
string BuildBody(const JSON_Object* rule)
{
    std::ostringstream body;

    const JSON_Object* parameters = json_object_get_object(rule, "parameters");
    body << "Parameters:\n";
    if (nullptr != parameters)
    {
        const size_t count = json_object_get_count(parameters);
        for (size_t i = 0; i < count; ++i)
        {
            const string key = StringOrEmpty(json_object_get_name(parameters, i));
            const JSON_Value* value = json_object_get_value_at(parameters, i);
            string valueStr = StringOrEmpty(json_value_get_string(value));
            if (valueStr.empty() && nullptr != value && json_value_get_type(value) != JSONString)
            {
                char* serialized = json_serialize_to_string(value);
                if (nullptr != serialized)
                {
                    // Deliberately deep-copy into our own std::string before
                    // freeing parson's buffer, so the value survives the free.
                    valueStr = std::string(serialized);
                    json_free_serialized_string(serialized);
                }
            }
            body << "  - " << key << ": " << valueStr << "\n";
        }
    }

    body << "\nIndicators:\n";
    AppendIndicators(json_object_get_array(rule, "indicators"), 0, body);
    return body.str();
}
} // anonymous namespace

Result<string> RenderJUnit(const string& canonicalJson, const string& suiteName)
{
    JSON_Value* root = json_parse_string(canonicalJson.c_str());
    if (nullptr == root)
    {
        return Error("Failed to parse canonical result JSON", EINVAL);
    }
    // Own the parsed document for the duration of this function.
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

    const size_t ruleCount = json_array_get_count(rules);
    size_t failureCount = 0;
    size_t skippedCount = 0;
    std::ostringstream cases;
    for (size_t i = 0; i < ruleCount; ++i)
    {
        const JSON_Object* rule = json_array_get_object(rules, i);
        if (nullptr == rule)
        {
            return Error("Canonical result JSON 'rules' entry is not an object", EINVAL);
        }
        const string section = StringOrEmpty(json_object_get_string(rule, "section"));
        const string ruleName = StringOrEmpty(json_object_get_string(rule, "ruleName"));
        const string status = StringOrEmpty(json_object_get_string(rule, "status"));

        // Guard against schema drift / upstream bugs: an unrecognised or missing
        // status must not be silently rendered as a passing test case.
        if (status != "Compliant" && status != "NonCompliant" && status != "NotApplicable")
        {
            return Error("Canonical result JSON rule has invalid 'status' value: '" + status + "'", EINVAL);
        }

        cases << "  <testcase classname=\"" << EscapeXml(section) << "\" name=\"" << EscapeXml(ruleName) << "\"";
        if (status == "NonCompliant")
        {
            ++failureCount;
            cases << ">\n";
            cases << "    <failure message=\"Rule is non-compliant\" type=\"NonCompliant\">" << EscapeXml(BuildBody(rule)) << "</failure>\n";
            cases << "  </testcase>\n";
        }
        else if (status == "NotApplicable")
        {
            // A not-applicable rule is neither a pass nor a failure; JUnit models
            // this as a skipped test case.
            ++skippedCount;
            cases << ">\n";
            cases << "    <skipped message=\"Rule is not applicable\">" << EscapeXml(BuildBody(rule)) << "</skipped>\n";
            cases << "  </testcase>\n";
        }
        else
        {
            // status == "Compliant": a bare passing test case.
            cases << "/>\n";
        }
    }

    std::ostringstream out;
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    out << "<testsuites>\n";
    out << "  <testsuite name=\"" << EscapeXml(suiteName) << "\" tests=\"" << ruleCount << "\" failures=\"" << failureCount << "\" skipped=\""
        << skippedCount << "\">\n";
    out << cases.str();
    out << "  </testsuite>\n";
    out << "</testsuites>\n";
    return out.str();
}

} // namespace Assessor
} // namespace ComplianceEngine
