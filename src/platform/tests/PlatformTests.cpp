// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>

#include <PlatformCommon.h>
#include <MpiServer.h>

namespace Tests
{
    class MpiServerTests : public ::testing::Test {};

    static const char* g_errorClientName = "Error_Client";
    static const char* g_errorComponent = "Error_Component";
    static const char* g_errorObject = "Error_Object";
    static const char* g_mockHandle = "Mock_Client_Handle";
    static const char* g_mockPayload = "\"MockPayload\"";

    static MPI_HANDLE MockCallMpiOpen(const char* clientName, const unsigned int maxPayloadSizeBytes)
    {
        UNUSED(maxPayloadSizeBytes);

        if (strcmp(clientName, g_errorClientName) == 0)
        {
            return nullptr;
        }
        else
        {
            return (MPI_HANDLE)strdup(g_mockHandle);
        }
    }

    static void MockCallMpiClose(MPI_HANDLE handle)
    {
        UNUSED(handle);
    }

    static int MockCallMpiSet(MPI_HANDLE handle, const char* componentName, const char* objectName, MPI_JSON_STRING payload, const int payloadSize)
    {
        UNUSED(handle);
        UNUSED(payload);
        UNUSED(payloadSize);

        return ((0 == strcmp(componentName, g_errorComponent)) && (0 == strcmp(objectName, g_errorObject))) ? -1 : MPI_OK;
    }

    static int MockCallMpiGet(MPI_HANDLE handle, const char* componentName, const char* objectName, MPI_JSON_STRING* payload, int* payloadSize)
    {
        UNUSED(handle);

        if ((0 == strcmp(componentName, g_errorComponent)) && (0 == strcmp(objectName, g_errorObject)))
        {
            return -1;
        }
        else
        {
            *payload = strdup(g_mockPayload);
            *payloadSize = strlen(g_mockPayload);
            return MPI_OK;
        }
    }

    static int MockCallMpiSetDesired(MPI_HANDLE handle, const MPI_JSON_STRING payload, const int payloadSize)
    {
        UNUSED(handle);
        UNUSED(payloadSize);

        return (0 == strcmp(payload, g_mockPayload)) ? MPI_OK : -1;
    }

    static int MockCallMpiGetReported(MPI_HANDLE handle, MPI_JSON_STRING* payload, int* payloadSize)
    {
        UNUSED(handle);

        *payload = strdup(g_mockPayload);
        *payloadSize = strlen(g_mockPayload);
        return MPI_OK;
    }

    static const MPI_CALLS g_mpiCalls =
    {
        MockCallMpiOpen,
        MockCallMpiClose,
        MockCallMpiSet,
        MockCallMpiGet,
        MockCallMpiSetDesired,
        MockCallMpiGetReported
    };

    TEST_F(MpiServerTests, HandleMpiRequestInvalidRequest)
    {
        char* response = nullptr;
        int responseSize = 0;

        EXPECT_EQ(HTTP_BAD_REQUEST, HandleMpiCall(nullptr, "", &response, &responseSize, g_mpiCalls));
        EXPECT_EQ(nullptr, response);
        EXPECT_EQ(0, responseSize);
        FREE_MEMORY(response);

        EXPECT_EQ(HTTP_BAD_REQUEST, HandleMpiCall("", nullptr, &response, &responseSize, g_mpiCalls));
        EXPECT_EQ(nullptr, response);
        EXPECT_EQ(0, responseSize);
        FREE_MEMORY(response);

        EXPECT_EQ(HTTP_BAD_REQUEST, HandleMpiCall("", "", nullptr, &responseSize, g_mpiCalls));
        EXPECT_EQ(nullptr, response);
        EXPECT_EQ(0, responseSize);
        FREE_MEMORY(response);

        EXPECT_EQ(HTTP_BAD_REQUEST, HandleMpiCall("", "", &response, nullptr, g_mpiCalls));
        EXPECT_EQ(nullptr, response);
        EXPECT_EQ(0, responseSize);
        FREE_MEMORY(response);
    }

    TEST_F(MpiServerTests, MpiOpenRequestInvalidRequestBody)
    {
        std::vector<std::string> requests = {
            "{\"MaxPayloadSizeBytes\": 0}",
            "{\"ClientName\": 123, \"MaxPayloadSizeBytes\": 0}",
            "{\"ClientName\": \"\"}",
            "{\"ClientName\": \"\", \"MaxPayloadSizeBytes\": -1}"
        };

        for (auto request : requests)
        {
            char* response = nullptr;
            int responseSize = 0;

            EXPECT_EQ(HTTP_BAD_REQUEST, HandleMpiCall(MPI_OPEN_URI, request.c_str(), &response, &responseSize, g_mpiCalls));
            EXPECT_EQ(nullptr, response);
            EXPECT_EQ(0, responseSize);
            FREE_MEMORY(response);
        }
    }

