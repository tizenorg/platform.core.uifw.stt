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

#include <dlog.h>
#include <dlfcn.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "stt_engine.h"
#include "sttd_engine_agent.h"

/*
* Internal data structure
*/

typedef struct {
	char*	engine_path;
	stte_request_callback_s *callback;
} sttengine_s;

extern const char* stt_tag();

/** stt engine */
static sttengine_s *g_engine = NULL;

static bool g_is_from_lib = false;

/** callback functions */
static stt_engine_result_cb g_result_cb = NULL;
static stte_private_data_set_cb g_set_private_data_cb = NULL;
static stte_private_data_requested_cb g_get_private_data_cb = NULL;


static int __stt_set_engine_from(bool is_from_lib)
{
	g_is_from_lib = is_from_lib;
	return 0;
}

static bool __stt_get_engine_from(void)
{
	return g_is_from_lib;
}

static const char* __stt_get_engine_error_code(stte_error_e err)
{
	switch (err) {
	case STTE_ERROR_NONE:			return "STTE_ERROR_NONE";
	case STTE_ERROR_OUT_OF_MEMORY:		return "STTE_ERROR_OUT_OF_MEMORY";
	case STTE_ERROR_IO_ERROR:		return "STTE_ERROR_IO_ERROR";
	case STTE_ERROR_INVALID_PARAMETER:	return "STTE_ERROR_INVALID_PARAMETER";
	case STTE_ERROR_NETWORK_DOWN:		return "STTE_ERROR_NETWORK_DOWN";
	case STTE_ERROR_PERMISSION_DENIED:	return "STTE_ERROR_PERMISSION_DENIED";
	case STTE_ERROR_NOT_SUPPORTED:		return "STTE_ERROR_NOT_SUPPORTED";
	case STTE_ERROR_INVALID_STATE:		return "STTE_ERROR_INVALID_STATE";
	case STTE_ERROR_INVALID_LANGUAGE:	return "STTE_ERROR_INVALID_LANGUAGE";
	case STTE_ERROR_OPERATION_FAILED:	return "STTE_ERROR_OPERATION_FAILED";
	case STTE_ERROR_NOT_SUPPORTED_FEATURE:	return "STTE_ERROR_NOT_SUPPORTED_FEATURE";
	case STTE_ERROR_RECORDING_TIMED_OUT:	return "STTE_ERROR_RECORDING_TIMED_OUT";
	default:
		return "Invalid error code";
	}
}

/* Register engine id */
int stt_engine_load(const char* filepath, stte_request_callback_s *callback)
{
	if (NULL == callback || NULL == filepath) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid Parameter");
		return STTE_ERROR_INVALID_PARAMETER;
	}

	/* allocation memory */
	if (NULL != g_engine) {
		SLOG(LOG_WARN, stt_tag(), "[Engine WARNING] engine is already loaded");
	} else {
		g_engine = (sttengine_s*)calloc(1, sizeof(sttengine_s));
		if (NULL == g_engine) {
			SLOG(LOG_ERROR, stt_tag(), "[ERROR] Fail to allocate memory");
			return STTE_ERROR_OUT_OF_MEMORY;
		}

		/* load engine */
		g_engine->callback = callback;
		g_engine->engine_path = strdup(filepath);
	}

	SLOG(LOG_DEBUG, stt_tag(), "[Engine Success] Load engine : version(%d)", g_engine->callback->version);

	return 0;
}

/* Unregister engine id */
int stt_engine_unload()
{
	if (NULL == g_engine) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] No engine");
		return STTE_ERROR_OPERATION_FAILED;
	}

	g_engine->callback = NULL;

	if (NULL != g_engine->engine_path) {
		free(g_engine->engine_path);
		g_engine->engine_path = NULL;
	}

	free(g_engine);
	g_engine = NULL;

	return 0;
}


