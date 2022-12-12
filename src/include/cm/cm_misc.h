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
 * cm_misc.h
 *
 *
 * IDENTIFICATION
 *    include/cm/cm_misc.h
 *
 * -------------------------------------------------------------------------
 */
#ifndef CM_MISC_H
#define CM_MISC_H

#include "common/config/cm_config.h"
#include <vector>
#include "cm_misc_base.h"
#include "c.h"
#include "cm_defs.h"
#include "cm/cs_ssl.h"
#include "cm/stringinfo.h"
#include "cm_msg.h"

using namespace std;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ResStatusCheckInfoSt {
    long checkTime;
    long startTime;
    uint32 startCount;

    uint32 checkInterval;
    uint32 timeOut;
    uint32 restartDelay;
    uint32 restartPeriod;
    uint32 restartTimes;
} ResStatusCheckInfo;

typedef struct CmResConfListSt {
    char resName[CM_MAX_RES_NAME];
    char script[MAX_PATH_LEN];
    uint32 nodeId;
    uint32 cmInstanceId;
    uint32 resInstanceId;
    uint32 isWorkMember;
    ResStatusCheckInfo checkInfo;
} CmResConfList;

typedef struct st_conn_option {
    int connect_timeout; /* ms */
    int socket_timeout;  /* ms */
    ssl_config_t ssl_para;
    uint8 enable_ssl;
    uint32 verify_peer;
    uint32 expire_time;
} conn_option_t;

typedef struct instance_not_exist_reason_string {
    const char* level_string;
    int level_val;
} instance_not_exist_reason_string;

typedef struct log_level_string {
    const char* level_string;
    int level_val;
} log_level_string;

typedef struct instance_datanode_build_reason_string {
    const char* reason_string;
    int reason_val;
} instance_datanode_build_reason_string;

typedef struct instacne_type_string {
    const char* type_string;
    int type_val;
} instacne_type_string;

typedef struct gtm_con_string {
    const char* con_string;
    int con_val;
} gtm_con_string;

typedef struct instance_coordinator_active_status_string {
    const char* active_status_string;
    int active_status_val;
} instance_coordinator_active_status_string;

typedef struct instance_datanode_lockmode_string {
    const char* lockmode_string;
    uint32 lockmode_val;
} instance_datanode_lockmode_string;

typedef struct instacne_datanode_role_string {
    const char* role_string;
    uint32 role_val;
} instacne_datanode_role_string;

typedef struct instacne_datanode_dbstate_string {
    const char* dbstate_string;
    int dbstate_val;
} instacne_datanode_dbstate_string;

typedef struct instacne_datanode_wal_send_state_string {
    const char* wal_send_state_string;
    int wal_send_state_val;
} instacne_datanode_wal_send_state_string;

typedef struct instacne_datanode_sync_state_string {
    const char* wal_sync_state_string;
    int wal_sync_state_val;
} instacne_datanode_sync_state_string;

typedef struct cluster_state_string {
    const char* cluster_state_string;
    int cluster_state_val;
} cluster_state_string;

typedef struct cluster_msg_string {
    const char* cluster_msg_str;
    int cluster_msg_val;
} cluster_msg_string;

typedef struct ObsBackupStatusMapString_t {
    const char *obsStatusStr;
    int backupStatus;
} ObsBackupStatusMapString;

typedef struct server_role_string {
    int role_val;
    const char* role_string;
} server_role_string;

typedef struct DbStateRoleString_t {
    int roleVal;
    char roleString;
} DbStateRoleString;

typedef enum {
    CM_CLUSTER_UNKNOWN = 0,
    CM_INNER_CENTRALIZE = 1,
    CM_INNER_DISTRIBUTE = 2,
    CM_SELF_DEFINED_ONLY = 4,
    CM_INNER_CENTRALIZE_AND_DEFINED = 5,
    CM_INNER_DISTRIBUTE_AND_DEFINED = 6
} CmClusterWorkMode;

extern CmClusterWorkMode g_cluster_work_mode;

#define HAS_RES_DEFINED (g_cluster_work_mode >= 4)
#define HAS_RES_DEFINED_ONLY (g_cluster_work_mode == 4)
#define HAS_RES_DN (g_cluster_work_mode != CM_SELF_DEFINED_ONLY)

#define LOCK_FILE_LINE_PID 1
#define LOCK_FILE_LINE_DATA_DIR 2
#define LOCK_FILE_LINE_START_TIME 3
#define LOCK_FILE_LINE_PORT 4
#define LOCK_FILE_LINE_SOCKET_DIR 5
#define LOCK_FILE_LINE_LISTEN_ADDR 6
#define LOCK_FILE_LINE_SHMEM_KEY 7

#ifndef ERROR_LIMIT_LEN
#define ERROR_LIMIT_LEN 256
#endif

typedef struct ResourceListInfo {
    char resName[NAMEDATALEN];
    uint32 nodeId;
    uint32 cmInstanceId;
    uint32 resInstanceId;
    char scriptPath[MAX_PATH_LEN];
    uint32 checkInterval;
    uint32 timeOut;
} ResourceListInfo;

