/*
 * Copyright (c) 2021 Huawei Technologies Co.,Ltd.
 *
 * CM is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------
 *
 * cm_misc_base.cpp
 *
 *
 * IDENTIFICATION
 *    src/cm_common/cm_misc_base.cpp
 *
 * -------------------------------------------------------------------------
 */

#include <time.h>
#include <string.h>
#include <signal.h>
#include "cm/cm_elog.h"
#include "cm/cm_misc.h"
#include "cm/cm_misc_base.h"

syscalllock g_cmEnvLock;

static void CmInitBlockSig(sigset_t* sleepBlockSig)
{
#ifdef SIGTRAP
    (void)sigdelset(sleepBlockSig, SIGTRAP);
#endif
#ifdef SIGABRT
    (void)sigdelset(sleepBlockSig, SIGABRT);
#endif
#ifdef SIGILL
    (void)sigdelset(sleepBlockSig, SIGILL);
#endif
#ifdef SIGFPE
    (void)sigdelset(sleepBlockSig, SIGFPE);
#endif
#ifdef SIGSEGV
    (void)sigdelset(sleepBlockSig, SIGSEGV);
#endif
#ifdef SIGBUS
    (void)sigdelset(sleepBlockSig, SIGBUS);
#endif
#ifdef SIGSYS
    (void)sigdelset(sleepBlockSig, SIGSYS);
#endif
}

void cm_sleep(unsigned int sec)
{
    sigset_t sleepBlockSig;
    sigset_t oldSig;
    (void)sigfillset(&sleepBlockSig);

    CmInitBlockSig(&sleepBlockSig);

    (void)sigprocmask(SIG_SETMASK, &sleepBlockSig, &oldSig);

    (void)sleep(sec);

    (void)sigprocmask(SIG_SETMASK, &oldSig, NULL);
}

void cm_usleep(unsigned int usec)
{
    sigset_t sleepBlockSig;
    sigset_t oldSig;
    (void)sigfillset(&sleepBlockSig);

    CmInitBlockSig(&sleepBlockSig);

    (void)sigprocmask(SIG_SETMASK, &sleepBlockSig, &oldSig);

    (void)usleep(usec);

    (void)sigprocmask(SIG_SETMASK, &oldSig, NULL);
}

void CheckEnvValue(const char* inputEnvValue)
{
    int i;
    const char* dangerCharacterList[] = {"|", ";", "&", "$", "<", ">", "`", "\\", "'", "\"", "{", "}", "(", ")", "[",
        "]", "~", "*", "?", "!", "\n", NULL};

    for (i = 0; dangerCharacterList[i] != NULL; i++) {
        if (strstr(inputEnvValue, dangerCharacterList[i]) != NULL) {
            if (logInitFlag) {
                write_runlog(ERROR, "invalid token \"%s\" in inputEnvValue: (%s)\n",
                    dangerCharacterList[i], inputEnvValue);
            } else {
                fprintf(stderr,
                    "invalid token \"%s\" in inputEnvValue: (%s)\n", dangerCharacterList[i], inputEnvValue);
            }
            exit(1);
        }
    }
}

int cm_getenv(const char *envVar, char *outputEnvValue, uint32 envValueLen, int elevel)
{
    char* envValue = NULL;
    elevel = (elevel == -1) ? ERROR : elevel;

    if (envVar == NULL) {
        write_runlog(elevel, "cm_getenv: invalid envVar !\n");
        return -1;
    }

    (void)syscalllockAcquire(&g_cmEnvLock);
    envValue = getenv(envVar);
    if (envValue == NULL || envValue[0] == '\0') {
        if (strcmp(envVar, "MPPDB_KRB5_FILE_PATH") == 0 ||
            strcmp(envVar, "KRB_HOME") == 0 ||
            strcmp(envVar, "MPPDB_ENV_SEPARATE_PATH") == 0) {
            /* MPPDB_KRB5_FILE_PATH, KRB_HOME, MPPDB_ENV_SEPARATE_PATH is not necessary,
            and do not print failed to get environment log */
            (void)syscalllockRelease(&g_cmEnvLock);
            return -1;
        } else {
            write_runlog(elevel,
                         "cm_getenv: failed to get environment variable:%s. Check and make sure it is configured!\n",
                         envVar);
        }
        (void)syscalllockRelease(&g_cmEnvLock);
        return -1;
    }
    CheckEnvValue(envValue);

    int rc = strcpy_s(outputEnvValue, envValueLen, envValue);
    if (rc != EOK) {
        write_runlog(elevel,
                     "cm_getenv: failed to get environment variable %s , variable length:%lu.\n",
                     envVar,
                     strlen(envValue));
        (void)syscalllockRelease(&g_cmEnvLock);
        return -1;
    }

    (void)syscalllockRelease(&g_cmEnvLock);
    return EOK;
}

void check_input_for_security(const char *input)
{
    int i;
    char* dangerToken[] = {"|", ";", "&", "$", "<", ">", "`", "\\", "!", "\n", NULL};

    for (i = 0; dangerToken[i] != NULL; i++) {
        if (strstr(input, dangerToken[i]) != NULL) {
            if (logInitFlag) {
                write_runlog(ERROR, "invalid token \"%s\" in string: %s.\n", dangerToken[i], input);
            } else {
                printf("invalid token \"%s\" in string: %s.\n", dangerToken[i], input);
            }
            exit(1);
        }
    }
}

int GetHomePath(char *outputEnvValue, uint32 envValueLen, int32 logLevel)
{
    errno_t rc = 0;
    rc = cm_getenv("CM_HOME", outputEnvValue, envValueLen, logLevel);
    if (rc == EOK) {
        check_input_for_security(outputEnvValue);
        return 0;
    }
    write_runlog(logLevel, "Line:%d Get CMHOME failed, will try to get GAUSSHOME.\n", __LINE__);

    rc = cm_getenv("GAUSSHOME", outputEnvValue, envValueLen);
    if (rc != EOK) {
        write_runlog(ERROR, "Line:%d Get GAUSSHOME failed, please check.\n", __LINE__);
        return -1;
    }
    check_input_for_security(outputEnvValue);
    return 0;
}

bool IsBoolCmParamTrue(const char *param)
{
    return (strcmp(param, "on") == 0) || (strcmp(param, "yes") == 0) ||
           (strcmp(param, "true") == 0) || (strcmp(param, "1") == 0);
}

bool IsSharedStorageMode()
{
    char env[MAX_PATH_LEN] = {0};

    if (cm_getenv("DORADO_REARRANGE", env, sizeof(env), DEBUG1) != EOK) {
        write_runlog(DEBUG1, "Get DORADO_REARRANGE failed, is not shared storage mode.\n");
        return false;
    }
    write_runlog(LOG, "Get DORADO_REARRANGE success, is shared storage mode.\n");

    return true;
}