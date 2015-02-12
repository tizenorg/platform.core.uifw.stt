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


#ifndef __STTD_CONFIG_H_
#define __STTD_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*sttd_config_engine_changed_cb)(const char* engine_id, const char* language, bool support_silence, void* user_data);

typedef void (*sttd_config_language_changed_cb)(const char* language, void* user_data);

typedef void (*sttd_config_silence_changed_cb)(bool value, void* user_data);


int sttd_config_initialize(sttd_config_engine_changed_cb engine_cb, 
			sttd_config_language_changed_cb lang_cb, 
			sttd_config_silence_changed_cb silence_cb, 
			void* user_data);

int sttd_config_finalize();

int sttd_config_set_default_engine(const char* engine_id);

int sttd_config_get_default_engine(char** engine_id);

int sttd_config_get_default_language(char** language);

int sttd_config_get_default_silence_detection(int* silence);


int sttd_config_time_add(int index, int event, const char* text, long start_time, long end_time);

int sttd_config_time_save();

int sttd_config_time_reset();

#ifdef __cplusplus
}
#endif

#endif /* __STTD_CONFIG_H_ */
