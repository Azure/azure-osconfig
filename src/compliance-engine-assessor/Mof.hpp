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
        enum class BenchmarkType
        {
            CIS,
        };

        enum class DistributionType
        {
            Ubuntu,
        };

        struct SemVer {
            int major;
            int minor;
            int patch;

            static ComplianceEngine::Result<SemVer> Parse(const std::string& version);

            SemVer() = delete;
            SemVer& operator=(const SemVer&) = default;
            SemVer(const SemVer&) = default;
            SemVer(SemVer&&) = default;
            SemVer& operator=(SemVer&&) = default;
        };

        struct PayloadKey
        {
            BenchmarkType benchmarkType;
            DistributionType distributionType;
            std::string distributionVersion;
            SemVer version;
            std::string section;

            static ComplianceEngine::Result<PayloadKey> Parse(const std::string& key);

            PayloadKey() = delete;
            PayloadKey& operator=(const PayloadKey&) = default;
            PayloadKey(const PayloadKey&) = default;
            PayloadKey(PayloadKey&&) = default;
            PayloadKey& operator=(PayloadKey&&) = default;
        };

        std::string resourceID;
        PayloadKey payloadKey;
        std::string procedure;
        ComplianceEngine::Optional<std::string> payload;
        std::string ruleName;
        bool hasInitAudit = false;

        MofEntry() = delete;
        MofEntry& operator=(const MofEntry&) = default;
        MofEntry(const MofEntry&) = default;
        MofEntry(MofEntry&&) = default;
        MofEntry& operator=(MofEntry&&) = default;
        MofEntry(std::string resourceID, PayloadKey payloadKey, std::string procedure, ComplianceEngine::Optional<std::string> payload, std::string ruleName, bool hasInitAudit)
            : resourceID(std::move(resourceID)), payloadKey(std::move(payloadKey)), procedure(std::move(procedure)), payload(std::move(payload)), ruleName(std::move(ruleName)), hasInitAudit(hasInitAudit) {}
    };

    ComplianceEngine::Result<MofEntry> ParseSingleEntry(std::istream& stream);
} // namespace mof

#endif // MOF_HPP