/* Initialize / Deinitialize */
int stt_engine_initialize(bool is_from_lib)
{
	if (NULL == g_engine) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] No engine");
		return STTE_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_engine->callback) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid engine");
		return STTE_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_engine->callback->initialize) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid engine");
		return STTE_ERROR_OPERATION_FAILED;
	}

	int ret;
	ret = __stt_set_engine_from(is_from_lib);
	if (0 != ret) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Fail to set engine : %s", __stt_get_engine_error_code(ret));
		return STTE_ERROR_OPERATION_FAILED;
	}

	ret = g_engine->callback->initialize();
	if (0 != ret) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Fail to initialize : %s", __stt_get_engine_error_code(ret));
	}

	return ret;
}

int stt_engine_deinitialize()
{
	if (NULL == g_engine) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] No engine");
		return STTE_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_engine->callback) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid engine");
		return STTE_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_engine->callback->deinitialize) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid engine");
		return STTE_ERROR_OPERATION_FAILED;
	}

	int ret;
	ret = g_engine->callback->deinitialize();
	if (0 != ret) {
		SLOG(LOG_WARN, stt_tag(), "[Engine WARNING] Fail to deinitialize : %s", __stt_get_engine_error_code(ret));
	}

	return ret;
}

static bool __supported_language_cb(const char* language, void* user_data)
{
	GSList** lang_list = (GSList**)user_data;

	if (NULL == language || NULL == lang_list) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Input parameter is NULL in callback!!!!");
		return false;
	}

	char* temp_lang = g_strdup(language);

	*lang_list = g_slist_append(*lang_list, temp_lang);

	return true;
}

/* Get option */
int stt_engine_get_supported_langs(GSList** lang_list)
{
	if (NULL == lang_list) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid Parameter");
		return STTE_ERROR_INVALID_PARAMETER;
	}

	if (NULL == g_engine) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] No engine");
		return STTE_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_engine->callback) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid engine");
		return STTE_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_engine->callback->foreach_langs) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid engine");
		return STTE_ERROR_OPERATION_FAILED;
	}

	int ret;
	ret = g_engine->callback->foreach_langs(__supported_language_cb, (void*)lang_list);
	if (0 != ret) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] get language list error : %s", __stt_get_engine_error_code(ret));
	}

	return ret;
}

int stt_engine_is_valid_language(const char* language, bool *is_valid)
{
	if (NULL == language || NULL == is_valid) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid Parameter");
		return STTE_ERROR_INVALID_PARAMETER;
	}

	if (NULL == g_engine) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] No engine");
		return STTE_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_engine->callback) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid engine");
		return STTE_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_engine->callback->is_valid_lang) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid engine");
		return STTE_ERROR_OPERATION_FAILED;
	}

	int ret = STTE_ERROR_NONE;
	ret = g_engine->callback->is_valid_lang(language, is_valid);
	if (0 != ret) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Fail to check valid language(%d)", ret);
	}
	return ret;
}

int stt_engine_set_private_data(const char* key, const char* data)
{
	if (NULL == key || NULL == data) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid Parameter");
		return STTE_ERROR_INVALID_PARAMETER;
	}

	int ret = STTE_ERROR_NONE;
	if (NULL != g_set_private_data_cb) {
		ret = g_set_private_data_cb(key, data);
		if (0 != ret) {
			SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Fail to set private data(%d)", ret);
		}
	}

	return ret;
}

int stt_engine_get_private_data(const char* key, char** data)
{
	if (NULL == key || NULL == data) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid Parameter");
		return STTE_ERROR_INVALID_PARAMETER;
	}

	int ret = STTE_ERROR_NONE;
	char* temp = NULL;
	if (NULL != g_get_private_data_cb) {
		ret = g_get_private_data_cb(key, &temp);
		if (0 != ret) {
			SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Fail to get private data(%d)", ret);
			return ret;
		}
	} else {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] There's no private data function)");
	}

	if (NULL == temp)
		*data = strdup("NULL");
	else
		*data = strdup(temp);

	return STTE_ERROR_NONE;
}

