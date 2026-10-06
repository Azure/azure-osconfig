// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

/*
 *  X.509 common functions for parsing and verification
 *
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: library/x509.c.
 */
/*
 *  The ITU-T X.509 standard defines a certificate format for PKI.
 *
 *  http://www.ietf.org/rfc/rfc5280.txt (Certificates and CRLs)
 *  http://www.ietf.org/rfc/rfc3279.txt (Alg IDs for CRLs)
 *  http://www.ietf.org/rfc/rfc2986.txt (CSRs, aka PKCS#10)
 *
 *  http://www.itu.int/ITU-T/studygroups/com17/languages/X.680-0207.pdf
 *  http://www.itu.int/ITU-T/studygroups/com17/languages/X.690-0207.pdf
 */

#include "common.h"

#include "x509_internal.h"
#include "asn1.h"
#include "error.h"
#include "oid.h"

#include <stdio.h>
#include <string.h>

#include "pem.h"

#include "asn1write.h"

#include "platform.h"

#include "platform_time.h"
#include "platform_util.h"
#include <time.h>

#define CHECK(code)                                     \
    do {                                                \
        if ((ret = (code)) != 0) {                      \
            MINTLS_RETURN(ret);                                 \
        }                                               \
    } while (0)

#define CHECK_RANGE(min, max, val)                      \
    do {                                                \
        if ((val) < (min) || (val) > (max)) {           \
            MINTLS_RETURN(ret);                                 \
        }                                               \
    } while (0)

/*
 *  CertificateSerialNumber  ::=  INTEGER
 */
int mbedtls_x509_get_serial(unsigned char **p, const unsigned char *end,
                            mbedtls_x509_buf *serial, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    if ((end - *p) < 1) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_SERIAL,
                                 MBEDTLS_ERR_ASN1_OUT_OF_DATA));
    }

    if (**p != (MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_ASN1_PRIMITIVE | 2) &&
        **p !=   MBEDTLS_ASN1_INTEGER) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_SERIAL,
                                 MBEDTLS_ERR_ASN1_UNEXPECTED_TAG));
    }

    serial->tag = *(*p)++;

    if ((ret = mbedtls_asn1_get_len(p, end, &serial->len, diagnostics)) != 0) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_SERIAL, ret));
    }

    serial->p = *p;
    *p += serial->len;

    MINTLS_RETURN(0);
}

/* Get an algorithm identifier without parameters (eg for signatures)
 *
 *  AlgorithmIdentifier  ::=  SEQUENCE  {
 *       algorithm               OBJECT IDENTIFIER,
 *       parameters              ANY DEFINED BY algorithm OPTIONAL  }
 */
int mbedtls_x509_get_alg_null(unsigned char **p, const unsigned char *end,
                              mbedtls_x509_buf *alg, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    if ((ret = mbedtls_asn1_get_alg_null(p, end, alg, diagnostics)) != 0) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_ALG, ret));
    }

    MINTLS_RETURN(0);
}

/*
 * Parse an algorithm identifier with (optional) parameters
 */
int mbedtls_x509_get_alg(unsigned char **p, const unsigned char *end,
                         mbedtls_x509_buf *alg, mbedtls_x509_buf *params, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    if ((ret = mbedtls_asn1_get_alg(p, end, alg, params, diagnostics)) != 0) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_ALG, ret));
    }

    MINTLS_RETURN(0);
}

/*
 * Convert md type to string
 */
#if !defined(MBEDTLS_X509_REMOVE_INFO) && defined(MBEDTLS_X509_RSASSA_PSS_SUPPORT)

const char *md_type_to_string(mbedtls_md_type_t md_alg)
{
    switch (md_alg) {
        case MBEDTLS_MD_SHA1:
            return "SHA1";
        case MBEDTLS_MD_SHA256:
            return "SHA256";
        case MBEDTLS_MD_SHA384:
            return "SHA384";
        case MBEDTLS_MD_SHA512:
            return "SHA512";
        case MBEDTLS_MD_NONE:
            return NULL;
        default:
            return NULL;
    }
}

#endif /* !defined(MBEDTLS_X509_REMOVE_INFO) && defined(MBEDTLS_X509_RSASSA_PSS_SUPPORT) */

/*
 * HashAlgorithm ::= AlgorithmIdentifier
 *
 * AlgorithmIdentifier  ::=  SEQUENCE  {
 *      algorithm               OBJECT IDENTIFIER,
 *      parameters              ANY DEFINED BY algorithm OPTIONAL  }
 *
 * For HashAlgorithm, parameters MUST be NULL or absent.
 */
int x509_get_hash_alg(const mbedtls_x509_buf *alg, mbedtls_md_type_t *md_alg, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    unsigned char *p;
    const unsigned char *end;
    mbedtls_x509_buf md_oid;
    size_t len;

    /* Make sure we got a SEQUENCE and setup bounds */
    if (alg->tag != (MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE)) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_ALG,
                                 MBEDTLS_ERR_ASN1_UNEXPECTED_TAG));
    }

    p = alg->p;
    end = p + alg->len;

    if (p >= end) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_ALG,
                                 MBEDTLS_ERR_ASN1_OUT_OF_DATA));
    }

    /* Parse md_oid */
    md_oid.tag = *p;

    if ((ret = mbedtls_asn1_get_tag(&p, end, &md_oid.len, MBEDTLS_ASN1_OID, diagnostics)) != 0) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_ALG, ret));
    }

    md_oid.p = p;
    p += md_oid.len;

    /* Get md_alg from md_oid */
    if ((ret = mbedtls_oid_get_md_alg(&md_oid, md_alg, diagnostics)) != 0) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_ALG, ret));
    }

    /* Make sure params is absent of NULL */
    if (p == end) {
        MINTLS_RETURN(0);
    }

    if ((ret = mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_NULL, diagnostics)) != 0 || len != 0) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_ALG, ret));
    }

    if (p != end) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_ALG,
                                 MBEDTLS_ERR_ASN1_LENGTH_MISMATCH));
    }

    MINTLS_RETURN(0);
}

/*
 *    RSASSA-PSS-params  ::=  SEQUENCE  {
 *       hashAlgorithm     [0] HashAlgorithm DEFAULT sha1Identifier,
 *       maskGenAlgorithm  [1] MaskGenAlgorithm DEFAULT mgf1SHA1Identifier,
 *       saltLength        [2] INTEGER DEFAULT 20,
 *       trailerField      [3] INTEGER DEFAULT 1  }
 *    -- Note that the tags in this Sequence are explicit.
 *
 * RFC 4055 (which defines use of RSASSA-PSS in PKIX) states that the value
 * of trailerField MUST be 1, and PKCS#1 v2.2 doesn't even define any other
 * option. Enforce this at parsing time.
 */
