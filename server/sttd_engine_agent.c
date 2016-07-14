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


#include <dlfcn.h>
#include <dirent.h>

#include "stt_defs.h"
#include "stt_engine.h"
#include "sttd_main.h"
#include "sttd_client_data.h"
#include "sttd_config.h"
#include "sttd_dbus.h"
#include "sttd_recorder.h"
#include "sttd_engine_agent.h"


#define AUDIO_CREATE_ON_START

/*
* Internal data structure
*/

typedef struct _sttengine_info {
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
	bool	need_credential;
} sttengine_info_s;

/** stt engine agent init */
static bool	g_agent_init;

static sttengine_info_s* g_engine_info = NULL;

/** default engine info */
static char*	g_default_language = NULL;
static bool	g_default_silence_detected;

/** callback functions */
static result_callback g_result_cb = NULL;
static result_time_callback g_result_time_cb = NULL;
static speech_status_callback g_speech_status_cb = NULL;
static error_callback g_error_cb = NULL;

/** callback functions */
bool __result_time_cb(int index, stte_result_time_event_e event, const char* text,
		      long start_time, long end_time, void* user_data);

bool __supported_language_cb(const char* language, void* user_data);

void __engine_info_cb(const char* engine_uuid, const char* engine_name, const char* setting_ug_name,
		      bool use_network, void* user_data);

/*
* Internal Interfaces
*/

/** get engine info */
int __internal_get_engine_info(stte_request_callback_s *callback, sttengine_info_s** info);

int __log_enginelist();

/*
* STT Engine Agent Interfaces
*/
int sttd_engine_agent_init(result_callback result_cb, result_time_callback time_cb,
			   speech_status_callback speech_status_cb, error_callback error_cb)
{
	/* initialize static data */
	if (NULL == result_cb || NULL == time_cb || NULL == speech_status_cb || NULL == error_cb) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine agent ERROR] Invalid parameter");
		return STTD_ERROR_INVALID_PARAMETER;
	}

	g_result_cb = result_cb;
	g_result_time_cb = time_cb;
	g_speech_status_cb = speech_status_cb;
	g_error_cb = error_cb;

	g_default_language = NULL;

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

	g_agent_init = true;

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
		engine = NULL;
	}

	return 0;
}

int sttd_engine_agent_release()
{
	if (NULL == g_engine_info) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent] No loaded engine");
		return STTD_ERROR_ENGINE_NOT_FOUND;
	}

	if (g_engine_info->is_loaded) {
		SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] Unload engine");

		if (0 != stt_engine_deinitialize()) {
			SLOG(LOG_WARN, TAG_STTD, "[Engine Agent] Fail to deinitialize");
		}

		if (0 != stt_engine_unload()) {
			SLOG(LOG_WARN, TAG_STTD, "[Engine Agent] Fail to unload engine");
		}

		g_engine_info->is_loaded = false;
			}

	__engine_agent_clear_engine(g_engine_info);

	g_result_cb = NULL;
	g_speech_status_cb = NULL;
	g_error_cb = NULL;
	g_result_time_cb = NULL;

	g_agent_init = false;

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

