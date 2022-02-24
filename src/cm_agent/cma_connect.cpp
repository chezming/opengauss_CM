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
 * cma_connect.cpp
 *
 *
 * IDENTIFICATION
 *    src/cm_agent/cma_connect.cpp
 *
 * -------------------------------------------------------------------------
 */
#include <time.h>
#include "cma_global_params.h"
#include "cma_connect.h"
#include "cm/cs_ssl.h"
#ifdef ENABLE_MULTIPLE_NODES
#include "cma_coordinator.h"
#include "cma_coordinator_utils.h"
#endif

CM_Conn* agent_cm_server_connect = NULL;

int g_connCmServerTimes = 0;
extern uint32 g_serverNodeId;

status_t cm_client_send_msg(CM_Conn *conn, char msgtype, const char *s, size_t lenmsg)
{
    int ret;

    if (conn == NULL) {
        write_runlog(ERROR, "cm_client_send_msg, conn is null\n");
        return CM_ERROR;
    }

    ret = CMPQPacketSend(conn, msgtype, s, lenmsg);
    if (ret != STATUS_OK) {
        write_runlog(LOG, "pqPacketSend failed! ret=%d, errmsg: %s\n", ret, conn->errorMessage.data);
        return CM_ERROR;
    }
    return CM_SUCCESS;
}

static status_t GetSslRequestAck(char *receiveMsg, bool *enableSsl)
{
    cm_msg_type *cm_msg_type_ptr = NULL;

    cm_msg_type_ptr = (cm_msg_type *)receiveMsg;
    if (cm_msg_type_ptr->msg_type != MSG_CM_SSL_CONN_ACK) {
        write_runlog(ERROR, "fail to get ssl ack msg_type=%d errno=%d.\n", cm_msg_type_ptr->msg_type, errno);
        return CM_ERROR;
    }
    CmToAgentConnectAck *msgAck = (CmToAgentConnectAck *)(receiveMsg);
    write_runlog(LOG, "(client) MSG_CM_SSL_CONN_ACK receive ssl require msg %u \n", msgAck->status);
    if (msgAck->status == 1) {
        *enableSsl = 1;
        return CM_SUCCESS;
    } else if (msgAck->status == 2) {
        *enableSsl = 0;
        return CM_SUCCESS;
    }
    return CM_ERROR;
}

static char* RecvSslRequestAck(CM_Conn* conn)
{
    CM_Result* res = NULL;

    if (conn == NULL) {
        write_runlog(ERROR, "[RecvSslRequestAck] cma is not connect to the cm server \n");
        return NULL;
    }

    if (cmpqReadData(conn) < 0) {
        return NULL;
    }

    if ((res = cmpqGetResult(conn)) == NULL) {
        return NULL;
    }
    return (char*)&(res->gr_resdata);
}

static status_t conn_ssl_requst(CM_Conn *conn, int ssl_req, bool *enableSsl)
{
    AgentToCmConnectRequest req_msg;
    req_msg.msg_type = ssl_req;
    req_msg.nodeid = g_nodeId;
    const int waitAckTime = 20;
    int timeOut = waitAckTime;

    if (cm_client_send_msg(conn, 'C', (const char *)&req_msg, sizeof(AgentToCmConnectRequest)) != 0) {
        return CM_ERROR;
    }

    char *receiveMsg = NULL;
    write_runlog(DEBUG5, "GetSslRequestAck start.\n");
    while (timeOut >= 0) {
        if (cm_client_flush_msg(conn) != 0) {
            return CM_ERROR;
        }

        receiveMsg = RecvSslRequestAck(conn);
        if (receiveMsg != NULL) {
            if (GetSslRequestAck(receiveMsg, enableSsl) != CM_SUCCESS) {
                continue;
            }
            write_runlog(DEBUG5, "GetSslRequestAck end %d\n", *enableSsl);
            return CM_SUCCESS;
        }

        timeOut--;
        cm_usleep(AGENT_RECV_CYCLE);
    }

    return CM_ERROR;
}

