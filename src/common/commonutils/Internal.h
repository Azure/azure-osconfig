// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef INTERNAL_H
#define INTERNAL_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#define _POSIX_C_SOURCE 200809L   /* enable open(), O_CREAT, etc. */
#include <unistd.h>               /* declares open(), close(), unlink(), etc. */
#include <fcntl.h>                /* declares O_CREAT, O_EXCL, O_WRONLY, mode flags */
#include <sys/stat.h>             /* declares S_IRUSR, S_IWUSR, S_IRGRP, etc. */

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
#include <shadow.h>
#include <parson.h>
#include <Logging.h>
#include <Reasons.h>
#include <CommonUtils.h>

#include "../asb/Asb.h"

#if ((__GLIBC__ == 2) && (__GLIBC_MINOR__ < 30))
#include <sys/syscall.h>
#define gettid() syscall(SYS_gettid)
#endif

#define INT_ENOENT -999

#define MAX_STRING_LENGTH 512

#endif // INTERNAL_H
