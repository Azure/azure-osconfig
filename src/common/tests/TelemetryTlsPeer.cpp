// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <CommonUtils.h>
#include <algorithm>
#include <arpa/inet.h>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <dlfcn.h>
#include <initializer_list>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

namespace
{
typedef struct ssl_st SSL;
typedef struct ssl_ctx_st SSL_CTX;
typedef struct ssl_method_st SSL_METHOD;
typedef struct ossl_init_settings_st InitSettings;

// These are real server APIs, not mocks. Dynamic loading keeps the test peer
// independent of development headers absent from some pinned CI images.
SSL_CTX* (*SSL_CTX_new)(const SSL_METHOD*);
void (*SSL_CTX_free)(SSL_CTX*);
SSL* (*SSL_new)(SSL_CTX*);
void (*SSL_free)(SSL*);
int (*SSL_set_fd)(SSL*, int);
int (*SSL_accept)(SSL*);
int (*SSL_read)(SSL*, void*, int);
int (*SSL_write)(SSL*, const void*, int);
int (*SSL_shutdown)(SSL*);
const char* (*SSL_get_servername)(const SSL*, int);
void (*SSL_get0_alpn_selected)(const SSL*, const unsigned char**, unsigned int*);
void (*SSL_CTX_set_alpn_select_cb)(SSL_CTX*, int (*)(SSL*, const unsigned char**,
    unsigned char*, const unsigned char*, unsigned int, void*), void*);
int (*SSL_CTX_set_cipher_list)(SSL_CTX*, const char*);
int (*SSL_CTX_use_certificate_chain_file)(SSL_CTX*, const char*);
int (*SSL_CTX_use_PrivateKey_file)(SSL_CTX*, const char*, int);
int (*SSL_CTX_check_private_key)(const SSL_CTX*);
long (*SSL_CTX_ctrl)(SSL_CTX*, int, long, void*);
void (*SSL_CTX_set_security_level)(SSL_CTX*, int);
void (*ERR_print_errors_fp)(FILE*);

const std::string collector = "mobile.events.data.microsoft.com";

// Test-only certificate setup. Arguments are shell-quoted by the caller; the
// parent owns the private temporary directory and removes every generated file.
const char certificateScript[] = R"BASH(
set -euo pipefail
umask 077
openssl=$1
cd -- "$2"
mode=$3
name=localhost
case "$mode" in
    aria-*) name=mobile.events.data.microsoft.com ;;
esac
case "$mode" in
    wrong-name|aria-wrong-name) name=wrong.example ;;
    numeric-dns) name=127.0.0.1 ;;
    partial-wildcard) name=local.example.test ;;
esac
san="DNS:$name"
if [ "$mode" = partial-wildcard ]; then san='DNS:loc*.example.test'; fi
if [ "$mode" != numeric-dns ]; then san="$san,IP:127.0.0.1"; fi
cat > openssl.cnf <<EOF
[req]
distinguished_name=dn
x509_extensions=extensions
[dn]
[extensions]
basicConstraints=critical,CA:TRUE
keyUsage=critical,digitalSignature,keyEncipherment,keyCertSign
extendedKeyUsage=serverAuth
subjectAltName=$san
EOF
certificate() {
    local file=$1 subject=$2
    local key=(-newkey rsa:2048)
    if [ "$file" = server ] && [ "$mode" = ecdsa ]; then
        key=(-newkey ec -pkeyopt ec_paramgen_curve:P-256)
    fi
    "$openssl" req -new -x509 "${key[@]}" -nodes -days 1 -sha256 \
        -subj "/CN=$subject" -config openssl.cnf \
        -keyout "$file-key.pem" -out "$file.pem"
}
certificate server "$name"
if [ "$mode" = expired ]; then
    certificate root 'Expiry Test Root'
    : > index
    printf '01\n' > serial
    cat > ca.cnf <<EOF
