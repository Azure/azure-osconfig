// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "osquery.h"

const std::string OSQuery::m_componentName = "OSQuery";

const char* g_emptyString = "";
const char* g_dash = "-";
const char* g_cmd_osquery_exists = "which ~/osquery/osqueryi";

const char* g_cmd_osquery_install_0 = "mkdir -p ~/osquery/osqueryi";
const char* g_cmd_osquery_install_1 = "wget --directory-prefix=/tmp https://pkg.osquery.io/linux/osquery-5.7.0_1.linux_x86_64.tar.gz";
const char* g_cmd_osquery_install_2 = "tar zxfv /tmp/osquery-5.7.0_1.linux_x86_64.tar.gz -C ~/osquery/";
const char* g_cmd_osquery_install_3 = "cp -f ~/osquery/opt/osquery/share/osquery/osquery.example.conf /etc/osquery/osquery.conf";
const char* g_cmd_osquery_install_4 = "ln -s ~/osquery/opt/osquery/bin/osqueryd ~/osquery/osqueryi";

const std::string OSQuery::m_info = R""""({
    "Name": "OSQuery",
    "Description": "A OSQuery module",
    "Manufacturer": "Microsoft",
    "VersionMajor": 1,
    "VersionMinor": 0,
    "VersionInfo": "",
    "Components": ["OSQuery"],
    "Lifetime": 1,
    "UserAccount": 0})"""";

OSCONFIG_LOG_HANDLE OSQueryLog::m_log = nullptr;

OSQuery::OSQuery(unsigned int maxPayloadSizeBytes) :
    m_maxPayloadSizeBytes(maxPayloadSizeBytes)
{
    // TODO: Add any initialization code here
    int status = MMI_OK;
    std::string payloadString;
    OsConfigLogInfo(OSQueryLog::Get(), "Checking for osquery installation");
    status = RunCommand(g_cmd_osquery_exists, payloadString);
    if (status != MMI_OK)
    {
        OsConfigLogInfo(OSQueryLog::Get(), "Installing osquery");
        status = RunCommand(g_cmd_osquery_install_0, payloadString);
        if (status == MMI_OK)
        {
            OsConfigLogInfo(OSQueryLog::Get(), "Downloading osquery");
            status = RunCommand(g_cmd_osquery_install_1, payloadString);
            if (status == MMI_OK)
            {
                OsConfigLogInfo(OSQueryLog::Get(), "Extracting osquery");
                status = RunCommand(g_cmd_osquery_install_2, payloadString);
                if (status == MMI_OK)
                {
                    OsConfigLogInfo(OSQueryLog::Get(), "Configuring osquery");
                    status = RunCommand(g_cmd_osquery_install_3, payloadString);
                    if (status == MMI_OK)
                    {
                        OsConfigLogInfo(OSQueryLog::Get(), "Creating symlinks");
                        status = RunCommand(g_cmd_osquery_install_4, payloadString);
                    }
                }
            }
        }
    }
    else
    {
        OsConfigLogInfo(OSQueryLog::Get(), "osquery already installed");
    }
}

int OSQuery::GetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MMI_OK;

    if (nullptr == clientName)
    {
        OsConfigLogError(OSQueryLog::Get(), "MmiGetInfo called with null clientName");
        status = EINVAL;
    }
    else if (nullptr == payload)
    {
        OsConfigLogError(OSQueryLog::Get(), "MmiGetInfo called with null payload");
        status = EINVAL;
    }
    else if (nullptr == payloadSizeBytes)
    {
        OsConfigLogError(OSQueryLog::Get(), "MmiGetInfo called with null payloadSizeBytes");
        status = EINVAL;
    }
    else
    {
        std::size_t len = m_info.length();
        *payload = new (std::nothrow) char[len];

        if (nullptr == *payload)
        {
            OsConfigLogError(OSQueryLog::Get(), "MmiGetInfo failed to allocate memory");
            status = ENOMEM;
        }
        else
        {
            std::memcpy(*payload, m_info.c_str(), len);
            *payloadSizeBytes = len;
        }
    }

    return status;
}

