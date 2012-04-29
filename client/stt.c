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


#include <sys/wait.h>
#include <sys/types.h> 
#include <unistd.h>

#include "stt.h"
#include "stt_main.h"
#include "stt_client.h"
#include "stt_dbus.h"

#define CONNECTION_RETRY_COUNT 2

static int __check_stt_daemon();

int stt_create(stt_h* stt)
{
	int ret = 0; 

	SLOG(LOG_DEBUG, TAG_STTC, "===== Create STT");

	if (NULL == stt) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] A handle is null");
		return STT_ERROR_INVALID_PARAMETER;
	}

	if (0 == stt_client_get_size()) {
		if (0 != stt_dbus_open_connection()) {
			SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to open connection\n ");
			return STT_ERROR_OPERATION_FAILED;
		}
	}

	/* Send hello */
	if (0 != stt_dbus_request_hello()) {
		__check_stt_daemon();
	}

	if (0 != stt_client_new(stt)) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to create client!!!!!");
		return STT_ERROR_OUT_OF_MEMORY;
	}

	/* request initialization */
	int i = 1;
	bool silence_supported = false;
	bool profanity_supported = false;
	bool punctuation_supported = false;

	while (1) {
		ret = stt_dbus_request_initialize((*stt)->handle, &silence_supported, &profanity_supported, &punctuation_supported);
		
		if (STT_ERROR_ENGINE_NOT_FOUND == ret) {
			stt_client_destroy(*stt);
			SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to initialize : STT Engine Not founded");
			return ret;
		} else if(0 != ret) {
			usleep(1);
			if(CONNECTION_RETRY_COUNT == i) {
				stt_client_destroy(*stt);
				SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to initialize : TIMED OUT");
				return STT_ERROR_TIMED_OUT;			    
			}    
			i++;
		} else {
			/* success to connect stt-daemon */
			stt_client_set_option_supported(*stt, silence_supported, profanity_supported, punctuation_supported);
			SLOG(LOG_DEBUG, TAG_STTC, "Supported options : silence(%s), profanity(%s), punctuation(%s)", 
				silence_supported ? "true" : "false", profanity_supported ? "true" : "false", punctuation_supported ? "true" : "false");
			break;
		}
	}

	SLOG(LOG_DEBUG, TAG_STTC, "[Success] uid(%d)", (*stt)->handle);
	
	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return STT_ERROR_NONE;
}

int stt_destroy(stt_h stt)
{
	SLOG(LOG_DEBUG, TAG_STTC, "===== Destroy STT");

	if (NULL == stt) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Input handle is null");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_ERROR_INVALID_PARAMETER;
	}
	
	stt_client_s* client = stt_client_get(stt);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] A handle is not available");
		return STT_ERROR_INVALID_PARAMETER;
	}
	
	int ret = stt_dbus_request_finalize(client->uid);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to request finalize");
	}
	
	/* Free resources */
	stt_client_destroy(stt);

	SLOG(LOG_DEBUG, TAG_STTC, "Success: destroy");

	if (0 == stt_client_get_size()) {
		if (0 != stt_dbus_close_connection()) {
			SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to close connection\n ");
		}
	}

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return STT_ERROR_NONE;
}

int stt_foreach_supported_languages(stt_h stt, stt_supported_language_cb callback, void* user_data)
{
	SLOG(LOG_DEBUG, TAG_STTC, "===== Foreach Supported Language");

	if (NULL == stt || NULL == callback) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Input parameter is NULL");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_ERROR_INVALID_PARAMETER;
	}

	stt_client_s* client = stt_client_get(stt);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_ERROR_INVALID_PARAMETER;
	}

	int ret = 0;
	ret = stt_dbus_request_get_support_langs(client->uid, client->stt, callback, user_data);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to get languages");
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS]");
	}

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return STT_ERROR_NONE;
}


