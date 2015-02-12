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


#include <dlfcn.h>
#include <dirent.h>

#include "stt_defs.h"
#include "stt_engine.h"
#include "sttd_main.h"
#include "sttd_client_data.h"
#include "sttd_config.h"
#include "sttd_recorder.h"
#include "sttd_engine_agent.h"


#define AUDIO_CREATE_ON_START

/*
* Internal data structure
*/

typedef struct {
	int	uid;
	int	engine_id;
	bool	use_default_engine;
} sttengine_client_s;

typedef struct _sttengine_info {
	int	engine_id;

	char*	engine_uuid;
	char*	engine_path;
	char*	engine_name;
	char*	engine_setting_path;
	bool	use_network;

	bool	is_loaded;
	
	/* engine base setting */
	char*	first_lang;
	bool	silence_detection;
	bool	support_silence_detection;
} sttengine_info_s;

/** stt engine agent init */
static bool	g_agent_init;

/** list */
static GSList*	g_engine_client_list;
static GSList*	g_engine_list;

/** default engine info */
static int	g_default_engine_id;
static char*	g_default_language;
static bool	g_default_silence_detected;

static int	g_engine_id_count;

/** current engine id */
static int	g_recording_engine_id;

/** callback functions */
static result_callback g_result_cb;
static result_time_callback g_result_time_cb;
static silence_dectection_callback g_silence_cb;


/** callback functions */
void __result_cb(sttp_result_event_e event, const char* type, const char** data, int data_count, 
		 const char* msg, void* time_info, void *user_data);

bool __result_time_cb(int index, sttp_result_time_event_e event, const char* text, 
		      long start_time, long end_time, void* user_data);

void __detect_silence_cb(sttp_silence_type_e type, void* user_data);

bool __supported_language_cb(const char* language, void* user_data);

void __engine_info_cb(const char* engine_uuid, const char* engine_name, const char* setting_ug_name, 
		      bool use_network, void* user_data);

/*
* Internal Interfaces 
*/
 
/** get engine info */
int __internal_get_engine_info(const char* filepath, sttengine_info_s** info);

int __log_enginelist();

/*
* STT Engine Agent Interfaces
*/
int sttd_engine_agent_init(result_callback result_cb, result_time_callback time_cb, 
			   silence_dectection_callback silence_cb)
{
	/* initialize static data */
	if (NULL == result_cb || NULL == time_cb || NULL == silence_cb) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine agent ERROR] Invalid parameter"); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	g_result_cb = result_cb;
	g_result_time_cb = time_cb;
	g_silence_cb = silence_cb;

	g_default_engine_id = -1;
	g_default_language = NULL;
	g_engine_id_count = 1;
	g_recording_engine_id = -1;

	if (0 != sttd_config_get_default_language(&(g_default_language))) {
		SLOG(LOG_WARN, TAG_STTD, "[Engine Agent WARNING] There is No default voice in config"); 
		/* Set default voice */
		g_default_language = strdup("en_US");
	} else {
		SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] Default language is %s", g_default_language);
	}

	int temp;
	if (0 != sttd_config_get_default_silence_detection(&temp)) {
		SLOG(LOG_WARN, TAG_STTD, "[Server WARNING] There is no silence detection in config"); 
		g_default_silence_detected = true;
	} else {
		g_default_silence_detected = (bool)temp;
	}

	g_agent_init = false;

	return 0;
}

int __engine_agent_clear_engine(sttengine_info_s *engine)
{
	if (NULL != engine) {
		if (NULL != engine->engine_uuid)	free(engine->engine_uuid);
		if (NULL != engine->engine_path)	free(engine->engine_path);
		if (NULL != engine->engine_name)	free(engine->engine_name);
		if (NULL != engine->engine_setting_path)free(engine->engine_setting_path);
		if (NULL != engine->first_lang)		free(engine->first_lang);

		free(engine);
	}

	return 0;
}

int sttd_engine_agent_release()
{
	/* Release client list */
	GSList *iter = NULL;
	sttengine_client_s *client = NULL;

	if (g_slist_length(g_engine_client_list) > 0) {
		/* Get a first item */
		iter = g_slist_nth(g_engine_client_list, 0);

		while (NULL != iter) {
			/* Get handle data from list */
			client = iter->data;
			g_engine_client_list = g_slist_remove_link(g_engine_client_list, iter);

			if (NULL != client) 
				free(client);

			iter = g_slist_nth(g_engine_client_list, 0);
		}
	}

	g_slist_free(g_engine_client_list);

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
			if (engine->is_loaded) {
				SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] Unload engine id(%d)", engine->engine_id);

				if (0 != stt_engine_deinitialize(engine->engine_id)) 
					SECURE_SLOG(LOG_WARN, TAG_STTD, "[Engine Agent] Fail to deinitialize engine id(%d)", engine->engine_id);

				if (0 != stt_engine_unload(engine->engine_id))
					SECURE_SLOG(LOG_WARN, TAG_STTD, "[Engine Agent] Fail to unload engine id(%d)", engine->engine_id);

				engine->is_loaded = false;
			}

			__engine_agent_clear_engine(engine);

			iter = g_slist_nth(g_engine_list, 0);
		}
	}

	g_result_cb = NULL;
	g_silence_cb = NULL;

	g_agent_init = false;
	g_default_engine_id = -1;

	return 0;
}

