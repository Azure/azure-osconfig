// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <errno.h>
#include <fstream>
#include <memory>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/document.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/schema.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <string>

#include <CommandRunner.h>
#include <CommonUtils.h>
#include <Mmi.h>
#include <ScopeGuard.h>

#define CACHEFILE "/etc/osconfig/osconfig_commandrunner.cache"

const std::string ComponentName = "CommandRunner";
const std::string DesiredObjectName = "CommandArguments";
const std::string ReportedObjectName = "CommandStatus";
const std::string CommandId = "CommandId";
const std::string Arguments = "Arguments";
const std::string Action = "Action";
const std::string Timeout = "Timeout";
const std::string SingleLineTextResult = "SingleLineTextResult";
const std::string ResultCode = "ResultCode";
const std::string TextResult = "TextResult";
const std::string CurrentState = "CurrentState";

const std::string ClientName = "ClientName";
const std::string CommandStatusValues = "CommandStatusValues";

constexpr const char CommandIdNotFoundErrorFormat[] = R""""(CommandId '%s' not found)"""";

unsigned int maxPayloadSizeBytes = 0;

// Manager mapping clientName <-> CommandRunner
std::map<std::string, std::weak_ptr<CommandRunner>> commandRunnerMap;
// Manager mapping MPI_HANDLE <-> CommandRunner
std::map<MMI_HANDLE, std::shared_ptr<CommandRunner>> mmiMap;

// Serializes the CommandStatus into JSON
void Serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer, const CommandRunner::CommandStatus commandStatus, bool serializeTextResult)
{
    writer.StartObject();

    writer.Key(CommandId.c_str());
    writer.String(commandStatus.commandId.c_str());

    writer.Key(ResultCode.c_str());
    writer.Int(commandStatus.resultCode);

    if (serializeTextResult)
    {
        writer.Key(TextResult.c_str());
        writer.String(commandStatus.textResult.c_str());
    }

    writer.Key(CurrentState.c_str());
    writer.Int(commandStatus.commandState);

    writer.EndObject();
}


// Serializes the CommandStatus into JSON
// Ignores CommandStatus.TextResult purposely
CommandRunner::CommandStatus DeSerialize(rapidjson::Value* value)
{
    CommandRunner::CommandStatus commandStatus;

    if (!value->IsObject())
    {
        OsConfigLogError(CommandRunnerLog::Get(), "DeSerialize: Expecting CommandStatus JSON object");
    }

    for (auto it=value->MemberBegin(); it != value->MemberEnd(); ++it)
    {
        if (strcmp(it->name.GetString(), CommandId.c_str()) == 0)
        {
            commandStatus.commandId = it->value.GetString();
            continue;
        }
        if (strcmp(it->name.GetString(), ResultCode.c_str()) == 0)
        {
            commandStatus.resultCode = it->value.GetInt();
            continue;
        }
        if (strcmp(it->name.GetString(), CurrentState.c_str()) == 0)
        {
            commandStatus.commandState = static_cast<CommandRunner::CommandState>(it->value.GetInt());
        }
    }

    return commandStatus;
}

int WriteToCache(rapidjson::StringBuffer& sb)
{
    if (sb.GetSize() > 0)
    {
        std::FILE* file = std::fopen(CACHEFILE, "w");
        if (nullptr == file)
        {
            OsConfigLogError(CommandRunnerLog::Get(), "Unable to open %s for cache", CACHEFILE);
            return EACCES;
        }
        int rc = std::fputs(sb.GetString(), file);
        if ((0 > rc) || (EOF == rc))
        {
            int status = errno ? errno : EINVAL;
            OsConfigLogError(CommandRunnerLog::Get(), "Unable to save last command results to %s, error: %d %s", CACHEFILE, status, errno ? strerror(errno) : "-");
            return status;
        }
        fflush(file);
        std::fclose(file);
    }

    return 0;
}