[ca]
default_ca=issuer
[issuer]
database=index
serial=serial
new_certs_dir=.
default_md=sha256
default_days=1
policy=policy
unique_subject=no
[policy]
commonName=supplied
[server]
basicConstraints=critical,CA:FALSE
keyUsage=critical,digitalSignature,keyEncipherment
extendedKeyUsage=serverAuth
subjectAltName=$san
EOF
    "$openssl" req -new -key server-key.pem -subj "/CN=$name" \
        -config openssl.cnf -out server.csr
    sign=(ca -batch -notext -config ca.cnf -cert root.pem -keyfile root-key.pem
          -in server.csr -extensions server)
    verify=(verify -CAfile root.pem -purpose sslserver -verify_hostname "$name")
    # First verify the same CSR, issuer and extensions without expiry.
    "$openssl" "${sign[@]}" -out valid.pem
    "$openssl" "${verify[@]}" valid.pem
    "$openssl" "${sign[@]}" -out server.pem \
        -startdate 20000101000000Z -enddate 20000102000000Z
    if result=$("$openssl" "${verify[@]}" server.pem 2>&1); then
        echo 'Expired fixture unexpectedly verified' >&2
        exit 1
    fi
    if [[ ! "$result" =~ error\ 10\ at\ 0\ depth\ lookup: ]]; then
        printf 'Unexpected certificate verification failure: %s\n' "$result" >&2
        exit 1
    fi
elif [ "$mode" = untrusted ]; then
    certificate root 'Other Test Root'
else
    cp -- server.pem root.pem
fi
)BASH";

