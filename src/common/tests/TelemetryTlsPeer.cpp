// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <algorithm>
#include <arpa/inet.h>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <ctime>
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
typedef struct evp_pkey_st EVP_PKEY;
typedef struct evp_pkey_ctx_st EVP_PKEY_CTX;
typedef struct engine_st ENGINE;
typedef struct evp_md_st EVP_MD;
typedef struct x509_st X509;
typedef struct X509_name_st X509_NAME;
typedef struct asn1_string_st ASN1_INTEGER;
typedef struct asn1_string_st ASN1_TIME;
typedef struct X509_extension_st X509_EXTENSION;
typedef struct lhash_st_CONF_VALUE ConfValues;
typedef struct v3_ext_ctx X509V3_CTX;
typedef struct x509_store_st X509_STORE;
typedef struct x509_store_ctx_st X509_STORE_CTX;
typedef struct X509_VERIFY_PARAM_st X509_VERIFY_PARAM;
typedef struct stack_st_X509 X509Stack;

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
int (*SSL_CTX_use_certificate)(SSL_CTX*, X509*);
int (*SSL_CTX_use_PrivateKey)(SSL_CTX*, EVP_PKEY*);
int (*SSL_CTX_check_private_key)(const SSL_CTX*);
long (*SSL_CTX_ctrl)(SSL_CTX*, int, long, void*);
void (*SSL_CTX_set_security_level)(SSL_CTX*, int);
void (*ERR_print_errors_fp)(FILE*);
void (*ERR_clear_error)(void);

EVP_PKEY_CTX* (*EVP_PKEY_CTX_new_id)(int, ENGINE*);
void (*EVP_PKEY_CTX_free)(EVP_PKEY_CTX*);
int (*EVP_PKEY_keygen_init)(EVP_PKEY_CTX*);
int (*EVP_PKEY_CTX_ctrl)(EVP_PKEY_CTX*, int, int, int, int, void*);
int (*EVP_PKEY_keygen)(EVP_PKEY_CTX*, EVP_PKEY**);
void (*EVP_PKEY_free)(EVP_PKEY*);
const EVP_MD* (*EVP_sha256)(void);
int (*OBJ_txt2nid)(const char*);
X509* (*X509_new)(void);
void (*X509_free)(X509*);
int (*X509_set_version)(X509*, long);
ASN1_INTEGER* (*X509_get_serialNumber)(X509*);
int (*ASN1_INTEGER_set)(ASN1_INTEGER*, long);
ASN1_TIME* (*ASN1_TIME_new)(void);
void (*ASN1_TIME_free)(ASN1_TIME*);
ASN1_TIME* (*ASN1_TIME_set)(ASN1_TIME*, time_t);
int (*SetNotBefore)(X509*, const ASN1_TIME*);
int (*SetNotAfter)(X509*, const ASN1_TIME*);
X509_NAME* (*X509_get_subject_name)(const X509*);
int (*X509_NAME_add_entry_by_txt)(X509_NAME*, const char*, int, const unsigned char*, int, int, int);
int (*X509_set_issuer_name)(X509*, X509_NAME*);
int (*X509_set_pubkey)(X509*, EVP_PKEY*);
X509_EXTENSION* (*X509V3_EXT_conf_nid)(ConfValues*, X509V3_CTX*, int, char*);
int (*X509_add_ext)(X509*, X509_EXTENSION*, int);
void (*X509_EXTENSION_free)(X509_EXTENSION*);
int (*X509_sign)(X509*, EVP_PKEY*, const EVP_MD*);
int (*PEM_write_X509)(FILE*, X509*);
X509_STORE* (*X509_STORE_new)(void);
void (*X509_STORE_free)(X509_STORE*);
int (*X509_STORE_add_cert)(X509_STORE*, X509*);
X509_STORE_CTX* (*X509_STORE_CTX_new)(void);
void (*X509_STORE_CTX_free)(X509_STORE_CTX*);
int (*X509_STORE_CTX_init)(X509_STORE_CTX*, X509_STORE*, X509*, X509Stack*);
int (*X509_STORE_CTX_set_purpose)(X509_STORE_CTX*, int);
X509_VERIFY_PARAM* (*X509_STORE_CTX_get0_param)(X509_STORE_CTX*);
int (*X509_VERIFY_PARAM_set1_host)(X509_VERIFY_PARAM*, const char*, size_t);
int (*X509_verify_cert)(X509_STORE_CTX*);
int (*X509_STORE_CTX_get_error)(const X509_STORE_CTX*);
int (*X509_STORE_CTX_get_error_depth)(const X509_STORE_CTX*);

