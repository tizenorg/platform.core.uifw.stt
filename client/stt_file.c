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

#include <aul.h>
#include <dlfcn.h>
#include <dirent.h>
#include <dlog.h>
#include <Ecore.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "stt_config_mgr.h"
#include "stt_defs.h"
#include "stt_engine.h"
#include "stt_file.h"
#include "stt_file_client.h"
#include "stt_network.h"


typedef struct _sttengine_info {
	int	engine_id;

	char*	engine_uuid;
	char*	engine_path;
	char*	engine_name;
	char*	engine_setting_path;
	bool	use_network;

	bool	is_loaded;
}sttengine_info_s;

static GSList*	g_engine_list;

static int	g_engine_id_count = 0;


#define STT_FILE_CONFIG_HANDLE	100000


const char* stt_tag()
{
	return TAG_STTFC;
}

static const char* __stt_file_get_error_code(stt_file_error_e err)
{
	switch(err) {
	case STT_FILE_ERROR_NONE:			return "STT_FILE_ERROR_NONE";
	case STT_FILE_ERROR_OUT_OF_MEMORY:		return "STT_FILE_ERROR_OUT_OF_MEMORY";
	case STT_FILE_ERROR_IO_ERROR:			return "STT_FILE_ERROR_IO_ERROR";
	case STT_FILE_ERROR_INVALID_PARAMETER:		return "STT_FILE_ERROR_INVALID_PARAMETER";
	case STT_FILE_ERROR_OUT_OF_NETWORK:		return "STT_FILE_ERROR_OUT_OF_NETWORK";
	case STT_FILE_ERROR_INVALID_STATE:		return "STT_FILE_ERROR_INVALID_STATE";
	case STT_FILE_ERROR_INVALID_LANGUAGE:		return "STT_FILE_ERROR_INVALID_LANGUAGE";
	case STT_FILE_ERROR_ENGINE_NOT_FOUND:		return "STT_FILE_ERROR_ENGINE_NOT_FOUND";
	case STT_FILE_ERROR_OPERATION_FAILED:		return "STT_FILE_ERROR_OPERATION_FAILED";
	case STT_FILE_ERROR_NOT_SUPPORTED_FEATURE:	return "STT_FILE_ERROR_NOT_SUPPORTED_FEATURE";
	case STT_FILE_ERROR_NOT_AGREE_SERVICE:		return "STT_FILE_ERROR_NOT_AGREE_SERVICE";
	default:
		return "Invalid error code";
	}
}

void __stt_file_engine_info_cb(const char* engine_uuid, const char* engine_name, const char* setting_ug_name, 
		      bool use_network, void* user_data)
{
	sttengine_info_s* temp = (sttengine_info_s*)user_data; 

	temp->engine_uuid = g_strdup(engine_uuid);
	temp->engine_name = g_strdup(engine_name);
	temp->engine_setting_path = g_strdup(setting_ug_name);
	temp->use_network = use_network;
}

static int __stt_file_get_engine_info(const char* filepath, sttengine_info_s** info)
{
	if (NULL == filepath || NULL == info) {
		SLOG(LOG_ERROR, TAG_STTFC, "[Engine Agent ERROR] Invalid Parameter"); 
		return STT_FILE_ERROR_INVALID_PARAMETER;
	}

	/* load engine */
	char *error;
	void* handle;

	handle = dlopen (filepath, RTLD_LAZY);

	if (!handle) {
		SECURE_SLOG(LOG_WARN, TAG_STTFC, "[Engine Agent] Invalid engine : %s", filepath); 
		return -1;
	}

	/* link engine to daemon */
	dlsym(handle, "sttp_load_engine");
	if ((error = dlerror()) != NULL) {
		SLOG(LOG_WARN, TAG_STTFC, "[Engine Agent] Invalid engine. Fail to open sttp_load_engine : %s", error); 
		dlclose(handle);
		return -1;
	}

	dlsym(handle, "sttp_unload_engine");
	if ((error = dlerror()) != NULL) {
		SLOG(LOG_WARN, TAG_STTFC, "[Engine Agent] Invalid engine. Fail to open sttp_unload_engine : %s", error); 
		dlclose(handle);
		return -1;
	}

	int (*get_engine_info)(sttpe_engine_info_cb callback, void* user_data);

	get_engine_info = (int (*)(sttpe_engine_info_cb, void*))dlsym(handle, "sttp_get_engine_info");
	if ((error = dlerror()) != NULL || NULL == get_engine_info) {
		SLOG(LOG_WARN, TAG_STTFC, "[Engine Agent WARNING] Invalid engine. Fail to open sttp_get_engine_info : %s", error); 
		dlclose(handle);
		return -1;
	}

	sttengine_info_s* temp;
	temp = (sttengine_info_s*)calloc(1, sizeof(sttengine_info_s));

	/* get engine info */
	if (0 != get_engine_info(__stt_file_engine_info_cb, (void*)temp)) {
		SLOG(LOG_ERROR, TAG_STTFC, "[Engine Agent ERROR] Fail to get engine info from engine"); 
		dlclose(handle);
		free(temp);
		return -1;
	}

	/* close engine */
	dlclose(handle);

	temp->engine_id = g_engine_id_count;
	g_engine_id_count++;

	temp->engine_path = g_strdup(filepath);
	temp->is_loaded = false;

	SLOG(LOG_DEBUG, TAG_STTFC, "----- Valid Engine");
	SECURE_SLOG(LOG_DEBUG, TAG_STTFC, "Engine id : %d", temp->engine_id);
	SECURE_SLOG(LOG_DEBUG, TAG_STTFC, "Engine uuid : %s", temp->engine_uuid);
	SECURE_SLOG(LOG_DEBUG, TAG_STTFC, "Engine name : %s", temp->engine_name);
	SECURE_SLOG(LOG_DEBUG, TAG_STTFC, "Engine path : %s", temp->engine_path);
	SECURE_SLOG(LOG_DEBUG, TAG_STTFC, "Engine setting path : %s", temp->engine_setting_path);
	SECURE_SLOG(LOG_DEBUG, TAG_STTFC, "Use network : %s", temp->use_network ? "true":"false");
	SLOG(LOG_DEBUG, TAG_STTFC, "-----");
	SLOG(LOG_DEBUG, TAG_STTFC, "  ");

	*info = temp;

	return 0;
}

