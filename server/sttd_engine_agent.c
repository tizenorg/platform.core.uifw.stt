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


#include <dlfcn.h>
#include <dirent.h>

#include "sttd_main.h"
#include "sttd_client_data.h"
#include "sttd_config.h"
#include "sttd_engine_agent.h"


/*
* Internal data structure
*/

typedef struct {
	/* engine info */
	char*	engine_uuid;
	char*	engine_name; 
	char*	engine_path;

	/* engine load info */
	bool	is_set;
	bool	is_loaded;	
	bool	need_network;
	bool	support_silence_detection;
	bool	support_profanity_filter;
	bool	support_punctuation_override;
	void	*handle;

	/* engine base setting */
	char*	default_lang;
	bool	profanity_filter;
	bool	punctuation_override;
	bool	silence_detection;

	sttpe_funcs_s*	pefuncs;
	sttpd_funcs_s*	pdfuncs;

	int (*sttp_load_engine)(sttpd_funcs_s* pdfuncs, sttpe_funcs_s* pefuncs);
	int (*sttp_unload_engine)();
} sttengine_s;

typedef struct _sttengine_info {
	char*	engine_uuid;
	char*	engine_path;
	char*	engine_name;
	char*	setting_ug_path;
	bool	use_network;
	bool	support_silence_detection;
} sttengine_info_s;


/*
* static data
*/

/** stt engine agent init */
static bool g_agent_init;

/** stt engine list */
static GList *g_engine_list;		

/** current engine infomation */
static sttengine_s g_cur_engine;

/** default option value */
static bool g_default_profanity_filter;
static bool g_default_punctuation_override;
static bool g_default_silence_detected;

/** callback functions */
static result_callback g_result_cb;
static partial_result_callback g_partial_result_cb;
static silence_dectection_callback g_silence_cb;


/** callback functions */
void __result_cb(sttp_result_event_e event, const char* type, 
			const char** data, int data_count, const char* msg, void *user_data);

void __partial_result_cb(sttp_result_event_e event, const char* data, void *user_data);

void __detect_silence_cb(void* user_data);

bool __supported_language_cb(const char* language, void* user_data);

void __engine_info_cb(const char* engine_uuid, const char* engine_name, const char* setting_ug_name, 
		      bool use_network, void* user_data);

bool __engine_setting_cb(const char* key, const char* value, void* user_data);

/** Free voice list */
void __free_language_list(GList* lang_list);


/*
* Internal Interfaces 
*/
 
/** Set current engine */
int __internal_set_current_engine(const char* engine_uuid);

/** check engine id */
int __internal_check_engine_id(const char* engine_uuid);

/** update engine list */
int __internal_update_engine_list();

/** get engine info */
int __internal_get_engine_info(const char* filepath, sttengine_info_s** info);

int __log_enginelist();

/*
* STT Engine Agent Interfaces
*/
int sttd_engine_agent_init(result_callback result_cb, partial_result_callback partial_result_cb, silence_dectection_callback silence_cb)
{
	/* initialize static data */
	if (NULL == result_cb || NULL == silence_cb) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine agent ERROR] sttd_engine_agent_init : invalid parameter"); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	g_result_cb = result_cb;
	g_partial_result_cb = partial_result_cb;
	g_silence_cb = silence_cb;

	g_cur_engine.engine_uuid = NULL;
	g_cur_engine.engine_name = NULL;
	g_cur_engine.engine_path = NULL;

	g_cur_engine.is_set = false;
	g_cur_engine.handle = NULL;
	g_cur_engine.pefuncs = (sttpe_funcs_s*)malloc( sizeof(sttpe_funcs_s) );
	g_cur_engine.pdfuncs = (sttpd_funcs_s*)malloc( sizeof(sttpd_funcs_s) );

	g_agent_init = true;

	if (0 != sttd_config_get_default_language(&(g_cur_engine.default_lang))) {
		SLOG(LOG_WARN, TAG_STTD, "[Server WARNING] There is No default voice in config"); 
		/* Set default voice */
		g_cur_engine.default_lang = strdup("en_US");
	}

	int temp;
	if (0 != sttd_config_get_default_silence_detection(&temp)) {
		SLOG(LOG_WARN, TAG_STTD, "[Server WARNING] There is no silence detection in config"); 
		g_default_silence_detected = true;
	} else {
		g_default_silence_detected = (bool)temp;
	}

	if (0 != sttd_config_get_default_profanity_filter(&temp)) {
		SLOG(LOG_WARN, TAG_STTD, "[Server WARNING] There is no profanity filter in config"); 
		g_default_profanity_filter = false;
	} else {
		g_default_profanity_filter = (bool)temp;
	}

	if (0 != sttd_config_get_default_punctuation_override(&temp)) {
		SLOG(LOG_WARN, TAG_STTD, "[Server WARNING] There is no punctuation override in config"); 
		g_default_punctuation_override = false;
	} else {
		g_default_punctuation_override = (bool)temp;
	}

	SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent SUCCESS] Engine Agent Initialize"); 

	return 0;
}

int sttd_engine_agent_release()
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	/* unload current engine */
	sttd_engine_agent_unload_current_engine();

	/* release engine list */
	GList *iter = NULL;
	sttengine_s *data = NULL;

	if (g_list_length(g_engine_list) > 0) {
		/* Get a first item */
		iter = g_list_first(g_engine_list);

		while (NULL != iter) {
			/* Get handle data from list */
			data = iter->data;

			iter = g_list_remove(iter, data);
		}
	}

	g_list_free(iter);
	
	/* release current engine data */
	if( NULL != g_cur_engine.pefuncs )
		free(g_cur_engine.pefuncs);

	if( NULL != g_cur_engine.pdfuncs )
		free(g_cur_engine.pdfuncs);

	g_result_cb = NULL;
	g_silence_cb = NULL;

	g_agent_init = false;

	SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent SUCCESS] Engine Agent release"); 

	return 0;
}

