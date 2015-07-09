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

#include <dlog.h>
#include <dlfcn.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "stt_engine.h"

/*
* Internal data structure
*/

typedef struct {
	int	engine_id;
	char*	engine_path;
	void	*handle;

	sttpe_funcs_s*	pefuncs;
	sttpd_funcs_s*	pdfuncs;

	int (*sttp_load_engine)(sttpd_funcs_s* pdfuncs, sttpe_funcs_s* pefuncs);
	int (*sttp_unload_engine)();
}sttengine_s;

extern const char* stt_tag();

/** stt engine list */
static GSList *g_engine_list;

static const char* __stt_get_engine_error_code(sttp_error_e err)
{
	switch(err) {
	case STTP_ERROR_NONE:			return "STTP_ERROR_NONE";
	case STTP_ERROR_OUT_OF_MEMORY:		return "STTP_ERROR_OUT_OF_MEMORY";
	case STTP_ERROR_IO_ERROR:		return "STTP_ERROR_IO_ERROR";
	case STTP_ERROR_INVALID_PARAMETER:	return "STTP_ERROR_INVALID_PARAMETER";
	case STTP_ERROR_OUT_OF_NETWORK:		return "STTP_ERROR_OUT_OF_NETWORK";
	case STTP_ERROR_INVALID_STATE:		return "STTP_ERROR_INVALID_STATE";
	case STTP_ERROR_INVALID_LANGUAGE:	return "STTP_ERROR_INVALID_LANGUAGE";
	case STTP_ERROR_OPERATION_FAILED:	return "STTP_ERROR_OPERATION_FAILED";
	case STTP_ERROR_NOT_SUPPORTED_FEATURE:	return "STTP_ERROR_NOT_SUPPORTED_FEATURE";
	default:
		return "Invalid error code";
	}
}

static sttengine_s* __get_engine(int engine_id)
{
	/* check whether engine id is valid or not.*/
	GSList *iter = NULL;
	sttengine_s *engine = NULL;

	if (g_slist_length(g_engine_list) > 0) {
		/*Get a first item*/
		iter = g_slist_nth(g_engine_list, 0);

		while (NULL != iter) {
			/*Get handle data from list*/
			engine = iter->data;

			if (engine_id == engine->engine_id) {
				return engine;
			}

			/*Get next item*/
			iter = g_slist_next(iter);
		}
	}

	return NULL;
}