// instance type before INST_TYPE_UNKNOWN shouldn't be change
typedef enum {
    INST_TYPE_INIT = 0,
    INST_TYPE_CMSERVER = 1,
    INST_TYPE_GTM = 2,
    INST_TYPE_DN = 3,
    INST_TYPE_CN = 4,
    INST_TYPE_FENCED_UDF = 5,
} InstanceType;

typedef struct Instance_t {
    uint32 node;
    InstanceType instType;
    union {
        dataNodeInfo *dnInst;
        staticNodeConfig *InstNode; // now, all inst info except dn is record on staticNodeConfig...
    };
} Instance;

extern vector<CmResStatList> g_resStatus;
extern vector<CmResConfList> g_resConf;
extern conn_option_t g_sslOption;

/**
 * @def SHELL_RETURN_CODE
 * @brief Get the shell command return code.
 * @return Return the shell command return code.
 */
#define SHELL_RETURN_CODE(systemReturn) \
    ((systemReturn) > 0 ? static_cast<int>(static_cast<uint32>(systemReturn) >> 8) : (systemReturn))

extern char** CmReadfile(const char* path);
extern void freefile(char** lines);
extern int log_level_string_to_int(const char* log_level);
extern int datanode_rebuild_reason_string_to_int(const char* reason);
extern const char* DcfRoleToString(int role);
extern const char* instance_not_exist_reason_to_string(int reason);
extern int datanode_lockmode_string_to_int(const char* lockmode);
extern int datanode_role_string_to_int(const char* role);
extern int datanode_dbstate_string_to_int(const char* dbstate);
extern int datanode_wal_send_state_string_to_int(const char* dbstate);
extern int datanode_wal_sync_state_string_to_int(const char* dbstate);
extern const char* log_level_int_to_string(int log_level);
extern const char* cluster_state_int_to_string(int cluster_state);
extern const char* cluster_msg_int_to_string(int cluster_msg);
extern int32 ObsStatusStr2Int(const char *statusStr);
extern const char* datanode_wal_sync_state_int_to_string(int dbstate);
extern const char* datanode_wal_send_state_int_to_string(int dbstate);

extern const char* datanode_dbstate_int_to_string(int dbstate);
extern const char* type_int_to_string(int type);
const char* gtm_con_int_to_string(int con);
extern const char* datanode_role_int_to_string(int role);
extern const char* datanode_static_role_int_to_string(uint32 role);
extern const char* datanode_rebuild_reason_int_to_string(int reason);
extern const char* server_role_to_string(int role);
extern const char* etcd_role_to_string(int role);
extern const char* kerberos_status_to_string(int role);
const char *DatanodeLockmodeIntToString(uint32 lockmode);

extern void print_environ(void);

extern const char* CmGetmsgtype(const CM_StringInfo msg, int datalen);
extern const char* CmGetmsgbytes(CM_StringInfo msg, int datalen);
extern const char* CmGetmsgbytesPtr(const CM_Result *msg, int datalen);

/**
 * @brief
 *  Creates a lock file for a process with a specified PID.
 *
 * @note
 *  When pid is set to -1, the specified process is the current process.
 *
 * @param [in] filename
 *  The name of the lockfile to create.
 * @param [in] data_path
 *  The data path of the instance.
 * @param [in] pid
 *  The pid of the process.
 *
 * @return
 *  - 0     Create successfully.
 *  - -1    Create failure.
 */
extern int create_lock_file(const char* filename, const char* data_path, const pid_t pid = -1);


/**
 * @brief
 *  Delete pid file.
 *
 * @param [in] filename
 *  The pid file to be deleted.
 *
 * @return
 *  void.
 */
extern void delete_lock_file(const char *filename);

extern void cm_pthread_rw_lock(pthread_rwlock_t* rwlock);
extern void cm_pthread_rw_unlock(pthread_rwlock_t* rwlock);
extern status_t ReadResourceDefConfig(bool isAgent);
extern int CmSSlConfigInit(bool is_client);
void GetRealFile(char *realFile, uint32 fileLen, const char *path);
#ifdef __cplusplus
}
#endif
status_t GetGlobalResStatusIndex(const char *resName, uint32 &index);
void GetAlarmConfig(const char *confDir);
int32 GetDbStaticRoleInt(char c);
char GetDbStaticRoleStr(int32 role);
status_t GetAllResConf(const char *confData, const char *confDir, bool isAgent);

#define RANDOM_LEN 16

#define SERVER_KEY_RAND_FILE       "server.key.rand"
#define SERVER_KEY_CIPHER_FILE     "server.key.cipher"
#define CLIENT_KEY_RAND_FILE       "client.key.rand"
#define CLIENT_KEY_CIPHER_FILE     "client.key.cipher"
#define HADR_KEY_RAND_FILE       "hadr.key.rand"
#define HADR_KEY_CIPHER_FILE     "hadr.key.cipher"

int CmAtoi(const char *str, int defaultValue);
bool CmAtoBool(const char *str);
long CmAtol(const char *str, int defaultValue);

#endif