int mbedtls_x509_get_rsassa_pss_params(const mbedtls_x509_buf *params,
                                       mbedtls_md_type_t *md_alg, mbedtls_md_type_t *mgf_md,
                                       int *salt_len, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    unsigned char *p;
    const unsigned char *end, *end2;
    size_t len;
    mbedtls_x509_buf alg_id, alg_params;

    /* First set everything to defaults */
    *md_alg = MBEDTLS_MD_SHA1;
    *mgf_md = MBEDTLS_MD_SHA1;
    *salt_len = 20;

    /* Make sure params is a SEQUENCE and setup bounds */
    if (params->tag != (MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE)) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_ALG,
                                 MBEDTLS_ERR_ASN1_UNEXPECTED_TAG));
    }

    p = (unsigned char *) params->p;
    end = p + params->len;

    if (p == end) {
        MINTLS_RETURN(0);
    }

    /*
     * HashAlgorithm
     */
    if ((ret = mbedtls_asn1_get_tag(&p, end, &len,
                                    MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_ASN1_CONSTRUCTED |
                                    0, diagnostics)) == 0) {
        end2 = p + len;

        /* HashAlgorithm ::= AlgorithmIdentifier (without parameters) */
        if ((ret = mbedtls_x509_get_alg_null(&p, end2, &alg_id, diagnostics)) != 0) {
            MINTLS_RETURN(ret);
        }

        if ((ret = mbedtls_oid_get_md_alg(&alg_id, md_alg, diagnostics)) != 0) {
            MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_ALG, ret));
        }

        if (p != end2) {
            MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_ALG,
                                     MBEDTLS_ERR_ASN1_LENGTH_MISMATCH));
        }
    } else if (ret != MBEDTLS_ERR_ASN1_UNEXPECTED_TAG) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_ALG, ret));
    }

    if (p == end) {
        MINTLS_RETURN(0);
    }

    /*
     * MaskGenAlgorithm
     */
    if ((ret = mbedtls_asn1_get_tag(&p, end, &len,
                                    MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_ASN1_CONSTRUCTED |
                                    1, diagnostics)) == 0) {
        end2 = p + len;

        /* MaskGenAlgorithm ::= AlgorithmIdentifier (params = HashAlgorithm) */
        if ((ret = mbedtls_x509_get_alg(&p, end2, &alg_id, &alg_params, diagnostics)) != 0) {
            MINTLS_RETURN(ret);
        }

        /* Only MFG1 is recognised for now */
        if (MBEDTLS_OID_CMP(MBEDTLS_OID_MGF1, &alg_id) != 0) {
            MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_FEATURE_UNAVAILABLE,
                                     MBEDTLS_ERR_OID_NOT_FOUND));
        }

        /* Parse HashAlgorithm */
        if ((ret = x509_get_hash_alg(&alg_params, mgf_md, diagnostics)) != 0) {
            MINTLS_RETURN(ret);
        }

        if (p != end2) {
            MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_ALG,
                                     MBEDTLS_ERR_ASN1_LENGTH_MISMATCH));
        }
    } else if (ret != MBEDTLS_ERR_ASN1_UNEXPECTED_TAG) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_ALG, ret));
    }

    if (p == end) {
        MINTLS_RETURN(0);
    }

    /*
     * salt_len
     */
    if ((ret = mbedtls_asn1_get_tag(&p, end, &len,
                                    MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_ASN1_CONSTRUCTED |
                                    2, diagnostics)) == 0) {
        end2 = p + len;

        if ((ret = mbedtls_asn1_get_int(&p, end2, salt_len, diagnostics)) != 0) {
            MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_ALG, ret));
        }

        if (p != end2) {
            MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_ALG,
                                     MBEDTLS_ERR_ASN1_LENGTH_MISMATCH));
        }
    } else if (ret != MBEDTLS_ERR_ASN1_UNEXPECTED_TAG) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_ALG, ret));
    }

    if (p == end) {
        MINTLS_RETURN(0);
    }

    /*
     * trailer_field (if present, must be 1)
     */
    if ((ret = mbedtls_asn1_get_tag(&p, end, &len,
                                    MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_ASN1_CONSTRUCTED |
                                    3, diagnostics)) == 0) {
        int trailer_field;

        end2 = p + len;

        if ((ret = mbedtls_asn1_get_int(&p, end2, &trailer_field, diagnostics)) != 0) {
            MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_ALG, ret));
        }

        if (p != end2) {
            MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_ALG,
                                     MBEDTLS_ERR_ASN1_LENGTH_MISMATCH));
        }

        if (trailer_field != 1) {
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_X509_INVALID_ALG);
        }
    } else if (ret != MBEDTLS_ERR_ASN1_UNEXPECTED_TAG) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_ALG, ret));
    }

    if (p != end) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_ALG,
                                 MBEDTLS_ERR_ASN1_LENGTH_MISMATCH));
    }

    MINTLS_RETURN(0);
}

/*
 *  AttributeTypeAndValue ::= SEQUENCE {
 *    type     AttributeType,
 *    value    AttributeValue }
 *
 *  AttributeType ::= OBJECT IDENTIFIER
 *
 *  AttributeValue ::= ANY DEFINED BY AttributeType
 */
int x509_get_attr_type_value(unsigned char **p,
                                    const unsigned char *end,
                                    mbedtls_x509_name *cur, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    size_t len;
    mbedtls_x509_buf *oid;
    mbedtls_x509_buf *val;

    if ((ret = mbedtls_asn1_get_tag(p, end, &len,
                                    MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE, diagnostics)) != 0) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_NAME, ret));
    }

    end = *p + len;

    if ((end - *p) < 1) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_NAME,
                                 MBEDTLS_ERR_ASN1_OUT_OF_DATA));
    }

    oid = &cur->oid;
    oid->tag = **p;

    if ((ret = mbedtls_asn1_get_tag(p, end, &oid->len, MBEDTLS_ASN1_OID, diagnostics)) != 0) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_NAME, ret));
    }

    oid->p = *p;
    *p += oid->len;

    if ((end - *p) < 1) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_NAME,
                                 MBEDTLS_ERR_ASN1_OUT_OF_DATA));
    }

    if (**p != MBEDTLS_ASN1_BMP_STRING && **p != MBEDTLS_ASN1_UTF8_STRING      &&
        **p != MBEDTLS_ASN1_T61_STRING && **p != MBEDTLS_ASN1_PRINTABLE_STRING &&
        **p != MBEDTLS_ASN1_IA5_STRING && **p != MBEDTLS_ASN1_UNIVERSAL_STRING &&
        **p != MBEDTLS_ASN1_BIT_STRING) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_NAME,
                                 MBEDTLS_ERR_ASN1_UNEXPECTED_TAG));
    }

    val = &cur->val;
    val->tag = *(*p)++;

    if ((ret = mbedtls_asn1_get_len(p, end, &val->len, diagnostics)) != 0) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_NAME, ret));
    }

    val->p = *p;
    *p += val->len;

    if (*p != end) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_NAME,
                                 MBEDTLS_ERR_ASN1_LENGTH_MISMATCH));
    }

    cur->next = NULL;

    MINTLS_RETURN(0);
}

/*
 *  Name ::= CHOICE { -- only one possibility for now --
 *       rdnSequence  RDNSequence }
 *
 *  RDNSequence ::= SEQUENCE OF RelativeDistinguishedName
 *
 *  RelativeDistinguishedName ::=
 *    SET OF AttributeTypeAndValue
 *
 *  AttributeTypeAndValue ::= SEQUENCE {
 *    type     AttributeType,
 *    value    AttributeValue }
 *
 *  AttributeType ::= OBJECT IDENTIFIER
 *
 *  AttributeValue ::= ANY DEFINED BY AttributeType
 *
 * The data structure is optimized for the common case where each RDN has only
 * one element, which is represented as a list of AttributeTypeAndValue.
 * For the general case we still use a flat list, but we mark elements of the
 * same set so that they are "merged" together in the functions that consume
 * this list, eg mbedtls_x509_dn_gets().
 *
 * On success, this function may allocate a linked list starting at cur->next
 * that must later be free'd by the caller using mbedtls_free(). In error
 * cases, this function frees all allocated memory internally and the caller
 * has no freeing responsibilities.
 */
