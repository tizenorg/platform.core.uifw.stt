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


#include "stt_client.h"

#define MUTEX_TIME 3

/* Max number of handle */
static const int g_max_handle = 999;
/* allocated handle */
static int g_allocated_handle = 0;
/* client list */
static GList *g_client_list = NULL;


/* private functions */
static int __client_generate_uid(int pid)
{
	g_allocated_handle++;

	if (g_allocated_handle > g_max_handle) {
		g_allocated_handle = 1;
	}

	/* generate uid, handle number should be smaller than 1000 */
	return pid * 1000 + g_allocated_handle;
}

int stt_client_new(stt_h* stt)
{
	stt_client_s *client = NULL;

	client = (stt_client_s*)g_malloc0 (sizeof(stt_client_s));

	stt_h temp = (stt_h)g_malloc0(sizeof(struct stt_s));
	temp->handle = __client_generate_uid(getpid()); 

	/* initialize client data */
	client->stt = temp;
	client->pid = getpid(); 
	client->uid = temp->handle;
	
	client->recognition_result_cb = NULL;
	client->recognition_result_user_data = NULL;
	client->state_changed_cb = NULL;
	client->state_changed_user_data = NULL;
	client->error_cb = NULL;
	client->error_user_data = NULL;
	client->default_lang_changed_cb = NULL;
	client->default_lang_changed_user_data = NULL;

	client->current_engine_id = NULL;

	client->silence_supported = false;
	client->silence = STT_OPTION_SILENCE_DETECTION_AUTO;

	client->event = 0;
	client->data_list = NULL;
	client->data_count = 0;
	client->msg = NULL;

	client->before_state = STT_STATE_CREATED;
	client->current_state = STT_STATE_CREATED;

	client->internal_state = STT_INTERNAL_STATE_NONE;

	client->cb_ref_count = 0;

	g_client_list = g_list_append(g_client_list, client);

	*stt = temp;

	return 0;	
}

int stt_client_destroy(stt_h stt)
{
	if (stt == NULL) {
		SLOG(LOG_ERROR, TAG_STTC, "Input parameter is NULL");
		return 0;
	}

	GList *iter = NULL;
	stt_client_s *data = NULL;

	/* if list have item */
	if (g_list_length(g_client_list) > 0) {
		/* Get a first item */
		iter = g_list_first(g_client_list);

		while (NULL != iter) {
			data = iter->data;
			if (stt->handle == data->stt->handle) {
				g_client_list = g_list_remove_link(g_client_list, iter);

				while (0 != data->cb_ref_count)
				{
					/* wait for release callback function */
				}
				
				if (NULL != data->current_engine_id) {
					free(data->current_engine_id);
				}

				free(data);
				free(stt);

				return 0;
			}

			/* Next item */
			iter = g_list_next(iter);
		}
	}

	SLOG(LOG_ERROR, TAG_STTC, "[ERROR] client Not founded");

	return -1;
}


stt_client_s* stt_client_get(stt_h stt)
{
	if (NULL == stt) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Input parameter is NULL");
		return NULL;
	}

	GList *iter = NULL;
	stt_client_s *data = NULL;

	if (g_list_length(g_client_list) > 0) {
		/* Get a first item */
		iter = g_list_first(g_client_list);

		while (NULL != iter) {
			data = iter->data;
			if (NULL != data) {
				if (stt->handle == data->stt->handle) 
					return data;
			}
			/* Next item */
			iter = g_list_next(iter);
		}
	}

	SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to get client by stt");

	return NULL;
}

stt_client_s* stt_client_get_by_uid(const int uid)
{
	if (uid < 0) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] out of range : handle");
		return NULL;
	}

	GList *iter = NULL;
	stt_client_s *data = NULL;

	if (g_list_length(g_client_list) > 0) {
		/* Get a first item */
		iter = g_list_first(g_client_list);

		while (NULL != iter) {
			data = iter->data;
			if (uid == data->uid) {
				return data;
			}

			/* Next item */
			iter = g_list_next(iter);
		}
	}

	SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to get client by uid");

	return NULL;
}

int stt_client_get_size()
{
	return g_list_length(g_client_list);
}

int stt_client_use_callback(stt_client_s* client)
{
	client->cb_ref_count++;
	return 0;
}

int stt_client_not_use_callback(stt_client_s* client)
{
	client->cb_ref_count--;
	return 0;
}

int stt_client_get_use_callback(stt_client_s* client)
{
	return client->cb_ref_count;
}

GList* stt_client_get_client_list()
{
	return g_client_list;
}