/*
*  Copyright (c) 2011-2014 Samsung Electronics Co., Ltd All Rights Reserved 
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


#include <dlog.h>
#include <vconf.h>

#include "stt_network.h"

extern const char* stt_tag();

int stt_network_initialize()
{
	return 0;
}

int stt_network_finalize()
{
	return 0;
}

bool stt_network_is_connected()
{
	/* Check network */
	int network_status = 0;
	vconf_get_int(VCONFKEY_NETWORK_STATUS, &network_status);

	if(network_status == VCONFKEY_NETWORK_OFF){
		SLOG(LOG_WARN, stt_tag(), "[Network] Current network connection is OFF.");
		return false;
	}
	
	SLOG(LOG_DEBUG, stt_tag(), "[Network] Network status is %d", network_status);

	return true;
}