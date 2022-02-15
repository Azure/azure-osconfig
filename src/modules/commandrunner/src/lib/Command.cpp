// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <ctime>
#include <fstream>
#include <unistd.h>

#include <Command.h>

static const char alphanum[] = "0123456789"\
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"\
    "abcdefghijklmnopqrstuvwxyz";

OSCONFIG_LOG_HANDLE CommandRunnerLog::m_log = nullptr;

template<typename T>
int DeserializeMember(const rapidjson::Value& document, const std::string key, T& value);

Command::Command(std::string id, std::string command, unsigned int timeout, bool replaceEol) :
    command(command),
    timeout(timeout),
    replaceEol(replaceEol),
    m_status(id, 0, "", Command::State::Unknown),
    m_statusMutex()
{
    std::string tmp;
    std::string uniqueId;

    char const* dir = getenv("TMPDIR");
    tmp = (NULL == dir) ? "/tmp" : dir;

    srand((unsigned)time(NULL) * getpid());
    uniqueId.reserve(10);

    for (int i = 0; i < 10; ++i)
    {
        uniqueId += alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    m_tmpFile = tmp + "/~osconfig-" + uniqueId;
}

Command::~Command()
{
    if (FileExists(m_tmpFile.c_str()) && (0 != remove(m_tmpFile.c_str())))
    {
        OsConfigLogError(CommandRunnerLog::Get(), "Failed to remove tmp file '%s'", m_tmpFile.c_str());
    }
}

int Command::Execute(std::function<int()> persistCacheToDisk, unsigned int maxPayloadSizeBytes)
{
    int exitCode = 0;
    Command::Status status = GetStatus();

    if (Command::State::Canceled == status.state)
    {
        exitCode = ECANCELED;
    }
    else
    {
        char* textResult = nullptr;
        void* context = reinterpret_cast<void*>(const_cast<char*>(m_tmpFile.c_str()));
        unsigned int maxTextResultSize = 0;

        if (maxPayloadSizeBytes > 0)
        {
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            Command::Status::Serialize(writer, status);

            maxTextResultSize = (maxPayloadSizeBytes > buffer.GetSize()) ? (maxPayloadSizeBytes - buffer.GetSize()) : 1;
        }

        exitCode = ExecuteCommand(context, command.c_str(), replaceEol, true, maxTextResultSize, timeout, &textResult, &Command::ExecutionCallback, CommandRunnerLog::Get());

        SetStatus(exitCode, (textResult != nullptr) ? std::string(textResult) : "");

        if (textResult != nullptr)
        {
            free(textResult);
        }

        if (persistCacheToDisk != nullptr)
        {
            if (0 != persistCacheToDisk())
            {
                OsConfigLogError(CommandRunnerLog::Get(), "Failed to persist cache to disk");
            }
        }
    }

    return exitCode;
}

int Command::Cancel()
{
    int status = 0;
    Status currentStatus = GetStatus();

    if ((Command::State::Canceled != currentStatus.state) && !FileExists(m_tmpFile.c_str()))
    {
        std::ofstream output(m_tmpFile);
        output.close();

        SetStatus(ECANCELED);
    }
    else
    {
        OsConfigLogError(CommandRunnerLog::Get(), "Command '%s' is already canceled", currentStatus.id.c_str());
        status = ECANCELED;
    }

    return status;
}

bool Command::IsComplete()
{
    std::lock_guard<std::mutex> lock(m_statusMutex);
    return (Command::State::Unknown != m_status.state) && (Command::State::Running != m_status.state);
}

bool Command::IsCanceled()
{
    bool canceled = false;
    std::lock_guard<std::mutex> lock(m_statusMutex);

    if (Command::State::Canceled == m_status.state)
    {
        if (FileExists(m_tmpFile.c_str()))
        {
            canceled = true;
        }
        else
        {
            OsConfigLogError(CommandRunnerLog::Get(), "Command '%s' is in a canceled state but no temporary file exists (%s)", m_status.id.c_str(), m_tmpFile.c_str());
        }
    }

    return canceled;
}

std::string Command::GetId()
{
    std::lock_guard<std::mutex> lock(m_statusMutex);
    return m_status.id;
}

Command::Status Command::GetStatus()
{
    std::lock_guard<std::mutex> lock(m_statusMutex);
    return m_status;
}

void Command::SetStatus(int exitCode, std::string textResult)
{
    Command::State state = Command::State::Unknown;
    switch (exitCode)
    {
        case EXIT_SUCCESS:
            state = Command::State::Succeeded;
            break;
        case ETIME:
            state = Command::State::TimedOut;
            break;
        case ECANCELED:
            state = Command::State::Canceled;
            break;
        default:
            state = Command::State::Failed;
    }

    SetStatus(exitCode, textResult, state);
}

void Command::SetStatus(int exitCode, std::string textResult, Command::State state)
{
    std::lock_guard<std::mutex> lock(m_statusMutex);
    m_status.exitCode = exitCode;
    m_status.textResult = textResult;
    m_status.state = state;
}

int Command::ExecutionCallback(void* context)
{
    int result = 0;

    if (nullptr != context)
    {
        char* name = reinterpret_cast<char*>(context);
        if (FileExists(name))
        {
            remove(name);
            result = 1;
        }
    }

    return result;
}

ShutdownCommand::ShutdownCommand(std::string id, std::string command, unsigned int timeout, bool replaceEol) :
    Command(id, command, timeout, replaceEol) { }

int ShutdownCommand::Execute(std::function<int()> persistCacheToDisk, unsigned int maxPayloadSizeBytes)
{
    int exitCode = 0;
    char* textResult;

    SetStatus(0, "", Command::State::Succeeded);

    if ((nullptr != persistCacheToDisk) && (0 != persistCacheToDisk()))
    {
        OsConfigLogError(CommandRunnerLog::Get(), "Failed to persist cache to disk, skipping command with id '%s' (%s)", GetId().c_str(), command.c_str());
    }
    else
    {
        exitCode = ExecuteCommand(nullptr, command.c_str(), replaceEol, true, maxPayloadSizeBytes, timeout, &textResult, nullptr, CommandRunnerLog::Get());

        if (nullptr != textResult)
        {
            free(textResult);
        }
    }

    return exitCode;
}

Command::Arguments::Arguments(std::string id, std::string command, Command::Action action, unsigned int timeout, bool singleLineTextResult) :
    id(id),
    command(command),
    action(action),
    timeout(timeout),
    singleLineTextResult(singleLineTextResult) { }

void Command::Arguments::Serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer, const Command::Arguments& arguments)
{
    writer.StartObject();

    writer.String(g_commandId.c_str());
    writer.String(arguments.id.c_str());

    writer.String(g_arguments.c_str());
    writer.String(arguments.command.c_str());

    writer.String(g_action.c_str());
    writer.Int(static_cast<int>(arguments.action));

    writer.String(g_timeout.c_str());
    writer.Uint(arguments.timeout);

    writer.String(g_singleLineTextResult.c_str());
    writer.Bool(arguments.singleLineTextResult);

    writer.EndObject();
}

