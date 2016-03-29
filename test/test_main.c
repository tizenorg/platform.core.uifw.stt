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

#include <stdio.h>
#include <Ecore.h>
#include <dlog.h>

#include <stt.h>
#include <stt_file.h>
#include <unistd.h>

#define TAG_STT_TEST "stt_test"

static stt_h g_stt = NULL;

static Eina_Bool __stt_test_finalize(void *data)
{
	int ret;
	SLOG(LOG_DEBUG, TAG_STT_TEST, "Unset callbacks");
	ret = stt_file_unset_recognition_result_cb();
	if (STT_FILE_ERROR_NONE != ret) {
		SLOG(LOG_ERROR, TAG_STT_TEST, "[ERROR] Fail to unset recognition cb");
	}

	SLOG(LOG_DEBUG, TAG_STT_TEST, "STT file deinitialize");
	ret = stt_file_deinitialize();
	if (STT_FILE_ERROR_NONE != ret) {
		SLOG(LOG_ERROR, TAG_STT_TEST, "[ERROR] Fail to deinitialize");
	}

	ecore_main_loop_quit();

	return EINA_FALSE;
}

static void __stt_file_state_changed_cb(stt_file_state_e previous, stt_file_state_e current, void* user_data)
{
}

static bool __stt_file_supported_language_cb(const char* language, void* user_data)
{
	if (NULL == language)
		return false;

	SLOG(LOG_DEBUG, TAG_STT_TEST, "=== %s ===", language);
	return true;
}

static bool __stt_file_result_time_cb(int index, stt_file_result_time_event_e event, const char* text,
				      long start_time, long end_time, void* user_data)
{
	if (NULL != text) {
		SLOG(LOG_DEBUG, TAG_STT_TEST, "[%d] %s - %ld to %ld", index, text, start_time, end_time);
		return true;
	}
	return false;
}

static void __stt_file_recognition_result_cb(stt_file_result_event_e event, const char** data, int data_count, const char* msg, void *user_data)
{
	SLOG(LOG_DEBUG, TAG_STT_TEST, "==== Result Callback ====");
	if (STT_FILE_RESULT_EVENT_ERROR == event) {
		SLOG(LOG_ERROR, TAG_STT_TEST, "[ERROR] Error Result");
		ecore_timer_add(0, __stt_test_finalize, NULL);
		return;
	}

	if (NULL != data) {
		SLOG(LOG_DEBUG, TAG_STT_TEST, "( %s )", data[0]);
	}


	int ret;
	ret = stt_file_foreach_detailed_result(__stt_file_result_time_cb, NULL);
	if (STT_FILE_ERROR_NONE != ret) {
		SLOG(LOG_ERROR, TAG_STT_TEST, "[ERROR] Fail to get result time");
	}

	if (STT_FILE_RESULT_EVENT_FINAL_RESULT == event) {
		ecore_timer_add(0, __stt_test_finalize, NULL);
	}

	return;
}

static bool __supported_engine_cb(const char* engine_id, const char* engine_name, void* user_data)
{
	SLOG(LOG_DEBUG, TAG_STT_TEST, "Engine id(%s) name(%s)", engine_id, engine_name);

	return true;
}

static Eina_Bool __get_engine_list(void *data)
{
	int ret;
	SLOG(LOG_DEBUG, TAG_STT_TEST, "STT get engine list");
	ret = stt_file_foreach_supported_engines(__supported_engine_cb, NULL);
	if (STT_FILE_ERROR_NONE != ret) {
		SLOG(LOG_ERROR, TAG_STT_TEST, "Fail to get engine list");
	}

	char *cur_engine;
	SLOG(LOG_DEBUG, TAG_STT_TEST, "STT get engine");
	ret = stt_file_get_engine(&cur_engine);
	if (STT_FILE_ERROR_NONE != ret) {
		SLOG(LOG_ERROR, TAG_STT_TEST, "[ERROR] Fail to get engine");
		return EINA_FALSE;
	}

	if (NULL != cur_engine) {
		SLOG(LOG_DEBUG, TAG_STT_TEST, "Current engine - %s", cur_engine);
		free(cur_engine);
	}

	return EINA_FALSE;
}