int OSQuery::Set(const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes)
{
    int status = MMI_OK;
    UNUSED(componentName);
    UNUSED(objectName);
    UNUSED(payload);
    UNUSED(payloadSizeBytes);

    return status;
}

int OSQuery::Get(const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MMI_OK;
    UNUSED(componentName);
    UNUSED(objectName);

    if (nullptr == payloadSizeBytes)
    {
        OsConfigLogError(OSQueryLog::Get(), "Invalid payloadSizeBytes");
        status = EINVAL;
    }
    else
    {
        *payload = nullptr;
        *payloadSizeBytes = 0;
        std::string payloadString;

        unsigned int maxPayloadSizeBytes = GetMaxPayloadSizeBytes();
        rapidjson::Document document;

        // Dispatch the get request to the appropriate method for the given component and object
        if (0 == m_componentName.compare(componentName))
        {
            // Check if osquery table exists
            std::string cmd = "~/osquery/osqueryi --json 'select * from " + std::string(objectName) + "'";
            status = RunCommand(cmd.c_str(), payloadString);
            if (status != MMI_OK)
            {
                OsConfigLogError(OSQueryLog::Get(), "Invalid osquery table: %s", objectName);
                status = EINVAL;
            }
            else
            {
                document.SetString(payloadString.c_str(), document.GetAllocator());
                status = SerializeJsonPayload(document, payload, payloadSizeBytes, maxPayloadSizeBytes);
            }
        }
        else
        {
            OsConfigLogError(OSQueryLog::Get(), "Invalid component name: %s", componentName);
            status = EINVAL;
        }
    }

    return status;
}

int OSQuery::RunCommand(const char* command, std::string& commandOutput)
{
    char *textResult = nullptr;
    int status = ExecuteCommand(nullptr, command, false, false, 0, 0, &textResult, nullptr, OSQueryLog::Get());
    if (MMI_OK == status)
    {
        commandOutput = (nullptr != textResult) ? std::string(textResult) : g_emptyString;
    }
    if (nullptr != textResult)
    {
        free(textResult);
    }

    return status;
}

// int OSQuery::SerializeStringEnumeration(rapidjson::Writer<rapidjson::StringBuffer>& writer, OSQuery::StringEnumeration value)
// {
//     int status = 0;

//     writer.Key(m_stringEnumerationSettingName.c_str());

//     switch (value)
//     {
//         case OSQuery::StringEnumeration::None:
//             writer.String(m_stringEnumerationNone.c_str());
//             break;
//         case OSQuery::StringEnumeration::Value1:
//             writer.String(m_stringEnumerationValue1.c_str());
//             break;
//         case OSQuery::StringEnumeration::Value2:
//             writer.String(m_stringEnumerationValue2.c_str());
//             break;
//         default:
//             OsConfigLogError(OSQueryLog::Get(), "Invalid string enumeration value: %d", static_cast<int>(value));
//             status = EINVAL;
//     }

//     return status;
// }

// int OSQuery::SerializeObject(rapidjson::Writer<rapidjson::StringBuffer>& writer, const OSQuery::Object& object)
// {
//     int status = 0;

//     writer.StartObject();

//     // Object string setting
//     writer.Key(m_stringSettingName.c_str());
//     writer.String(object.stringSetting.c_str());

//     // Object boolean setting
//     writer.Key(m_booleanSettingName.c_str());
//     writer.Bool(object.booleanSetting);

//     // Object integer setting
//     writer.Key(m_integerSettingName.c_str());
//     writer.Int(object.integerSetting);

//     // Object integer enumeration setting
//     writer.Key(m_integerEnumerationSettingName.c_str());
//     writer.Int(static_cast<int>(object.integerEnumerationSetting));

//     // Object string enumeration setting
//     status = SerializeStringEnumeration(writer, object.stringEnumerationSetting);

//     // Object string array setting
//     writer.Key(m_stringArraySettingName.c_str());
//     writer.StartArray();

//     for (auto& string : object.stringArraySetting)
//     {
//         writer.String(string.c_str());
//     }

//     writer.EndArray();

//     // Object integer array setting
//     writer.Key(m_integerArraySettingName.c_str());
//     writer.StartArray();

//     for (auto& integer : object.integerArraySetting)
//     {
//         writer.Int(integer);
//     }