static inline void cs_securec_clear(char *content, uint32 len)
{
    if (!CM_IS_EMPTY_STR(content)) {
        errno_t rc = memset_s(content, len, 0, len);
        securec_check_errno(rc, (void)rc);
    }
    return;
}

static status_t conn_ssl_establish(CM_Conn *conn, conn_option_t *option, bool *enableSsl)
{
    const uint32 plainLen = CM_PASSWD_MAX_LEN + 1;
    char plain[plainLen] = {0};

    CM_RETURN_IFERR(conn_ssl_requst(conn, MSG_CM_SSL_CONN_REQUEST, enableSsl));
    write_runlog(DEBUG5, "conn_ssl_requst %d\n", *enableSsl);

    if (*enableSsl == CM_FALSE) {
        return CM_SUCCESS;
    }

    write_runlog(LOG, "begin to create ssl connection\n");
    CM_RETURN_IFERR(cm_verify_ssl_key_pwd(plain, plainLen, CLIENT_CIPHER));
    g_sslOption.ssl_para.key_password = plain;
    g_sslOption.ssl_para.verify_peer = 1;

    ssl_ctx_t *ssl_fd = NULL;
    /* check certificate file access permission */
    if (strlen(option->ssl_para.ca_file) > 0) {
        CM_RETURN_IFERR_EX(cm_ssl_verify_file_stat(option->ssl_para.ca_file),
            cs_securec_clear(plain, plainLen));
    }
    if (strlen(option->ssl_para.key_file) > 0) {
        CM_RETURN_IFERR_EX(cm_ssl_verify_file_stat(option->ssl_para.key_file),
            cs_securec_clear(plain, plainLen));
    }
    if (strlen(option->ssl_para.cert_file) > 0) {
        CM_RETURN_IFERR_EX(cm_ssl_verify_file_stat(option->ssl_para.cert_file),
            cs_securec_clear(plain, plainLen));
    }

    if (!cm_file_exist((const char*)g_sslOption.ssl_para.crl_file)) {
        free(g_sslOption.ssl_para.crl_file);
        g_sslOption.ssl_para.crl_file = NULL;
    }

    /* create the ssl connector - init ssl and load certs */
    ssl_fd = cm_ssl_create_connector_fd(&option->ssl_para);

    /* erase key_password for security issue */
    cs_securec_clear(plain, plainLen);

    if (ssl_fd == NULL) {
        write_runlog(ERROR, "sl_create_connector_fd failed.\n");
        return CM_ERROR;
    }
    conn->ssl_connector_fd = ssl_fd;

    /* connect to the server */
    if (cm_cs_ssl_connect(ssl_fd, &conn->pipe) != CM_SUCCESS) {
        write_runlog(ERROR, "create ssl connection failed\n");
        return CM_ERROR;
    }

    conn->status= CONNECTION_OK;
    return CM_SUCCESS;
}

status_t TryGetSslConnToCmserver(CM_Conn *conn, int timeOut)
{
    const uint32 upgradeVersion = 92574;
    if (undocumentedVersion != 0 && undocumentedVersion < upgradeVersion) {
        return CM_SUCCESS;
    }
    conn->pipe.link.tcp.sock = conn->sock;
    conn->pipe.link.tcp.closed = CM_FALSE;
    conn->pipe.link.tcp.remote = *(sock_addr_t *)&conn->raddr;
    conn->pipe.link.tcp.local = *(sock_addr_t *)&conn->laddr;
    conn->pipe.connect_timeout = timeOut * 1000;
    conn->pipe.socket_timeout = 3 * 1000;
    conn->pipe.l_onoff = 1;
    conn->pipe.l_linger = 1;
    conn->pipe.type = CS_TYPE_TCP;
    conn->status = CONNECTION_SSL_STARTUP;
    bool enableSsl = CM_FALSE;
    if (conn_ssl_establish(conn, &g_sslOption, &enableSsl) != CM_SUCCESS) {
        write_runlog(ERROR, "create ssl connection failed.\n");
        return CM_ERROR;
    }

    if (enableSsl) {
        write_runlog(LOG, "create ssl connection success.\n");
    } else {
        write_runlog(LOG, "ssl connection not enable.\n");
        conn->status = CONNECTION_OK;
    }

    return CM_SUCCESS;
}

