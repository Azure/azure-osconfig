// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef MANAGEMENTMODULE_H
#define MANAGEMENTMODULE_H

#include <string>
#include <map>
#include <memory>
#include <sstream>
#include <Mmi.h>

// MMI function definitions
using MmiGetInfo_ptr = int (*)(const char *, MMI_JSON_STRING *, int *);
using MmiFree_ptr = void (*)(MMI_JSON_STRING);
using MmiOpen_ptr = MMI_HANDLE (*)(const char *, const unsigned int);
using MmiSet_ptr = int (*)(MMI_HANDLE, const char *, const char *, const MMI_JSON_STRING, const int);
using MmiGet_ptr = int (*)(MMI_HANDLE, const char *, const char *, const MMI_JSON_STRING *, int *);
using MmiClose_ptr = void (*)(MMI_HANDLE);

class ManagementModule
{
public:
    // Lifetime of the operation - see MmiGetInfo Schema for Lifetime property
    enum Lifetime
    {
        Undefined = 0,
        KeepAlive = 1,
        Short = 2
    };

    struct Version
    {
        unsigned int Major = 0;
        unsigned int Minor = 0;
        unsigned int Patch = 0;
        unsigned int Tweak = 0;

        // Only defining "<" as we are only using that operator in comparisons
        bool operator < (const Version &rhs) const
        {
            return (((Major < rhs.Major) ||
                     ((Major == rhs.Major) && (Minor < rhs.Minor)) ||
                     ((Major == rhs.Major) && (Minor == rhs.Minor) && (Patch < rhs.Patch)) ||
                     ((Major == rhs.Major) && (Minor == rhs.Minor) && (Patch == rhs.Patch) && (Tweak < rhs.Tweak)))
                        ? true : false);
        }

        std::string ToString() const
        {
            std::ostringstream ostream;
            ostream << Major << "." << Minor << "." << Patch << "." << Tweak;
            return ostream.str();
        }
    };

    // Structure maps to the GetMMIInfo JSON Schema - see src/modules/schema/MmiGetInfoSchema.json for more info
    struct Info
    {
        std::string name;
        std::string description;
        std::string manufacturer;
        Version version;
        std::string versionInfo;
        std::vector<std::string> components;
        std::map<std::string, std::vector<std::string>> reportedObjects;
        Lifetime lifetime;
        std::string licenseUri;
        std::string projectUri;
        unsigned int userAccount;
    };

    ManagementModule(const std::string clientName, const std::string path, const unsigned int maxPayloadSize = 0);
    ~ManagementModule();

    // Is a valid Management Module (MM) eg. exposes the MM interface
    bool IsValid() const;
    // Is the Management Module (MM) loaded eg. MM handles open
    bool IsLoaded() const;
    // Unloads the Management Module (MM) - Closes MMI and .so handles
    void UnloadModule();
    // Loads the Management Module (MM) - Opens the MMI and .so handles
    void LoadModule();
    // Sends a payload to the respective component
    int MmiSet(std::string componentName, std::string objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes);
    // Receive a payload to the respective component
    int MmiGet(std::string componentName, std::string objectName, MMI_JSON_STRING *payload, int *payloadSizeBytes);

    // Is the module defining the valid MMI?
    static bool IsExportingMmi(const std::string path);

    // MmiGetInfo Properties
    // See MmiGetInfo Schema for property descriptions -> src/modules/schema/MmiGetInfoSchema.json
    Lifetime GetLifetime() const;
    const std::vector<std::string> GetSupportedComponents() const;
    void AddReportedObject(const std::string& componentName, const std::string& objectName);
    const std::vector<std::string> GetReportedObjects(const std::string& componentName) const;
    const Version GetVersion() const;
    const std::string GetName() const;

    const std::string GetModulePath() const;

private:
    void* handle;
    MMI_HANDLE mmiHandle;
    // Is a valid MM eg. exposes the MM interface (MMI)
    bool isValid;
    // The client name of the MM
    const std::string clientName;
    // Path of the MM
    const std::string modulePath;
    // The maximum payload size
    int maxPayloadSizeBytes;

    // Module Metadata
    Info info;

    // Management Module Interface (MMI) imported functions
    MmiGetInfo_ptr mmiGetInfo;
    MmiFree_ptr mmiFree;
    MmiOpen_ptr mmiOpen;
    MmiSet_ptr mmiSet;
    MmiGet_ptr mmiGet;
    MmiClose_ptr mmiClose;
};

#endif // MANAGEMENTMODULE_H