/*
* Copyright (c) 2011 Samsung Electronics Co., Ltd All Rights Reserved 
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*  http://www.apache.org/licenses/LICENSE-2.0
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
*/


#include "sttd_main.h"
#include "sttd_network.h"

#include <vconf.h>

static bool g_is_connected;

bool sttd_network_is_connected()
{
	return g_is_connected;
}

void __net_config_change_cb(keynode_t* node, void *data) 
{
	int network_configuration = 0;
	vconf_get_int(VCONFKEY_NETWORK_CONFIGURATION_CHANGE_IND , &network_configuration);

	if (network_configuration == 0) {
		SLOG(LOG_DEBUG, TAG_STTD, "[Network DEBUG] Change Network Connection is OFF ");
		g_is_connected = false;
	} else {
		SLOG(LOG_DEBUG, TAG_STTD, "[Network DEBUG] Change Network Connection is ON ");
		g_is_connected = true;
		
		/* need to notify changing net to engine. */
	}

	return;
}

int sttd_network_initialize()
{
	int network_configuration = 0;
	vconf_get_int(VCONFKEY_NETWORK_CONFIGURATION_CHANGE_IND , &network_configuration);

	if (network_configuration == 0) {
		/*	"0" means the network configuration is not set. 
		*	It could be network connection is not open
		*/

		int network_status = 0;
		vconf_get_int(VCONFKEY_NETWORK_STATUS, &network_status);

		if(network_status == VCONFKEY_NETWORK_OFF){
			printf("Current network connection is OFF!! \n");
			SLOG(LOG_DEBUG, TAG_STTD, "[Network DEBUG] Current network connection is OFF.");
		}
		else{
			/*
			*	This is the problem of network connection
			*	Just terminate the application, network f/w will fix the problem automatically.
			*/
			printf("network status is wrong or IP is not set\n");
			printf("network has problem, try again\n");
			exit(0);
		}

		g_is_connected = false;
	} else {
		printf("Current network connection is ON. \n");
		SLOG(LOG_DEBUG, TAG_STTD, "[Network DEBUG] Current network connection is ON.");
			
		g_is_connected = true;
	}

	vconf_notify_key_changed(VCONFKEY_NETWORK_CONFIGURATION_CHANGE_IND, __net_config_change_cb, NULL);
	
	return 0;
}

int sttd_network_finalize()
{
	vconf_ignore_key_changed(VCONFKEY_NETWORK_CONFIGURATION_CHANGE_IND, __net_config_change_cb);

	return 0;
}