void Require(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

template<typename Function>
void Load(void* library, Function& function, const char* name)
{
    void* symbol = dlsym(library, name);
    if (!symbol)
    {
        throw std::runtime_error(std::string("Test OpenSSL server lacks ") + name);
    }
    static_assert(sizeof(symbol) == sizeof(function), "Unsupported POSIX function-pointer ABI");
    memcpy(&function, &symbol, sizeof(symbol));
}

const SSL_METHOD* LoadOpenSsl(const std::string& mode)
{
    const char* names[] = {"libssl.so.3", "libssl.so.1.1", "libssl.so.1.0.2", "libssl.so.10", "libssl.so.1.0.0"};
    void* library = nullptr;
    unsigned long version = 0;
    for (size_t i = 0; i < ARRAY_SIZE(names); ++i)
    {
        library = dlopen(names[i], RTLD_NOW | RTLD_LOCAL);
        if (!library)
        {
            continue;
        }
        unsigned long (*getVersion)(void) = nullptr;
        Load(library, getVersion, i < 2 ? "OpenSSL_version_num" : "SSLeay");
        version = getVersion();
        Require(i == 0 ? version >= 0x30000000UL && version < 0x40000000UL :
            i == 1 ? version >= 0x10100000UL && version < 0x10200000UL :
            version >= 0x10002000UL && version < 0x10003000UL, "Unsupported test OpenSSL server version");
        fprintf(stderr, "Telemetry TLS test peer: %s version 0x%lx\n", names[i], version);
        break;
    }
    Require(library != nullptr, "No system OpenSSL runtime for the test server");
#define LOAD_SERVER_API(name) Load(library, name, #name)
    LOAD_SERVER_API(SSL_CTX_new);
    LOAD_SERVER_API(SSL_CTX_free);
    LOAD_SERVER_API(SSL_new);
    LOAD_SERVER_API(SSL_free);
    LOAD_SERVER_API(SSL_set_fd);
    LOAD_SERVER_API(SSL_accept);
    LOAD_SERVER_API(SSL_read);
    LOAD_SERVER_API(SSL_write);
    LOAD_SERVER_API(SSL_shutdown);
    LOAD_SERVER_API(SSL_get_servername);
    LOAD_SERVER_API(SSL_get0_alpn_selected);
    LOAD_SERVER_API(SSL_CTX_set_alpn_select_cb);
    LOAD_SERVER_API(SSL_CTX_set_cipher_list);
    LOAD_SERVER_API(SSL_CTX_use_certificate_chain_file);
    LOAD_SERVER_API(SSL_CTX_use_PrivateKey_file);
    LOAD_SERVER_API(SSL_CTX_check_private_key);
    LOAD_SERVER_API(SSL_CTX_ctrl);
    LOAD_SERVER_API(ERR_print_errors_fp);
#undef LOAD_SERVER_API
    const SSL_METHOD* (*method)(void) = nullptr;
    if (version < 0x10100000UL)
    {
        int (*initialize)(void) = nullptr;
        void (*loadErrors)(void) = nullptr;
        Load(library, initialize, "SSL_library_init");
        Load(library, loadErrors, "SSL_load_error_strings");
        loadErrors();
        Require(1 == initialize(), "Legacy OpenSSL initialization failed");
        Load(library, method, mode == "tls10" ? "TLSv1_server_method" :
            mode == "tls11" ? "TLSv1_1_server_method" : "TLSv1_2_server_method");
    }
    else
    {
        int (*initialize)(uint64_t, const InitSettings*) = nullptr;
        Load(library, initialize, "OPENSSL_init_ssl");
        Load(library, SSL_CTX_set_security_level, "SSL_CTX_set_security_level");
        Require(1 == initialize(0, nullptr), "OpenSSL initialization failed");
        Load(library, method, "TLS_server_method");
    }
    // Keep the library resident until this dedicated test process exits.
    return method();
}

std::string Quote(const std::string& value)
{
    std::string result = "'";
    for (char c : value)
    {
        result += c == '\'' ? "'\\''" : std::string(1, c);
    }
    return result + "'";
}

void Certificates(const char* openssl, const char* directory, const char* mode)
{
    const std::string command = Quote(TELEMETRY_TLS_BASH) + " -c " + Quote(certificateScript) +
        " fixture " + Quote(openssl) + " " + Quote(directory) + " " + Quote(mode);
    char* output = nullptr;
    const int status = ExecuteCommand(nullptr, command.c_str(), false, false, 16384, 20,
        &output, nullptr, nullptr);
    if (status)
    {
        fprintf(stderr, "TLS peer certificate setup failed (%d): %s\n", status, output ? output : "");
    }
    free(output);
    Require(0 == status, "Certificate setup failed");
}

class Socket
{
public:
    explicit Socket(int descriptor) : value(descriptor)
    {
        Require(value >= 0, "Socket creation/accept failed");
    }
    ~Socket() { close(value); }
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
    int value;
};

using Context = std::unique_ptr<SSL_CTX, decltype(SSL_CTX_free)>;
using Session = std::unique_ptr<SSL, decltype(SSL_free)>;

// Real system OpenSSL server, isolated in this test process. No production TLS
// functions or mocks implement the peer's cryptography or certificate handling.
class Connection
{
public:
    explicit Connection(int listener) : socket(accept(listener, nullptr, nullptr)), tls(nullptr, SSL_free)
    {
        const timeval timeout = {10, 0};
        Require(0 == setsockopt(socket.value, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)),
            "Receive timeout setup failed");
        Require(0 == setsockopt(socket.value, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)),
            "Send timeout setup failed");
    }

    bool Handshake(SSL_CTX* context)
    {
        tls.reset(SSL_new(context));
        Require(nullptr != tls, "SSL_new failed");
        Require(1 == SSL_set_fd(tls.get(), socket.value), "SSL_set_fd failed");
        return 1 == SSL_accept(tls.get());
    }

    int Read(char* bytes, int size)
    {
        int count = 0;
        do
        {
            errno = 0;
            count = tls ? SSL_read(tls.get(), bytes, size) :
                static_cast<int>(recv(socket.value, bytes, static_cast<size_t>(size), 0));
        } while (count < 0 && errno == EINTR);
        return count;
    }

    std::string Receive(size_t size)
    {
        std::string result;
        char bytes[16384];
        while (result.size() < size)
        {
            const int count = Read(bytes, static_cast<int>(std::min(sizeof(bytes), size - result.size())));
            Require(count > 0, "Client disconnected/timed out before completing its request");
            result.append(bytes, static_cast<size_t>(count));
        }
        return result;
    }

    void Send(const std::string& bytes)
    {
        size_t offset = 0;
        while (offset < bytes.size())
        {
            const size_t size = std::min(bytes.size() - offset, static_cast<size_t>(16384));
            errno = 0;
            const int count = tls ? SSL_write(tls.get(), bytes.data() + offset, static_cast<int>(size)) :
                static_cast<int>(send(socket.value, bytes.data() + offset, size, 0));
            if (count < 0 && errno == EINTR)
            {
                continue;
            }
            Require(count > 0, "Peer response write failed");
            offset += static_cast<size_t>(count);
        }
    }

    void ExpectDisconnect()
    {
        char byte = 0;
        const int count = Read(&byte, 1);
        Require(count <= 0, "Unexpected request after suppression/rejection");
        Require(errno != EAGAIN && errno != EWOULDBLOCK, "Client did not disconnect before timeout");
    }

    Socket socket;
    Session tls;
};