const std::string collector = "mobile.events.data.microsoft.com";

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
    for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); ++i)
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
    LOAD_SERVER_API(SSL_CTX_use_certificate);
    LOAD_SERVER_API(SSL_CTX_use_PrivateKey);
    LOAD_SERVER_API(SSL_CTX_check_private_key);
    LOAD_SERVER_API(SSL_CTX_ctrl);
    LOAD_SERVER_API(ERR_print_errors_fp);
    LOAD_SERVER_API(ERR_clear_error);
    LOAD_SERVER_API(EVP_PKEY_CTX_new_id);
    LOAD_SERVER_API(EVP_PKEY_CTX_free);
    LOAD_SERVER_API(EVP_PKEY_keygen_init);
    LOAD_SERVER_API(EVP_PKEY_CTX_ctrl);
    LOAD_SERVER_API(EVP_PKEY_keygen);
    LOAD_SERVER_API(EVP_PKEY_free);
    LOAD_SERVER_API(EVP_sha256);
    LOAD_SERVER_API(OBJ_txt2nid);
    LOAD_SERVER_API(X509_new);
    LOAD_SERVER_API(X509_free);
    LOAD_SERVER_API(X509_set_version);
    LOAD_SERVER_API(X509_get_serialNumber);
    LOAD_SERVER_API(ASN1_INTEGER_set);
    LOAD_SERVER_API(ASN1_TIME_new);
    LOAD_SERVER_API(ASN1_TIME_free);
    LOAD_SERVER_API(ASN1_TIME_set);
    LOAD_SERVER_API(X509_get_subject_name);
    LOAD_SERVER_API(X509_NAME_add_entry_by_txt);
    LOAD_SERVER_API(X509_set_issuer_name);
    LOAD_SERVER_API(X509_set_pubkey);
    LOAD_SERVER_API(X509V3_EXT_conf_nid);
    LOAD_SERVER_API(X509_add_ext);
    LOAD_SERVER_API(X509_EXTENSION_free);
    LOAD_SERVER_API(X509_sign);
    LOAD_SERVER_API(PEM_write_X509);
    LOAD_SERVER_API(X509_STORE_new);
    LOAD_SERVER_API(X509_STORE_free);
    LOAD_SERVER_API(X509_STORE_add_cert);
    LOAD_SERVER_API(X509_STORE_CTX_new);
    LOAD_SERVER_API(X509_STORE_CTX_free);
    LOAD_SERVER_API(X509_STORE_CTX_init);
    LOAD_SERVER_API(X509_STORE_CTX_set_purpose);
    LOAD_SERVER_API(X509_STORE_CTX_get0_param);
    LOAD_SERVER_API(X509_VERIFY_PARAM_set1_host);
    LOAD_SERVER_API(X509_verify_cert);
    LOAD_SERVER_API(X509_STORE_CTX_get_error);
    LOAD_SERVER_API(X509_STORE_CTX_get_error_depth);
#undef LOAD_SERVER_API
    // The legacy setters became set1 APIs when X509 became opaque in 1.1.
    Load(library, SetNotBefore, version < 0x10100000UL ? "X509_set_notBefore" : "X509_set1_notBefore");
    Load(library, SetNotAfter, version < 0x10100000UL ? "X509_set_notAfter" : "X509_set1_notAfter");
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

using Key = std::unique_ptr<EVP_PKEY, decltype(EVP_PKEY_free)>;
using Certificate = std::unique_ptr<X509, decltype(X509_free)>;

Key GenerateKey(bool ecdsa)
{
    const int algorithm = OBJ_txt2nid(ecdsa ? "id-ecPublicKey" : "rsaEncryption");
    const int parameter = ecdsa ? OBJ_txt2nid("prime256v1") : 2048;
    Require(algorithm != 0 && parameter != 0, "Unknown test key algorithm/curve");
    std::unique_ptr<EVP_PKEY_CTX, decltype(EVP_PKEY_CTX_free)> context(
        EVP_PKEY_CTX_new_id(algorithm, nullptr), EVP_PKEY_CTX_free);
    Require(context != nullptr, "Key generation context allocation failed");
    Require(1 == EVP_PKEY_keygen_init(context.get()), "Key generation initialization failed");
    // Public controls shared by 1.0.2/1.1/3: EC_PARAMGEN_CURVE_NID and RSA_KEYGEN_BITS.
    Require(1 == EVP_PKEY_CTX_ctrl(context.get(), -1, -1, ecdsa ? 0x1001 : 0x1003,
        parameter, nullptr), "Key generation parameters failed");
    if (ecdsa)
    {
        // EC_PARAM_ENC = OPENSSL_EC_NAMED_CURVE. Legacy explicit encoding can
        // decode to a different EC_METHOD and fail comparison with the key.
        Require(1 == EVP_PKEY_CTX_ctrl(context.get(), -1, -1, 0x1002, 1, nullptr),
            "Setting named-curve encoding failed");
    }
    EVP_PKEY* generated = nullptr;
    const int status = EVP_PKEY_keygen(context.get(), &generated);
    Key key(generated, EVP_PKEY_free);
    Require(status == 1 && key != nullptr, "Test key generation failed");
    return key;
}

