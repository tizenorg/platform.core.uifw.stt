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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "stt_file_client.h"

static stt_file_client_s* g_client_info = NULL;

int stt_file_client_new()
{
	if (NULL != g_client_info) {
		SLOG(LOG_ERROR, TAG_STTFC, "[ERROR] Already allocated client info");
		return STT_FILE_ERROR_INVALID_STATE;
	}

	g_client_info = (stt_file_client_s*)calloc(1, sizeof(stt_file_client_s));

	/* initialize client data */
	g_client_info->recognition_result_cb = NULL;
	g_client_info->recognition_result_user_data = NULL;

	g_client_info->result_time_cb = NULL;
	g_client_info->result_time_user_data = NULL;

	g_client_info->state_changed_cb = NULL;
	g_client_info->state_changed_user_data = NULL;

	g_client_info->supported_lang_cb = NULL;
	g_client_info->supported_lang_user_data = NULL;

	g_client_info->current_engine_id = -1;

	g_client_info->time_info = NULL;
	
	g_client_info->before_state = STT_FILE_STATE_READY;
	g_client_info->current_state = STT_FILE_STATE_READY;

	g_client_info->cb_ref_count = 0;

	return 0;	
}

int stt_file_client_destroy()
{
	if (NULL == g_client_info) {
		SLOG(LOG_ERROR, TAG_STTFC, "[ERROR] Client info is NULL");
		return STT_FILE_ERROR_INVALID_STATE;
	} else {
		while (0 != g_client_info->cb_ref_count) {
			/* wait for release callback function */
		}
		
		free(g_client_info);

		g_client_info = NULL;
	}

	return 0;
}

stt_file_client_s* stt_file_client_get()
{
	return g_client_info;
}

int stt_file_client_use_callback(stt_file_client_s* client)
{
	client->cb_ref_count++;
	return 0;
}

int stt_file_client_not_use_callback(stt_file_client_s* client)
{
	client->cb_ref_count--;
	return 0;
}

int stt_file_client_get_use_callback(stt_file_client_s* client)
{
	return client->cb_ref_count;
}
