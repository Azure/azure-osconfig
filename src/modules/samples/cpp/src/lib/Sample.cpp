// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "CommonUtils.h"
#include "Mmi.h"
#include "Sample.h"

static const std::string g_componentName = "SampleComponent";

static const std::string g_desiredStringObjectName = "desiredStringObject";
static const std::string g_reportedStringObjectName = "reportedStringObject";
static const std::string g_desiredIntegerObjectName = "desiredIntegerObject";
static const std::string g_reportedIntegerObjectName = "reportedIntegerObject";
static const std::string g_desiredBooleanObjectName = "desiredBooleanObject";
static const std::string g_reportedBooleanObjectName = "reportedBooleanObject";
static const std::string g_desiredObjectName = "desiredObject";
static const std::string g_reportedObjectName = "reportedObject";
static const std::string g_desiredArrayObjectName = "desiredArrayObject";
static const std::string g_reportedArrayObjectName = "reportedArrayObject";

static const std::string g_stringSettingName = "stringSetting";
static const std::string g_integerSettingName = "integerSetting";
static const std::string g_booleanSettingName = "booleanSetting";
static const std::string g_integerEnumerationSettingName = "integerEnumerationSetting";
static const std::string g_stringArraySettingName = "stringsArraySetting";
static const std::string g_integerArraySettingName = "integerArraySetting";
static const std::string g_stringMapSettingName = "stringMapSetting";
static const std::string g_integerMapSettingName = "integerMapSetting";

constexpr const char info[] = R""""({
    "Name": "C++ Sample",
    "Description": "A sample module written in C++",
    "Manufacturer": "Microsoft",
    "VersionMajor": 1,
    "VersionMinor": 0,
    "VersionInfo": "",
    "Components": ["SampleComponent"],
    "Lifetime": 1,
    "UserAccount": 0})"""";

OSCONFIG_LOG_HANDLE SampleLog::m_log = nullptr;

Sample::Sample(unsigned int maxPayloadSizeBytes)
{
    m_maxPayloadSizeBytes = maxPayloadSizeBytes;
}

int Sample::GetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MMI_OK;

    if (nullptr == clientName)
    {
        OsConfigLogError(SampleLog::Get(), "MmiGetInfo called with null clientName");
        status = EINVAL;
    }
    else if (nullptr == payload)
    {
        OsConfigLogError(SampleLog::Get(), "MmiGetInfo called with null payload");
        status = EINVAL;
    }
    else if (nullptr == payloadSizeBytes)
    {
        OsConfigLogError(SampleLog::Get(), "MmiGetInfo called with null payloadSizeBytes");
        status = EINVAL;
    }
    else
    {
        try
        {
            std::size_t len = ARRAY_SIZE(info) - 1;
            *payload = new (std::nothrow) char[len];
            if (nullptr == *payload)
            {
                OsConfigLogError(SampleLog::Get(), "MmiGetInfo failed to allocate memory");
                status = ENOMEM;
            }
            else
            {
                std::memcpy(*payload, info, len);
                *payloadSizeBytes = len;
            }
        }
        catch (const std::exception& e)
        {
            OsConfigLogError(SampleLog::Get(), "MmiGetInfo exception thrown: %s", e.what());
            status = EINTR;

            if (nullptr != *payload)
            {
                delete[] * payload;
                *payload = nullptr;
            }

            if (nullptr != payloadSizeBytes)
            {
                *payloadSizeBytes = 0;
            }
        }
    }

    return status;
}