int stt_get_default_language(stt_h stt, char** language)
{
	SLOG(LOG_DEBUG, TAG_STTC, "===== Get Default Language");

	if (NULL == stt || NULL == language) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Input parameter is NULL");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_ERROR_INVALID_PARAMETER;
	}

	stt_client_s* client = stt_client_get(stt);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_ERROR_INVALID_PARAMETER;
	}

	int ret = 0;
	ret = stt_dbus_request_get_default_lang(client->uid, language);

	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail : request get default language");
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Current language = %s", *language);
	}

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return ret;
}

int stt_get_state(stt_h stt, stt_state_e* state)
{
	if (NULL == stt || NULL == state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Input parameter is NULL");
		return STT_ERROR_INVALID_PARAMETER;
	}

	stt_client_s* client = stt_client_get(stt);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Get state : A handle is not valid");
		return STT_ERROR_INVALID_PARAMETER;
	}

	*state = client->current_state;

	switch(*state) {
		case STT_STATE_READY:		SLOG(LOG_DEBUG, TAG_STTC, "Current state is 'Ready'");		break;
		case STT_STATE_RECORDING:	SLOG(LOG_DEBUG, TAG_STTC, "Current state is 'Recording'");	break;
		case STT_STATE_PROCESSING:	SLOG(LOG_DEBUG, TAG_STTC, "Current state is 'Processing'");	break;
	}

	return STT_ERROR_NONE;
}

int stt_is_partial_result_supported(stt_h stt, bool* partial_result)
{
	if (NULL == stt || NULL == partial_result) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Input parameter is NULL");
		return STT_ERROR_INVALID_PARAMETER;
	}

	stt_client_s* client = stt_client_get(stt);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] A handle is not valid");
		return STT_ERROR_INVALID_PARAMETER;
	}

	int ret = 0;
	ret = stt_dbus_request_is_partial_result_supported(client->uid, partial_result);

	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to get partial result supported");
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Partial result supporting is %s", *partial_result ? "true " : "false");
	}

	return STT_ERROR_NONE;
}

int stt_set_profanity_filter(stt_h stt, stt_option_profanity_e type)
{
	if (NULL == stt) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Input parameter is NULL");
		return STT_ERROR_INVALID_PARAMETER;
	}

	stt_client_s* client = stt_client_get(stt);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Get state : A handle is not valid");
		return STT_ERROR_INVALID_PARAMETER;
	}

	if (true == client->profanity_supported) {
		if (type >= STT_OPTION_PROFANITY_FALSE && type <= STT_OPTION_PROFANITY_AUTO)
			client->profanity = type;	
		else {
			SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Type is invalid");
			return STT_ERROR_INVALID_PARAMETER;
		}
	} else {
		return STT_ERROR_NOT_SUPPORTED_FEATURE; 
	}

	return STT_ERROR_NONE;
}

int stt_set_punctuation_override(stt_h stt, stt_option_punctuation_e type)
{
	if (NULL == stt) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Input parameter is NULL");
		return STT_ERROR_INVALID_PARAMETER;
	}

	stt_client_s* client = stt_client_get(stt);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Get state : A handle is not valid");
		return STT_ERROR_INVALID_PARAMETER;
	}
	
	if (true == client->punctuation_supported) {
		if (type >= STT_OPTION_PUNCTUATION_FALSE && type <= STT_OPTION_PUNCTUATION_AUTO)
			client->punctuation = type;	
		else {
			SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Type is invalid");
			return STT_ERROR_INVALID_PARAMETER;
		}
	} else {
		return STT_ERROR_NOT_SUPPORTED_FEATURE; 
	}

	return STT_ERROR_NONE;
}