void Validity(X509* certificate, bool expired)
{
    const time_t now = time(nullptr);
    Require(now != static_cast<time_t>(-1), "Certificate clock lookup failed");
    std::unique_ptr<ASN1_TIME, decltype(ASN1_TIME_free)> value(ASN1_TIME_new(), ASN1_TIME_free);
    Require(value != nullptr, "Certificate time allocation failed");
    Require(ASN1_TIME_set(value.get(), now - (expired ? 172800 : 3600)) != nullptr &&
        1 == SetNotBefore(certificate, value.get()), "Setting certificate start time failed");
    Require(ASN1_TIME_set(value.get(), now + (expired ? -86400 : 86400)) != nullptr &&
        1 == SetNotAfter(certificate, value.get()), "Setting certificate expiration failed");
}

void Extension(X509* certificate, const char* name, const std::string& value)
{
    const int nid = OBJ_txt2nid(name);
    Require(nid != 0, "Unknown certificate extension");
    std::string text = value;
    std::unique_ptr<X509_EXTENSION, decltype(X509_EXTENSION_free)> extension(
        X509V3_EXT_conf_nid(nullptr, nullptr, nid, &text[0]), X509_EXTENSION_free);
    Require(extension != nullptr, "Certificate extension creation failed");
    Require(1 == X509_add_ext(certificate, extension.get(), -1), "Adding certificate extension failed");
}

Certificate GenerateCertificate(EVP_PKEY* key, const std::string& name, const std::string& san,
    X509* issuer = nullptr, EVP_PKEY* issuerKey = nullptr)
{
    Certificate certificate(X509_new(), X509_free);
    Require(certificate != nullptr, "Certificate allocation failed");
    Require(1 == X509_set_version(certificate.get(), 2), "Setting X509v3 version failed");
    ASN1_INTEGER* serial = X509_get_serialNumber(certificate.get());
    Require(serial != nullptr && 1 == ASN1_INTEGER_set(serial, issuer ? 2 : 1), "Setting serial failed");
    Validity(certificate.get(), false);
    X509_NAME* subject = X509_get_subject_name(certificate.get());
    // V_ASN1_UTF8STRING; all fixture common names are plain ASCII.
    Require(subject != nullptr && 1 == X509_NAME_add_entry_by_txt(subject, "CN", 12,
        reinterpret_cast<const unsigned char*>(name.c_str()), -1, -1, 0), "Setting common name failed");
    Require(1 == X509_set_issuer_name(certificate.get(), issuer ? X509_get_subject_name(issuer) : subject),
        "Setting issuer failed");
    Require(1 == X509_set_pubkey(certificate.get(), key), "Setting public key failed");
    Extension(certificate.get(), "basicConstraints", issuer ? "critical,CA:FALSE" : "critical,CA:TRUE");
    Extension(certificate.get(), "keyUsage", issuer ? "critical,digitalSignature,keyEncipherment" :
        "critical,digitalSignature,keyEncipherment,keyCertSign");
    Extension(certificate.get(), "extendedKeyUsage", "serverAuth");
    Extension(certificate.get(), "subjectAltName", san);
    Require(X509_sign(certificate.get(), issuerKey ? issuerKey : key, EVP_sha256()) > 0,
        "Signing certificate failed");
    return certificate;
}