int Sample::Set(const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes)
{
    int status = MMI_OK;
    rapidjson::Document document;

    if (document.Parse(payload, payloadSizeBytes).HasParseError())
    {
        OsConfigLogError(SampleLog::Get(), "Unabled to parse JSON payload: %s", payload);
        status = EINVAL;
    }
    else
    {
        // Dispatch the request to the appropriate method for the given component and object
        if (0 == g_componentName.compare(componentName))
        {
            if (0 == g_desiredStringObjectName.compare(objectName))
            {
                // Parse the string from the payload
                if (document.IsString())
                {
                    // Do something with the string
                    std::string value = document.GetString();
                    m_stringValue = value;
                    status = MMI_OK;
                }
                else
                {
                    OsConfigLogError(SampleLog::Get(), "JSON payload is not a string");
                    status = EINVAL;
                }
            }
            else if (0 == g_desiredBooleanObjectName.compare(objectName))
            {
                if (document.IsBool())
                {
                    // Do something with the boolean
                    bool value = document.GetBool();
                    m_booleanValue = value;
                    status = MMI_OK;
                }
                else
                {
                    OsConfigLogError(SampleLog::Get(), "JSON payload is not a boolean");
                    status = EINVAL;
                }
            }
            else if (0 == g_desiredIntegerObjectName.compare(objectName))
            {
                if (document.IsInt())
                {
                    // Do something with the integer
                    int value = document.GetInt();
                    m_integerValue = value;
                    status = MMI_OK;
                }
                else
                {
                    OsConfigLogError(SampleLog::Get(), "JSON payload is not an integer");
                    status = EINVAL;
                }
            }
            else if (0 == g_desiredObjectName.compare(objectName))
            {
                if (document.IsObject())
                {
                    // Deserialize the object
                    Sample::Object object;
                    if (0 == DeserializeObject(document, object))
                    {
                        // Do something with the object
                        m_objectValue = object;
                        status = MMI_OK;
                    }
                    else
                    {
                        OsConfigLogError(SampleLog::Get(), "Failed to deserialize object");
                        status = EINVAL;
                    }
                }
                else
                {
                    OsConfigLogError(SampleLog::Get(), "JSON payload is not an object");
                    status = EINVAL;
                }
            }
            else if (0 == g_desiredArrayObjectName.compare(objectName))
            {
                if (document.IsArray())
                {
                    // Deserialize the array of objects
                    std::vector<Sample::Object> objects;
                    if (0 == DeserializeObjectArray(document, objects))
                    {
                        // Do something with the objects
                        m_objectArrayValue = objects;
                        status = MMI_OK;
                    }
                    else
                    {
                        OsConfigLogError(SampleLog::Get(), "Failed to deserialize array of objects");
                        status = EINVAL;
                    }
                }
                else
                {
                    OsConfigLogError(SampleLog::Get(), "JSON payload is not an array");
                    status = EINVAL;
                }
            }
            else
            {
                OsConfigLogError(SampleLog::Get(), "Invalid object name: %s", objectName);
                status = EINVAL;
            }
        }
        else
        {
            OsConfigLogError(SampleLog::Get(), "Invalid component name: %s", componentName);
            status = EINVAL;
        }
    }

    return status;
}

int Sample::Get(const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MMI_OK;

    if (nullptr == payloadSizeBytes)
    {
        OsConfigLogError(SampleLog::Get(), "Invalid payloadSizeBytes");
        status = EINVAL;
    }
    else
    {
        *payload = nullptr;
        *payloadSizeBytes = 0;

        unsigned int maxPayloadSizeBytes = GetMaxPayloadSizeBytes();
        rapidjson::Document document;

        // Dispatch the get request to the appropriate method for the given component and object
        if (0 == g_componentName.compare(componentName))
        {
            if (0 == g_reportedStringObjectName.compare(objectName))
            {
                // Get the string value to report
                std::string value = m_stringValue;
                document.SetString(value.c_str(), document.GetAllocator());

                // Serialize the JSON object to the payload buffer
                status = Sample::SerializeJsonPayload(document, payload, payloadSizeBytes, maxPayloadSizeBytes);
            }
            else if (0 == g_reportedBooleanObjectName.compare(objectName))
            {
                // Get the boolean value to report
                bool value = m_booleanValue;
                document.SetBool(value);

                // Serialize the JSON object to the payload buffer
                status = Sample::SerializeJsonPayload(document, payload, payloadSizeBytes, maxPayloadSizeBytes);
            }
            else if (0 == g_reportedIntegerObjectName.compare(objectName))
            {
                // Get the integer value to report
                int value = m_integerValue;
                document.SetInt(value);

                // Serialize the JSON object to the payload buffer
                status = Sample::SerializeJsonPayload(document, payload, payloadSizeBytes, maxPayloadSizeBytes);
            }
            else if (0 == g_reportedObjectName.compare(objectName))
            {
                // Get the object value to report
                Object object = m_objectValue;

                // Serialize the object
                rapidjson::StringBuffer buffer;
                rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

                if (0 == Sample::SerializeObject(writer, object))
                {
                    status = Sample::CopyJsonPayload(buffer, payload, payloadSizeBytes);
                }
                else
                {
                    OsConfigLogError(SampleLog::Get(), "Failed to serialize object");
                    status = EINVAL;
                }
            }
            else if (0 == g_reportedArrayObjectName.compare(objectName))
            {
                // Get the objects array to report
                std::vector<Object> objects = m_objectArrayValue;

                // Serialize the array of objects
                rapidjson::StringBuffer buffer;
                rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

                if (0 == Sample::SerializeObjectArray(writer, objects))
                {
                    status = Sample::CopyJsonPayload(buffer, payload, payloadSizeBytes);
                }
                else
                {
                    OsConfigLogError(SampleLog::Get(), "Failed to serialize array of objects");
                    status = EINVAL;
                }
            }
            else
            {
                OsConfigLogError(SampleLog::Get(), "Invalid object name: %s", objectName);
                status = EINVAL;
            }
        }
        else
        {
            OsConfigLogError(SampleLog::Get(), "Invalid component name: %s", componentName);
            status = EINVAL;
        }
    }

    return status;
}