/* Register engine id */
int stt_engine_load(int engine_id, const char* filepath)
{
	if (NULL == filepath || engine_id < 0) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid Parameter"); 
		return STTP_ERROR_INVALID_PARAMETER;
	}

	sttengine_s* engine = NULL;
	engine = __get_engine(engine_id);
	if (NULL != engine) {
		SECURE_SLOG(LOG_WARN, stt_tag(), "[Engine WARNING] engine id(%d) is already loaded", engine_id);
		return 0;
	}

	SECURE_SLOG(LOG_DEBUG, stt_tag(), "[Engine] Load engine id(%d), path(%s)", engine_id, filepath);

	/* allocation memory */
	engine = (sttengine_s*)calloc(1, sizeof(sttengine_s));
	if (NULL == engine) {
		SLOG(LOG_ERROR, stt_tag(), "[ERROR] Fail to allocate memory");
		return STTP_ERROR_OUT_OF_MEMORY;
	}

	/* load engine */
	char *error;

	engine->handle = dlopen(filepath, RTLD_LAZY);
	if (!engine->handle) {
		SECURE_SLOG(LOG_WARN, stt_tag(), "[Engine] Invalid engine (Fail dlopen) : %s", filepath);
		free(engine);
		return STTP_ERROR_OPERATION_FAILED;
	}

	engine->pefuncs = (sttpe_funcs_s*)calloc(1, sizeof(sttpe_funcs_s));
	if (NULL == engine->pefuncs) {
		SLOG(LOG_ERROR, stt_tag(), "[ERROR] Fail to allocate memory");
		dlclose(engine->handle);
		free(engine);
		return STTP_ERROR_OUT_OF_MEMORY;
	}
	engine->pdfuncs = (sttpd_funcs_s*)calloc(1, sizeof(sttpd_funcs_s));
	if (NULL == engine->pdfuncs) {
		SLOG(LOG_ERROR, stt_tag(), "[ERROR] Fail to allocate memory");
		dlclose(engine->handle);
		free(engine->pefuncs);
		free(engine);
		return STTP_ERROR_OUT_OF_MEMORY;
	}

	engine->sttp_unload_engine = NULL;
	engine->sttp_load_engine = NULL;

	engine->sttp_unload_engine = (int (*)())dlsym(engine->handle, "sttp_unload_engine");
	if (NULL != (error = dlerror()) || NULL == engine->sttp_unload_engine) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Fail to link daemon to sttp_unload_engine() : %s", error);
		dlclose(engine->handle);
		free(engine->pefuncs);
		free(engine->pdfuncs);
		free(engine);
		return STTP_ERROR_OPERATION_FAILED;
	}

	engine->sttp_load_engine = (int (*)(sttpd_funcs_s*, sttpe_funcs_s*) )dlsym(engine->handle, "sttp_load_engine");
	if (NULL != (error = dlerror()) || NULL == engine->sttp_load_engine) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Fail to link daemon to sttp_load_engine() : %s", error);
		dlclose(engine->handle);
		free(engine->pefuncs);
		free(engine->pdfuncs);
		free(engine);
		return STTP_ERROR_OPERATION_FAILED;
	}

	engine->engine_id = engine_id;
	engine->engine_path = strdup(filepath);

	engine->pdfuncs->version = 1;
	engine->pdfuncs->size = sizeof(sttpd_funcs_s);
	
	int ret = engine->sttp_load_engine(engine->pdfuncs, engine->pefuncs); 
	if (0 != ret) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Fail sttp_load_engine() : %s", __stt_get_engine_error_code(ret));
		dlclose(engine->handle);
		free(engine->pefuncs);
		free(engine->pdfuncs);
		free(engine->engine_path);
		free(engine);
		return STTP_ERROR_OPERATION_FAILED;
	}

	/* engine error check */
	if (engine->pefuncs->size != sizeof(sttpe_funcs_s)) {
		SLOG(LOG_WARN, stt_tag(), "[Engine WARNING] engine is not valid : function size is not matched");
	}

	if (NULL == engine->pefuncs->initialize ||
		NULL == engine->pefuncs->deinitialize ||
		NULL == engine->pefuncs->foreach_langs ||
		NULL == engine->pefuncs->is_valid_lang ||
		NULL == engine->pefuncs->support_silence ||
		NULL == engine->pefuncs->support_recognition_type ||
		NULL == engine->pefuncs->get_audio_format ||
		NULL == engine->pefuncs->set_silence_detection ||
		NULL == engine->pefuncs->start ||
		NULL == engine->pefuncs->set_recording ||
		NULL == engine->pefuncs->stop ||
		NULL == engine->pefuncs->cancel ||
		NULL == engine->pefuncs->foreach_result_time)
		/* Current unused functions
		NULL == engine->pefuncs->start_file_recognition ||
		*/
	{
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] The engine functions are NOT valid");
		dlclose(engine->handle);
		free(engine->pefuncs);
		free(engine->pdfuncs);
		free(engine->engine_path);
		free(engine);

		return STTP_ERROR_OPERATION_FAILED; 
	}

	SECURE_SLOG(LOG_DEBUG, stt_tag(), "[Engine Success] Load engine : version(%d), size(%d)",engine->pefuncs->version, engine->pefuncs->size);

	g_engine_list = g_slist_append(g_engine_list, engine);

	return 0;
}

/* Unregister engine id */
int stt_engine_unload(int engine_id)
{
	sttengine_s* engine = NULL;
	engine = __get_engine(engine_id);
	if (NULL == engine) {
		SECURE_SLOG(LOG_WARN, stt_tag(), "[Engine WARNING] engine id(%d) is invalid", engine_id);
		return STTP_ERROR_INVALID_PARAMETER;
	}

	/* unload engine */
	engine->sttp_unload_engine();
	dlclose(engine->handle);
	
	if (NULL != engine->engine_path)	free(engine->engine_path);
	if (NULL != engine->pefuncs)		free(engine->pefuncs);
	if (NULL != engine->pdfuncs)		free(engine->pdfuncs);

	g_engine_list = g_slist_remove(g_engine_list, engine);

	free(engine);
	
	return 0;
}


/* Initialize / Deinitialize */
int stt_engine_initialize(int engine_id, sttpe_result_cb result_cb, sttpe_silence_detected_cb silence_cb)
{
	if (NULL == result_cb || NULL == silence_cb) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid Parameter"); 
		return STTP_ERROR_INVALID_PARAMETER;
	}

	sttengine_s* engine = NULL;
	engine = __get_engine(engine_id);
	if (NULL == engine) {
		SECURE_SLOG(LOG_WARN, stt_tag(), "[Engine WARNING] engine id(%d) is invalid", engine_id);
		return STTP_ERROR_INVALID_PARAMETER;
	}

	int ret;
	ret = engine->pefuncs->initialize(result_cb, silence_cb);
	if (0 != ret) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Fail to initialize : %s", __stt_get_engine_error_code(ret));
		return STTP_ERROR_OPERATION_FAILED;
	}

	return 0;
}