static bool __stt_file_is_engine(const char* filepath)
{
	GSList *iter = NULL;
	sttengine_info_s *engine = NULL;

	if (0 < g_slist_length(g_engine_list)) {
		/* Get a first item */
		iter = g_slist_nth(g_engine_list, 0);

		while (NULL != iter) {
			/* Get handle data from list */
			engine = iter->data;

			if (0 == strcmp(engine->engine_path, filepath)) {
				return true;
			}
		
			iter = g_slist_next(iter);
		}
	}

	return false;
}

void __stt_file_relseae_engine_info()
{
	GSList *iter = NULL;

	/* Release engine list */
	sttengine_info_s *engine = NULL;

	if (0 < g_slist_length(g_engine_list)) {
		/* Get a first item */
		iter = g_slist_nth(g_engine_list, 0);

		while (NULL != iter) {
			/* Get handle data from list */
			engine = iter->data;
			g_engine_list = g_slist_remove_link(g_engine_list, iter);
			
			/* Check engine unload */
			if (NULL != engine) {
				if (engine->is_loaded) {
					SECURE_SLOG(LOG_DEBUG, TAG_STTFC, "[Engine Agent] Unload engine id(%d)", engine->engine_id);

					if (0 != stt_engine_deinitialize(engine->engine_id)) 
						SECURE_SLOG(LOG_WARN, TAG_STTFC, "[Engine Agent] Fail to deinitialize engine id(%d)", engine->engine_id);

					if (0 != stt_engine_unload(engine->engine_id))
						SECURE_SLOG(LOG_WARN, TAG_STTFC, "[Engine Agent] Fail to unload engine id(%d)", engine->engine_id);

					engine->is_loaded = false;
				}

				if (NULL != engine->engine_uuid)	free(engine->engine_uuid);
				if (NULL != engine->engine_path)	free(engine->engine_path);
				if (NULL != engine->engine_name)	free(engine->engine_name);
				if (NULL != engine->engine_setting_path)free(engine->engine_setting_path);

				free(engine);
			}

			iter = g_slist_nth(g_engine_list, 0);
		}
	}
}

static sttengine_info_s* __stt_file_get_engine_by_id(int engine_id)
{
	GSList *iter = NULL;
	sttengine_info_s *data = NULL;

	iter = g_slist_nth(g_engine_list, 0);

	while (NULL != iter) {

		data = iter->data;

		if (data->engine_id == engine_id) 
			return data;

		iter = g_slist_next(iter);
	}

	return NULL;
}