int SelectAlpn(SSL*, const unsigned char** out, unsigned char* size,
    const unsigned char* input, unsigned int length, void*)
{
    for (unsigned int offset = 0; offset < length;)
    {
        const unsigned int count = input[offset++];
        if (count > length - offset)
        {
            return 2;
        }
        if (count == 8 && memcmp(input + offset, "http/1.1", 8) == 0)
        {
            *out = input + offset;
            *size = 8;
            return 0;
        }
        offset += count;
    }
    return 2;
}

void CheckIdentity(Connection& connection, const std::string& expected)
{
    const char* name = SSL_get_servername(connection.tls.get(), 0);
    Require(expected.empty() ? name == nullptr : name != nullptr && expected == name, "Unexpected TLS SNI");
    const unsigned char* protocol = nullptr;
    unsigned int length = 0;
    SSL_get0_alpn_selected(connection.tls.get(), &protocol, &length);
    Require(length == 8 && protocol != nullptr && memcmp(protocol, "http/1.1", 8) == 0,
        "Client did not negotiate HTTP/1.1");
}

struct Headers
{
    std::string first;
    std::map<std::string, std::string> fields;
};

Headers ReadHeaders(Connection& connection)
{
    std::string wire;
    while (wire.size() < 4 || wire.compare(wire.size() - 4, 4, "\r\n\r\n") != 0)
    {
        wire += connection.Receive(1);
        Require(wire.size() <= 8192, "Oversized request headers");
    }
    const size_t firstEnd = wire.find("\r\n");
    Headers headers;
    headers.first = wire.substr(0, firstEnd);
    size_t start = firstEnd + 2;
    while (start < wire.size() - 2)
    {
        const size_t end = wire.find("\r\n", start);
        const size_t colon = wire.find(':', start);
        Require(end != std::string::npos && colon < end && colon > start, "Malformed request header");
        std::string name = wire.substr(start, colon - start);
        for (char& c : name)
        {
            if (c >= 'A' && c <= 'Z')
            {
                c = static_cast<char>(c + ('a' - 'A'));
            }
        }
        const size_t begin = wire.find_first_not_of(" \t", colon + 1);
        const size_t last = wire.find_last_not_of(" \t", end - 1);
        const std::string value = begin < end ? wire.substr(begin, last - begin + 1) : "";
        Require(headers.fields.emplace(name, value).second, "Duplicate request header");
        start = end + 2;
    }
    return headers;
}

bool Starts(const std::string& text, const char* prefix)
{
    return text.compare(0, strlen(prefix), prefix) == 0;
}

bool OneOf(const std::string& value, std::initializer_list<const char*> values)
{
    for (const char* item : values)
    {
        if (value == item)
        {
            return true;
        }
    }
    return false;
}

std::set<std::string> Correlations(const std::string& body)
{
    std::set<std::string> result;
    for (size_t i = 0; i + 36 <= body.size(); ++i)
    {
        bool valid = true;
        for (size_t j = 0; j < 36 && valid; ++j)
        {
            const char c = body[i + j];
            valid = (j == 8 || j == 13 || j == 18 || j == 23) ? c == '-' :
                ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'));
        }
        if (valid && body[i + 14] == '4' && strchr("89ab", body[i + 19]) != nullptr)
        {
            result.insert(body.substr(i, 36));
        }
    }
    return result;
}