//     writer.EndArray();

//     // Object string map setting
//     writer.Key(m_stringMapSettingName.c_str());
//     writer.StartObject();

//     for (auto& pair : object.stringMapSetting)
//     {
//         writer.Key(pair.first.c_str());
//         writer.String(pair.second.c_str());
//     }

//     // Add key-value pairs for removed map elements
//     for (auto& key : object.removedStringMapSettingKeys)
//     {
//         writer.Key(key.c_str());
//         writer.Null();
//     }

//     writer.EndObject();

//     // Object integer map setting
//     writer.Key(m_integerMapSettingName.c_str());
//     writer.StartObject();

//     for (auto& pair : object.integerMapSetting)
//     {
//         writer.Key(pair.first.c_str());
//         writer.Int(pair.second);
//     }

//     // Add key-value pairs for removed map elements
//     for (auto& key : object.removedIntegerMapSettingKeys)
//     {
//         writer.Key(key.c_str());
//         writer.Null();
//     }

//     writer.EndObject();

//     writer.EndObject();

//     return status;
// }

// int OSQuery::SerializeObjectArray(rapidjson::Writer<rapidjson::StringBuffer>& writer, const std::vector<OSQuery::Object>& objects)
// {
//     int status = 0;

//     writer.StartArray();

//     for (auto& object : objects)
//     {
//         OSQuery::SerializeObject(writer, object);
//     }

//     writer.EndArray();

//     return status;
// }

// int OSQuery::DeserializeStringEnumeration(std::string str, OSQuery::StringEnumeration& value)
// {
//     int status = 0;

//     if (0 == m_stringEnumerationNone.compare(str))
//     {
//         value = OSQuery::StringEnumeration::None;
//     }
//     else if (0 == m_stringEnumerationValue1.compare(str))
//     {
//         value = OSQuery::StringEnumeration::Value1;
//     }
//     else if (0 == m_stringEnumerationValue2.compare(str))
//     {
//         value = OSQuery::StringEnumeration::Value2;
//     }
//     else
//     {
//         OsConfigLogError(OSQueryLog::Get(), "Invalid string enumeration value: %s", str.c_str());
//         status = EINVAL;
//     }

//     return status;
// }

// int OSQuery::DeserializeObject(rapidjson::Document& document, Object& object)
// {
//     int status = 0;

//     // Deserialize a string setting
//     if (document.HasMember(m_stringSettingName.c_str()))
//     {
//         if (document[m_stringSettingName.c_str()].IsString())
//         {
//             object.stringSetting = document[m_stringSettingName.c_str()].GetString();
//         }
//         else
//         {
//             OsConfigLogError(OSQueryLog::Get(), "%s is not a string", m_stringSettingName.c_str());
//             status = EINVAL;
//         }
//     }
//     else
//     {
//         OsConfigLogError(OSQueryLog::Get(), "JSON object does not contain a string setting");
//         status = EINVAL;
//     }

//     // Deserialize a boolean setting
//     if (document.HasMember(m_booleanSettingName.c_str()))
//     {
//         if (document[m_booleanSettingName.c_str()].IsBool())
//         {
//             object.booleanSetting = document[m_booleanSettingName.c_str()].GetBool();
//         }
//         else
//         {
//             OsConfigLogError(OSQueryLog::Get(), "%s is not a boolean", m_booleanSettingName.c_str());
//             status = EINVAL;
//         }
//     }
//     else
//     {
//         OsConfigLogError(OSQueryLog::Get(), "JSON object does not contain a boolean setting");
//         status = EINVAL;
//     }

//     // Deserialize an integer setting
//     if (document.HasMember(m_integerSettingName.c_str()))
//     {
//         if (document[m_integerSettingName.c_str()].IsInt())
//         {
//             object.integerSetting = document[m_integerSettingName.c_str()].GetInt();
//         }
//         else
//         {
//             OsConfigLogError(OSQueryLog::Get(), "%s is not an integer", m_integerSettingName.c_str());
//             status = EINVAL;
//         }
//     }
//     else
//     {
//         OsConfigLogError(OSQueryLog::Get(), "JSON object does not contain an integer setting");
//         status = EINVAL;
//     }

