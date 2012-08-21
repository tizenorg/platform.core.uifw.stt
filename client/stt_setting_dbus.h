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

 
#ifndef __STT_SETTING_DBUS_H_
#define __STT_SETTING_DBUS_H_

#include "stt_setting.h"

#ifdef __cplusplus
extern "C" {
#endif

int stt_setting_dbus_open_connection();

int stt_setting_dbus_close_connection();


int stt_setting_dbus_request_hello();

int stt_setting_dbus_request_initialize();

int stt_setting_dbus_request_finalilze();

int stt_setting_dbus_request_get_engine_list(stt_setting_supported_engine_cb callback, void* user_data);

int stt_setting_dbus_request_get_engine(char** engine_id);

int stt_setting_dbus_request_set_engine(const char* engine_id );

int stt_setting_dbus_request_get_language_list(stt_setting_supported_language_cb callback, void* user_data);

int stt_setting_dbus_request_get_default_language(char** language);

int stt_setting_dbus_request_set_default_language(const char* language);

int stt_setting_dbus_request_get_profanity_filter(bool* value);

int stt_setting_dbus_request_set_profanity_filter(const bool value);

int stt_setting_dbus_request_get_punctuation_override(bool* value);

int stt_setting_dbus_request_set_punctuation_override(const bool value);

int stt_setting_dbus_request_get_silence_detection(bool* value);

int stt_setting_dbus_request_set_silence_detection(const bool value);

int stt_setting_dbus_request_get_engine_setting(stt_setting_engine_setting_cb callback, void* user_data);

int stt_setting_dbus_request_set_engine_setting(const char* key, const char* value);

#ifdef __cplusplus
}
#endif

#endif /* __STT_SETTING_DBUS_H_ */