int stt_set_silence_detection(stt_h stt, stt_option_silence_detection_e type)
{
	if (NULL == stt) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Input parameter is NULL");
		return STT_ERROR_INVALID_PARAMETER;
	}

	stt_client_s* client = stt_client_get(stt);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Get state : A handle is not valid");
		return STT_ERROR_INVALID_PARAMETER;
	}

	if (true == client->silence_supported) {
		if (type >= STT_OPTION_SILENCE_DETECTION_FALSE && type <= STT_OPTION_SILENCE_DETECTION_AUTO)
			client->silence = type;	
		else {
			SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Type is invalid");
			return STT_ERROR_INVALID_PARAMETER;
		}
	} else {
		return STT_ERROR_NOT_SUPPORTED_FEATURE; 
	}

	return STT_ERROR_NONE;
}

int stt_start(stt_h stt, const char* language, const char* type)
{
	SLOG(LOG_DEBUG, TAG_STTC, "===== STT START");

	if (NULL == stt) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Input parameter is NULL");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_ERROR_INVALID_PARAMETER;
	}

	stt_client_s* client = stt_client_get(stt);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_ERROR_INVALID_PARAMETER;
	}

	/* check state */
	if (client->current_state != STT_STATE_READY) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid State: Current state is not READY"); 
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_ERROR_INVALID_STATE;
	}

	char* temp;
	if (NULL == language) {
		temp = strdup("default");
	} else {
		temp = strdup(language);
	}

	int ret; 
	/* do request */
	ret = stt_dbus_request_start(client->uid, temp, type, client->profanity, client->punctuation, client->silence);

	if (ret) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to start");
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS]");

		if (NULL != client->state_changed_cb) {
			client->state_changed_cb(client->stt, client->current_state, STT_STATE_RECORDING, client->state_changed_user_data); 
			SLOG(LOG_DEBUG, TAG_STTC, "Called state changed : STT_STATE_RECORDING");
		} else {
			SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Don't register state changed callback");
		}    

		client->current_state = STT_STATE_RECORDING;
	}

	free(temp);

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return ret;
}

int stt_stop(stt_h stt)
{
	SLOG(LOG_DEBUG, TAG_STTC, "===== STT STOP");

	if (NULL == stt) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] [ERROR] Input parameter is NULL");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_ERROR_INVALID_PARAMETER;
	}

	stt_client_s* client = stt_client_get(stt);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_ERROR_INVALID_PARAMETER;
	}   
	
	/* check state */
	if (client->current_state != STT_STATE_RECORDING) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid State : Current state is NOT RECORDING"); 
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_ERROR_INVALID_STATE;
	}

	int ret = 0; 
	/* do request */
	ret = stt_dbus_request_stop(client->uid);
	if (0 != ret) {
		SLOG(LOG_DEBUG, TAG_STTC, "[ERROR] Fail to stop");
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS]");

		if (NULL != client->state_changed_cb) {
			client->state_changed_cb(client->stt, client->current_state, STT_STATE_PROCESSING, client->state_changed_user_data); 
			SLOG(LOG_DEBUG, TAG_STTC, "Called state changed : STT_STATE_PROCESSING");
		} else {
			SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Don't register state changed callback");
		}

		client->current_state = STT_STATE_PROCESSING;
	}

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return ret;
}


int stt_cancel(stt_h stt)
{
	SLOG(LOG_DEBUG, TAG_STTC, "===== STT CANCEL");

	if (NULL == stt) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Input handle is null");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_ERROR_INVALID_PARAMETER;
	}

	stt_client_s* client = stt_client_get(stt);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_ERROR_INVALID_PARAMETER;
	} 	

	/* check state */
	if (STT_STATE_RECORDING != client->current_state && STT_STATE_PROCESSING != client->current_state) {
		SLOG(LOG_DEBUG, TAG_STTC, "[ERROR] Invalid state : Current state is 'Ready'");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_ERROR_INVALID_STATE;
	}

	int ret; 
	/* do request */
	ret = stt_dbus_request_cancel(client->uid);
	if (0 != ret) {
		SLOG(LOG_DEBUG, TAG_STTC, "[ERROR] Fail to cancel");
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS]");
		if (NULL != client->state_changed_cb) {
			client->state_changed_cb(client->stt, client->current_state, STT_STATE_READY, client->state_changed_user_data); 
			SLOG(LOG_DEBUG, TAG_STTC, "Called state changed : STT_STATE_READY");
		} else {
			SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Don't register state changed callback");
		}

		client->current_state = STT_STATE_READY;
	}

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return ret;
}