Command::Arguments Command::Arguments::Deserialize(const rapidjson::Value& object)
{
    std::string id = "";
    std::string command = "";
    Command::Action action = Command::Action::None;
    unsigned int timeout = 0;
    bool singleLineTextResult = false;

    if (object.IsObject())
    {
        int actionValue = 0;
        if (0 != DeserializeMember(object, g_action.c_str(), actionValue))
        {
            OsConfigLogError(CommandRunnerLog::Get(), "Failed to deserialize %s.%s", g_commandArguments.c_str(), g_action.c_str());
        }
        else
        {
            action = static_cast<Command::Action>(actionValue);

            switch (action)
            {
                case Command::Action::Reboot:
                case Command::Action::Shutdown:
                case Command::Action::RefreshCommandStatus:
                case Command::Action::CancelCommand:
                    if (0 == DeserializeMember(object, g_commandId, id))
                    {
                        if (id.empty())
                        {
                            OsConfigLogError(CommandRunnerLog::Get(), "%s.%s is empty", g_commandArguments.c_str(), g_commandId.c_str());
                        }
                    }
                    else
                    {
                        OsConfigLogError(CommandRunnerLog::Get(), "Failed to deserialize %s.%s", g_commandArguments.c_str(), g_commandId.c_str());
                    }
                    break;

                case Command::Action::RunCommand:
                    if (0 == DeserializeMember(object, g_commandId, id) && !id.empty())
                    {
                        if (!id.empty())
                        {
                            if (0 == DeserializeMember(object, g_arguments, command))
                            {
                                if (!command.empty())
                                {
                                    // Timeout is an optional field
                                    if (0 != DeserializeMember(object, g_timeout, timeout))
                                    {
                                        // Assume default of no timeout (0)
                                        timeout = 0;
                                        OsConfigLogInfo(CommandRunnerLog::Get(), "%s.%s default value '0' (no timeout) used for command id: %s", g_commandArguments.c_str(), g_timeout.c_str(), id.c_str());
                                    }

                                    // SingleLineTextResult is an optional field
                                    if (0 != DeserializeMember(object, g_singleLineTextResult, singleLineTextResult))
                                    {
                                        // Assume default of true
                                        singleLineTextResult = true;
                                        OsConfigLogInfo(CommandRunnerLog::Get(), "%s.%s default value 'true' used for command id: %s", g_commandArguments.c_str(), g_singleLineTextResult.c_str(), id.c_str());
                                    }
                                }
                                else
                                {
                                    OsConfigLogError(CommandRunnerLog::Get(), "%s.%s is empty for command id: %s", g_commandArguments.c_str(), g_arguments.c_str(), id.c_str());
                                }
                            }
                            else
                            {
                                OsConfigLogError(CommandRunnerLog::Get(), "Failed to deserialize %s.%s for command id: %s", g_commandArguments.c_str(), g_arguments.c_str(), id.c_str());
                            }
                        }
                        else
                        {
                            OsConfigLogError(CommandRunnerLog::Get(), "%s.%s is empty", g_commandArguments.c_str(), g_commandId.c_str());
                        }
                    }
                    else
                    {
                        OsConfigLogError(CommandRunnerLog::Get(), "Failed to deserialize %s.%s '%s'", g_commandArguments.c_str(), g_commandId.c_str(), id.c_str());
                    }
                    break;

                case Command::Action::None:
                default:
                    break;
            }
        }
    }
    else
    {
        OsConfigLogError(CommandRunnerLog::Get(), "Invalid command arguments JSON object");
    }

    return Command::Arguments(id, command, action, timeout, singleLineTextResult);
}

