// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "TelemetryEncoder.h"

#include <errno.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <string.h>

_Static_assert(8 == CHAR_BIT, "Telemetry encoding requires eight-bit bytes");
_Static_assert((sizeof(double) == sizeof(uint64_t)) && (53 == DBL_MANT_DIG) && (1024 == DBL_MAX_EXP),
    "Telemetry encoding requires IEEE-754 binary64 doubles");

enum TelemetryBondType
{
    TelemetryBondStop = 0,
    TelemetryBondDouble = 8,
    TelemetryBondString = 9,
    TelemetryBondStruct = 10,
    TelemetryBondList = 11,
    TelemetryBondMap = 13,
    TelemetryBondInt32 = 16,
    TelemetryBondInt64 = 17
};

typedef struct TelemetryWriter
{
    unsigned char* buffer;
    size_t capacity;
    size_t size;
    bool failed;
} TelemetryWriter;

typedef struct TelemetryValidatedProperty
{
    const TelemetryProperty* property;
    size_t nameLength;
    size_t stringLength;
} TelemetryValidatedProperty;

typedef struct TelemetryValidatedEvent
{
    const TelemetryEvent* event;
    size_t nameLength;
    size_t iKeyLength;
    size_t deviceIdLength;
    size_t sdkVersionLength;
    size_t sdkEpochLength;
    TelemetryValidatedProperty properties[TELEMETRY_MAX_PROPERTY_COUNT];
} TelemetryValidatedEvent;

static int ValidateString(const char* value, size_t minimumLength, size_t maximumLength,
    size_t* length, const char* field, OsConfigLogHandle log)
{
    size_t i = 0;
    unsigned int remaining = 0;
    uint32_t codePoint = 0;
    uint32_t minimumCodePoint = 0;
    unsigned char byte = '\0';

    *length = 0;

    if (NULL == value)
    {
        if (0 == minimumLength)
        {
            return 0;
        }

        OsConfigLogInfo(log, "TelemetryEncodeEvent: Missing %s", field);
        return EINVAL;
    }

    for (i = 0; '\0' != value[i]; ++i)
    {
        byte = (unsigned char)value[i];

        if (i == maximumLength)
        {
            OsConfigLogInfo(log, "TelemetryEncodeEvent: %s exceeds %zu bytes", field, maximumLength);
            return EMSGSIZE;
        }

        if (0 != remaining)
        {
            if (0x80 != (byte & 0xC0))
            {
                break;
            }

            codePoint = (codePoint << 6) | (byte & 0x3F);
            --remaining;

            if ((0 == remaining) && ((codePoint < minimumCodePoint) || (codePoint > 0x10FFFF) ||
                ((codePoint >= 0xD800) && (codePoint <= 0xDFFF))))
            {
                break;
            }
        }
        else if (byte < 0x80)
        {
            continue;
        }
        else if ((byte >= 0xC2) && (byte <= 0xDF))
        {
            remaining = 1;
            codePoint = byte & 0x1F;
            minimumCodePoint = 0x80;
        }
        else if ((byte >= 0xE0) && (byte <= 0xEF))
        {
            remaining = 2;
            codePoint = byte & 0x0F;
            minimumCodePoint = 0x800;
        }
        else if ((byte >= 0xF0) && (byte <= 0xF4))
        {
            remaining = 3;
            codePoint = byte & 0x07;
            minimumCodePoint = 0x10000;
        }
        else
        {
            break;
        }
    }

    if (('\0' != value[i]) || (0 != remaining))
    {
        OsConfigLogInfo(log, "TelemetryEncodeEvent: Invalid UTF-8 in %s", field);
        return EILSEQ;
    }

    if (i < minimumLength)
    {
        OsConfigLogInfo(log, "TelemetryEncodeEvent: %s is shorter than %zu bytes", field, minimumLength);
        return EINVAL;
    }

    *length = i;
    return 0;
}

