// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef INTERNAL_H
#define INTERNAL_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <mntent.h>
#include <dirent.h>
#include <math.h>
#include <libgen.h>
#include <parson.h>
#include <Logging.h>
#include <CommonUtils.h>

#include "../asb/Asb.h"

#if ((__GLIBC__ == 2) && (__GLIBC_MINOR__ < 30))
#include <sys/syscall.h>
#define gettid() syscall(SYS_gettid)
#endif

#define PLAIN_STATUS_FROM_ERRNO(a) ((0 == a) ? "passed" : "failed")

#define INT_ENOENT -999

#define MAX_STRING_LENGTH 512

#endif // INTERNAL_H