int sttd_engine_agent_initialize_current_engine()
{
	/* check agent init */
	if (false == g_agent_init ) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	/* update engine list */
	if (0 != __internal_update_engine_list()) {
		SLOG(LOG_ERROR, TAG_STTD, "[engine agent] sttd_engine_agent_init : __internal_update_engine_list : no engine error"); 
		return STTD_ERROR_ENGINE_NOT_FOUND;
	}

	/* get current engine from config */
	char* cur_engine_uuid = NULL;
	bool is_get_engineid_from_config = false;

	if (0 != sttd_config_get_default_engine(&cur_engine_uuid)) {

		SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] There is not current engine from config"); 

		/* not set current engine */
		/* set system default engine */
		GList *iter = NULL;
		if (0 < g_list_length(g_engine_list)) {
			iter = g_list_first(g_engine_list);
		} else {
			SLOG(LOG_ERROR, TAG_STTD, "[engine agent ERROR] sttd_engine_agent_initialize_current_engine() : no engine error"); 
			return -1;	
		}

		sttengine_info_s *data = NULL;
		data = iter->data;

		cur_engine_uuid = g_strdup(data->engine_uuid);

		is_get_engineid_from_config = false;
	} else {
		SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] current engine from config : %s", cur_engine_uuid); 

		is_get_engineid_from_config = true;
	}

	/* check whether cur engine uuid is valid or not. */
	if (0 != __internal_check_engine_id(cur_engine_uuid)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] It is not valid engine id and find other engine id");

		GList *iter = NULL;
		if (0 < g_list_length(g_engine_list)) {
			iter = g_list_first(g_engine_list);
		} else {
			SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] sttd_engine_agent_initialize_current_engine() : no engine error"); 
			return -1;	
		}

		if (NULL != cur_engine_uuid)	
			free(cur_engine_uuid);
		
		sttengine_info_s *data = NULL;
		data = iter->data;

		cur_engine_uuid = g_strdup(data->engine_uuid);
		
		is_get_engineid_from_config = false;
	}

	SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] Current Engine Id : %s", cur_engine_uuid);

	/* set current engine */
	if (0 != __internal_set_current_engine(cur_engine_uuid)) {
		SLOG(LOG_ERROR, TAG_STTD, "[engine agent ERROR] __internal_set_current_engine : no engine error"); 

		if( cur_engine_uuid != NULL)	
			free(cur_engine_uuid);

		return STTD_ERROR_ENGINE_NOT_FOUND;
	}

	if (false == is_get_engineid_from_config) {
		if (0 != sttd_config_set_default_engine(cur_engine_uuid))
			SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to set default engine "); 
	}

	if( cur_engine_uuid != NULL )	
		free(cur_engine_uuid);

	return 0;
}

int __internal_check_engine_id(const char* engine_uuid)
{
	if (NULL == engine_uuid) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Invalid Parameter"); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	GList *iter = NULL;
	sttengine_s *data = NULL;

	if (0 < g_list_length(g_engine_list)) {
		/*Get a first item*/
		iter = g_list_first(g_engine_list);

		while (NULL != iter) {
			data = iter->data;
			
			if (0 == strncmp(engine_uuid, data->engine_uuid, strlen(data->engine_uuid))) {
				return 0;
			}

			iter = g_list_next(iter);
		}
	}

	return -1;
}

void __engine_info_cb(const char* engine_uuid, const char* engine_name, const char* setting_ug_name, 
		      bool use_network, void* user_data)
{
	sttengine_info_s* temp = (sttengine_info_s*)user_data; 

	temp->engine_uuid = g_strdup(engine_uuid);
	temp->engine_name = g_strdup(engine_name);
	temp->setting_ug_path = g_strdup(setting_ug_name);
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
		SLOG(LOG_WARN, TAG_STTD, "[Engine Agent] Invalid engine : %s", filepath); 
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
	temp = (sttengine_info_s*)g_malloc0( sizeof(sttengine_info_s) );

	/* get engine info */
	if (0 != get_engine_info(__engine_info_cb, (void*)temp)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to get engine info from engine"); 
		dlclose(handle);
		g_free(temp);
		return -1;
	}

	/* close engine */
	dlclose(handle);

	temp->engine_path = g_strdup(filepath);

	SLOG(LOG_DEBUG, TAG_STTD, "----- Valid Engine");
	SLOG(LOG_DEBUG, TAG_STTD, "Engine uuid : %s", temp->engine_uuid);
	SLOG(LOG_DEBUG, TAG_STTD, "Engine name : %s", temp->engine_name);
	SLOG(LOG_DEBUG, TAG_STTD, "Setting ug path : %s", temp->setting_ug_path);
	SLOG(LOG_DEBUG, TAG_STTD, "Engine path : %s", temp->engine_path);
	SLOG(LOG_DEBUG, TAG_STTD, "Use network : %s", temp->use_network ? "true":"false");
	SLOG(LOG_DEBUG, TAG_STTD, "-----");
	SLOG(LOG_DEBUG, TAG_STTD, "  ");

	*info = temp;

	return 0;
}