void __engine_info_cb(const char* engine_uuid, const char* engine_name, const char* setting_ug_name, 
		      bool use_network, void* user_data)
{
	sttengine_info_s* temp = (sttengine_info_s*)user_data; 

	temp->engine_uuid = g_strdup(engine_uuid);
	temp->engine_name = g_strdup(engine_name);
	temp->engine_setting_path = g_strdup(setting_ug_name);
	temp->use_network = use_network;
}

int __internal_get_engine_info(const char* filepath, sttengine_info_s** info)
{
	if (NULL == filepath || NULL == info) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Invalid Parameter"); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	/* load engine */
	char *error;
	void* handle;

	handle = dlopen (filepath, RTLD_LAZY);

	if (!handle) {
		SECURE_SLOG(LOG_WARN, TAG_STTD, "[Engine Agent] Invalid engine : %s", filepath); 
		return -1;
	}

	/* link engine to daemon */
	dlsym(handle, "sttp_load_engine");
	if ((error = dlerror()) != NULL) {
		SLOG(LOG_WARN, TAG_STTD, "[Engine Agent] Invalid engine. Fail to open sttp_load_engine : %s", error); 
		dlclose(handle);
		return -1;
	}

	dlsym(handle, "sttp_unload_engine");
	if ((error = dlerror()) != NULL) {
		SLOG(LOG_WARN, TAG_STTD, "[Engine Agent] Invalid engine. Fail to open sttp_unload_engine : %s", error); 
		dlclose(handle);
		return -1;
	}

	int (*get_engine_info)(sttpe_engine_info_cb callback, void* user_data);

	get_engine_info = (int (*)(sttpe_engine_info_cb, void*))dlsym(handle, "sttp_get_engine_info");
	if ((error = dlerror()) != NULL || NULL == get_engine_info) {
		SLOG(LOG_WARN, TAG_STTD, "[Engine Agent WARNING] Invalid engine. Fail to open sttp_get_engine_info : %s", error); 
		dlclose(handle);
		return -1;
	}

	sttengine_info_s* temp;
	temp = (sttengine_info_s*)calloc(1, sizeof(sttengine_info_s));

	/* get engine info */
	if (0 != get_engine_info(__engine_info_cb, (void*)temp)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to get engine info from engine"); 
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

	SLOG(LOG_DEBUG, TAG_STTD, "----- Valid Engine");
	SECURE_SLOG(LOG_DEBUG, TAG_STTD, "Engine id : %d", temp->engine_id);
	SECURE_SLOG(LOG_DEBUG, TAG_STTD, "Engine uuid : %s", temp->engine_uuid);
	SECURE_SLOG(LOG_DEBUG, TAG_STTD, "Engine name : %s", temp->engine_name);
	SECURE_SLOG(LOG_DEBUG, TAG_STTD, "Engine path : %s", temp->engine_path);
	SECURE_SLOG(LOG_DEBUG, TAG_STTD, "Engine setting path : %s", temp->engine_setting_path);
	SECURE_SLOG(LOG_DEBUG, TAG_STTD, "Use network : %s", temp->use_network ? "true":"false");
	SLOG(LOG_DEBUG, TAG_STTD, "-----");
	SLOG(LOG_DEBUG, TAG_STTD, "  ");

	*info = temp;

	return 0;
}

bool __is_engine(const char* filepath)
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

int sttd_engine_agent_initialize_engine_list()
{
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
				SLOG(LOG_ERROR, TAG_STTD, "[File ERROR] Fail to read directory");
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
					SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Memory not enough!!");
					continue;
				}

				if (false  == __is_engine(filepath)) {
					/* get its info and update engine list */
					if (0 == __internal_get_engine_info(filepath, &info)) {
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
		SLOG(LOG_WARN, TAG_STTD, "[Engine Agent WARNING] Fail to open default directory"); 
	}

	/* Get file name from downloadable engine directory */
	dp  = opendir(STT_DOWNLOAD_ENGINE);
	if (NULL != dp) {
		do {
			ret = readdir_r(dp, &entry, &dirp);
			if (0 != ret) {
				SLOG(LOG_ERROR, TAG_STTD, "[File ERROR] Fail to read directory");
				break;
			}

			if (NULL != dirp) {
				sttengine_info_s* info;
				char* filepath;
				int filesize;

				filesize = strlen(STT_DOWNLOAD_ENGINE) + strlen(dirp->d_name) + 5;
				filepath = (char*)calloc(filesize, sizeof(char));

				if (NULL != filepath) {
					snprintf(filepath, filesize, "%s/%s", STT_DOWNLOAD_ENGINE, dirp->d_name);
				} else {
					SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Memory not enouth!!");
					continue;
				}

				/* get its info and update engine list */
				if (0 == __internal_get_engine_info(filepath, &info)) {
					/* add engine info to g_engine_list */
					g_engine_list = g_slist_append(g_engine_list, info);
				}

				if (NULL != filepath)
					free(filepath);
			}
		} while (NULL != dirp);

		closedir(dp);
	} else {
		SLOG(LOG_WARN, TAG_STTD, "[Engine Agent WARNING] Fail to open downloadable directory"); 
	}

	if (0 >= g_slist_length(g_engine_list)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] No Engine"); 
		return STTD_ERROR_ENGINE_NOT_FOUND;	
	}

	__log_enginelist();

	/* Set default engine */
	GSList *iter = NULL;
	sttengine_info_s *engine = NULL;
	char* cur_engine_uuid = NULL;
	bool is_default_engine = false;

	/* get current engine from config */
	if (0 == sttd_config_get_default_engine(&cur_engine_uuid)) {
		SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] current engine from config : %s", cur_engine_uuid);

		if (0 < g_slist_length(g_engine_list)) {
			/* Get a first item */
			iter = g_slist_nth(g_engine_list, 0);

			while (NULL != iter) {
				/* Get handle data from list */
				engine = iter->data;

				if (0 == strcmp(engine->engine_uuid, cur_engine_uuid)) {
					is_default_engine = true;
					g_default_engine_id = engine->engine_id;
					break;
				}

				iter = g_slist_next(iter);
			}
		}

		if (cur_engine_uuid != NULL )
			free(cur_engine_uuid);
	} else {
		SLOG(LOG_WARN, TAG_STTD, "[Engine Agent WARNING] There is not current engine from config"); 
	}

	if (false == is_default_engine) {
		if (0 < g_slist_length(g_engine_list)) {
			/* Get a first item */
			iter = g_slist_nth(g_engine_list, 0);

			/* Get handle data from list */
			engine = iter->data;

			if (NULL != engine) {
				is_default_engine = true;
				g_default_engine_id = engine->engine_id;
			}
		}
	}

	if (NULL != engine) {
		if (NULL != engine->engine_uuid) {
			SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] Default engine Id(%d) uuid(%s)", engine->engine_id, engine->engine_uuid);

			if (false == is_default_engine) {
				if (0 != sttd_config_set_default_engine(engine->engine_uuid))
					SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to set default engine "); 
			}
		}
	} else {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Default engine is NULL");
		return STTD_ERROR_ENGINE_NOT_FOUND;
	}

	g_agent_init = true;

	return 0;
}

