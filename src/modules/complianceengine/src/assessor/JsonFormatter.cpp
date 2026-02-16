#include <JsonFormatter.hpp>
#include <parson.h>
#include <sstream>
#include <version.h>

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
Result<JsonWrapper> GetModuleInfo()
{
    return JsonWrapper::FromString(Engine::GetModuleInfo());
}
} // anonymous namespace

Optional<Error> JsonFormatter::Begin(const Action action)
{
    auto json = JsonWrapper::FromRawValue(json_value_init_object());
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

    if (JSONSuccess != json_object_set_string(object, "osconfigVersion", OSCONFIG_VERSION))
    {
        return Error("Failed to set OsConfig version", ENOMEM);
    }

    auto moduleInfo = GetModuleInfo();
    if (!moduleInfo.HasValue())
    {
        return moduleInfo.Error();
    }

    if (JSONSuccess != json_object_set_value(object, "module", moduleInfo.Value().release()))
    {
        return Error("Failed to set module info", ENOMEM);
    }

    if (JSONSuccess != json_object_set_string(object, "timestamp", ToISODatetime(system_clock::now()).c_str()))
    {
        return Error("Failed to set timestamp", ENOMEM);
    }

    if (JSONSuccess != json_object_set_string(object, "action", action == Action::Audit ? "Audit" : "Remediation"))
    {
        return Error("Failed to set action", ENOMEM);
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

Optional<Error> JsonFormatter::AddEntry(const MOF::Resource& entry, const Status status, const string& payload)
{
    auto resultWrapper = JsonWrapper::FromRawValue(json_value_init_object());
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

    if (JSONSuccess != json_object_set_string(object, "resourceID", entry.resourceID.c_str()))
    {
        return Error("Failed to set JSON resourceID", ENOMEM);
    }

    if (JSONSuccess != json_object_set_string(object, "section", entry.benchmarkInfo.section.c_str()))
    {
        return Error("Failed to set JSON payloadKey", ENOMEM);
    }

    if (JSONSuccess != json_object_set_string(object, "ruleName", entry.ruleName.c_str()))
    {
        return Error("Failed to set JSON ruleName", ENOMEM);
    }

    if (JSONSuccess != json_object_set_string(object, "status", status == ComplianceEngine::Status::Compliant ? "Compliant" : "NonCompliant"))
    {
        return Error("Failed to set JSON status", ENOMEM);
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

    if (JSONSuccess != json_object_set_string(object, "status", status == ComplianceEngine::Status::Compliant ? "Compliant" : "NonCompliant"))
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
