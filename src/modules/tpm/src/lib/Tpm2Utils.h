// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <ScopeGuard.h>
#include <Tpm.h>

#define INT_MAX 0x7FFFFFF
#define TPM_RESPONSE_MAX_SIZE 4096
#define TPM_COMMUNICATION_ERROR -1
#define TPM_PATH "/dev/tpm0"

static const uint8_t g_getTpmProperties[] =
{
    0x80, 0x01, // TPM_ST_NO_SESSIONS
    0x00, 0x00, 0x00, 0x16, // commandSize
    0x00, 0x00, 0x01, 0x7A, // TPM_CC_GetCapability
    0x00, 0x00, 0x00, 0x06, // TPM_CAP_TPM_PROPERTIES
    0x00, 0x00, 0x01, 0x00, // Property: TPM_PT_FAMILY_INDICATOR
    0x00, 0x00, 0x00, 0x66  // propertyCount (102)
};

class Tpm2Utils
{
public:
    static int UnsignedInt8ToUnsignedInt64(uint8_t* inputBuf, uint32_t inputBufSize, uint32_t dataOffset, uint32_t dataLength, uint64_t* output)
    {
        int status = MMI_OK;
        uint32_t i = 0;
        uint64_t temp = 0;

        if (nullptr == inputBuf)
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(TpmLog::Get(), "Invalid argument, inputBuf is null");
            }
            status = EINVAL;
        }
        if ((nullptr == output) && (status == MMI_OK))
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(TpmLog::Get(), "Invalid argument, output is null");
            }
            status = EINVAL;
        }
        if ((dataOffset >= inputBufSize) && (status == MMI_OK))
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(TpmLog::Get(), "Invalid argument, inputBufSize %u must be greater than dataOffset %u", inputBufSize, dataOffset);
            }
            status = EINVAL;
        }
        if ((INT_MAX < inputBufSize) && (status == MMI_OK))
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(TpmLog::Get(), "Invalid argument, inputBufSize %u must be less than or equal to %u", inputBufSize, INT_MAX);
            }
            status = EINVAL;
        }
        if ((0 >= dataLength) && (status == MMI_OK))
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(TpmLog::Get(), "Invalid argument, dataLength %u must greater than 0", dataLength);
            }
            status = EINVAL;
        }
        if ((dataLength > (inputBufSize - dataOffset)) && (status == MMI_OK))
        {
            if (IsFullLoggingEnabled())
            {   
                OsConfigLogError(TpmLog::Get(), "Invalid argument, dataLength %u must be less than or equal to %i", dataLength, inputBufSize - dataOffset);
            }
            status = EINVAL;
        }
        if ((sizeof(uint64_t) < dataLength) && (status == MMI_OK))
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(TpmLog::Get(), "Invalid argument, input buffer dataLength remaining from dataOffset must be less than %zu", sizeof(uint64_t));
            }
            status = EINVAL;
        }
        
        if (status == MMI_OK)
        {
            *output = 0;
            for (i = 0; i < dataLength; i++)
            {
                temp = temp << 8; // Make space to add the next byte of data from the input buffer
                temp += inputBuf[dataOffset + i];
            }
            *output = temp;
        }

        return status;
    }

    static int BufferToString(unsigned char* buf, std::string& str)
    {
        int status = MMI_OK;
        if (nullptr == buf)
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(TpmLog::Get(), "Invalid argument, buf is null");
            }
            status = EINVAL;
        }
        else
        {
            std::ostringstream os;
            os << buf;
            if (os.good())
            {
                str = os.str();
            }
            else
            {
                if (IsFullLoggingEnabled())
                {
                    OsConfigLogError(TpmLog::Get(), "Error populating std::ostringstream");
                }
                status = ENOMEM;
            }
        }

        return status; 
    }

    static int GetTpmPropertyFromBuffer(uint8_t* buf, ssize_t bufSize, const char* objectName, std::string& tpmProperty)
    {
        int status = MMI_OK;
        uint64_t tpmPropertyKey = 0;

        if (nullptr == buf)
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(TpmLog::Get(), "Invalid argument, buf is null");
            }
            status = EINVAL;
        }
        if ((nullptr == objectName) && (status == MMI_OK))
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(TpmLog::Get(), "Invalid argument, objectName is null");
            }
            status = EINVAL;
        }

        if (status == MMI_OK)
        {
            for(int n = 0x13; n < (bufSize - 8); n += 8)
            {
                if (MMI_OK != (status = UnsignedInt8ToUnsignedInt64(buf, TPM_RESPONSE_MAX_SIZE, n, 4, &tpmPropertyKey)))
                {
                    break;
                }

                unsigned char nullTerminator = '\0';

                switch(tpmPropertyKey)
                {
                    case 0x100:
                    {
                        if (0 == std::strcmp(objectName, TPM_VERSION))
                        {
                            unsigned char tpmPropertyBuffer[5] = {buf[n + 4], buf[n + 5], buf[n + 6], buf[n + 7], nullTerminator};
                            status = BufferToString(tpmPropertyBuffer, tpmProperty);
                        }
                        break;
                    }
                    case 0x100+5:
                    {
                        if (0 == std::strcmp(objectName, TPM_MANUFACTURER))
                        {
                            unsigned char tpmPropertyBuffer[5] = {buf[n + 4], buf[n + 5], buf[n + 6], buf[n + 7], nullTerminator};
                            status = BufferToString(tpmPropertyBuffer, tpmProperty);
                        }
                        break;
                    }
                    default:
                        break;
                }

                if ((!tpmProperty.empty()) || (status != MMI_OK))
                {
                    break;
                }
            }
        }

        return status;
    }

    static int GetTpmPropertyFromDeviceFile(const char* objectName, std::string& tpmProperty)
    {
        int status = MMI_OK;
        int tpm = TPM_COMMUNICATION_ERROR;
        ssize_t requestSize = sizeof(g_getTpmProperties);
        ssize_t responseSize = TPM_RESPONSE_MAX_SIZE;
        uint8_t* response = (uint8_t*)malloc(TPM_RESPONSE_MAX_SIZE);

        ScopeGuard sg{[&]()
        {
            if (nullptr != response)
            {
                memset(response, 0, TPM_RESPONSE_MAX_SIZE);
                free(response);
                response = nullptr;
            }
        }};

        if (nullptr == response)
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(TpmLog::Get(), "Insufficient buffer space available to allocate %d bytes", TPM_RESPONSE_MAX_SIZE);
            }
            status = ENOMEM;
        }

        if (status == MMI_OK)
        {
            memset(response, 0, TPM_RESPONSE_MAX_SIZE);

            tpm = open(TPM_PATH, O_RDWR);
            if (TPM_COMMUNICATION_ERROR == tpm)
            {
                if (IsFullLoggingEnabled())
                {
                    OsConfigLogError(TpmLog::Get(), "Error opening the device");
                }
                status = errno;
            }
        }

        if (status == MMI_OK)
        {
            responseSize = write(tpm, g_getTpmProperties, requestSize);
            if ((responseSize == TPM_COMMUNICATION_ERROR) || (requestSize != responseSize))
            {
                if (IsFullLoggingEnabled())
                {
                    OsConfigLogError(TpmLog::Get(), "Error sending request to the device");
                }
                status = errno;
            }
        }

        if (status == MMI_OK)
        {
            responseSize = read(tpm, response, TPM_RESPONSE_MAX_SIZE);
            if (responseSize == TPM_COMMUNICATION_ERROR)
            {
                if (IsFullLoggingEnabled())
                {
                    OsConfigLogError(TpmLog::Get(), "Error reading response from the device");
                }
                status = errno;
            }
        }

        if ((TPM_COMMUNICATION_ERROR != tpm) && (status == MMI_OK))
        {
            close(tpm);
            tpm = TPM_COMMUNICATION_ERROR;
        }

        if (status == MMI_OK)
        {
            status = GetTpmPropertyFromBuffer(response, responseSize, objectName, tpmProperty);
        }

        return status;
    }
};