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


#ifndef __STT_CLIENT_H_
#define __STT_CLIENT_H_

#include <pthread.h>
#include "stt.h"
#include "stt_main.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	STT_INTERNAL_STATE_NONE		= 0,
	STT_INTERNAL_STATE_STARTING	= 1,
	STT_INTERNAL_STATE_STOPING	= 2
}stt_internal_state_e;

typedef struct {
	/* base info */
	stt_h	stt;
	int	pid; 
	int	uid;	/*<< unique id = pid + handle */

	stt_recognition_result_cb	recognition_result_cb;
	void*				recognition_result_user_data;
	stt_result_time_cb		result_time_cb;
	void*				result_time_user_data;

	stt_state_changed_cb		state_changed_cb;
	void*				state_changed_user_data;
	stt_error_cb			error_cb;
	void*				error_user_data;
	stt_default_language_changed_cb	default_lang_changed_cb;
	void*				default_lang_changed_user_data;

	stt_supported_engine_cb		supported_engine_cb;
	void*				supported_engine_user_data;
	stt_supported_language_cb	supported_lang_cb;
	void*				supported_lang_user_data;

	char*		current_engine_id;

	/* option */
	bool		silence_supported;
	stt_option_silence_detection_e	silence;

	/* state */
	stt_state_e	before_state;
	stt_state_e	current_state;

	stt_internal_state_e	internal_state;

	/* mutex */
	int		cb_ref_count;

	/* result data */
	int	event;
	char**	data_list;
	int	data_count;
	char*	msg;
	
	/* error data */
	int	reason;
}stt_client_s;


typedef bool (*stt_time_cb)(int index, int event, const char* text, long start_time, long end_time, void *user_data);

int stt_client_new(stt_h* stt);

int stt_client_destroy(stt_h stt);

stt_client_s* stt_client_get(stt_h stt);

stt_client_s* stt_client_get_by_uid(const int uid);

int stt_client_get_size();

int stt_client_use_callback(stt_client_s* client);

int stt_client_not_use_callback(stt_client_s* client);

int stt_client_get_use_callback(stt_client_s* client);

GList* stt_client_get_client_list();

#ifdef __cplusplus
}
#endif

#endif /* __STT_CLIENT_H_ */
