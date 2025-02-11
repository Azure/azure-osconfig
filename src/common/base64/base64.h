/*
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root.
 */
#ifndef BASE64_H
#define BASE64_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define Base64Length(l) ((l == 0) ? (1) : (((((l - 1) / 3) + 1) * 4) + 1))

int
Base64Encode(
    const unsigned char *Input,
    uint32_t             Length,
    char                *Output,
    uint32_t            *OutLen
);

int
Base64Decode(
    const char      *Input,
    unsigned char   *Output,
    uint32_t        *OutLen
);

#ifdef __cplusplus
}
#endif

#endif
