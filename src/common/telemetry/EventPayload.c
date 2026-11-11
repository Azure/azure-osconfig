// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define _POSIX_C_SOURCE 200809L
#include "EventPayload.h"
#include <parson.h>
#include <errno.h>
#include <string.h>

static bool ValidName(const char* name, size_t minimum, bool property)
{
    size_t i = 0;
    size_t size = (NULL == name) ? 0 : strnlen(name, 101);
    if ((size < minimum) || (size > 100) ||
        (property && (('.' == name[0]) || ('.' == name[size - 1]))))
    {
        return false;
    }
    for (i = 0; i < size; ++i)
    {
        unsigned char c = (unsigned char)name[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') || ('_' == c) || ('.' == c)))
        {
            return false;
        }
    }
    return true;
}

int TelemetryPackEvent(const char* name, const TelemetryProperty* properties, size_t count,
    unsigned char* payload, size_t capacity, size_t* size, OsConfigLogHandle log)
{
    int status = EINVAL;
    size_t i = 0;
    size_t j = 0;
    size_t encoded = 0;
    size_t total = 0;
    JSON_Value* event = NULL;
    JSON_Value* fields = NULL;
    JSON_Value* field = NULL;
    JSON_Object* object = NULL;
    JSON_Array* array = NULL;
    char* serialized = NULL;

    if (NULL != size)
    {
        *size = 0;
    }
    if ((NULL == size) || (NULL == payload) || ((NULL == properties) && (0 != count)) ||
        (count > TELEMETRY_MAX_PROPERTY_COUNT) || (0 == capacity) ||
        (capacity > TELEMETRY_MAX_EVENT_SIZE) || !ValidName(name, 4, false))
    {
        goto cleanup;
    }
    for (i = 0; i < count; ++i)
    {
        if ((TelemetryPropertyString != properties[i].type) ||
            !ValidName(properties[i].name, 1, true) || (NULL == properties[i].value.stringValue))
        {
            goto cleanup;
        }
        total += strnlen(properties[i].value.stringValue, TELEMETRY_MAX_EVENT_SIZE);
        if (total >= TELEMETRY_MAX_EVENT_SIZE)
        {
            status = EMSGSIZE;
            goto cleanup;
        }
        for (j = 0; j < i; ++j)
        {
            if (0 == strcmp(properties[j].name, properties[i].name))
            {
                goto cleanup;
            }
        }
    }
    status = ENOMEM;
    event = json_value_init_object();
    fields = json_value_init_array();
    if ((NULL == event) || (NULL == fields))
    {
        goto cleanup;
    }
    object = json_value_get_object(event);
    array = json_value_get_array(fields);
    if ((JSONSuccess != json_object_set_string(object, "EventName", name)) ||
        (JSONSuccess != json_object_set_value(object, "EventFields", fields)))
    {
        goto cleanup;
    }
    fields = NULL;
    for (i = 0; i < count; ++i)
    {
        field = json_value_init_object();
        if (NULL == field)
        {
            goto cleanup;
        }
        object = json_value_get_object(field);
        if ((JSONSuccess != json_object_set_string(object, "FieldName", properties[i].name)) ||
            (JSONSuccess != json_object_set_string(object, "FieldType", "string")) ||
            (JSONSuccess != json_object_set_string(object, "FieldValue", properties[i].value.stringValue)) ||
            (JSONSuccess != json_array_append_value(array, field)))
        {
            goto cleanup;
        }
        field = NULL;
    }
    encoded = json_serialization_size(event);
    if (0 == encoded)
    {
        goto cleanup;
    }
    if ((encoded - 1) > capacity)
    {
        status = EMSGSIZE;
        goto cleanup;
    }
    serialized = json_serialize_to_string(event);
    if (NULL == serialized)
    {
        goto cleanup;
    }
    memcpy(payload, serialized, encoded - 1);
    *size = encoded - 1;
    status = 0;
cleanup:
    json_free_serialized_string(serialized);
    json_value_free(field);
    json_value_free(fields);
    json_value_free(event);
    if (0 != status)
    {
        OsConfigLogInfo(log, "TelemetryPackEvent: JSON event packing failed with %d (%s)", status, strerror(status));
    }
    return status;
}