int stt_get_recording_volume(stt_h stt, float* volume)
{
	if (NULL == stt || NULL == volume) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Input parameter is NULL");
		return STT_ERROR_INVALID_PARAMETER;
	}

	stt_client_s* client = stt_client_get(stt);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] A handle is not available");
		return STT_ERROR_INVALID_PARAMETER;
	} 
	
	if (STT_STATE_RECORDING != client->current_state) {
		SLOG(LOG_DEBUG, TAG_STTC, "[ERROR] Invalid state : NO 'Recording' state");
		return STT_ERROR_INVALID_STATE;
	}    
	
	int ret = 0; 
	ret = stt_dbus_request_get_audio_volume(client->uid, volume);
	if (ret) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to get audio volume");
		return ret;
	}    

	return STT_ERROR_NONE;
}

int __stt_cb_error(int uid, int reason)
{
	stt_client_s* client = stt_client_get_by_uid(uid);
	if( NULL == client ) {
		SLOG(LOG_ERROR, TAG_STTC, "Handle not found\n");
		return -1;
	}

	client->current_state = STT_STATE_READY;

	if (NULL != client->error_cb) {
		stt_client_use_callback(client);
		client->error_cb(client->stt, reason, client->error_user_data); 
		stt_client_not_use_callback(client);
		SLOG(LOG_DEBUG, TAG_STTC, "client error callback called");
	} else {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Error occur but user callback is null");
	}    

	return 0;
}

int __stt_cb_result(int uid, const char* type, const char** data, int data_count, const char* msg)
{
	stt_client_s* client = NULL;
	
	client = stt_client_get_by_uid(uid);
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "Handle is NOT valid");
		return -1;
	}

	SLOG(LOG_DEBUG, TAG_STTC, "Recognition Result Message = %s", msg);

	int i=0;
	for (i = 0;i < data_count;i++) {
		if(NULL != data[i])
			SLOG(LOG_DEBUG, TAG_STTC, "Recognition Result[%d] = %s", i, data[i]);
	}	

	if (NULL != client->result_cb) {
		stt_client_use_callback(client);
		client->result_cb(client->stt, type, data, data_count, msg, client->result_user_data);
		stt_client_not_use_callback(client);
		SLOG(LOG_DEBUG, TAG_STTC, "client result callback called");
	} else {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] User result callback is null");
	}   

	if (NULL != client->state_changed_cb) {
		client->state_changed_cb(client->stt, client->current_state, STT_STATE_READY, client->state_changed_user_data); 
		SLOG(LOG_DEBUG, TAG_STTC, "client state changed callback called");
	} else {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Don't register result callback");
	}

	client->current_state = STT_STATE_READY;

	return 0;
}

int __stt_cb_partial_result(int uid, const char* data)
{
	stt_client_s* client = NULL;

	client = stt_client_get_by_uid(uid);
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "Handle is NOT valid");
		return -1;
	}

	if (client->current_state == STT_STATE_READY) {
		SLOG(LOG_ERROR, TAG_STTC, "Current state has already been 'Ready' state");	
		return 0;
	}

	if (client->partial_result_cb) {
		stt_client_use_callback(client);
		client->partial_result_cb(client->stt, data, client->partial_result_user_data);
		stt_client_not_use_callback(client);
		SLOG(LOG_DEBUG, TAG_STTC, "client partial result callback called");
	} else {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Don't register partial result callback");
	}   

	return 0;
}

