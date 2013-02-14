/*
*  Copyright (c) 2012, 2013 Samsung Electronics Co., Ltd All Rights Reserved 
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


typedef struct {
	/* base info */
	stt_h	stt;
	int	pid; 
	int	uid;	/*<< unique id = pid + handle */

	stt_result_cb		result_cb;
	void*			result_user_data;
	stt_partial_result_cb	partial_result_cb;
	void*			partial_result_user_data;
	stt_state_changed_cb	state_changed_cb;
	void*			state_changed_user_data;
	stt_error_cb		error_cb;
	void*			error_user_data;

	/* option */
	bool	silence_supported;
	bool	profanity_supported;
	bool	punctuation_supported;

	stt_option_profanity_e		profanity;	
	stt_option_punctuation_e	punctuation;
	stt_option_silence_detection_e	silence;

	/* state */
	stt_state_e	before_state;
	stt_state_e	current_state;

	/* mutex */
	int		cb_ref_count;

	/* result data */
	char*	partial_result;
	char*	type;
	char**	data_list;
	int	data_count;
	char*	msg;

	/* error data */
	int	reason;
}stt_client_s;

int stt_client_new(stt_h* stt);

int stt_client_destroy(stt_h stt);

stt_client_s* stt_client_get(stt_h stt);

stt_client_s* stt_client_get_by_uid(const int uid);

int stt_client_get_size();

int stt_client_use_callback(stt_client_s* client);

int stt_client_not_use_callback(stt_client_s* client);

int stt_client_get_use_callback(stt_client_s* client);

int stt_client_set_option_supported(stt_h stt, bool silence, bool profanity, bool punctuation);

#ifdef __cplusplus
}
#endif

#endif /* __STT_CLIENT_H_ */
