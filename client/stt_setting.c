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

#include <dirent.h>
#include <Ecore.h>
#include <libxml/parser.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "stt_config_mgr.h"
#include "stt_main.h"
#include "stt_setting.h"


static stt_setting_state_e g_state = STT_SETTING_STATE_NONE;

static stt_setting_supported_engine_cb g_engine_cb;
static void* g_user_data;

static stt_setting_config_changed_cb g_config_changed_cb;
static void* g_config_changed_user_data;

static stt_setting_engine_changed_cb g_engine_changed_cb;
static void* g_engine_changed_user_data;

const char* stt_tag()
{
	return "sttc";
}

void __config_engine_changed_cb(const char* engine_id, const char* setting, const char* language, bool support_silence, void* user_data)
{
	if (NULL != engine_id)	SECURE_SLOG(LOG_DEBUG, TAG_STTC, "Engine id(%s)", engine_id);
	if (NULL != setting)	SECURE_SLOG(LOG_DEBUG, TAG_STTC, "Engine setting(%s)", setting);
	if (NULL != language)	SECURE_SLOG(LOG_DEBUG, TAG_STTC, "Language(%s)", language);
	SECURE_SLOG(LOG_DEBUG, TAG_STTC, "Silence(%s)", support_silence ? "on" : "off");

	if (NULL != g_config_changed_cb)
		g_engine_changed_cb(g_config_changed_user_data);
}

void __config_lang_changed_cb(const char* before_language, const char* current_language, void* user_data)
{
	SECURE_SLOG(LOG_DEBUG, TAG_STTC, "Lang changed : lang(%s)", current_language);

	if (NULL != g_config_changed_cb)
		g_config_changed_cb(g_config_changed_user_data);
}

void __config_bool_changed_cb(stt_config_type_e type, bool bool_value, void* user_data)
{
	SECURE_SLOG(LOG_DEBUG, TAG_STTC, " type(%d) bool(%s)", type, bool_value ? "on" : "off");

	if (NULL != g_config_changed_cb)
		g_config_changed_cb(g_config_changed_user_data);
}

int stt_setting_initialize(void)
{
	SLOG(LOG_DEBUG, TAG_STTC, "===== Initialize STT Setting");

	if (STT_SETTING_STATE_READY == g_state) {
		SLOG(LOG_WARN, TAG_STTC, "[WARNING] STT Setting has already been initialized. ");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_SETTING_ERROR_NONE;
	}

	int ret = stt_config_mgr_initialize(getpid());
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to initialize config manager : %d", ret);
		return STT_SETTING_ERROR_OPERATION_FAILED;
	}

	ret = stt_config_mgr_set_callback(getpid(), __config_engine_changed_cb, __config_lang_changed_cb, __config_bool_changed_cb, NULL);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to set config changed : %d", ret);
		return STT_SETTING_ERROR_OPERATION_FAILED;
	}

	g_state = STT_SETTING_STATE_READY;

	g_engine_changed_cb = NULL;
	g_engine_changed_user_data = NULL;

	g_config_changed_cb = NULL;
	g_config_changed_user_data = NULL;

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return STT_SETTING_ERROR_NONE;
}

int stt_setting_finalize ()
{
	SLOG(LOG_DEBUG, TAG_STTC, "===== Finalize STT Setting");
	
	stt_config_mgr_finalize(getpid());

	g_state = STT_SETTING_STATE_NONE;

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return STT_SETTING_ERROR_NONE;
}

bool __config_mgr_get_engine_list(const char* engine_id, const char* engine_name, const char* setting, bool support_silence, void* user_data)
{
	return g_engine_cb(engine_id, engine_name, setting, g_user_data);
}

int stt_setting_foreach_supported_engines(stt_setting_supported_engine_cb callback, void* user_data)
{
	SLOG(LOG_DEBUG, TAG_STTC, "===== Foreach supported engines");

	if (STT_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Not initialized");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_SETTING_ERROR_INVALID_STATE;
	}

	if (NULL == callback) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Callback is NULL");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_SETTING_ERROR_INVALID_PARAMETER;
	}

	g_engine_cb = callback;
	g_user_data = user_data;

	int ret = stt_config_mgr_get_engine_list(__config_mgr_get_engine_list, NULL);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Result : %d", ret);
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Foreach supported engines");
	}

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return ret;
}

int stt_setting_get_engine(char** engine_id)
{
	SLOG(LOG_DEBUG, TAG_STTC, "===== Get current engine");

	if (STT_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Not initialized");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_SETTING_ERROR_INVALID_STATE;
	}

	if (NULL == engine_id) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Engine id is NULL");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_SETTING_ERROR_INVALID_PARAMETER;
	}

	int ret = stt_config_mgr_get_engine(engine_id);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Result : %d", ret);
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Get current engine");
	}

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return ret;
}

int stt_setting_set_engine(const char* engine_id)
{
	SLOG(LOG_DEBUG, TAG_STTC, "===== Set current engine");

	if (STT_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Not initialized");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_SETTING_ERROR_INVALID_STATE;
	}

	if (NULL == engine_id) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Engine id is NULL");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_SETTING_ERROR_INVALID_PARAMETER;
	}

	SECURE_SLOG(LOG_DEBUG, TAG_STTC, "New engine id : %s", engine_id);

	int ret = stt_config_mgr_set_engine(engine_id);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Result : %d", ret);
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Set current engine");
	}

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");
    
	return ret;
}