int __internal_update_engine_list()
{
	/* relsease engine list */
	GList *iter = NULL;
	sttengine_info_s *data = NULL;

	if (0 < g_list_length(g_engine_list)) {
		/* Get a first item */
		iter = g_list_first(g_engine_list);

		while (NULL != iter) {
			/* Get handle data from list */
			data = iter->data;

			if (NULL != data) {
				if (NULL != data->engine_uuid)		free(data->engine_uuid);
				if (NULL != data->engine_path)		free(data->engine_path);
				if (NULL != data->engine_name)		free(data->engine_name);
				if (NULL != data->setting_ug_path)	free(data->setting_ug_path);
				
				free(data);
			}

			g_engine_list = g_list_remove_link(g_engine_list, iter);
			iter = g_list_first(g_engine_list);
		}
	}

	/* Get file name from default engine directory */
	DIR *dp;
	struct dirent *dirp;

	dp  = opendir(ENGINE_DIRECTORY_DEFAULT);
	if (NULL != dp) {
		while (NULL != (dirp = readdir(dp))) {
			sttengine_info_s* info;
			char* filepath;
			int filesize;

			filesize = strlen(ENGINE_DIRECTORY_DEFAULT) + strlen(dirp->d_name) + 5;
			filepath = (char*) g_malloc0(sizeof(char) * filesize);

			if (NULL != filepath) {
				strncpy(filepath, ENGINE_DIRECTORY_DEFAULT, strlen(ENGINE_DIRECTORY_DEFAULT) );
				strncat(filepath, "/", strlen("/") );
				strncat(filepath, dirp->d_name, strlen(dirp->d_name) );
			} else {
				SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Memory not enough!!" );
				continue;	
			}

			/* get its info and update engine list */
			if (0 == __internal_get_engine_info(filepath, &info)) {
				/* add engine info to g_engine_list */
				g_engine_list = g_list_append(g_engine_list, info);
			}

			if (NULL != filepath)
				g_free(filepath);
		}

		closedir(dp);
	} else {
		SLOG(LOG_WARN, TAG_STTD, "[Engine Agent WARNING] Fail to open default directory"); 
	}
	
	/* Get file name from downloadable engine directory */
	dp  = opendir(ENGINE_DIRECTORY_DOWNLOAD);
	if (NULL != dp) {
		while (NULL != (dirp = readdir(dp))) {
			sttengine_info_s* info;
			char* filepath;
			int filesize;

			filesize = strlen(ENGINE_DIRECTORY_DOWNLOAD) + strlen(dirp->d_name) + 5;
			filepath = (char*) g_malloc0(sizeof(char) * filesize);

			if (NULL != filepath) {
				strncpy(filepath, ENGINE_DIRECTORY_DOWNLOAD, strlen(ENGINE_DIRECTORY_DOWNLOAD) );
				strncat(filepath, "/", strlen("/") );
				strncat(filepath, dirp->d_name, strlen(dirp->d_name) );
			} else {
				SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Memory not enough!!" );
				continue;	
			}

			/* get its info and update engine list */
			if (0 == __internal_get_engine_info(filepath, &info)) {
				/* add engine info to g_engine_list */
				g_engine_list = g_list_append(g_engine_list, info);
			}

			if (NULL != filepath)
				g_free(filepath);
		}

		closedir(dp);
	} else {
		SLOG(LOG_WARN, TAG_STTD, "[Engine Agent WARNING] Fail to open downloadable directory"); 
	}

	if (0 >= g_list_length(g_engine_list)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] No Engine"); 
		return STTD_ERROR_ENGINE_NOT_FOUND;	
	}

	__log_enginelist();
	
	return 0;
}

int __internal_set_current_engine(const char* engine_uuid)
{
	if (NULL == engine_uuid) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Invalid Parameter"); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	/* check whether engine id is valid or not.*/
	GList *iter = NULL;
	sttengine_info_s *data = NULL;

	bool flag = false;
	if (g_list_length(g_engine_list) > 0) {
		/*Get a first item*/
		iter = g_list_first(g_engine_list);

		while (NULL != iter) {
			/*Get handle data from list*/
			data = iter->data;

			SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] engine_uuid(%s) engine list->uuid(%s)", engine_uuid, data->engine_uuid);

			if (0 == strncmp(data->engine_uuid, engine_uuid, strlen(engine_uuid))) {
				flag = true;
				break;
			}

			/*Get next item*/
			iter = g_list_next(iter);
		}
	}

	/* If current engine does not exist, return error */
	if (false == flag) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] __internal_set_current_engine : Cannot find engine id"); 
		return STTD_ERROR_INVALID_PARAMETER;
	} else {
		if (NULL != g_cur_engine.engine_uuid) {
			/*compare current engine uuid */
			if (0 == strncmp(g_cur_engine.engine_uuid, data->engine_uuid, strlen(engine_uuid))) {
				SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent Check] stt engine has already been set");
				return 0;
			}
		}
	}

	/* set data from g_engine_list */
	if (g_cur_engine.engine_uuid != NULL)	free(g_cur_engine.engine_uuid);
	if (g_cur_engine.engine_name != NULL)	free(g_cur_engine.engine_name);
	if (g_cur_engine.engine_path != NULL)	free(g_cur_engine.engine_path);

	g_cur_engine.engine_uuid = g_strdup(data->engine_uuid);
	g_cur_engine.engine_name = g_strdup(data->engine_name);
	g_cur_engine.engine_path = g_strdup(data->engine_path);

	g_cur_engine.handle = NULL;
	g_cur_engine.is_loaded = false;
	g_cur_engine.is_set = true;
	g_cur_engine.need_network = data->use_network;

	g_cur_engine.profanity_filter = g_default_profanity_filter;
	g_cur_engine.punctuation_override = g_default_punctuation_override;
	g_cur_engine.silence_detection = g_default_silence_detected;

	SLOG(LOG_DEBUG, TAG_STTD, "-----");
	SLOG(LOG_DEBUG, TAG_STTD, " Current engine uuid : %s", g_cur_engine.engine_uuid);
	SLOG(LOG_DEBUG, TAG_STTD, " Current engine name : %s", g_cur_engine.engine_name);
	SLOG(LOG_DEBUG, TAG_STTD, " Current engine path : %s", g_cur_engine.engine_path);
	SLOG(LOG_DEBUG, TAG_STTD, "-----");

	return 0;
}

