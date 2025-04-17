#include <JsonFormatter.hpp>
#include <parson.h>
#include <sstream>
#include <version.h>

namespace formatters
{
using compliance::Engine;
using compliance::Error;
using compliance::Evaluator;
using compliance::JsonWrapper;
using compliance::JsonWrapperDeleter;
using compliance::Optional;
using compliance::Result;
using compliance::Status;
using std::string;
using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::steady_clock;
using std::chrono::system_clock;

namespace
{
    Result<JsonWrapper> GetModuleInfo()
    {
        auto* value = json_parse_string(Engine::GetModuleInfo());
        if (nullptr == value)
        {
            return Error("Failed to parse JSON string", EINVAL);
        }

        return JsonWrapper(value, JsonWrapperDeleter());
    }
} // anonymous namespace

Optional<Error> JsonFormatter::Begin(Evaluator::Action action)
{
    auto* value = json_value_init_object();
    if (nullptr == value)
    {
        return Error("Failed to initialize JSON object", ENOMEM);
    }

    mJson = JsonWrapper(value, JsonWrapperDeleter());
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

    if (JSONSuccess != json_object_set_value(object, "module", moduleInfo.Value().get()))
    {
        return Error("Failed to set module info", ENOMEM);
    }
    moduleInfo.Value().release(); // Transfer ownership to the object

    if (JSONSuccess != json_object_set_string(object, "timestamp", to_iso_datetime(system_clock::now()).c_str()))
    {
        return Error("Failed to set timestamp", ENOMEM);
    }

    if (JSONSuccess != json_object_set_string(object, "action", action == Evaluator::Action::Audit ? "Audit" : "Remediation"))
    {
        return Error("Failed to set action", ENOMEM);
    }

    value = json_value_init_array();
    if (nullptr == value)
    {
        return Error("Failed to initialize JSON array", ENOMEM);
    }
    if (JSONSuccess != json_object_set_value(object, "rules", value))
    {
        json_value_free(value);
        return Error("Failed to set rules", ENOMEM);
    }

    return Optional<Error>();
}

    Optional<Error> JsonFormatter::AddEntry(const mof::MofEntry& entry, Status status, const string& payload)
    {
        auto* value = json_value_init_object();
        if (nullptr == value)
        {
            return Error("Failed to initialize JSON object", ENOMEM);
        }
        auto result = JsonWrapper(value, compliance::JsonWrapperDeleter());
        auto* object = json_value_get_object(result.get());
        if (nullptr == object)
        {
            return Error("Failed to get JSON object", ENOMEM);
        }

        value = json_parse_string(payload.c_str());
        if(nullptr == value)
        {
            return Error("Failed to parse JSON payload", ENOMEM);
        }
        if(json_value_get_type(value) != JSONArray)
        {
            json_value_free(value);
            return Error("Invalid JSON payload", EINVAL);
        }

        if (JSONSuccess != json_object_set_value(object, "indicators", value))
        {
            json_value_free(value);
            return Error("Failed to set JSON payload", ENOMEM);
        }
        value = nullptr;

        if(JSONSuccess != json_object_set_string(object, "resourceID", entry.resourceID.c_str()))
        {
            return Error("Failed to set JSON resourceID", ENOMEM);
        }

        if(JSONSuccess != json_object_set_string(object, "payloadKey", entry.payloadKey.c_str()))
        {
            return Error("Failed to set JSON payloadKey", ENOMEM);
        }

        if(JSONSuccess != json_object_set_string(object, "ruleName", entry.ruleName.c_str()))
        {
            return Error("Failed to set JSON ruleName", ENOMEM);
        }

        if(JSONSuccess != json_object_set_string(object, "status", status == compliance::Status::Compliant ? "Compliant" : "NonCompliant"))
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

        if (JSONSuccess != json_array_append_value(array, result.get()))
        {
            return Error("Failed to append JSON value", ENOMEM);
        }
        result.release(); // Transfer ownership to the array

        return Optional<Error>();
    }

    Result<string> JsonFormatter::Finish(compliance::Status status)
    {
        auto* object = json_value_get_object(mJson.get());
        if (nullptr == object)
        {
            return Error("Failed to get JSON object", ENOMEM);
        }

        if(JSONSuccess != json_object_set_number(object, "durationMs", std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - mBegin).count()))
        {
            return Error("Failed to set JSON duration", ENOMEM);
        }

        if(JSONSuccess != json_object_set_string(object, "status", status == compliance::Status::Compliant ? "Compliant" : "NonCompliant"))
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
} // namespace formatters
