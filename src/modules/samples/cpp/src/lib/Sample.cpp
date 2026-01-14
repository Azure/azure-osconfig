// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Sample.h"

const std::string Sample::m_componentName = "SampleComponent";

const std::string Sample::m_desiredStringObjectName = "desiredStringObject";
const std::string Sample::m_reportedStringObjectName = "reportedStringObject";
const std::string Sample::m_desiredIntegerObjectName = "desiredIntegerObject";
const std::string Sample::m_reportedIntegerObjectName = "reportedIntegerObject";
const std::string Sample::m_desiredBooleanObjectName = "desiredBooleanObject";
const std::string Sample::m_reportedBooleanObjectName = "reportedBooleanObject";
const std::string Sample::m_desiredObjectName = "desiredObject";
const std::string Sample::m_reportedObjectName = "reportedObject";
const std::string Sample::m_desiredArrayObjectName = "desiredArrayObject";
const std::string Sample::m_reportedArrayObjectName = "reportedArrayObject";

const std::string Sample::m_stringSettingName = "stringSetting";
const std::string Sample::m_integerSettingName = "integerSetting";
const std::string Sample::m_booleanSettingName = "booleanSetting";
const std::string Sample::m_integerEnumerationSettingName = "integerEnumerationSetting";
const std::string Sample::m_stringEnumerationSettingName = "stringEnumerationSetting";
const std::string Sample::m_stringArraySettingName = "stringsArraySetting";
const std::string Sample::m_integerArraySettingName = "integerArraySetting";
const std::string Sample::m_stringMapSettingName = "stringMapSetting";
const std::string Sample::m_integerMapSettingName = "integerMapSetting";

const std::string Sample::m_stringEnumerationNone = "none";
const std::string Sample::m_stringEnumerationValue1 = "value1";
const std::string Sample::m_stringEnumerationValue2 = "value2";

const std::string Sample::m_info = R""""({
    "Name": "C++ Sample",
    "Description": "A sample module written in C++",
    "Manufacturer": "Microsoft",
    "VersionMajor": 1,
    "VersionMinor": 0,
    "VersionInfo": "",
    "Components": ["SampleComponent"],
    "Lifetime": 1,
    "UserAccount": 0})"""";

OsConfigLogHandle SampleLog::m_log = nullptr;

Sample::Sample(unsigned int maxPayloadSizeBytes) :
    m_stringValue(""),
    m_integerValue(0),
    m_booleanValue(false),
    m_maxPayloadSizeBytes(maxPayloadSizeBytes)
{
    m_objectValue.stringSetting = "";
    m_objectValue.integerSetting = 0;
    m_objectValue.booleanSetting = false;
    m_objectValue.integerEnumerationSetting = IntegerEnumeration::None;
    m_objectValue.stringEnumerationSetting = StringEnumeration::None;
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
        std::size_t len = m_info.length();
        *payload = new (std::nothrow) char[len];

        if (nullptr == *payload)
        {
            status = ENOMEM;
            OsConfigLogError(SampleLog::Get(), "MmiGetInfo failed to allocate memory");
        }
        else
        {
            std::memcpy(*payload, m_info.c_str(), len);
            *payloadSizeBytes = len;
        }
    }

    return status;
}