static Eina_Bool __file_start(void *data)
{
	char *tmp = (char *)data;
	int ret;
	SLOG(LOG_DEBUG, TAG_STT_TEST, "STT start file recognition");
	ret = stt_file_start("en_US", "stt.recognition.type.FREE", tmp, STT_FILE_AUDIO_TYPE_RAW_S16, 16000);
	if (STT_FILE_ERROR_NONE != ret) {
		SLOG(LOG_ERROR, TAG_STT_TEST, "[ERROR] Fail to start");
	}

	return EINA_FALSE;
}

static Eina_Bool __stt_get_volume(void *data)
{
	float volume = 0.0;
	int ret = stt_get_recording_volume(g_stt, &volume);
	if (STT_ERROR_NONE != ret) {
		SLOG(LOG_ERROR, TAG_STT_TEST, "[ERROR] Fail to get volume");
		return EINA_FALSE;
	}

	SLOG(LOG_DEBUG, TAG_STT_TEST, "Get volume : %f", volume);

	return EINA_TRUE;
}

static Eina_Bool __stt_start(void *data)
{
	int ret;
	SLOG(LOG_DEBUG, TAG_STT_TEST, "STT set silence detection");
	ret = stt_set_silence_detection(g_stt, STT_OPTION_SILENCE_DETECTION_TRUE);
	if (STT_ERROR_NONE != ret) {
		SLOG(LOG_ERROR, TAG_STT_TEST, "[ERROR] Fail to set silence detection");
	}

	SLOG(LOG_DEBUG, TAG_STT_TEST, "STT start");
	ret = stt_start(g_stt, "en_US", "stt.recognition.type.FREE");
	if (STT_ERROR_NONE != ret) {
		SLOG(LOG_ERROR, TAG_STT_TEST, "[ERROR] Fail to start");
	}

	ecore_timer_add(0.1, __stt_get_volume, NULL);

	return EINA_FALSE;
}

static void __stt_state_changed_cb(stt_h stt, stt_state_e previous, stt_state_e current, void* user_data)
{
	if (STT_STATE_CREATED == previous && STT_STATE_READY == current) {
		SLOG(LOG_DEBUG, TAG_STT_TEST, "State is Created -> Ready");
		ecore_timer_add(0, __stt_start, NULL);
	}
}

static Eina_Bool __stt_finalize(void *data)
{
	int ret;
	SLOG(LOG_DEBUG, TAG_STT_TEST, "STT destroy");
	ret = stt_destroy(g_stt);
	if (STT_ERROR_NONE != ret) {
		SLOG(LOG_ERROR, TAG_STT_TEST, "[ERROR] Fail to stt destroy");
	}

	ecore_main_loop_quit();
	return EINA_FALSE;
}

static void __stt_recognition_result_cb(stt_h stt, stt_result_event_e event, const char** data, int data_count, const char* msg, void *user_data)
{
	SLOG(LOG_DEBUG, TAG_STT_TEST, "==== STT result cb ====");

	if (NULL != data) {
		SLOG(LOG_DEBUG, TAG_STT_TEST, "( %s )", data[0]);
	}

	ecore_timer_add(0, __stt_finalize, NULL);
}

