// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef MANAGEMENTMODULE_H
#define MANAGEMENTMODULE_H

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
        unsigned int major = 0;
        unsigned int minor = 0;
        unsigned int patch = 0;
        unsigned int tweak = 0;

        // Only defining "<" as we are only using that operator in comparisons
        bool operator < (const Version &rhs) const
        {
            return (((major < rhs.major) ||
                     ((major == rhs.major) && (minor < rhs.minor)) ||
                     ((major == rhs.major) && (minor == rhs.minor) && (patch < rhs.patch)) ||
                     ((major == rhs.major) && (minor == rhs.minor) && (patch == rhs.patch) && (tweak < rhs.tweak)))
                        ? true : false);
        }

        std::string ToString() const
        {
            std::ostringstream ostream;
            ostream << major << "." << minor << "." << patch << "." << tweak;
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

        static int Deserialize(const rapidjson::Value& object, Info& info);
    };

    ManagementModule();
    ManagementModule(const std::string path);
    virtual ~ManagementModule();

    virtual int Load();
    virtual void Unload();

    Info GetInfo() const;

protected:
    const std::string m_modulePath;

    // The handle retuned by dlopen()
    void* m_handle;

    // Management Module Interface (MMI) imported functions
    Mmi_GetInfo m_mmiGetInfo;
    Mmi_Open m_mmiOpen;
    Mmi_Close m_mmiClose;
    Mmi_Set m_mmiSet;
    Mmi_Get m_mmiGet;
    Mmi_Free m_mmiFree;

    Info m_info;

    virtual int CallMmiGetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
    virtual MMI_HANDLE CallMmiOpen(const char* componentName, unsigned int maxPayloadSizeBytes);
    virtual void CallMmiClose(MMI_HANDLE handle);
    virtual int CallMmiSet(MMI_HANDLE handle, const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes);
    virtual int CallMmiGet(MMI_HANDLE handle, const char* componentName, const char* objectName, MMI_JSON_STRING *payload, int *payloadSizeBytes);

    friend class MmiSession;
};

class MmiSession
{
public:
    MmiSession(std::shared_ptr<ManagementModule> module, const std::string& clientName, unsigned int maxPayloadSizeBytes = 0);
    ~MmiSession();

    int Open();
    void Close();

    int Set(const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes);
    int Get(const char* componentName, const char* objectName, MMI_JSON_STRING *payload, int *payloadSizeBytes);

    ManagementModule::Info GetInfo();
private:
    const std::string m_clientName;
    const unsigned int m_maxPayloadSizeBytes;
    std::shared_ptr<ManagementModule> m_module;

    MMI_HANDLE m_mmiHandle;
};

#endif // MANAGEMENTMODULE_H