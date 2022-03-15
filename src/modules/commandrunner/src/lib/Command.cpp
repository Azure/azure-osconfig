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
    m_arguments(command),
    m_timeout(timeout),
    m_replaceEol(replaceEol),
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
        OsConfigLogError(CommandRunnerLog::Get(), "Failed to remove file: %s", m_tmpFile.c_str());
    }
}

int Command::Execute(unsigned int maxPayloadSizeBytes)
{
    int exitCode = 0;
    Command::Status status = GetStatus();

    if (IsCanceled())
    {
        SetStatus(ECANCELED, "");
        exitCode = ECANCELED;
    }
    else
    {
        char* textResult = nullptr;
        void* context = reinterpret_cast<void*>(const_cast<char*>(m_tmpFile.c_str()));
        unsigned int maxTextResultSize = 0;

        if (maxPayloadSizeBytes > 0)
        {
            unsigned int estimatedSize = Command::Status::Serialize(Command::Status(status.m_id, 0, "", Command::State::Unknown)).size();
            maxTextResultSize = (maxPayloadSizeBytes > estimatedSize) ? (maxPayloadSizeBytes - estimatedSize) : 1;
        }

        SetStatus(0, "", Command::State::Running);

        exitCode = ExecuteCommand(context, m_arguments.c_str(), m_replaceEol, true, maxTextResultSize, m_timeout, &textResult, &Command::ExecutionCallback, CommandRunnerLog::Get());

        SetStatus(exitCode, (textResult != nullptr) ? std::string(textResult) : "");

        if (textResult != nullptr)
        {
            free(textResult);
        }
    }

    return exitCode;
}

int Command::Cancel()
{
    int status = 0;
    std::lock_guard<std::mutex> lock(m_statusMutex);

    if ((Command::State::Canceled != m_status.m_state) && !FileExists(m_tmpFile.c_str()))
    {
        std::ofstream output(m_tmpFile);
        output.close();
    }
    else
    {
        status = ECANCELED;
    }

    return status;
}

bool Command::IsComplete()
{
    std::lock_guard<std::mutex> lock(m_statusMutex);
    return (Command::State::Unknown != m_status.m_state) && (Command::State::Running != m_status.m_state);
}

bool Command::IsCanceled()
{
    return FileExists(m_tmpFile.c_str());
}

std::string Command::GetId()
{
    std::lock_guard<std::mutex> lock(m_statusMutex);
    return m_status.m_id;
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
    m_status.m_exitCode = exitCode;
    m_status.m_textResult = textResult;
    m_status.m_state = state;
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

int ShutdownCommand::Execute(unsigned int maxPayloadSizeBytes)
{
    int exitCode = 0;
    char* textResult = nullptr;

    if (IsCanceled())
    {
        exitCode = ECANCELED;
    }
    else
    {
        SetStatus(0, "", Command::State::Succeeded);

        exitCode = ExecuteCommand(nullptr, m_arguments.c_str(), m_replaceEol, true, maxPayloadSizeBytes, m_timeout, &textResult, nullptr, CommandRunnerLog::Get());

        if (nullptr != textResult)
        {
            free(textResult);
        }
    }

    return exitCode;
}

bool Command::operator ==(const Command& other) const
{
    return ((m_status.m_id == other.m_status.m_id) && (m_arguments == other.m_arguments) && (m_timeout == other.m_timeout) && (m_replaceEol == other.m_replaceEol));
}

Command::Arguments::Arguments(std::string id, std::string command, Command::Action action, unsigned int timeout, bool singleLineTextResult) :
    m_id(id),
    m_arguments(command),
    m_action(action),
    m_timeout(timeout),
    m_singleLineTextResult(singleLineTextResult) { }

std::string Command::Arguments::Serialize(const Command::Arguments& arguments)
{
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    Command::Arguments::Serialize(writer, arguments);
    return buffer.GetString();
}

void Command::Arguments::Serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer, const Command::Arguments& arguments)
{
    writer.StartObject();

    writer.String(g_commandId.c_str());
    writer.String(arguments.m_id.c_str());

    writer.String(g_arguments.c_str());
    writer.String(arguments.m_arguments.c_str());

    writer.String(g_action.c_str());
    writer.Int(static_cast<int>(arguments.m_action));

    writer.String(g_timeout.c_str());
    writer.Uint(arguments.m_timeout);

    writer.String(g_singleLineTextResult.c_str());
    writer.Bool(arguments.m_singleLineTextResult);

    writer.EndObject();
}

