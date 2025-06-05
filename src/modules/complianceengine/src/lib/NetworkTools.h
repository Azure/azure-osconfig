#ifndef NETWORKTOOLS_H
#define NETWORKTOOLS_H

#include "ContextInterface.h"
#include "Indicators.h"
#include "Result.h"

#include <arpa/inet.h>
#include <cstdint>
#include <string>
#include <sys/socket.h>

namespace ComplianceEngine
{

class OpenPort
{
public:
    unsigned short family;
    unsigned short type;
    unsigned short port;
    union
    {
        struct in_addr ip4;
        struct in6_addr ip6;
    };
    bool IsLocal() const
    {
        if (AF_INET == family)
        {
            return ((ip4.s_addr & htonl(0xff000000)) == htonl(0x7f000000));
        }
        else if (AF_INET6 == family)
        {
            return (!memcmp(&ip6, &in6addr_loopback, sizeof(ip6)));
        }
        return false;
    }
};

Result<std::vector<OpenPort>> GetOpenPorts(ContextInterface& context);

} // namespace ComplianceEngine

#endif // NETWORKTOOLS_H