static int ValidateName(const char* name, bool property, size_t* length, OsConfigLogHandle log)
{
    size_t i = 0;
    int status = ValidateString(name, property ? 1 : 4, TELEMETRY_MAX_NAME_LENGTH,
        length, property ? "property name" : "event name", log);
    char byte = '\0';

    if (0 != status)
    {
        return status;
    }

    for (i = 0; i < *length; ++i)
    {
        byte = name[i];

        if (!(((byte >= 'a') && (byte <= 'z')) || ((byte >= 'A') && (byte <= 'Z')) ||
            ((byte >= '0') && (byte <= '9')) || ('_' == byte) || ('.' == byte)))
        {
            OsConfigLogInfo(log, "TelemetryEncodeEvent: Invalid character in %s name", property ? "property" : "event");
            return EINVAL;
        }
    }

    if ((property) && (('.' == name[0]) || ('.' == name[*length - 1])))
    {
        OsConfigLogInfo(log, "TelemetryEncodeEvent: Property name starts or ends with a dot");
        return EINVAL;
    }

    return 0;
}

static int ValidateEvent(const TelemetryEvent* event, TelemetryValidatedEvent* validated, OsConfigLogHandle log)
{
    size_t i = 0;
    size_t j = 0;
    int status = 0;
    const TelemetryProperty* property = NULL;
    TelemetryValidatedProperty entry = {0};

    if ((event->time <= 0) || (event->flags < 0) || (event->sequence < 0) ||
        ((0 != event->propertyCount) && (NULL == event->properties)))
    {
        OsConfigLogInfo(log, "TelemetryEncodeEvent: Invalid event time, flags, sequence, or property array");
        return EINVAL;
    }

    if (event->propertyCount > TELEMETRY_MAX_PROPERTY_COUNT)
    {
        OsConfigLogInfo(log, "TelemetryEncodeEvent: Property count exceeds %d", TELEMETRY_MAX_PROPERTY_COUNT);
        return EMSGSIZE;
    }

    validated->event = event;
    status = ValidateName(event->name, false, &validated->nameLength, log);

    if (0 == status)
    {
        status = ValidateString(event->iKey, 3, TELEMETRY_MAX_EVENT_SIZE, &validated->iKeyLength, "iKey", log);
    }

    if (0 != status)
    {
        return status;
    }

    if (('o' != event->iKey[0]) || (':' != event->iKey[1]))
    {
        OsConfigLogInfo(log, "TelemetryEncodeEvent: iKey must use the Common Schema o: prefix");
        return EINVAL;
    }

    status = ValidateString(event->deviceId, 0, TELEMETRY_MAX_EVENT_SIZE,
        &validated->deviceIdLength, "device ID", log);

    if (0 == status)
    {
        status = ValidateString(event->sdkVersion, 0, TELEMETRY_MAX_EVENT_SIZE,
            &validated->sdkVersionLength, "SDK version", log);
    }

    if (0 == status)
    {
        status = ValidateString(event->sdkEpoch, 0, TELEMETRY_MAX_EVENT_SIZE,
            &validated->sdkEpochLength, "SDK epoch", log);
    }

    if (0 != status)
    {
        return status;
    }

    for (i = 0; i < event->propertyCount; ++i)
    {
        property = &event->properties[i];
        entry = (TelemetryValidatedProperty){0};

        entry.property = property;
        status = ValidateName(property->name, true, &entry.nameLength, log);

        if (0 != status)
        {
            return status;
        }

        switch (property->type)
        {
            case TelemetryPropertyString:
                if (NULL == property->value.stringValue)
                {
                    OsConfigLogInfo(log, "TelemetryEncodeEvent: Missing string value at property %zu", i);
                    return EINVAL;
                }

                status = ValidateString(property->value.stringValue, 0, TELEMETRY_MAX_EVENT_SIZE,
                    &entry.stringLength, "property string", log);
                break;

            case TelemetryPropertyDouble:
                if (!isfinite(property->value.doubleValue))
                {
                    OsConfigLogInfo(log, "TelemetryEncodeEvent: Non-finite double at property %zu", i);
                    return EINVAL;
                }
                break;

            case TelemetryPropertyInt64:
            case TelemetryPropertyBoolean:
                break;

            default:
                OsConfigLogInfo(log, "TelemetryEncodeEvent: Unsupported type at property %zu", i);
                return EINVAL;
        }

        if (0 != status)
        {
            return status;
        }

        for (j = i; (j > 0) && (strcmp(validated->properties[j - 1].property->name, property->name) > 0); --j)
        {
            validated->properties[j] = validated->properties[j - 1];
        }

        if ((j > 0) && (0 == strcmp(validated->properties[j - 1].property->name, property->name)))
        {
            OsConfigLogInfo(log, "TelemetryEncodeEvent: Duplicate property name at property %zu", i);
            return EINVAL;
        }

        validated->properties[j] = entry;
    }

    return 0;
}

