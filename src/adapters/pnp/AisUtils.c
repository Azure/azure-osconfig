// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "inc/AgentCommon.h"
#include "inc/AisUtils.h"

#define AIS_SOCKET_PREFIX "/run/aziot"
#define AIS_IDENTITY_SOCKET AIS_SOCKET_PREFIX "/identityd.sock"
#define AIS_SIGN_SOCKET AIS_SOCKET_PREFIX "/keyd.sock"
#define AIS_CERT_SOCKET AIS_SOCKET_PREFIX "/certd.sock"

#define AIS_API_URI_PREFIX "http://aziot"
#define AIS_IDENTITY_URI AIS_API_URI_PREFIX "/identities/identity"
#define AIS_SIGN_URI AIS_API_URI_PREFIX "/sign"
#define AIS_CERT_URI AIS_API_URI_PREFIX "/certificates"

#define AIS_API_VERSION "api-version=2020-09-01"
#define AIS_IDENTITY_REQUEST_URI AIS_IDENTITY_URI "?" AIS_API_VERSION
#define AIS_SIGN_REQUEST_URI AIS_SIGN_URI "?" AIS_API_VERSION

#define AIS_SIGN_ALGORITHM_VALUE "HMAC-SHA256"

#define AIS_REQUEST_PORT 80

// AIS sign request format:
// {
//   "keyHandle":"<key>",
//   "algorithm":"HMAC-SHA256",
//   "parameters" : {
//       "message":"<message>"
//   }
// }
#define AIS_SIGN_KEYHANDLE "keyHandle"
#define AIS_SIGN_ALGORITHM "algorithm"
#define AIS_SIGN_PARAMETERS_MESSAGE "parameters.message"

// AIS identity response format:
// {
//   "type":"aziot",
//   "spec":{
//       "hubName":"<hub name>.azure-devices.net",
//       "gatewayHost":"<gateway host>",
//       "deviceId":"<device id>",
//       "moduleId":"<module id>",
//       "auth":{
//           "type":"<sas/x509>",
//           "keyHandle":"<key handle>",
//           "certId":"<certificate id, for x509>"
//       }
//    }
// }
#define AIS_RESPONSE_SPEC "spec"
#define AIS_RESPONSE_HUBNAME "hubName"
#define AIS_RESPONSE_GATEWAYHOST "gatewayHost"
#define AIS_RESPONSE_DEVICEID "deviceId"
#define AIS_RESPONSE_MODULEID "moduleId"
#define AIS_RESPONSE_AUTH "auth"
#define AIS_RESPONSE_AUTH_KEYHANDLE "keyHandle"
#define AIS_RESPONSE_AUTH_TYPE "type"
#define AIS_RESPONSE_AUTH_TYPE_SAS "sas"
#define AIS_RESPONSE_AUTH_TYPE_X509 "x509"
#define AIS_RESPONSE_AUTH_CERTID "certId"

#define AIS_SIGN_RESP_SIGNATURE "signature"
#define AIS_CERT_RESP_PEM "pem"

#define HTTP_HEADER_NAME "Content-Type"
#define HTTP_HEADER_NAME_LOWERCASE "content-type"
#define HTTP_HEADER_VALUE "application/json"

#define AIS_RESPONSE_SIZE_MIN 16
#define AIS_RESPONSE_SIZE_MAX 8192

#define AIS_SUCCESS 200
#define AIS_ERROR 400

// 400 milliseconds
#define AIS_WAIT_TIMEOUT 400

// 2 hours, in seconds (60 * 60 * 2 = 7,200)
#define AIS_TOKEN_EXPIRY_TIME 7200

#define TIME_T_MAX_CHARS 12

typedef struct AIS_HTTP_CONTEXT
{
    bool inProgress;
    BUFFER_HANDLE httpResponse;
    int httpStatus;
} AIS_HTTP_CONTEXT;

const char* g_uriToSignTemplate = "%s\n%s";
const char* g_certificateUriTemplate = "%s/%s?%s";
const char* g_resourceUriDeviceTemplate = "%s/devices/%s";
const char* g_resourceUriModuleTemplate = "%s/devices/%s/modules/%s";
const char* g_sharedAccessSignatureTemplate = "SharedAccessSignature sr=%s&sig=%s&se=%s";

