// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef NETWORKTOOLS_H
#define NETWORKTOOLS_H

#include "ContextInterface.h"
#include "Result.h"

#include <arpa/inet.h>
#include <string>
#include <vector>

namespace ComplianceEngine
{

class OpenPort
{
public:
    unsigned short family;
    unsigned short type;
    unsigned short port;
    std::string interface;
    union
    {
        struct in_addr ip4;
        struct in6_addr ip6;
    };
    bool IsLocal() const;
};

Result<std::vector<OpenPort>> GetOpenPorts(ContextInterface& context);

} // namespace ComplianceEngine

#endif // NETWORKTOOLS_H