void __stt_file_result_cb(sttp_result_event_e event, const char* type, const char** data, int data_count, 
		 const char* msg, void* time_info, void *user_data)
{
	
	SLOG(LOG_DEBUG, TAG_STTFC, "[STT FILE] Result event(%d) type(%s) msg(%s)", event, type, msg);

	if (NULL != data) {
		int i = 0;
		for (i = 0;i < data_count;i++) {
			if (NULL != data[i]) {
				SLOG(LOG_DEBUG, TAG_STTFC, "[STT FILE] [%d] %s", i, data[i]);
			}
		}
	}

	stt_file_client_s* client = stt_file_client_get();

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTFC, "[ERROR] Fail to notify error : A handle is not valid");
		return;
	}

	if (NULL != time_info) {
		client->time_info = time_info;
	}

	if (NULL != client->recognition_result_cb) {
		stt_file_client_use_callback(client);
		client->recognition_result_cb(event, data, data_count, msg, client->recognition_result_user_data);
		stt_file_client_not_use_callback(client);
		SLOG(LOG_DEBUG, TAG_STTFC, "client recognition result callback called");
	} else {
		SLOG(LOG_WARN, TAG_STTFC, "[WARNING] User recognition result callback is NULL");
	}

	client->time_info = NULL;

	if (STTP_RESULT_EVENT_FINAL_RESULT == event || STTP_RESULT_EVENT_ERROR == event) {
		SLOG(LOG_DEBUG, TAG_STTFC, "[STT FILE] State change : 'Ready'");
		
		client->before_state = client->current_state;
		client->current_state = STT_FILE_STATE_READY;

		if (NULL != client->state_changed_cb) {
			stt_file_client_use_callback(client);
			client->state_changed_cb(client->before_state, client->current_state, client->state_changed_user_data); 
			stt_file_client_not_use_callback(client);
			SLOG(LOG_DEBUG, TAG_STTFC, "State changed callback is called");
		} else {
			SLOG(LOG_WARN, TAG_STTFC, "[WARNING] State changed callback is null");
		}
	}

	return;
}

void __stt_file_silence_cb(sttp_silence_type_e type, void *user_data)
{
	SLOG(LOG_WARN, TAG_STTFC, "[WARNING] This callback should NOT be called.");
	return;
}

static int __stt_file_load_engine(sttengine_info_s* engine)
{
	if (NULL == engine) {
		SLOG(LOG_ERROR, TAG_STTFC, "[STT FILE ERROR] Input engine is NULL");
		return STT_FILE_ERROR_INVALID_PARAMETER;
	}

	if (0 != stt_engine_load(engine->engine_id, engine->engine_path)) {
		SLOG(LOG_ERROR, TAG_STTFC, "[STT FILE ERROR] Fail to load engine(%s)", engine->engine_path);
		return STT_FILE_ERROR_OPERATION_FAILED;
	}

	int ret = stt_engine_initialize(engine->engine_id, __stt_file_result_cb, __stt_file_silence_cb);
	if (0 != ret) {
		SECURE_SLOG(LOG_ERROR, TAG_STTFC, "[Engine Agent ERROR] Fail to initialize engine : id(%d) path(%s)", engine->engine_id, engine->engine_path);
		return STT_FILE_ERROR_OPERATION_FAILED;
	}

	engine->is_loaded = true;

	SECURE_SLOG(LOG_DEBUG, TAG_STTFC, "[STT FILE Success] Load engine id(%d) path(%s)", engine->engine_id, engine->engine_path);

	return STT_FILE_ERROR_NONE;
}