int __stt_cb_stop_by_daemon(int uid)
{
	stt_client_s* client = stt_client_get_by_uid(uid);
	if( NULL == client ) {
		SLOG(LOG_ERROR, TAG_STTC, "Handle not found\n");
		return -1;
	}

	if (client->current_state != STT_STATE_RECORDING) {
		SLOG(LOG_ERROR, TAG_STTC, "Current state is NOT 'Recording' state");	
		return 0;
	}

	if (NULL != client->state_changed_cb) {
		stt_client_use_callback(client);
		client->state_changed_cb(client->stt, client->current_state, STT_STATE_PROCESSING, client->state_changed_user_data); 
		stt_client_not_use_callback(client);
		SLOG(LOG_DEBUG, TAG_STTC, "client state changed callback called");
	} else {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Error occur but user callback is null");
	}

	client->current_state = STT_STATE_PROCESSING;

	return 0;
}

int stt_set_result_cb(stt_h stt, stt_result_cb callback, void* user_data)
{
	if (stt == NULL || callback == NULL)
		return STT_ERROR_INVALID_PARAMETER;

	stt_client_s* client = stt_client_get(stt);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] A handle is not available");
		return STT_ERROR_INVALID_PARAMETER;
	}

	if (STT_STATE_READY != client->current_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Current state is not 'ready'."); 
		return STT_ERROR_INVALID_STATE;
	}

	client->result_cb = callback;
	client->result_user_data = user_data;

	return 0;
}

int stt_unset_result_cb(stt_h stt)
{
	if (NULL == stt)
		return STT_ERROR_INVALID_PARAMETER;

	stt_client_s* client = stt_client_get(stt);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] A handle is not available");
		return STT_ERROR_INVALID_PARAMETER;
	}

	if (STT_STATE_READY != client->current_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Current state is not 'ready'."); 
		return STT_ERROR_INVALID_STATE;
	}

	client->result_cb = NULL;
	client->result_user_data = NULL;

	return 0;
}

int stt_set_partial_result_cb(stt_h stt, stt_partial_result_cb callback, void* user_data)
{
	if (NULL == stt || NULL == callback)
		return STT_ERROR_INVALID_PARAMETER;

	stt_client_s* client = stt_client_get(stt);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] A handle is not available");
		return STT_ERROR_INVALID_PARAMETER;
	}

	if (STT_STATE_READY != client->current_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Current state is not 'ready'."); 
		return STT_ERROR_INVALID_STATE;
	}

	client->partial_result_cb = callback;
	client->partial_result_user_data = user_data;

	return 0;
}

int stt_unset_partial_result_cb(stt_h stt)
{
	if (NULL == stt)
		return STT_ERROR_INVALID_PARAMETER;

	stt_client_s* client = stt_client_get(stt);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] A handle is not available");
		return STT_ERROR_INVALID_PARAMETER;
	}

	if (STT_STATE_READY != client->current_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Current state is not 'ready'."); 
		return STT_ERROR_INVALID_STATE;
	}

	client->partial_result_cb = NULL;
	client->partial_result_user_data = NULL;

	return 0;
}

int stt_set_state_changed_cb(stt_h stt, stt_state_changed_cb callback, void* user_data)
{
	if (NULL == stt || NULL == callback)
		return STT_ERROR_INVALID_PARAMETER;

	stt_client_s* client = stt_client_get(stt);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] A handle is not available");
		return STT_ERROR_INVALID_PARAMETER;
	}

	if (STT_STATE_READY != client->current_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Current state is not 'ready'."); 
		return STT_ERROR_INVALID_STATE;
	}

	client->state_changed_cb = callback;
	client->state_changed_user_data = user_data;

	return 0;
}

int stt_unset_state_changed_cb(stt_h stt)
{
	if (NULL == stt)
		return STT_ERROR_INVALID_PARAMETER;

	stt_client_s* client = stt_client_get(stt);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] A handle is not available");
		return STT_ERROR_INVALID_PARAMETER;
	}

	if (STT_STATE_READY != client->current_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Current state is not 'ready'."); 
		return STT_ERROR_INVALID_STATE;
	}

	client->state_changed_cb = NULL;
	client->state_changed_user_data = NULL;

	return 0;
}


