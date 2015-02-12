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


#include "stt_config_mgr.h"
#include "sttd_config.h"
#include "sttd_main.h"


static sttd_config_engine_changed_cb g_engine_cb;

static sttd_config_language_changed_cb g_lang_cb;

static sttd_config_silence_changed_cb g_silence_cb;

static void* g_user_data;


const char* stt_tag()
{
	return "sttd";
}


void __sttd_config_engine_changed_cb(const char* engine_id, const char* setting, const char* language, bool support_silence, void* user_data)
{
	/* Need to check engine is valid */
	if (false == stt_config_check_default_engine_is_valid(engine_id)) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "Engine id is NOT valid : %s", engine_id);
		return;
	}

	if (NULL != g_engine_cb)
		g_engine_cb(engine_id, language, support_silence, g_user_data);
	else
		SLOG(LOG_ERROR, TAG_STTD, "Engine changed callback is NULL");
}

void __sttd_config_lang_changed_cb(const char* before_language, const char* current_language, void* user_data)
{
	if (false == stt_config_check_default_language_is_valid(current_language)) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "Language is NOT valid : %s", current_language);
		return;
	}

	if (NULL != g_lang_cb)
		g_lang_cb(current_language, g_user_data);
	else
		SLOG(LOG_ERROR, TAG_STTD, "Language changed callback is NULL");
}

void __config_bool_changed_cb(stt_config_type_e type, bool bool_value, void* user_data)
{
	SECURE_SLOG(LOG_DEBUG, TAG_STTD, " type(%d) bool(%s)", type, bool_value ? "on" : "off");

	if (STT_CONFIG_TYPE_OPTION_SILENCE_DETECTION == type) {
		if (NULL != g_silence_cb){
			g_silence_cb(bool_value, g_user_data);
			SLOG(LOG_DEBUG, TAG_STTD, "Call back silence detection changed");
		}
	}

	return;
}

int sttd_config_initialize(sttd_config_engine_changed_cb engine_cb, 
			   sttd_config_language_changed_cb lang_cb, 
			   sttd_config_silence_changed_cb silence_cb, 
			   void* user_data)
{
	if (NULL == engine_cb || NULL == lang_cb || NULL == silence_cb) {
		SLOG(LOG_ERROR, TAG_STTD, "[Config] Invalid parameter");
		return -1;
	}

	int ret = -1;
	ret = stt_config_mgr_initialize(getpid());
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Config] Fail to initialize config manager");
		return -1;
	}

	ret = stt_config_mgr_set_callback(getpid(), __sttd_config_engine_changed_cb, __sttd_config_lang_changed_cb, 
		__config_bool_changed_cb, NULL);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[ERROR] Fail to set config changed : %d", ret);
		return -1;
	}

	g_engine_cb = engine_cb;
	g_lang_cb = lang_cb;
	g_silence_cb = silence_cb;
	g_user_data = user_data;

	return 0;
}

int sttd_config_finalize()
{
	stt_config_mgr_finalize(getpid());

	return 0;
}

int sttd_config_get_default_engine(char** engine_id)
{
	if (NULL == engine_id)
		return -1;

	if (0 != stt_config_mgr_get_engine(engine_id)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Config ERROR] Fail to get engine id");
	}

	return 0;
}

int sttd_config_set_default_engine(const char* engine_id)
{
	if (NULL == engine_id)
		return -1;

	if (0 != stt_config_mgr_set_engine(engine_id)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Config ERROR] Fail to set engine id");
	}

	return 0;
}

int sttd_config_get_default_language(char** language)
{
	if (NULL == language)
		return -1;

	if (0 != stt_config_mgr_get_default_language(language)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Config ERROR] Fail to get language");
		return -1;
	}

	return 0;
}

int sttd_config_get_default_silence_detection(int* silence)
{
	if (NULL == silence)
		return -1;

	bool value;
	if (0 != stt_config_mgr_get_silence_detection(&value)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Config ERROR] Fail to set language");
		return -1;
	}

	*silence = (int)value;

	return 0;
}

int sttd_config_time_add(int index, int event, const char* text, long start_time, long end_time)
{
	return stt_config_mgr_add_time_info(index, event, text, start_time, end_time);
}

int sttd_config_time_save()
{
	return stt_config_mgr_save_time_info_file();
}

int sttd_config_time_reset()
{
	return stt_config_mgr_reset_time_info();
}