int stt_file_initialize()
{
	SLOG(LOG_DEBUG, TAG_STTFC, "===== Initialize STT FILE");

	stt_file_client_s* client = stt_file_client_get();
	if (NULL != client) {
		SLOG(LOG_ERROR, TAG_STTFC, "[STT FILE ERROR] Already initialized");
		return STT_FILE_ERROR_INVALID_STATE;
	}

	/* Get file name from default engine directory */
	DIR *dp = NULL;
	int ret = -1;
	struct dirent entry;
	struct dirent *dirp = NULL;
	dp  = opendir(STT_DEFAULT_ENGINE);
	if (NULL != dp) {
		do {
			ret = readdir_r(dp, &entry, &dirp);
			if (0 != ret) {
				SLOG(LOG_ERROR, TAG_STTFC, "[File ERROR] Fail to read directory");
				break;
			}

			if (NULL != dirp) {
				sttengine_info_s* info;
				char* filepath;
				int filesize;

				filesize = strlen(STT_DEFAULT_ENGINE) + strlen(dirp->d_name) + 5;
				filepath = (char*)calloc(filesize, sizeof(char));


				if (NULL != filepath) {
					snprintf(filepath, filesize, "%s/%s", STT_DEFAULT_ENGINE, dirp->d_name);
				} else {
					SLOG(LOG_ERROR, TAG_STTFC, "[Engine Agent ERROR] Memory not enough!!");
					continue;
				}

				if (false == __stt_file_is_engine(filepath)) {
					/* get its and update engine list */
					if (0 == __stt_file_get_engine_info(filepath, &info)) {
						/* add engine info to g_engine_list */
						g_engine_list = g_slist_append(g_engine_list, info);
					}
				}

				if (NULL != filepath)
					free(filepath);
			}
		} while (NULL != dirp);

		closedir(dp);
	} else {
		SLOG(LOG_WARN, TAG_STTFC, "[Engine Agent WARNING] Fail to open default directory"); 
	}

	if (0 >= g_slist_length(g_engine_list)) {
		SLOG(LOG_WARN, TAG_STTFC, "[STT FILE WARNING] Not found engine");
		return STT_FILE_ERROR_ENGINE_NOT_FOUND;
	}

	ret = stt_file_client_new();
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTFC, "[STT FILE ERROR] Fail to create client : %s", __stt_file_get_error_code(ret));
		return STT_FILE_ERROR_OPERATION_FAILED;
	}

	client = stt_file_client_get();
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTFC, "[STT FILE ERROR] Not initialized");
		return STT_FILE_ERROR_OPERATION_FAILED;
	}

	ret = stt_config_mgr_initialize(getpid() + STT_FILE_CONFIG_HANDLE);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTFC, "[STT FILE ERROR] Fail to init config manager : %s", __stt_file_get_error_code(ret));
		stt_file_client_destroy();
		return STT_FILE_ERROR_OPERATION_FAILED;
	}

	/* Get engine */
	char* engine_id = NULL;
	ret = stt_config_mgr_get_engine(&engine_id);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTFC, "[STT FILE ERROR] Fail to get engine id : %s", __stt_file_get_error_code(ret));
		stt_file_client_destroy();
		stt_config_mgr_finalize(getpid() + STT_FILE_CONFIG_HANDLE);
		__stt_file_relseae_engine_info();
		return STT_FILE_ERROR_OPERATION_FAILED;
	}
	
	SLOG(LOG_DEBUG, TAG_STTFC, "[STT FILE] Get engine id : %s", engine_id);

	bool is_found = false;

	sttengine_info_s* engine = NULL;
	GSList *iter = NULL;
	/* load engine */
	if (0 < g_slist_length(g_engine_list)) {
		/* Get a first item */
		iter = g_slist_nth(g_engine_list, 0);

		while (NULL != iter) {
			/* Get handle data from list */
			engine = iter->data;

			if (0 == strcmp(engine->engine_uuid, engine_id)) {
				is_found = true;
				break;
			}

			iter = g_slist_next(iter);
		}
	}

	if (NULL != engine_id)	free(engine_id);

	if (false == is_found) {
		SLOG(LOG_WARN, TAG_STTFC, "[STT FILE WARNING] Fail to find default engine");
		iter = g_slist_nth(g_engine_list, 0);
		engine = iter->data;

		if (NULL == engine) {
			SLOG(LOG_ERROR, TAG_STTFC, "[Engine Agent ERROR] Fail to initialize engine");
			stt_file_client_destroy();
			stt_config_mgr_finalize(getpid() + STT_FILE_CONFIG_HANDLE);
			__stt_file_relseae_engine_info();
			return STT_FILE_ERROR_OPERATION_FAILED;
		}
		SLOG(LOG_WARN, TAG_STTFC, "[STT FILE WARNING] Set default engine (%s)", engine->engine_name);
	}

	ret = __stt_file_load_engine(engine);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTFC, "[Engine Agent ERROR] Fail to initialize engine");
		stt_file_client_destroy();
		stt_config_mgr_finalize(getpid() + STT_FILE_CONFIG_HANDLE);
		__stt_file_relseae_engine_info();
		return STT_FILE_ERROR_OPERATION_FAILED;
	}

	client->current_engine_id = engine->engine_id;

	stt_network_initialize();

	SECURE_SLOG(LOG_DEBUG, TAG_STTFC, "[STT FILE Success] Initialize : pid(%d)", getpid());

	SLOG(LOG_DEBUG, TAG_STTFC, "=====");
	SLOG(LOG_DEBUG, TAG_STTFC, " ");

	return STT_FILE_ERROR_NONE;
}

int stt_file_deinitialize()
{
	SLOG(LOG_DEBUG, TAG_STTFC, "===== Deinitialize STT FILE");
	
	stt_file_client_s* client = stt_file_client_get();
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTFC, "[STT FILE ERROR] Not initialized");
		return STT_FILE_ERROR_INVALID_STATE;
	}

	/* check used callback */
	if (0 != stt_file_client_get_use_callback(client)) {
		SLOG(LOG_ERROR, TAG_STTFC, "[STT FILE ERROR] Client should NOT deinitialize STT FILE in callback function");
		return STT_FILE_ERROR_OPERATION_FAILED;
	}

	stt_network_finalize();

	stt_config_mgr_finalize(getpid() + STT_FILE_CONFIG_HANDLE);

	/* check state */
	switch (client->current_state) {
	case STT_FILE_STATE_PROCESSING:
		/* Cancel file recognition */
		stt_engine_recognize_cancel_file(client->current_engine_id);

	case STT_FILE_STATE_READY:
		/* Unload engine */
		__stt_file_relseae_engine_info();

		/* Free resources */
		stt_file_client_destroy();
		break;

	case STT_FILE_STATE_NONE:
		SLOG(LOG_WARN, TAG_STTFC, "[STT FILE WARNING] NOT initialized");
		return 0;
	default:
		break;
	}

	SLOG(LOG_DEBUG, TAG_STTFC, "=====");
	SLOG(LOG_DEBUG, TAG_STTFC, " ");

	return STT_FILE_ERROR_NONE;
}