sttengine_info_s* __engine_agent_get_engine_by_id(int engine_id)
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

sttengine_info_s* __engine_agent_get_engine_by_uuid(const char* engine_uuid)
{
	GSList *iter = NULL;
	sttengine_info_s *data = NULL;

	iter = g_slist_nth(g_engine_list, 0);

	while (NULL != iter) {

		data = iter->data;

		if (0 == strcmp(data->engine_uuid, engine_uuid)) 
			return data;

		iter = g_slist_next(iter);
	}

	return NULL;
}

sttengine_client_s* __engine_agent_get_client(int uid)
{
	GSList *iter = NULL;
	sttengine_client_s *data = NULL;

	if (0 < g_slist_length(g_engine_client_list)) {
		iter = g_slist_nth(g_engine_client_list, 0);

		while (NULL != iter) {
			/* Get handle data from list */
			data = iter->data;

			if (uid == data->uid) 
				return data;

			iter = g_slist_next(iter);
		}
	}

	return NULL;
}

sttengine_info_s* __engine_agent_get_engine_by_uid(int uid)
{
	sttengine_client_s *data;

	data = __engine_agent_get_client(uid);
	if (NULL != data) 
		return __engine_agent_get_engine_by_id(data->engine_id);

	return NULL;
}

int __engine_agent_check_engine_unload(int engine_id)
{
	/* Check the count of client to use this engine */
	GSList *iter = NULL;
	int client_count = 0;
	sttengine_client_s *data = NULL;
	
	if (0 < g_slist_length(g_engine_client_list)) {
		iter = g_slist_nth(g_engine_client_list, 0);

		while (NULL != iter) {
			/* Get handle data from list */
			data = iter->data;

			if (data->engine_id == engine_id)
				client_count++;

			iter = g_slist_next(iter);
		}
	}

	if (0 == client_count) {
		sttengine_info_s* engine = NULL;
		engine = __engine_agent_get_engine_by_id(engine_id);
		if (NULL == engine) {
			SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to get engine from client(%d)", engine_id);
		} else {
			if (engine->is_loaded) {
				/* unload engine */
#ifndef AUDIO_CREATE_ON_START
				SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] Destroy recorder");
				if (0 != sttd_recorder_destroy(engine->engine_id))
					SECURE_SLOG(LOG_WARN, TAG_STTD, "[Engine Agent] Fail to destroy recorder(%d)", engine->engine_id);
#endif
				SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] Unload engine id(%d)", engine_id);
				if (0 != stt_engine_deinitialize(engine->engine_id))
					SECURE_SLOG(LOG_WARN, TAG_STTD, "[Engine Agent] Fail to deinitialize engine id(%d)", engine->engine_id);

				if (0 != stt_engine_unload(engine->engine_id))
					SECURE_SLOG(LOG_WARN, TAG_STTD, "[Engine Agent] Fail to unload engine id(%d)", engine->engine_id);

				engine->is_loaded = false;
			}
		}
	}

	return 0;
}

