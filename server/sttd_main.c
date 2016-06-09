/*
*  Copyright (c) 2011-2016 Samsung Electronics Co., Ltd All Rights Reserved
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

#include <Ecore.h>

#include "stt_defs.h"
#include "stt_network.h"
#include "sttd_dbus.h"
#include "sttd_main.h"
#include "sttd_server.h"


#define CLIENT_CLEAN_UP_TIME 500

static Ecore_Timer* g_check_client_timer = NULL;

int main(int argc, char** argv)
{
	SLOG(LOG_DEBUG, TAG_STTD, "  ");
	SLOG(LOG_DEBUG, TAG_STTD, "  ");
	SLOG(LOG_DEBUG, TAG_STTD, "===== STT Daemon Initialize");

	if (!ecore_init()) {
		SLOG(LOG_ERROR, TAG_STTD, "[ERROR] Fail to initialize Ecore"); 
		return EXIT_FAILURE;
	}

	if (0 != sttd_dbus_open_connection()) {
		SLOG(LOG_ERROR, TAG_STTD, "[ERROR] Fail to open connection");
		return EXIT_FAILURE;
	}

	if (0 != sttd_initialize()) {
		SLOG(LOG_ERROR, TAG_STTD, "[ERROR] Fail to initialize stt-daemon");
		return EXIT_FAILURE;
	}

	stt_network_initialize();

	g_check_client_timer = ecore_timer_add(CLIENT_CLEAN_UP_TIME, sttd_cleanup_client, NULL);
	if (NULL == g_check_client_timer) {
		SLOG(LOG_WARN, TAG_STTD, "[Main Warning] Fail to create timer of client check");
	}

	SLOG(LOG_DEBUG, TAG_STTD, "[Main] stt-daemon start..."); 

	SLOG(LOG_DEBUG, TAG_STTD, "=====");
	SLOG(LOG_DEBUG, TAG_STTD, "  ");
	SLOG(LOG_DEBUG, TAG_STTD, "  ");

	ecore_main_loop_begin();

	SLOG(LOG_DEBUG, TAG_STTD, "===== STT Daemon Finalize");

	if (NULL != g_check_client_timer) {
		ecore_timer_del(g_check_client_timer);
	}

	sttd_dbus_close_connection();

	stt_network_finalize();

	sttd_finalize();

	ecore_shutdown();

	SLOG(LOG_DEBUG, TAG_STTD, "=====");
	SLOG(LOG_DEBUG, TAG_STTD, "  ");
	SLOG(LOG_DEBUG, TAG_STTD, "  ");

	return 0;
}