int sttd_engine_agent_load_current_engine()
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.is_set) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] sttd_engine_agent_load_current_engine : No Current Engine "); 
		return -1;
	}

	/* check whether current engine is loaded or not */
	if (true == g_cur_engine.is_loaded) {
		SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] sttd_engine_agent_load_current_engine : Engine has already been loaded ");
		return 0;
	}
	
	SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] Current engine path : %s", g_cur_engine.engine_path);

	/* open engine */
	char *error;
	g_cur_engine.handle = dlopen(g_cur_engine.engine_path, RTLD_LAZY);

	if (NULL != (error = dlerror()) || !g_cur_engine.handle) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to get engine handle"); 
		return STTD_ERROR_OPERATION_FAILED;
	}
	
	g_cur_engine.sttp_unload_engine = (int (*)())dlsym(g_cur_engine.handle, "sttp_unload_engine");
	if (NULL != (error = dlerror()) || NULL == g_cur_engine.sttp_unload_engine) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to link daemon to sttp_unload_engine() : %s", error); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	g_cur_engine.sttp_load_engine = (int (*)(sttpd_funcs_s*, sttpe_funcs_s*) )dlsym(g_cur_engine.handle, "sttp_load_engine");
	if (NULL != (error = dlerror()) || NULL == g_cur_engine.sttp_load_engine) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to link daemon to sttp_load_engine() : %s", error); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	/* load engine */
	g_cur_engine.pdfuncs->version = 1;
	g_cur_engine.pdfuncs->size = sizeof(sttpd_funcs_s);

	if (0 != g_cur_engine.sttp_load_engine(g_cur_engine.pdfuncs, g_cur_engine.pefuncs)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail sttp_load_engine()"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] engine info : version(%d), size(%d)",g_cur_engine.pefuncs->version, g_cur_engine.pefuncs->size); 

	/* engine error check */
	if (g_cur_engine.pefuncs->size != sizeof(sttpe_funcs_s)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] sttd_engine_agent_load_current_engine : engine is not valid"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	/* initalize engine */
	if (0 != g_cur_engine.pefuncs->initialize(__result_cb, __partial_result_cb, __detect_silence_cb)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to initialize stt-engine"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	/* set default setting */
	int ret = 0;

	if (NULL == g_cur_engine.pefuncs->set_profanity_filter) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] set_profanity_filter of engine is NULL!!");
		return STTD_ERROR_OPERATION_FAILED;
	}
	
	/* check and set profanity filter */
	ret = g_cur_engine.pefuncs->set_profanity_filter(g_cur_engine.profanity_filter);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent] Not support profanity filter");
		g_cur_engine.support_profanity_filter = false;
	} else {
		g_cur_engine.support_profanity_filter = true;
	}
	
	/* check and set punctuation */
	if (NULL == g_cur_engine.pefuncs->set_punctuation) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] set_punctuation of engine is NULL!!");
		return STTD_ERROR_OPERATION_FAILED;
	}

	ret = g_cur_engine.pefuncs->set_punctuation(g_cur_engine.punctuation_override);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine ERROR] Not support punctuation override");
		g_cur_engine.support_punctuation_override = false;
	} else {
		g_cur_engine.support_punctuation_override = true;
	}

	/* check and set silence detection */
	if (NULL == g_cur_engine.pefuncs->set_silence_detection) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] set_silence_detection of engine is NULL!!");
		return STTD_ERROR_OPERATION_FAILED;
	}

	ret = g_cur_engine.pefuncs->set_silence_detection(g_cur_engine.silence_detection);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine ERROR] Not support silence detection");
		g_cur_engine.support_silence_detection = false;
	} else {
		g_cur_engine.support_silence_detection = true;
	}
	
	/* select default language */
	bool set_voice = false;
	if (NULL != g_cur_engine.default_lang) {
		if (NULL == g_cur_engine.pefuncs->is_valid_lang) {
			SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] set_default_lang of engine is NULL!!");
			return STTD_ERROR_OPERATION_FAILED;
		}

		if (true == g_cur_engine.pefuncs->is_valid_lang(g_cur_engine.default_lang)) {
			set_voice = true;
			SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent SUCCESS] Set origin default voice to current engine : lang(%s)", g_cur_engine.default_lang);
		} else {
			SLOG(LOG_WARN, TAG_STTD, "[Engine Agent WARNING] Fail set origin default language : lang(%s)", g_cur_engine.default_lang);
		}
	}

	if (false == set_voice) {
		if (NULL == g_cur_engine.pefuncs->foreach_langs) {
			SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] foreach_langs of engine is NULL!!");
			return STTD_ERROR_OPERATION_FAILED;
		}

		/* get language list */
		int ret;
		GList* lang_list = NULL;

		ret = g_cur_engine.pefuncs->foreach_langs(__supported_language_cb, &lang_list);

		if (0 == ret && 0 < g_list_length(lang_list)) {
			GList *iter = NULL;
			iter = g_list_first(lang_list);

			if (NULL != iter) {
				char* temp_lang = iter->data;

				if (true != g_cur_engine.pefuncs->is_valid_lang(temp_lang)) {
					SLOG(LOG_ERROR, TAG_STTD, "[Engine ERROR] Fail voice is NOT valid");
					return STTD_ERROR_OPERATION_FAILED;
				}

				sttd_config_set_default_language(temp_lang);

				g_cur_engine.default_lang = g_strdup(temp_lang);
				
				SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent SUCCESS] Select default voice : lang(%s)", temp_lang);
			} else {
				SLOG(LOG_ERROR, TAG_STTD, "[Engine ERROR] Fail to get language list : result(%d)\n", ret);
				return STTD_ERROR_OPERATION_FAILED;
			}

			__free_language_list(lang_list);
		} else {
			SLOG(LOG_ERROR, TAG_STTD, "[Engine ERROR] Fail to get language list : result(%d)\n", ret);
			return STTD_ERROR_OPERATION_FAILED;
		}

	} 

	g_cur_engine.is_loaded = true;

	SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent SUCCESS] The %s has been loaded !!!", g_cur_engine.engine_name); 

	return 0;
}

