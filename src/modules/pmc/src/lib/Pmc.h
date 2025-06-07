// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <PmcBase.h>

class Pmc : public PmcBase
{
public:
    Pmc(unsigned int maxPayloadSizeBytes);
    ~Pmc() = default;
private:
    int RunCommand(const char* command, std::string* textResult, bool isLongRunning = false) override;
    std::string GetPackagesFingerprint() override;
    std::string GetSourcesFingerprint(const char* sourcesDirectory) override;
    bool CanRunOnThisPlatform() override;
};
