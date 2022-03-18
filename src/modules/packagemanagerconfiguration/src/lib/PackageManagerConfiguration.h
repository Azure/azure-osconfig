// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <PackageManagerConfigurationBase.h>

class PackageManagerConfiguration : public PackageManagerConfigurationBase
{
public:
    PackageManagerConfiguration(unsigned int maxPayloadSizeBytes);
    ~PackageManagerConfiguration() = default;
};