int stt_file_get_state(stt_file_state_e* state)
{
	if (NULL == state) {
		SLOG(LOG_ERROR, TAG_STTFC, "[ERROR] Input parameter is NULL");
		return STT_FILE_ERROR_INVALID_PARAMETER;
	}

	stt_file_client_s* client = stt_file_client_get();

	if (NULL == client) {
		SLOG(LOG_DEBUG, TAG_STTFC, "[STT FILE] Not initialized");
		*state = STT_FILE_STATE_NONE;
		return STT_FILE_ERROR_NONE;
	}

	*state = client->current_state;

	switch(*state) {
		case STT_FILE_STATE_NONE:	SLOG(LOG_DEBUG, TAG_STTFC, "Current state is 'NONE'");		break;
		case STT_FILE_STATE_READY:	SLOG(LOG_DEBUG, TAG_STTFC, "Current state is 'Ready'");		break;
		case STT_FILE_STATE_PROCESSING:	SLOG(LOG_DEBUG, TAG_STTFC, "Current state is 'Processing'");	break;
		default:			SLOG(LOG_ERROR, TAG_STTFC, "[ERROR] Invalid value");		break;
	}

	return STT_FILE_ERROR_NONE;
}

int stt_file_foreach_supported_engines(stt_file_supported_engine_cb callback, void* user_data)
{
	SLOG(LOG_DEBUG, TAG_STTFC, "===== Foreach Supported engine");

	if (NULL == callback) {
		SLOG(LOG_ERROR, TAG_STTFC, "[ERROR] Input parameter is NULL");
		SLOG(LOG_DEBUG, TAG_STTFC, "=====");
		SLOG(LOG_DEBUG, TAG_STTFC, " ");
		return STT_FILE_ERROR_INVALID_PARAMETER;
	}

	stt_file_client_s* client = stt_file_client_get();

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTFC, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_STTFC, "=====");
		SLOG(LOG_DEBUG, TAG_STTFC, " ");
		return STT_FILE_ERROR_INVALID_PARAMETER;
	}

	if (client->current_state != STT_FILE_STATE_READY) {
		SLOG(LOG_ERROR, TAG_STTFC, "[ERROR] Invalid State: Current state is not 'Ready'");
		SLOG(LOG_DEBUG, TAG_STTFC, "=====");
		SLOG(LOG_DEBUG, TAG_STTFC, " ");
		return STT_FILE_ERROR_INVALID_STATE;
	}

	GSList *iter = NULL;
	sttengine_info_s *engine = NULL;

	/* Get a first item */
	iter = g_slist_nth(g_engine_list, 0);

	while (NULL != iter) {
		/* Get handle data from list */
		engine = iter->data;

		if (NULL != engine) {
			if (false == callback(engine->engine_uuid, engine->engine_name, user_data)) {
				break;
			}
		}
		iter = g_slist_next(iter);
	}

	SLOG(LOG_DEBUG, TAG_STTFC, "=====");
	SLOG(LOG_DEBUG, TAG_STTFC, " ");
	
	return STT_FILE_ERROR_NONE;
}

int stt_file_get_engine(char** engine_id)
{
	SLOG(LOG_DEBUG, TAG_STTFC, "===== Get current engine");

	if (NULL == engine_id) {
		SLOG(LOG_ERROR, TAG_STTFC, "[ERROR] Input parameter is NULL");
		SLOG(LOG_DEBUG, TAG_STTFC, "=====");
		SLOG(LOG_DEBUG, TAG_STTFC, " ");
		return STT_FILE_ERROR_INVALID_PARAMETER;
	}

	stt_file_client_s* client = stt_file_client_get();

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTFC, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_STTFC, "=====");
		SLOG(LOG_DEBUG, TAG_STTFC, " ");
		return STT_FILE_ERROR_INVALID_PARAMETER;
	}

	if (client->current_state != STT_FILE_STATE_READY) {
		SLOG(LOG_ERROR, TAG_STTFC, "[ERROR] Invalid State: Current state is not 'Ready'");
		SLOG(LOG_DEBUG, TAG_STTFC, "=====");
		SLOG(LOG_DEBUG, TAG_STTFC, " ");
		return STT_FILE_ERROR_INVALID_STATE;
	}

	sttengine_info_s* engine = NULL;
	engine = __stt_file_get_engine_by_id(client->current_engine_id);
	if (NULL == engine) {
		SECURE_SLOG(LOG_ERROR, TAG_STTFC, "[ERROR] Fail to get engine info(%d)", client->current_engine_id);
		return STT_FILE_ERROR_OPERATION_FAILED;
	}

	*engine_id = strdup(engine->engine_uuid);

	SLOG(LOG_DEBUG, TAG_STTFC, "=====");
	SLOG(LOG_DEBUG, TAG_STTFC, " ");

	return STT_FILE_ERROR_NONE;
}