int stt_engine_deinitialize(int engine_id)
{
	sttengine_s* engine = NULL;
	engine = __get_engine(engine_id);
	if (NULL == engine) {
		SECURE_SLOG(LOG_WARN, stt_tag(), "[Engine WARNING] engine id(%d) is invalid", engine_id);
		return STTP_ERROR_INVALID_PARAMETER;
	}

	int ret;
	ret = engine->pefuncs->deinitialize();
	if (0 != ret) {
		SLOG(LOG_WARN, stt_tag(), "[Engine WARNING] Fail to deinitialize : %s", __stt_get_engine_error_code(ret));
	}

	return 0;
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
int stt_engine_get_supported_langs(int engine_id, GSList** lang_list)
{
	if (NULL == lang_list) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid Parameter"); 
		return STTP_ERROR_INVALID_PARAMETER;
	}

	sttengine_s* engine = NULL;
	engine = __get_engine(engine_id);
	if (NULL == engine) {
		SECURE_SLOG(LOG_WARN, stt_tag(), "[Engine WARNING] engine id(%d) is invalid", engine_id);
		return STTP_ERROR_INVALID_PARAMETER;
	}

	int ret;
	ret = engine->pefuncs->foreach_langs(__supported_language_cb, (void*)lang_list);
	if (0 != ret) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] get language list error : %s", __stt_get_engine_error_code(ret));
		return STTP_ERROR_OPERATION_FAILED;
	}

	return 0;
}

int stt_engine_is_valid_language(int engine_id, const char* language, bool *is_valid)
{
	if (NULL == language || NULL == is_valid) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid Parameter"); 
		return STTP_ERROR_INVALID_PARAMETER;
	}

	sttengine_s* engine = NULL;
	engine = __get_engine(engine_id);
	if (NULL == engine) {
		SECURE_SLOG(LOG_WARN, stt_tag(), "[Engine WARNING] engine id(%d) is invalid", engine_id);
		return STTP_ERROR_INVALID_PARAMETER;
	}
	
	bool result;
	result = engine->pefuncs->is_valid_lang(language);

	*is_valid = result;

	return 0;
}

int stt_engine_get_first_language(int engine_id, char** language)
{
	if (NULL == language) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid Parameter"); 
		return STTP_ERROR_INVALID_PARAMETER;
	}

	sttengine_s* engine = NULL;
	engine = __get_engine(engine_id);
	if (NULL == engine) {
		SECURE_SLOG(LOG_WARN, stt_tag(), "[Engine WARNING] engine id(%d) is invalid", engine_id);
		return STTP_ERROR_INVALID_PARAMETER;
	}

	GSList* lang_list = NULL;
	int ret;
	ret = engine->pefuncs->foreach_langs(__supported_language_cb, &lang_list);
	if (0 != ret) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] get language list error : %s", __stt_get_engine_error_code(ret));
		return STTP_ERROR_OPERATION_FAILED;
	}

	GSList *iter = NULL;
	char* data = NULL;

	iter = g_slist_nth(lang_list, 0);
	if (NULL != iter) {
		data = iter->data;

		if (true == engine->pefuncs->is_valid_lang(data)) {
			*language = strdup(data);
		} else {
			ret = STTP_ERROR_OPERATION_FAILED;
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

int stt_engine_support_silence(int engine_id, bool* support)
{
	if (NULL == support) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid Parameter"); 
		return STTP_ERROR_INVALID_PARAMETER;
	}

	sttengine_s* engine = NULL;
	engine = __get_engine(engine_id);
	if (NULL == engine) {
		SECURE_SLOG(LOG_WARN, stt_tag(), "[Engine WARNING] engine id(%d) is invalid", engine_id);
		return STTP_ERROR_INVALID_PARAMETER;
	}

	bool result;
	result = engine->pefuncs->support_silence();
	*support = result;

	return 0;
}

int stt_engine_support_recognition_type(int engine_id, const char* type, bool* support)
{
	if (NULL == type || NULL == support) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid Parameter"); 
		return STTP_ERROR_INVALID_PARAMETER;
	}

	sttengine_s* engine = NULL;
	engine = __get_engine(engine_id);
	if (NULL == engine) {
		SECURE_SLOG(LOG_WARN, stt_tag(), "[Engine WARNING] engine id(%d) is invalid", engine_id);
		return STTP_ERROR_INVALID_PARAMETER;
	}

	bool result;
	if (NULL != engine->pefuncs->support_recognition_type) {
		result = engine->pefuncs->support_recognition_type(type);
		*support = result;
		return 0;
	}

	return STTP_ERROR_OPERATION_FAILED;
}