int PersistCommandResults()
{
    // Serialize command results from all clients
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    writer.StartArray();
    for (auto i = commandRunnerMap.begin(); i != commandRunnerMap.end(); ++i)
    {
        if (!i->second.expired())
        {
            writer.StartObject();
            writer.Key(ClientName.c_str());
            writer.String(i->first.c_str());

            writer.Key(CommandStatusValues.c_str());
            writer.StartArray();

            Serialize(writer, i->second.lock()->GetCommandStatusToPersist(), false);

            writer.EndArray();
            writer.EndObject();
        }
    }
    writer.EndArray();

    return WriteToCache(sb);
}

void LoadLastCommandResults(const std::string& clientName, std::shared_ptr<CommandRunner> commandRunner)
{
    std::ifstream file(CACHEFILE);
    if (!file.good())
    {
        OsConfigLogInfo(CommandRunnerLog::Get(), "File %s not found, cannot find previous command results", CACHEFILE);
        return;
    }

    rapidjson::IStreamWrapper isw(file);
    rapidjson::Document jsonDoc;
    if (jsonDoc.ParseStream(isw).HasParseError())
    {
        OsConfigLogInfo(CommandRunnerLog::Get(), "Invalid JSON found in %s", CACHEFILE);
        return;
    }

    if (!jsonDoc.IsArray())
    {
        OsConfigLogInfo(CommandRunnerLog::Get(), "Cache file JSON is not an array");
        return;
    }

    for (auto itClients=jsonDoc.Begin(); itClients != jsonDoc.End(); ++itClients)
    {
        assert(itClients->IsObject());
        assert(itClients->GetObject().HasMember(ClientName.c_str()));

        std::string docName = itClients->GetObject()[ClientName.c_str()].GetString();

        if (docName == clientName)
        {
            auto commandStatuses = itClients->GetObject()[CommandStatusValues.c_str()].GetArray();
            for (auto itCommandStatus=commandStatuses.Begin(); itCommandStatus != commandStatuses.End(); ++itCommandStatus)
            {
                auto commandStatus = DeSerialize(itCommandStatus);
                commandRunner->AddCommandStatus(commandStatus.commandId, true);
                commandRunner->UpdateCommandStatus(commandStatus.commandId, commandStatus.resultCode, commandStatus.textResult, commandStatus.commandState);
            }
        }
    }
}

void __attribute__((constructor)) InitModule()
{
    CommandRunnerLog::OpenLog();
    OsConfigLogInfo(CommandRunnerLog::Get(), "CommandRunner module loaded");
}
void __attribute__((destructor)) DestroyModule()
{
    OsConfigLogInfo(CommandRunnerLog::Get(), "CommandRunner module unloaded");
    for (auto it = commandRunnerMap.begin(); it != commandRunnerMap.end(); ++it)
    {
        if (!it->second.expired())
        {
            it->second.lock()->CancelAll();
        }
    }
    CommandRunnerLog::CloseLog();
}

int MmiGetInfoInternal(
    const char* clientName,
    MMI_JSON_STRING* payload,
    int* payloadSizeBytes)
{
    int status = MMI_OK;

    ScopeGuard sg{[&]()
    {
        if (MMI_OK == status)
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogInfo(CommandRunnerLog::Get(), "MmiGetInfo(%s, %.*s, %d) returned %d", clientName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
            }
            else
            {
                OsConfigLogInfo(CommandRunnerLog::Get(), "MmiGetInfo(%s, -, %d) returned %d", clientName, *payloadSizeBytes, status);
            }
        }
        else
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(CommandRunnerLog::Get(), "MmiGetInfo(%s, %.*s, %d) returned %d", clientName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
            }
            else
            {
                OsConfigLogError(CommandRunnerLog::Get(), "MmiGetInfo(%s, -, %d) returned %d", clientName, *payloadSizeBytes, status);
            }
        }
    }};

    if ((nullptr == payload) || (nullptr == payloadSizeBytes))
    {
        OsConfigLogError(CommandRunnerLog::Get(), "MmiGetInfo called with invalid arguments");
        status = EINVAL;
        return status;
    }

    constexpr const char ret[] = R""""({
        "Name": "CommandRunner",
        "Description": "Provides functionality to remotely run commands on the device",
        "Manufacturer": "Microsoft",
        "VersionMajor": 2,
        "VersionMinor": 0,
        "VersionInfo": "Nickel",
        "Components": ["CommandRunner"],
        "Lifetime": 1,
        "UserAccount": 0})"""";

    std::size_t len = ARRAY_SIZE(ret) - 1;

    *payloadSizeBytes = len;
    *payload = new char[len];
    std::memcpy(*payload, ret, len);

    return status;
}