int sttd_engine_agent_load_current_engine(int uid, const char* engine_uuid)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized" );
		return STTD_ERROR_OPERATION_FAILED;
	}

	sttengine_client_s* client = NULL;
	sttengine_info_s* engine = NULL;
	int before_engine = -1;

	client = __engine_agent_get_client(uid);
	
	if (NULL == client) {
		client = (sttengine_client_s*)calloc(1, sizeof(sttengine_client_s));
		
		/* initialize */
		client->uid = uid;
		client->engine_id = -1;
	
		g_engine_client_list = g_slist_append(g_engine_client_list, client);

		SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] Registered client(%d)", uid);
	} 

	if (NULL == engine_uuid) {
		/* Set default engine */
		engine = __engine_agent_get_engine_by_id(g_default_engine_id);

		if (NULL == engine) {
			SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to get default engine : %d", g_default_engine_id);
			return STTD_ERROR_OPERATION_FAILED;
		}
		before_engine = client->engine_id;

		client->engine_id = engine->engine_id;
		client->use_default_engine = true;
	} else {
		/* Set engine by uid */
		engine = __engine_agent_get_engine_by_uuid(engine_uuid);

		if (NULL == engine) {
			SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to get engine : %s", engine_uuid);
			return STTD_ERROR_OPERATION_FAILED;
		}
		before_engine = client->engine_id;

		client->engine_id = engine->engine_id;
		client->use_default_engine = false;
	}

	if (-1 != before_engine) {
		/* Unload engine if reference count is 0 */
		__engine_agent_check_engine_unload(before_engine);
	}

	if (true == engine->is_loaded) {
		SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[Engine] engine id(%d) is already loaded", engine->engine_id);
		return 0;
	}

	/* Load engine */
	int ret;
	ret = stt_engine_load(engine->engine_id, engine->engine_path);
	if (0 != ret) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to load engine : id(%d) path(%s)", engine->engine_id, engine->engine_path);
		return STTD_ERROR_OPERATION_FAILED;
	}

	ret = stt_engine_initialize(engine->engine_id, __result_cb, __detect_silence_cb);
	if (0 != ret) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to initialize engine : id(%d) path(%s)", engine->engine_id, engine->engine_path);
		return STTD_ERROR_OPERATION_FAILED;
	}

	ret = stt_engine_set_silence_detection(engine->engine_id, g_default_silence_detected);
	if (0 != ret) {
		SLOG(LOG_WARN, TAG_STTD, "[Engine Agent WARNING] Not support silence detection");
		engine->support_silence_detection = false;
	} else {
		SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] Silence detection : %s", g_default_silence_detected ? "true" : "false");
		engine->support_silence_detection = true;
		engine->silence_detection = g_default_silence_detected;
	}

	/* Set first language */
	char* tmp_lang = NULL;
	ret = stt_engine_get_first_language(engine->engine_id, &tmp_lang);
	if (0 == ret && NULL != tmp_lang) {
		engine->first_lang = strdup(tmp_lang);
		free(tmp_lang);
	} else {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to get first language from engine : %d %s", engine->engine_id, engine->engine_name);
		return STTD_ERROR_OPERATION_FAILED;
	}