void VerifyExpiryFixture(X509* certificate, X509* root, const std::string& name, bool expired)
{
    std::unique_ptr<X509_STORE, decltype(X509_STORE_free)> store(X509_STORE_new(), X509_STORE_free);
    Require(store != nullptr, "Certificate store allocation failed");
    Require(1 == X509_STORE_add_cert(store.get(), root), "Adding test trust anchor failed");
    std::unique_ptr<X509_STORE_CTX, decltype(X509_STORE_CTX_free)> context(
        X509_STORE_CTX_new(), X509_STORE_CTX_free);
    Require(context != nullptr, "Certificate verification context allocation failed");
    Require(1 == X509_STORE_CTX_init(context.get(), store.get(), certificate, nullptr),
        "Certificate verification initialization failed");
    // X509_PURPOSE_SSL_SERVER; verify identity as well as chain, purpose and dates.
    Require(1 == X509_STORE_CTX_set_purpose(context.get(), 2), "Setting verification purpose failed");
    X509_VERIFY_PARAM* parameters = X509_STORE_CTX_get0_param(context.get());
    Require(parameters != nullptr && 1 == X509_VERIFY_PARAM_set1_host(parameters, name.c_str(), name.size()),
        "Setting verification hostname failed");
    const int status = X509_verify_cert(context.get());
    const int error = X509_STORE_CTX_get_error(context.get());
    const int depth = X509_STORE_CTX_get_error_depth(context.get());
    if (expired ? status != 0 || error != 10 || depth != 0 : status != 1)
    {
        fprintf(stderr, "TLS fixture verification: expired=%d status=%d error=%d depth=%d\n",
            expired, status, error, depth);
        throw std::runtime_error("Certificate fixture sanity check failed");
    }
    // Expected X509_V_ERR_CERT_HAS_EXPIRED is not a later handshake diagnostic.
    ERR_clear_error();
}

// Real keys and signatures, not mocks. Only the public trust certificate is
// written to the parent's private directory; private keys remain in memory.
void Certificates(SSL_CTX* context, const std::string& directory, const std::string& mode)
{
    std::string name = mode.compare(0, 5, "aria-") == 0 ? collector : "localhost";
    if (mode == "wrong-name" || mode == "aria-wrong-name") { name = "wrong.example"; }
    if (mode == "numeric-dns") { name = "127.0.0.1"; }
    if (mode == "partial-wildcard") { name = "local.example.test"; }
    std::string san = "DNS:" + (mode == "partial-wildcard" ? std::string("loc*.example.test") : name);
    if (mode != "numeric-dns") { san += ",IP:127.0.0.1"; }

    Key key = GenerateKey(mode == "ecdsa");
    Key rootKey(nullptr, EVP_PKEY_free);
    Certificate root(nullptr, X509_free);
    if (mode == "expired" || mode == "untrusted")
    {
        rootKey = GenerateKey(false);
        root = GenerateCertificate(rootKey.get(),
            mode == "expired" ? "Expiry Test Root" : "Other Test Root", san);
    }
    Certificate server = GenerateCertificate(key.get(), name, san,
        mode == "expired" ? root.get() : nullptr, mode == "expired" ? rootKey.get() : nullptr);
    if (mode == "expired")
    {
        // The same key, issuer, identity and extensions must first verify when
        // valid, then fail solely for leaf expiration after changing the dates.
        VerifyExpiryFixture(server.get(), root.get(), name, false);
        Validity(server.get(), true);
        Require(X509_sign(server.get(), rootKey.get(), EVP_sha256()) > 0, "Signing expired certificate failed");
        VerifyExpiryFixture(server.get(), root.get(), name, true);
    }

    Require(1 == SSL_CTX_use_certificate(context, server.get()), "Loading test certificate failed");
    Require(1 == SSL_CTX_use_PrivateKey(context, key.get()), "Loading test key failed");
    Require(1 == SSL_CTX_check_private_key(context), "Test key does not match certificate");
    const auto closeFile = [](FILE* file) { fclose(file); };
    std::unique_ptr<FILE, decltype(closeFile)> output(fopen((directory + "/root.pem").c_str(), "w"), closeFile);
    Require(output != nullptr, "Opening test trust certificate failed");
    Require(1 == PEM_write_X509(output.get(), root ? root.get() : server.get()), "Writing trust certificate failed");
    Require(0 == fclose(output.release()), "Closing test trust certificate failed");
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
        Require(argc == 3, "Usage: telemetrytlspeer <temporary-directory> <mode>");
        const std::string mode = argv[2];
        Require(SIG_ERR != signal(SIGPIPE, SIG_IGN), "Cannot ignore SIGPIPE");
        Require(SIG_ERR != signal(SIGALRM, SIG_DFL), "Cannot restore SIGALRM");
        alarm(mode == "aria-live" ? 1200 : 40);
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
        Certificates(context.get(), argv[1], mode);
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