    TEST_F(MpiServerTests, MpiOpenRequest)
    {
        char* response = nullptr;
        int responseSize = 0;

        EXPECT_EQ(HTTP_INTERNAL_SERVER_ERROR, HandleMpiCall(MPI_OPEN_URI, "{\"ClientName\": \"Error_Client\", \"MaxPayloadSizeBytes\": 0}", &response, &responseSize, g_mpiCalls));
        EXPECT_EQ(nullptr, response);
        EXPECT_EQ(0, responseSize);
        FREE_MEMORY(response);

        EXPECT_EQ(HTTP_OK, HandleMpiCall(MPI_OPEN_URI, "{\"ClientName\": \"Valid_Client\", \"MaxPayloadSizeBytes\": 0}", &response, &responseSize, g_mpiCalls));
        std::string expectedHandle = "\"" + std::string(g_mockHandle) + "\"";
        EXPECT_STREQ(expectedHandle.c_str(), response);
        EXPECT_EQ(expectedHandle.length(), responseSize);
        FREE_MEMORY(response);
    }

    TEST_F(MpiServerTests, MpiCloseRequestInvalidRequestBody)
    {
        std::vector<std::string> requests = {
            "{\"ClientSession\": 123}",
            "{}"
        };

        for (auto request : requests)
        {
            char* response = nullptr;
            int responseSize = 0;

            EXPECT_EQ(HTTP_BAD_REQUEST, HandleMpiCall(MPI_CLOSE_URI, request.c_str(), &response, &responseSize, g_mpiCalls));
            EXPECT_EQ(nullptr, response);
            EXPECT_EQ(0, responseSize);
            FREE_MEMORY(response);
        }
    }

    TEST_F(MpiServerTests, MpiSetRequestInvalidRequestBody)
    {
        std::vector<std::string> requests = {
            "{\"ClientSession\": 123, \"ComponentName\": \"\", \"ObjectName\": \"\", \"Payload\": {}}",
            "{\"ComponentName\": \"\", \"ObjectName\": \"\", \"Payload\": {}}",
            "{\"ClientSession\": \"\", \"ComponentName\": 123, \"ObjectName\": \"\", \"Payload\": {}}",
            "{\"ClientSession\": \"\", \"ObjectName\": \"\", \"Payload\": {}}",
            "{\"ClientSession\": \"\", \"ComponentName\": \"\", \"ObjectName\": 123, \"Payload\": {}}",
            "{\"ClientSession\": \"\", \"ComponentName\": \"\", \"Payload\": {}}",
            "{\"ClientSession\": \"\", \"ComponentName\": \"\", \"ObjectName\": \"\"}"
        };

        for (auto request : requests)
        {
            char* response = nullptr;
            int responseSize = 0;

            EXPECT_EQ(HTTP_BAD_REQUEST, HandleMpiCall(MPI_SET_URI, request.c_str(), &response, &responseSize, g_mpiCalls));
            EXPECT_EQ(nullptr, response);
            EXPECT_EQ(0, responseSize);
            FREE_MEMORY(response);
        }
    }

    TEST_F(MpiServerTests, MpiGetRequestInvalidRequestBody)
    {
        std::vector<std::string> requests = {
            "{\"ClientSession\": 123, \"ComponentName\": \"\", \"ObjectName\": \"\"}",
            "{\"ComponentName\": \"\", \"ObjectName\": \"\"}",
            "{\"ClientSession\": \"\", \"ComponentName\": 123, \"ObjectName\": \"\"}",
            "{\"ClientSession\": \"\", \"ObjectName\": \"\"}",
            "{\"ClientSession\": \"\", \"ComponentName\": \"\", \"ObjectName\": 123}"
        };

        for (auto request : requests)
        {
            char* response = nullptr;
            int responseSize = 0;

            EXPECT_EQ(HTTP_BAD_REQUEST, HandleMpiCall(MPI_GET_URI, request.c_str(), &response, &responseSize, g_mpiCalls));
            EXPECT_EQ(nullptr, response);
            EXPECT_EQ(0, responseSize);
            FREE_MEMORY(response);
        }
    }

    TEST_F(MpiServerTests, MpiSetDesiredRequestInvalidRequestBody)
    {
        std::vector<std::string> requests = {
            "{\"ClientSession\": 123, \"Payload\": {}}",
            "{\"Payload\": {}}",
            "{\"ClientSession\": \"\"}"
        };

        for (auto request : requests)
        {
            char* response = nullptr;
            int responseSize = 0;

            EXPECT_EQ(HTTP_BAD_REQUEST, HandleMpiCall(MPI_SET_DESIRED_URI, request.c_str(), &response, &responseSize, g_mpiCalls));
            EXPECT_EQ(nullptr, response);
            EXPECT_EQ(0, responseSize);
            FREE_MEMORY(response);
        }
    }