int stt_file_set_engine(const char* engine_id)
{	
	SLOG(LOG_DEBUG, TAG_STTFC, "===== Set current engine");

	if (NULL == engine_id) {
		SLOG(LOG_ERROR, TAG_STTFC, "[ERROR] Input parameter is NULL");
		SLOG(LOG_DEBUG, TAG_STTFC, "=====");
		SLOG(LOG_DEBUG, TAG_STTFC, " ");
		return STT_FILE_ERROR_INVALID_PARAMETER;
	}

	stt_file_client_s* client = stt_file_client_get();

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTFC, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_STTFC, "=====");
		SLOG(LOG_DEBUG, TAG_STTFC, " ");
		return STT_FILE_ERROR_INVALID_PARAMETER;
	}

	/* check state */
	if (client->current_state != STT_FILE_STATE_READY) {
		SLOG(LOG_ERROR, TAG_STTFC, "[ERROR] Invalid State: Current state is not 'Ready'");
		SLOG(LOG_DEBUG, TAG_STTFC, "=====");
		SLOG(LOG_DEBUG, TAG_STTFC, " ");
		return STT_FILE_ERROR_INVALID_STATE;
	}

	GSList *iter = NULL;
	sttengine_info_s *engine = NULL;

	int temp_old_engine = -1;

	/* Get a first item */
	iter = g_slist_nth(g_engine_list, 0);

	while (NULL != iter) {
		/* Get handle data from list */
		engine = iter->data;

		if (0 == strcmp(engine->engine_uuid, engine_id)) {
			if (client->current_engine_id != engine->engine_id) {
				temp_old_engine = client->current_engine_id;
			} else {
				SLOG(LOG_DEBUG, TAG_STTFC, "Already loaded engine : %s", engine_id);
				return 0;
			}

			break;
		}
	
		iter = g_slist_next(iter);
		engine = NULL;
	}
	
	if (NULL == engine) {
		SLOG(LOG_ERROR, TAG_STTFC, "[Engine Agent ERROR] Engine id is NOT valid");
		return STT_FILE_ERROR_INVALID_PARAMETER;
	}

	int ret = __stt_file_load_engine(engine);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTFC, "[Engine Agent ERROR] Fail to initialize engine");
		return STT_FILE_ERROR_OPERATION_FAILED;
	}

	client->current_engine_id = engine->engine_id;

	if (-1 != temp_old_engine) {
		stt_engine_deinitialize(temp_old_engine);
		stt_engine_unload(temp_old_engine);
	}

	SLOG(LOG_DEBUG, TAG_STTFC, "=====");
	SLOG(LOG_DEBUG, TAG_STTFC, " ");

	return 0;
}

bool __stt_config_supported_language_cb(const char* engine_id, const char* language, void* user_data)
{
	stt_file_client_s* client = stt_file_client_get();
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTFC, "[WARNING] A handle is not valid");
		return false;
	}

	/* call callback function */
	if (NULL != client->supported_lang_cb) {
		return client->supported_lang_cb(language, client->supported_lang_user_data);
	} else {
		SLOG(LOG_WARN, TAG_STTFC, "No registered callback function of supported languages");
	}

	return false;
}

int stt_file_foreach_supported_languages(stt_file_supported_language_cb callback, void* user_data)
{
	SLOG(LOG_DEBUG, TAG_STTFC, "===== Foreach Supported Language");

	if (NULL == callback) {
		SLOG(LOG_ERROR, TAG_STTFC, "[ERROR] Input parameter is NULL");
		SLOG(LOG_DEBUG, TAG_STTFC, "=====");
		SLOG(LOG_DEBUG, TAG_STTFC, " ");
		return STT_FILE_ERROR_INVALID_PARAMETER;
	}

	stt_file_client_s* client = stt_file_client_get();

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTFC, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_STTFC, "=====");
		SLOG(LOG_DEBUG, TAG_STTFC, " ");
		return STT_FILE_ERROR_INVALID_PARAMETER;
	}

	sttengine_info_s* engine = NULL;
	engine = __stt_file_get_engine_by_id(client->current_engine_id);
	if (NULL == engine) {
		SECURE_SLOG(LOG_ERROR, TAG_STTFC, "[ERROR] Fail to get engine info(%d)", client->current_engine_id);
		return STT_FILE_ERROR_OPERATION_FAILED;
	}

	client->supported_lang_cb = callback;
	client->supported_lang_user_data = user_data;

	int ret = -1;
	ret = stt_config_mgr_get_language_list(engine->engine_uuid, __stt_config_supported_language_cb, NULL);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTFC, "[ERROR] Fail to get languages");
	}

	client->supported_lang_cb = NULL;
	client->supported_lang_user_data = NULL;

	SLOG(LOG_DEBUG, TAG_STTFC, "=====");
	SLOG(LOG_DEBUG, TAG_STTFC, " ");

	return STT_FILE_ERROR_NONE;
}

