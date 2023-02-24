// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"

// etc/group: each line is in the following format:
// group_name:password:GID:user1,user2, ....userN
typedef struct ETC_GROUP_INFO
{
    char* groupName;
    char* groupPassword;
    unsigned int groupId;
    char* groupUsernames[0];
} ETC_GROUP_INFO;

static ETC_GROUP_INFO* g_etcGroupInfo = NULL;
static unsigned int g_numberOfEtcGroupInfos = 0;

// etc/passwd: each line is in the following format:
// username:password:uid:gid:geco1,geco2,..gecoN:home directory:login shell
typedef struct ETC_PASSWD_INFO
{
    char* username;
    char* password;
    unsigned int uid;
    unsigned int gid;
    char* homeDirectory;
    char* loginShell;
} ETC_PASSWD_INFO;

static ETC_PASSWD_INFO* g_etcPasswdInfo = NULL;
static unsigned int g_numberOfEtcPasswdInfos = 0;

// etc/shadow: each line is in the following format:
// login_name:encrypted_password:date_of_last_password_change:minimum_password_age:maximum_password_age:
// password_warning_period:password_inactivity_period:account_expiration_date:reserved_field
typedef struct ETC_SHADOW_INFO
{
    char* loginName;
    char* encryptedPassword; //see crypt(3)
    unsigned int dateOfLastPasswordChange; //number of days since Jan 1, 1970 00:00 UTC
    unsigned int minimumPasswordAge; //number of days
    unsigned int maximumPasswordAge; //number of days
    unsigned int passwordWarningPeriod; //number of days
    unsigned int passwordInactivityPeriod; //number of days
    unsigned int accountExpirationDate; //number of days since Jan 1, 1970 00:00 UTC
} ETC_SHADOW_INFO;

static ETC_SHADOW_INFO* g_etcShadowInfo = NULL;
static unsigned int g_numberOfEtcShadowInfos = 0;