int mbedtls_x509_get_name(unsigned char **p, const unsigned char *end,
                          mbedtls_x509_name *cur, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    size_t set_len;
    const unsigned char *end_set;
    mbedtls_x509_name *head = cur;

    /* don't use recursion, we'd risk stack overflow if not optimized */
    while (1) {
        /*
         * parse SET
         */
        if ((ret = mbedtls_asn1_get_tag(p, end, &set_len,
                                        MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SET, diagnostics)) != 0) {
            ret = MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_NAME, ret);
            goto error;
        }

        end_set  = *p + set_len;

        while (1) {
            if ((ret = x509_get_attr_type_value(p, end_set, cur, diagnostics)) != 0) {
                goto error;
            }

            if (*p == end_set) {
                break;
            }

            /* Mark this item as being no the only one in a set */
            cur->next_merged = 1;

            cur->next = mbedtls_calloc(1, sizeof(mbedtls_x509_name));

            if (cur->next == NULL) {
                ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_X509_ALLOC_FAILED);
                goto error;
            }

            cur = cur->next;
        }

        /*
         * continue until end of SEQUENCE is reached
         */
        if (*p == end) {
            MINTLS_RETURN(0);
        }

        cur->next = mbedtls_calloc(1, sizeof(mbedtls_x509_name));

        if (cur->next == NULL) {
            ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_X509_ALLOC_FAILED);
            goto error;
        }

        cur = cur->next;
    }

error:
    /* Skip the first element as we did not allocate it */
    mbedtls_asn1_free_named_data_list_shallow(head->next);
    head->next = NULL;

    MINTLS_RETURN(ret);
}

int x509_date_is_valid(const mbedtls_x509_time *t, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    unsigned int month_days;
    unsigned int year;
    switch (t->mon) {
        case 1: case 3: case 5: case 7: case 8: case 10: case 12:
            month_days = 31;
            break;
        case 4: case 6: case 9: case 11:
            month_days = 30;
            break;
        case 2:
            year = (unsigned int) t->year;
            month_days = ((year & 3) || (!(year % 100)
                                         && (year % 400)))
                          ? 28 : 29;
            break;
        default:
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_X509_INVALID_DATE);
    }

    if ((unsigned int) (t->day - 1) >= month_days ||      /* (1 - days in month) */
        /* (unsigned int) (t->mon - 1) >= 12 || */  /* (1 - 12) checked above */
        (unsigned int) t->year > 9999 ||         /* (0 - 9999) */
        (unsigned int) t->hour > 23 ||           /* (0 - 23) */
        (unsigned int) t->min  > 59 ||           /* (0 - 59) */
        (unsigned int) t->sec  > 59) {           /* (0 - 59) */
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_X509_INVALID_DATE);
    }

    MINTLS_RETURN(0);
}

int x509_parse2_int(const unsigned char *p)
{
    uint32_t d1 = p[0] - '0';
    uint32_t d2 = p[1] - '0';
    return (d1 < 10 && d2 < 10) ? (int) (d1 * 10 + d2) : -1;
}

/*
 * Parse an ASN1_UTC_TIME (yearlen=2) or ASN1_GENERALIZED_TIME (yearlen=4)
 * field.
 */
int x509_parse_time(const unsigned char *p, mbedtls_x509_time *tm,
                           size_t yearlen, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int x;

    /*
     * Parse year, month, day, hour, minute, second
     */
    tm->year = x509_parse2_int(p);
    if (tm->year < 0) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_X509_INVALID_DATE);
    }

    if (4 == yearlen) {
        x = tm->year * 100;
        p += 2;
        tm->year = x509_parse2_int(p);
        if (tm->year < 0) {
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_X509_INVALID_DATE);
        }
    } else {
        x = (tm->year < 50) ? 2000 : 1900;
    }
    tm->year += x;

    tm->mon  = x509_parse2_int(p + 2);
    tm->day  = x509_parse2_int(p + 4);
    tm->hour = x509_parse2_int(p + 6);
    tm->min  = x509_parse2_int(p + 8);
    tm->sec  = x509_parse2_int(p + 10);

    MINTLS_RETURN(x509_date_is_valid(tm, diagnostics));
}

/*
 *  Time ::= CHOICE {
 *       utcTime        UTCTime,
 *       generalTime    GeneralizedTime }
 */
int mbedtls_x509_get_time(unsigned char **p, const unsigned char *end,
                          mbedtls_x509_time *tm, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    size_t len, year_len;
    unsigned char tag;

    if ((end - *p) < 1) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_DATE,
                                 MBEDTLS_ERR_ASN1_OUT_OF_DATA));
    }

    tag = **p;

    if (tag == MBEDTLS_ASN1_UTC_TIME) {
        year_len = 2;
    } else if (tag == MBEDTLS_ASN1_GENERALIZED_TIME) {
        year_len = 4;
    } else {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_DATE,
                                 MBEDTLS_ERR_ASN1_UNEXPECTED_TAG));
    }

    (*p)++;
    ret = mbedtls_asn1_get_len(p, end, &len, diagnostics);

    if (ret != 0) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_DATE, ret));
    }

    /* len is 12 or 14 depending on year_len, plus optional trailing 'Z' */
    if (len != year_len + 10 &&
        !(len == year_len + 11 && (*p)[(len - 1)] == 'Z')) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_X509_INVALID_DATE);
    }

    (*p) += len;
    MINTLS_RETURN(x509_parse_time(*p - len, tm, year_len, diagnostics));
}

int mbedtls_x509_get_sig(unsigned char **p, const unsigned char *end, mbedtls_x509_buf *sig, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    size_t len;
    int tag_type;

    if ((end - *p) < 1) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_SIGNATURE,
                                 MBEDTLS_ERR_ASN1_OUT_OF_DATA));
    }

    tag_type = **p;

    if ((ret = mbedtls_asn1_get_bitstring_null(p, end, &len, diagnostics)) != 0) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_SIGNATURE, ret));
    }

    sig->tag = tag_type;
    sig->len = len;
    sig->p = *p;

    *p += len;

    MINTLS_RETURN(0);
}

/*
 * Get signature algorithm from alg OID and optional parameters
 */
int mbedtls_x509_get_sig_alg(const mbedtls_x509_buf *sig_oid, const mbedtls_x509_buf *sig_params,
                             mbedtls_md_type_t *md_alg, mbedtls_pk_type_t *pk_alg,
                             void **sig_opts, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    if (*sig_opts != NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_X509_BAD_INPUT_DATA);
    }

    if ((ret = mbedtls_oid_get_sig_alg(sig_oid, md_alg, pk_alg, diagnostics)) != 0) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_UNKNOWN_SIG_ALG, ret));
    }

    if (*pk_alg == MBEDTLS_PK_RSASSA_PSS) {
        mbedtls_pk_rsassa_pss_options *pss_opts;

        pss_opts = mbedtls_calloc(1, sizeof(mbedtls_pk_rsassa_pss_options));
        if (pss_opts == NULL) {
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_X509_ALLOC_FAILED);
        }

        ret = mbedtls_x509_get_rsassa_pss_params(sig_params,
                                                 md_alg,
                                                 &pss_opts->mgf1_hash_id,
                                                 &pss_opts->expected_salt_len, diagnostics);
        if (ret != 0) {
            mbedtls_free(pss_opts);
            MINTLS_RETURN(ret);
        }

        *sig_opts = (void *) pss_opts;
    } else
    {
        /* Make sure parameters are absent or NULL */
        if ((sig_params->tag != MBEDTLS_ASN1_NULL && sig_params->tag != 0) ||
            sig_params->len != 0) {
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_X509_INVALID_ALG);
        }
    }

    MINTLS_RETURN(0);
}