int MmiGetInfo(
    const char* clientName,
    MMI_JSON_STRING* payload,
    int* payloadSizeBytes)
{
    try
    {
        return MmiGetInfoInternal(clientName, payload, payloadSizeBytes);
    }
    catch (const std::exception &e)
    {
        OsConfigLogError(CommandRunnerLog::Get(), "MmiGetInfo exception occurred");
        return EFAULT;
    }
}

MMI_HANDLE MmiOpenInternal(
    const char* clientName,
    const unsigned int maxPayloadSize)
{
    int status = MMI_OK;
    MMI_HANDLE handle = nullptr;

    ScopeGuard sg{[&]()
    {
        if (MMI_OK == status)
        {
            OsConfigLogInfo(CommandRunnerLog::Get(), "MmiOpen(%s, %d) returned: %p, status: %d", clientName, maxPayloadSizeBytes, handle, status);
        }
        else
        {
            OsConfigLogError(CommandRunnerLog::Get(), "MmiOpen(%s, %d) returned: %p, status: %d", clientName, maxPayloadSizeBytes, handle, status);
        }
    }};

    OsConfigLogInfo(CommandRunnerLog::Get(), "MmiOpen(%s, %d)", clientName, maxPayloadSize);

    if (nullptr == clientName)
    {
        OsConfigLogError(CommandRunnerLog::Get(), "MmiOpen called without a clientName.");
        status = EINVAL;
        return handle;
    }

    bool commandRunnerExistsForClient = (commandRunnerMap.end() != commandRunnerMap.find(clientName)) ? true : false;
    if (!commandRunnerExistsForClient || (commandRunnerExistsForClient && commandRunnerMap[clientName].expired()))
    {
        // No CommandRunner found for clientName or expired CommandRunner, create new one
        OsConfigLogInfo(CommandRunnerLog::Get(), "Initializing %s for clientName: %s", ComponentName.c_str(), clientName);
        maxPayloadSizeBytes = maxPayloadSize;
        auto commandRunner = std::shared_ptr<CommandRunner>(new CommandRunner(clientName, PersistCommandResults, maxPayloadSizeBytes));
        MMI_HANDLE mmiHandle = reinterpret_cast<MMI_HANDLE>(commandRunner.get());
        commandRunnerMap[clientName] = commandRunner;
        mmiMap[mmiHandle] = commandRunner;
        LoadLastCommandResults(clientName, commandRunner);
        handle = mmiHandle;
    }
    else if (!(commandRunnerMap[clientName].expired()))
    {
        handle = reinterpret_cast<MMI_HANDLE>(commandRunnerMap[clientName].lock().get());
        OsConfigLogInfo(CommandRunnerLog::Get(), "MmiOpen already called for %s, returning original handle: %p.", clientName, handle);
    }

    return handle;
}

MMI_HANDLE MmiOpen(
    const char* clientName,
    const unsigned int maxPayloadSize)
{
    try
    {
        return MmiOpenInternal(clientName, maxPayloadSize);
    }
    catch (const std::exception &e)
    {
        OsConfigLogError(CommandRunnerLog::Get(), "MmiOpen exception occurred");
        return nullptr;
    }
}

void MmiClose(MMI_HANDLE clientSession)
{
    OsConfigLogInfo(CommandRunnerLog::Get(), "MmiClose(%p)", clientSession);
    if (mmiMap.end() != mmiMap.find(clientSession))
    {
        std::string clientName = mmiMap[clientSession].get()->GetClientName();
        mmiMap[clientSession]->CancelAll();

        if (0 != PersistCommandResults())
        {
            OsConfigLogError(CommandRunnerLog::Get(), "MmiClose: error writing to cache");
        }

        mmiMap[clientSession].reset();
    }
    else
    {
        OsConfigLogError(CommandRunnerLog::Get(), "MmiClose invalid MMI_HANDLE. handle: %p", clientSession);
    }
}

