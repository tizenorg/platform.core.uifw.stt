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


#ifndef __STT_FILE_CLIENT_H_
#define __STT_FILE_CLIENT_H_


#include "stt_file.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TAG_STTFC "sttfile"

typedef struct {
	/* callback functions */
	stt_file_recognition_result_cb	recognition_result_cb;
	void*				recognition_result_user_data;

	stt_file_result_time_cb		result_time_cb;
	void*				result_time_user_data;

	stt_file_state_changed_cb	state_changed_cb;
	void*				state_changed_user_data;

	stt_file_supported_language_cb	supported_lang_cb;
	void*				supported_lang_user_data;

	/* current engine */
	int	current_engine_id;

	/* state */
	stt_file_state_e	before_state;
	stt_file_state_e	current_state;

	/* mutex */
	int	cb_ref_count;

	/* result data */
	void*	time_info;

	/* error data */
	int	reason;
}stt_file_client_s;


int stt_file_client_new();

int stt_file_client_destroy();

stt_file_client_s* stt_file_client_get();

int stt_file_client_use_callback(stt_file_client_s* client);

int stt_file_client_not_use_callback(stt_file_client_s* client);

int stt_file_client_get_use_callback(stt_file_client_s* client);



#ifdef __cplusplus
}
#endif

#endif /* __STT_FILE_CLIENT_H_ */
