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

 
#ifndef __STT_CONFIG_MANAGER_H_
#define __STT_CONFIG_MANAGER_H_

#include <tizen.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	STT_CONFIG_ERROR_NONE			= TIZEN_ERROR_NONE,		/**< Successful */
	STT_CONFIG_ERROR_OUT_OF_MEMORY		= TIZEN_ERROR_OUT_OF_MEMORY,	/**< Out of Memory */
	STT_CONFIG_ERROR_IO_ERROR		= TIZEN_ERROR_IO_ERROR,		/**< I/O error */
	STT_CONFIG_ERROR_INVALID_PARAMETER	= TIZEN_ERROR_INVALID_PARAMETER,/**< Invalid parameter */
	STT_CONFIG_ERROR_PERMISSION_DENIED	= TIZEN_ERROR_PERMISSION_DENIED,/**< Permission denied */
	STT_CONFIG_ERROR_NOT_SUPPORTED		= TIZEN_ERROR_NOT_SUPPORTED,	/**< STT NOT supported */
	STT_CONFIG_ERROR_INVALID_STATE		= TIZEN_ERROR_STT | 0x01,	/**< Invalid state */
	STT_CONFIG_ERROR_INVALID_LANGUAGE	= TIZEN_ERROR_STT | 0x02,	/**< Invalid language */
	STT_CONFIG_ERROR_ENGINE_NOT_FOUND	= TIZEN_ERROR_STT | 0x03,	/**< No available engine  */	
	STT_CONFIG_ERROR_OPERATION_FAILED	= TIZEN_ERROR_STT | 0x04,	/**< Operation failed  */
}stt_config_error_e;

typedef enum {
	STT_CONFIG_TYPE_OPTION_SILENCE_DETECTION
}stt_config_type_e;

typedef bool (*stt_config_supported_engine_cb)(const char* engine_id, const char* engine_name, const char* setting, bool support_silence, void* user_data);

typedef bool (*stt_config_supported_langauge_cb)(const char* engine_id, const char* language, void* user_data);

typedef void (*stt_config_engine_changed_cb)(const char* engine_id, const char* setting, const char* language, bool support_silence, void* user_data);

typedef void (*stt_config_lang_changed_cb)(const char* before_language, const char* current_language, void* user_data);

typedef void (*stt_config_bool_changed_cb)(stt_config_type_e type, bool bool_value, void* user_data);

typedef bool (*stt_config_result_time_cb)(int index, int event, const char* text, long start_time, long end_time, void* user_data);


int stt_config_mgr_initialize(int uid);

int stt_config_mgr_finalize(int uid);


int stt_config_mgr_set_callback(int uid, stt_config_engine_changed_cb engine_cb, stt_config_lang_changed_cb lang_cb, stt_config_bool_changed_cb bool_cb, void* user_data);

int stt_config_mgr_unset_callback(int uid);


int stt_config_mgr_get_engine_list(stt_config_supported_engine_cb callback, void* user_data);

int stt_config_mgr_get_engine(char** engine);

int stt_config_mgr_set_engine(const char* engine);

int stt_config_mgr_get_engine_agreement(const char* engine, char** agreement);

int stt_config_mgr_get_language_list(const char* engine_id, stt_config_supported_langauge_cb callback, void* user_data);

int stt_config_mgr_get_default_language(char** language);

int stt_config_mgr_set_default_language(const char* language);

int stt_config_mgr_get_auto_language(bool* value);

int stt_config_mgr_set_auto_language(bool value);

int stt_config_mgr_get_silence_detection(bool* value);

int stt_config_mgr_set_silence_detection(bool value);

bool stt_config_check_default_engine_is_valid(const char* engine);

bool stt_config_check_default_language_is_valid(const char* language);


int stt_config_mgr_reset_time_info();

int stt_config_mgr_add_time_info(int index, int event, const char* text, long start_time, long end_time);

int stt_config_mgr_foreach_time_info(stt_config_result_time_cb callback, void* user_data);


int stt_config_mgr_save_time_info_file();

int stt_config_mgr_remove_time_info_file();


#ifdef __cplusplus
}
#endif

#endif /* __STT_CONFIG_MANAGER_H_ */