int stt_file_start(const char* language, const char* type, const char* filepath, 
		stt_file_audio_type_e audio_type, int sample_rate)
{
	SLOG(LOG_DEBUG, TAG_STTFC, "===== STT FILE START");

	stt_file_client_s* client = stt_file_client_get();

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTFC, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_STTFC, "=====");
		SLOG(LOG_DEBUG, TAG_STTFC, " ");
		return STT_FILE_ERROR_INVALID_STATE;
	}

	/* check state */
	if (client->current_state != STT_FILE_STATE_READY) {
		SLOG(LOG_ERROR, TAG_STTFC, "[ERROR] Invalid State: Current state is not READY");
		SLOG(LOG_DEBUG, TAG_STTFC, "=====");
		SLOG(LOG_DEBUG, TAG_STTFC, " ");
		return STT_FILE_ERROR_INVALID_STATE;
	}

	if (NULL == language || NULL == filepath) {
		SLOG(LOG_ERROR, TAG_STTFC, "[ERROR] Input parameter is NULL");
		SLOG(LOG_DEBUG, TAG_STTFC, "=====");
		SLOG(LOG_DEBUG, TAG_STTFC, " ");
		return STT_FILE_ERROR_INVALID_PARAMETER;
	}

	/* check if engine use network */
	sttengine_info_s* engine = NULL;
	engine = __stt_file_get_engine_by_id(client->current_engine_id);
	if (NULL == engine) {
		SECURE_SLOG(LOG_ERROR, TAG_STTFC, "[ERROR] Fail to get engine info(%d)", client->current_engine_id);
		return STT_FILE_ERROR_OPERATION_FAILED;
	}

	if (true == engine->use_network) {
		if (false == stt_network_is_connected()) {
			SLOG(LOG_ERROR, TAG_STTFC, "[ERROR] Disconnect network. Current engine needs to network connection.");
			return STT_FILE_ERROR_OUT_OF_NETWORK;
		}
	}

	SLOG(LOG_DEBUG, TAG_STTFC, "[START Info] Engine(%d) Lang(%s) Type(%s) Filepath(%s) Audio(%d) Sample rate(%d)"
		,client->current_engine_id, language, type, filepath, audio_type, sample_rate);

	int ret = -1;
	ret = stt_engine_recognize_start_file(client->current_engine_id, language, type, filepath, audio_type, sample_rate, NULL);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTFC, "[ERROR] Fail to start file recognition");
		SLOG(LOG_DEBUG, TAG_STTFC, "=====");
		SLOG(LOG_DEBUG, TAG_STTFC, " ");
		return STT_FILE_ERROR_OPERATION_FAILED;
	}

	client->before_state = client->current_state;
	client->current_state = STT_FILE_STATE_PROCESSING;

	if (NULL != client->state_changed_cb) {
		stt_file_client_use_callback(client);
		client->state_changed_cb(client->before_state, client->current_state, client->state_changed_user_data); 
		stt_file_client_not_use_callback(client);
		SLOG(LOG_DEBUG, TAG_STTFC, "State changed callback is called");
	} else {
		SLOG(LOG_WARN, TAG_STTFC, "[WARNING] State changed callback is null");
	}

	SLOG(LOG_DEBUG, TAG_STTFC, "=====");
	SLOG(LOG_DEBUG, TAG_STTFC, " ");

	return STT_FILE_ERROR_NONE;
}

int stt_file_cancel()
{
	SLOG(LOG_DEBUG, TAG_STTFC, "===== STT FILE CANCEL");

	stt_file_client_s* client = stt_file_client_get();

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTFC, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_STTFC, "=====");
		SLOG(LOG_DEBUG, TAG_STTFC, " ");
		return STT_FILE_ERROR_INVALID_PARAMETER;
	} 	

	/* check state */
	if (STT_FILE_STATE_PROCESSING != client->current_state) {
		SLOG(LOG_ERROR, TAG_STTFC, "[ERROR] Invalid state : Current state is NOT 'Processing'");
		SLOG(LOG_DEBUG, TAG_STTFC, "=====");
		SLOG(LOG_DEBUG, TAG_STTFC, " ");
		return STT_FILE_ERROR_INVALID_STATE;
	}

	int ret = -1;
	ret = stt_engine_recognize_cancel_file(client->current_engine_id);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTFC, "[ERROR] Fail to cancel file recognition");
		SLOG(LOG_DEBUG, TAG_STTFC, "=====");
		SLOG(LOG_DEBUG, TAG_STTFC, " ");
		return STT_FILE_ERROR_OPERATION_FAILED;
	}

	client->before_state = client->current_state;
	client->current_state = STT_FILE_STATE_READY;

	if (NULL != client->state_changed_cb) {
		stt_file_client_use_callback(client);
		client->state_changed_cb(client->before_state, client->current_state, client->state_changed_user_data); 
		stt_file_client_not_use_callback(client);
		SLOG(LOG_DEBUG, TAG_STTFC, "State changed callback is called");
	} else {
		SLOG(LOG_WARN, TAG_STTFC, "[WARNING] State changed callback is null");
	}

	SLOG(LOG_DEBUG, TAG_STTFC, "=====");
	SLOG(LOG_DEBUG, TAG_STTFC, " ");

	return STT_FILE_ERROR_NONE;
}