void Aria(int listener, SSL_CTX* context, const std::string& mode)
{
    const int connections = OneOf(mode, {"aria-close", "aria-recover", "aria-worker-recover"}) ? 2 : 1;
    std::set<std::string> bodies;
    std::string correlation;
    for (int index = 0; index < connections; ++index)
    {
        Connection connection(listener);
        Headers proxy = ReadHeaders(connection);
        Require(proxy.first == "CONNECT " + collector + ":443 HTTP/1.1" &&
            proxy.fields["host"] == collector + ":443" && proxy.fields.count("apikey") == 0 &&
            proxy.fields.count("content-length") == 0, "Invalid CONNECT or exposed collector credentials");
        Require(mode == "aria-auth" ? proxy.fields["proxy-authorization"] == "Basic dXNlcjpwQHNz" :
            proxy.fields.count("proxy-authorization") == 0, "Unexpected proxy authorization");
        if (mode == "aria-proxy-timeout")
        {
            sleep(3);
            return;
        }
        const std::map<std::string, std::string> replies = {
            {"aria-407", "HTTP/1.1 407 Proxy Authentication Required\r\n\r\n"},
            {"aria-redirect", "HTTP/1.1 302 Redirect\r\nLocation: https://untrusted.invalid/\r\n\r\n"},
            {"aria-bad-connect", "HTTP/1.1 200 OK\r\n Invalid: header\r\n\r\n"},
            {"aria-long-connect", "HTTP/1.1 200 OK\r\nX: " + std::string(2048, 'x') + "\r\n\r\n"},
            {"aria-truncated-connect", "HTTP/1.1 200 OK\r\n"},
            {"aria-upgrade", "HTTP/1.1 101 Switching Protocols\r\n\r\n"},
            {"aria-many-interim", "HTTP/1.1 100 Continue\r\n\r\nHTTP/1.1 100 Continue\r\n\r\n"
                "HTTP/1.1 100 Continue\r\n\r\nHTTP/1.1 100 Continue\r\n\r\nHTTP/1.1 100 Continue\r\n\r\n"}
        };
        const auto reply = replies.find(mode);
        if (reply != replies.end())
        {
            connection.Send(reply->second);
            return;
        }
        const std::string tunnel = "HTTP/1.1 100 Continue\r\n\r\nHTTP/1.1 200 Connection Established\r\n"
            "Content-Length: 999999\r\nTransfer-Encoding: chunked\r\n\r\n";
        for (char c : tunnel)
        {
            connection.Send(std::string(1, c));
        }
        if (!connection.Handshake(context))
        {
            Require(mode == "aria-wrong-name", "Collector TLS handshake failed");
            return;
        }
        Require(mode != "aria-wrong-name", "Client accepted wrong collector identity");
        CheckIdentity(connection, collector);
        const int count = mode == "aria-live" ? 10000 :
            OneOf(mode, {"aria-reuse", "aria-auth", "aria-worker", "aria-producer"}) ? 2 : 1;
        for (int sequence = 0; sequence < count; ++sequence)
        {
            Headers request = ReadHeaders(connection);
            Require(request.first == "POST /OneCollector/1.0/ HTTP/1.1" &&
                request.fields["host"] == collector && request.fields["apikey"] == "fixture-token" &&
                request.fields["client-id"] == "NO_AUTH" && request.fields["accept-encoding"] == "identity" &&
                request.fields["content-type"] == "application/bond-compact-binary" &&
                request.fields.count("proxy-authorization") == 0, "Invalid collector headers or leaked credentials");
            const std::string sizeText = request.fields["content-length"];
            Require(!sizeText.empty() && sizeText.find_first_not_of("0123456789") == std::string::npos,
                "Invalid content length");
            char* end = nullptr;
            errno = 0;
            const unsigned long size = strtoul(sizeText.c_str(), &end, 10);
            Require(errno == 0 && *end == '\0' && size > 0 && size <= 16384, "Invalid collector request length");
            const std::string body = connection.Receive(static_cast<size_t>(size));
            if (Starts(mode, "aria-live"))
            {
                for (const char* required : {"StatusTrace", "*** Distilled 1DS SDK test No. 66 ***", "731001",
                    "CorrelationId", "ResultString", "o:fixture"})
                {
                    Require(body.find(required) != std::string::npos, "Synthetic event missing required field");
                }
                const auto identifiers = Correlations(body);
                Require(identifiers.size() == 1, "Missing or inconsistent run correlation");
                Require(correlation.empty() || correlation == *identifiers.begin(), "Run correlation changed");
                correlation = *identifiers.begin();
                Require(bodies.insert(body).second, "Repeated synthetic event");
            }
            else if (Starts(mode, "aria-worker") || mode == "aria-producer")
            {
                const char* event = mode == "aria-producer" && sequence == 1 ? "BaselineRun" : "CrashDetected";
                for (const char* required : {event, "OSConfig-C/0.1", "o:fixture", "CorrelationId", "worker-fixture"})
                {
                    Require(body.find(required) != std::string::npos, "Worker event missing schema or marker");
                }
                Require(bodies.insert(body).second, "Worker replayed an event");
            }
            else
            {
                Require(body == "encoded-fixture", "Incorrect or replayed event body");
            }
            // Deliberately omit close_notify in these transport failure fixtures.
            if (OneOf(mode, {"aria-drop", "aria-live-drop"}))
            {
                return;
            }
            if (OneOf(mode, {"aria-recover", "aria-worker-recover"}) && index == 0)
            {
                break;
            }
            if (OneOf(mode, {"aria-read-timeout", "aria-worker-timeout"}))
            {
                sleep(3);
                return;
            }
            std::string status = "200 OK";
            std::string extra;
            std::string response = mode == "aria-live" ? "{\"acc\":1}" : "{\"acc\":1,\"rej\":0}";
            if (OneOf(mode, {"aria-reject", "aria-live-reject", "aria-worker-reject"}))
            {
                response = "{\"acc\":0,\"rej\":1}";
            }
            else if (OneOf(mode, {"aria-empty", "aria-live-empty", "aria-worker-empty"}))
            {
                response.clear();
            }
            else if (OneOf(mode, {"aria-throttle", "aria-live-throttle", "aria-worker-throttle"}))
            {
                status = "429 Too Many Requests";
                extra = "Retry-After: 60\r\n";
                response.clear();
            }
            else if (OneOf(mode, {"aria-kill", "aria-worker-kill", "aria-worker-bad-kill", "aria-bad-kill"}))
            {
                extra = "kill-tokens: fixture-token\r\nkill-duration: 60\r\n";
                if (OneOf(mode, {"aria-worker-bad-kill", "aria-bad-kill"}))
                {
                    response = "{\"acc\":";
                }
            }
            else if (mode == "aria-close")
            {
                extra = "Connection: close\r\n";
            }
            extra += "time-delta-millis: 0\r\n";
            connection.Send("HTTP/1.1 " + status + "\r\nContent-Length: " + std::to_string(response.size()) +
                "\r\n" + extra + "\r\n" + response);
        }
        if (OneOf(mode, {"aria-throttle", "aria-kill", "aria-reject", "aria-empty", "aria-bad-kill",
            "aria-live-throttle", "aria-live-reject", "aria-live-empty", "aria-worker-throttle",
            "aria-worker-kill", "aria-worker-bad-kill", "aria-worker-reject", "aria-worker-empty"}))
        {
            connection.ExpectDisconnect();
        }
    }
}

