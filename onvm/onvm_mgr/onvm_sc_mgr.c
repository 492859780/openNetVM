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
 * onvm_sc_mgr.h - service chain functions for manager 
 ********************************************************************/

#include <rte_common.h>
#include <rte_memory.h>
#include <rte_debug.h>
#include <rte_malloc.h>
#include "onvm_mgr/onvm_sc_mgr.h"
#include "shared/onvm_sc_common.h"

struct onvm_service_chain*
onvm_sc_get(void) {
	return NULL;
}

struct onvm_service_chain*
onvm_sc_create(void)
{
        struct onvm_service_chain *chain;
	int retval; 

        chain = rte_calloc("service chain",
                        1, sizeof(struct onvm_service_chain), 0);
        if (chain == NULL) {
                rte_exit(EXIT_FAILURE, "Cannot allocate memory for service chain\n");
        }
        /*init test: chain length is 4*/
        retval = onvm_sc_append_entry(chain, ONVM_NF_ACTION_NEXT, 0);
	if (retval == 1) {
		printf("chain length can not be larger than the maximum chain length\n");
		return NULL;
	}
        /*onvm_sc_append_entry(chain, ONVM_NF_ACTION_NEXT, 1);
	if (retval == 1) {
		printf("chain length can not be larger than the maximum chain length\n");
		return NULL;
	}*/
        onvm_sc_append_entry(chain, ONVM_NF_ACTION_OUT, 0);
	if (retval == 1) {
		printf("chain length can not be larger than the maximum chain length\n");
		return NULL;
	}
       	
	return chain;
}