const char* g_connectionStringSasDeviceTemplate = "HostName=%s;DeviceId=%s;SharedAccessSignature=%s";
const char* g_connectionStringSasModuleTemplate = "HostName=%s;DeviceId=%s;ModuleId=%s;SharedAccessSignature=%s";
const char* g_connectionStringX509DeviceTemplate = "HostName=%s;DeviceId=%s;x509=true";
const char* g_connectionStringX509ModuleTemplate = "HostName=%s;DeviceId=%s;ModuleId=%s;x509=true";

const char* g_connectionStringSasGatewayHostDeviceTemplate = "HostName=%s;DeviceId=%s;SharedAccessSignature=%s;GatewayHostName=%s";
const char* g_connectionStringSasGatewayHostModuleTemplate = "HostName=%s;DeviceId=%s;ModuleId=%s;SharedAccessSignature=%s;GatewayHostName=%s";
const char* g_connectionStringX509GatewayHostDeviceTemplate = "HostName=%s;DeviceId=%s;x509=true;GatewayHostName=%s";
const char* g_connectionStringX509GatewayHostModuleTemplate = "HostName=%s;DeviceId=%s;ModuleId=%s;x509=true;GatewayHostName=%s";

static void HttpErrorCallback(void* callback, HTTP_CALLBACK_REASON reason)
{
    AIS_HTTP_CONTEXT* context = (AIS_HTTP_CONTEXT*)callback;
    if (NULL != context)
    {
        context->inProgress = false;
        context->httpStatus = AIS_ERROR;
        OsConfigLogError(GetLog(), "HttpErrorCallback: HTTP error");
    }
    UNUSED(reason);
}

static void HttpReceiveCallback(void* callback, HTTP_CALLBACK_REASON reason, const unsigned char* content, size_t contentSize, unsigned int statusCode, HTTP_HEADERS_HANDLE responseHeaders)
{
    const char* contentType = NULL;
    AIS_HTTP_CONTEXT* context = (AIS_HTTP_CONTEXT*)callback;

    OsConfigLogInfo(GetLog(), "HttpReceiveCallback: reason %d, content size %d, status code %u", reason, (int)contentSize, statusCode);

    if (NULL == context)
    {
        OsConfigLogError(GetLog(), "HttpReceiveCallback: invalid callback");
        return;
    }

    context->httpStatus = AIS_ERROR;

    if ((HTTP_CALLBACK_REASON_OK != reason) || (300 <= statusCode) || (NULL == content))
    {
        OsConfigLogError(GetLog(), "HttpReceiveCallback: HTTP error (status code: %u)", statusCode);
    }
    else if ((AIS_RESPONSE_SIZE_MIN > contentSize) || (AIS_RESPONSE_SIZE_MAX < contentSize))
    {
        OsConfigLogError(GetLog(), "HttpReceiveCallback: response content size out of supported range (%u, %u)", AIS_RESPONSE_SIZE_MIN, AIS_RESPONSE_SIZE_MAX);
    }
    else
    {
        contentType = HTTPHeaders_FindHeaderValue(responseHeaders, HTTP_HEADER_NAME_LOWERCASE);
        if ((NULL != contentType) && (0 == strcmp(contentType, HTTP_HEADER_VALUE)))
        {
            context->httpResponse = BUFFER_create(content, contentSize);
            if (NULL == context->httpResponse)
            {
                OsConfigLogError(GetLog(), "HttpReceiveCallback: out of memory");
            }
            else
            {
                context->httpStatus = AIS_SUCCESS;
                OsConfigLogInfo(GetLog(), "HttpReceiveCallback: success");
            }
        }
        else
        {
            OsConfigLogError(GetLog(), "HttpReceiveCallback: invalid value");
        }
    }

    context->inProgress = false;
}

static void HttpConnectedCallback(void* callback, HTTP_CALLBACK_REASON connectResult)
{
    AIS_HTTP_CONTEXT* context = (AIS_HTTP_CONTEXT*)callback;
    if (context == NULL)
    {
        OsConfigLogError(GetLog(), "HttpConnectedCallback: invalid callback");
        return;
    }

    OsConfigLogInfo(GetLog(), "HttpConnectedCallback: connect result %d", (int)connectResult);
    context->httpStatus = (HTTP_CALLBACK_REASON_OK == connectResult) ? AIS_SUCCESS : AIS_ERROR;
}

