/*
*  Copyright (c) 2012, 2013 Samsung Electronics Co., Ltd All Rights Reserved 
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
static GList *g_client_list = NULL;

static GList *g_setting_client_list = NULL;

int client_show_list()
{
	GList *iter = NULL;
	client_info_s *data = NULL;

	SLOG(LOG_DEBUG, TAG_STTD, "----- client list"); 

	if (g_list_length(g_client_list) > 0) {
		/* Get a first item */
		iter = g_list_first(g_client_list);

		int i = 1;	
		while (NULL != iter) {
			/*Get handle data from list*/
			data = iter->data;

			SLOG(LOG_DEBUG, TAG_STTD, "[%dth] uid(%d), state(%d)", i, data->uid, data->state); 
			
			/*Get next item*/
			iter = g_list_next(iter);
			i++;
		}
	} else {
		SLOG(LOG_DEBUG, TAG_STTD, "No Client"); 
	}

	SLOG(LOG_DEBUG, TAG_STTD, "-----"); 

	SLOG(LOG_DEBUG, TAG_STTD, "----- setting client list"); 

	setting_client_info_s *setting_data = NULL;

	if (g_list_length(g_setting_client_list) > 0) {
		/* Get a first item */
		iter = g_list_first(g_setting_client_list);

		int i = 1;	
		while (NULL != iter) {
			/*Get handle data from list*/
			setting_data = iter->data;

			SLOG(LOG_DEBUG, TAG_STTD, "[%dth] pid(%d)", i, setting_data->pid); 

			/*Get next item*/
			iter = g_list_next(iter);
			i++;
		}
	} else {
		SLOG(LOG_DEBUG, TAG_STTD, "No setting client"); 
	}

	SLOG(LOG_DEBUG, TAG_STTD, "-----"); 

	return 0;
}

GList* __client_get_item(const int uid)
{
	GList *iter = NULL;
	client_info_s *data = NULL;

	if (0 < g_list_length(g_client_list)) {
		iter = g_list_first(g_client_list);

		while (NULL != iter) {
			/* Get handle data from list */
			data = iter->data;

			if (uid == data->uid) 
				return iter;
			
			iter = g_list_next(iter);
		}
	}

	return NULL;
}

