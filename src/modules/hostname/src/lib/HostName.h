// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <string>
#include <HostNameBase.h>

class HostName : public HostNameBase
{
public:
    HostName(size_t maxPayloadSizeBytes);
    ~HostName();

    int RunCommand(const char* command, bool replaceEol, std::string *textResult) override;

    static inline void MmiFreeInternal(MMI_JSON_STRING payload)
    {
        if (payload)
        {
            delete[] payload;
        }
    }
};