#ifndef AUDIO_CREATE_ON_START
	/* Ready recorder */
	sttp_audio_type_e atype;
	int rate;
	int channels;

	ret = stt_engine_get_audio_type(engine->engine_id, &atype, &rate, &channels);
	if (0 != ret) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to get audio type : %d %s", engine->engine_id, engine->engine_name);
		return STTD_ERROR_OPERATION_FAILED;
	}

	ret = sttd_recorder_create(engine->engine_id, atype, channels, rate);
	if (0 != ret) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to create recorder : %d %s", engine->engine_id, engine->engine_name);
		return STTD_ERROR_OPERATION_FAILED;
	}
#endif

	engine->is_loaded = true;
	SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent SUCCESS] The %s(%d) has been loaded !!!", engine->engine_name, engine->engine_id); 

	return 0;
}

int sttd_engine_agent_unload_current_engine(int uid)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized "); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	/* Remove client */
	int engine_id = -1;

	GSList *iter = NULL;
	sttengine_client_s *data = NULL;

	if (0 < g_slist_length(g_engine_client_list)) {
		iter = g_slist_nth(g_engine_client_list, 0);

		while (NULL != iter) {
			/* Get handle data from list */
			data = iter->data;

			if (NULL != data) {
				if (uid == data->uid) {
					g_engine_client_list = g_slist_remove_link(g_engine_client_list, iter);
					engine_id = data->engine_id;
					free(data);
					break;
				}
			}

			iter = g_slist_next(iter);
		}
	}

	if (-1 != engine_id) {
		__engine_agent_check_engine_unload(engine_id);
	}

	return 0;
}

bool sttd_engine_agent_is_default_engine()
{
	if (g_default_engine_id > 0) 
		return true;

	return false;
}

int sttd_engine_agent_get_engine_list(GSList** engine_list)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	GSList *iter = NULL;
	sttengine_info_s *data = NULL;

	iter = g_slist_nth(g_engine_list, 0);

	SLOG(LOG_DEBUG, TAG_STTD, "----- [Engine Agent] engine list -----");

	while (NULL != iter) {
		engine_s* temp_engine;

		temp_engine = (engine_s*)calloc(1, sizeof(engine_s));

		data = iter->data;

		temp_engine->engine_id = strdup(data->engine_uuid);
		temp_engine->engine_name = strdup(data->engine_name);
		temp_engine->ug_name = strdup(data->engine_setting_path);

		*engine_list = g_slist_append(*engine_list, temp_engine);

		iter = g_slist_next(iter);

		SECURE_SLOG(LOG_DEBUG, TAG_STTD, " -- Engine id(%s)", temp_engine->engine_id); 
		SECURE_SLOG(LOG_DEBUG, TAG_STTD, "    Engine name(%s)", temp_engine->engine_name);
		SECURE_SLOG(LOG_DEBUG, TAG_STTD, "    Engine ug name(%s)", temp_engine->ug_name);
	}

	SLOG(LOG_DEBUG, TAG_STTD, "--------------------------------------");

	return 0;
}

int sttd_engine_agent_get_current_engine(int uid, char** engine_uuid)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized" );
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (NULL == engine_uuid) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Invalid parameter" );
		return STTD_ERROR_INVALID_PARAMETER;
	}

	sttengine_info_s* engine;
	engine = __engine_agent_get_engine_by_uid(uid);

	if (NULL == engine) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] The engine of uid(%d) is not valid", uid);
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (false == engine->is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not loaded engine"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	*engine_uuid = strdup(engine->engine_uuid);

	return 0;
}

bool sttd_engine_agent_need_network(int uid)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized" );
		return STTD_ERROR_OPERATION_FAILED;
	}

	sttengine_info_s* engine;
	engine = __engine_agent_get_engine_by_uid(uid);
	
	if (NULL != engine)
		return engine->use_network;

	return false;
}

int sttd_engine_agent_supported_langs(int uid, GSList** lang_list)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized");
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (NULL == lang_list) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Input parameter is NULL");
		return STTD_ERROR_INVALID_PARAMETER;
	}

	sttengine_info_s* engine = NULL;
	engine = __engine_agent_get_engine_by_uid(uid);

	if (NULL == engine) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] The engine of uid(%d) is not valid", uid);
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (false == engine->is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not loaded engine"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	int ret = stt_engine_get_supported_langs(engine->engine_id, lang_list);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] get language list error(%d)", ret); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	return 0;
}

int sttd_engine_agent_get_default_lang(int uid, char** lang)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (NULL == lang) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Invalid Parameter"); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	sttengine_info_s* engine = NULL;
	engine = __engine_agent_get_engine_by_uid(uid);

	if (NULL == engine) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] The engine of uid(%d) is not valid", uid);
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (false == engine->is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not loaded engine"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	/* get default language */
	bool is_valid = false;
	if (0 != stt_engine_is_valid_language(engine->engine_id, g_default_language, &is_valid)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to check valid language");
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (true == is_valid) {
		*lang = strdup(g_default_language);
	} else 
		*lang = strdup(engine->first_lang);

	return 0;
}

int sttd_engine_agent_get_option_supported(int uid, bool* silence)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized");
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (NULL == silence) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Invalid Parameter"); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	sttengine_info_s* engine = NULL;
	engine = __engine_agent_get_engine_by_uid(uid);

	if (NULL == engine) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] uid(%d) is not valid", uid);
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (false == engine->is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not loaded engine"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	*silence = engine->support_silence_detection;

	return 0;
}

