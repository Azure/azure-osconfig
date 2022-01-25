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
using Mmi_GetInfo = int (*)(const char*, MMI_JSON_STRING*, int*);
using Mmi_Free = void (*)(MMI_JSON_STRING);
using Mmi_Open = MMI_HANDLE (*)(const char*, const unsigned int);
using Mmi_Set = int (*)(MMI_HANDLE, const char*, const char*, const MMI_JSON_STRING, const int);
using Mmi_Get = int (*)(MMI_HANDLE, const char*, const char*, MMI_JSON_STRING*, int*);
using Mmi_Close = void (*)(MMI_HANDLE);

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

    // Structure maps to the MmiGetInfo JSON Schema - see src/modules/schema/mmi-get-info.schema.json for more info
    struct Info
    {
        std::string name;
        std::string description;
        std::string manufacturer;
        Version version;
        std::string versionInfo;
        std::vector<std::string> components;
        Lifetime lifetime;
        std::string licenseUri;
        std::string projectUri;
        unsigned int userAccount;
    };

    std::map<std::string, std::vector<std::string>> reportedObjects;

    ManagementModule(const std::string clientName, const std::string path, const unsigned int maxPayloadSize = 0);
    virtual ~ManagementModule();

    // Is a valid Management Module (MM) eg. exposes the MM interface
    bool IsValid() const;

    // Is the Management Module (MM) loaded eg. MM handles open
    bool IsLoaded() const;

    virtual void LoadModule();
    virtual void UnloadModule();

    virtual int CallMmiSet(const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes);
    virtual int CallMmiGet(const char* componentName, const char* objectName, MMI_JSON_STRING *payload, int *payloadSizeBytes);

    virtual bool IsExportingMmi(const std::string path);

    // MmiGetInfo Properties
    // See MmiGetInfo Schema for property descriptions -> src/modules/schema/MmiGetInfoSchema.json
    const std::string GetName() const;
    const Version GetVersion() const;
    Lifetime GetLifetime() const;
    const std::vector<std::string> GetSupportedComponents() const;

    void AddReportedObject(const std::string& componentName, const std::string& objectName);
    const std::vector<std::string> GetReportedObjects(const std::string& componentName) const;

    const std::string GetModulePath() const;

protected:
    void* handle;
    MMI_HANDLE mmiHandle;

    // Is a valid MM eg. exposes the MM interface (MMI)
    bool isValid;

    const std::string clientName;
    const std::string modulePath;

    // The maximum payload size
    int maxPayloadSizeBytes;

    // Module Metadata
    Info info;

    // Management Module Interface (MMI) imported functions
    Mmi_GetInfo mmiGetInfo;
    Mmi_Open mmiOpen;
    Mmi_Close mmiClose;
    Mmi_Set mmiSet;
    Mmi_Get mmiGet;
    Mmi_Free mmiFree;
};

#endif // MANAGEMENTMODULE_H