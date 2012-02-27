/*
* Copyright (c) 2011 Samsung Electronics Co., Ltd All Rights Reserved 
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


#ifndef __STTD_SERVER_H_
#define __STTD_SERVER_H_

#include "sttd_main.h"

#ifdef __cplusplus
extern "C" {
#endif


/** Daemon initialize */
int sttd_initialize();

/*
* API for client
*/

int sttd_server_initialize(int pid, int uid);

int sttd_server_finalize(const int uid);

int sttd_server_get_supported_languages(const int uid, GList** lang_list);

int sttd_server_get_current_langauage(const int uid, char** current_lang);

int sttd_server_set_engine_data(int uid, const char* key, const char* value);

int sttd_server_is_partial_result_supported(int uid, int* partial_result);

int sttd_server_get_audio_volume(const int uid, float* current_volume);

int sttd_server_start(const int uid, const char* lang, const char* recognition_type, 
			int profanity, int punctuation, int silence);

int sttd_server_stop(const int uid);

int sttd_server_cancel(const int uid);

/*
* API for setting
*/

int sttd_server_setting_initialize(int uid);

int sttd_server_setting_finalize(int uid);

int sttd_server_setting_get_engine_list(int uid, GList** engine_list);

int sttd_server_setting_get_engine(int uid, char** engine_id);

int sttd_server_setting_set_engine(const int uid, const char* engine_id);

int sttd_server_setting_get_lang_list(int uid, char** engine_id, GList** lang_list);

int sttd_server_setting_get_default_language(int uid, char** language);

int sttd_server_setting_set_default_language(int uid, const char* language);

int sttd_server_setting_get_profanity_filter(int uid, bool* value);

int sttd_server_setting_set_profanity_filter(int uid, bool value);

int sttd_server_setting_get_punctuation_override(int uid, bool* value);

int sttd_server_setting_set_punctuation_override(int uid, bool value);

int sttd_server_setting_get_silence_detection(int uid, bool* value);

int sttd_server_setting_set_silence_detection(int uid, bool value);

int sttd_server_setting_get_engine_setting(int uid, char** engine_id, GList** lang_list);

int sttd_server_setting_set_engine_setting(int uid, const char* key, const char* value);


#ifdef __cplusplus
}
#endif

#endif /* __STTD_SERVER_H_ */