static void WriteBytes(TelemetryWriter* writer, const void* bytes, size_t size)
{
    if (writer->failed)
    {
        return;
    }

    if (size > writer->capacity - writer->size)
    {
        writer->failed = true;
        return;
    }

    if ((NULL != writer->buffer) && (0 != size))
    {
        memcpy(writer->buffer + writer->size, bytes, size);
    }

    writer->size += size;
}

static void WriteByte(TelemetryWriter* writer, unsigned char value)
{
    WriteBytes(writer, &value, sizeof(value));
}

static void WriteUnsigned(TelemetryWriter* writer, uint64_t value)
{
    while (value >= 0x80)
    {
        WriteByte(writer, (unsigned char)((value & 0x7F) | 0x80));
        value >>= 7;
    }

    WriteByte(writer, (unsigned char)value);
}

static void WriteSigned(TelemetryWriter* writer, int64_t value)
{
    uint64_t encoded = ((uint64_t)value << 1) ^ (UINT64_C(0) - (uint64_t)(value < 0));
    WriteUnsigned(writer, encoded);
}

static void WriteField(TelemetryWriter* writer, unsigned char type, unsigned char id)
{
    if (id <= 5)
    {
        WriteByte(writer, (unsigned char)(type | (id << 5)));
    }
    else
    {
        WriteByte(writer, (unsigned char)(type | 0xC0));
        WriteByte(writer, id);
    }
}

static void WriteString(TelemetryWriter* writer, const char* value, size_t length)
{
    WriteUnsigned(writer, length);
    WriteBytes(writer, value, length);
}

static void WriteStringField(TelemetryWriter* writer, unsigned char id, const char* value, size_t length)
{
    if (0 != length)
    {
        WriteField(writer, TelemetryBondString, id);
        WriteString(writer, value, length);
    }
}

static void WriteInt64Field(TelemetryWriter* writer, unsigned char id, int64_t value)
{
    if (0 != value)
    {
        WriteField(writer, TelemetryBondInt64, id);
        WriteSigned(writer, value);
    }
}

static void WriteStructList(TelemetryWriter* writer, unsigned char id)
{
    WriteField(writer, TelemetryBondList, id);
    WriteByte(writer, TelemetryBondStruct);
    WriteUnsigned(writer, 1);
}

static void WriteProperty(TelemetryWriter* writer, const TelemetryValidatedProperty* entry)
{
    const TelemetryProperty* property = entry->property;
    int kind = 5;
    uint64_t bits = 0;
    size_t i = 0;

    WriteString(writer, property->name, entry->nameLength);

    switch (property->type)
    {
        case TelemetryPropertyInt64:
            kind = 0;
            break;
        case TelemetryPropertyDouble:
            kind = 4;
            break;
        case TelemetryPropertyBoolean:
            kind = 6;
            break;
        case TelemetryPropertyString:
            break;
    }

    if (5 != kind)
    {
        WriteField(writer, TelemetryBondInt32, 1);
        WriteSigned(writer, kind);
    }

    switch (property->type)
    {
        case TelemetryPropertyString:
            WriteStringField(writer, 3, property->value.stringValue, entry->stringLength);
            break;

        case TelemetryPropertyInt64:
            WriteInt64Field(writer, 4, property->value.int64Value);
            break;

        case TelemetryPropertyBoolean:
            WriteInt64Field(writer, 4, property->value.booleanValue ? 1 : 0);
            break;

        case TelemetryPropertyDouble:
            if (0.0 != property->value.doubleValue)
            {

                memcpy(&bits, &property->value.doubleValue, sizeof(bits));
                WriteField(writer, TelemetryBondDouble, 5);

                for (i = 0; i < sizeof(bits); ++i)
                {
                    WriteByte(writer, (unsigned char)(bits >> (i * 8)));
                }
            }
            break;
    }

    WriteByte(writer, TelemetryBondStop);
}