static HTTP_CLIENT_HANDLE HttpOpenClient(const char* udsSocketPath, AIS_HTTP_CONTEXT* context)
{
    SOCKETIO_CONFIG config = {0};
    HTTP_CLIENT_RESULT httpResult = HTTP_CLIENT_OK;
    HTTP_CLIENT_HANDLE clientHandle = NULL;

    if (NULL == udsSocketPath)
    {
        OsConfigLogError(GetLog(), "HttpOpenClient: invalid argument");
        return clientHandle;
    }

    config.accepted_socket = NULL;
    config.hostname = udsSocketPath;
    config.port = AIS_REQUEST_PORT;

    if (NULL == (clientHandle = uhttp_client_create(socketio_get_interface_description(), &config, HttpErrorCallback, context)))
    {
        OsConfigLogError(GetLog(), "HttpOpenClient: uhttp_client_create failed");
    }
    else if (HTTP_CLIENT_OK != (httpResult = uhttp_client_set_option(clientHandle, OPTION_ADDRESS_TYPE, OPTION_ADDRESS_TYPE_DOMAIN_SOCKET)))
    {
        OsConfigLogError(GetLog(), "HttpOpenClient: uhttp_client_set_option failed with %d", httpResult);
        uhttp_client_destroy(clientHandle);
        clientHandle = NULL;

    }
    else if (HTTP_CLIENT_OK != (httpResult = uhttp_client_open(clientHandle, udsSocketPath, 0, HttpConnectedCallback, context)))
    {
        OsConfigLogError(GetLog(), "HttpOpenClient: uhttp_client_open failed with %d", httpResult);
        uhttp_client_destroy(clientHandle);
        clientHandle = NULL;
    }

    return clientHandle;
}

static HTTP_HEADERS_HANDLE HttpCreateHeader(void)
{
    HTTP_HEADERS_RESULT httpHeadersResult = HTTP_HEADERS_OK;
    HTTP_HEADERS_HANDLE httpHeadersHandle = NULL;

    if (NULL == (httpHeadersHandle = HTTPHeaders_Alloc()))
    {
        OsConfigLogError(GetLog(), "HttpCreateHeader: HTTPHeaders_Alloc failed, out of memory");
    }
    else if (HTTP_HEADERS_OK != (httpHeadersResult = HTTPHeaders_AddHeaderNameValuePair(httpHeadersHandle, HTTP_HEADER_NAME, HTTP_HEADER_VALUE)))
    {
        OsConfigLogError(GetLog(), "HttpCreateHeader: HTTPHeaders_AddHeaderNameValuePair(%s, %s) failed with %d", HTTP_HEADER_NAME, HTTP_HEADER_VALUE, (int)httpHeadersResult);
    }

    return httpHeadersHandle;
}