int __internal_get_engine_info(stte_request_callback_s *callback, sttengine_info_s** info)
{
	sttengine_info_s* temp;
	temp = (sttengine_info_s*)calloc(1, sizeof(sttengine_info_s));
	if (NULL == temp) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] fail to allocate memory");
		return STTD_ERROR_OUT_OF_MEMORY;
	}

	if (NULL == callback) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Invalid engine");
		return STTD_ERROR_ENGINE_NOT_FOUND;
	}

	if (NULL == callback->get_info) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Invalid engine");
		return STTD_ERROR_ENGINE_NOT_FOUND;
	}

	if (0 != callback->get_info(&(temp->engine_uuid), &(temp->engine_name), &(temp->engine_setting_path), &(temp->use_network))) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to get engine info");
		return STTD_ERROR_ENGINE_NOT_FOUND;
	}

	/* todo - removed? */
	temp->engine_path = strdup("empty");
	temp->is_loaded = false;

	SLOG(LOG_DEBUG, TAG_STTD, "----- Valid Engine");
	SLOG(LOG_DEBUG, TAG_STTD, "Engine uuid : %s", temp->engine_uuid);
	SLOG(LOG_DEBUG, TAG_STTD, "Engine name : %s", temp->engine_name);
	SLOG(LOG_DEBUG, TAG_STTD, "Engine path : %s", temp->engine_path);
	SLOG(LOG_DEBUG, TAG_STTD, "Engine setting path : %s", temp->engine_setting_path);
	SLOG(LOG_DEBUG, TAG_STTD, "Use network : %s", temp->use_network ? "true":"false");
	SLOG(LOG_DEBUG, TAG_STTD, "-----");
	SLOG(LOG_DEBUG, TAG_STTD, "  ");

	*info = temp;

	return STTD_ERROR_NONE;
}

bool __is_engine(const char* filepath)
{
	if (NULL == filepath) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] No filepath");
		return false;
	}

	if (NULL != g_engine_info) {
		if (!strcmp(g_engine_info->engine_path, filepath)) {
			return true;
		}
	}

	return false;
}

int __engine_agent_check_engine_unload()
{
	/* Check the count of client to use this engine */
	if (NULL == g_engine_info) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] No engine");
	} else {
		if (g_engine_info->is_loaded) {
			/* unload engine */
#ifndef AUDIO_CREATE_ON_START
			SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] Destroy recorder");
			if (0 != sttd_recorder_destroy())
				SECURE_SLOG(LOG_WARN, TAG_STTD, "[Engine Agent] Fail to destroy recorder");
#endif
			SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] Unload engine");
			if (0 != stt_engine_deinitialize())
				SECURE_SLOG(LOG_WARN, TAG_STTD, "[Engine Agent] Fail to deinitialize engine");

			if (0 != stt_engine_unload())
				SECURE_SLOG(LOG_WARN, TAG_STTD, "[Engine Agent] Fail to unload engine");

			g_engine_info->is_loaded = false;
		}
	}

	return 0;
}

int sttd_engine_agent_load_current_engine(stte_request_callback_s *callback)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized" );
		return STTD_ERROR_OPERATION_FAILED;
	}

	/* Get current engine info */
	sttengine_info_s* info;
	int ret = __internal_get_engine_info(callback, &info);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to get engine info");
		return ret;
	} else {
		g_engine_info = info;
	}

	__log_enginelist();

	/* Set default engine */
	char* cur_engine_uuid = NULL;
	bool is_default_engine = false;

	/* get current engine from config */
	if (0 == sttd_config_get_default_engine(&cur_engine_uuid)) {
		SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] current engine from config : %s", cur_engine_uuid);
		if (NULL != g_engine_info && NULL != g_engine_info->engine_uuid) {
			if (!strcmp(g_engine_info->engine_uuid, cur_engine_uuid)) {
						is_default_engine = true;
			}
		}

		if (NULL != cur_engine_uuid)
			free(cur_engine_uuid);
	} else {
		SLOG(LOG_WARN, TAG_STTD, "[Engine Agent WARNING] There is not current engine from config");
	}

	if (false == is_default_engine) {
		SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] Current engine is not Default engine");
	} else {
		SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] Current engine is Default engine");
	}

	/* Load engine */
	ret = stt_engine_load(g_engine_info->engine_path, callback);
	if (0 != ret) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to load engine : path(%s)", g_engine_info->engine_path);
		return STTD_ERROR_OPERATION_FAILED;
	}

	ret = stt_engine_initialize(false);
	if (0 != ret) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to initialize engine : path(%s)", g_engine_info->engine_path);
		return STTD_ERROR_OPERATION_FAILED;
	}

	ret = stt_engine_set_silence_detection(g_default_silence_detected);
	if (0 != ret) {
		SLOG(LOG_WARN, TAG_STTD, "[Engine Agent WARNING] Not support silence detection");
		g_engine_info->support_silence_detection = false;
	} else {
		SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] Silence detection : %s", g_default_silence_detected ? "true" : "false");
		g_engine_info->support_silence_detection = true;
		g_engine_info->silence_detection = g_default_silence_detected;
	}

	/* Set first language */
	char* tmp_lang = NULL;
	ret = stt_engine_get_first_language(&tmp_lang);
	if (0 == ret && NULL != tmp_lang) {
		g_engine_info->first_lang = strdup(tmp_lang);
		free(tmp_lang);
	} else {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to get first language from engine : %s", g_engine_info->engine_name);
		return STTD_ERROR_OPERATION_FAILED;
	}