static void WriteEvent(TelemetryWriter* writer, const TelemetryValidatedEvent* validated)
{
    const TelemetryEvent* event = validated->event;
    size_t i = 0;

    WriteStringField(writer, 1, "3.0", 3);
    WriteStringField(writer, 2, event->name, validated->nameLength);
    WriteInt64Field(writer, 3, event->time);
    WriteStringField(writer, 5, event->iKey, validated->iKeyLength);
    WriteInt64Field(writer, 6, event->flags);

    if (0 != validated->deviceIdLength)
    {
        WriteStructList(writer, 23);
        WriteStringField(writer, 2, event->deviceId, validated->deviceIdLength);
        WriteByte(writer, TelemetryBondStop);
    }

    if ((0 != validated->sdkVersionLength) || (0 != validated->sdkEpochLength) || (0 != event->sequence))
    {
        WriteStructList(writer, 32);
        WriteStringField(writer, 1, event->sdkVersion, validated->sdkVersionLength);
        WriteStringField(writer, 2, event->sdkEpoch, validated->sdkEpochLength);
        WriteInt64Field(writer, 3, event->sequence);
        WriteByte(writer, TelemetryBondStop);
    }

    WriteStringField(writer, 60, "custom", 6);
    WriteStructList(writer, 70);

    if (0 != event->propertyCount)
    {
        WriteField(writer, TelemetryBondMap, 1);
        WriteByte(writer, TelemetryBondString);
        WriteByte(writer, TelemetryBondStruct);
        WriteUnsigned(writer, event->propertyCount);

        for (i = 0; i < event->propertyCount; ++i)
        {
            WriteProperty(writer, &validated->properties[i]);
        }
    }

    WriteByte(writer, TelemetryBondStop);
    WriteByte(writer, TelemetryBondStop);
}

int TelemetryEncodeEvent(const TelemetryEvent* event, unsigned char* buffer,
    size_t capacity, size_t* encodedSize, OsConfigLogHandle log)
{
    TelemetryValidatedEvent validated = {0};
    TelemetryWriter writer = {0};
    int status = 0;

    if (NULL != encodedSize)
    {
        *encodedSize = 0;
    }

    if ((NULL == event) || (NULL == buffer) || (NULL == encodedSize))
    {
        OsConfigLogInfo(log, "TelemetryEncodeEvent: Invalid event, buffer, or size output");
        return EINVAL;
    }

    status = ValidateEvent(event, &validated, log);

    if (0 != status)
    {
        return status;
    }

    // Measure first so failure never leaves a partially encoded output buffer.
    writer.capacity = (capacity < TELEMETRY_MAX_EVENT_SIZE) ? capacity : TELEMETRY_MAX_EVENT_SIZE;
    WriteEvent(&writer, &validated);

    if (writer.failed)
    {
        OsConfigLogInfo(log, "TelemetryEncodeEvent: Event exceeds buffer capacity or the %d-byte event limit",
            TELEMETRY_MAX_EVENT_SIZE);
        return EMSGSIZE;
    }

    writer.buffer = buffer;
    writer.size = 0;
    WriteEvent(&writer, &validated);

    *encodedSize = writer.size;
    OsConfigLogDebug(log, "TelemetryEncodeEvent: Encoded %zu properties into %zu bytes", event->propertyCount, writer.size);
    return 0;
}