int sttd_engine_agent_unload_current_engine()
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized "); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.is_set) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] sttd_engine_agent_unload_current_engine : No Current Engine "); 
		return -1;
	}

	if (false == g_cur_engine.is_loaded) {
		SLOG(LOG_WARN, TAG_STTD, "[Engine Agent WARNING] Current engine has already been unloaded "); 
		return 0;
	}

	/* shutdown engine */
	if (NULL == g_cur_engine.pefuncs->deinitialize) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] shutdown of engine is NULL!!");
	} else {
		g_cur_engine.pefuncs->deinitialize();
	}

	/* unload engine */
	if (0 != g_cur_engine.sttp_unload_engine()) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to unload engine"); 
	}

	dlclose(g_cur_engine.handle);

	/* reset current engine data */
	g_cur_engine.handle = NULL;
	g_cur_engine.is_loaded = false;

	return 0;
}

bool sttd_engine_agent_need_network()
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized" );
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not loaded engine");
		return STTD_ERROR_OPERATION_FAILED;
	}

	return g_cur_engine.need_network;
}

int sttd_engine_get_option_supported(bool* silence, bool* profanity, bool* punctuation)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized" );
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not loaded engine");
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (NULL == silence || NULL == profanity || NULL == punctuation) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Invalid Parameter"); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	*silence = g_cur_engine.support_silence_detection;
	*profanity = g_cur_engine.support_profanity_filter;
	*punctuation = g_cur_engine.support_punctuation_override;

	return 0;
}

/*
* STT Engine Interfaces for client
*/

int __set_option(int profanity, int punctuation, int silence)
{
	if (2 == profanity) {
		/* Default selection */
		if (g_default_profanity_filter != g_cur_engine.profanity_filter) {
			if (NULL != g_cur_engine.pefuncs->set_profanity_filter) {
				if (0 != g_cur_engine.pefuncs->set_profanity_filter(g_default_profanity_filter)) {
					SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to set profanity filter");
					return STTD_ERROR_OPERATION_FAILED;
				}
				g_cur_engine.profanity_filter = g_default_profanity_filter;
				SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] Set profanity filter : %s", g_cur_engine.profanity_filter ? "true" : "false");
			} else {
				SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] profanity_filter() of engine is NULL!!");
				return STTD_ERROR_OPERATION_FAILED;
			}
		}
	} else {
		/* Client selection */
		if (g_cur_engine.profanity_filter != profanity) {
			if (NULL != g_cur_engine.pefuncs->set_profanity_filter) {
				if (0 != g_cur_engine.pefuncs->set_profanity_filter((bool)profanity)) {
					SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to set profanity filter");
					return STTD_ERROR_OPERATION_FAILED;
				}

				g_cur_engine.profanity_filter = (bool)profanity;
				SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] Set profanity filter : %s", g_cur_engine.profanity_filter ? "true" : "false");
			} else {
				SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] profanity_filter() of engine is NULL!!");
				return STTD_ERROR_OPERATION_FAILED;
			}
		}
	}

	if (2 == punctuation) {
		/* Default selection */
		if (g_default_punctuation_override != g_cur_engine.punctuation_override) {
			if (NULL != g_cur_engine.pefuncs->set_punctuation) {
				if (0 != g_cur_engine.pefuncs->set_punctuation(g_default_punctuation_override)) {
					SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to set punctuation override");
					return STTD_ERROR_OPERATION_FAILED;
				}
				g_cur_engine.punctuation_override = g_default_punctuation_override;
				SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] Set punctuation override : %s", g_cur_engine.punctuation_override ? "true" : "false");
			} else {
				SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] set_punctuation() of engine is NULL!!");
				return STTD_ERROR_OPERATION_FAILED;
			}
		}
	} else {
		/* Client selection */
		if (g_cur_engine.punctuation_override != punctuation) {
			if (NULL != g_cur_engine.pefuncs->set_punctuation) {
				if (0 != g_cur_engine.pefuncs->set_punctuation((bool)punctuation)) {
					SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to set punctuation override");
					return STTD_ERROR_OPERATION_FAILED;
				}

				g_cur_engine.punctuation_override = (bool)punctuation;
				SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] Set punctuation override : %s", g_cur_engine.punctuation_override ? "true" : "false");
			} else {
				SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] set_punctuation() of engine is NULL!!");
				return STTD_ERROR_OPERATION_FAILED;
			}
		}
	}

	if (2 == silence) {
		/* Default selection */
		if (g_default_silence_detected != g_cur_engine.silence_detection) {
			if (NULL != g_cur_engine.pefuncs->set_silence_detection) {
				if (0 != g_cur_engine.pefuncs->set_silence_detection(g_default_silence_detected)) {
					SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to set silence detection");
					return STTD_ERROR_OPERATION_FAILED;
				}
				g_cur_engine.silence_detection = g_default_silence_detected;
				SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] Set silence detection : %s", g_cur_engine.silence_detection ? "true" : "false");
			} else {
				SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] set_silence() of engine is NULL!!");
				return STTD_ERROR_OPERATION_FAILED;
			}
		}
	} else {
		/* Client selection */
		if (g_cur_engine.silence_detection != silence) {
			if (NULL != g_cur_engine.pefuncs->set_silence_detection) {
				if (0 != g_cur_engine.pefuncs->set_silence_detection((bool)silence)) {
					SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to set silence detection");
					return STTD_ERROR_OPERATION_FAILED;
				}

				g_cur_engine.silence_detection = (bool)silence;
				SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] Set silence detection : %s", g_cur_engine.silence_detection ? "true" : "false");
			} else {
				SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] set_silence() of engine is NULL!!");
				return STTD_ERROR_OPERATION_FAILED;
			}
		}
	}
	
	return 0;
}