#ifndef AUDIO_CREATE_ON_START
	/* Ready recorder */
	stte_audio_type_e atype;
	int rate;
	int channels;

	ret = stt_engine_get_audio_type(&atype, &rate, &channels);
	if (0 != ret) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to get audio type : %d %s", g_engine_info->engine_name);
		return STTD_ERROR_OPERATION_FAILED;
	}

	ret = sttd_recorder_create(atype, channels, rate);
	if (0 != ret) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to create recorder : %s", g_engine_info->engine_name);
		return STTD_ERROR_OPERATION_FAILED;
	}
#endif

	g_engine_info->is_loaded = true;
	SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent SUCCESS] The %s has been loaded !!!", g_engine_info->engine_name);

	return 0;
}

int sttd_engine_agent_unload_current_engine()
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized ");
		return STTD_ERROR_OPERATION_FAILED;
	}

	/* Remove client */
	__engine_agent_check_engine_unload();

	return 0;
}

bool sttd_engine_agent_is_default_engine()
{
		return true;
}

int sttd_engine_agent_get_engine_list(GSList** engine_list)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized");
		return STTD_ERROR_OPERATION_FAILED;
	}

	SLOG(LOG_DEBUG, TAG_STTD, "--------------------------------------");

	return 0;
}

int sttd_engine_agent_get_current_engine(char** engine_uuid)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized");
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (NULL == engine_uuid) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Invalid parameter");
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (NULL == g_engine_info) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] The engine is not valid");
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (false == g_engine_info->is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not loaded engine");
		return STTD_ERROR_OPERATION_FAILED;
	}

	*engine_uuid = strdup(g_engine_info->engine_uuid);

	return 0;
}

bool sttd_engine_agent_need_network()
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized");
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (NULL != g_engine_info)
		return g_engine_info->use_network;

	return false;
}

int sttd_engine_agent_supported_langs(GSList** lang_list)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized");
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (NULL == lang_list) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Input parameter is NULL");
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (NULL == g_engine_info) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] The engine is not valid");
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (false == g_engine_info->is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not loaded engine");
		return STTD_ERROR_OPERATION_FAILED;
	}

	int ret = stt_engine_get_supported_langs(lang_list);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] get language list error(%d)", ret);
	}

	return ret;
}

int sttd_engine_agent_get_default_lang(char** lang)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized");
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (NULL == lang) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Invalid Parameter");
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (NULL == g_engine_info) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] The engine is not valid");
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (false == g_engine_info->is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not loaded engine");
		return STTD_ERROR_OPERATION_FAILED;
	}

	/* get default language */
	bool is_valid = false;
	if (0 != stt_engine_is_valid_language(g_default_language, &is_valid)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to check valid language");
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (true == is_valid) {
		*lang = strdup(g_default_language);
	} else
		*lang = strdup(g_engine_info->first_lang);

	return 0;
}

int sttd_engine_agent_set_private_data(const char* key, const char* data)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized");
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (NULL == key || NULL == data) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Invalid Parameter");
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (NULL == g_engine_info) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] The engine is not valid");
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (false == g_engine_info->is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not loaded engine");
		return STTD_ERROR_OPERATION_FAILED;
	}

	/* set private data */
	int ret = -1;
	ret = stt_engine_set_private_data(key, data);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to set private data");
	}

	return ret;
}

