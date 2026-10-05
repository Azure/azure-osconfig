// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define _POSIX_C_SOURCE 200809L

#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/utsname.h>

static const char* const g_openSslSymbols[] = {
    "TLS_client_method",
    "SSL_CTX_new",
    "SSL_CTX_free",
    "SSL_CTX_set_verify",
    "SSL_CTX_set_default_verify_paths",
    "SSL_CTX_ctrl",
    "SSL_new",
    "SSL_free",
    "SSL_set_fd",
    "SSL_set1_host",
    "SSL_ctrl",
    "SSL_connect",
    "SSL_get_error",
    "SSL_get_verify_result",
    "SSL_read",
    "SSL_write",
    "SSL_shutdown",
    NULL
};

static const char* const g_nssSymbols[] = {
    "NSS_GetVersion",
    "NSS_InitContext",
    "NSS_ShutdownContext",
    "CERT_GetDefaultCertDB",
    "CERT_VerifyCertName",
    "SECMOD_LoadUserModule",
    "SECMOD_UnloadUserModule",
    "SECMOD_DestroyModule",
    NULL
};

static const char* const g_nssSslSymbols[] = {
    "SSL_ImportFD",
    "SSL_OptionSet",
    "SSL_VersionRangeGetSupported",
    "SSL_VersionRangeSet",
    "SSL_CipherPolicyGet",
    "SSL_CipherPrefGet",
    "SSL_CipherPrefSet",
    "SSL_SetURL",
    "SSL_AuthCertificate",
    "SSL_AuthCertificateHook",
    "SSL_ResetHandshake",
    "SSL_ForceHandshake",
    NULL
};

static const char* const g_nsprSymbols[] = {
    "PR_OpenTCPSocket",
    "PR_Connect",
    "PR_SetSocketOption",
    "PR_Poll",
    "PR_Read",
    "PR_Write",
    "PR_Close",
    "PR_GetError",
    NULL
};

static const char* const g_trustModuleSymbols[] = {
    "C_GetFunctionList",
    NULL
};

static int InspectLibrary(const char* name, const char* const* symbols)
{
    void* library = NULL;
    const char* error = NULL;
    size_t missing = 0;
    size_t count = 0;
    void* address = NULL;

    dlerror();
    library = dlopen(name, RTLD_NOW | RTLD_LOCAL);
    if (NULL == library)
    {
        error = dlerror();
        printf("Library %s: UNAVAILABLE (%s)\n", name,
            (NULL != error) ? error : "loader supplied no diagnostic");
        return 0;
    }

    printf("Library %s: LOADABLE\n", name);
    if (NULL != symbols)
    {
        for (; NULL != symbols[count]; ++count)
        {
            address = NULL;

            dlerror();
            address = dlsym(library, symbols[count]);
            error = dlerror();
            if ((NULL != error) || (NULL == address))
            {
                printf("  Symbol %s: MISSING (%s)\n", symbols[count],
                    (NULL != error) ? error : "null symbol address");
                ++missing;
            }
        }

        printf("  Sampled API surface: %zu/%zu symbols found; not a TLS readiness result\n",
            count - missing, count);
    }

    if (0 != dlclose(library))
    {
        error = dlerror();
        fprintf(stderr, "Cannot close %s: %s\n", name,
            (NULL != error) ? error : "loader supplied no diagnostic");
        return 1;
    }

    return 0;
}

static int InspectTrustPath(const char* path)
{
    struct stat metadata = {0};
    int error = 0;

    if (0 == stat(path, &metadata))
    {
        printf("Trust path %s: PRESENT (%s; contents and trust not inspected)\n",
            path, S_ISREG(metadata.st_mode) ? "file" :
                (S_ISDIR(metadata.st_mode) ? "directory" : "other"));
        return 0;
    }

    error = errno;
    if ((ENOENT == error) || (ENOTDIR == error))
    {
        printf("Trust path %s: ABSENT\n", path);
        return 0;
    }

    fprintf(stderr, "Cannot inspect trust path %s: %s\n", path, strerror(error));
    return 1;
}

int main(int argc, char** argv)
{
    static const char* const trustPaths[] = {
        "/etc/ssl/certs",
        "/etc/ssl/certs/ca-certificates.crt",
        "/etc/ssl/ca-bundle.pem",
        "/etc/pki/tls/certs/ca-bundle.crt",
        "/etc/pki/ca-trust/extracted/pem/tls-ca-bundle.pem",
        "/var/lib/ca-certificates/ca-bundle.pem",
        "/etc/pki/nssdb"
    };
    struct utsname platform = {0};
    size_t i = 0;
    int failed = 0;

    if (1 != argc)
    {
        fprintf(stderr, "Usage: %s (no arguments; offline TLS inventory only)\n",
            (argc > 0) ? argv[0] : "telemetrytlsprobe");
        return EXIT_FAILURE;
    }

    puts("Offline TLS inventory; NOT a provider-selection or secure-connection test.");
    puts("Uses the normal dynamic-loader search path, including environment overrides.");
    puts("Loads/unloads libraries but does not call their TLS or certificate APIs.");
    puts("Library constructors run; execute only on a trusted, controlled image.");
    puts("This process does not establish policy-host coexistence or trust-store usability.");

    if (0 != uname(&platform))
    {
        fprintf(stderr, "Cannot identify platform: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }
    printf("Platform: %s %s\n\n", platform.sysname, platform.machine);

    failed |= InspectLibrary("libssl.so.3", g_openSslSymbols);
    failed |= InspectLibrary("libssl.so.1.1", g_openSslSymbols);
    puts("\nLegacy OpenSSL inventory only; not an approved adapter:");
    failed |= InspectLibrary("libssl.so.1.0.0", NULL);
    failed |= InspectLibrary("libssl.so.10", NULL);

    puts("\nNSS/NSPR candidate libraries:");
    failed |= InspectLibrary("libnss3.so", g_nssSymbols);
    failed |= InspectLibrary("libssl3.so", g_nssSslSymbols);
    failed |= InspectLibrary("libnspr4.so", g_nsprSymbols);

    puts("\nOptional trust modules; these are not interchangeable trust policies:");
    failed |= InspectLibrary("libnssckbi.so", g_trustModuleSymbols);
    failed |= InspectLibrary("p11-kit-trust.so", g_trustModuleSymbols);
    failed |= InspectLibrary("libnsspem.so", g_trustModuleSymbols);

    puts("\nCommon trust locations; not an exhaustive list or runtime search order:");
    for (i = 0; i < sizeof(trustPaths) / sizeof(trustPaths[0]); ++i)
    {
        failed |= InspectTrustPath(trustPaths[i]);
    }

    puts("\nMissing libraries, symbols, and paths are inventory findings, not probe errors.");
    puts("Exit 0 means inventory completed, NOT that telemetry TLS is supported.");
    if ((0 != fflush(stdout)) || ferror(stdout))
    {
        fprintf(stderr, "Cannot write complete TLS inventory output\n");
        failed = 1;
    }
    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