int sttd_engine_recognize_start(const char* lang, const char* recognition_type, 
				int profanity, int punctuation, int silence, void* user_param)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not loaded engine"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (NULL == lang || NULL == recognition_type) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Invalid Parameter"); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (0 != __set_option(profanity, punctuation, g_default_silence_detected)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to set options"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_cur_engine.pefuncs->start) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] start() of engine is NULL!!");
		return STTD_ERROR_OPERATION_FAILED;
	}

	char* temp;
	if (0 == strncmp(lang, "default", strlen("default"))) {
		temp = strdup(g_cur_engine.default_lang);
	} else {
		temp = strdup(lang);
	}

	int ret = g_cur_engine.pefuncs->start(temp, recognition_type, user_param);
	free(temp);

	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] sttd_engine_recognize_start : recognition start error(%d)", ret); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent SUCCESS] sttd_engine_recognize_start");

	return 0;
}

int sttd_engine_recognize_audio(const void* data, unsigned int length)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not loaded engine"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (NULL == data) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Invalid Parameter"); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (NULL == g_cur_engine.pefuncs->set_recording) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] The function of engine is NULL!!");
		return STTD_ERROR_OPERATION_FAILED;
	}

	int ret = g_cur_engine.pefuncs->set_recording(data, length);
	if (0 != ret) {
		SLOG(LOG_WARN, TAG_STTD, "[Engine Agent WARNING] set recording error(%d)", ret); 
		return ret;
	}

	return 0;
}

int sttd_engine_recognize_stop()
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not loaded engine"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_cur_engine.pefuncs->stop) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] The function of engine is NULL!!");
		return STTD_ERROR_OPERATION_FAILED;
	}

	int ret = g_cur_engine.pefuncs->stop();
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] stop recognition error(%d)", ret); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	return 0;
}

int sttd_engine_recognize_cancel()
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not loaded engine"); 
		return STTD_ERROR_OPERATION_FAILED;
	}
	
	if (NULL == g_cur_engine.pefuncs->cancel) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] The function of engine is NULL!!");
		return STTD_ERROR_OPERATION_FAILED;
	}

	int ret = g_cur_engine.pefuncs->cancel();
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] cancel recognition error(%d)", ret); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	return 0;
}

int sttd_engine_get_audio_format(sttp_audio_type_e* types, int* rate, int* channels)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not loaded engine"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_cur_engine.pefuncs->get_audio_format) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] The function of engine is NULL!!");
		return STTD_ERROR_OPERATION_FAILED;
	}

	int ret = g_cur_engine.pefuncs->get_audio_format(types, rate, channels);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] get audio format error(%d)", ret); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	return 0;
}

int sttd_engine_recognize_start_file(const char* filepath, const char* lang, const char* recognition_type, 
				     int profanity, int punctuation, void* user_param)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not loaded engine"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (NULL == filepath || NULL == lang || NULL == recognition_type) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Invalid Parameter"); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (0 != __set_option(profanity, punctuation, g_default_silence_detected)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to set options"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_cur_engine.pefuncs->start_file_recognition) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] start() of engine is NULL!!");
		return STTD_ERROR_OPERATION_FAILED;
	}

	char* temp;
	if (0 == strncmp(lang, "default", strlen("default"))) {
		temp = strdup(g_cur_engine.default_lang);
	} else {
		temp = strdup(lang);
	}

	int ret = g_cur_engine.pefuncs->start_file_recognition(filepath, temp, recognition_type, user_param);
	free(temp);

	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to start file recognition(%d)", ret); 
		return ret;
	}

	SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent SUCCESS] File recognition");

	return 0;
}

/*
* STT Engine Interfaces for client and setting
*/
bool __supported_language_cb(const char* language, void* user_data)
{
	GList** lang_list = (GList**)user_data;

	if (NULL == language || NULL == lang_list) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Input parameter is NULL in callback!!!!");
		return false;
	}

	SLOG(LOG_DEBUG, TAG_STTD, "-- Language(%s)", language);

	char* temp_lang = g_strdup(language);

	*lang_list = g_list_append(*lang_list, temp_lang);

	return true;
}

int sttd_engine_supported_langs(GList** lang_list)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not loaded engine"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_cur_engine.pefuncs->foreach_langs) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] The function of engine is NULL!!");
		return STTD_ERROR_OPERATION_FAILED;
	}

	int ret = g_cur_engine.pefuncs->foreach_langs(__supported_language_cb, (void*)lang_list);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] get language list error(%d)", ret); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	return 0;
}


int sttd_engine_get_default_lang(char** lang)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not loaded engine"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (NULL == lang) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Invalid Parameter"); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	/* get default language */
	*lang = g_strdup(g_cur_engine.default_lang);

	return 0;
}

int sttd_engine_is_partial_result_supported(bool* partial_result)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not loaded engine"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (NULL == partial_result) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Invalid Parameter"); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (NULL == g_cur_engine.pefuncs->support_partial_result) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] The function of engine is NULL!!");
		return STTD_ERROR_OPERATION_FAILED;
	}

	*partial_result = g_cur_engine.pefuncs->support_partial_result();

	return 0;
}


/*
* STT Engine Interfaces for setting
*/

int sttd_engine_setting_get_engine_list(GList** engine_list)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not loaded engine"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	/* update engine list */
	if (0 != __internal_update_engine_list()) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] sttd_engine_setting_get_engine_list : __internal_update_engine_list()"); 
		return -1;
	}

	GList *iter = NULL;
	sttengine_info_s *data = NULL;

	iter = g_list_first(g_engine_list);

	SLOG(LOG_DEBUG, TAG_STTD, "----- [Engine Agent] engine list -----");

	while (NULL != iter) {
		engine_s* temp_engine;

		temp_engine = (engine_s*)g_malloc0(sizeof(engine_s));

		data = iter->data;

		temp_engine->engine_id = strdup(data->engine_uuid);
		temp_engine->engine_name = strdup(data->engine_name);
		temp_engine->ug_name = strdup(data->setting_ug_path);

		*engine_list = g_list_append(*engine_list, temp_engine);

		iter = g_list_next(iter);

		SLOG(LOG_DEBUG, TAG_STTD, " -- engine id(%s) engine name(%s) ug name(%s) \n", 
			temp_engine->engine_id, temp_engine->engine_name, temp_engine->ug_name);
	}

	SLOG(LOG_DEBUG, TAG_STTD, "--------------------------------------");

	return 0;
}

