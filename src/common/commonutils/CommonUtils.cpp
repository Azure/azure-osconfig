// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"

bool IsValidClientName(const char* name)
{
    // "Azure OSConfig <model version>;<major>.<minor>.<patch>.<yyyymmdd><build>"
    const std::string productInfoTemplate = "^((Azure OSConfig )([0-9]+);(0|[1-9]\\d*)\\.(0|[1-9]\\d*)\\.(0|[1-9]\\d*)\\.([0-9]{8})).*$";
    const std::string clientNamePrefix = "Azure OSConfig ";
    const std::string modelVersionDelimiter = ";";
    const std::string semanticVersionDelimeter = ".";

    // OSConfig model version 5 published on September 27, 2021
    const int referenceModelVersion = 5;
    const int referenceReleaseDay = 27;
    const int referenceReleaseMonth = 9;
    const int referenceReleaseYear = 2021;

    // String length of date string yyyymmmdd
    const int dateLength = 9;
    
    bool isValid = true;

    const std::string clientName = name;

    // Regex for validating the client name against the OSConfig product info
    std::regex pattern(productInfoTemplate);

    if (!clientName.empty() && std::regex_match(clientName, pattern))
    {
        std::string versionInfo = clientName.substr(clientNamePrefix.length());
        std::string modelVersion = versionInfo.substr(0, versionInfo.find(modelVersionDelimiter));

        int modelVersionNumber = std::stoi(modelVersion);
        if (modelVersionNumber < referenceModelVersion)
        {
            isValid = false;
        }

        // Get build date from versionInfo
        int position = 0;
        for (int i = 0; i < 3; i++)
        {
            position = versionInfo.find(semanticVersionDelimeter, position + 1);
        }

        std::string buildDate = versionInfo.substr(position + 1, position + dateLength);
        int year = std::stoi(buildDate.substr(0, 4));
        int month = std::stoi(buildDate.substr(4, 2));
        int day = std::stoi(buildDate.substr(6, 2));

        if ((month < 1) || (month > 12) || (day < 1) || (day > 31))
        {
            isValid = false;
        }

        char dateNow[dateLength] = {0};
        int monthNow = 0, dayNow = 0, yearNow = 0;
        time_t t = time(0);
        strftime(dateNow, dateLength, "%Y%m%d", localtime(&t));
        sscanf(dateNow, "%4d%2d%2d", &yearNow, &monthNow, &dayNow);

        // Check if the build date is in the future
        if ((yearNow < year) || ((yearNow == year) && ((monthNow < month) || ((monthNow == month) && (dayNow < day)))))
        {
            isValid = false;
        }

        // Check if the build date is from the past - before the reference release date
        if ((year < referenceReleaseYear) || ((year == referenceReleaseYear) && ((month < referenceReleaseMonth) || ((month == referenceReleaseMonth) && (day < referenceReleaseDay)))))
        {
            isValid = false;
        }
    }
    else
    {
        isValid = false;
    }

    return isValid;
}

bool IsValidMimObjectPayload(const char* payload, const int payloadSizeBytes, void* log)
{
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
        "stringEnumeration": {
          "type": "string"
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
                "$ref": "#/definitions/stringEnumeration"
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
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(log, "MIM object JSON payload pcannot be parsed");
        }
        isValid = false;
    }
    else
    {
        rapidjson::SchemaValidator validator(schema);
        if (!document.Accept(validator))
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(log, "MIM object JSON payload is invalid according to the schema");
            }
            isValid = false;
        }
    }

    if (IsFullLoggingEnabled() && (false == isValid))
    {
        OsConfigLogError(log, "Invalid JSON payload: '%.*s' (%d bytes)", payloadSizeBytes, payload, payloadSizeBytes);
    }

    return isValid;
}