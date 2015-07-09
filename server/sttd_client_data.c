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


#include "sttd_main.h"
#include "sttd_client_data.h"

/* Client list */
static GSList *g_client_list = NULL;

static int g_cur_recog_uid = 0;

int client_show_list()
{
	GSList *iter = NULL;
	client_info_s *data = NULL;

	SLOG(LOG_DEBUG, TAG_STTD, "----- client list");

	if (g_slist_length(g_client_list) > 0) {
		/* Get a first item */
		iter = g_slist_nth(g_client_list, 0);

		int i = 1;	
		while (NULL != iter) {
			/*Get handle data from list*/
			data = iter->data;

			SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[%dth] uid(%d), state(%d)", i, data->uid, data->state);
			
			/*Get next item*/
			iter = g_slist_next(iter);
			i++;
		}
	} else {
		SLOG(LOG_DEBUG, TAG_STTD, "No Client");
	}

	SLOG(LOG_DEBUG, TAG_STTD, "-----");

	return 0;
}

GSList* __client_get_item(int uid)
{
	GSList *iter = NULL;
	client_info_s *data = NULL;

	if (0 < g_slist_length(g_client_list)) {
		iter = g_slist_nth(g_client_list, 0);

		while (NULL != iter) {
			/* Get handle data from list */
			data = iter->data;

			if (uid == data->uid) 
				return iter;
			
			iter = g_slist_next(iter);
		}
	}

	return NULL;
}

int sttd_client_add(int pid, int uid)
{
	/*Check uid is duplicated*/
	GSList *tmp = NULL;
	tmp = __client_get_item(uid);
	
	if (NULL != tmp) {
		SLOG(LOG_WARN, TAG_STTD, "[Client Data] Client uid is already registered");
		return STTD_ERROR_INVALID_PARAMETER;
	}

	client_info_s *info = (client_info_s*)calloc(1, sizeof(client_info_s));
	if (NULL == info) {
		SLOG(LOG_ERROR, TAG_STTD, "[Client Data ERROR] Fail to allocate memory");
		return STTD_ERROR_OUT_OF_MEMORY;
	}

	info->pid = pid;
	info->uid = uid;
	info->start_beep = NULL;
	info->stop_beep = NULL;
	info->state = APP_STATE_READY;

	info->app_agreed = false;

	/* Add item to global list */
	g_client_list = g_slist_append(g_client_list, info);
	
	if (NULL == g_client_list) {
		SLOG(LOG_ERROR, TAG_STTD, "[Client Data ERROR] Fail to add new client");
		return -1;
	}

#ifdef CLIENT_DATA_DEBUG
	client_show_list();
#endif 
	return 0;
}

int sttd_client_delete(int uid)
{
	GSList *tmp = NULL;
	client_info_s* hnd = NULL;

	/*Get handle*/
	tmp = __client_get_item(uid);
	if (NULL == tmp) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Client Data ERROR] uid(%d) is NOT valid", uid);
		return STTD_ERROR_INVALID_PARAMETER;
	}

	/*Free client structure*/
	hnd = tmp->data;
	if (NULL != hnd) {
		if (NULL != hnd->start_beep)	free(hnd->start_beep);
		if (NULL != hnd->stop_beep)	free(hnd->stop_beep);
		free(hnd);
	}

	/*Remove handle from list*/
	g_client_list = g_slist_remove_link(g_client_list, tmp);

#ifdef CLIENT_DATA_DEBUG
	client_show_list();
#endif 

	return 0;
}

int sttd_client_get_start_sound(int uid, char** filename)
{
	if (NULL == filename) {
		SLOG(LOG_ERROR, TAG_STTD, "[Client Data ERROR] Filename is NULL");
		return STTD_ERROR_INVALID_PARAMETER;
	}

	GSList *tmp = NULL;
	client_info_s* hnd = NULL;

	tmp = __client_get_item(uid);
	if (NULL == tmp) {
		return STTD_ERROR_INVALID_PARAMETER;
	}

	hnd = tmp->data;
	if (NULL != hnd->start_beep) {
		*filename = strdup(hnd->start_beep);
	} else {
		*filename = NULL;
	}

	return 0;
}

int sttd_client_set_start_sound(int uid, const char* filename)
{
	GSList *tmp = NULL;
	client_info_s* hnd = NULL;

	tmp = __client_get_item(uid);
	if (NULL == tmp) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Client Data ERROR] uid(%d) is NOT valid", uid);
		return STTD_ERROR_INVALID_PARAMETER;
	}

	hnd = tmp->data;
	if (NULL != hnd->start_beep) {
		free(hnd->start_beep);
	}
	
	if (NULL != filename) {
		hnd->start_beep = strdup(filename);
		SLOG(LOG_DEBUG, TAG_STTD, "[Client Data] Start sound file : %s", hnd->start_beep);
	} else {
		hnd->start_beep = NULL;
	}

	return 0;
}

int sttd_client_get_stop_sound(int uid, char** filename)
{
	if (NULL == filename) {
		SLOG(LOG_ERROR, TAG_STTD, "[Client Data ERROR] Filename is NULL");
		return STTD_ERROR_INVALID_PARAMETER;
	}

	GSList *tmp = NULL;
	client_info_s* hnd = NULL;

	tmp = __client_get_item(uid);
	if (NULL == tmp) {
		return STTD_ERROR_INVALID_PARAMETER;
	}

	hnd = tmp->data;
	if (NULL != hnd->stop_beep) {
		*filename = strdup(hnd->stop_beep);
	} else {
		*filename = NULL;
	}

	return 0;
}

