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



#ifndef __STT_ENGINE_H_
#define __STT_ENGINE_H_

#include <glib.h>
#include "stte.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef void (*stt_engine_result_cb)(stte_result_event_e event, const char* type, const char** data, int data_count,
				const char* msg, void* time_info, void* user_data);

/*
* STT Engine Interfaces
*/


/* Register engine id */
int stt_engine_load(const char* filepath, stte_request_callback_s *callback);

/* Unregister engine id */
int stt_engine_unload();


/* Init / Deinit */
int stt_engine_initialize(bool is_from_lib);

int stt_engine_deinitialize();

/* Get option */
int stt_engine_get_supported_langs(GSList** lang_list);

int stt_engine_is_valid_language(const char* language, bool *is_valid);

int stt_engine_set_private_data(const char* key, const char* data);

int stt_engine_get_private_data(const char* key, char** data);

int stt_engine_get_first_language(char** language);

int stt_engine_support_silence(bool* support);

int stt_engine_need_app_credential(bool* need);

int stt_engine_support_recognition_type(const char* type, bool* support);

int stt_engine_get_audio_type(stte_audio_type_e* types, int* rate, int* channels);

/* Set option */
int stt_engine_set_silence_detection(bool value);

/* Get right */
int stt_engine_check_app_agreed(const char* appid, bool* is_agreed);

/* Recognition */
int stt_engine_recognize_start(const char* lang, const char* recognition_type, const char* appid, const char* credential, void* user_param);

int stt_engine_set_recording_data(const void* data, unsigned int length);

int stt_engine_recognize_stop();

int stt_engine_recognize_cancel();

int stt_engine_foreach_result_time(void* time_info, stte_result_time_cb callback, void* user_data);


/* File recognition */
int stt_engine_recognize_start_file(const char* lang, const char* recognition_type, 
				const char* filepath, stte_audio_type_e audio_type, int sample_rate, void* user_param);

int stt_engine_recognize_cancel_file();

int stt_engine_set_recognition_result_cb(stt_engine_result_cb result_cb, void* user_data);

int stt_engine_send_result(stte_result_event_e event, const char* type, const char** result, int result_count,
				const char* msg, void* time_info, void* user_data);

int stt_engine_send_error(stte_error_e error, const char* msg);

int stt_engine_send_speech_status(stte_speech_status_e status, void* user_data);

int stt_engine_set_private_data_set_cb(stte_private_data_set_cb private_data_set_cb, void* user_data);

int stt_engine_set_private_data_requested_cb(stte_private_data_requested_cb private_data_requested_cb, void* user_data);


#ifdef __cplusplus
}
#endif

#endif /* __STT_ENGINE_H_ */
