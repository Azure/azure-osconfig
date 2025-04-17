#ifndef MOF_HPP
#define MOF_HPP

#include <string>
#include <istream>
#include <Engine.h>
#include <Optional.h>
#include <Result.h>

namespace mof
{
    struct MofEntry
    {
        std::string resourceID;
        std::string payloadKey;
        compliance::Optional<std::string> procedure;
        std::string payload;
        std::string ruleName;
        bool hasInitAudit = false;
    };

    compliance::Result<MofEntry> ParseSingleEntry(std::istream& stream);
} // namespace mof

#endif // MOF_HPP