    TEST_F(MpiServerTests, MpiGetReportedRequestInvalidRequestBody)
    {
        std::vector<std::string> requests = {
            "{\"ClientSession\": 123}",
            "{}"
        };

        for (auto request : requests)
        {
            char* response = nullptr;
            int responseSize = 0;

            EXPECT_EQ(HTTP_BAD_REQUEST, HandleMpiCall(MPI_GET_REPORTED_URI, request.c_str(), &response, &responseSize, g_mpiCalls));
            EXPECT_EQ(nullptr, response);
            EXPECT_EQ(0, responseSize);
            FREE_MEMORY(response);
        }
    }

    TEST_F(MpiServerTests, MpiCloseRequest)
    {
        char* response = nullptr;
        int responseSize = 0;

        EXPECT_EQ(HTTP_OK, HandleMpiCall(MPI_CLOSE_URI, "{\"ClientSession\": \"Valid_Client\"}", &response, &responseSize, g_mpiCalls));
        EXPECT_EQ(nullptr, response);
        EXPECT_EQ(0, responseSize);
        FREE_MEMORY(response);
    }

    TEST_F(MpiServerTests, MpiSetRequest)
    {
        char* response = nullptr;
        int responseSize = 0;

        EXPECT_EQ(HTTP_INTERNAL_SERVER_ERROR, HandleMpiCall(MPI_SET_URI, "{\"ClientSession\": \"Valid_Client\", \"ComponentName\": \"Error_Component\", \"ObjectName\": \"Error_Object\", \"Payload\": {}}", &response, &responseSize, g_mpiCalls));
        EXPECT_NE(nullptr, response);
        EXPECT_GT(responseSize, strlen("\"\""));

        responseSize = 0;
        FREE_MEMORY(response);

        EXPECT_EQ(HTTP_OK, HandleMpiCall(MPI_SET_URI, "{\"ClientSession\": \"Valid_Client\", \"ComponentName\": \"\", \"ObjectName\": \"\", \"Payload\": {}}", &response, &responseSize, g_mpiCalls));
        EXPECT_EQ(nullptr, response);
        EXPECT_EQ(0, responseSize);
        FREE_MEMORY(response);
    }

    TEST_F(MpiServerTests, MpiGetRequest)
    {
        char* response = nullptr;
        int responseSize = 0;

        EXPECT_EQ(HTTP_INTERNAL_SERVER_ERROR, HandleMpiCall(MPI_GET_URI, "{\"ClientSession\": \"Valid_Client\", \"ComponentName\": \"Error_Component\", \"ObjectName\": \"Error_Object\"}", &response, &responseSize, g_mpiCalls));
        EXPECT_NE(nullptr, response);
        EXPECT_GT(responseSize, strlen("\"\""));

        responseSize = 0;
        FREE_MEMORY(response);

        EXPECT_EQ(HTTP_OK, HandleMpiCall(MPI_GET_URI, "{\"ClientSession\": \"Valid_Client\", \"ComponentName\": \"\", \"ObjectName\": \"\"}", &response, &responseSize, g_mpiCalls));
        EXPECT_STREQ(g_mockPayload, response);
        EXPECT_EQ(strlen(g_mockPayload), responseSize);
        FREE_MEMORY(response);
    }

    TEST_F(MpiServerTests, MpiSetDesiredRequest)
    {
        char* response = nullptr;
        int responseSize = 0;

        EXPECT_EQ(HTTP_INTERNAL_SERVER_ERROR, HandleMpiCall(MPI_SET_DESIRED_URI, "{\"ClientSession\": \"Valid_Client\", \"Payload\": {}}", &response, &responseSize, g_mpiCalls));
        EXPECT_NE(nullptr, response);
        EXPECT_GT(responseSize, strlen("\"\""));

        responseSize = 0;
        FREE_MEMORY(response);

        EXPECT_EQ(HTTP_OK, HandleMpiCall(MPI_SET_DESIRED_URI, "{\"ClientSession\": \"Valid_Client\", \"Payload\": \"MockPayload\"}", &response, &responseSize, g_mpiCalls));
        EXPECT_EQ(nullptr, response);
        EXPECT_EQ(0, responseSize);
        FREE_MEMORY(response);
    }

    TEST_F(MpiServerTests, MpiGetReportedRequest)
    {
        char* response = nullptr;
        int responseSize = 0;

        EXPECT_EQ(HTTP_OK, HandleMpiCall(MPI_GET_REPORTED_URI, "{\"ClientSession\": \"Valid_Client\"}", &response, &responseSize, g_mpiCalls));
        EXPECT_STREQ(g_mockPayload, response);
        EXPECT_EQ(strlen(g_mockPayload), responseSize);
        FREE_MEMORY(response);
    }
}