Command::Arguments Command::Arguments::Deserialize(const rapidjson::Value& value)
{
    std::string id = "";
    std::string command = "";
    Command::Action action = Command::Action::None;
    unsigned int timeout = 0;
    bool singleLineTextResult = false;

    if (value.IsObject())
    {
        int actionValue = 0;
        if (0 != DeserializeMember(value, g_action.c_str(), actionValue))
        {
            OsConfigLogError(CommandRunnerLog::Get(), "Failed to deserialize %s.%s", g_commandArguments.c_str(), g_action.c_str());
        }
        else
        {
            action = static_cast<Command::Action>(actionValue);

            if (0 == DeserializeMember(value, g_commandId, id))
            {
                switch (action)
                {
                    case Command::Action::Reboot:
                    case Command::Action::Shutdown:
                    case Command::Action::RefreshCommandStatus:
                    case Command::Action::CancelCommand:
                        if (id.empty())
                        {
                            OsConfigLogError(CommandRunnerLog::Get(), "%s.%s is empty", g_commandArguments.c_str(), g_commandId.c_str());
                        }
                        break;

                    case Command::Action::RunCommand:
                        if (!id.empty())
                        {
                            if (0 == DeserializeMember(value, g_arguments, command))
                            {
                                if (!command.empty())
                                {
                                    // Timeout is an optional field
                                    if (0 != DeserializeMember(value, g_timeout, timeout))
                                    {
                                        // Assume default of no timeout (0)
                                        timeout = 0;
                                        OsConfigLogInfo(CommandRunnerLog::Get(), "%s.%s default value '0' (no timeout) used for command id: %s", g_commandArguments.c_str(), g_timeout.c_str(), id.c_str());
                                    }

                                    // SingleLineTextResult is an optional field
                                    if (0 != DeserializeMember(value, g_singleLineTextResult, singleLineTextResult))
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
                        break;

                    case Command::Action::None:
                    default:
                        break;
                }
            }
        }
    }
    else
    {
        OsConfigLogError(CommandRunnerLog::Get(), "Invalid command arguments JSON value");
    }

    return Command::Arguments(id, command, action, timeout, singleLineTextResult);
}

Command::Status::Status(const std::string id, int exitCode, std::string textResult, Command::State state) :
    m_id(id),
    m_exitCode(exitCode),
    m_textResult(textResult),
    m_state(state) { }

std::string Command::Status::Serialize(const Command::Status& status, bool serializeTextResult)
{
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    Command::Status::Serialize(writer, status, serializeTextResult);
    return buffer.GetString();
}

void Command::Status::Serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer, const Command::Status& status, bool serializeTextResult)
{
    writer.StartObject();

    writer.Key(g_commandId.c_str());
    writer.String(status.m_id.c_str());

    writer.Key(g_resultCode.c_str());
    writer.Int(status.m_exitCode);

    if (serializeTextResult)
    {
        writer.Key(g_textResult.c_str());
        writer.String(status.m_textResult.c_str());
    }

    writer.Key(g_currentState.c_str());
    writer.Int(status.m_state);

    writer.EndObject();
}

Command::Status Command::Status::Deserialize(const rapidjson::Value& value)
{
    std::string id = "";
    int exitCode = 0;
    std::string textResult;
    Command::State state = Command::State::Unknown;

    if (value.IsObject())
    {
        // Command id is a required field
        if (0 == DeserializeMember(value, g_commandId, id))
        {
            // Use defaults for other fields
            DeserializeMember(value, g_resultCode, exitCode);
            DeserializeMember(value, g_textResult, textResult);

            int stateValue = 0;
            if (0 == DeserializeMember(value, g_currentState, stateValue))
            {
                state = static_cast<Command::State>(stateValue);
            }
        }
    }
    else
    {
        OsConfigLogError(CommandRunnerLog::Get(), "Invalid command status JSON value");
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