bool __stt_file_result_time_cb(int index, sttp_result_time_event_e event, const char* text, long start_time, long end_time, void* user_data) 
{
	SLOG(LOG_DEBUG, TAG_STTFC, "(%d) event(%d) text(%s) start(%ld) end(%ld)",
			index, event, text, start_time, end_time);

	stt_file_client_s* client = stt_file_client_get();

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTFC, "[ERROR] Fail to notify error : A handle is not valid");
		return EINA_FALSE;
	}

	if (NULL != client->result_time_cb) {
		client->result_time_cb(index, (stt_file_result_time_event_e)event, 
			text, start_time, end_time, client->result_time_user_data);
	} else {
		SLOG(LOG_ERROR, TAG_STTFC, "[ERROR] Callback is NULL");
		return false;
	}

	return true;
}

int stt_file_foreach_detailed_result(stt_file_result_time_cb callback, void* user_data)
{
	SLOG(LOG_DEBUG, TAG_STTFC, "===== STT FILE FOREACH DETAILED RESULT");

	if (NULL == callback) {
		SLOG(LOG_ERROR, TAG_STTFC, "[ERROR] Input parameter is NULL");
		return STT_FILE_ERROR_INVALID_PARAMETER;
	}

	stt_file_client_s* client = stt_file_client_get();

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTFC, "[ERROR] Fail : A handle is not valid");
		return STT_FILE_ERROR_INVALID_PARAMETER;
	}

	client->result_time_cb = callback;
	client->result_time_user_data = user_data;

	stt_engine_foreach_result_time(client->current_engine_id, client->time_info, __stt_file_result_time_cb, NULL);

	client->result_time_cb = NULL;
	client->result_time_user_data = NULL;

	SLOG(LOG_DEBUG, TAG_STTFC, "=====");
	SLOG(LOG_DEBUG, TAG_STTFC, " ");

	return STT_FILE_ERROR_NONE;
}

int stt_file_set_recognition_result_cb(stt_file_recognition_result_cb callback, void* user_data)
{
	if (NULL == callback)
		return STT_FILE_ERROR_INVALID_PARAMETER;

	stt_file_client_s* client = stt_file_client_get();

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTFC, "[ERROR] A handle is not available");
		return STT_FILE_ERROR_INVALID_PARAMETER;
	}

	if (STT_FILE_STATE_READY != client->current_state) {
		SLOG(LOG_ERROR, TAG_STTFC, "[ERROR] Current state is not 'ready'");
		return STT_FILE_ERROR_INVALID_STATE;
	}

	client->recognition_result_cb = callback;
	client->recognition_result_user_data = user_data;

	return 0;
}

int stt_file_unset_recognition_result_cb()
{
	stt_file_client_s* client = stt_file_client_get();

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTFC, "[ERROR] A handle is not available");
		return STT_FILE_ERROR_INVALID_PARAMETER;
	}

	if (STT_FILE_STATE_READY != client->current_state) {
		SLOG(LOG_ERROR, TAG_STTFC, "[ERROR] Current state is not 'ready'");
		return STT_FILE_ERROR_INVALID_STATE;
	}

	client->recognition_result_cb = NULL;
	client->recognition_result_user_data = NULL;

	return 0;
}

int stt_file_set_state_changed_cb(stt_file_state_changed_cb callback, void* user_data)
{
	if (NULL == callback)
		return STT_FILE_ERROR_INVALID_PARAMETER;

	stt_file_client_s* client = stt_file_client_get();

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTFC, "[ERROR] A handle is not available");
		return STT_FILE_ERROR_INVALID_PARAMETER;
	}

	if (STT_FILE_STATE_READY != client->current_state) {
		SLOG(LOG_ERROR, TAG_STTFC, "[ERROR] Current state is not 'ready'");
		return STT_FILE_ERROR_INVALID_STATE;
	}

	client->state_changed_cb = callback;
	client->state_changed_user_data = user_data;

	return 0;
}

int stt_file_unset_state_changed_cb()
{
	stt_file_client_s* client = stt_file_client_get();

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTFC, "[ERROR] A handle is not available");
		return STT_FILE_ERROR_INVALID_PARAMETER;
	}

	if (STT_FILE_STATE_READY != client->current_state) {
		SLOG(LOG_ERROR, TAG_STTFC, "[ERROR] Current state is not 'ready'");
		return STT_FILE_ERROR_INVALID_STATE;
	}

	client->state_changed_cb = NULL;
	client->state_changed_user_data = NULL;

	return 0;
}