static int SendAisRequest(const char* udsSocketPath, const char* apiUriPath, const char* payload, char** response)
{
    size_t payloadLen = 0;
    HTTP_CLIENT_RESULT httpResult = HTTP_CLIENT_OK;
    HTTP_CLIENT_HANDLE clientHandle = NULL;
    HTTP_HEADERS_HANDLE httpHeadersHandle = NULL;
    HTTP_CLIENT_REQUEST_TYPE clientRequestType = HTTP_CLIENT_REQUEST_GET;
    AIS_HTTP_CONTEXT context = {0};
    time_t startTime = 0;
    bool timedOut = false;
    size_t responseLen = 0;
    int bufferSize = 0;
    int result = AIS_ERROR;

    context.httpStatus = AIS_ERROR;

    if ((NULL == udsSocketPath) || (NULL == apiUriPath) || (NULL == response))
    {
        OsConfigLogError(GetLog(), "SendAisRequest: invalid argument");
        return result;
    }

    context.inProgress = true;
    context.httpResponse = NULL;

    *response = NULL;

    if (NULL == (clientHandle = HttpOpenClient(udsSocketPath, &context)))
    {
        OsConfigLogError(GetLog(), "SendAisRequest: HttpOpenClient failed");
    }

    // Send the request and wait for completion
    if (NULL != clientHandle)
    {
        if (NULL != payload)
        {
            if (NULL != (httpHeadersHandle = HttpCreateHeader()))
            {
                clientRequestType = HTTP_CLIENT_REQUEST_POST;
                payloadLen = strlen(payload);
            }
        }

        OsConfigLogInfo(GetLog(), "SendAisRequest: %s %s to %s, %d long", (HTTP_CLIENT_REQUEST_POST == clientRequestType) ? "POST" : "GET", apiUriPath, udsSocketPath, (int)payloadLen);
        if (IsFullLoggingEnabled())
        {
            OsConfigLogInfo(GetLog(), "SendAisRequest payload: %s", payload);
        }

        if (HTTP_CLIENT_OK != (httpResult = uhttp_client_execute_request(clientHandle, clientRequestType, apiUriPath, httpHeadersHandle, (const unsigned char*)payload, payloadLen, HttpReceiveCallback, &context)))
        {
            OsConfigLogError(GetLog(), "SendAisRequest: uhttp_client_execute_request failed with %d", (int)httpResult);
        }
        else
        {
            OsConfigLogInfo(GetLog(), "SendAisRequest: uhttp_client_execute_request sent, entering wait");
            startTime = get_time(NULL);
            timedOut = false;

            do
            {
                uhttp_client_dowork(clientHandle);
                timedOut = ((difftime(get_time(NULL), startTime) > AIS_WAIT_TIMEOUT)) ? true : false;

            } while ((true == context.inProgress) && (false == timedOut));

            if (true == timedOut)
            {
                OsConfigLogError(GetLog(), "SendAisRequest: timed out waiting for uhttp_client_execute_request completion");
                result = AIS_ERROR;
            }
            else
            {
                result = context.httpStatus;
                OsConfigLogInfo(GetLog(), "SendAisRequest: uhttp_client_execute_request complete, wait ended with %d", result);
            }
        }
    }

    // Get the response
    if (AIS_SUCCESS == result)
    {
        if (0 != (bufferSize = BUFFER_size(context.httpResponse, &responseLen)))
        {
            OsConfigLogError(GetLog(), "SendAisRequest: BUFFER_size failed returning non-zero %d", bufferSize);
            result = AIS_ERROR;
        }
        else if ((responseLen > AIS_RESPONSE_SIZE_MAX) && (responseLen < AIS_RESPONSE_SIZE_MIN))
        {
            OsConfigLogError(GetLog(), "SendAisRequest: response size out of supported range (%d, %d)", AIS_RESPONSE_SIZE_MIN, AIS_RESPONSE_SIZE_MAX);
            result = AIS_ERROR;
        }
        else
        {
            if (NULL != (*response = (char*)malloc(responseLen + 1)))
            {
                memcpy(&(*response)[0], BUFFER_u_char(context.httpResponse), responseLen);
                (*response)[responseLen] = 0;
                result = AIS_SUCCESS;
                OsConfigLogInfo(GetLog(), "SendAisRequest: ok");
                if (IsFullLoggingEnabled())
                {
                    OsConfigLogInfo(GetLog(), "SendAisRequest response: %s", *response);
                }
            }
            else
            {
                OsConfigLogError(GetLog(), "SendAisRequest: out of memory allocating %d bytes", (int)(responseLen + 1));
                result = AIS_ERROR;
            }
        }
    }

    // Clean-up
    HTTPHeaders_Free(httpHeadersHandle);
    uhttp_client_close(clientHandle, NULL, NULL);
    uhttp_client_destroy(clientHandle);

    if (NULL != context.httpResponse)
    {
        BUFFER_delete(context.httpResponse);
    }

    if (AIS_SUCCESS != result)
    {
        FREE_MEMORY(*response);
    }

    if (AIS_SUCCESS == result)
    {
        OsConfigLogInfo(GetLog(), "SendAisRequest(%s) ok", udsSocketPath);
    }
    else
    {
        OsConfigLogError(GetLog(), "SendAisRequest(%s) failed with %d", udsSocketPath, result);
    }

    return result;
}