int sttd_engine_agent_get_private_data(const char* key, char** data)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized");
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (NULL == key || NULL == data) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Invalid Parameter");
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (NULL == g_engine_info) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] The engine is not valid");
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (false == g_engine_info->is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not loaded engine");
		return STTD_ERROR_OPERATION_FAILED;
	}

	/* get default language */
	int ret = -1;
	ret = stt_engine_get_private_data(key, data);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to get private data");
	}

	return ret;
}

int sttd_engine_agent_get_option_supported(bool* silence)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized");
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (NULL == silence) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Invalid Parameter");
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (NULL == g_engine_info) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Engine is not valid");
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (false == g_engine_info->is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not loaded engine");
		return STTD_ERROR_OPERATION_FAILED;
	}

	*silence = g_engine_info->support_silence_detection;

	return 0;
}

int sttd_engine_agent_is_credential_needed(int uid, bool* credential)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized");
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (NULL == credential) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Invalid Parameter");
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (NULL == g_engine_info) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] uid(%d) is not valid", uid);
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (false == g_engine_info->is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not loaded engine");
		return STTD_ERROR_OPERATION_FAILED;
	}

	bool temp = false;
	int ret;

	ret = stt_engine_need_app_credential(&temp);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to get to support recognition type : %d", ret);
		return ret;
	}

	*credential = temp;
	return 0;
}

int sttd_engine_agent_is_recognition_type_supported(const char* type, bool* support)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized");
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (NULL == type || NULL == support) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Invalid Parameter");
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (NULL == g_engine_info) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] The engine is not valid");
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (false == g_engine_info->is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not loaded engine");
		return STTD_ERROR_OPERATION_FAILED;
	}

	bool temp = false;
	int ret;

	ret = stt_engine_support_recognition_type(type, &temp);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to get to support recognition type : %d", ret);
		return ret;
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
				if (0 != stt_engine_set_silence_detection(g_default_silence_detected)) {
					SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to set silence detection : %s", g_default_silence_detected ? "true" : "false");
				} else {
					engine->silence_detection = g_default_silence_detected;
					SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] Set silence detection : %s", g_default_silence_detected ? "true" : "false");
				}
			}
		} else {
			if (silence != engine->silence_detection) {
				if (0 != stt_engine_set_silence_detection(silence)) {
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
				      int silence, const char* appid, const char* credential, void* user_param)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized");
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (NULL == lang || NULL == recognition_type) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Invalid Parameter");
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (NULL == g_engine_info) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] The engine is not valid");
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (false == g_engine_info->is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not loaded engine");
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (0 != __set_option(g_engine_info, silence)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to set options");
		return STTD_ERROR_OPERATION_FAILED;
	}

	SLOG(LOG_DEBUG, TAG_STTD, "g_default_language %s", g_default_language);

	int ret;
	char* temp = NULL;
	if (0 == strncmp(lang, "default", strlen("default"))) {
		bool is_valid = false;
		ret = stt_engine_is_valid_language(g_default_language, &is_valid);
		if (0 != ret) {
			SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to check valid language");
			return ret;
		}

		if (true == is_valid) {
			temp = strdup(g_default_language);
			SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent DEBUG] Default language is %s", temp);
		} else {
			temp = strdup(g_engine_info->first_lang);
			SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent DEBUG] Default language is engine first lang : %s", temp);
		}
	} else {
		temp = strdup(lang);
	}

	SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] Start engine");

	ret = stt_engine_recognize_start(temp, recognition_type, appid, credential, user_param);
	if (NULL != temp)	free(temp);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Recognition start error(%d)", ret);
		sttd_recorder_destroy();
		return ret;
	}

#ifdef AUDIO_CREATE_ON_START
	/* Ready recorder */
	stte_audio_type_e atype;
	int rate;
	int channels;

	ret = stt_engine_get_audio_type(&atype, &rate, &channels);
	if (0 != ret) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to get audio type : %s", g_engine_info->engine_name);
		return ret;
	}

	SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] Create recorder");

	ret = sttd_recorder_create(atype, channels, rate);
	if (0 != ret) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to create recorder : %s", g_engine_info->engine_name);
		return ret;
	}
