/*
* Copyright (c) 2012, 2013 Samsung Electronics Co., Ltd All Rights Reserved 
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
#include "sttd_server.h"
#include "sttd_network.h"
#include "sttd_dbus.h"

#include <Ecore.h>
#include "sttd_server.h"

#define CLIENT_CLEAN_UP_TIME 500

int main(int argc, char** argv)
{
	SLOG(LOG_DEBUG, TAG_STTD, "  ");
	SLOG(LOG_DEBUG, TAG_STTD, "  ");
	SLOG(LOG_DEBUG, TAG_STTD, "===== STT Daemon Initialize");

	if (0 != sttd_initialize()) {
		SLOG(LOG_ERROR, TAG_STTD, "[ERROR] Fail to initialize stt-daemon"); 
		return EXIT_FAILURE;
	}

	if (0 != sttd_dbus_open_connection()) {
		SLOG(LOG_ERROR, TAG_STTD, "[ERROR] Fail to open connection");
		return EXIT_FAILURE;
	}

	sttd_network_initialize();

	ecore_timer_add(CLIENT_CLEAN_UP_TIME, sttd_cleanup_client, NULL);

	printf("stt-daemon start...\n");

	SLOG(LOG_DEBUG, TAG_STTD, "[Main] stt-daemon start..."); 

	SLOG(LOG_DEBUG, TAG_STTD, "=====");
	SLOG(LOG_DEBUG, TAG_STTD, "  ");
	SLOG(LOG_DEBUG, TAG_STTD, "  ");

	ecore_main_loop_begin();

	SLOG(LOG_DEBUG, TAG_STTD, "===== STT Daemon Finalize");

	sttd_dbus_close_connection();

	sttd_network_finalize();

	ecore_shutdown();

	SLOG(LOG_DEBUG, TAG_STTD, "=====");
	SLOG(LOG_DEBUG, TAG_STTD, "  ");
	SLOG(LOG_DEBUG, TAG_STTD, "  ");

	return 0;
}