int Sample::SerializeObject(rapidjson::Writer<rapidjson::StringBuffer>& writer, const Sample::Object& object)
{
    int status = 0;

    writer.StartObject();

    // Object string setting
    writer.Key(g_stringSettingName.c_str());
    writer.String(object.stringSetting.c_str());

    // Object boolean setting
    writer.Key(g_booleanSettingName.c_str());
    writer.Bool(object.booleanSetting);

    // Object integer setting
    writer.Key(g_integerSettingName.c_str());
    writer.Int(object.integerSetting);

    // Object enumeration setting
    writer.Key(g_integerEnumerationSettingName.c_str());
    writer.Int(static_cast<int>(object.enumerationSetting));

    // Object string array setting
    writer.Key(g_stringArraySettingName.c_str());
    writer.StartArray();

    for (auto& string : object.stringArraySetting)
    {
        writer.String(string.c_str());
    }

    writer.EndArray();

    // Object integer array setting
    writer.Key(g_integerArraySettingName.c_str());
    writer.StartArray();

    for (auto& integer : object.integerArraySetting)
    {
        writer.Int(integer);
    }

    writer.EndArray();

    // Object string map setting
    writer.Key(g_stringMapSettingName.c_str());
    writer.StartObject();

    for (auto& pair : object.stringMapSetting)
    {
        writer.Key(pair.first.c_str());
        writer.String(pair.second.c_str());
    }

    // Add key-value pairs for removed map elements
    for (auto& key : object.removedStringMapSettingKeys)
    {
        writer.Key(key.c_str());
        writer.Null();
    }

    writer.EndObject();

    // Object integer map setting
    writer.Key(g_integerMapSettingName.c_str());
    writer.StartObject();

    for (auto& pair : object.integerMapSetting)
    {
        writer.Key(pair.first.c_str());
        writer.Int(pair.second);
    }

    // Add key-value pairs for removed map elements
    for (auto& key : object.removedIntegerMapSettingKeys)
    {
        writer.Key(key.c_str());
        writer.Null();
    }

    writer.EndObject();

    writer.EndObject();

    return status;
}

int Sample::SerializeObjectArray(rapidjson::Writer<rapidjson::StringBuffer>& writer, const std::vector<Sample::Object>& objects)
{
    int status = 0;

    writer.StartArray();

    for (auto& object : objects)
    {
        Sample::SerializeObject(writer, object);
    }

    writer.EndArray();

    return status;
}