#endif

#if 0
	SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] Start recorder(%d)", uid);

	ret = sttd_recorder_start(uid);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to start recorder : result(%d)", ret);
		return ret;
	}

#endif

	return 0;
}

int sttd_engine_agent_recognize_start_recorder(int uid)
{
	if (NULL == g_engine_info) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] The engine is not valid");
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (false == g_engine_info->is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not loaded engine");
		return STTD_ERROR_OPERATION_FAILED;
	}

	SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] Start recorder");

	int ret;
	ret = sttd_recorder_start(uid);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to start recorder : result(%d)", ret);
		stt_engine_recognize_cancel();
		sttd_recorder_stop();
		return ret;
	}

	return 0;
}

int sttd_engine_agent_set_recording_data(const void* data, unsigned int length)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized");
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (NULL == data || 0 == length) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Invalid Parameter");
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (NULL == g_engine_info) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] The engine is not valid");
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (false == g_engine_info->is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not loaded engine");
		return STTD_ERROR_OPERATION_FAILED;
	}

	int ret = stt_engine_set_recording_data(data, length);
	if (0 != ret) {
		SLOG(LOG_WARN, TAG_STTD, "[Engine Agent WARNING] set recording error(%d)", ret);
	}

	return ret;
}

int sttd_engine_agent_recognize_stop_recorder()
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized");
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_engine_info) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] The engine is not valid");
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (false == g_engine_info->is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not loaded engine");
		return STTD_ERROR_OPERATION_FAILED;
	}

	SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] Stop recorder");
	int ret;
	ret = sttd_recorder_stop();
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to stop recorder : result(%d)", ret);
		return ret;
	}

#ifdef AUDIO_CREATE_ON_START
	SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] Destroy recorder");
	if (0 != sttd_recorder_destroy())
		SECURE_SLOG(LOG_WARN, TAG_STTD, "[Engine Agent] Fail to destroy recorder");
#endif

	SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent Success] Stop recorder");
	return 0;
}

int sttd_engine_agent_recognize_stop_engine()
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized");
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_engine_info) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] The engine is not valid");
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (false == g_engine_info->is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not loaded engine");
		return STTD_ERROR_OPERATION_FAILED;
	}

	SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] Stop engine");

	int ret;
	ret = stt_engine_recognize_stop();
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] stop recognition error(%d)", ret);
		return ret;
	}

	SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent Success] Stop engine");

	return 0;
}

int sttd_engine_agent_recognize_cancel()
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized");
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_engine_info) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] The engine is not valid");
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (false == g_engine_info->is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not loaded engine");
		return STTD_ERROR_OPERATION_FAILED;
	}

	SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] Cancel engine");

	int ret;
	ret = stt_engine_recognize_cancel();
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] cancel recognition error(%d)", ret);
		return ret;
	}

	SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] Stop recorder");

	ret = sttd_recorder_stop();
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to stop recorder : result(%d)", ret);
		return ret;
	}

#ifdef AUDIO_CREATE_ON_START
	SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] Destroy recorder");
	if (0 != sttd_recorder_destroy())
		SECURE_SLOG(LOG_WARN, TAG_STTD, "[Engine Agent] Fail to destroy recorder");
#endif

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

	if (NULL == g_engine_info) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Default engine is not valid");
		return STTD_ERROR_ENGINE_NOT_FOUND;
	}

	SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] Default engine uuid(%s)", g_engine_info->engine_uuid);

	//if (0 != sttd_engine_agent_load_current_engine(data->uid, NULL, NULL)) {
	//	SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to load current engine : uid(%d)", data->uid);
	//}

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

