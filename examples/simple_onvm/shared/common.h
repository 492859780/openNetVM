/*********************************************************************
 *                     openNetVM
 *       https://github.com/sdnfv/openNetVM
 *
 *  Copyright 2015 George Washington University
 *            2015 University of California Riverside
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * common.h - shared data between host and NFs
 ********************************************************************/

#ifndef _COMMON_H_
#define _COMMON_H_

#define MAX_CLIENTS             16

#define ONVM_NF_ACTION_DROP 0 // drop packet
#define ONVM_NF_ACTION_NEXT 1 // to whatever the next action is configured by the SDN controller in the flow table
#define ONVM_NF_ACTION_TONF 2 // send to the NF specified in the argument field (assume it is on the same host)
#define ONVM_NF_ACTION_OUT 3  // send the packet out the NIC port set in the argument field

/*
 * Shared port info, including statistics information for display by server.
 * Structure will be put in a memzone.
 * - All port id values share one cache line as this data will be read-only
 * during operation.
 * - All rx statistic values share cache lines, as this data is written only
 * by the server process. (rare reads by stats display)
 * - The tx statistics have values for all ports per cache line, but the stats
 * themselves are written by the clients, so we have a distinct set, on different
 * cache lines for each client to use.
 */
struct rx_stats{
        uint64_t rx[RTE_MAX_ETHPORTS];
} __rte_cache_aligned;

struct tx_stats{
        uint64_t tx[RTE_MAX_ETHPORTS];
        uint64_t tx_drop[RTE_MAX_ETHPORTS];
} __rte_cache_aligned;

struct port_info {
        uint8_t num_ports;
        uint8_t id[RTE_MAX_ETHPORTS];
        volatile struct rx_stats rx_stats;
        volatile struct tx_stats tx_stats[MAX_CLIENTS];
};

struct onvm_pkt_action {
        uint8_t action;
        uint16_t destination;
};

/* define common names for structures shared between server and client */
#define MP_CLIENT_RXQ_NAME "MProc_Client_%u_RX"
#define MP_CLIENT_TXQ_NAME "MProc_Client_%u_TX"
#define PKTMBUF_POOL_NAME "MProc_pktmbuf_pool"
#define MZ_PORT_INFO "MProc_port_info"

/*
 * Given the rx queue name template above, get the queue name
 */
static inline const char *
get_rx_queue_name(unsigned id)
{
        /* buffer for return value. Size calculated by %u being replaced
         * by maximum 3 digits (plus an extra byte for safety) */
        static char buffer[sizeof(MP_CLIENT_RXQ_NAME) + 2];

        snprintf(buffer, sizeof(buffer) - 1, MP_CLIENT_RXQ_NAME, id);
        return buffer;
}

/*
 * Given the tx queue name template above, get the queue name
 */
static inline const char *
get_tx_queue_name(unsigned id)
{
        /* buffer for return value. Size calculated by %u being replaced
         * by maximum 3 digits (plus an extra byte for safety) */
        static char buffer[sizeof(MP_CLIENT_TXQ_NAME) + 2];

        snprintf(buffer, sizeof(buffer) - 1, MP_CLIENT_TXQ_NAME, id);
        return buffer;
}

#define RTE_LOGTYPE_APP RTE_LOGTYPE_USER1

#endif