int sttd_engine_setting_get_engine(char** engine_id)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized" );
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not loaded engine");
		return STTD_ERROR_ENGINE_NOT_FOUND;
	}

	*engine_id = strdup(g_cur_engine.engine_uuid);

	return 0;
}

int sttd_engine_setting_set_engine(const char* engine_id)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (NULL == engine_id) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Invalid Parameter"); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	/* compare current engine and new engine. */
	if (NULL != g_cur_engine.engine_uuid) {
		if (0 == strncmp(g_cur_engine.engine_uuid, engine_id, strlen(g_cur_engine.engine_uuid))) {
			SLOG(LOG_WARN, TAG_STTD, "[Engine Agent] New engine is the same as current engine"); 
			return 0;
		}
	}

	char* tmp_uuid = NULL;
	tmp_uuid = g_strdup(g_cur_engine.engine_uuid);
	if (NULL == tmp_uuid) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not enough memory!!"); 
		return STTD_ERROR_OUT_OF_MEMORY;
	}

	/* unload engine */
	if (0 != sttd_engine_agent_unload_current_engine()) 
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to unload current engine"); 
	else
		SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent SUCCESS] unload current engine");

	/* change current engine */
	if (0 != __internal_set_current_engine(engine_id)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] __internal_set_current_engine : no engine error"); 
		
		/* roll back to old current engine. */
		__internal_set_current_engine(tmp_uuid);
		sttd_engine_agent_load_current_engine();

		if (NULL != tmp_uuid)	
			free(tmp_uuid);

		return STTD_ERROR_OPERATION_FAILED;
	}

	if (0 != sttd_engine_agent_load_current_engine()) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to load new engine"); 

		if (NULL != tmp_uuid)	
			free(tmp_uuid);

		return STTD_ERROR_OPERATION_FAILED;
	}

	SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent SUCCESS] sttd_engine_setting_set_engine() : Load new engine");

	if( tmp_uuid != NULL )	
		free(tmp_uuid);

	/* set engine id to config */
	if (0 != sttd_config_set_default_engine(engine_id)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to set engine id"); 
	}

	return 0;
}

int sttd_engine_setting_get_lang_list(char** engine_id, GList** lang_list)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not loaded engine"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (NULL == lang_list || NULL == engine_id) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Invalid Parameter"); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	/* get language list from engine */
	int ret = sttd_engine_supported_langs(lang_list); 
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail get lang list (%d)", ret); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	*engine_id = strdup(g_cur_engine.engine_uuid);

	return 0;
}

int sttd_engine_setting_get_default_lang(char** language)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not loaded engine"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (NULL == language) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Invalid Parameter"); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (NULL != g_cur_engine.default_lang) {
		*language = strdup(g_cur_engine.default_lang);

		SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent SUCCESS] Get default lanaguae : language(%s)", *language);
	} else {
		if (NULL == g_cur_engine.pefuncs->foreach_langs) {
			SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] foreach_langs of engine is NULL!!");
			return STTD_ERROR_OPERATION_FAILED;
		}

		/* get language list */
		int ret;
		GList* lang_list = NULL;

		ret = g_cur_engine.pefuncs->foreach_langs(__supported_language_cb, &lang_list);

		if (0 == ret && 0 < g_list_length(lang_list)) {
			GList *iter = NULL;
			iter = g_list_first(lang_list);

			if (NULL != iter) {
				char* temp_lang = iter->data;

				if (true != g_cur_engine.pefuncs->is_valid_lang(temp_lang)) {
					SLOG(LOG_ERROR, TAG_STTD, "[Engine ERROR] Fail voice is NOT valid");
					return STTD_ERROR_OPERATION_FAILED;
				}

				sttd_config_set_default_language(temp_lang);

				g_cur_engine.default_lang = g_strdup(temp_lang);

				*language = strdup(g_cur_engine.default_lang);

				SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent SUCCESS] Select default voice : lang(%s)", temp_lang);
			} else {
				SLOG(LOG_ERROR, TAG_STTD, "[Engine ERROR] Fail to get language list : result(%d)\n", ret);
				return STTD_ERROR_OPERATION_FAILED;
			}

			__free_language_list(lang_list);
		} else {
			SLOG(LOG_ERROR, TAG_STTD, "[Engine ERROR] Fail to get language list : result(%d)\n", ret);
			return STTD_ERROR_OPERATION_FAILED;
		}
	}
	
	return 0;
}

int sttd_engine_setting_set_default_lang(const char* language)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not loaded engine"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_cur_engine.pefuncs->is_valid_lang) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] get_voice_list() of engine is NULL!!");
		return STTD_ERROR_OPERATION_FAILED;
	}

	int ret = -1;
	if(false == g_cur_engine.pefuncs->is_valid_lang(language)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Language is NOT valid !!");
		return STTD_ERROR_INVALID_LANGUAGE;
	}

	if (NULL != g_cur_engine.default_lang)
		g_free(g_cur_engine.default_lang);

	g_cur_engine.default_lang = strdup(language);

	ret = sttd_config_set_default_language(language);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to set default lang (%d)", ret); 
	}

	return 0;
}

int sttd_engine_setting_get_profanity_filter(bool* value)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not loaded engine"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (NULL == value) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Invalid Parameter"); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	*value = g_default_profanity_filter;	

	return 0;
}

int sttd_engine_setting_set_profanity_filter(bool value)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not loaded engine"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_cur_engine.pefuncs->set_profanity_filter) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] The function of engine is NULL!!");
		return STTD_ERROR_OPERATION_FAILED;
	}

	int ret = g_cur_engine.pefuncs->set_profanity_filter(value);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail set profanity filter : result(%d)", ret); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	g_default_profanity_filter = value;

	ret = sttd_config_set_default_profanity_filter((int)value);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to set default lang (%d)", ret); 
	}

	return 0;
}