int Sample::DeserializeObject(rapidjson::Document& document, Object& object)
{
    int status = 0;

    // Deserialize a string setting
    if (document.HasMember(g_stringSettingName.c_str()))
    {
        if (document[g_stringSettingName.c_str()].IsString())
        {
            object.stringSetting = document[g_stringSettingName.c_str()].GetString();
        }
        else
        {
            OsConfigLogError(SampleLog::Get(), "%s is not a string", g_stringSettingName.c_str());
            status = EINVAL;
        }
    }
    else
    {
        OsConfigLogError(SampleLog::Get(), "JSON object does not contain a string setting");
        status = EINVAL;
    }

    // Deserialize a boolean setting
    if (document.HasMember(g_booleanSettingName.c_str()))
    {
        if (document[g_booleanSettingName.c_str()].IsBool())
        {
            object.booleanSetting = document[g_booleanSettingName.c_str()].GetBool();
        }
        else
        {
            OsConfigLogError(SampleLog::Get(), "%s is not a boolean", g_booleanSettingName.c_str());
            status = EINVAL;
        }
    }
    else
    {
        OsConfigLogError(SampleLog::Get(), "JSON object does not contain a boolean setting");
        status = EINVAL;
    }

    // Deserialize an integer setting
    if (document.HasMember(g_integerSettingName.c_str()))
    {
        if (document[g_integerSettingName.c_str()].IsInt())
        {
            object.integerSetting = document[g_integerSettingName.c_str()].GetInt();
        }
        else
        {
            OsConfigLogError(SampleLog::Get(), "%s is not an integer", g_integerSettingName.c_str());
            status = EINVAL;
        }
    }
    else
    {
        OsConfigLogError(SampleLog::Get(), "JSON object does not contain an integer setting");
        status = EINVAL;
    }

    // Deserialize an enumeration setting
    if (document.HasMember(g_integerEnumerationSettingName.c_str()))
    {
        if (document[g_integerEnumerationSettingName.c_str()].IsInt())
        {
            object.enumerationSetting = static_cast<Sample::IntegerEnumeration>(document[g_integerEnumerationSettingName.c_str()].GetInt());
        }
        else
        {
            OsConfigLogError(SampleLog::Get(), "%s is not an integer", g_integerEnumerationSettingName.c_str());
            status = EINVAL;
        }
    }
    else
    {
        OsConfigLogError(SampleLog::Get(), "JSON object does not contain an integer enumeration setting");
        status = EINVAL;
    }

    // Deserialize an string array setting
    if (document.HasMember(g_stringArraySettingName.c_str()))
    {
        if (document[g_stringArraySettingName.c_str()].IsArray())
        {
            for (rapidjson::SizeType i = 0; i < document[g_stringArraySettingName.c_str()].Size(); ++i)
            {
                if (document[g_stringArraySettingName.c_str()][i].IsString())
                {
                    object.stringArraySetting.push_back(document[g_stringArraySettingName.c_str()][i].GetString());
                }
                else
                {
                    OsConfigLogError(SampleLog::Get(), "Invalid string in JSON object string array at position %d", i);
                    status = EINVAL;
                }
            }
        }
        else
        {
            OsConfigLogError(SampleLog::Get(), "%s is not an array", g_stringArraySettingName.c_str());
            status = EINVAL;
        }
    }
    else
    {
        OsConfigLogError(SampleLog::Get(), "JSON object does not contain a string array setting");
        status = EINVAL;
    }

    // Deserialize an integer array setting
    if (document.HasMember(g_integerArraySettingName.c_str()))
    {
        if (document[g_integerArraySettingName.c_str()].IsArray())
        {
            for (rapidjson::SizeType i = 0; i < document[g_integerArraySettingName.c_str()].Size(); ++i)
            {
                if (document[g_integerArraySettingName.c_str()][i].IsInt())
                {
                    object.integerArraySetting.push_back(document[g_integerArraySettingName.c_str()][i].GetInt());
                }
                else
                {
                    OsConfigLogError(SampleLog::Get(), "Invalid integer in JSON object integer array at position %d", i);
                    status = EINVAL;
                }
            }
        }
        else
        {
            OsConfigLogError(SampleLog::Get(), "%s is not an array", g_integerArraySettingName.c_str());
            status = EINVAL;
        }
    }
    else
    {
        OsConfigLogError(SampleLog::Get(), "JSON object does not contain an integer array setting");
        status = EINVAL;
    }

    // Deserialize a map of strings to strings
    if (document.HasMember(g_stringMapSettingName.c_str()))
    {
        if (document[g_stringMapSettingName.c_str()].IsObject())
        {
            for(auto& member : document[g_stringMapSettingName.c_str()].GetObject())
            {
                if (member.value.IsString())
                {
                    object.stringMapSetting[member.name.GetString()] = member.value.GetString();
                }
                else if (member.value.IsNull())
                {
                    object.stringMapSetting.erase(member.name.GetString());

                    // Store the removed element key for reporting
                    object.removedStringMapSettingKeys.push_back(member.name.GetString());
                }
                else
                {
                    OsConfigLogError(SampleLog::Get(), "Invalid string in JSON object string map at key %s", member.name.GetString());
                    status = EINVAL;
                }
            }
        }
        else
        {
            OsConfigLogError(SampleLog::Get(), "%s is not an object", g_stringMapSettingName.c_str());
            status = EINVAL;
        }
    }
    else
    {
        OsConfigLogError(SampleLog::Get(), "JSON object does not contain a string map setting");
        status = EINVAL;
    }

    // Deserialize a map of strings to integers
    if (document.HasMember(g_integerMapSettingName.c_str()))
    {
        if (document[g_integerMapSettingName.c_str()].IsObject())
        {
            for (auto& member : document[g_integerMapSettingName.c_str()].GetObject())
            {
                if (member.value.IsInt())
                {
                    object.integerMapSetting[member.name.GetString()] = member.value.GetInt();
                }
                else if (member.value.IsNull())
                {
                    object.integerMapSetting.erase(member.name.GetString());

                    // Store the removed element key for reporting
                    object.removedIntegerMapSettingKeys.push_back(member.name.GetString());
                }
                else
                {
                    OsConfigLogError(SampleLog::Get(), "Invalid integer in JSON object integer map at key %s", member.name.GetString());
                    status = EINVAL;
                }
            }
        }
        else
        {
            OsConfigLogError(SampleLog::Get(), "%s is not an object", g_integerMapSettingName.c_str());
            status = EINVAL;
        }
    }
    else
    {
        OsConfigLogError(SampleLog::Get(), "JSON object does not contain a string integer map setting");
        status = EINVAL;
    }

    return status;
}