int stt_engine_get_audio_type(int engine_id, sttp_audio_type_e* types, int* rate, int* channels)
{
	if (NULL == types || NULL == rate || NULL == channels) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid Parameter"); 
		return STTP_ERROR_INVALID_PARAMETER;
	}

	sttengine_s* engine = NULL;
	engine = __get_engine(engine_id);
	if (NULL == engine) {
		SECURE_SLOG(LOG_WARN, stt_tag(), "[Engine WARNING] engine id(%d) is invalid", engine_id);
		return STTP_ERROR_INVALID_PARAMETER;
	}

	int ret;
	ret = engine->pefuncs->get_audio_format(types, rate, channels);
	if (0 != ret) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Fail to get audio format : %s", __stt_get_engine_error_code(ret));
		return STTP_ERROR_OPERATION_FAILED;
	}

	return 0;
}

/* Set option */
int stt_engine_set_silence_detection(int engine_id, bool value)
{
	sttengine_s* engine = NULL;
	engine = __get_engine(engine_id);
	if (NULL == engine) {
		SECURE_SLOG(LOG_WARN, stt_tag(), "[Engine WARNING] engine id(%d) is invalid", engine_id);
		return STTP_ERROR_INVALID_PARAMETER;
	}

	int ret = engine->pefuncs->set_silence_detection(value);
	if (STTP_ERROR_NOT_SUPPORTED_FEATURE == ret) {
		SLOG(LOG_WARN, stt_tag(), "[Engine WARNING] Not support silence detection"); 
		return STTP_ERROR_NOT_SUPPORTED_FEATURE;
	} else if (0 != ret) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Fail to set silence detection : %d", ret); 
		return STTP_ERROR_OPERATION_FAILED;
	}
	
	return 0;
}

int stt_engine_check_app_agreed(int engine_id, const char* appid, bool* value)
{
	sttengine_s* engine = NULL;
	engine = __get_engine(engine_id);
	if (NULL == engine) {
		SECURE_SLOG(LOG_WARN, stt_tag(), "[Engine WARNING] engine id(%d) is invalid", engine_id);
		return STTP_ERROR_INVALID_PARAMETER;
	}

	if (NULL == engine->pefuncs->check_app_agreed) {
		SLOG(LOG_WARN, stt_tag(), "[Engine WARNING] Not support app agreement. All app is available");
		*value = true;
		return 0;
	}

	int ret = engine->pefuncs->check_app_agreed(appid, value);
	if (0 != ret) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Fail to get app agreement : %s", __stt_get_engine_error_code(ret));
		*value = false;
		return STTP_ERROR_OPERATION_FAILED;
	}
	
	return 0;
}

/* Recognition */
int stt_engine_recognize_start(int engine_id, const char* lang, const char* recognition_type, void* user_param)
{
	if (NULL == lang || NULL == recognition_type) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid Parameter"); 
		return STTP_ERROR_INVALID_PARAMETER;
	}

	sttengine_s* engine = NULL;
	engine = __get_engine(engine_id);
	if (NULL == engine) {
		SECURE_SLOG(LOG_WARN, stt_tag(), "[Engine WARNING] engine id(%d) is invalid", engine_id);
		return STTP_ERROR_INVALID_PARAMETER;
	}

	int ret = engine->pefuncs->start(lang, recognition_type, user_param);
	if (0 != ret) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Fail to start recognition : %s", __stt_get_engine_error_code(ret));
		return STTP_ERROR_OPERATION_FAILED;
	}

	return 0;
}

int stt_engine_set_recording_data(int engine_id, const void* data, unsigned int length)
{
	if (NULL == data) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid Parameter"); 
		return STTP_ERROR_INVALID_PARAMETER;
	}

	sttengine_s* engine = NULL;
	engine = __get_engine(engine_id);
	if (NULL == engine) {
		SECURE_SLOG(LOG_WARN, stt_tag(), "[Engine WARNING] engine id(%d) is invalid", engine_id);
		return STTP_ERROR_INVALID_PARAMETER;
	}

	int ret = engine->pefuncs->set_recording(data, length);
	if (0 != ret) {
		SLOG(LOG_WARN, stt_tag(), "[Engine WARNING] Fail to set recording : %s", __stt_get_engine_error_code(ret));
		return STTP_ERROR_OPERATION_FAILED;
	}

	return 0;
}