int stt_engine_get_first_language(char** language)
{
	if (NULL == language) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid Parameter");
		return STTE_ERROR_INVALID_PARAMETER;
	}

	if (NULL == g_engine) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] No engine");
		return STTE_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_engine->callback) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid engine");
		return STTE_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_engine->callback->foreach_langs || NULL == g_engine->callback->is_valid_lang) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid engine");
		return STTE_ERROR_OPERATION_FAILED;
	}

	GSList* lang_list = NULL;
	int ret;
	ret = g_engine->callback->foreach_langs(__supported_language_cb, &lang_list);
	if (0 != ret) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] get language list error : %s", __stt_get_engine_error_code(ret));
		return ret;
	}

	GSList *iter = NULL;
	char* data = NULL;

	iter = g_slist_nth(lang_list, 0);
	if (NULL != iter) {
		data = iter->data;

		bool is_valid = false;
		ret = g_engine->callback->is_valid_lang(data, &is_valid);
		if (0 == ret && true == is_valid) {
			*language = strdup(data);
		} else {
			ret = STTE_ERROR_OPERATION_FAILED;
		}
	}

	/* if list have item */
	if (g_slist_length(lang_list) > 0) {
		/* Get a first item */
		iter = g_slist_nth(lang_list, 0);

		while (NULL != iter) {
			data = iter->data;

			if (NULL != data)
				free(data);

			lang_list = g_slist_remove_link(lang_list, iter);

			iter = g_slist_nth(lang_list, 0);
		}
	}

	return ret;
}

int stt_engine_support_silence(bool* support)
{
	if (NULL == support) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid Parameter");
		return STTE_ERROR_INVALID_PARAMETER;
	}

	if (NULL == g_engine) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] No engine");
		return STTE_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_engine->callback) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid engine");
		return STTE_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_engine->callback->support_silence) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid engine");
		return STTE_ERROR_OPERATION_FAILED;
	}

	bool result;
	result = g_engine->callback->support_silence();
	*support = result;

	return STTE_ERROR_NONE;
}

int stt_engine_need_app_credential(bool* need)
{
	if (NULL == need) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid Parameter");
		return STTE_ERROR_INVALID_PARAMETER;
	}

	if (NULL == g_engine) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] No engine");
		return STTE_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_engine->callback) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid engine");
		return STTE_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_engine->callback->need_app_credential) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid engine");
		return STTE_ERROR_OPERATION_FAILED;
	}

	bool result;
	result = g_engine->callback->need_app_credential();
	*need = result;

	return STTE_ERROR_NONE;
}

int stt_engine_support_recognition_type(const char* type, bool* support)
{
	if (NULL == type || NULL == support) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid Parameter");
		return STTE_ERROR_INVALID_PARAMETER;
	}

	if (NULL == g_engine) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] No engine");
		return STTE_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_engine->callback) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid engine");
		return STTE_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_engine->callback->support_recognition_type) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR} Invalid engine");
		return STTE_ERROR_OPERATION_FAILED;
	}

	int ret = STTE_ERROR_NONE;
	ret = g_engine->callback->support_recognition_type(type, support);
	if (0 != ret) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Fail to get supporting recognition type(%d)", ret);
	}
	return ret;
}

int stt_engine_get_audio_type(stte_audio_type_e* types, int* rate, int* channels)
{
	if (NULL == types || NULL == rate || NULL == channels) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid Parameter");
		return STTE_ERROR_INVALID_PARAMETER;
	}

	if (NULL == g_engine) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] No engine");
		return STTE_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_engine->callback) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid engine");
		return STTE_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_engine->callback->get_audio_format) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid engine");
		return STTE_ERROR_OPERATION_FAILED;
	}

	int ret;
	ret = g_engine->callback->get_audio_format(types, rate, channels);
	if (0 != ret) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Fail to get audio format : %s", __stt_get_engine_error_code(ret));
	}

	return ret;
}