int sttd_engine_agent_check_app_agreed(const char* appid, bool* result)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not Initialized");
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_engine_info) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] The engine of is not valid");
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (false == g_engine_info->is_loaded) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Not loaded engine");
		return STTD_ERROR_OPERATION_FAILED;
	}

	int ret;
	ret = stt_engine_check_app_agreed(appid, result);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] cancel recognition error(%d)", ret);
		return ret;
	}


	SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] Get engine right : %s", *result ? "true" : "false");
	return 0;
}

int sttd_engine_agent_send_result(stte_result_event_e event, const char* type, const char** result, int result_count,
		 const char* msg, void* time_info, void *user_data)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Result Callback : Not Initialized");
		return STTD_ERROR_OPERATION_FAILED;
	}

	SLOG(LOG_DEBUG, TAG_STTD, "[Server] === Result time callback ===");

	if (NULL != time_info) {
		/* Get the time info */
		int ret = stt_engine_foreach_result_time(time_info, __result_time_cb, NULL);
		if (0 != ret) {
			SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Fail to get time info : %d", ret);
			return STTD_ERROR_OPERATION_FAILED;
		}
	}

	SLOG(LOG_DEBUG, TAG_STTD, "[Server] ============================");

	g_result_cb(event, type, result, result_count, msg, user_data);

#ifdef AUDIO_CREATE_ON_START
	if (event == STTE_RESULT_EVENT_ERROR) {
		SLOG(LOG_DEBUG, TAG_STTD, "[Engine Agent] Destroy recorder");
		if (0 != sttd_recorder_destroy())
			SECURE_SLOG(LOG_WARN, TAG_STTD, "[Engine Agent] Fail to destroy recorder");
	}
#endif

	return STTD_ERROR_NONE;
}

int sttd_engine_agent_send_error(stte_error_e error, const char* msg)
{
	/* check uid */
	int uid = stt_client_get_current_recognition();

	char* err_msg = strdup(msg);
	int ret = STTE_ERROR_NONE;

	ret = sttdc_send_error_signal(uid, error, err_msg);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to send error info.");
	}

	if (NULL != err_msg) {
		free(err_msg);
		err_msg = NULL;
	}

	g_error_cb(error, msg);

	return ret;
}

int sttd_engine_agent_send_speech_status(stte_speech_status_e status, void* user_data)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_STTD, "[Engine Agent ERROR] Silence Callback : Not Initialized");
		return STTD_ERROR_OPERATION_FAILED;
	}

	g_speech_status_cb(status, user_data);
	return STTD_ERROR_NONE;
}

bool __result_time_cb(int index, stte_result_time_event_e event, const char* text, long start_time, long end_time, void* user_data)
{
	return g_result_time_cb(index, event, text, start_time, end_time, user_data);
}

/* A function forging */
int __log_enginelist()
{
	if (NULL != g_engine_info) {
		SLOG(LOG_DEBUG, TAG_STTD, "------------------ engine -----------------------");
		SLOG(LOG_DEBUG, TAG_STTD, "engine uuid : %s", g_engine_info->engine_uuid);
		SLOG(LOG_DEBUG, TAG_STTD, "engine name : %s", g_engine_info->engine_name);
		SLOG(LOG_DEBUG, TAG_STTD, "engine path : %s", g_engine_info->engine_path);
		SLOG(LOG_DEBUG, TAG_STTD, "use network : %s", g_engine_info->use_network ? "true" : "false");
		SLOG(LOG_DEBUG, TAG_STTD, "is loaded   : %s", g_engine_info->is_loaded ? "true" : "false");
		if (NULL != g_engine_info->first_lang) {
			SLOG(LOG_DEBUG, TAG_STTD, "default lang : %s", g_engine_info->first_lang);
		}
		SLOG(LOG_DEBUG, TAG_STTD, "-------------------------------------------------");
	} else {
		SLOG(LOG_DEBUG, TAG_STTD, "------------------ engine -----------------------");
		SLOG(LOG_DEBUG, TAG_STTD, "	No engine");
		SLOG(LOG_DEBUG, TAG_STTD, "-------------------------------------------------");
	}

	return 0;
}
