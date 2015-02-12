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

typedef bool (*result_time_callback)(int index, sttp_result_time_event_e event, const char* text, 
				long start_time, long end_time, void *user_data);

typedef void (*silence_dectection_callback)(sttp_silence_type_e type, void *user_data);



/*
* STT Engine Agent Interfaces
*/

/** Init / Release engine agent */
int sttd_engine_agent_init(result_callback result_cb, result_time_callback time_cb, 
			   silence_dectection_callback silence_cb);

int sttd_engine_agent_release();


/** Manage engine */
int sttd_engine_agent_initialize_engine_list();


/** load / unload engine */
int sttd_engine_agent_load_current_engine(int uid, const char* engine_uuid);

int sttd_engine_agent_unload_current_engine(int uid);

bool sttd_engine_agent_is_default_engine();

int sttd_engine_agent_get_engine_list(GSList** engine_list);

int sttd_engine_agent_get_current_engine(int uid, char** engine_uuid);


/** Get/Set app option */
bool sttd_engine_agent_need_network(int uid);

int sttd_engine_agent_supported_langs(int uid, GSList** lang_list);

int sttd_engine_agent_get_default_lang(int uid, char** lang);

int sttd_engine_agent_get_option_supported(int uid, bool* silence);

int sttd_engine_agent_is_recognition_type_supported(int uid, const char* type, bool* support);

int sttd_engine_agent_set_default_engine(const char* engine_uuid);

int sttd_engine_agent_set_default_language(const char* language);

int sttd_engine_agent_set_silence_detection(bool value);

int sttd_engine_agent_check_app_agreed(int uid, const char* appid, bool* result);

/** Control engine */
int sttd_engine_agent_recognize_start_engine(int uid, const char* lang, const char* recognition_type, 
				int silence, void* user_param);

int sttd_engine_agent_recognize_start_recorder(int uid);

int sttd_engine_agent_set_recording_data(int uid, const void* data, unsigned int length);

int sttd_engine_agent_recognize_stop(int uid);

int sttd_engine_agent_recognize_stop_recorder(int uid);

int sttd_engine_agent_recognize_stop_engine(int uid);

int sttd_engine_agent_recognize_cancel(int uid);


#ifdef __cplusplus
}
#endif

#endif /* __STTD_ENGINE_AGENT_H_ */