//     // Deserialize an enumeration setting
//     if (document.HasMember(m_integerEnumerationSettingName.c_str()))
//     {
//         if (document[m_integerEnumerationSettingName.c_str()].IsInt())
//         {
//             object.integerEnumerationSetting = static_cast<OSQuery::IntegerEnumeration>(document[m_integerEnumerationSettingName.c_str()].GetInt());
//         }
//         else
//         {
//             OsConfigLogError(OSQueryLog::Get(), "%s is not an integer", m_integerEnumerationSettingName.c_str());
//             status = EINVAL;
//         }
//     }
//     else
//     {
//         OsConfigLogError(OSQueryLog::Get(), "JSON object does not contain an integer enumeration setting");
//         status = EINVAL;
//     }

//     // Deserialize a string enumeration setting
//     if (document.HasMember(m_stringEnumerationSettingName.c_str()))
//     {
//         if (document[m_stringEnumerationSettingName.c_str()].IsString())
//         {
//             std::string stringValue = document[m_stringEnumerationSettingName.c_str()].GetString();
//             status = DeserializeStringEnumeration(stringValue, object.stringEnumerationSetting);
//         }
//         else
//         {
//             OsConfigLogError(OSQueryLog::Get(), "%s is not a string", m_stringEnumerationSettingName.c_str());
//             status = EINVAL;
//         }
//     }
//     else
//     {
//         OsConfigLogError(OSQueryLog::Get(), "JSON object does not contain a string enumeration setting");
//         status = EINVAL;
//     }

//     // Deserialize a string array setting
//     if (document.HasMember(m_stringArraySettingName.c_str()))
//     {
//         if (document[m_stringArraySettingName.c_str()].IsArray())
//         {
//             for (rapidjson::SizeType i = 0; i < document[m_stringArraySettingName.c_str()].Size(); ++i)
//             {
//                 if (document[m_stringArraySettingName.c_str()][i].IsString())
//                 {
//                     object.stringArraySetting.push_back(document[m_stringArraySettingName.c_str()][i].GetString());
//                 }
//                 else
//                 {
//                     OsConfigLogError(OSQueryLog::Get(), "Invalid string in JSON object string array at position %d", i);
//                     status = EINVAL;
//                 }
//             }
//         }
//         else
//         {
//             OsConfigLogError(OSQueryLog::Get(), "%s is not an array", m_stringArraySettingName.c_str());
//             status = EINVAL;
//         }
//     }
//     else
//     {
//         OsConfigLogError(OSQueryLog::Get(), "JSON object does not contain a string array setting");
//         status = EINVAL;
//     }

//     // Deserialize an integer array setting
//     if (document.HasMember(m_integerArraySettingName.c_str()))
//     {
//         if (document[m_integerArraySettingName.c_str()].IsArray())
//         {
//             for (rapidjson::SizeType i = 0; i < document[m_integerArraySettingName.c_str()].Size(); ++i)
//             {
//                 if (document[m_integerArraySettingName.c_str()][i].IsInt())
//                 {
//                     object.integerArraySetting.push_back(document[m_integerArraySettingName.c_str()][i].GetInt());
//                 }
//                 else
//                 {
//                     OsConfigLogError(OSQueryLog::Get(), "Invalid integer in JSON object integer array at position %d", i);
//                     status = EINVAL;
//                 }
//             }
//         }
//         else
//         {
//             OsConfigLogError(OSQueryLog::Get(), "%s is not an array", m_integerArraySettingName.c_str());
//             status = EINVAL;
//         }
//     }
//     else
//     {
//         OsConfigLogError(OSQueryLog::Get(), "JSON object does not contain an integer array setting");
//         status = EINVAL;
//     }

//     // Deserialize a map of strings to strings
//     if (document.HasMember(m_stringMapSettingName.c_str()))
//     {
//         if (document[m_stringMapSettingName.c_str()].IsObject())
//         {
//             for (auto& member : document[m_stringMapSettingName.c_str()].GetObject())
//             {
//                 if (member.value.IsString())
//                 {
//                     object.stringMapSetting[member.name.GetString()] = member.value.GetString();
//                 }
//                 else if (member.value.IsNull())
//                 {
//                     object.stringMapSetting.erase(member.name.GetString());

