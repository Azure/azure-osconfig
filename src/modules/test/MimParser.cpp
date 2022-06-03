#include "MimParser.h"

static const std::string g_mimModel = "MimModel";
static const std::string g_mimObject = "mimObject";

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
    JSON_Array *components = json_object_get_array(root_object, "contents");
    JSON_Array *jsonMimObjects = nullptr;

    for (size_t i = 0; i < json_array_get_count(components); i++)
    {
        // Components
        root_object = json_array_get_object(components, i);

        std::string componentName = json_object_get_string(root_object, "name");

        jsonMimObjects = json_object_get_array(root_object, "contents");
        for (size_t y = 0; y < json_array_get_count(jsonMimObjects); y++)
        {
            root_object = json_array_get_object(jsonMimObjects, y);

            if (0 == strcmp(json_object_get_string(root_object, "type"), g_mimObject.c_str()))
            {
                MimObject mim = {
                    json_object_get_string(root_object, "name"),
                    json_object_get_string(root_object, "type"),
                    !!json_object_get_boolean(root_object, "desired"),
                    std::make_shared<std::map<std::string, MimSetting>>()};

                if (json_object_has_value_of_type(root_object, "schema", JSONObject))
                {
                    JSON_Object *schema_object = json_object_get_object(root_object, "schema");

                    JSON_Array *jsonSettings = nullptr;
                    if (nullptr == json_object_get_string(schema_object, "type"))
                    {
                        throw std::runtime_error("MimObject schema missing 'type'");
                    }
                    else if ((0 == strcmp(json_object_get_string(schema_object, "type"), "array")) &&
                        (0 == strcmp(json_object_dotget_string(schema_object, "elementSchema.type"), "object")))
                    {
                        mim.m_type = json_object_get_string(schema_object, "type"),
                        jsonSettings = json_object_dotget_array(schema_object, "elementSchema.fields");
                    }
                    else
                    {
                        jsonSettings = json_object_get_array(schema_object, "fields");
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
    if (json_object_has_value_of_type(jsonField, "schema", JSONString) ||
        json_object_has_value_of_type(jsonField, "schema", JSONNumber) ||
        json_object_has_value_of_type(jsonField, "schema", JSONBoolean))
    {
        mimField = {
            json_object_get_string(jsonField, "name"),
            json_object_get_string(jsonField, "schema"),
            std::make_shared<std::vector<std::string>>()};
    }
    else if (json_object_has_value_of_type(jsonField, "schema", JSONObject))
    {
        JSON_Object *jsonSchema = json_object_get_object(jsonField, "schema");

        if (0 == strcmp(json_object_get_string(jsonSchema, "type"), "enum"))
        {
            mimField = {
                json_object_get_string(jsonField, "name"),
                json_object_get_string(jsonSchema, "valueSchema"),
                std::make_shared<std::vector<std::string>>()};

            // Add supported values
            if (json_object_has_value_of_type(jsonSchema, "enumValues", JSONArray))
            {
                JSON_Array *supportedValues = json_object_get_array(jsonSchema, "enumValues");
                for (size_t a = 0; a < json_array_get_count(supportedValues); a++)
                {
                    JSON_Object *jsonEnumValues = json_array_get_object(supportedValues, a);
                    mimField.allowedValues->push_back(std::to_string(json_object_get_number(jsonEnumValues, "enumValue")));
                }
            }
        }
        else if (0 == strcmp(json_object_get_string(jsonSchema, "type"), "array"))
        {
            if (json_object_has_value_of_type(jsonSchema, "elementSchema", JSONObject))
            {
                MimParser::ParseMimSetting(json_object_get_object(jsonField, "elementSchema"), mimObject);
            }
            else
            {
                mimField = {
                    json_object_get_string(jsonField, "name"),
                    std::string("array-") + json_object_dotget_string(jsonField, "schema.elementSchema"),
                    std::make_shared<std::vector<std::string>>()};
            }
        }
        else if (0 == strcmp(json_object_get_string(jsonSchema, "type"), "map"))
        {
            const char* keySchema = json_object_dotget_string(jsonSchema, "mapKey.schema");
            const char* valueSchema = json_object_dotget_string(jsonSchema, "mapValue.schema");

            if (nullptr == keySchema || nullptr == valueSchema)
            {
                TestLogError("Missing key or value schema for map field '%s'", json_object_get_string(jsonSchema, "name"));
            }

            mimField = {
                json_object_get_string(jsonField, "name"),
                std::string("map-") + keySchema + "-" + valueSchema,
                std::make_shared<std::vector<std::string>>()};
        }
        else
        {
            TestLogError("Invalid type '%s'", json_object_get_string(jsonSchema, "type"));
        }
    }

    (*mimObject.m_settings)[mimField.name] = mimField;
}