int stt_engine_recognize_stop(int engine_id)
{
	sttengine_s* engine = NULL;
	engine = __get_engine(engine_id);
	if (NULL == engine) {
		SECURE_SLOG(LOG_WARN, stt_tag(), "[Engine WARNING] engine id(%d) is invalid", engine_id);
		return STTP_ERROR_INVALID_PARAMETER;
	}

	int ret = engine->pefuncs->stop();
	if (0 != ret) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Fail to stop : %s", __stt_get_engine_error_code(ret));
		return STTP_ERROR_OPERATION_FAILED;
	}

	return 0;
}

int stt_engine_recognize_cancel(int engine_id)
{
	sttengine_s* engine = NULL;
	engine = __get_engine(engine_id);
	if (NULL == engine) {
		SECURE_SLOG(LOG_WARN, stt_tag(), "[Engine WARNING] engine id(%d) is invalid", engine_id);
		return STTP_ERROR_INVALID_PARAMETER;
	}

	int ret = engine->pefuncs->cancel();
	if (0 != ret) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Fail to cancel : %s", __stt_get_engine_error_code(ret));
		return STTP_ERROR_OPERATION_FAILED;
	}

	return 0;
}

int stt_engine_foreach_result_time(int engine_id, void* time_info, sttpe_result_time_cb callback, void* user_data)
{
	sttengine_s* engine = NULL;
	engine = __get_engine(engine_id);
	if (NULL == engine) {
		SECURE_SLOG(LOG_WARN, stt_tag(), "[Engine WARNING] engine id(%d) is invalid", engine_id);
		return STTP_ERROR_INVALID_PARAMETER;
	}

	int ret = engine->pefuncs->foreach_result_time(time_info, callback, user_data);
	if (0 != ret) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Fail to foreach result time : %s", __stt_get_engine_error_code(ret));
		return STTP_ERROR_OPERATION_FAILED;
	}
	
	return 0;
}

int stt_engine_recognize_start_file(int engine_id, const char* lang, const char* recognition_type, 
				     const char* filepath, sttp_audio_type_e audio_type, int sample_rate, void* user_param)
{
	if (NULL == filepath || NULL == lang || NULL == recognition_type) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Invalid Parameter"); 
		return STTP_ERROR_INVALID_PARAMETER;
	}

	sttengine_s* engine = NULL;
	engine = __get_engine(engine_id);
	if (NULL == engine) {
		SECURE_SLOG(LOG_WARN, stt_tag(), "[Engine WARNING] engine id(%d) is invalid", engine_id);
		return STTP_ERROR_INVALID_PARAMETER;
	}

	if (NULL == engine->pefuncs->start_file) {
		SLOG(LOG_WARN, stt_tag(), "[Engine WARNING] engine API is invalid");
		return STTP_ERROR_NOT_SUPPORTED_FEATURE;
	}

	int ret = engine->pefuncs->start_file(lang, recognition_type, filepath, audio_type, sample_rate, user_param);
	if (0 != ret) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Fail to start file recognition : %s", __stt_get_engine_error_code(ret));
		return STTP_ERROR_OPERATION_FAILED;
	}

	return 0;
}

int stt_engine_recognize_cancel_file(int engine_id)
{
	sttengine_s* engine = NULL;
	engine = __get_engine(engine_id);
	if (NULL == engine) {
		SECURE_SLOG(LOG_WARN, stt_tag(), "[Engine WARNING] engine id(%d) is invalid", engine_id);
		return STTP_ERROR_INVALID_PARAMETER;
	}

	if (NULL == engine->pefuncs->cancel_file) {
		SLOG(LOG_WARN, stt_tag(), "[Engine WARNING] engine API is invalid");
		return STTP_ERROR_NOT_SUPPORTED_FEATURE;
	}

	int ret = engine->pefuncs->cancel_file();
	if (0 != ret) {
		SLOG(LOG_ERROR, stt_tag(), "[Engine ERROR] Fail to start file recognition : %s", __stt_get_engine_error_code(ret));
		return STTP_ERROR_OPERATION_FAILED;
	}

	return 0;
}