int Sample::Set(const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes)
{
    int status = MMI_OK;
    nlohmann::ordered_json document;

    if (payloadSizeBytes < 0)
    {
        OsConfigLogError(SampleLog::Get(), "Invalid payloadSizeBytes: %d", payloadSizeBytes);
        status = EINVAL;
        return status;
    }

    try
    {
        document = nlohmann::ordered_json::parse(std::string(payload, payloadSizeBytes));
    }
    catch (const std::exception& e)
    {
        OsConfigLogError(SampleLog::Get(), "Unable to parse JSON payload: %s", e.what());
        status = EINVAL;
        return status;
    }

    // Dispatch the request to the appropriate method for the given component and object
    if (0 == m_componentName.compare(componentName))
    {
        if (0 == m_desiredStringObjectName.compare(objectName))
        {
            // Parse the string from the payload
            if (document.is_string())
            {
                // Do something with the string
                std::string value = document.get<std::string>();
                m_stringValue = value;
                status = MMI_OK;
            }
            else
            {
                OsConfigLogError(SampleLog::Get(), "JSON payload is not a string");
                status = EINVAL;
            }
        }
        else if (0 == m_desiredBooleanObjectName.compare(objectName))
        {
            if (document.is_boolean())
            {
                // Do something with the boolean
                bool value = document.get<bool>();
                m_booleanValue = value;
                status = MMI_OK;
            }
            else
            {
                OsConfigLogError(SampleLog::Get(), "JSON payload is not a boolean");
                status = EINVAL;
            }
        }
        else if (0 == m_desiredIntegerObjectName.compare(objectName))
        {
            if (document.is_number_integer())
            {
                // Do something with the integer
                int value = document.get<int>();
                m_integerValue = value;
                status = MMI_OK;
            }
            else
            {
                OsConfigLogError(SampleLog::Get(), "JSON payload is not an integer");
                status = EINVAL;
            }
        }
        else if (0 == m_desiredObjectName.compare(objectName))
        {
            if (document.is_object())
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
                    status = EINVAL;
                    OsConfigLogError(SampleLog::Get(), "Failed to deserialize object");
                }
            }
            else
            {
                OsConfigLogError(SampleLog::Get(), "JSON payload is not an object");
                status = EINVAL;
            }
        }
        else if (0 == m_desiredArrayObjectName.compare(objectName))
        {
            if (document.is_array())
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
                    status = EINVAL;
                    OsConfigLogError(SampleLog::Get(), "Failed to deserialize array of objects");
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
            status = EINVAL;
            OsConfigLogError(SampleLog::Get(), "Invalid object name: %s", objectName);
        }
    }
    else
    {
        status = EINVAL;
        OsConfigLogError(SampleLog::Get(), "Invalid component name: %s", componentName);
    }

    return status;
}

int Sample::Get(const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MMI_OK;

    if (nullptr == payloadSizeBytes)
    {
        status = EINVAL;
        OsConfigLogError(SampleLog::Get(), "Invalid payloadSizeBytes");
    }
    else
    {
        *payload = nullptr;
        *payloadSizeBytes = 0;

        unsigned int maxPayloadSizeBytes = GetMaxPayloadSizeBytes();
        nlohmann::ordered_json document;

        // Dispatch the get request to the appropriate method for the given component and object
        if (0 == m_componentName.compare(componentName))
        {
            if (0 == m_reportedStringObjectName.compare(objectName))
            {
                // Get the string value to report
                std::string value = m_stringValue;
                document = value;

                // Serialize the JSON object to the payload buffer
                status = Sample::SerializeJsonPayload(document, payload, payloadSizeBytes, maxPayloadSizeBytes);
            }
            else if (0 == m_reportedBooleanObjectName.compare(objectName))
            {
                // Get the boolean value to report
                bool value = m_booleanValue;
                document = value;

                // Serialize the JSON object to the payload buffer
                status = Sample::SerializeJsonPayload(document, payload, payloadSizeBytes, maxPayloadSizeBytes);
            }
            else if (0 == m_reportedIntegerObjectName.compare(objectName))
            {
                // Get the integer value to report
                int value = m_integerValue;
                document = value;

                // Serialize the JSON object to the payload buffer
                status = Sample::SerializeJsonPayload(document, payload, payloadSizeBytes, maxPayloadSizeBytes);
            }
            else if (0 == m_reportedObjectName.compare(objectName))
            {
                // Get the object value to report
                Object object = m_objectValue;

                // Serialize the object
                nlohmann::ordered_json jsonObj = Sample::SerializeObject(object);
                std::string jsonString = jsonObj.dump();

                status = Sample::CopyJsonPayload(jsonString, payload, payloadSizeBytes);
            }
            else if (0 == m_reportedArrayObjectName.compare(objectName))
            {
                // Get the objects array to report
                std::vector<Object> objects = m_objectArrayValue;

                // Serialize the array of objects
                nlohmann::ordered_json jsonArray = Sample::SerializeObjectArray(objects);
                std::string jsonString = jsonArray.dump();

                status = Sample::CopyJsonPayload(jsonString, payload, payloadSizeBytes);
            }
            else
            {
                status = EINVAL;
                OsConfigLogError(SampleLog::Get(), "Invalid object name: %s", objectName);
            }
        }
        else
        {
            status = EINVAL;
            OsConfigLogError(SampleLog::Get(), "Invalid component name: %s", componentName);
        }
    }

    return status;
}