CM_Conn* GetConnToCmserver(uint32 nodeid)
{
    CM_Conn* conn = NULL;
    uint32 ii = 0;
    uint32 jj = 0;
    char connstr[3][CONNSTR_LEN];
    int rc;
    int rcs;

    uint32 tmpCmserverIndex[CM_PRIMARY_STANDBY_NUM] = {INVALID_NODE_NUM};

    long primaryNodeId = -1;

    /* put the primary index to the first of the cmserver index array. */
    for (uint32 i = 0; i < g_cm_server_num; i++) {
        uint32 cm_server_node_index = g_nodeIndexForCmServer[i];
        if (primaryNodeId != -1 && (long)g_node[cm_server_node_index].node == primaryNodeId) {
            tmpCmserverIndex[i] = tmpCmserverIndex[0];
            tmpCmserverIndex[0] = cm_server_node_index;
        } else {
            tmpCmserverIndex[i] = g_nodeIndexForCmServer[i];
        }
    }

    if (nodeid == 0) {
        g_connCmServerTimes++;
    }

    g_cmaConnectCmsInOtherAzCount = 0;
    g_cmaConnectCmsPrimaryInLocalNodeCount = 0;
    g_cmaConnectCmsInOtherNodeCount = 0;
    g_cmaConnectCmsPrimaryInLocalAzCount = 0;

    for (int kk = (g_cm_server_num - 1); kk >= 0; kk--) {
        uint32 cm_server_node_index = tmpCmserverIndex[kk];

        if (cm_server_node_index < g_node_num) {
            /* try to conn */
            for (ii = 0; ii < g_node[cm_server_node_index].cmServerListenCount; ii++) {
                int connTimeOut = (int)agent_connect_timeout;
                if ((g_cm_server_num == 2 || kk == 0) && g_connCmServerTimes < MAX_PRE_CONN_CMS &&
                    connTimeOut < MAX_CONN_TIMEOUT) {
                    connTimeOut = MAX_CONN_TIMEOUT;
                }
                for (jj = 0; jj < g_currentNode->cmAgentListenCount; jj++) {
                    rc = memset_s(connstr[jj], CONNSTR_LEN, 0, CONNSTR_LEN);
                    securec_check_errno(rc, (void)rc);
                    rcs = snprintf_s(connstr[jj], sizeof(connstr[jj]), sizeof(connstr[jj]) - 1,
                        "host=%s port=%u localhost=%s connect_timeout=%d node_id=%u node_name=%s remote_type=%d",
                        g_node[cm_server_node_index].cmServer[ii],
                        g_node[cm_server_node_index].port,
                        g_currentNode->cmAgentIP[jj],
                        connTimeOut,
                        (nodeid == 0) ? g_currentNode->node : nodeid,
                        g_currentNode->nodeName,
                        CM_AGENT);
                    securec_check_intval(rcs, (void)rcs);

                    if ((conn = PQconnectCM(connstr[jj])) != NULL && (CMPQstatus(conn) == CONNECTION_OK)) {
                        write_runlog(DEBUG5, "socket is [%d]. try to request ssl connection: %s\n",
                            conn->sock, connstr[jj]);
                        if (TryGetSslConnToCmserver(conn, connTimeOut) != CM_SUCCESS) {
                            CMPQfinish(conn);
                            conn = NULL;
                            return NULL;
                        }

                        if (nodeid == 0) {
                            write_runlog(
                                LOG, "cm_agent connect to cm_server primary successfully: %s\n", connstr[jj]);
                            g_connCmServerTimes = 0;
                        }
                        /* If cm_agent successfully connect to cm_server primary, we count the number 
                         of connection according to whether the cm_server and cm_agent is in the same node */
                        if (g_currentNode->node == g_node[cm_server_node_index].node) {
                            g_cmaConnectCmsPrimaryInLocalNodeCount++;
                        } else {
                            g_cmaConnectCmsInOtherNodeCount++;
                        }
                        /* count the number of connection according to whether the cm_server and cm_agent 
                            is in the same az */
                        if (strcmp(g_currentNode->azName, g_node[cm_server_node_index].azName) == 0) {
                            g_cmaConnectCmsPrimaryInLocalAzCount++;
                        } else {
                            g_cmaConnectCmsInOtherAzCount++;
                        }
                        /* record server id */
                        g_serverNodeId = g_node[cm_server_node_index].node;
                        return conn;
                    } else {
                        if (strcmp(CMPQerrorMessage(conn), "invalid host") == 0) {
                            if (g_syncDroppedCoordinator == false) {
                                write_runlog(LOG, "%d: sync_dropped_coordinator changes to true.\n", __LINE__);
                            }
                            g_syncDroppedCoordinator = true;
                        }
                        if (strcmp(CMPQerrorMessage(conn), "local cmserver is not the primary") == 0) {
                            write_runlog(LOG, "cm_agent connect to cm_server standy successfully.\n");
                            if (g_currentNode->node != g_node[cm_server_node_index].node) {
                                g_cmaConnectCmsInOtherNodeCount++;
                            }
                            if (strcmp(g_currentNode->azName, g_node[cm_server_node_index].azName) != 0) {
                                g_cmaConnectCmsInOtherAzCount++;
                            }
                        }
                        if (nodeid == 0) {
                            if (strcmp(CMPQerrorMessage(conn), "local cmserver is not the primary") == 0) {
                                write_runlog(LOG, "cm_agent connect to cm_server standy successfully\n");
                            } else {
                                write_runlog(ERROR, "%d: connect to cm_server failed, %s. %s\n",
                                    __LINE__, connstr[jj], CMPQerrorMessage(conn));
                            }
                        }
                        CMPQfinish(conn);
                        conn = NULL;
                    }
                }
            }
        }

        if (nodeid == 0) {
            write_runlog(ERROR, "connect to cm server failed! The %ust of cm server node id is = %u\n",
                cm_server_node_index, g_node[cm_server_node_index].node);
        }
    }

    return NULL;
}