int Sample::DeserializeObjectArray(rapidjson::Document& document, std::vector<Object>& objects)
{
    int status = 0;

    for (rapidjson::Value::ConstValueIterator arrayItem = document.Begin(); arrayItem != document.End(); ++arrayItem)
    {
        Object object;
        rapidjson::Document objectDocument;
        objectDocument.CopyFrom(*arrayItem, objectDocument.GetAllocator());
        if (objectDocument.IsObject() && (0 == (status = DeserializeObject(objectDocument, object))))
        {
            objects.push_back(object);
        }
        else
        {
            OsConfigLogError(SampleLog::Get(), "Failed to deserialize object");
            break;
        }
    }

    return status;
}

// A helper method for serializing a JSON document to a payload string
int Sample::SerializeJsonPayload(rapidjson::Document& document, MMI_JSON_STRING* payload, int* payloadSizeBytes, unsigned int maxPayloadSizeBytes)
{
    int status = MMI_OK;
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

    document.Accept(writer);

    if ((0 != maxPayloadSizeBytes) && (buffer.GetSize() > maxPayloadSizeBytes))
    {
        OsConfigLogError(SampleLog::Get(), "Failed to serialize JSON object to buffer");
        status = E2BIG;
    }
    else
    {
        status = Sample::CopyJsonPayload(buffer, payload, payloadSizeBytes);
    }

    return status;
}

int Sample::CopyJsonPayload(rapidjson::StringBuffer& buffer, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MMI_OK;

    try
    {
        *payload = new (std::nothrow) char[buffer.GetSize()];
        if (nullptr == *payload)
        {
            OsConfigLogError(SampleLog::Get(), "Unable to allocate memory for payload");
            status = ENOMEM;
        }
        else
        {
            std::fill(*payload, *payload + buffer.GetSize(), 0);
            std::memcpy(*payload, buffer.GetString(), buffer.GetSize());
            *payloadSizeBytes = buffer.GetSize();
        }
    }
    catch (const std::exception& e)
    {
        OsConfigLogError(SampleLog::Get(), "Could not allocate payload: %s", e.what());
        status = EINTR;

        if (nullptr != *payload)
        {
            delete[] *payload;
            *payload = nullptr;
        }

        if (nullptr != payloadSizeBytes)
        {
            *payloadSizeBytes = 0;
        }
    }

    return status;
}

unsigned int Sample::GetMaxPayloadSizeBytes()
{
    return m_maxPayloadSizeBytes;
}