int sttd_client_add(const int pid, const int uid)
{
	/*Check uid is duplicated*/
	GList *tmp = NULL;
	tmp = __client_get_item(uid);
	
	if (NULL != tmp) {
		SLOG(LOG_WARN, TAG_STTD, "[Client Data] Client uid is already registered"); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	client_info_s *info = (client_info_s*)g_malloc0(sizeof(client_info_s));

	info->pid = pid;
	info->uid = uid;
	info->state = APP_STATE_READY;

	/* Add item to global list */
	g_client_list = g_list_append(g_client_list, info);
	
	if (NULL == g_client_list) {
		SLOG(LOG_ERROR, TAG_STTD, "[Client Data ERROR] Fail to add new client"); 
		return -1;
	} else {
		SLOG(LOG_DEBUG, TAG_STTD, "[Client Data SUCCESS] Add new client"); 
	}

#ifdef CLIENT_DATA_DEBUG
	client_show_list();
#endif 
	return 0;
}

int sttd_client_delete(const int uid)
{
	GList *tmp = NULL;
	client_info_s* hnd = NULL;

	/*Get handle*/
	tmp = __client_get_item(uid);
	if (NULL == tmp) {
		SLOG(LOG_ERROR, TAG_STTD, "[Client Data ERROR] uid(%d) is NOT valid", uid); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	/*Free client structure*/
	hnd = tmp->data;
	if (NULL != hnd) {
		g_free(hnd);
	}

	/*Remove handle from list*/
	g_client_list = g_list_remove_link(g_client_list, tmp);

#ifdef CLIENT_DATA_DEBUG
	client_show_list();
#endif 

	return 0;
}

int sttd_client_get_state(const int uid, app_state_e* state)
{
	GList *tmp = NULL;
	client_info_s* hnd = NULL;

	tmp = __client_get_item(uid);
	if (NULL == tmp) {
		return STTD_ERROR_INVALID_PARAMETER;
	}

	hnd = tmp->data;
	*state = hnd->state;

	SLOG(LOG_DEBUG, TAG_STTD, "[Client Data] Get state : uid(%d), state(%d)", uid, *state);

	return 0;
}

int sttd_client_set_state(const int uid, const app_state_e state)
{
	GList *tmp = NULL;
	client_info_s* hnd = NULL;

	tmp = __client_get_item(uid);
	if (NULL == tmp) {
		SLOG(LOG_ERROR, TAG_STTD, "[Client Data ERROR] uid(%d) is NOT valid", uid); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	hnd = tmp->data;
	hnd->state = state ;

	SLOG(LOG_DEBUG, TAG_STTD, "[Client Data SUCCESS] Set state : uid(%d), state(%d)", uid, state);

	return 0;
}

int sttd_client_get_ref_count()
{
	int count = g_list_length(g_client_list) + g_list_length(g_setting_client_list);
	SLOG(LOG_DEBUG, TAG_STTD, "[Client Data] client count : %d", count);

	return count;
}

int sttd_client_get_pid(const int uid)
{
	GList *tmp = NULL;
	client_info_s* hnd = NULL;

	tmp = __client_get_item(uid);
	if (NULL == tmp) {
		SLOG(LOG_ERROR, TAG_STTD, "[Client Data ERROR] sttd_client_get_pid : uid(%d) is not found", uid); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	hnd = tmp->data;

	return hnd->pid;
}

int sttd_client_get_current_recording()
{
	GList *iter = NULL;
	client_info_s *data = NULL;

	if (0 < g_list_length(g_client_list)) {
		iter = g_list_first(g_client_list);

		while (NULL != iter) {
			/* Get handle data from list */
			data = iter->data;

			if (APP_STATE_RECORDING == data->state) 
				return data->uid;

			iter = g_list_next(iter);
		}
	}

	return -1;
}

int sttd_client_get_current_thinking()
{
	GList *iter = NULL;
	client_info_s *data = NULL;

	if (0 < g_list_length(g_client_list)) {
		iter = g_list_first(g_client_list);

		while (NULL != iter) {
			/* Get handle data from list */
			data = iter->data;

			if (APP_STATE_PROCESSING == data->state) 
				return data->uid;

			iter = g_list_next(iter);
		}
	}

	return -1;
}

int sttd_cliet_set_timer(int uid, Ecore_Timer* timer)
{
	GList *tmp = NULL;
	client_info_s* hnd = NULL;

	tmp = __client_get_item(uid);
	if (NULL == tmp) {
		SLOG(LOG_ERROR, TAG_STTD, "[Client Data ERROR] uid(%d) is NOT valid", uid); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	hnd = tmp->data;
	hnd->timer = timer;

	SLOG(LOG_DEBUG, TAG_STTD, "[Client Data SUCCESS] Set timer : uid(%d)", uid);

	return 0;
}

int sttd_cliet_get_timer(int uid, Ecore_Timer** timer)
{
	GList *tmp = NULL;
	client_info_s* hnd = NULL;

	tmp = __client_get_item(uid);
	if (NULL == tmp) {
		SLOG(LOG_ERROR, TAG_STTD, "[Client Data ERROR] uid(%d) is NOT valid", uid); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	hnd = tmp->data;
	*timer = hnd->timer;

	SLOG(LOG_DEBUG, TAG_STTD, "[Client Data SUCCESS] Get timer : uid(%d)", uid);

	return 0;
}


int sttd_client_get_list(int** uids, int* uid_count)
{
	if (NULL == uids || NULL == uid_count)
		return -1;
	
	int count = g_list_length(g_client_list);

	if (0 == count)
		return -1; 

	int *tmp;
	tmp = (int*)malloc(sizeof(int) * count);
	
	GList *iter = NULL;
	client_info_s *data = NULL;
	int i = 0;

	iter = g_list_first(g_client_list);
	for (i = 0;i < count;i++) {
		data = iter->data;
		tmp[i] = data->uid;
		iter = g_list_next(iter);
	}

	*uids = tmp;
	*uid_count = count;

	return 0;
}

/*
* Functions for setting
*/

GList* __setting_client_get_item(int pid)
{
	GList *iter = NULL;
	setting_client_info_s *data = NULL;

	if (0 < g_list_length(g_setting_client_list)) {
		iter = g_list_first(g_setting_client_list);

		while (NULL != iter) {
			/* Get handle data from list */
			data = iter->data;

			if (pid == data->pid) 
				return iter;

			iter = g_list_next(iter);
		}
	}

	return NULL;
}

int sttd_setting_client_add(int pid)
{	
	/* Check uid is duplicated */
	GList *tmp = NULL;
	tmp = __setting_client_get_item(pid);

	if (NULL != tmp) {
		SLOG(LOG_WARN, TAG_STTD, "[Client Data] Setting client(%d) is already registered", pid); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	setting_client_info_s *info = (setting_client_info_s*)g_malloc0(sizeof(setting_client_info_s));

	info->pid = pid;

	/* Add item to global list */
	g_setting_client_list = g_list_append(g_setting_client_list, info);

	if (NULL == g_setting_client_list) {
		SLOG(LOG_ERROR, TAG_STTD, "[Client Data ERROR] Fail to add new client"); 
		return -1;
	} else {
		SLOG(LOG_DEBUG, TAG_STTD, "[Client Data SUCCESS] Add new client"); 
	}

#ifdef CLIENT_DATA_DEBUG
	client_show_list();
#endif 
	return 0;
}

int sttd_setting_client_delete(int pid)
{
	GList *tmp = NULL;
	setting_client_info_s* hnd = NULL;

	/*Get handle*/
	tmp = __setting_client_get_item(pid);
	if (NULL == tmp) {
		SLOG(LOG_ERROR, TAG_STTD, "[Client Data ERROR] Setting uid(%d) is NOT valid", pid); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	/*Free client structure*/
	hnd = tmp->data;
	if (NULL != hnd) {
		g_free(hnd);
	}

	/*Remove handle from list*/
	g_setting_client_list = g_list_remove_link(g_setting_client_list, tmp);

#ifdef CLIENT_DATA_DEBUG
	client_show_list();
#endif 

	return 0;
}

bool sttd_setting_client_is(int pid)
{
	GList *tmp = __setting_client_get_item(pid);
	if (NULL == tmp) {
		return false;
	}

	return true;
}