/*
 * X.509 Extensions (No parsing of extensions, pointer should
 * be either manually updated or extensions should be parsed!)
 */
int mbedtls_x509_get_ext(unsigned char **p, const unsigned char *end,
                         mbedtls_x509_buf *ext, int tag, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    size_t len;

    /* Extension structure use EXPLICIT tagging. That is, the actual
     * `Extensions` structure is wrapped by a tag-length pair using
     * the respective context-specific tag. */
    ret = mbedtls_asn1_get_tag(p, end, &ext->len,
                               MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_ASN1_CONSTRUCTED | tag, diagnostics);
    if (ret != 0) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_EXTENSIONS, ret));
    }

    ext->tag = MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_ASN1_CONSTRUCTED | tag;
    ext->p   = *p;
    end      = *p + ext->len;

    /*
     * Extensions  ::=  SEQUENCE SIZE (1..MAX) OF Extension
     */
    if ((ret = mbedtls_asn1_get_tag(p, end, &len,
                                    MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE, diagnostics)) != 0) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_EXTENSIONS, ret));
    }

    if (end != *p + len) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_EXTENSIONS,
                                 MBEDTLS_ERR_ASN1_LENGTH_MISMATCH));
    }

    MINTLS_RETURN(0);
}

char nibble_to_hex_digit(int i)
{
    return (i < 10) ? (i + '0') : (i - 10 + 'A');
}

/*
 * Store the name in printable form into buf; no more
 * than size characters will be written
 */
int mbedtls_x509_dn_gets(char *buf, size_t size, const mbedtls_x509_name *dn, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    size_t i, j, n, asn1_len_size, asn1_tag_size, asn1_tag_len_buf_start;
    /* 6 is enough as our asn1 write functions only write one byte for the tag and at most five bytes for the length*/
    unsigned char asn1_tag_len_buf[6];
    unsigned char *asn1_len_p;
    unsigned char c, merge = 0;
    const mbedtls_x509_name *name;
    const char *short_name = NULL;
    char lowbits, highbits;
    char s[MBEDTLS_X509_MAX_DN_NAME_SIZE], *p;
    int print_hexstring;

    memset(s, 0, sizeof(s));

    name = dn;
    p = buf;
    n = size;

    while (name != NULL) {
        if (!name->oid.p) {
            name = name->next;
            continue;
        }

        if (name != dn) {
            ret = mbedtls_snprintf(p, n, merge ? " + " : ", ");
            MBEDTLS_X509_SAFE_SNPRINTF;
        }

        print_hexstring = (name->val.tag != MBEDTLS_ASN1_UTF8_STRING) &&
                          (name->val.tag != MBEDTLS_ASN1_PRINTABLE_STRING) &&
                          (name->val.tag != MBEDTLS_ASN1_IA5_STRING);

        if ((ret = mbedtls_oid_get_attr_short_name(&name->oid, &short_name, diagnostics)) == 0) {
            ret = mbedtls_snprintf(p, n, "%s=", short_name);
        } else {
            if ((ret = mbedtls_oid_get_numeric_string(p, n, &name->oid, diagnostics)) > 0) {
                n -= ret;
                p += ret;
                ret = mbedtls_snprintf(p, n, "=");
                print_hexstring = 1;
            } else if (ret == MBEDTLS_ERR_OID_BUF_TOO_SMALL) {
                MINTLS_RETURN_ERROR(MBEDTLS_ERR_X509_BUFFER_TOO_SMALL);
            } else {
                ret = mbedtls_snprintf(p, n, "\?\?=");
            }
        }
        MBEDTLS_X509_SAFE_SNPRINTF;

        if (print_hexstring) {
            s[0] = '#';

            asn1_len_p = asn1_tag_len_buf + sizeof(asn1_tag_len_buf);
            if ((ret = mbedtls_asn1_write_len(&asn1_len_p, asn1_tag_len_buf, name->val.len, diagnostics)) < 0) {
                MINTLS_RETURN_ERROR(MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED);
            }
            asn1_len_size = ret;
            if ((ret = mbedtls_asn1_write_tag(&asn1_len_p, asn1_tag_len_buf, name->val.tag, diagnostics)) < 0) {
                MINTLS_RETURN_ERROR(MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED);
            }
            asn1_tag_size = ret;
            asn1_tag_len_buf_start = sizeof(asn1_tag_len_buf) - asn1_len_size - asn1_tag_size;
            for (i = 0, j = 1; i < asn1_len_size + asn1_tag_size; i++) {
                if (j + 1 >= sizeof(s) - 1) {
                    MINTLS_RETURN_ERROR(MBEDTLS_ERR_X509_BUFFER_TOO_SMALL);
                }
                c = asn1_tag_len_buf[asn1_tag_len_buf_start+i];
                lowbits = (c & 0x0F);
                highbits = c >> 4;
                s[j++] = nibble_to_hex_digit(highbits);
                s[j++] = nibble_to_hex_digit(lowbits);
            }
            for (i = 0; i < name->val.len; i++) {
                if (j + 1 >= sizeof(s) - 1) {
                    MINTLS_RETURN_ERROR(MBEDTLS_ERR_X509_BUFFER_TOO_SMALL);
                }
                c = name->val.p[i];
                lowbits = (c & 0x0F);
                highbits = c >> 4;
                s[j++] = nibble_to_hex_digit(highbits);
                s[j++] = nibble_to_hex_digit(lowbits);
            }
        } else {
            for (i = 0, j = 0; i < name->val.len; i++, j++) {
                if (j >= sizeof(s) - 1) {
                    MINTLS_RETURN_ERROR(MBEDTLS_ERR_X509_BUFFER_TOO_SMALL);
                }

                c = name->val.p[i];
                // Special characters requiring escaping, RFC 4514 Section 2.4
                if (c == '\0') {
                    MINTLS_RETURN_ERROR(MBEDTLS_ERR_X509_INVALID_NAME);
                } else {
                    if (strchr(",=+<>;\"\\", c) ||
                        ((i == 0) && strchr("# ", c)) ||
                        ((i == name->val.len-1) && (c == ' '))) {
                        if (j + 1 >= sizeof(s) - 1) {
                            MINTLS_RETURN_ERROR(MBEDTLS_ERR_X509_BUFFER_TOO_SMALL);
                        }
                        s[j++] = '\\';
                    }
                }
                if (c < 32 || c >= 127) {
                    if (j + 3 >= sizeof(s) - 1) {
                        MINTLS_RETURN_ERROR(MBEDTLS_ERR_X509_BUFFER_TOO_SMALL);
                    }
                    s[j++] = '\\';
                    lowbits = (c & 0x0F);
                    highbits = c >> 4;
                    s[j++] = nibble_to_hex_digit(highbits);
                    s[j] = nibble_to_hex_digit(lowbits);
                } else {
                    s[j] = c;
                }
            }
        }
        s[j] = '\0';
        ret = mbedtls_snprintf(p, n, "%s", s);
        MBEDTLS_X509_SAFE_SNPRINTF;

        merge = name->next_merged;
        name = name->next;
    }

    MINTLS_RETURN((int) (size - n));
}