/* Set option */
int stt_engine_set_silence_detection(bool value)
{
	if (NULL == g_engine) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] No engine");
		return STTE_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_engine->callback) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid engine");
		return STTE_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_engine->callback->set_silence_detection) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid engine");
		return STTE_ERROR_OPERATION_FAILED;
	}

	int ret = g_engine->callback->set_silence_detection(value);
	if (STTE_ERROR_NOT_SUPPORTED_FEATURE == ret) {
		SLOG(LOG_WARN, stt_tag(), "[Engine WARNING] Not support silence detection");
	} else if (0 != ret) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Fail to set silence detection : %d", ret);
	}
	return ret;
}

int stt_engine_check_app_agreed(const char* appid, bool* is_agreed)
{
	if (NULL == is_agreed) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid parameter");
		return STTE_ERROR_INVALID_PARAMETER;
	}

	if (NULL == g_engine) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] No engine");
		return STTE_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_engine->callback) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid engine");
		return STTE_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_engine->callback->check_app_agreed) {
		SLOG(LOG_WARN, stt_tag(), "[Engine WARNING] Not support app agreement. All app is available");
		*is_agreed = true;
		return 0;
	}

	int ret = g_engine->callback->check_app_agreed(appid, is_agreed);
	if (0 != ret) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Fail to get app agreement : %s", __stt_get_engine_error_code(ret));
		*is_agreed = false;
	}

	return ret;
}

/* Recognition */
int stt_engine_recognize_start(const char* lang, const char* recognition_type, const char* appid, const char* credential, void* user_param)
{
	if (NULL == lang || NULL == recognition_type) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid Parameter");
		return STTE_ERROR_INVALID_PARAMETER;
	}

	if (NULL == g_engine) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] No engine");
		return STTE_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_engine->callback) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid engine");
		return STTE_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_engine->callback->start) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid engine");
		return STTE_ERROR_OPERATION_FAILED;
	}

	int ret = g_engine->callback->start(lang, recognition_type, appid, credential, user_param);

	if (0 != ret) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Fail to start recognition : %s", __stt_get_engine_error_code(ret));
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Fail to start recognition : lang(%s), recognition_type(%s), credential(%s)", lang, recognition_type, credential);
	}

	return ret;
}

int stt_engine_set_recording_data(const void* data, unsigned int length)
{
	if (NULL == data) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid Parameter");
		return STTE_ERROR_INVALID_PARAMETER;
	}

	if (NULL == g_engine) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] No engine");
		return STTE_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_engine->callback) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid engine");
		return STTE_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_engine->callback->set_recording) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid engine");
		return STTE_ERROR_OPERATION_FAILED;
	}

	int ret = g_engine->callback->set_recording(data, length);
	if (0 != ret) {
		SLOG(LOG_WARN, stt_tag(), "[Engine WARNING] Fail to set recording : %s", __stt_get_engine_error_code(ret));
	}

	return ret;
}

int stt_engine_recognize_stop()
{
	if (NULL == g_engine) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] No engine");
		return STTE_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_engine->callback) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid engine");
		return STTE_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_engine->callback->stop) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid engine");
		return STTE_ERROR_OPERATION_FAILED;
	}

	int ret = g_engine->callback->stop();
	if (0 != ret) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Fail to stop : %s", __stt_get_engine_error_code(ret));
	}

	return ret;
}

int stt_engine_recognize_cancel()
{
	if (NULL == g_engine) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] No engine");
		return STTE_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_engine->callback) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid engine");
		return STTE_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_engine->callback->cancel) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid engine");
		return STTE_ERROR_OPERATION_FAILED;
	}

	int ret = g_engine->callback->cancel();
	if (0 != ret) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Fail to cancel : %s", __stt_get_engine_error_code(ret));
	}

	return ret;
}

int stt_engine_foreach_result_time(void* time_info, stte_result_time_cb callback, void* user_data)
{
	if (NULL == g_engine) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] No engine");
		return STTE_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_engine->callback) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid engine");
		return STTE_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_engine->callback->foreach_result_time) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid engine");
		return STTE_ERROR_OPERATION_FAILED;
	}

	int ret = g_engine->callback->foreach_result_time(time_info, callback, user_data);
	if (0 != ret) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Fail to foreach result time : %s", __stt_get_engine_error_code(ret));
	}

	return ret;
}