int stt_set_error_cb(stt_h stt, stt_error_cb callback, void* user_data)
{
	if (NULL == stt || NULL == callback)
		return STT_ERROR_INVALID_PARAMETER;

	stt_client_s* client = stt_client_get(stt);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] A handle is not available");
		return STT_ERROR_INVALID_PARAMETER;
	}

	if (STT_STATE_READY != client->current_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Current state is not 'ready'."); 
		return STT_ERROR_INVALID_STATE;
	}

	client->error_cb = callback;
	client->error_user_data = user_data;

	return 0;
}

int stt_unset_error_cb(stt_h stt)
{
	if (NULL == stt)
		return STT_ERROR_INVALID_PARAMETER;

	stt_client_s* client = stt_client_get(stt);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] A handle is not available");
		return STT_ERROR_INVALID_PARAMETER;
	}

	if (STT_STATE_READY != client->current_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Current state is not 'ready'."); 
		return STT_ERROR_INVALID_STATE;
	}

	client->error_cb = NULL;
	client->error_user_data = NULL;

	return 0;
}

static bool __stt_is_alive()
{
	FILE *fp = NULL;
	char buff[256] = {'\0',};
	char cmd[256] = {'\0',};

	memset(buff, '\0', 256);
	memset(cmd, '\0', 256);

	fp = popen("ps", "r");
	if (NULL == fp) {
		SLOG(LOG_DEBUG, TAG_STTC, "[STT SETTING ERROR] popen error \n");
		return FALSE;
	}

	while (fgets(buff, 255, fp)) {
		strcpy(cmd, buff + 26);

		if( 0 == strncmp(cmd, "[stt-daemon]", strlen("[stt-daemon]")) ||
		0 == strncmp(cmd, "stt-daemon", strlen("stt-daemon")) ||
		0 == strncmp(cmd, "/usr/bin/stt-daemon", strlen("/usr/bin/stt-daemon"))) {
			SLOG(LOG_DEBUG, TAG_STTC, "stt-daemon is ALIVE !! \n");
			fclose(fp);
			return TRUE;
		}
	}

	fclose(fp);

	SLOG(LOG_DEBUG, TAG_STTC, "THERE IS NO stt-daemon !! \n");

	return FALSE;
}


static void __my_sig_child(int signo, siginfo_t *info, void *data)
{
	int status;
	pid_t child_pid, child_pgid;

	child_pgid = getpgid(info->si_pid);
	SLOG(LOG_DEBUG, TAG_STTC, "Signal handler: dead pid = %d, pgid = %d\n", info->si_pid, child_pgid);

	while((child_pid = waitpid(-1, &status, WNOHANG)) > 0) {
		if(child_pid == child_pgid)
			killpg(child_pgid, SIGKILL);
	}

	return;
}


int __check_stt_daemon()
{
	if( TRUE == __stt_is_alive() )
		return 0;
	
	/* fork-exec stt-daemon */
	int pid, i;
	struct sigaction act, dummy;

	act.sa_handler = NULL;
	act.sa_sigaction = __my_sig_child;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_NOCLDSTOP | SA_SIGINFO;
		
	if(sigaction(SIGCHLD, &act, &dummy) < 0) {
		SLOG(LOG_DEBUG, TAG_STTC, "%s\n", "Cannot make a signal handler\n");
		return -1;
	}

	pid = fork();

	switch(pid) {
	case -1:
		SLOG(LOG_DEBUG, TAG_STTC, "[STT ERROR] fail to create STT-DAEMON \n");
		break;

	case 0:
		setsid();
		for (i = 0;i < _NSIG;i++)
			signal(i, SIG_DFL);

		execl("/usr/bin/stt-daemon", "/usr/bin/stt-daemon", NULL);
		break;

	default:
		sleep(1);
		break;
	}

	return 0;
}