std::string ParseStringFromPayload(rapidjson::Document& jsonDoc, std::string property)
{
    std::string value = "";
    if (jsonDoc.HasMember(property.c_str()))
    {
        if (jsonDoc[property.c_str()].IsString())
        {
            value = jsonDoc[property.c_str()].GetString();
        }
        else
        {
            OsConfigLogError(CommandRunnerLog::Get(), "CommandArguments.%s result must be of type 'string'", property.c_str());
        }
    }
    return value;
}

int ParseDesiredPayload(rapidjson::Document& jsonDoc, CommandRunner::CommandArguments* commandArgs)
{
    int status = 0;

    if (jsonDoc.HasMember(Action.c_str()))
    {
        if (jsonDoc[Action.c_str()].IsInt())
        {
            commandArgs->action = static_cast<CommandRunner::Action>(jsonDoc[Action.c_str()].GetInt());
        }
        else
        {
            OsConfigLogError(CommandRunnerLog::Get(), "CommandArguments.Action must be of type 'integer'");
            status = EINVAL;
        }
    }
    else
    {
        OsConfigLogError(CommandRunnerLog::Get(), "CommandArguments.Action not received");
        status = EINVAL;
    }

    if (0 == status)
    {
        // Set default values
        commandArgs->commandId = "";
        commandArgs->arguments = "";
        commandArgs->timeout = 0;
        commandArgs->singleLineTextResult = false;

        switch (commandArgs->action)
        {
            case CommandRunner::Action::Reboot:
            case CommandRunner::Action::Shutdown:
            case CommandRunner::Action::RefreshCommandStatus:
            case CommandRunner::Action::CancelCommand:
                commandArgs->commandId = ParseStringFromPayload(jsonDoc, CommandId);
                if (commandArgs->commandId.empty())
                {
                    OsConfigLogError(CommandRunnerLog::Get(), "CommandArguments.CommandId is empty for action: %d", commandArgs->action);
                    status = EINVAL;
                }
                break;

            case CommandRunner::Action::RunCommand:
                commandArgs->commandId = ParseStringFromPayload(jsonDoc, CommandId);
                if (!commandArgs->commandId.empty())
                {
                    commandArgs->arguments = ParseStringFromPayload(jsonDoc, Arguments);
                    if (!commandArgs->arguments.empty())
                    {
                        // Timeout is an optional field
                        if (jsonDoc.HasMember(Timeout.c_str()) && jsonDoc[Timeout.c_str()].IsUint())
                        {
                            commandArgs->timeout = jsonDoc[Timeout.c_str()].GetUint();
                        }
                        else
                        {
                            // Assume default of no timeout
                            commandArgs->timeout = 0;
                            OsConfigLogInfo(CommandRunnerLog::Get(), "CommandArguments.Timeout assumed 0 (no timeout) for commandId: %s", commandArgs->commandId.c_str());
                        }
                        
                        // SingleLineTextResult is an optional field
                        if (jsonDoc.HasMember(SingleLineTextResult.c_str()) && jsonDoc[SingleLineTextResult.c_str()].IsBool())
                        {
                            commandArgs->singleLineTextResult = jsonDoc[SingleLineTextResult.c_str()].GetBool();
                        }
                        else
                        {
                            // Assume default of true (text result as a single line)
                            commandArgs->singleLineTextResult = true;
                            OsConfigLogInfo(CommandRunnerLog::Get(), "CommandArguments.SingleLineTextResult assumed true for commandId: %s", commandArgs->commandId.c_str());
                        }
                    }
                    else
                    {
                        OsConfigLogError(CommandRunnerLog::Get(), "CommandArguments.Arguments is empty for commandId: %s", commandArgs->commandId.c_str());
                        status = EINVAL;
                    }
                }
                else
                {
                    OsConfigLogError(CommandRunnerLog::Get(), "CommandArguments.CommandId is empty for action: %d", commandArgs->action);
                    status = EINVAL;
                }
                break;

            case CommandRunner::Action::None:
            default:
                status = 0;
        }
    }

    return status;
}