int Sample::SerializeStringEnumeration(nlohmann::ordered_json& jsonObj, Sample::StringEnumeration value)
{
    int status = 0;

    switch (value)
    {
        case Sample::StringEnumeration::None:
            jsonObj[m_stringEnumerationSettingName] = m_stringEnumerationNone;
            break;
        case Sample::StringEnumeration::Value1:
            jsonObj[m_stringEnumerationSettingName] = m_stringEnumerationValue1;
            break;
        case Sample::StringEnumeration::Value2:
            jsonObj[m_stringEnumerationSettingName] = m_stringEnumerationValue2;
            break;
        default:
            status = EINVAL;
            OsConfigLogError(SampleLog::Get(), "Invalid string enumeration value: %d", static_cast<int>(value));
    }

    return status;
}

nlohmann::ordered_json Sample::SerializeObject(const Sample::Object& object)
{
    nlohmann::ordered_json jsonObj;

    // Object string setting
    jsonObj[m_stringSettingName] = object.stringSetting;

    // Object boolean setting
    jsonObj[m_booleanSettingName] = object.booleanSetting;

    // Object integer setting
    jsonObj[m_integerSettingName] = object.integerSetting;

    // Object integer enumeration setting
    jsonObj[m_integerEnumerationSettingName] = static_cast<int>(object.integerEnumerationSetting);

    // Object string enumeration setting
    SerializeStringEnumeration(jsonObj, object.stringEnumerationSetting);

    // Object string array setting
    jsonObj[m_stringArraySettingName] = nlohmann::ordered_json::array();
    for (auto& string : object.stringArraySetting)
    {
        jsonObj[m_stringArraySettingName].push_back(string);
    }

    // Object integer array setting
    jsonObj[m_integerArraySettingName] = nlohmann::ordered_json::array();
    for (auto& integer : object.integerArraySetting)
    {
        jsonObj[m_integerArraySettingName].push_back(integer);
    }

    // Object string map setting
    jsonObj[m_stringMapSettingName] = nlohmann::ordered_json::object();
    for (auto& pair : object.stringMapSetting)
    {
        jsonObj[m_stringMapSettingName][pair.first] = pair.second;
    }

    // Add key-value pairs for removed map elements
    for (auto& key : object.removedStringMapSettingKeys)
    {
        jsonObj[m_stringMapSettingName][key] = nullptr;
    }

    // Object integer map setting
    jsonObj[m_integerMapSettingName] = nlohmann::ordered_json::object();
    for (auto& pair : object.integerMapSetting)
    {
        jsonObj[m_integerMapSettingName][pair.first] = pair.second;
    }

    // Add key-value pairs for removed map elements
    for (auto& key : object.removedIntegerMapSettingKeys)
    {
        jsonObj[m_integerMapSettingName][key] = nullptr;
    }

    return jsonObj;
}

nlohmann::ordered_json Sample::SerializeObjectArray(const std::vector<Sample::Object>& objects)
{
    nlohmann::ordered_json jsonArray = nlohmann::ordered_json::array();

    for (auto& object : objects)
    {
        jsonArray.push_back(Sample::SerializeObject(object));
    }

    return jsonArray;
}

int Sample::DeserializeStringEnumeration(std::string str, Sample::StringEnumeration& value)
{
    int status = 0;

    if (0 == m_stringEnumerationNone.compare(str))
    {
        value = Sample::StringEnumeration::None;
    }
    else if (0 == m_stringEnumerationValue1.compare(str))
    {
        value = Sample::StringEnumeration::Value1;
    }
    else if (0 == m_stringEnumerationValue2.compare(str))
    {
        value = Sample::StringEnumeration::Value2;
    }
    else
    {
        status = EINVAL;
        OsConfigLogError(SampleLog::Get(), "Invalid string enumeration value: %s", str.c_str());
    }

    return status;
}

