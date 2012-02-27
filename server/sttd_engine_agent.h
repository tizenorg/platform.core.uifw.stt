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



#ifndef __STTD_ENGINE_AGENT_H_
#define __STTD_ENGINE_AGENT_H_

#include "sttd_main.h"
#include "sttp.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
* Constants & Structures	
*/

#define	ENGINE_PATH_SIZE 256

typedef void (*result_callback)(sttp_result_event_e event, const char* type, 
				const char** data, int data_count, const char* msg, void *user_data);

typedef void (*partial_result_callback)(sttp_result_event_e event, const char* data, void *user_data);

typedef void (*silence_dectection_callback)(void *user_data);



/*
* STT Engine Agent Interfaces
*/

/** Init engine agent */
int sttd_engine_agent_init(result_callback result_cb, partial_result_callback partial_result_cb, silence_dectection_callback silence_cb);

/** Release engine agent */
int sttd_engine_agent_release();

/** Set current engine */
int sttd_engine_agent_initialize_current_engine();

/** load current engine */
int sttd_engine_agent_load_current_engine();

/** Unload current engine */
int sttd_engine_agent_unload_current_engine();

/** test for language list */
int sttd_print_enginelist();

/** Get state of current engine to need network */
bool sttd_engine_agent_need_network();

/*
* STT Engine Interfaces for client
*/

int sttd_engine_supported_langs(GList** lang_list);

int sttd_engine_get_default_lang(char** lang);

int sttd_engine_recognize_start(const char* lang, const char* recognition_type, 
				int profanity, int punctuation, int silence, void* user_param);

int sttd_engine_recognize_audio(const void* data, unsigned int length);

int sttd_engine_is_partial_result_supported(bool* partial_result);

int sttd_engine_recognize_stop();

int sttd_engine_recognize_cancel();

int sttd_engine_get_audio_format(sttp_audio_type_e* types, int* rate, int* channels);


/*
* STT Engine Interfaces for setting
*/

int sttd_engine_setting_get_engine_list(GList** engine_list);

int sttd_engine_setting_get_engine(char** engine_id);

int sttd_engine_setting_set_engine(const char* engine_id);

int sttd_engine_setting_get_lang_list(char** engine_id, GList** lang_list);

int sttd_engine_setting_get_default_lang(char** language);

int sttd_engine_setting_set_default_lang(const char* language);

int sttd_engine_setting_get_profanity_filter(bool* value);

int sttd_engine_setting_set_profanity_filter(bool value);

int sttd_engine_setting_get_punctuation_override(bool* value);

int sttd_engine_setting_set_punctuation_override(bool value);

int sttd_engine_setting_get_silence_detection(bool* value);

int sttd_engine_setting_set_silence_detection(bool value);

int sttd_engine_setting_get_engine_setting_info(char** engine_id, GList** setting_list); 

int sttd_engine_setting_set_engine_setting(const char* key, const char* value);


#ifdef __cplusplus
}
#endif

#endif /* __STTD_ENGINE_AGENT_H_ */