//                     // Store the removed element key for reporting
//                     object.removedStringMapSettingKeys.push_back(member.name.GetString());
//                 }
//                 else
//                 {
//                     OsConfigLogError(OSQueryLog::Get(), "Invalid string in JSON object string map at key %s", member.name.GetString());
//                     status = EINVAL;
//                 }
//             }
//         }
//         else
//         {
//             OsConfigLogError(OSQueryLog::Get(), "%s is not an object", m_stringMapSettingName.c_str());
//             status = EINVAL;
//         }
//     }
//     else
//     {
//         OsConfigLogError(OSQueryLog::Get(), "JSON object does not contain a string map setting");
//         status = EINVAL;
//     }

//     // Deserialize a map of strings to integers
//     if (document.HasMember(m_integerMapSettingName.c_str()))
//     {
//         if (document[m_integerMapSettingName.c_str()].IsObject())
//         {
//             for (auto& member : document[m_integerMapSettingName.c_str()].GetObject())
//             {
//                 if (member.value.IsInt())
//                 {
//                     object.integerMapSetting[member.name.GetString()] = member.value.GetInt();
//                 }
//                 else if (member.value.IsNull())
//                 {
//                     object.integerMapSetting.erase(member.name.GetString());

//                     // Store the removed element key for reporting
//                     object.removedIntegerMapSettingKeys.push_back(member.name.GetString());
//                 }
//                 else
//                 {
//                     OsConfigLogError(OSQueryLog::Get(), "Invalid integer in JSON object integer map at key %s", member.name.GetString());
//                     status = EINVAL;
//                 }
//             }
//         }
//         else
//         {
//             OsConfigLogError(OSQueryLog::Get(), "%s is not an object", m_integerMapSettingName.c_str());
//             status = EINVAL;
//         }
//     }
//     else
//     {
//         OsConfigLogError(OSQueryLog::Get(), "JSON object does not contain a string integer map setting");
//         status = EINVAL;
//     }

//     return status;
// }

// int OSQuery::DeserializeObjectArray(rapidjson::Document& document, std::vector<Object>& objects)
// {
//     int status = 0;

//     for (rapidjson::Value::ConstValueIterator arrayItem = document.Begin(); arrayItem != document.End(); ++arrayItem)
//     {
//         Object object;
//         rapidjson::Document objectDocument;
//         objectDocument.CopyFrom(*arrayItem, objectDocument.GetAllocator());
//         if (objectDocument.IsObject() && (0 == (status = DeserializeObject(objectDocument, object))))
//         {
//             objects.push_back(object);
//         }
//         else
//         {
//             OsConfigLogError(OSQueryLog::Get(), "Failed to deserialize object");
//             break;
//         }
//     }

//     return status;
// }

// A helper method for serializing a JSON document to a payload string
int OSQuery::SerializeJsonPayload(rapidjson::Document& document, MMI_JSON_STRING* payload, int* payloadSizeBytes, unsigned int maxPayloadSizeBytes)
{
    int status = MMI_OK;
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

    document.Accept(writer);

    if ((0 != maxPayloadSizeBytes) && (buffer.GetSize() > maxPayloadSizeBytes))
    {
        OsConfigLogError(OSQueryLog::Get(), "Failed to serialize JSON object to buffer");
        status = E2BIG;
    }
    else
    {
        status = OSQuery::CopyJsonPayload(buffer, payload, payloadSizeBytes);
    }

    return status;
}

int OSQuery::CopyJsonPayload(rapidjson::StringBuffer& buffer, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MMI_OK;

    *payload = new (std::nothrow) char[buffer.GetSize()];
    if (nullptr == *payload)
    {
        OsConfigLogError(OSQueryLog::Get(), "Unable to allocate memory for payload");
        status = ENOMEM;
    }
    else
    {
        std::fill(*payload, *payload + buffer.GetSize(), 0);
        std::memcpy(*payload, buffer.GetString(), buffer.GetSize());
        *payloadSizeBytes = buffer.GetSize();
    }

    return status;
}

unsigned int OSQuery::GetMaxPayloadSizeBytes()
{
    return m_maxPayloadSizeBytes;
}