int Sample::DeserializeObject(const nlohmann::ordered_json& document, Object& object)
{
    int status = 0;

    // Deserialize a string setting
    if (document.contains(m_stringSettingName))
    {
        if (document[m_stringSettingName].is_string())
        {
            object.stringSetting = document[m_stringSettingName].get<std::string>();
        }
        else
        {
            status = EINVAL;
            OsConfigLogError(SampleLog::Get(), "%s is not a string", m_stringSettingName.c_str());
        }
    }
    else
    {
        status = EINVAL;
        OsConfigLogError(SampleLog::Get(), "JSON object does not contain a string setting");
    }

    // Deserialize a boolean setting
    if (document.contains(m_booleanSettingName))
    {
        if (document[m_booleanSettingName].is_boolean())
        {
            object.booleanSetting = document[m_booleanSettingName].get<bool>();
        }
        else
        {
            status = EINVAL;
            OsConfigLogError(SampleLog::Get(), "%s is not a boolean", m_booleanSettingName.c_str());
        }
    }
    else
    {
        status = EINVAL;
        OsConfigLogError(SampleLog::Get(), "JSON object does not contain a boolean setting");
    }

    // Deserialize an integer setting
    if (document.contains(m_integerSettingName))
    {
        if (document[m_integerSettingName].is_number_integer())
        {
            object.integerSetting = document[m_integerSettingName].get<int>();
        }
        else
        {
            status = EINVAL;
            OsConfigLogError(SampleLog::Get(), "%s is not an integer", m_integerSettingName.c_str());
        }
    }
    else
    {
        status = EINVAL;
        OsConfigLogError(SampleLog::Get(), "JSON object does not contain an integer setting");
    }

    // Deserialize an enumeration setting
    if (document.contains(m_integerEnumerationSettingName))
    {
        if (document[m_integerEnumerationSettingName].is_number_integer())
        {
            object.integerEnumerationSetting = static_cast<Sample::IntegerEnumeration>(document[m_integerEnumerationSettingName].get<int>());
        }
        else
        {
            status = EINVAL;
            OsConfigLogError(SampleLog::Get(), "%s is not an integer", m_integerEnumerationSettingName.c_str());
        }
    }
    else
    {
        status = EINVAL;
        OsConfigLogError(SampleLog::Get(), "JSON object does not contain an integer enumeration setting");
    }

    // Deserialize a string enumeration setting
    if (document.contains(m_stringEnumerationSettingName))
    {
        if (document[m_stringEnumerationSettingName].is_string())
        {
            std::string stringValue = document[m_stringEnumerationSettingName].get<std::string>();
            status = DeserializeStringEnumeration(stringValue, object.stringEnumerationSetting);
        }
        else
        {
            status = EINVAL;
            OsConfigLogError(SampleLog::Get(), "%s is not a string", m_stringEnumerationSettingName.c_str());
        }
    }
    else
    {
        status = EINVAL;
        OsConfigLogError(SampleLog::Get(), "JSON object does not contain a string enumeration setting");
    }

    // Deserialize a string array setting
    if (document.contains(m_stringArraySettingName))
    {
        if (document[m_stringArraySettingName].is_array())
        {
            for (size_t i = 0; i < document[m_stringArraySettingName].size(); ++i)
            {
                if (document[m_stringArraySettingName][i].is_string())
                {
                    object.stringArraySetting.push_back(document[m_stringArraySettingName][i].get<std::string>());
                }
                else
                {
                    status = EINVAL;
                    OsConfigLogError(SampleLog::Get(), "Invalid string in JSON object string array at position %zu", i);
                }
            }
        }
        else
        {
            status = EINVAL;
            OsConfigLogError(SampleLog::Get(), "%s is not an array", m_stringArraySettingName.c_str());
        }
    }
    else
    {
        status = EINVAL;
        OsConfigLogError(SampleLog::Get(), "JSON object does not contain a string array setting");
    }

    // Deserialize an integer array setting
    if (document.contains(m_integerArraySettingName))
    {
        if (document[m_integerArraySettingName].is_array())
        {
            for (size_t i = 0; i < document[m_integerArraySettingName].size(); ++i)
            {
                if (document[m_integerArraySettingName][i].is_number_integer())
                {
                    object.integerArraySetting.push_back(document[m_integerArraySettingName][i].get<int>());
                }
                else
                {
                    status = EINVAL;
                    OsConfigLogError(SampleLog::Get(), "Invalid integer in JSON object integer array at position %zu", i);
                }
            }
        }
        else
        {
            status = EINVAL;
            OsConfigLogError(SampleLog::Get(), "%s is not an array", m_integerArraySettingName.c_str());

        }
    }
    else
    {
        status = EINVAL;
        OsConfigLogError(SampleLog::Get(), "JSON object does not contain an integer array setting");
    }

    // Deserialize a map of strings to strings
    if (document.contains(m_stringMapSettingName))
    {
        if (document[m_stringMapSettingName].is_object())
        {
            for (auto& element : document[m_stringMapSettingName].items())
            {
                if (element.value().is_string())
                {
                    object.stringMapSetting[element.key()] = element.value().get<std::string>();
                }
                else if (element.value().is_null())
                {
                    object.stringMapSetting.erase(element.key());

                    // Store the removed element key for reporting
                    object.removedStringMapSettingKeys.push_back(element.key());
                }
                else
                {
                    OsConfigLogError(SampleLog::Get(), "Invalid string in JSON object string map at key %s", element.key().c_str());
                    status = EINVAL;
                }
            }
        }
        else
        {
            OsConfigLogError(SampleLog::Get(), "%s is not an object", m_stringMapSettingName.c_str());
            status = EINVAL;
        }
    }
    else
    {
        OsConfigLogError(SampleLog::Get(), "JSON object does not contain a string map setting");
        status = EINVAL;
    }

    // Deserialize a map of strings to integers
    if (document.contains(m_integerMapSettingName))
    {
        if (document[m_integerMapSettingName].is_object())
        {
            for (auto& element : document[m_integerMapSettingName].items())
            {
                if (element.value().is_number_integer())
                {
                    object.integerMapSetting[element.key()] = element.value().get<int>();
                }
                else if (element.value().is_null())
                {
                    object.integerMapSetting.erase(element.key());

                    // Store the removed element key for reporting
                    object.removedIntegerMapSettingKeys.push_back(element.key());
                }
                else
                {
                    OsConfigLogError(SampleLog::Get(), "Invalid integer in JSON object integer map at key %s", element.key().c_str());
                    status = EINVAL;
                }
            }
        }
        else
        {
            OsConfigLogError(SampleLog::Get(), "%s is not an object", m_integerMapSettingName.c_str());
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

int Sample::DeserializeObjectArray(const nlohmann::ordered_json& document, std::vector<Object>& objects)
{
    int status = 0;

    for (const auto& arrayItem : document)
    {
        Object object;
        if (arrayItem.is_object() && (0 == (status = DeserializeObject(arrayItem, object))))
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
int Sample::SerializeJsonPayload(const nlohmann::ordered_json& document, MMI_JSON_STRING* payload, int* payloadSizeBytes, unsigned int maxPayloadSizeBytes)
{
    int status = MMI_OK;
    std::string jsonString = document.dump();

    if ((0 != maxPayloadSizeBytes) && (jsonString.size() > maxPayloadSizeBytes))
    {
        status = E2BIG;
        OsConfigLogError(SampleLog::Get(), "Failed to serialize JSON object to buffer");
    }
    else
    {
        status = Sample::CopyJsonPayload(jsonString, payload, payloadSizeBytes);
    }

    return status;
}

int Sample::CopyJsonPayload(const std::string& jsonString, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MMI_OK;

    *payload = new (std::nothrow) char[jsonString.size()];
    if (nullptr == *payload)
    {
        status = ENOMEM;
        OsConfigLogError(SampleLog::Get(), "Unable to allocate memory for payload");
    }
    else
    {
        std::fill(*payload, *payload + jsonString.size(), 0);
        std::memcpy(*payload, jsonString.c_str(), jsonString.size());
        *payloadSizeBytes = jsonString.size();
    }

    return status;
}

unsigned int Sample::GetMaxPayloadSizeBytes()
{
    return m_maxPayloadSizeBytes;
}

void Sample::MmiFree(MMI_JSON_STRING payload)
{
    if (nullptr != payload)
    {
        delete[] payload;
    }
}
