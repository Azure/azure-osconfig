#include <JsonFormatter.hpp>
#include <cerrno>
#include <parson.h>
#include <sstream>

namespace ComplianceEngine
{
namespace BenchmarkFormatters
{
using std::string;
using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::steady_clock;
using std::chrono::system_clock;

namespace
{
const char* StatusToString(const Status status)
{
    switch (status)
    {
        case Status::Compliant:
            return "Compliant";
        case Status::NotApplicable:
            return "NotApplicable";
        case Status::NonCompliant:
        default:
            return "NonCompliant";
    }
}
} // anonymous namespace

Optional<Error> JsonFormatter::Begin(const Action action)
{
    auto json = JsonWrapper::MakeObject();
    if (!json.HasValue())
    {
        return Error("Failed to initialize JSON object", ENOMEM);
    }

    mJson = std::move(json.Value());
    auto* object = json_value_get_object(mJson.get());
    if (nullptr == object)
    {
        return Error("Failed to get JSON object", ENOMEM);
    }

    mBegin = std::chrono::steady_clock::now();

    if (JSONSuccess != json_object_set_string(object, "timestamp", ToISODatetime(system_clock::now()).c_str()))
    {
        return Error("Failed to set timestamp", ENOMEM);
    }

    if (JSONSuccess != json_object_set_string(object, "action", action == Action::Audit ? "Audit" : "Remediation"))
    {
        return Error("Failed to set action", ENOMEM);
    }

    // Record host provenance: the benchmark definitions are architecture-agnostic,
    // so the arch/distribution the scan actually ran on lives in the result for
    // multi-arch traceability. Host info is mandatory in the canonical result
    // (see assessor-result.schema.json); refuse to emit a schema-invalid result.
    if (!mHostInfo.HasValue())
    {
        return Error("Host info is required before Begin(); call SetHostInfo() first", EINVAL);
    }
    {
        auto* hostValue = json_value_init_object();
        if (nullptr == hostValue)
        {
            return Error("Failed to initialize host JSON object", ENOMEM);
        }
        auto* hostObject = json_value_get_object(hostValue);
        if (nullptr == hostObject || JSONSuccess != json_object_set_string(hostObject, "arch", mHostInfo->arch.c_str()) ||
            JSONSuccess != json_object_set_string(hostObject, "distribution", mHostInfo->distribution.c_str()) ||
            JSONSuccess != json_object_set_string(hostObject, "distributionVersion", mHostInfo->distributionVersion.c_str()))
        {
            json_value_free(hostValue);
            return Error("Failed to set host info", ENOMEM);
        }
        if (JSONSuccess != json_object_set_value(object, "host", hostValue))
        {
            json_value_free(hostValue);
            return Error("Failed to set host info", ENOMEM);
        }
    }

    auto* arrayValue = json_value_init_array();
    if (nullptr == arrayValue)
    {
        return Error("Failed to initialize JSON array", ENOMEM);
    }
    if (JSONSuccess != json_object_set_value(object, "rules", arrayValue))
    {
        json_value_free(arrayValue);
        return Error("Failed to set rules", ENOMEM);
    }

    return Optional<Error>();
}

Optional<Error> JsonFormatter::AddEntry(const MOF::Resource& entry, const Status status, const string& payload, const std::map<std::string, std::string>& parameters)
{
    auto resultWrapper = JsonWrapper::MakeObject();
    if (!resultWrapper.HasValue())
    {
        return Error("Failed to initialize JSON object", ENOMEM);
    }
    auto result = std::move(resultWrapper.Value());
    auto* object = json_value_get_object(result.get());
    if (nullptr == object)
    {
        return Error("Failed to get JSON object", ENOMEM);
    }

    auto* indicatorsValue = json_parse_string(payload.c_str());
    if (nullptr == indicatorsValue)
    {
        return Error("Failed to parse JSON payload", ENOMEM);
    }
    if (json_value_get_type(indicatorsValue) != JSONArray)
    {
        json_value_free(indicatorsValue);
        return Error("Invalid JSON payload", EINVAL);
    }

    if (JSONSuccess != json_object_set_value(object, "indicators", indicatorsValue))
    {
        json_value_free(indicatorsValue);
        return Error("Failed to set JSON payload", ENOMEM);
    }

    if (JSONSuccess != json_object_set_string(object, "title", entry.resourceID.c_str()))
    {
        return Error("Failed to set JSON title", ENOMEM);
    }

    if (JSONSuccess != json_object_set_string(object, "ruleId", entry.ruleId.c_str()))
    {
        return Error("Failed to set JSON ruleId", ENOMEM);
    }

    if (JSONSuccess != json_object_set_string(object, "section", entry.benchmarkInfo.section.c_str()))
    {
        return Error("Failed to set JSON payloadKey", ENOMEM);
    }

    if (JSONSuccess != json_object_set_string(object, "ruleName", entry.ruleName.c_str()))
    {
        return Error("Failed to set JSON ruleName", ENOMEM);
    }

    if (JSONSuccess != json_object_set_string(object, "status", StatusToString(status)))
    {
        return Error("Failed to set JSON status", ENOMEM);
    }

    // Surface the effective parameters (payload defaults merged with any user
    // overrides) so renderers can show them without decoding the procedure blob.
    auto* parametersValue = json_value_init_object();
    if (nullptr == parametersValue)
    {
        return Error("Failed to initialize parameters JSON object", ENOMEM);
    }
    auto* parametersObject = json_value_get_object(parametersValue);
    if (nullptr == parametersObject)
    {
        json_value_free(parametersValue);
        return Error("Failed to get parameters JSON object", ENOMEM);
    }
    for (const auto& parameter : parameters)
    {
        if (JSONSuccess != json_object_set_string(parametersObject, parameter.first.c_str(), parameter.second.c_str()))
        {
            json_value_free(parametersValue);
            return Error("Failed to set parameter value", ENOMEM);
        }
    }
    if (JSONSuccess != json_object_set_value(object, "parameters", parametersValue))
    {
        json_value_free(parametersValue);
        return Error("Failed to set parameters", ENOMEM);
    }

    object = json_value_get_object(mJson.get());
    if (nullptr == object)
    {
        return Error("Failed to get JSON object", ENOMEM);
    }
    auto* array = json_object_get_array(object, "rules");
    if (nullptr == array)
    {
        return Error("Failed to get JSON array", ENOMEM);
    }

    if (JSONSuccess != json_array_append_value(array, result.release()))
    {
        return Error("Failed to append JSON value", ENOMEM);
    }

    return Optional<Error>();
}

Result<string> JsonFormatter::Finish(ComplianceEngine::Status status)
{
    auto* object = json_value_get_object(mJson.get());
    if (nullptr == object)
    {
        return Error("Failed to get JSON object", ENOMEM);
    }

    if (JSONSuccess != json_object_set_number(object, "durationMs",
                           std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - mBegin).count()))
    {
        return Error("Failed to set JSON duration", ENOMEM);
    }

    if (JSONSuccess != json_object_set_string(object, "status", StatusToString(status)))
    {
        return Error("Failed to set JSON status", ENOMEM);
    }

    auto* serializedString = json_serialize_to_string_pretty(mJson.get());
    if (nullptr == serializedString)
    {
        return Error("Failed to serialize JSON string", ENOMEM);
    }

    string result(serializedString);
    json_free_serialized_string(serializedString);

    return result;
}
} // namespace BenchmarkFormatters
} // namespace ComplianceEngine