int sttd_engine_setting_get_punctuation_override(bool* value)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not loaded engine"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (NULL == value) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Invalid Parameter"); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	*value = g_default_punctuation_override;

	return 0;
}

int sttd_engine_setting_set_punctuation_override(bool value)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not loaded engine"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_cur_engine.pefuncs->set_punctuation) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] The function of engine is NULL!!");
		return STTD_ERROR_OPERATION_FAILED;
	}

	int ret = g_cur_engine.pefuncs->set_punctuation(value);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail set punctuation override : result(%d)", ret); 
		return STTD_ERROR_OPERATION_FAILED;
	}
	g_default_punctuation_override = value;

	ret = sttd_config_set_default_punctuation_override((int)value);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to set punctuation override (%d)", ret); 
	}

	return 0;
}

int sttd_engine_setting_get_silence_detection(bool* value)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not loaded engine"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (NULL == value) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Invalid Parameter"); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	*value = g_default_silence_detected;

	return 0;
}

int sttd_engine_setting_set_silence_detection(bool value)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not loaded engine"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	int ret = g_cur_engine.pefuncs->set_silence_detection(value);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail set silence detection : result(%d)", ret); 
		return STTD_ERROR_OPERATION_FAILED;
	}
	
	g_default_silence_detected = value;

	ret = sttd_config_set_default_silence_detection((int)value);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to set silence detection (%d)", ret); 
	}
	
	return 0;
}

bool __engine_setting_cb(const char* key, const char* value, void* user_data)
{
	GList** engine_setting_list = (GList**)user_data;

	if (NULL == engine_setting_list || NULL == key || NULL == value) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Input parameter is NULL in engine setting callback!!!!");
		return false;
	}

	engine_setting_s* temp = g_malloc0(sizeof(engine_setting_s));
	temp->key = g_strdup(key);
	temp->value = g_strdup(value);

	*engine_setting_list = g_list_append(*engine_setting_list, temp);

	return true;
}

int sttd_engine_setting_get_engine_setting_info(char** engine_id, GList** setting_list)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not loaded engine"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (NULL == setting_list) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Input parameter is NULL");
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (NULL == g_cur_engine.pefuncs->foreach_engine_settings) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] foreach_engine_settings() of engine is NULL!!");
		return STTD_ERROR_OPERATION_FAILED;
	}

	/* get setting info and move setting info to input parameter */
	int result = 0;

	result = g_cur_engine.pefuncs->foreach_engine_settings(__engine_setting_cb, setting_list);

	if (0 == result && 0 < g_list_length(*setting_list)) {
		*engine_id = strdup(g_cur_engine.engine_uuid);
	} else {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] fail to get setting info : result(%d)\n", result);
		result = STTD_ERROR_OPERATION_FAILED;
	}

	return result;
}

int sttd_engine_setting_set_engine_setting(const char* key, const char* value)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not loaded engine"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (NULL == key || NULL == value) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Invalid Parameter"); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (NULL == g_cur_engine.pefuncs->set_engine_setting) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] set_engine_setting() of engine is NULL!!");
		return STTD_ERROR_OPERATION_FAILED;
	}

	/* get setting info and move setting info to input parameter */
	int ret = g_cur_engine.pefuncs->set_engine_setting(key, value);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail set setting info (%d) ", ret); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	return 0;
}

/*
* STT Engine Callback Functions											`				  *
*/

void __result_cb(sttp_result_event_e event, const char* type, 
					const char** data, int data_count, const char* msg, void *user_data)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Result Callback : Not Initialized"); 
		return;
	}

	if (false == g_cur_engine.is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Result Callback : Not loaded engine"); 
		return;
	}

	return g_result_cb(event, type, data, data_count, msg, user_data);
}

void __partial_result_cb(sttp_result_event_e event, const char* data, void *user_data)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Partial Result Callback : Not Initialized"); 
		return;
	}

	if (false == g_cur_engine.is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Partial Result Callback : Not loaded engine"); 
		return;
	}

	return g_partial_result_cb(event, data, user_data);
}

void __detect_silence_cb(void* user_data)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Silence Callback : Not Initialized"); 
		return;
	}

	if (false == g_cur_engine.is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Silence Callback : Not loaded engine"); 
		return;
	}

	if (true == g_cur_engine.silence_detection) {
		g_silence_cb(user_data);
	} else {
		SLOG(LOG_WARN, TAG_STTD, "[Engine Agent] Silence detection callback is blocked because option value is false.");
	}
}

void __free_language_list(GList* lang_list)
{
	GList *iter = NULL;
	char* data = NULL;

	/* if list have item */
	if (g_list_length(lang_list) > 0) {
		/* Get a first item */
		iter = g_list_first(lang_list);

		while (NULL != iter) {
			data = iter->data;

			if (NULL != data)
				g_free(data);
			
			lang_list = g_list_remove_link(lang_list, iter);

			iter = g_list_first(lang_list);
		}
	}
}

/* A function forging */
int __log_enginelist()
{
	GList *iter = NULL;
	sttengine_info_s *data = NULL;

	if (0 < g_list_length(g_engine_list)) {

		/* Get a first item */
		iter = g_list_first(g_engine_list);

		SLOG(LOG_DEBUG, TAG_STTD, "--------------- engine list -------------------");

		int i = 1;	
		while (NULL != iter) {
			/* Get handle data from list */
			data = iter->data;

			SLOG(LOG_DEBUG, TAG_STTD, "[%dth]", i);
			SLOG(LOG_DEBUG, TAG_STTD, "  engine uuid : %s", data->engine_uuid);
			SLOG(LOG_DEBUG, TAG_STTD, "  engine name : %s", data->engine_name);
			SLOG(LOG_DEBUG, TAG_STTD, "  engine path : %s", data->engine_path);
			SLOG(LOG_DEBUG, TAG_STTD, "  setting ug path : %s", data->setting_ug_path);

			iter = g_list_next(iter);
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