int stt_engine_recognize_start_file(const char* lang, const char* recognition_type, 
				     const char* filepath, stte_audio_type_e audio_type, int sample_rate, void* user_param)
{
	if (NULL == filepath || NULL == lang || NULL == recognition_type) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid Parameter");
		return STTE_ERROR_INVALID_PARAMETER;
	}

	if (NULL == g_engine) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] No engine");
		return STTE_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_engine->callback) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid engine");
		return STTE_ERROR_OPERATION_FAILED;
	}

#ifdef __UNUSED_CODES__
	if (NULL == g_engine->callback->start_file) {
		SLOG(LOG_WARN, stt_tag(), "[Engine WARNING] engine API is invalid");
		return STTE_ERROR_NOT_SUPPORTED_FEATURE;
	}

	int ret = g_engine->callback->start_file(lang, recognition_type, filepath, audio_type, sample_rate, user_param);
	if (0 != ret) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Fail to start file recognition : %s", __stt_get_engine_error_code(ret));
	}
#endif
	return 0;
}

int stt_engine_recognize_cancel_file()
{
	if (NULL == g_engine) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] No engine");
		return STTE_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_engine->callback) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid engine");
		return STTE_ERROR_OPERATION_FAILED;
	}

#ifdef __UNUSED_CODES__
	if (NULL == g_engine->callback->cancel_file) {
		SLOG(LOG_WARN, stt_tag(), "[Engine WARNING] engine API is invalid");
		return STTE_ERROR_NOT_SUPPORTED_FEATURE;
	}

	int ret = g_engine->callback->cancel_file();
	if (0 != ret) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Fail to start file recognition : %s", __stt_get_engine_error_code(ret));
	}
#endif
	return 0;
}

int stt_engine_set_recognition_result_cb(stt_engine_result_cb result_cb, void* user_data)
{
	if (NULL == result_cb) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid parameter");
		return STTE_ERROR_INVALID_PARAMETER;
	}

	g_result_cb = result_cb;

	return 0;
}

int stt_engine_send_result(stte_result_event_e event, const char* type, const char** result, int result_count,
				const char* msg, void* time_info, void* user_data)
{
	if (NULL == type || NULL == result) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid parameter");
	}

	int ret = STTE_ERROR_NONE;
	if (false == __stt_get_engine_from()) {
		ret = sttd_engine_agent_send_result(event, type, result, result_count, msg, time_info, user_data);
		if (0 != ret) {
			SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Fail to send result");
		}
	} else {
		g_result_cb(event, type, result, result_count, msg, time_info, user_data);
	}
	return ret;
}

int stt_engine_send_error(stte_error_e error, const char* msg)
{
	if (NULL == msg) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid parameter");
	}

	int ret = STTE_ERROR_NONE;
	ret = sttd_engine_agent_send_error(error, msg);
	if (0 != ret) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Fail to send error info");
	}
	return ret;
}

int stt_engine_send_speech_status(stte_speech_status_e status, void* user_data)
{
	int ret = STTE_ERROR_NONE;
	ret = sttd_engine_agent_send_speech_status(status, user_data);
	if (0 != ret) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Fail to send speech status");
	}
	return ret;
}

int stt_engine_set_private_data_set_cb(stte_private_data_set_cb private_data_set_cb, void* user_data)
{
	if (NULL == private_data_set_cb) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid parameter");
		return STTE_ERROR_INVALID_PARAMETER;
	}

	g_set_private_data_cb = private_data_set_cb;

	return 0;
}

int stt_engine_set_private_data_requested_cb(stte_private_data_requested_cb private_data_requested_cb, void* user_data)
{
	if (NULL == private_data_requested_cb) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid parameter");
		return STTE_ERROR_INVALID_PARAMETER;
	}

	g_get_private_data_cb = private_data_requested_cb;

	return 0;
}