void Exchange(int listener, SSL_CTX* context, const std::string& mode)
{
    Connection connection(listener);
    if (mode == "silent-write")
    {
        const int size = 4096;
        Require(0 == setsockopt(connection.socket.value, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size)),
            "Socket buffer setup failed");
    }
    if (mode == "silent-handshake")
    {
        sleep(5);
        return;
    }
    if (mode == "reset")
    {
        const linger reset = {1, 0};
        Require(0 == setsockopt(connection.socket.value, SOL_SOCKET, SO_LINGER, &reset, sizeof(reset)),
            "Socket reset setup failed");
        return;
    }
    const bool rejection = OneOf(mode, {"wrong-name", "wrong-ip", "untrusted", "tls10", "tls11",
        "expired", "numeric-dns", "partial-wildcard"});
    if (!connection.Handshake(context))
    {
        Require(rejection, "TLS handshake failed");
        return;
    }
    Require(!rejection, "Client accepted a rejected TLS fixture");
    CheckIdentity(connection, mode == "ip" ? "" : "localhost");
    if (OneOf(mode, {"silent-read", "silent-write"}))
    {
        sleep(5);
        return;
    }
    if (mode == "abrupt")
    {
        connection.Send("x");
        return;
    }
    if (mode == "large-write")
    {
        usleep(200000);
        const std::string bytes = connection.Receive(512 * 1024);
        for (size_t i = 0; i < bytes.size(); ++i)
        {
            Require(static_cast<unsigned char>(bytes[i]) == i % 251, "Corrupted or replayed client write");
        }
        connection.Send("ok");
    }
    else
    {
        for (int i = 0; i < 2; ++i)
        {
            Require(connection.Receive(4) == "ping", "Unexpected request");
            for (char c : std::string("pong"))
            {
                connection.Send(std::string(1, c));
            }
        }
    }
    // Send authenticated EOF without waiting for the client, which frees its
    // session instead of performing a shutdown round trip.
    Require(SSL_shutdown(connection.tls.get()) >= 0, "Sending close_notify failed");
}
}