static int RequestSignatureFromAis(const char* keyHandle, const char* deviceUri, const char* expiry, char** response)
{
    JSON_Object* payloadObj = NULL;
    JSON_Value* payloadValue = NULL;
    char* serializedPayload = NULL;
    char* uriToSign = NULL;
    STRING_HANDLE encodedUriToSign = NULL;
    int result = AIS_ERROR;

    if ((NULL == response) || (NULL == keyHandle) || (NULL == deviceUri) || (NULL == expiry))
    {
        OsConfigLogError(GetLog(), "RequestSignatureFromAis: invalid argument");
        return result;
    }

    *response = NULL;

    if (NULL == (payloadValue = json_value_init_object()))
    {
        OsConfigLogError(GetLog(), "RequestSignatureFromAis: json_value_init_object failed");
    }
    else if (NULL == (payloadObj = json_value_get_object(payloadValue)))
    {
        OsConfigLogError(GetLog(), "RequestSignatureFromAis: json_value_get_object failed");
    }
    else if (NULL == (uriToSign = FormatAllocateString(g_uriToSignTemplate, deviceUri, expiry)))
    {
        OsConfigLogError(GetLog(), "RequestSignatureFromAis: failed to format device URI to sign");
    }
    else if (NULL == (encodedUriToSign = Azure_Base64_Encode_Bytes((unsigned char*)uriToSign, strlen(uriToSign))))
    {
        OsConfigLogError(GetLog(), "RequestSignatureFromAis: Azure_Base64_Encode_Bytes failed");
    }
    else if (JSONSuccess != json_object_set_string(payloadObj, AIS_SIGN_KEYHANDLE, keyHandle))
    {
        OsConfigLogError(GetLog(), "RequestSignatureFromAis: json_object_set_string(%s) failed", AIS_SIGN_KEYHANDLE);
    }
    else if (JSONSuccess != json_object_set_string(payloadObj, AIS_SIGN_ALGORITHM, AIS_SIGN_ALGORITHM_VALUE))
    {
        OsConfigLogError(GetLog(), "RequestSignatureFromAis: json_object_set_string(AIS_SIGN_ALGORITHM) failed");
    }
    else if (JSONSuccess != json_object_dotset_string(payloadObj, AIS_SIGN_PARAMETERS_MESSAGE, STRING_c_str(encodedUriToSign)))
    {
        OsConfigLogError(GetLog(), "RequestSignatureFromAis: json_object_dotset_string(AIS_SIGN_PARAMETERS_MESSAGE) failed");
    }
    else
    {
        serializedPayload = json_serialize_to_string(payloadValue);
        if (NULL != serializedPayload)
        {
            result = SendAisRequest(AIS_SIGN_SOCKET, AIS_SIGN_REQUEST_URI, serializedPayload, response);
        }
        else
        {
            OsConfigLogError(GetLog(), "RequestSignatureFromAis: json_serialize_to_string failed");
        }
    }

    json_value_free(payloadValue);
    FREE_MEMORY(serializedPayload);
    STRING_delete(encodedUriToSign);
    FREE_MEMORY(uriToSign);

    if (AIS_SUCCESS != result)
    {
        FREE_MEMORY(*response);
    }

    OsConfigLogInfo(GetLog(), "RequestSignatureFromAis: %d", result);
    return result;
}

static int RequestCertificateFromAis(const char* certificateId, char** response)
{
    int result = AIS_ERROR;

    char* requestUri = NULL;

    if (NULL == response)
    {
        OsConfigLogError(GetLog(), "RequestCertificateFromAis: invalid argument");
        return result;
    }

    *response = NULL;

    if (NULL == (requestUri = FormatAllocateString(g_certificateUriTemplate, AIS_CERT_URI, certificateId, AIS_API_VERSION)))
    {
        OsConfigLogError(GetLog(), "RequestCertificateFromAis: failed to format certificate URI string");
    }
    else
    {
        result = SendAisRequest(AIS_CERT_SOCKET, requestUri, NULL, response);
    }

    FREE_MEMORY(requestUri);

    return result;
}