/*
 * Store the serial in printable form into buf; no more
 * than size characters will be written
 */
int mbedtls_x509_serial_gets(char *buf, size_t size, const mbedtls_x509_buf *serial, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    size_t i, n, nr;
    char *p;

    p = buf;
    n = size;

    nr = (serial->len <= 32)
        ? serial->len  : 28;

    for (i = 0; i < nr; i++) {
        if (i == 0 && nr > 1 && serial->p[i] == 0x0) {
            continue;
        }

        ret = mbedtls_snprintf(p, n, "%02X%s",
                               serial->p[i], (i < nr - 1) ? ":" : "");
        MBEDTLS_X509_SAFE_SNPRINTF;
    }

    if (nr != serial->len) {
        ret = mbedtls_snprintf(p, n, "....");
        MBEDTLS_X509_SAFE_SNPRINTF;
    }

    MINTLS_RETURN((int) (size - n));
}

#if !defined(MBEDTLS_X509_REMOVE_INFO)
/*
 * Helper for writing signature algorithms
 */
int mbedtls_x509_sig_alg_gets(char *buf, size_t size, const mbedtls_x509_buf *sig_oid,
                              mbedtls_pk_type_t pk_alg, mbedtls_md_type_t md_alg,
                              const void *sig_opts, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    char *p = buf;
    size_t n = size;
    const char *desc = NULL;

    ret = mbedtls_oid_get_sig_alg_desc(sig_oid, &desc, diagnostics);
    if (ret != 0) {
        ret = mbedtls_snprintf(p, n, "???");
    } else {
        ret = mbedtls_snprintf(p, n, "%s", desc);
    }
    MBEDTLS_X509_SAFE_SNPRINTF;

    if (pk_alg == MBEDTLS_PK_RSASSA_PSS) {
        const mbedtls_pk_rsassa_pss_options *pss_opts;

        pss_opts = (const mbedtls_pk_rsassa_pss_options *) sig_opts;

        const char *name = md_type_to_string(md_alg);
        const char *mgf_name = md_type_to_string(pss_opts->mgf1_hash_id);

        ret = mbedtls_snprintf(p, n, " (%s, MGF1-%s, 0x%02X)",
                               name ? name : "???",
                               mgf_name ? mgf_name : "???",
                               (unsigned int) pss_opts->expected_salt_len);
        MBEDTLS_X509_SAFE_SNPRINTF;
    }

    MINTLS_RETURN((int) (size - n));
}
#endif /* MBEDTLS_X509_REMOVE_INFO */

/*
 * Helper for writing "RSA key size", "EC key size", etc
 */
int mbedtls_x509_key_size_helper(char *buf, size_t buf_size, const char *name, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    char *p = buf;
    size_t n = buf_size;
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    ret = mbedtls_snprintf(p, n, "%s key size", name);
    MBEDTLS_X509_SAFE_SNPRINTF;

    MINTLS_RETURN(0);
}

int mbedtls_x509_time_cmp(const mbedtls_x509_time *t1,
                          const mbedtls_x509_time *t2)
{
    int x;

    x = (((t1->year << 9) | (t1->mon << 5) | (t1->day)) -
         ((t2->year << 9) | (t2->mon << 5) | (t2->day)));
    if (x != 0) {
        return x;
    }

    x = (((t1->hour << 12) | (t1->min << 6) | (t1->sec)) -
         ((t2->hour << 12) | (t2->min << 6) | (t2->sec)));
    return x;
}

int mbedtls_x509_time_gmtime(mbedtls_time_t tt, mbedtls_x509_time *now)
{
    struct tm tm;

    if (mbedtls_platform_gmtime_r(&tt, &tm) == NULL) {
        return -1;
    }

    now->year = tm.tm_year + 1900;
    now->mon  = tm.tm_mon  + 1;
    now->day  = tm.tm_mday;
    now->hour = tm.tm_hour;
    now->min  = tm.tm_min;
    now->sec  = tm.tm_sec;
    return 0;
}

int x509_get_current_time(mbedtls_x509_time *now)
{
    return mbedtls_x509_time_gmtime(mbedtls_time(NULL), now);
}

int mbedtls_x509_time_is_past(const mbedtls_x509_time *to)
{
    mbedtls_x509_time now;

    if (x509_get_current_time(&now) != 0) {
        return 1;
    }

    return mbedtls_x509_time_cmp(to, &now) < 0;
}

int mbedtls_x509_time_is_future(const mbedtls_x509_time *from)
{
    mbedtls_x509_time now;

    if (x509_get_current_time(&now) != 0) {
        return 1;
    }

    return mbedtls_x509_time_cmp(from, &now) > 0;
}

/* Common functions for parsing CRT and CSR. */
/*
 * OtherName ::= SEQUENCE {
 *      type-id    OBJECT IDENTIFIER,
 *      value      [0] EXPLICIT ANY DEFINED BY type-id }
 *
 * HardwareModuleName ::= SEQUENCE {
 *                           hwType OBJECT IDENTIFIER,
 *                           hwSerialNum OCTET STRING }
 *
 * NOTE: we currently only parse and use otherName of type HwModuleName,
 * as defined in RFC 4108.
 */
int x509_get_other_name(const mbedtls_x509_buf *subject_alt_name,
                               mbedtls_x509_san_other_name *other_name, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = 0;
    size_t len;
    unsigned char *p = subject_alt_name->p;
    const unsigned char *end = p + subject_alt_name->len;
    mbedtls_x509_buf cur_oid;

    if ((subject_alt_name->tag &
         (MBEDTLS_ASN1_TAG_CLASS_MASK | MBEDTLS_ASN1_TAG_VALUE_MASK)) !=
        (MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_X509_SAN_OTHER_NAME)) {
        /*
         * The given subject alternative name is not of type "othername".
         */
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_X509_BAD_INPUT_DATA);
    }

    if ((ret = mbedtls_asn1_get_tag(&p, end, &len,
                                    MBEDTLS_ASN1_OID, diagnostics)) != 0) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_EXTENSIONS, ret));
    }

    cur_oid.tag = MBEDTLS_ASN1_OID;
    cur_oid.p = p;
    cur_oid.len = len;

    /*
     * Only HwModuleName is currently supported.
     */
    if (MBEDTLS_OID_CMP(MBEDTLS_OID_ON_HW_MODULE_NAME, &cur_oid) != 0) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_X509_FEATURE_UNAVAILABLE);
    }
    other_name->type_id = cur_oid;

    p += len;
    if ((ret = mbedtls_asn1_get_tag(&p, end, &len,
                                    MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_CONTEXT_SPECIFIC, diagnostics)) !=
        0) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_EXTENSIONS, ret));
    }

    if (end != p + len) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_EXTENSIONS,
                                 MBEDTLS_ERR_ASN1_LENGTH_MISMATCH));
    }

    if ((ret = mbedtls_asn1_get_tag(&p, end, &len,
                                    MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE, diagnostics)) != 0) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_EXTENSIONS, ret));
    }

    if (end != p + len) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_EXTENSIONS,
                                 MBEDTLS_ERR_ASN1_LENGTH_MISMATCH));
    }

    if ((ret = mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_OID, diagnostics)) != 0) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_EXTENSIONS, ret));
    }

    other_name->value.hardware_module_name.oid.tag = MBEDTLS_ASN1_OID;
    other_name->value.hardware_module_name.oid.p = p;
    other_name->value.hardware_module_name.oid.len = len;

    p += len;
    if ((ret = mbedtls_asn1_get_tag(&p, end, &len,
                                    MBEDTLS_ASN1_OCTET_STRING, diagnostics)) != 0) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_EXTENSIONS, ret));
    }

    other_name->value.hardware_module_name.val.tag = MBEDTLS_ASN1_OCTET_STRING;
    other_name->value.hardware_module_name.val.p = p;
    other_name->value.hardware_module_name.val.len = len;
    p += len;
    if (p != end) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_EXTENSIONS,
                                 MBEDTLS_ERR_ASN1_LENGTH_MISMATCH));
    }
    MINTLS_RETURN(0);
}