int main(int argc, char** argv)
{
    try
    {
        Require(argc == 4, "Usage: telemetrytlspeer <openssl> <temporary-directory> <mode>");
        const std::string mode = argv[3];
        Require(SIG_ERR != signal(SIGPIPE, SIG_IGN), "Cannot ignore SIGPIPE");
        Require(SIG_ERR != signal(SIGALRM, SIG_DFL), "Cannot restore SIGALRM");
        alarm(mode == "aria-live" ? 1200 : 40);
        // stdout is exclusively the port handshake with the parent test.
        SetConsoleLoggingEnabled(false);
        Certificates(argv[1], argv[2], argv[3]);
        const SSL_METHOD* method = LoadOpenSsl(mode);
        Context context(SSL_CTX_new(method), SSL_CTX_free);
        Require(nullptr != context, "SSL_CTX_new failed");
        if (SSL_CTX_set_security_level)
        {
            const int version = mode == "tls10" ? 0x0301 : mode == "tls11" ? 0x0302 : 0x0303;
            // Public SSL_CTRL_SET_MIN/MAX_PROTO_VERSION controls.
            Require(1 == SSL_CTX_ctrl(context.get(), 123, version, nullptr) &&
                1 == SSL_CTX_ctrl(context.get(), 124, version, nullptr), "TLS version setup failed");
            if (OneOf(mode, {"tls10", "tls11"}))
            {
                SSL_CTX_set_security_level(context.get(), 0);
            }
        }
        else
        {
            // OpenSSL 1.0.2 SSL_CTRL_SET_ECDH_AUTO.
            Require(1 == SSL_CTX_ctrl(context.get(), 94, 1, nullptr), "ECDH setup failed");
        }
        Require(1 == SSL_CTX_set_cipher_list(context.get(), "DEFAULT"), "Cipher setup failed");
        SSL_CTX_set_alpn_select_cb(context.get(), SelectAlpn, nullptr);
        const std::string directory = argv[2];
        Require(1 == SSL_CTX_use_certificate_chain_file(context.get(), (directory + "/server.pem").c_str()),
            "Loading test certificate failed");
        Require(1 == SSL_CTX_use_PrivateKey_file(context.get(), (directory + "/server-key.pem").c_str(),
            1), "Loading test key failed");
        Require(1 == SSL_CTX_check_private_key(context.get()), "Test key does not match certificate");
        Socket listener(socket(AF_INET, SOCK_STREAM, 0));
        sockaddr_in address = {};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        Require(0 == bind(listener.value, reinterpret_cast<sockaddr*>(&address), sizeof(address)), "Bind failed");
        Require(0 == listen(listener.value, 1), "Listen failed");
        socklen_t length = sizeof(address);
        Require(0 == getsockname(listener.value, reinterpret_cast<sockaddr*>(&address), &length), "Port lookup failed");
        Require(printf("%u\n", static_cast<unsigned int>(ntohs(address.sin_port))) > 0 && fflush(stdout) == 0,
            "Port output failed");
        if (Starts(mode, "aria-"))
        {
            Aria(listener.value, context.get(), mode);
        }
        else
        {
            Exchange(listener.value, context.get(), mode);
        }
        return EXIT_SUCCESS;
    }
    catch (const std::exception& error)
    {
        fprintf(stderr, "Telemetry TLS test peer: %s (errno=%d)\n", error.what(), errno);
        if (ERR_print_errors_fp)
        {
            ERR_print_errors_fp(stderr);
        }
        return EXIT_FAILURE;
    }
}