int MmiSetInternal(
    MMI_HANDLE clientSession,
    const char* componentName,
    const char* objectName,
    const MMI_JSON_STRING payload,
    const int payloadSizeBytes)
{
    int status = MMI_OK;
    CommandRunner* session = nullptr;

    ScopeGuard sg{[&]()
    {
        if (MMI_OK == status)
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogInfo(CommandRunnerLog::Get(), "MmiSet(%p, %s, %s, %.*s, %d) returned %d", clientSession, componentName, objectName, payloadSizeBytes, payload, payloadSizeBytes, status);
            }
        }
        else
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(CommandRunnerLog::Get(), "MmiSet(%p, %s, %s, %.*s, %d) returned %d", clientSession, componentName, objectName, payloadSizeBytes, payload, payloadSizeBytes, status);
            }
            else
            {
                OsConfigLogError(CommandRunnerLog::Get(), "MmiSet(%p, %s, %s, -, %d) returned %d", clientSession, componentName, objectName, payloadSizeBytes, status);
            }
        }
    }};

    if (nullptr == clientSession)
    {
        OsConfigLogError(CommandRunnerLog::Get(), "MmiSet invalid clientSession!");
        status = EPERM;
        return status;
    }

    if (payloadSizeBytes > static_cast<int> (maxPayloadSizeBytes))
    {
        OsConfigLogError(CommandRunnerLog::Get(), "MmiSet payload size is too large: %u bytes (max: %u bytes)", (unsigned int)payloadSizeBytes, maxPayloadSizeBytes);
        status = E2BIG;
        return status;
    }

    rapidjson::Document jsonDoc;
    if (jsonDoc.Parse(payload).HasParseError())
    {
        OsConfigLogError(CommandRunnerLog::Get(), "MmiSet unable to parse JSON payload!");
        status = EINVAL;
        return status;
    }

    if (0 == ComponentName.compare(componentName))
    {
        if (0 == DesiredObjectName.compare(objectName))
        {
            // Populate CommandArguments struct from parsed JSON
            CommandRunner::CommandArguments commandargs;
            if (0 != (status = ParseDesiredPayload(jsonDoc, &commandargs)))
            {
                OsConfigLogError(CommandRunnerLog::Get(), "MmiSet unable to validate JSON payload");
            }
            else
            {
                session = reinterpret_cast<CommandRunner*>(clientSession);

                // Do not re-run the same command again - we receive the previous payload
                if ((commandargs.action != CommandRunner::Action::None) &&
                    (commandargs.action != CommandRunner::Action::RefreshCommandStatus) &&
                    (commandargs.action != CommandRunner::Action::CancelCommand))
                {
                    if (session->CommandExists(commandargs.commandId))
                    {
                        OsConfigLogError(CommandRunnerLog::Get(), "Cannot execute a new request with the same commandId: %s", commandargs.commandId.c_str());
                        status = EINVAL;
                    }
                }

                if (MMI_OK == status)
                {
                    if (commandargs.action == CommandRunner::Action::CancelCommand)
                    {
                        status = session->Cancel(commandargs.commandId);
                    }
                    else
                    {
                        status = session->Run(commandargs);
                    }
                }
            }
        }
        else
        {
            // Invalid objectName
            OsConfigLogError(CommandRunnerLog::Get(), "MmiGet invalid objectName %s", objectName);
            status = ENOENT;
        }
    }
    else
    {
        // Invalid componentName
        OsConfigLogError(CommandRunnerLog::Get(), "MmiSet invalid componentName: %s, return: %d", componentName, EINVAL);
        status = EINVAL;
    }

    return status;
}

int MmiSet(
    MMI_HANDLE clientSession,
    const char* componentName,
    const char* objectName,
    const MMI_JSON_STRING payload,
    const int payloadSizeBytes)
{
    try
    {
        return MmiSetInternal(clientSession, componentName, objectName, payload, payloadSizeBytes);
    }
    catch (const std::exception &e)
    {
        OsConfigLogError(CommandRunnerLog::Get(), "MmiSet exception occurred");
        return EFAULT;
    }
}