/* Check mbedtls_x509_get_subject_alt_name for detailed description.
 *
 * In some cases while parsing subject alternative names the sequence tag is optional
 * (e.g. CertSerialNumber). This function is designed to handle such case.
 */
int mbedtls_x509_get_subject_alt_name_ext(unsigned char **p,
                                          const unsigned char *end,
                                          mbedtls_x509_sequence *subject_alt_name, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    size_t tag_len;
    mbedtls_asn1_sequence *cur = subject_alt_name;

    while (*p < end) {
        mbedtls_x509_subject_alternative_name tmp_san_name;
        mbedtls_x509_buf tmp_san_buf;
        memset(&tmp_san_name, 0, sizeof(tmp_san_name));

        tmp_san_buf.tag = **p;
        (*p)++;

        if ((ret = mbedtls_asn1_get_len(p, end, &tag_len, diagnostics)) != 0) {
            MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_EXTENSIONS, ret));
        }

        tmp_san_buf.p = *p;
        tmp_san_buf.len = tag_len;

        if ((tmp_san_buf.tag & MBEDTLS_ASN1_TAG_CLASS_MASK) !=
            MBEDTLS_ASN1_CONTEXT_SPECIFIC) {
            MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_EXTENSIONS,
                                     MBEDTLS_ERR_ASN1_UNEXPECTED_TAG));
        }

        /*
         * Check that the SAN is structured correctly by parsing it.
         * The SAN structure is discarded afterwards.
         */
        ret = mbedtls_x509_parse_subject_alt_name(&tmp_san_buf, &tmp_san_name, diagnostics);
        /*
         * In case the extension is malformed, return an error,
         * and clear the allocated sequences.
         */
        if (ret != 0 && ret != MBEDTLS_ERR_X509_FEATURE_UNAVAILABLE) {
            mbedtls_asn1_sequence_free(subject_alt_name->next);
            subject_alt_name->next = NULL;
            MINTLS_RETURN(ret);
        }

        mbedtls_x509_free_subject_alt_name(&tmp_san_name);
        /* Allocate and assign next pointer */
        if (cur->buf.p != NULL) {
            if (cur->next != NULL) {
                MINTLS_RETURN_ERROR(MBEDTLS_ERR_X509_INVALID_EXTENSIONS);
            }

            cur->next = mbedtls_calloc(1, sizeof(mbedtls_asn1_sequence));

            if (cur->next == NULL) {
                MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_EXTENSIONS,
                                         MBEDTLS_ERR_ASN1_ALLOC_FAILED));
            }

            cur = cur->next;
        }

        cur->buf = tmp_san_buf;
        *p += tmp_san_buf.len;
    }

    /* Set final sequence entry's next pointer to NULL */
    cur->next = NULL;

    if (*p != end) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_EXTENSIONS,
                                 MBEDTLS_ERR_ASN1_LENGTH_MISMATCH));
    }

    MINTLS_RETURN(0);
}

/*
 * SubjectAltName ::= GeneralNames
 *
 * GeneralNames ::= SEQUENCE SIZE (1..MAX) OF GeneralName
 *
 * GeneralName ::= CHOICE {
 *      otherName                       [0]     OtherName,
 *      rfc822Name                      [1]     IA5String,
 *      dNSName                         [2]     IA5String,
 *      x400Address                     [3]     ORAddress,
 *      directoryName                   [4]     Name,
 *      ediPartyName                    [5]     EDIPartyName,
 *      uniformResourceIdentifier       [6]     IA5String,
 *      iPAddress                       [7]     OCTET STRING,
 *      registeredID                    [8]     OBJECT IDENTIFIER }
 *
 * OtherName ::= SEQUENCE {
 *      type-id    OBJECT IDENTIFIER,
 *      value      [0] EXPLICIT ANY DEFINED BY type-id }
 *
 * EDIPartyName ::= SEQUENCE {
 *      nameAssigner            [0]     DirectoryString OPTIONAL,
 *      partyName               [1]     DirectoryString }
 *
 * We list all types, but use the following GeneralName types from RFC 5280:
 * "dnsName", "uniformResourceIdentifier" and "hardware_module_name"
 * of type "otherName", as defined in RFC 4108.
 */
int mbedtls_x509_get_subject_alt_name(unsigned char **p,
                                      const unsigned char *end,
                                      mbedtls_x509_sequence *subject_alt_name, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    size_t len;

    /* Get main sequence tag */
    if ((ret = mbedtls_asn1_get_tag(p, end, &len,
                                    MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE, diagnostics)) != 0) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_EXTENSIONS, ret));
    }

    if (*p + len != end) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_EXTENSIONS,
                                 MBEDTLS_ERR_ASN1_LENGTH_MISMATCH));
    }

    MINTLS_RETURN(mbedtls_x509_get_subject_alt_name_ext(p, end, subject_alt_name, diagnostics));
}

int mbedtls_x509_get_ns_cert_type(unsigned char **p,
                                  const unsigned char *end,
                                  unsigned char *ns_cert_type, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    mbedtls_x509_bitstring bs = { 0, 0, NULL };

    if ((ret = mbedtls_asn1_get_bitstring(p, end, &bs, diagnostics)) != 0) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_EXTENSIONS, ret));
    }

    /* A bitstring with no flags set is still technically valid, as it will mean
       that the certificate has no designated purpose at the time of creation. */
    if (bs.len == 0) {
        *ns_cert_type = 0;
        MINTLS_RETURN(0);
    }

    if (bs.len != 1) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_EXTENSIONS,
                                 MBEDTLS_ERR_ASN1_INVALID_LENGTH));
    }

    /* Get actual bitstring */
    *ns_cert_type = *bs.p;
    MINTLS_RETURN(0);
}

int mbedtls_x509_get_key_usage(unsigned char **p,
                               const unsigned char *end,
                               unsigned int *key_usage, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    size_t i;
    mbedtls_x509_bitstring bs = { 0, 0, NULL };

    if ((ret = mbedtls_asn1_get_bitstring(p, end, &bs, diagnostics)) != 0) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_X509_INVALID_EXTENSIONS, ret));
    }

    /* A bitstring with no flags set is still technically valid, as it will mean
       that the certificate has no designated purpose at the time of creation. */
    if (bs.len == 0) {
        *key_usage = 0;
        MINTLS_RETURN(0);
    }

    /* Get actual bitstring */
    *key_usage = 0;
    for (i = 0; i < bs.len && i < sizeof(unsigned int); i++) {
        *key_usage |= (unsigned int) bs.p[i] << (8*i);
    }

    MINTLS_RETURN(0);
}