int stt_setting_foreach_supported_languages(stt_setting_supported_language_cb callback, void* user_data)
{
	SLOG(LOG_DEBUG, TAG_STTC, "===== Foreach supported languages");

	if (STT_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Not initialized");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_SETTING_ERROR_INVALID_STATE;
	}

	if (NULL == callback) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Param is NULL");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_SETTING_ERROR_INVALID_PARAMETER;
	}

	char* current_engine;
	int ret = stt_config_mgr_get_engine(&current_engine);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to get current engine : %d", ret);
		return -1;
	} 

	ret = stt_config_mgr_get_language_list(current_engine, (stt_config_supported_langauge_cb)callback, user_data);

	if (NULL != current_engine)
		free(current_engine);

	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Result : %d", ret);
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Foreach supported languages");
	}
	
	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return ret;
}

int stt_setting_get_default_language(char** language)
{
	SLOG(LOG_DEBUG, TAG_STTC, "===== Get default language");

	if (STT_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Not initialized");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_SETTING_ERROR_INVALID_STATE;
	}

	if (NULL == language) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Input parameter is NULL");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_SETTING_ERROR_INVALID_PARAMETER;
	}

	int ret = stt_config_mgr_get_default_language(language);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Result : %d", ret);
	} else {
		SECURE_SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Default language : %s", *language);
	}

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return ret;
}

int stt_setting_set_default_language(const char* language)
{
	SLOG(LOG_DEBUG, TAG_STTC, "===== Set default language");

	if (STT_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Not initialized");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_SETTING_ERROR_INVALID_STATE;
	}

	if (NULL == language) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Input parameter is NULL");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_SETTING_ERROR_INVALID_PARAMETER;
	}

	int ret = stt_config_mgr_set_default_language(language);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Result : %d", ret);
	} else {
		SECURE_SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Set default language : %s", language);
	}

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return ret;
}

int stt_setting_set_auto_language(bool value)
{
	SLOG(LOG_DEBUG, TAG_STTC, "===== Set auto voice");

	if (STT_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Not initialized");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_SETTING_ERROR_INVALID_STATE;
	}

	if (value != true && value != false) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid value");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_SETTING_ERROR_INVALID_PARAMETER;
	}

	int ret = stt_config_mgr_set_auto_language(value);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Result : %d", ret);
	} else {
		SECURE_SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Set auto language (%s)", value ? "on" : "off");
	}

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return ret;
}

int stt_setting_get_auto_language(bool* value)
{
	SLOG(LOG_DEBUG, TAG_STTC, "===== Get auto language");

	if (STT_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Not initialized");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_SETTING_ERROR_INVALID_STATE;
	}

	if (NULL == value) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Param is NULL");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_SETTING_ERROR_INVALID_PARAMETER;
	}

	int ret = stt_config_mgr_get_auto_language(value);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Result : %d", ret);
	} else {
		/* Copy value */
		SECURE_SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Get auto language (%s)", *value ? "true":"false");
	}

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return 0;
}

int stt_setting_get_silence_detection(bool* value)
{
	SLOG(LOG_DEBUG, TAG_STTC, "===== Get silence detection");

	if (STT_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] not initialized");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_SETTING_ERROR_INVALID_STATE;
	}

	if (NULL == value) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] param is NULL");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_SETTING_ERROR_INVALID_PARAMETER;
	}

	int ret = stt_config_mgr_get_silence_detection(value);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Result (%d)", ret);    
	} else {
		SECURE_SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Get silence detection(%s)", *value ? "true":"false");
	}

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return ret;
}

int stt_setting_set_silence_detection(bool value)
{
	SLOG(LOG_DEBUG, TAG_STTC, "===== Set silence detection");

	if (STT_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] not initialized");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_SETTING_ERROR_INVALID_STATE;
	}

	int ret = stt_config_mgr_set_silence_detection(value);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Result (%d)", ret);    
	} else {
		SECURE_SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Set silence detection(%s)", value ? "true":"false");
	}

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return ret;
}

int stt_setting_set_engine_changed_cb(stt_setting_engine_changed_cb callback, void* user_data)
{
	if (NULL == callback) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Input param is NULL");
		return STT_SETTING_ERROR_INVALID_PARAMETER;
	}

	g_engine_changed_cb = callback;
	g_engine_changed_user_data = user_data;

	return STT_SETTING_ERROR_NONE;
}

int stt_setting_unset_engine_changed_cb()
{
	g_engine_changed_cb = NULL;
	g_engine_changed_user_data = NULL;

	return STT_SETTING_ERROR_NONE;
}

int stt_setting_set_config_changed_cb(stt_setting_config_changed_cb callback, void* user_data)
{
	if (NULL == callback) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Input param is NULL");
		return STT_SETTING_ERROR_INVALID_PARAMETER;
	}

	g_config_changed_cb = callback;
	g_config_changed_user_data = user_data;

	return STT_SETTING_ERROR_NONE;
}

int stt_setting_unset_config_changed_cb()
{
	g_config_changed_cb = NULL;
	g_config_changed_user_data = NULL;

	return STT_SETTING_ERROR_NONE;
}