int sttd_engine_agent_is_recognition_type_supported(int uid, const char* type, bool* support)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (NULL == type || NULL == support) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Invalid Parameter"); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	sttengine_info_s* engine = NULL;
	engine = __engine_agent_get_engine_by_uid(uid);

	if (NULL == engine) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] The engine of uid(%d) is not valid", uid);
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (false == engine->is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not loaded engine"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	bool temp = false;
	int ret;

	ret = stt_engine_support_recognition_type(engine->engine_id, type, &temp);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to get to support recognition type : %d", ret); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	*support = temp;

	return 0;
}

/*
* STT Engine Interfaces for client
*/

int __set_option(sttengine_info_s* engine, int silence)
{
	if (NULL == engine)
		return -1;

	/* Check silence detection */
	if (engine->support_silence_detection) {
		if (2 == silence) {
			/* default option set */
			if (g_default_silence_detected != engine->silence_detection) {
				if (0 != stt_engine_set_silence_detection(engine->engine_id, g_default_silence_detected)) {
					SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to set silence detection : %s", g_default_silence_detected ? "true" : "false");
				} else {
					engine->silence_detection = g_default_silence_detected;
					SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] Set silence detection : %s", g_default_silence_detected ? "true" : "false");
				}
			}
		} else {
			if (silence != engine->silence_detection) {
				if (0 != stt_engine_set_silence_detection(engine->engine_id, silence)) {
					SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to set silence detection : %s", silence ? "true" : "false");
				} else {
					engine->silence_detection = silence;
					SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] Set silence detection : %s", silence ? "true" : "false");
				}
			}
		}
	}
	
	return 0;
}

int sttd_engine_agent_recognize_start_engine(int uid, const char* lang, const char* recognition_type, 
				      int silence, void* user_param)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized"); 
		return STTD_ERROR_OPERATION_FAILED;
	}
	
	if (NULL == lang || NULL == recognition_type) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Invalid Parameter"); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	sttengine_info_s* engine = NULL;
	engine = __engine_agent_get_engine_by_uid(uid);

	if (NULL == engine) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] The engine of uid(%d) is not valid", uid);
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (false == engine->is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not loaded engine"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (0 != __set_option(engine, silence)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to set options"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	SLOG(LOG_DEBUG, TAG_STTD, "g_default_language %s", g_default_language);

	int ret;
	char* temp = NULL;
	if (0 == strncmp(lang, "default", strlen("default"))) {
		bool is_valid = false;
		if (0 != stt_engine_is_valid_language(engine->engine_id, g_default_language, &is_valid)) {
			SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to check valid language");
			return STTD_ERROR_OPERATION_FAILED;
		}

		if (true == is_valid) {
			temp = strdup(g_default_language);
			SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent DEBUG] Default language is %s", temp);
		} else {
			temp = strdup(engine->first_lang);
			SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent DEBUG] Default language is engine first lang : %s", temp);
		}
	} else {
		temp = strdup(lang);
	}

	SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] Start engine");

	ret = stt_engine_recognize_start(engine->engine_id, temp, recognition_type, user_param);
	if (NULL != temp)	free(temp);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Recognition start error(%d)", ret);
		sttd_recorder_destroy(engine->engine_id);
		return STTD_ERROR_OPERATION_FAILED;
	}

#ifdef AUDIO_CREATE_ON_START
	/* Ready recorder */
	sttp_audio_type_e atype;
	int rate;
	int channels;

	ret = stt_engine_get_audio_type(engine->engine_id, &atype, &rate, &channels);
	if (0 != ret) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to get audio type : %d %s", engine->engine_id, engine->engine_name);
		return STTD_ERROR_OPERATION_FAILED;
	}

	SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] Create recorder");

	ret = sttd_recorder_create(engine->engine_id, atype, channels, rate);
	if (0 != ret) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to create recorder : %d %s", engine->engine_id, engine->engine_name);
		return STTD_ERROR_OPERATION_FAILED;
	}
#endif

#if 0
	SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] Start recorder");

	ret = sttd_recorder_start(engine->engine_id);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to start recorder : result(%d)", ret);
		return ret;
	}

	g_recording_engine_id = engine->engine_id;
	SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] g_recording_engine_id : %d", g_recording_engine_id); 