char* RequestConnectionStringFromAis(char** x509Certificate, char** x509PrivateKeyHandle)
{
    char* connectionString = NULL;
    char* resourceUri = NULL;
    char* sharedAccessSignature = NULL;
    char* identityResponseString = NULL;
    char* signResponseString = NULL;
    char* certificateResponseString = NULL;
    char expiryStr[TIME_T_MAX_CHARS] = {0};
    JSON_Value* identityResponseJson = NULL;
    JSON_Value* signResponseJson = NULL;
    JSON_Value* certificateResponseJson = NULL;
    const JSON_Object* identityResponseJsonObject = NULL;
    const JSON_Object* signResponseJsonObject = NULL;
    const JSON_Object* certificateResponseJsonObject = NULL;
    const JSON_Object* specJsonObject = NULL;
    const JSON_Object* authJsonObject = NULL;
    STRING_HANDLE encodedSignature = NULL;
    const char* hubName = NULL;
    const char* gatewayHost = NULL;
    const char* deviceId = NULL;
    const char* moduleId = NULL;
    const char* authType = NULL;
    const char* keyHandle = NULL;
    const char* certId = NULL;
    const char* signature = NULL;
    const char* certificate = NULL;
    char* connectAs = NULL;
    char* connectTo = NULL;
    bool useModuleId = true;
    bool useGatewayHost = true;
    int result = AIS_ERROR;

    time_t expiryTime = (time_t)(time(NULL) + AIS_TOKEN_EXPIRY_TIME);

    if (AIS_SUCCESS != (result = SendAisRequest(AIS_IDENTITY_SOCKET, AIS_IDENTITY_REQUEST_URI, NULL, &identityResponseString)))
    {
        // Failure already logged by SendAisRequest
    }
    else if (NULL == (identityResponseJson = json_parse_string(identityResponseString)))
    {
        OsConfigLogError(GetLog(), "RequestConnectionStringFromAis: json_parse_string failed");
        result = AIS_ERROR;
    }
    else if (NULL == (identityResponseJsonObject = json_value_get_object(identityResponseJson)))
    {
        OsConfigLogError(GetLog(), "RequestConnectionStringFromAis: json_value_get_object(identityResponseJsonObject) failed");
        result = AIS_ERROR;
    }
    else if (NULL == (specJsonObject = json_value_get_object(json_object_get_value(identityResponseJsonObject, AIS_RESPONSE_SPEC))))
    {
        OsConfigLogError(GetLog(), "RequestConnectionStringFromAis: json_object_get_value(%s) failed", AIS_RESPONSE_SPEC);
        result = AIS_ERROR;
    }
    else if (NULL == (hubName = json_object_get_string(specJsonObject, AIS_RESPONSE_HUBNAME)))
    {
        OsConfigLogError(GetLog(), "RequestConnectionStringFromAis: json_object_get_string(%s) failed", AIS_RESPONSE_HUBNAME);
        result = AIS_ERROR;
    }
    else if (NULL == (deviceId = json_object_get_string(specJsonObject, AIS_RESPONSE_DEVICEID)))
    {
        OsConfigLogError(GetLog(), "RequestConnectionStringFromAis: json_object_get_string(%s) failed", AIS_RESPONSE_DEVICEID);
        result = AIS_ERROR;
    }
    else if (NULL == (authJsonObject = json_value_get_object(json_object_get_value(specJsonObject, AIS_RESPONSE_AUTH))))
    {
        OsConfigLogError(GetLog(), "RequestConnectionStringFromAis: json_value_get_object(%s) failed", AIS_RESPONSE_AUTH);
        result = AIS_ERROR;
    }
    else if (NULL == (authType = json_object_get_string(authJsonObject, AIS_RESPONSE_AUTH_TYPE)))
    {
        OsConfigLogError(GetLog(), "RequestConnectionStringFromAis: json_value_get_object(%s) failed", AIS_RESPONSE_AUTH_TYPE);
        result = AIS_ERROR;
    }
    else if (NULL == (keyHandle = json_object_get_string(authJsonObject, AIS_RESPONSE_AUTH_KEYHANDLE)))
    {
        OsConfigLogError(GetLog(), "RequestConnectionStringFromAis: json_object_get_string(%s) failed", AIS_RESPONSE_AUTH_KEYHANDLE);
        result = AIS_ERROR;
    }
    else if (NULL == (moduleId = json_object_get_string(specJsonObject, AIS_RESPONSE_MODULEID)))
    {
        OsConfigLogInfo(GetLog(), "RequestConnectionStringFromAis: no module id");
        useModuleId = false;
        result = AIS_SUCCESS;
    }
    else if (NULL == (gatewayHost = json_object_get_string(specJsonObject, AIS_RESPONSE_GATEWAYHOST)))
    {
        OsConfigLogInfo(GetLog(), "RequestConnectionStringFromAis: no gateway host");
        useGatewayHost = false;
        result = AIS_SUCCESS;
    }

    if (AIS_SUCCESS == result)
    {
        if (0 == strcmp(authType, AIS_RESPONSE_AUTH_TYPE_SAS))
        {
            OsConfigLogInfo(GetLog(), "RequestConnectionStringFromAis: SAS Token Authentication");

            if (NULL == (resourceUri = (useModuleId ?
                FormatAllocateString(g_resourceUriModuleTemplate, hubName, deviceId, moduleId) :
                FormatAllocateString(g_resourceUriDeviceTemplate, hubName, deviceId))))
            {
                OsConfigLogError(GetLog(), "RequestConnectionStringFromAis: failed to format resource URI string");
                result = AIS_ERROR;
            }
            else if (0 == snprintf(expiryStr, ARRAY_SIZE(expiryStr), "%ld", expiryTime))
            {
                OsConfigLogError(GetLog(), "RequestConnectionStringFromAis: snprintf for expiry string failed");
                result = AIS_ERROR;
            }
            else if (AIS_SUCCESS != (result = RequestSignatureFromAis(keyHandle, resourceUri, expiryStr, &signResponseString)))
            {
                OsConfigLogError(GetLog(), "RequestConnectionStringFromAis: RequestSignatureFromAis failed");
            }
            else if (NULL == (signResponseJson = json_parse_string(signResponseString)))
            {
                OsConfigLogError(GetLog(), "RequestConnectionStringFromAis: json_parse_string(signResponseString) failed");
                result = AIS_ERROR;
            }
            else if (NULL == (signResponseJsonObject = json_value_get_object(signResponseJson)))
            {
                OsConfigLogError(GetLog(), "RequestConnectionStringFromAis: json_value_get_object(signResponseJsonObject) failed");
                result = AIS_ERROR;
            }
            else if (NULL == (signature = json_object_get_string(signResponseJsonObject, AIS_SIGN_RESP_SIGNATURE)))
            {
                OsConfigLogError(GetLog(), "RequestConnectionStringFromAis: json_object_get_string(%s) failed", AIS_SIGN_RESP_SIGNATURE);
                result = AIS_ERROR;
            }
            else if (NULL == (encodedSignature = URL_EncodeString(signature)))
            {
                OsConfigLogError(GetLog(), "RequestConnectionStringFromAis: URL_EncodeString(signature) failed");
                result = AIS_ERROR;
            }
            else if (NULL == (sharedAccessSignature = FormatAllocateString(g_sharedAccessSignatureTemplate, resourceUri, STRING_c_str(encodedSignature), expiryStr)))
            {
                OsConfigLogError(GetLog(), "RequestConnectionStringFromAis: failed to format shared access signature string");
                result = AIS_ERROR;
            }
            else if (NULL == (connectionString = (useModuleId ? (useGatewayHost ?
                FormatAllocateString(g_connectionStringSasGatewayHostModuleTemplate, hubName, deviceId, moduleId, sharedAccessSignature, gatewayHost) :
                FormatAllocateString(g_connectionStringSasModuleTemplate, hubName, deviceId, moduleId, sharedAccessSignature)) : (useGatewayHost ?
                FormatAllocateString(g_connectionStringSasGatewayHostDeviceTemplate, hubName, deviceId, sharedAccessSignature, gatewayHost) :
                FormatAllocateString(g_connectionStringSasDeviceTemplate, hubName, deviceId, sharedAccessSignature)))))
            {
                OsConfigLogError(GetLog(), "RequestConnectionStringFromAis: failed to format connection string");
                result = AIS_ERROR;
            }
        }
        else if (0 == strcmp(authType, AIS_RESPONSE_AUTH_TYPE_X509))
        {
            OsConfigLogInfo(GetLog(), "RequestConnectionStringFromAis: X.509 Certificate-based Authentication");

            if ((NULL == x509Certificate) || (NULL == x509PrivateKeyHandle))
            {
                OsConfigLogError(GetLog(), "RequestConnectionStringFromAis: invalid argument(s) for X.509 authentication");
                result = AIS_ERROR;
            }
            else if (NULL == (certId = json_object_get_string(authJsonObject, AIS_RESPONSE_AUTH_CERTID)))
            {
                OsConfigLogError(GetLog(), "RequestConnectionStringFromAis: json_object_get_string(%s) failed", AIS_RESPONSE_AUTH_CERTID);
                result = AIS_ERROR;
            }
            else if (AIS_ERROR == RequestCertificateFromAis(certId, &certificateResponseString))
            {
                OsConfigLogError(GetLog(), "RequestConnectionStringFromAis: RequestCertificateFromAis failed");
                result = AIS_ERROR;
            }
            else if (NULL == (certificateResponseJson = json_parse_string(certificateResponseString)))
            {
                OsConfigLogError(GetLog(), "RequestConnectionStringFromAis: json_parse_string(certificateResponseString) failed");
                result = AIS_ERROR;
            }
            else if (NULL == (certificateResponseJsonObject = json_value_get_object(certificateResponseJson)))
            {
                OsConfigLogError(GetLog(), "RequestConnectionStringFromAis: json_value_get_object(certificateResponseJson) failed");
                result = AIS_ERROR;
            }
            else if (NULL == (certificate = json_object_get_string(certificateResponseJsonObject, AIS_CERT_RESP_PEM)))
            {
                OsConfigLogError(GetLog(), "RequestConnectionStringFromAis: json_object_get_string(%s) failed", AIS_CERT_RESP_PEM);
                result = AIS_ERROR;
            }
            else if (0 != mallocAndStrcpy_s(x509Certificate, certificate))
            {
                OsConfigLogError(GetLog(), "RequestConnectionStringFromAis: mallocAndStrcpy_s(%s) failed, cannot make copy of X.509 certificate", certificate);
                result = AIS_ERROR;
            }
            else if (0 != mallocAndStrcpy_s(x509PrivateKeyHandle, keyHandle))
            {
                OsConfigLogError(GetLog(), "RequestConnectionStringFromAis: mallocAndStrcpy_s(%s) failed, cannot make copy of X.509 private key handle", keyHandle);
                result = AIS_ERROR;
            }
            else if (NULL == (connectionString = (useModuleId ? (useGatewayHost ?
                FormatAllocateString(g_connectionStringX509GatewayHostModuleTemplate, hubName, deviceId, moduleId, gatewayHost) :
                FormatAllocateString(g_connectionStringX509ModuleTemplate, hubName, deviceId, moduleId)) : (useGatewayHost ?
                FormatAllocateString(g_connectionStringX509GatewayHostDeviceTemplate, hubName, deviceId, gatewayHost) :
                FormatAllocateString(g_connectionStringX509DeviceTemplate, hubName, deviceId)))))
            {
                OsConfigLogError(GetLog(), "RequestConnectionStringFromAis: failed to format connection string");
                result = AIS_ERROR;
            }
        }
        else
        {
            OsConfigLogError(GetLog(), "RequestConnectionStringFromAis: unsupported authentication type (%s)", authType);
            result = AIS_ERROR;
        }
    }

    if (AIS_SUCCESS == result)
    {
        connectAs = useModuleId ? "module" : "device";
        connectTo = useGatewayHost ? "Edge gateway" : "IoT Hub";
        OsConfigLogInfo(GetLog(), "RequestConnectionStringFromAis: connected to %s as %s (%d)", connectTo, connectAs, result);
        if (IsFullLoggingEnabled())
        {
            OsConfigLogInfo(GetLog(), "Connection string: %s", connectionString);
        }
    }
    else
    {
        OsConfigLogError(GetLog(), "RequestConnectionStringFromAis failed with %d", result);
    }

    json_value_free(identityResponseJson);
    json_value_free(signResponseJson);
    json_value_free(certificateResponseJson);
    FREE_MEMORY(resourceUri);
    FREE_MEMORY(sharedAccessSignature);
    FREE_MEMORY(identityResponseString);
    FREE_MEMORY(signResponseString);
    STRING_delete(encodedSignature);

    if (AIS_SUCCESS != result)
    {
        FREE_MEMORY(connectionString);
        FREE_MEMORY(*x509Certificate);
        FREE_MEMORY(*x509PrivateKeyHandle);
    }

    return connectionString;
}