int mbedtls_x509_parse_subject_alt_name(const mbedtls_x509_buf *san_buf,
                                        mbedtls_x509_subject_alternative_name *san, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    switch (san_buf->tag &
            (MBEDTLS_ASN1_TAG_CLASS_MASK |
             MBEDTLS_ASN1_TAG_VALUE_MASK)) {
        /*
         * otherName
         */
        case (MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_X509_SAN_OTHER_NAME):
        {
            mbedtls_x509_san_other_name other_name;

            ret = x509_get_other_name(san_buf, &other_name, diagnostics);
            if (ret != 0) {
                MINTLS_RETURN(ret);
            }

            memset(san, 0, sizeof(mbedtls_x509_subject_alternative_name));
            san->type = MBEDTLS_X509_SAN_OTHER_NAME;
            memcpy(&san->san.other_name,
                   &other_name, sizeof(other_name));

        }
        break;
        /*
         * uniformResourceIdentifier
         */
        case (MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_X509_SAN_UNIFORM_RESOURCE_IDENTIFIER):
        {
            memset(san, 0, sizeof(mbedtls_x509_subject_alternative_name));
            san->type = MBEDTLS_X509_SAN_UNIFORM_RESOURCE_IDENTIFIER;

            memcpy(&san->san.unstructured_name,
                   san_buf, sizeof(*san_buf));

        }
        break;
        /*
         * dNSName
         */
        case (MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_X509_SAN_DNS_NAME):
        {
            memset(san, 0, sizeof(mbedtls_x509_subject_alternative_name));
            san->type = MBEDTLS_X509_SAN_DNS_NAME;

            memcpy(&san->san.unstructured_name,
                   san_buf, sizeof(*san_buf));
        }
        break;
        /*
         * IP address
         */
        case (MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_X509_SAN_IP_ADDRESS):
        {
            memset(san, 0, sizeof(mbedtls_x509_subject_alternative_name));
            san->type = MBEDTLS_X509_SAN_IP_ADDRESS;
            // Only IPv6 (16 bytes) and IPv4 (4 bytes) types are supported
            if (san_buf->len == 4 || san_buf->len == 16) {
                memcpy(&san->san.unstructured_name,
                       san_buf, sizeof(*san_buf));
            } else {
                MINTLS_RETURN_ERROR(MBEDTLS_ERR_X509_BAD_INPUT_DATA);
            }
        }
        break;
        /*
         * rfc822Name
         */
        case (MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_X509_SAN_RFC822_NAME):
        {
            memset(san, 0, sizeof(mbedtls_x509_subject_alternative_name));
            san->type = MBEDTLS_X509_SAN_RFC822_NAME;
            memcpy(&san->san.unstructured_name, san_buf, sizeof(*san_buf));
        }
        break;
        /*
         * directoryName
         */
        case (MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_X509_SAN_DIRECTORY_NAME):
        {
            size_t name_len;
            unsigned char *p = san_buf->p;
            memset(san, 0, sizeof(mbedtls_x509_subject_alternative_name));
            san->type = MBEDTLS_X509_SAN_DIRECTORY_NAME;

            ret = mbedtls_asn1_get_tag(&p, p + san_buf->len, &name_len,
                                       MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE, diagnostics);

            if (ret != 0) {
                MINTLS_RETURN(ret);
            }

            if ((ret = mbedtls_x509_get_name(&p, p + name_len,
                                             &san->san.directory_name, diagnostics)) != 0) {
                MINTLS_RETURN(ret);
            }
        }
        break;
        /*
         * Type not supported
         */
        default:
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_X509_FEATURE_UNAVAILABLE);
    }
    MINTLS_RETURN(0);
}

void mbedtls_x509_free_subject_alt_name(mbedtls_x509_subject_alternative_name *san)
{
    if (san->type == MBEDTLS_X509_SAN_DIRECTORY_NAME) {
        mbedtls_asn1_free_named_data_list_shallow(san->san.directory_name.next);
    }
}