void CloseConnToCmserver(void)
{
    if (agent_cm_server_connect != NULL) {
        (void)clock_gettime(CLOCK_MONOTONIC, &g_disconnectTime);
        CMPQfinish(agent_cm_server_connect);
        agent_cm_server_connect = NULL;
        write_runlog(LOG, "close agent to cmserver connection.\n");
    }
}

status_t cm_client_flush_msg(CM_Conn* conn)
{
    int ret;
    if (conn != NULL) {
        ret = cmpqFlush(conn);
        if (ret != 0) {
            write_runlog(LOG, "pq_flush return value is %d\n", ret);
            return CM_ERROR;
        }
    }
    return CM_SUCCESS;
}

bool CmaSendMsgToCms(const void *reportMsg, size_t len, const char* msg)
{
    int ret = 0;
    ret = cm_client_send_msg(agent_cm_server_connect, 'C', (char *)reportMsg, len);
    if (ret != 0) {
        write_runlog(ERROR, "cma fail to send %s to cms.\n", msg);
        CloseConnToCmserver();
        return false;
    }
    return true;
}

bool CmaSendSslMsgToCms(CM_Conn *conn, const void *reportMsg, size_t len, const char* msg)
{
    int ret = 0;
    ret = cm_client_send_msg(conn, 'C', (char *)reportMsg, len);
    if (ret != 0) {
        write_runlog(ERROR, "cma fail to send %s to cms.\n", msg);
        CloseConnToCmserver();
        return false;
    }
    return true;
}