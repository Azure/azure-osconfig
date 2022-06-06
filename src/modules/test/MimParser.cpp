#include "Common.h"

static const std::string g_array = "array";
static const std::string g_contents = "contents";
static const std::string g_desired = "desired";
static const std::string g_elementSchema = "elementSchema";
static const std::string g_elementSchema_Fields = "elementSchema.fields";
static const std::string g_elementSchema_Type = "elementSchema.type";
static const std::string g_enum = "enum";
static const std::string g_enumValue = "enumValue";
static const std::string g_enumValues = "enumValues";
static const std::string g_fields = "fields";
static const std::string g_map = "map";
static const std::string g_mapKey_Schema = "mapKey.schema";
static const std::string g_mapValue_Schema = "mapValue.schema";
static const std::string g_mimModel = "MimModel";
static const std::string g_mimObject = "mimObject";
static const std::string g_name = "name";
static const std::string g_object = "object";
static const std::string g_schema = "schema";
static const std::string g_type = "type";
static const std::string g_valueSchema = "valueSchema";

pMimObjects MimParser::ParseMim(std::string path)
{
    JSON_Value *root_value;
    root_value = json_parse_file(path.c_str());

    pMimObjects mimObjects = std::make_shared<MimObjects>();

    if (json_value_get_type(root_value) != JSONObject)
    {
        json_value_free(root_value);
        return mimObjects;
    }

    JSON_Object *root_object = json_value_get_object(root_value);
    JSON_Array *components = json_object_get_array(root_object, g_contents.c_str());
    JSON_Array *jsonMimObjects = nullptr;

    for (size_t i = 0; i < json_array_get_count(components); i++)
    {
        // Components
        root_object = json_array_get_object(components, i);

        std::string componentName = json_object_get_string(root_object, g_name.c_str());

        jsonMimObjects = json_object_get_array(root_object, g_contents.c_str());
        for (size_t y = 0; y < json_array_get_count(jsonMimObjects); y++)
        {
            root_object = json_array_get_object(jsonMimObjects, y);

            if (0 == strcmp(json_object_get_string(root_object, g_type.c_str()), g_mimObject.c_str()))
            {
                MimObject mim = {
                    json_object_get_string(root_object, g_name.c_str()),
                    json_object_get_string(root_object, g_type.c_str()),
                    !!json_object_get_boolean(root_object, g_desired.c_str()),
                    std::make_shared<std::map<std::string, MimSetting>>()};

                if (json_object_has_value_of_type(root_object, g_schema.c_str(), JSONObject))
                {
                    JSON_Object *schema_object = json_object_get_object(root_object, g_schema.c_str());

                    JSON_Array *jsonSettings = nullptr;
                    if (nullptr != json_object_get_string(schema_object, g_type.c_str()) &&
                        (0 == strcmp(json_object_get_string(schema_object, g_type.c_str()), g_array.c_str())) &&
                        (0 == strcmp(json_object_dotget_string(schema_object, g_elementSchema_Type.c_str()), g_object.c_str())))
                    {
                        mim.m_type = json_object_get_string(schema_object, g_type.c_str()),
                        jsonSettings = json_object_dotget_array(schema_object, g_elementSchema_Fields.c_str());
                    }
                    else
                    {
                        jsonSettings = json_object_get_array(schema_object, g_fields.c_str());
                    }

                    for (size_t z = 0; z < json_array_get_count(jsonSettings); z++)
                    {
                        MimParser::ParseMimSetting(json_array_get_object(jsonSettings, z), mim);
                    }
                }

                std::shared_ptr<std::map<std::string, MimObject>> mimObjectsPtr = nullptr;
                try
                {
                    mimObjectsPtr = mimObjects->at(componentName);
                }
                catch (const std::out_of_range &e)
                {
                    (*mimObjects)[componentName] = std::make_shared<std::map<std::string, MimObject>>();
                    mimObjectsPtr = (*mimObjects)[componentName];
                }

                (*mimObjectsPtr)[mim.m_name] = mim;
            }
        }
    }

    json_value_free(root_value);
    return mimObjects;
}

void MimParser::ParseMimSetting(JSON_Object* jsonField, MimObject& mimObject)
{
    MimSetting mimField;
    if (json_object_has_value_of_type(jsonField, g_schema.c_str(), JSONString) ||
        json_object_has_value_of_type(jsonField, g_schema.c_str(), JSONNumber) ||
        json_object_has_value_of_type(jsonField, g_schema.c_str(), JSONBoolean))
    {
        mimField = {
            json_object_get_string(jsonField, g_name.c_str()),
            json_object_get_string(jsonField, g_schema.c_str()),
            std::make_shared<std::vector<std::string>>()};
    }
    else if (json_object_has_value_of_type(jsonField, g_schema.c_str(), JSONObject))
    {
        JSON_Object *jsonSchema = json_object_get_object(jsonField, g_schema.c_str());

        if (0 == strcmp(json_object_get_string(jsonSchema, g_type.c_str()), g_enum.c_str()))
        {
            mimField = {
                json_object_get_string(jsonField, g_name.c_str()),
                json_object_get_string(jsonSchema, g_valueSchema.c_str()),
                std::make_shared<std::vector<std::string>>()};

            // Add supported values
            if (json_object_has_value_of_type(jsonSchema, g_enumValues.c_str(), JSONArray))
            {
                JSON_Array *supportedValues = json_object_get_array(jsonSchema, g_enumValues.c_str());
                for (size_t a = 0; a < json_array_get_count(supportedValues); a++)
                {
                    JSON_Object *jsonEnumValues = json_array_get_object(supportedValues, a);
                    mimField.allowedValues->push_back(std::to_string(json_object_get_number(jsonEnumValues, g_enumValue.c_str())));
                }
            }
        }
        else if (0 == strcmp(json_object_get_string(jsonSchema, g_type.c_str()), g_array.c_str()))
        {
            if (json_object_has_value_of_type(jsonSchema, g_elementSchema.c_str(), JSONObject))
            {
                MimParser::ParseMimSetting(json_object_get_object(jsonField, g_elementSchema.c_str()), mimObject);
            }
            else
            {
                mimField = {
                    json_object_get_string(jsonField, g_name.c_str()),
                    std::string("array-") + json_object_dotget_string(jsonField, "schema.elementSchema"),
                    std::make_shared<std::vector<std::string>>()};
            }
        }
        else if (0 == strcmp(json_object_get_string(jsonSchema, g_type.c_str()), g_map.c_str()))
        {
            const char* keySchema = json_object_dotget_string(jsonSchema, g_mapKey_Schema.c_str());
            const char* valueSchema = json_object_dotget_string(jsonSchema, g_mapValue_Schema.c_str());

            if (nullptr == keySchema || nullptr == valueSchema)
            {
                TestLogError("Missing key or value schema for map field '%s'", json_object_get_string(jsonSchema, g_name.c_str()));
            }

            mimField = {
                json_object_get_string(jsonField, g_name.c_str()),
                std::string("map-") + keySchema + "-" + valueSchema,
                std::make_shared<std::vector<std::string>>()};
        }
        else
        {
            TestLogError("Invalid type '%s'", json_object_get_string(jsonSchema, g_type.c_str()));
        }
    }

    (*mimObject.m_settings)[mimField.name] = mimField;
}