#if !defined(MBEDTLS_X509_REMOVE_INFO)
int mbedtls_x509_info_subject_alt_name(char **buf, size_t *size,
                                       const mbedtls_x509_sequence
                                       *subject_alt_name,
                                       const char *prefix, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    size_t i;
    size_t n = *size;
    char *p = *buf;
    const mbedtls_x509_sequence *cur = subject_alt_name;
    mbedtls_x509_subject_alternative_name san;
    int parse_ret;

    while (cur != NULL) {
        memset(&san, 0, sizeof(san));
        parse_ret = mbedtls_x509_parse_subject_alt_name(&cur->buf, &san, diagnostics);
        if (parse_ret != 0) {
            if (parse_ret == MBEDTLS_ERR_X509_FEATURE_UNAVAILABLE) {
                ret = mbedtls_snprintf(p, n, "\n%s    <unsupported>", prefix);
                MBEDTLS_X509_SAFE_SNPRINTF;
            } else {
                ret = mbedtls_snprintf(p, n, "\n%s    <malformed>", prefix);
                MBEDTLS_X509_SAFE_SNPRINTF;
            }
            cur = cur->next;
            continue;
        }

        switch (san.type) {
            /*
             * otherName
             */
            case MBEDTLS_X509_SAN_OTHER_NAME:
            {
                mbedtls_x509_san_other_name *other_name = &san.san.other_name;

                ret = mbedtls_snprintf(p, n, "\n%s    otherName :", prefix);
                MBEDTLS_X509_SAFE_SNPRINTF;

                if (MBEDTLS_OID_CMP(MBEDTLS_OID_ON_HW_MODULE_NAME,
                                    &other_name->type_id) == 0) {
                    ret = mbedtls_snprintf(p, n, "\n%s        hardware module name :", prefix);
                    MBEDTLS_X509_SAFE_SNPRINTF;
                    ret =
                        mbedtls_snprintf(p, n, "\n%s            hardware type          : ", prefix);
                    MBEDTLS_X509_SAFE_SNPRINTF;

                    ret = mbedtls_oid_get_numeric_string(p,
                                                         n,
                                                         &other_name->value.hardware_module_name.oid, diagnostics);
                    MBEDTLS_X509_SAFE_SNPRINTF;

                    ret =
                        mbedtls_snprintf(p, n, "\n%s            hardware serial number : ", prefix);
                    MBEDTLS_X509_SAFE_SNPRINTF;

                    for (i = 0; i < other_name->value.hardware_module_name.val.len; i++) {
                        ret = mbedtls_snprintf(p,
                                               n,
                                               "%02X",
                                               other_name->value.hardware_module_name.val.p[i]);
                        MBEDTLS_X509_SAFE_SNPRINTF;
                    }
                }/* MBEDTLS_OID_ON_HW_MODULE_NAME */
            }
            break;
            /*
             * uniformResourceIdentifier
             */
            case MBEDTLS_X509_SAN_UNIFORM_RESOURCE_IDENTIFIER:
            {
                ret = mbedtls_snprintf(p, n, "\n%s    uniformResourceIdentifier : ", prefix);
                MBEDTLS_X509_SAFE_SNPRINTF;
                if (san.san.unstructured_name.len >= n) {
                    if (n > 0) {
                        *p = '\0';
                    }
                    MINTLS_RETURN_ERROR(MBEDTLS_ERR_X509_BUFFER_TOO_SMALL);
                }

                memcpy(p, san.san.unstructured_name.p, san.san.unstructured_name.len);
                p += san.san.unstructured_name.len;
                n -= san.san.unstructured_name.len;
            }
            break;
            /*
             * dNSName
             * RFC822 Name
             */
            case MBEDTLS_X509_SAN_DNS_NAME:
            case MBEDTLS_X509_SAN_RFC822_NAME:
            {
                const char *dns_name = "dNSName";
                const char *rfc822_name = "rfc822Name";

                ret = mbedtls_snprintf(p, n,
                                       "\n%s    %s : ",
                                       prefix,
                                       san.type ==
                                       MBEDTLS_X509_SAN_DNS_NAME ? dns_name : rfc822_name);
                MBEDTLS_X509_SAFE_SNPRINTF;
                if (san.san.unstructured_name.len >= n) {
                    if (n > 0) {
                        *p = '\0';
                    }
                    MINTLS_RETURN_ERROR(MBEDTLS_ERR_X509_BUFFER_TOO_SMALL);
                }

                memcpy(p, san.san.unstructured_name.p, san.san.unstructured_name.len);
                p += san.san.unstructured_name.len;
                n -= san.san.unstructured_name.len;
            }
            break;
            /*
             * iPAddress
             */
            case MBEDTLS_X509_SAN_IP_ADDRESS:
            {
                ret = mbedtls_snprintf(p, n, "\n%s    %s : ",
                                       prefix, "iPAddress");
                MBEDTLS_X509_SAFE_SNPRINTF;
                if (san.san.unstructured_name.len >= n) {
                    if (n > 0) {
                        *p = '\0';
                    }
                    MINTLS_RETURN_ERROR(MBEDTLS_ERR_X509_BUFFER_TOO_SMALL);
                }

                unsigned char *ip = san.san.unstructured_name.p;
                // Only IPv6 (16 bytes) and IPv4 (4 bytes) types are supported
                if (san.san.unstructured_name.len == 4) {
                    ret = mbedtls_snprintf(p, n, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
                    MBEDTLS_X509_SAFE_SNPRINTF;
                } else if (san.san.unstructured_name.len == 16) {
                    ret = mbedtls_snprintf(p, n,
                                           "%X%X:%X%X:%X%X:%X%X:%X%X:%X%X:%X%X:%X%X",
                                           ip[0], ip[1], ip[2], ip[3], ip[4], ip[5], ip[6],
                                           ip[7], ip[8], ip[9], ip[10], ip[11], ip[12], ip[13],
                                           ip[14], ip[15]);
                    MBEDTLS_X509_SAFE_SNPRINTF;
                } else {
                    if (n > 0) {
                        *p = '\0';
                    }
                    MINTLS_RETURN_ERROR(MBEDTLS_ERR_X509_BAD_INPUT_DATA);
                }
            }
            break;
            /*
             * directoryName
             */
            case MBEDTLS_X509_SAN_DIRECTORY_NAME:
            {
                ret = mbedtls_snprintf(p, n, "\n%s    directoryName : ", prefix);
                if (ret < 0 || (size_t) ret >= n) {
                    mbedtls_x509_free_subject_alt_name(&san);
                }

                MBEDTLS_X509_SAFE_SNPRINTF;
                ret = mbedtls_x509_dn_gets(p, n, &san.san.directory_name, diagnostics);

                if (ret < 0) {
                    mbedtls_x509_free_subject_alt_name(&san);
                    if (n > 0) {
                        *p = '\0';
                    }
                    MINTLS_RETURN(ret);
                }

                p += ret;
                n -= ret;
            }
            break;
            /*
             * Type not supported, skip item.
             */
            default:
                ret = mbedtls_snprintf(p, n, "\n%s    <unsupported>", prefix);
                MBEDTLS_X509_SAFE_SNPRINTF;
                break;
        }

        /* So far memory is freed only in the case of directoryName
         * parsing succeeding, as mbedtls_x509_get_name allocates memory. */
        mbedtls_x509_free_subject_alt_name(&san);
        cur = cur->next;
    }

    *p = '\0';

    *size = n;
    *buf = p;

    MINTLS_RETURN(0);
}

#define PRINT_ITEM(i)                                   \
    do {                                                \
        ret = mbedtls_snprintf(p, n, "%s" i, sep);      \
        MBEDTLS_X509_SAFE_SNPRINTF;                     \
        sep = ", ";                                     \
    } while (0)

#define CERT_TYPE(type, name)                           \
    do {                                                \
        if (ns_cert_type & (type)) {                    \
            PRINT_ITEM(name);                           \
        }                                               \
    } while (0)

int mbedtls_x509_info_cert_type(char **buf, size_t *size,
                                unsigned char ns_cert_type, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    size_t n = *size;
    char *p = *buf;
    const char *sep = "";

    CERT_TYPE(MBEDTLS_X509_NS_CERT_TYPE_SSL_CLIENT,         "SSL Client");
    CERT_TYPE(MBEDTLS_X509_NS_CERT_TYPE_SSL_SERVER,         "SSL Server");
    CERT_TYPE(MBEDTLS_X509_NS_CERT_TYPE_EMAIL,              "Email");
    CERT_TYPE(MBEDTLS_X509_NS_CERT_TYPE_OBJECT_SIGNING,     "Object Signing");
    CERT_TYPE(MBEDTLS_X509_NS_CERT_TYPE_RESERVED,           "Reserved");
    CERT_TYPE(MBEDTLS_X509_NS_CERT_TYPE_SSL_CA,             "SSL CA");
    CERT_TYPE(MBEDTLS_X509_NS_CERT_TYPE_EMAIL_CA,           "Email CA");
    CERT_TYPE(MBEDTLS_X509_NS_CERT_TYPE_OBJECT_SIGNING_CA,  "Object Signing CA");

    *size = n;
    *buf = p;

    MINTLS_RETURN(0);
}

#define KEY_USAGE(code, name)       \
    do {                            \
        if ((key_usage) & (code)) { \
            PRINT_ITEM(name);       \
        }                           \
    } while (0)

int mbedtls_x509_info_key_usage(char **buf, size_t *size,
                                unsigned int key_usage, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    size_t n = *size;
    char *p = *buf;
    const char *sep = "";

    KEY_USAGE(MBEDTLS_X509_KU_DIGITAL_SIGNATURE,    "Digital Signature");
    KEY_USAGE(MBEDTLS_X509_KU_NON_REPUDIATION,      "Non Repudiation");
    KEY_USAGE(MBEDTLS_X509_KU_KEY_ENCIPHERMENT,     "Key Encipherment");
    KEY_USAGE(MBEDTLS_X509_KU_DATA_ENCIPHERMENT,    "Data Encipherment");
    KEY_USAGE(MBEDTLS_X509_KU_KEY_AGREEMENT,        "Key Agreement");
    KEY_USAGE(MBEDTLS_X509_KU_KEY_CERT_SIGN,        "Key Cert Sign");
    KEY_USAGE(MBEDTLS_X509_KU_CRL_SIGN,             "CRL Sign");
    KEY_USAGE(MBEDTLS_X509_KU_ENCIPHER_ONLY,        "Encipher Only");
    KEY_USAGE(MBEDTLS_X509_KU_DECIPHER_ONLY,        "Decipher Only");

    *size = n;
    *buf = p;

    MINTLS_RETURN(0);
}
#endif /* MBEDTLS_X509_REMOVE_INFO */