#endif

	return 0;
}

int sttd_engine_agent_recognize_start_recorder(int uid)
{
	sttengine_info_s* engine = NULL;
	engine = __engine_agent_get_engine_by_uid(uid);

	if (NULL == engine) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] The engine of uid(%d) is not valid", uid);
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (false == engine->is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not loaded engine"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] Start recorder");

	int ret;
	ret = sttd_recorder_start(engine->engine_id);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to start recorder : result(%d)", ret);
		stt_engine_recognize_cancel(engine->engine_id);
		sttd_recorder_stop(engine->engine_id);
		return ret;
	}

	g_recording_engine_id = engine->engine_id;
	SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] g_recording_engine_id : %d", g_recording_engine_id); 

	return 0;
}

int sttd_engine_agent_set_recording_data(int uid, const void* data, unsigned int length)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (NULL == data || 0 == length) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Invalid Parameter"); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	sttengine_info_s* engine = NULL;
	engine = __engine_agent_get_engine_by_uid(uid);

	if (NULL == engine) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] The engine of uid(%d) is not valid", uid);
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (false == engine->is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not loaded engine"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	int ret = stt_engine_set_recording_data(engine->engine_id, data, length);
	if (0 != ret) {
		SLOG(LOG_WARN, TAG_STTD, "[Engine Agent WARNING] set recording error(%d)", ret); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	return 0;
}

int sttd_engine_agent_recognize_stop_recorder(int uid)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	sttengine_info_s* engine = NULL;
	engine = __engine_agent_get_engine_by_uid(uid);

	if (NULL == engine) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] The engine of uid(%d) is not valid", uid);
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (false == engine->is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not loaded engine"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] Stop recorder");
	int ret;
	ret = sttd_recorder_stop(engine->engine_id);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to stop recorder : result(%d)", ret); 
		return STTD_ERROR_OPERATION_FAILED;
	}

#ifdef AUDIO_CREATE_ON_START
	SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] Destroy recorder");
	if (0 != sttd_recorder_destroy(engine->engine_id))
		SECURE_SLOG(LOG_WARN, TAG_STTD, "[Engine Agent] Fail to destroy recorder(%d)", engine->engine_id);
#endif

	SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent Success] Stop recorder");
	return 0;
}

int sttd_engine_agent_recognize_stop_engine(int uid)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	sttengine_info_s* engine = NULL;
	engine = __engine_agent_get_engine_by_uid(uid);

	if (NULL == engine) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] The engine of uid(%d) is not valid", uid);
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (false == engine->is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not loaded engine"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] Stop engine");

	int ret;
	ret = stt_engine_recognize_stop(engine->engine_id);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] stop recognition error(%d)", ret); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent Success] Stop engine");

	return 0;
}

int sttd_engine_agent_recognize_cancel(int uid)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	sttengine_info_s* engine = NULL;
	engine = __engine_agent_get_engine_by_uid(uid);

	if (NULL == engine) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] The engine of uid(%d) is not valid", uid);
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (false == engine->is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not loaded engine"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] Cancel engine");

	int ret;
	ret = stt_engine_recognize_cancel(engine->engine_id);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] cancel recognition error(%d)", ret); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] Stop recorder");

	ret = sttd_recorder_stop(engine->engine_id);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to stop recorder : result(%d)", ret); 
		return STTD_ERROR_OPERATION_FAILED;
	}

#ifdef AUDIO_CREATE_ON_START
	SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] Destroy recorder");
	if (0 != sttd_recorder_destroy(engine->engine_id))
		SECURE_SLOG(LOG_WARN, TAG_STTD, "[Engine Agent] Fail to destroy recorder(%d)", engine->engine_id);
#endif

	g_recording_engine_id = -1;

	SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent Success] Cancel recognition");

	return 0;
}


/*
* STT Engine Interfaces for configure
*/

int sttd_engine_agent_set_default_engine(const char* engine_uuid)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (NULL == engine_uuid) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Invalid Parameter"); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	__log_enginelist();

	sttengine_info_s* engine;
	engine = __engine_agent_get_engine_by_uuid(engine_uuid);
	if (NULL == engine) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Default engine is not valid");
		return STTD_ERROR_ENGINE_NOT_FOUND;
	}

	SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] Default engine id(%d) engine uuid(%s)", engine->engine_id, engine->engine_uuid);

	g_default_engine_id = engine->engine_id;

	/* Update default engine of client */
	GSList *iter = NULL;
	sttengine_client_s *data = NULL;

	if (0 < g_slist_length(g_engine_client_list)) {
		iter = g_slist_nth(g_engine_client_list, 0);

		while (NULL != iter) {
			/* Get handle data from list */
			data = iter->data;

			if (true == data->use_default_engine) {
				SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] uid(%d) change engine from id(%d) to id(%d)", 
					data->uid, data->engine_id, engine->engine_id);

				if (0 != sttd_engine_agent_load_current_engine(data->uid, NULL)) {
					SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to load current engine : uid(%d)", data->uid);
				}
			}

			iter = g_slist_next(iter);
		}
	}

	return 0;
}