Command::Status::Status(const std::string id, int exitCode, std::string textResult, Command::State state) :
    id(id),
    exitCode(exitCode),
    textResult(textResult),
    state(state) { }

void Command::Status::Serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer, const Command::Status& status, bool serializeTextResult)
{
    writer.StartObject();

    writer.Key(g_commandId.c_str());
    writer.String(status.id.c_str());

    writer.Key(g_resultCode.c_str());
    writer.Int(status.exitCode);

    if (serializeTextResult)
    {
        writer.Key(g_textResult.c_str());
        writer.String(status.textResult.c_str());
    }

    writer.Key(g_currentState.c_str());
    writer.Int(status.state);

    writer.EndObject();
}

Command::Status Command::Status::Deserialize(const rapidjson::Value& object)
{
    std::string id = "";
    int exitCode = 0;
    std::string textResult;
    Command::State state = Command::State::Unknown;

    if (object.IsObject())
    {
        // Command id is a required field
        if (0 == DeserializeMember(object, g_commandId, id))
        {
            // Use defaults for other fields
            DeserializeMember(object, g_resultCode, exitCode);
            DeserializeMember(object, g_textResult, textResult);

            int stateValue = 0;
            if (0 == DeserializeMember(object, g_currentState, stateValue))
            {
                state = static_cast<Command::State>(stateValue);
            }
        }
    }
    else
    {
        OsConfigLogError(CommandRunnerLog::Get(), "Invalid command status JSON object");
    }

    return Command::Status(id, exitCode, textResult, state);
}

int Deserialize(const rapidjson::Value& object, const char* key, std::string& value)
{
    int status = 0;

    if (object[key].IsString())
    {
        value = object[key].GetString();
    }
    else
    {
        OsConfigLogError(CommandRunnerLog::Get(), "%s is not a string", key);
        status = EINVAL;
    }

    return status;
}

int Deserialize(const rapidjson::Value& object, const char* key, int& value)
{
    int status = 0;

    if (object[key].IsInt())
    {
        value = object[key].GetInt();
    }
    else
    {
        OsConfigLogError(CommandRunnerLog::Get(), "%s is not an int", key);
        status = EINVAL;
    }

    return status;
}

int Deserialize(const rapidjson::Value& object, const char* key, bool& value)
{
    int status = 0;

    if (object[key].IsBool())
    {
        value = object[key].GetBool();
    }
    else
    {
        OsConfigLogError(CommandRunnerLog::Get(), "%s is not a bool", key);
        status = EINVAL;
    }

    return status;
}

int Deserialize(const rapidjson::Value& object, const char* key, unsigned int& value)
{
    int status = 0;

    if (object[key].IsUint())
    {
        value = object[key].GetUint();
    }
    else
    {
        OsConfigLogError(CommandRunnerLog::Get(), "%s is not an unsigned int", key);
        status = EINVAL;
    }

    return status;
}

template<typename T>
int DeserializeMember(const rapidjson::Value& object, const std::string key, T& value)
{
    int status = 0;

    if (object.IsObject() && object.HasMember(key.c_str()))
    {
        status = Deserialize(object, key.c_str(), value);
    }
    else
    {
        status = EINVAL;
    }

    return status;
}