int main (int argc, char *argv[])
{
	if (2 > argc) {
		SLOG(LOG_DEBUG, TAG_STT_TEST, "Please check parameter");
		SLOG(LOG_DEBUG, TAG_STT_TEST, "Ex> stt-test -f <file path>");
		SLOG(LOG_DEBUG, TAG_STT_TEST, "Ex> stt-test -m");
		return 0;
	}

	if (0 != strcmp("-f", argv[1]) && 0 != strcmp("-m", argv[1])) {
		SLOG(LOG_DEBUG, TAG_STT_TEST, "Please check parameter");
		SLOG(LOG_DEBUG, TAG_STT_TEST, "Ex> stt-test -f <file path>");
		SLOG(LOG_DEBUG, TAG_STT_TEST, "Ex> stt-test -m");
		return 0;
	}

	if (!strcmp("-f", argv[1])) {
		if (0 != access(argv[2], F_OK)) {
			SLOG(LOG_DEBUG, TAG_STT_TEST, "Please check filepath");
			return 0;
		}

		SLOG(LOG_DEBUG, TAG_STT_TEST, "===== STT File (%s) Sample start =====", argv[2]);

		if (!ecore_init()) {
			SLOG(LOG_ERROR, TAG_STT_TEST, "[Main ERROR] Fail ecore_init()");
			return 0;
		}

		int ret;
		SLOG(LOG_DEBUG, TAG_STT_TEST, "STT File initialize");
		ret = stt_file_initialize();
		if (STT_FILE_ERROR_NONE != ret) {
			SLOG(LOG_ERROR, TAG_STT_TEST, "Fail to initialize");
			return 0;
		}

		SLOG(LOG_DEBUG, TAG_STT_TEST, "STT Set Callbacks");
		ret = stt_file_set_recognition_result_cb(__stt_file_recognition_result_cb, NULL);
		if (STT_FILE_ERROR_NONE != ret) {
			SLOG(LOG_ERROR, TAG_STT_TEST, "Fail to set recognition result cb");
			return 0;
		}

		ret = stt_file_set_state_changed_cb(__stt_file_state_changed_cb, NULL);
		if (STT_FILE_ERROR_NONE != ret) {
			SLOG(LOG_ERROR, TAG_STT_TEST, "Fail to set state changed cb");
			return 0;
		}

		SLOG(LOG_DEBUG, TAG_STT_TEST, "Get supported langauge");
		ret = stt_file_foreach_supported_languages(__stt_file_supported_language_cb, NULL);
		if (STT_FILE_ERROR_NONE != ret) {
			SLOG(LOG_ERROR, TAG_STT_TEST, "Fail to get supported language");
			return 0;
		}

		ecore_timer_add(1, __get_engine_list, NULL);

		char *tmp = strdup(argv[2]);
		ecore_timer_add(1, __file_start, tmp);
	} else if (!strcmp("-m", argv[1])) {

		ecore_init();

		int ret;

		SLOG(LOG_DEBUG, TAG_STT_TEST, "STT Create");
		ret = stt_create(&g_stt);
		if (STT_ERROR_NONE != ret) {
			SLOG(LOG_ERROR, TAG_STT_TEST, "Fail to create");
			return 0;
		}

		SLOG(LOG_DEBUG, TAG_STT_TEST, "Set callback");
		ret = stt_set_state_changed_cb(g_stt, __stt_state_changed_cb, NULL);
		if (STT_ERROR_NONE != ret) {
			stt_destroy(g_stt);
			SLOG(LOG_ERROR, TAG_STT_TEST, "Fail to set state changed cb");
			return 0;
		}
		ret = stt_set_recognition_result_cb(g_stt, __stt_recognition_result_cb, NULL);
		if (STT_ERROR_NONE != ret) {
			stt_destroy(g_stt);
			SLOG(LOG_ERROR, TAG_STT_TEST, "Fail to set recognition result cb");
			return 0;
		}

		SLOG(LOG_DEBUG, TAG_STT_TEST, "STT prepare");
		ret = stt_prepare(g_stt);
		if (STT_ERROR_NONE != ret) {
			stt_destroy(g_stt);
			SLOG(LOG_ERROR, TAG_STT_TEST, "Fail to prepare");
			return 0;
		}
	}

	ecore_main_loop_begin();

	ecore_shutdown();

	SLOG(LOG_DEBUG, TAG_STT_TEST, "===== STT END =====");

	return 0;
}
