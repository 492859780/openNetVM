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
 * loop_latency.c - create pkts and loop through NFs to test the layency.
 * This example refers to dpdk rxtx_callback code.
 ********************************************************************/

#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/queue.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

#include <rte_common.h>
#include <rte_mbuf.h>
#include <rte_ip.h>
#include <rte_mempool.h>
#include <rte_cycles.h>
#include <rte_memcpy.h>

#include "onvm_nflib.h"
#include "onvm_pkt_helper.h"

#define NF_TAG "latecny"

#define NUM_PKTS 128
#define PKTMBUF_POOL_NAME "MProc_pktmbuf_pool"

#define FLAG ((uint32_t)123456789)

/* Struct that contains information about this NF */
struct onvm_nf_info *nf_info;

struct stamp {
	uint32_t flag;
	uint64_t now;
};

static struct {
	uint64_t total_cycles;
	uint64_t total_pkts;
} latency_numbers;

/* number of package between each print */
static uint32_t print_delay = 10000000;
static uint16_t destination;

/*
 * Print a usage message
 */
static void
usage(const char *progname) {
        printf("Usage: %s [EAL args] -- [NF_LIB args] -- -d <destination> -p <print_delay>\n\n", progname);
}

/*
 * Parse the application arguments.
 */
static int
parse_app_args(int argc, char *argv[], const char *progname) {
        int c;

        while ((c = getopt (argc, argv, "d:p:")) != -1) {
                switch (c) {
                case 'd':
                        destination = strtoul(optarg, NULL, 10);
                        break;
                case 'p':
                        print_delay = strtoul(optarg, NULL, 10);
                        break;
                case '?':
                        usage(progname);
                        if (optopt == 'd')
                                RTE_LOG(INFO, APP, "Option -%c requires an argument.\n", optopt);
                        else if (optopt == 'p')
                                RTE_LOG(INFO, APP, "Option -%c requires an argument.\n", optopt);
                        else if (isprint(optopt))
                                RTE_LOG(INFO, APP, "Unknown option `-%c'.\n", optopt);
                        else
                                RTE_LOG(INFO, APP, "Unknown option character `\\x%x'.\n", optopt);
                        return -1;
                default:
                        usage(progname);
                        return -1;
                }
        }
        return optind;
}

/*
 * This function displays stats. It uses ANSI terminal codes to clear
 * screen when called. It is called from a single non-master
 * thread in the server process, when the process is run with more
 * than one lcore enabled.
 */
static void
do_stats_display(struct rte_mbuf* pkt) {
        static uint64_t last_cycles;
        static uint64_t cur_pkts = 0;
        static uint64_t last_pkts = 0;
        const char clr[] = { 27, '[', '2', 'J', '\0' };
        const char topLeft[] = { 27, '[', '1', ';', '1', 'H', '\0' };
        (void)pkt;

        uint64_t cur_cycles = rte_get_tsc_cycles();
        cur_pkts += print_delay;

        /* Clear screen and move to top left */
        printf("%s%s", clr, topLeft);

        printf("Total packets: %9"PRIu64" \n", cur_pkts);
        printf("TX pkts per second: %9"PRIu64" \n", (cur_pkts - last_pkts)
                * rte_get_timer_hz() / (cur_cycles - last_cycles));
        printf("Packets per group: %d\n", NUM_PKTS);

        last_pkts = cur_pkts;
        last_cycles = cur_cycles;

        printf("\n\n");
}

static void
add_timestamps(struct rte_mbuf *pkt)
{
	uint64_t now = rte_rdtsc();
	char* data;

	data = rte_pktmbuf_prepend(pkt, sizeof(struct stamp));
	if (data != NULL) {
		((struct stamp*)data)->flag = FLAG;
		((struct stamp*)data)->now = now;
	}
}

static void
calc_latency(struct rte_mbuf *pkt)
{
	uint64_t now = rte_rdtsc();
	struct stamp* data = rte_pktmbuf_mtod(pkt, struct stamp*);
	
	latency_numbers.total_cycles += now - data->now;
	latency_numbers.total_pkts++;
	rte_pktmbuf_adj(pkt, sizeof(struct stamp));
}

static void
print_latency(void)
{
	printf("Latency = %"PRIu64" cycles\n",
		latency_numbers.total_cycles / latency_numbers.total_pkts);
	latency_numbers.total_cycles = latency_numbers.total_pkts = 0;
}

static int
packet_handler(struct rte_mbuf* pkt, struct onvm_pkt_meta* meta) {
        static uint32_t counter = 0;
        struct stamp* data;
	
	if (counter++ == print_delay) {
	        do_stats_display(pkt);
      		print_latency();	
                counter = 0;
        }

	if(pkt->port == 3) {
		data = rte_pktmbuf_mtod(pkt, struct stamp*);
		if (data != NULL && data->flag == FLAG) {
			calc_latency(pkt);
		}       
        	else {
			add_timestamps(pkt);
		}
	        /* one of our fake pkts to forward */
                meta->destination = destination;
		meta->action = ONVM_NF_ACTION_TONF;
        }
        else {
                /* Drop real incoming packets */
                meta->action = ONVM_NF_ACTION_DROP;
        }
        return 0;
}


int main(int argc, char *argv[]) {
        int arg_offset;

        const char *progname = argv[0];

        if ((arg_offset = onvm_nf_init(argc, argv, NF_TAG)) < 0)
                return -1;
        argc -= arg_offset;
        argv += arg_offset;

        destination = nf_info->service_id;

        if (parse_app_args(argc, argv, progname) < 0)
                rte_exit(EXIT_FAILURE, "Invalid command-line arguments\n");

        struct rte_mempool *pktmbuf_pool;
        struct rte_mbuf* pkts[NUM_PKTS];
        int i;

        pktmbuf_pool = rte_mempool_lookup(PKTMBUF_POOL_NAME);
        if(pktmbuf_pool == NULL) {
                rte_exit(EXIT_FAILURE, "Cannot find mbuf pool!\n");
        }
        printf("Creating %d packets to send to %d\n", NUM_PKTS, destination);
        for (i=0; i < NUM_PKTS; i++) {
                struct onvm_pkt_meta* pmeta;
                pkts[i] = rte_pktmbuf_alloc(pktmbuf_pool);
                pmeta = onvm_get_pkt_meta(pkts[i]);
                pmeta->destination = destination;
                pmeta->action = ONVM_NF_ACTION_TONF;
		pkts[i]->port = 3;
                pkts[i]->hash.rss = i;
		onvm_nf_return_pkt(pkts[i]);
        }

        onvm_nf_run(nf_info, &packet_handler);
        printf("If we reach here, program is ending");
        return 0;
}
