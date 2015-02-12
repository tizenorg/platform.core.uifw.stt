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



#ifndef __STT_ENGINE_H_
#define __STT_ENGINE_H_

#include <glib.h>
#include "sttp.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
* STT Engine Interfaces
*/


/* Register engine id */
int stt_engine_load(int engine_id, const char* filepath);

/* Unregister engine id */
int stt_engine_unload(int engine_id);


/* Init / Deinit */
int stt_engine_initialize(int engine_id, sttpe_result_cb result_cb, sttpe_silence_detected_cb silence_cb);

int stt_engine_deinitialize(int engine_id);

/* Get option */
int stt_engine_get_supported_langs(int engine_id, GSList** lang_list);

int stt_engine_is_valid_language(int engine_id, const char* language, bool *is_valid);

int stt_engine_get_first_language(int engine_id, char** language);

int stt_engine_support_silence(int engine_id, bool* support);

int stt_engine_support_recognition_type(int engine_id, const char* type, bool* support);

int stt_engine_get_audio_type(int engine_id, sttp_audio_type_e* types, int* rate, int* channels);

/* Set option */
int stt_engine_set_silence_detection(int engine_id, bool value);

/* Get right */
int stt_engine_check_app_agreed(int engine_id, const char* appid, bool* value);

/* Recognition */
int stt_engine_recognize_start(int engine_id, const char* lang, const char* recognition_type, void* user_param);

int stt_engine_set_recording_data(int engine_id, const void* data, unsigned int length);

int stt_engine_recognize_stop(int engine_id);

int stt_engine_recognize_cancel(int engine_id);

int stt_engine_foreach_result_time(int engine_id, void* time_info, sttpe_result_time_cb callback, void* user_data);


/* File recognition */
int stt_engine_recognize_start_file(int engine_id, const char* lang, const char* recognition_type, 
				const char* filepath, sttp_audio_type_e audio_type, int sample_rate, void* user_param);

int stt_engine_recognize_cancel_file(int engine_id);


#ifdef __cplusplus
}
#endif

#endif /* __STT_ENGINE_H_ */
