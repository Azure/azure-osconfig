#include "Common.h"

bool IsValidMimObjectPayload(const char* payload, const int payloadSizeBytes, void* log)
{
    UNUSED(log);
    if ((0 == payloadSizeBytes) || (nullptr == payload))
    {
      return false;
    }

    bool isValid = true;

    const char schemaJson[] = R"""({
      "$schema": "http://json-schema.org/draft-04/schema#",
      "description": "MIM object JSON payload schema",
      "definitions": {
        "string": {
          "type": "string"
        },
        "integer": {
          "type": "integer"
        },
        "boolean": {
          "type": "boolean"
        },
        "integerEnumeration": {
          "type": "integer"
        },
        "stringArray": {
          "type": "array",
          "items": {
            "type": "string"
          }
        },
        "integerArray": {
          "type": "array",
          "items": {
            "type": "integer"
          }
        },
        "stringMap": {
          "type": "object",
          "additionalProperties": {
            "type": ["string", "null"]
          }
        },
        "integerMap": {
          "type": "object",
          "additionalProperties": {
            "type": ["integer", "null"]
          }
        },
        "object": {
          "type": "object",
          "additionalProperties": {
            "anyOf": [
              {
                "$ref": "#/definitions/string"
              },
              {
                "$ref": "#/definitions/integer"
              },
              {
                "$ref": "#/definitions/boolean"
              },
              {
                "$ref": "#/definitions/integerEnumeration"
              },
              {
                "$ref": "#/definitions/stringArray"
              },
              {
                "$ref": "#/definitions/integerArray"
              },
              {
                "$ref": "#/definitions/stringMap"
              },
              {
                "$ref": "#/definitions/integerMap"
              }
            ]
          }
        },
        "objectArray": {
          "type": "array",
          "items": {
            "$ref": "#/definitions/object"
          }
        }
      },
      "anyOf": [
        {
          "$ref": "#/definitions/string"
        },
        {
          "$ref": "#/definitions/integer"
        },
        {
          "$ref": "#/definitions/boolean"
        },
        {
          "$ref": "#/definitions/object"
        },
        {
          "$ref": "#/definitions/objectArray"
        },
        {
          "$ref": "#/definitions/stringArray"
        },
        {
          "$ref": "#/definitions/integerArray"
        },
        {
          "$ref": "#/definitions/stringMap"
        },
        {
          "$ref": "#/definitions/integerMap"
        }
      ]
    })""";

    rapidjson::Document sd;
    sd.Parse(schemaJson);
    rapidjson::SchemaDocument schema(sd);
    rapidjson::Document document;

    if (document.Parse(payload, payloadSizeBytes).HasParseError())
    {
        ADD_FAILURE() << "MIM object JSON payload pcannot be parsed";
        isValid = false;
    }
    else
    {
        rapidjson::SchemaValidator validator(schema);
        if (!document.Accept(validator))
        {
            ADD_FAILURE() << "MIM object JSON payload is invalid according to the schema";
            isValid = false;
        }
    }

    if (false == isValid)
    {
        ADD_FAILURE() << "Invalid JSON payload";
    }

    return isValid;
}