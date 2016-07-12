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

#include "stt_engine.h"
#include "stt_defs.h"
#include "stt_network.h"
#include "sttd_dbus.h"
#include "sttd_server.h"

#include "stte.h"


#define CLIENT_CLEAN_UP_TIME 500

static Ecore_Timer* g_check_client_timer = NULL;

int stte_main(int argc, char**argv, stte_request_callback_s *callback)
{
	SLOG(LOG_DEBUG, TAG_STTD, "===== Start engine");

	if (!ecore_init()) {
		SLOG(LOG_ERROR, TAG_STTD, "[ERROR] Fail to initialize Ecore");
		return EXIT_FAILURE;
	}

	if (0 != sttd_dbus_open_connection()) {
		SLOG(LOG_ERROR, TAG_STTD, "[ERROR] Fail to open connection");
		return EXIT_FAILURE;
	}

	if (0 != sttd_initialize(callback)) {
		SLOG(LOG_ERROR, TAG_STTD, "[ERROR] Fail to initialize stt-service");
		return EXIT_FAILURE;
	}

	stt_network_initialize();

	g_check_client_timer = ecore_timer_add(CLIENT_CLEAN_UP_TIME, sttd_cleanup_client, NULL);
	if (NULL == g_check_client_timer) {
		SLOG(LOG_WARN, TAG_STTD, "[Main Warning] Fail to create timer of client check");
	}

	SLOG(LOG_DEBUG, TAG_STTD, "[Main] stt-service start...");

	SLOG(LOG_DEBUG, TAG_STTD, "=====");
	SLOG(LOG_DEBUG, TAG_STTD, "  ");
	SLOG(LOG_DEBUG, TAG_STTD, "  ");

	return 0;
}

int stte_send_result(stte_result_event_e event, const char* type, const char** result, int result_count,
				const char* msg, void* time_info, void* user_data)
{
	if (NULL == type || NULL == result) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Invalid parameter");
	}

	int ret = STTE_ERROR_NONE;
	ret = stt_engine_send_result(event, type, result, result_count, msg, time_info, user_data);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to send result");
	}
	return ret;
}

int stte_send_error(stte_error_e error, const char* msg)
{
	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Invalid parameter");
	}

	int ret = STTE_ERROR_NONE;
	ret = stt_engine_send_error(error, msg);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to send error info");
	}
	return ret;
}

int stte_send_speech_status(stte_speech_status_e status, void* user_data)
{
	int ret = STTE_ERROR_NONE;
	ret = stt_engine_send_speech_status(status, user_data);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to send speech status");
	}
	return ret;
}

int stte_set_private_data_set_cb(stte_private_data_set_cb callback)
{
	int ret = STTE_ERROR_NONE;
	ret = stt_engine_set_private_data_set_cb(callback, NULL);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to send speech status");
	}
	return ret;
}

int stte_set_private_data_requested_cb(stte_private_data_requested_cb callback)
{
	int ret = STTE_ERROR_NONE;
	ret = stt_engine_set_private_data_requested_cb(callback, NULL);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to send speech status");
	}
	return ret;
}