int sttd_engine_agent_set_default_language(const char* language)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (NULL == language) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Invalid Parameter"); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (NULL != g_default_language)
		free(g_default_language);

	g_default_language = strdup(language);

	return 0;
}

int sttd_engine_agent_set_silence_detection(bool value)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	g_default_silence_detected = value;

	return 0;
}

int sttd_engine_agent_check_app_agreed(int uid, const char* appid, bool* result)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	sttengine_info_s* engine = NULL;
	engine = __engine_agent_get_engine_by_uid(uid);

	if (NULL == engine) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] The engine of uid(%d) is not valid", uid);
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (false == engine->is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not loaded engine"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	int ret;
	ret = stt_engine_check_app_agreed(engine->engine_id, appid, result);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] cancel recognition error(%d)", ret); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	
	SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] Get engine right : %s", *result ? "true" : "false");
	return 0;
}

/*
* STT Engine Callback Functions											`				  *
*/

void __result_cb(sttp_result_event_e event, const char* type, const char** data, int data_count, 
		 const char* msg, void* time_info, void *user_data)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Result Callback : Not Initialized");
		return;
	}

	SLOG(LOG_DEBUG, TAG_STTD, "[Server] === Result time callback ===");

	if (NULL != time_info) {
		/* Get the time info */
		int ret = stt_engine_foreach_result_time(g_recording_engine_id, time_info, __result_time_cb, NULL);
		if (0 != ret) {
			SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to get time info : %d", ret);
		}
	}

	SLOG(LOG_DEBUG, TAG_STTD, "[Server] ============================");

	g_result_cb(event, type, data, data_count, msg, user_data);

#ifdef AUDIO_CREATE_ON_START
	if (event == STTP_RESULT_EVENT_ERROR) {
		SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] Destroy recorder");
		if (0 != sttd_recorder_destroy(g_recording_engine_id))
			SECURE_SLOG(LOG_WARN, TAG_STTD, "[Engine Agent] Fail to destroy recorder(%d)", g_recording_engine_id);
	}
#endif

	if (event == STTP_RESULT_EVENT_FINAL_RESULT || event == STTP_RESULT_EVENT_ERROR) {
		g_recording_engine_id = -1;
	}

	return;
}

bool __result_time_cb(int index, sttp_result_time_event_e event, const char* text, long start_time, long end_time, void* user_data)
{
	return g_result_time_cb(index, event, text, start_time, end_time, user_data);
}

void __detect_silence_cb(sttp_silence_type_e type, void* user_data)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Silence Callback : Not Initialized"); 
		return;
	}

	g_silence_cb(type, user_data);
	return;
}

/* A function forging */
int __log_enginelist()
{
	GSList *iter = NULL;
	sttengine_info_s *data = NULL;

	if (0 < g_slist_length(g_engine_list)) {

		/* Get a first item */
		iter = g_slist_nth(g_engine_list, 0);

		SLOG(LOG_DEBUG, TAG_STTD, "--------------- engine list -------------------");

		int i = 1;	
		while (NULL != iter) {
			/* Get handle data from list */
			data = iter->data;

			SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[%dth]", i);
			SECURE_SLOG(LOG_DEBUG, TAG_STTD, "  engine uuid : %s", data->engine_uuid);
			SECURE_SLOG(LOG_DEBUG, TAG_STTD, "  engine name : %s", data->engine_name);
			SECURE_SLOG(LOG_DEBUG, TAG_STTD, "  engine path : %s", data->engine_path);
			SECURE_SLOG(LOG_DEBUG, TAG_STTD, "  use network : %s", data->use_network ? "true" : "false");
			SECURE_SLOG(LOG_DEBUG, TAG_STTD, "  is loaded : %s", data->is_loaded ? "true" : "false");
			if (NULL != data->first_lang)
				SECURE_SLOG(LOG_DEBUG, TAG_STTD, "  default lang : %s", data->first_lang);

			iter = g_slist_next(iter);
			i++;
		}
		SLOG(LOG_DEBUG, TAG_STTD, "----------------------------------------------");
	} else {
		SLOG(LOG_DEBUG, TAG_STTD, "-------------- engine list -------------------");
		SLOG(LOG_DEBUG, TAG_STTD, "  No Engine in engine directory");
		SLOG(LOG_DEBUG, TAG_STTD, "----------------------------------------------");
	}

	return 0;
}