int sttd_client_set_stop_sound(int uid, const char* filename)
{
	GSList *tmp = NULL;
	client_info_s* hnd = NULL;

	tmp = __client_get_item(uid);
	if (NULL == tmp) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Client Data ERROR] uid(%d) is NOT valid", uid);
		return STTD_ERROR_INVALID_PARAMETER;
	}

	hnd = tmp->data;
	if (NULL != hnd->stop_beep) {
		free(hnd->stop_beep);
	}
	
	if (NULL != filename) {
		hnd->stop_beep = strdup(filename);
		SLOG(LOG_DEBUG, TAG_STTD, "[Client Data] Stop sound file : %s", hnd->stop_beep);
	} else {
		hnd->stop_beep = NULL;
	}

	return 0;
}

int sttd_client_get_state(int uid, app_state_e* state)
{
	GSList *tmp = NULL;
	client_info_s* hnd = NULL;

	tmp = __client_get_item(uid);
	if (NULL == tmp) {
		return STTD_ERROR_INVALID_PARAMETER;
	}

	hnd = tmp->data;
	*state = hnd->state;

	return 0;
}

int sttd_client_set_state(int uid, app_state_e state)
{
	GSList *tmp = NULL;
	client_info_s* hnd = NULL;

	tmp = __client_get_item(uid);
	if (NULL == tmp) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Client Data ERROR] uid(%d) is NOT valid", uid);
		return STTD_ERROR_INVALID_PARAMETER;
	}

	hnd = tmp->data;
	hnd->state = state ;

	return 0;
}

int sttd_client_get_ref_count()
{
	int count = g_slist_length(g_client_list);

	return count;
}

int sttd_client_get_pid(int uid)
{
	GSList *tmp = NULL;
	client_info_s* hnd = NULL;

	tmp = __client_get_item(uid);
	if (NULL == tmp) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Client Data ERROR] sttd_client_get_pid : uid(%d) is not found", uid);
		return STTD_ERROR_INVALID_PARAMETER;
	}

	hnd = tmp->data;

	return hnd->pid;
}

#if 0
int sttd_client_get_current_recording()
{
	GSList *iter = NULL;
	client_info_s *data = NULL;

	if (0 < g_slist_length(g_client_list)) {
		iter = g_slist_nth(g_client_list, 0);

		while (NULL != iter) {
			/* Get handle data from list */
			data = iter->data;

			if (APP_STATE_RECORDING == data->state) 
				return data->uid;

			iter = g_slist_next(iter);
		}
	}

	return -1;
}

int sttd_client_get_current_thinking()
{
	GSList *iter = NULL;
	client_info_s *data = NULL;

	if (0 < g_slist_length(g_client_list)) {
		iter = g_slist_nth(g_client_list, 0);

		while (NULL != iter) {
			/* Get handle data from list */
			data = iter->data;

			if (APP_STATE_PROCESSING == data->state) 
				return data->uid;

			iter = g_slist_next(iter);
		}
	}

	return -1;
}

int sttd_cliet_set_timer(int uid, Ecore_Timer* timer)
{
	GSList *tmp = NULL;
	client_info_s* hnd = NULL;

	tmp = __client_get_item(uid);
	if (NULL == tmp) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Client Data ERROR] uid(%d) is NOT valid", uid);
		return STTD_ERROR_INVALID_PARAMETER;
	}

	hnd = tmp->data;
	hnd->timer = timer;

	return 0;
}

int sttd_cliet_get_timer(int uid, Ecore_Timer** timer)
{
	GSList *tmp = NULL;
	client_info_s* hnd = NULL;

	tmp = __client_get_item(uid);
	if (NULL == tmp) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Client Data ERROR] uid(%d) is NOT valid", uid);
		return STTD_ERROR_INVALID_PARAMETER;
	}

	hnd = tmp->data;
	*timer = hnd->timer;

	return 0;
}
#endif

int sttd_client_get_list(int** uids, int* uid_count)
{
	if (NULL == uids || NULL == uid_count)
		return -1;
	
	int count = g_slist_length(g_client_list);

	if (0 == count)
		return -1;

	int *tmp;
	tmp = (int*)calloc(count, sizeof(int));
	if (NULL == tmp) {
		SLOG(LOG_ERROR, TAG_STTD, "[Client Data ERROR] Fail to allocate memory");
		return STTD_ERROR_OUT_OF_MEMORY;
	}

	GSList *iter = NULL;
	client_info_s *data = NULL;
	int i = 0;

	iter = g_slist_nth(g_client_list, 0);
	for (i = 0;i < count;i++) {
		data = iter->data;
		tmp[i] = data->uid;
		iter = g_slist_next(iter);
	}

	*uids = tmp;
	*uid_count = count;

	return 0;
}

int stt_client_set_current_recognition(int uid)
{
	if (0 != g_cur_recog_uid) {
		return -1;
	}

	g_cur_recog_uid = uid;

	return 0;
}

int stt_client_get_current_recognition()
{
	return g_cur_recog_uid;
}

int stt_client_unset_current_recognition()
{
	g_cur_recog_uid = 0;
	return 0;
}

int stt_client_set_app_agreed(int uid)
{
	GSList *tmp = NULL;
	client_info_s* hnd = NULL;

	tmp = __client_get_item(uid);
	if (NULL == tmp) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Client Data ERROR] uid(%d) is NOT valid", uid); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	hnd = tmp->data;
	hnd->app_agreed = true;

	return 0;
}

bool stt_client_get_app_agreed(int uid)
{
	GSList *tmp = NULL;
	client_info_s* hnd = NULL;

	tmp = __client_get_item(uid);
	if (NULL == tmp) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Client Data ERROR] uid(%d) is NOT valid", uid); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	hnd = tmp->data;
	return hnd->app_agreed;
}