int MmiGet(
    MMI_HANDLE clientSession,
    const char* componentName,
    const char* objectName,
    MMI_JSON_STRING* payload,
    int *payloadSizeBytes)
{
    int retVal = MMI_OK;
    CommandRunner* session = nullptr;
    CommandRunner::CommandStatus* status = nullptr;
    bool heapAllocatedStatus = false;

    ScopeGuard sg{[&]()
    {
        if (heapAllocatedStatus)
        {
            delete status;
        }

        if ((MMI_OK != retVal) || (nullptr == *payload) || (nullptr == payloadSizeBytes) || (0 == *payloadSizeBytes))
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(CommandRunnerLog::Get(), "MmiGet(%p, %s, %s, %.*s, %d) returned %d", clientSession, componentName, objectName, *payloadSizeBytes, *payload, *payloadSizeBytes, retVal);
            }
            else
            {
                OsConfigLogError(CommandRunnerLog::Get(), "MmiGet(%p, %s, %s, -, %d) returned %d", clientSession, componentName, objectName, *payloadSizeBytes, retVal);
            }
        }
    }};

    if (nullptr == clientSession)
    {
        OsConfigLogError(CommandRunnerLog::Get(), "MmiGet called without MmiOpen");
        retVal = EPERM;
        return retVal;
    }

    if ((0 == ComponentName.compare(componentName)))
    {
        if ((0 == ReportedObjectName.compare(objectName)))
        {
            try
            {
                session = reinterpret_cast<CommandRunner*>(clientSession);
                status = session->GetCommandStatus(session->GetCommandIdToRefresh());

                if (nullptr == status)
                {
                    // CommandStatus is not available, but in order to return successfully we must return a CommandStatus on success
                    status = new CommandRunner::CommandStatus();
                    status->commandId = "";
                    status->resultCode = 0;
                    status->textResult = "";
                    status->commandState = CommandRunner::CommandState::Unknown;
                    heapAllocatedStatus = true;
                }
                else if (status->commandId.empty())
                {
                    // CommandStatus not available, do not fail request but return CommandStatus with error number
                    // resultCode: EINVAL - Invalid argument
                    status->commandId = session->GetCommandIdToRefresh();
                    status->resultCode = EINVAL;
                    status->commandState = CommandRunner::CommandState::Unknown;

                    int size = std::snprintf(nullptr, 0, CommandIdNotFoundErrorFormat, status->commandId.c_str()) + 1;
                    std::unique_ptr<char[]> errorMsg(new char[size]);
                    std::snprintf(errorMsg.get(), size, CommandIdNotFoundErrorFormat, status->commandId.c_str());
                    status->textResult = errorMsg.get();
                }


                if (nullptr != payloadSizeBytes)
                {
                    *payload = nullptr;
                    *payloadSizeBytes = 0;

                    rapidjson::StringBuffer sb;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
                    Serialize(writer, *status, true);

                    *payload = new (std::nothrow) char[sb.GetSize()];
                    if (nullptr == *payload)
                    {
                        OsConfigLogError(CommandRunnerLog::Get(), "MmiGet failed to allocate memory");
                        retVal = ENOMEM;
                    }
                    else
                    {
                        std::fill(*payload, *payload + sb.GetSize(), 0);
                        std::memcpy(*payload, sb.GetString(), sb.GetSize());
                        *payloadSizeBytes = sb.GetSize();
                    }
                }
                else
                {
                    OsConfigLogError(CommandRunnerLog::Get(), "MmiGet called with nullptr payloadSizeBytes");
                    retVal = EINVAL;
                }
            }
            catch (const std::exception& e)
            {
                OsConfigLogError(CommandRunnerLog::Get(), "MmiGet exception thrown: %s", e.what());
                retVal = EINTR;

                if (nullptr != *payload)
                {
                    delete[] *payload;
                    *payload = nullptr;
                }

                if (nullptr != payloadSizeBytes)
                {
                    *payloadSizeBytes = 0;
                }
            }
        }
        else
        {
            // Invalid objectName
            OsConfigLogError(CommandRunnerLog::Get(), "MmiGet invalid objectName %s", objectName);
            retVal = ENOENT;
        }
    }
    else
    {
        // Invalid componentName
        OsConfigLogError(CommandRunnerLog::Get(), "MmiGet invalid componentName %s", componentName);
        retVal = ENOENT;
    }

    return retVal;
}

void MmiFree(MMI_JSON_STRING payload)
{
    if (nullptr != payload)
    {
        delete[] payload;
    }
}