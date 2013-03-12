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


#include <sys/wait.h>
#include <sys/types.h> 
#include <unistd.h>
#include <Ecore.h>
#include <sys/stat.h>
#include <dirent.h>

#include "stt.h"
#include "stt_main.h"
#include "stt_client.h"
#include "stt_dbus.h"

#define CONNECTION_RETRY_COUNT 3

static bool g_is_daemon_started = false;

static int __check_stt_daemon();
static Eina_Bool __stt_notify_state_changed(void *data);
static Eina_Bool __stt_notify_error(void *data);

int stt_create(stt_h* stt)
{
	SLOG(LOG_DEBUG, TAG_STTC, "===== Create STT");

	if (NULL == stt) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] A handle is null");
		return STT_ERROR_INVALID_PARAMETER;
	}

	if (0 == stt_client_get_size()) {
		if (0 != stt_dbus_open_connection()) {
			SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to open connection");
			return STT_ERROR_OPERATION_FAILED;
		}
	}

	if (0 != stt_client_new(stt)) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to create client!");
		return STT_ERROR_OUT_OF_MEMORY;
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

	int ret = -1;

	/* check state */
	switch (client->current_state) {
	case STT_STATE_PROCESSING:
	case STT_STATE_RECORDING:
	case STT_STATE_READY:
		ret = stt_dbus_request_finalize(client->uid);
		if (0 != ret) {
			SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to request finalize");
		}
	case STT_STATE_CREATED:
		/* Free resources */
		stt_client_destroy(stt);
		break;
	}

	SLOG(LOG_DEBUG, TAG_STTC, "Success: destroy");

	if (0 == stt_client_get_size()) {
		if (0 != stt_dbus_close_connection()) {
			SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to close connection");
		}
	}

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return STT_ERROR_NONE;
}

static Eina_Bool __stt_connect_daemon(void *data)
{
	SLOG(LOG_DEBUG, TAG_STTC, "===== Connect daemon");

	stt_h stt = (stt_h)data;

	stt_client_s* client = stt_client_get(stt);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_ERROR_INVALID_PARAMETER;
	}

	/* Send hello */
	if (0 != stt_dbus_request_hello()) {
		if (false == g_is_daemon_started) {
			g_is_daemon_started = true;
			__check_stt_daemon();
		}
		return EINA_TRUE;
	}

	/* request initialization */
	int ret = -1;
	int i = 1;
	bool silence_supported = false;
	bool profanity_supported = false;
	bool punctuation_supported = false;

	while (1) {
		ret = stt_dbus_request_initialize(client->uid, &silence_supported, &profanity_supported, &punctuation_supported);

		if (STT_ERROR_ENGINE_NOT_FOUND == ret) {
			SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to initialize : STT Engine Not found");
			
			client->reason = STT_ERROR_ENGINE_NOT_FOUND;

			ecore_timer_add(0, __stt_notify_error, (void*)stt);

			return EINA_FALSE;

		} else if(0 != ret) {
			usleep(1);
			if(CONNECTION_RETRY_COUNT == i) {
				SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to initialize : TIMED OUT");

				client->reason = STT_ERROR_TIMED_OUT;

				ecore_timer_add(0, __stt_notify_error, (void*)stt);

				return EINA_FALSE;
			}    
			i++;
		} else {
			/* success to connect stt-daemon */
			stt_client_set_option_supported(client->stt, silence_supported, profanity_supported, punctuation_supported);
			SLOG(LOG_DEBUG, TAG_STTC, "Supported options : silence(%s), profanity(%s), punctuation(%s)", 
				silence_supported ? "true" : "false", profanity_supported ? "true" : "false", punctuation_supported ? "true" : "false");
			break;
		}
	}

	client->before_state = client->current_state;
	client->current_state = STT_STATE_READY;

	ecore_timer_add(0, __stt_notify_state_changed, (void*)stt);

	SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] uid(%d)", client->uid);

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, "  ");

	return EINA_FALSE;
}


int stt_prepare(stt_h stt)
{
	SLOG(LOG_DEBUG, TAG_STTC, "===== Prepare STT");

	stt_client_s* client = stt_client_get(stt);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_ERROR_INVALID_PARAMETER;
	}

	/* check state */
	if (client->current_state != STT_STATE_CREATED) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid State: Current state is not 'CREATED'"); 
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_ERROR_INVALID_STATE;
	}

	ecore_timer_add(0, __stt_connect_daemon, (void*)stt);

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return STT_ERROR_NONE;
}

int stt_unprepare(stt_h stt)
{
	SLOG(LOG_DEBUG, TAG_STTC, "===== Unprepare STT");

	stt_client_s* client = stt_client_get(stt);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] A handle is not available");
		return STT_ERROR_INVALID_PARAMETER;
	}

	/* check state */
	if (client->current_state != STT_STATE_READY) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid State: Current state is not 'READY'"); 
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_ERROR_INVALID_STATE;
	}

	int ret = stt_dbus_request_finalize(client->uid);
	if (0 != ret) {
		SLOG(LOG_WARN, TAG_STTC, "[ERROR] Fail to request finalize");
	}

	client->before_state = client->current_state;
	client->current_state = STT_STATE_CREATED;

	ecore_timer_add(0, __stt_notify_state_changed, (void*)stt);

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

	/* check state */
	if (client->current_state != STT_STATE_READY) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid State: Current state is not READY"); 
		return STT_ERROR_INVALID_STATE;
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

	/* check state */
	if (client->current_state != STT_STATE_READY) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid State: Current state is not READY"); 
		return STT_ERROR_INVALID_STATE;
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
		case STT_STATE_CREATED:		SLOG(LOG_DEBUG, TAG_STTC, "Current state is 'CREATED'");	break;
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

	/* check state */
	if (client->current_state != STT_STATE_READY) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid State: Current state is not READY"); 
		return STT_ERROR_INVALID_STATE;
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

	/* check state */
	if (client->current_state != STT_STATE_READY) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid State: Current state is not READY"); 
		return STT_ERROR_INVALID_STATE;
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
	
	/* check state */
	if (client->current_state != STT_STATE_READY) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid State: Current state is not READY"); 
		return STT_ERROR_INVALID_STATE;
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

	/* check state */
	if (client->current_state != STT_STATE_READY) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid State: Current state is not READY"); 
		return STT_ERROR_INVALID_STATE;
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

		client->before_state = client->current_state;
		client->current_state = STT_STATE_RECORDING;

		ecore_timer_add(0, __stt_notify_state_changed, (void*)stt);
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

		client->before_state = client->current_state;
		client->current_state = STT_STATE_PROCESSING;

		ecore_timer_add(0, __stt_notify_state_changed, (void*)stt);
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

		client->before_state = client->current_state;
		client->current_state = STT_STATE_READY;

		ecore_timer_add(0, __stt_notify_state_changed, (void*)stt);
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

static Eina_Bool __stt_notify_error(void *data)
{
	stt_h stt = (stt_h)data;

	stt_client_s* client = stt_client_get(stt);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to notify error : A handle is not valid");
		return EINA_FALSE;
	}

	if (NULL != client->error_cb) {
		stt_client_use_callback(client);
		client->error_cb(client->stt, client->reason, client->error_user_data); 
		stt_client_not_use_callback(client);
		SLOG(LOG_DEBUG, TAG_STTC, "Error callback is called");
	} else {
		SLOG(LOG_WARN, TAG_STTC, "[WARNING] Error callback is null");
	}

	return EINA_FALSE;
}

int __stt_cb_error(int uid, int reason)
{
	stt_client_s* client = stt_client_get_by_uid(uid);
	if( NULL == client ) {
		SLOG(LOG_ERROR, TAG_STTC, "Handle not found\n");
		return -1;
	}

	client->reason = reason;

	if (NULL != client->error_cb) {
		ecore_timer_add(0, __stt_notify_error, client->stt);
	} else {
		SLOG(LOG_WARN, TAG_STTC, "[WARNING] Error callback is null");
	}    

	return 0;
}

static Eina_Bool __stt_notify_result(void *data)
{
	stt_h stt = (stt_h)data;

	stt_client_s* client = stt_client_get(stt);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to notify error : A handle is not valid");
		return EINA_FALSE;
	}

	if (NULL != client->result_cb) {
		stt_client_use_callback(client);
		client->result_cb(client->stt, client->type, (const char**)client->data_list, client->data_count, client->msg, client->result_user_data);
		stt_client_not_use_callback(client);
		SLOG(LOG_DEBUG, TAG_STTC, "client result callback called");
	} else {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] User result callback is null");
	} 

	/* Free result */
	if (NULL != client->type)
		free(client->type);

	if (NULL != client->data_list) {
		char **temp = NULL;
		temp = client->data_list;

		int i = 0;
		for (i = 0;i < client->data_count;i++) {
			if(NULL != temp[i])
				free(temp[i]);
			else 
				SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Result data is error");
		}
		free(client->data_list);
	}
	
	if (NULL != client->msg) 
		free(client->msg);

	client->data_count = 0;

	return EINA_FALSE;
}

static Eina_Bool __stt_notify_state_changed(void *data)
{
	stt_h stt = (stt_h)data;

	stt_client_s* client = stt_client_get(stt);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to notify error : A handle is not valid");
		return EINA_FALSE;
	}

	if (NULL != client->state_changed_cb) {
		stt_client_use_callback(client);
		client->state_changed_cb(client->stt, client->before_state, client->current_state, client->state_changed_user_data); 
		stt_client_not_use_callback(client);
		SLOG(LOG_DEBUG, TAG_STTC, "State changed callback is called");
	} else {
		SLOG(LOG_WARN, TAG_STTC, "[WARNING] State changed callback is null");
	}

	return EINA_FALSE;
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
		client->type = strdup(type);
		client->msg = strdup(msg);
		client->data_count = data_count;

		if (data_count > 0) {
			char **temp = NULL;
			temp = malloc( sizeof(char*) * data_count);

			for (i = 0;i < data_count;i++) {
				if(NULL != data[i])
					temp[i] = strdup(data[i]);
				else 
					SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Result data is error");
			}

			client->data_list = temp;
		}

		ecore_timer_add(0, __stt_notify_result, client->stt);
	} else {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] User result callback is null");
	}   

	client->before_state = client->current_state;
	client->current_state = STT_STATE_READY;

	if (NULL != client->state_changed_cb) {
		ecore_timer_add(0, __stt_notify_state_changed, client->stt);
	} else {
		SLOG(LOG_WARN, TAG_STTC, "[WARNING] State changed callback is null");
	}

	return 0;
}

static Eina_Bool __stt_notify_partial_result(void *data)
{
	stt_h stt = (stt_h)data;

	stt_client_s* client = stt_client_get(stt);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to notify error : A handle is not valid");
		return EINA_FALSE;
	}

	if (client->partial_result_cb) {
		stt_client_use_callback(client);
		client->partial_result_cb(client->stt, client->partial_result, client->partial_result_user_data);
		stt_client_not_use_callback(client);
		SLOG(LOG_DEBUG, TAG_STTC, "Partial result callback is called");
	} else {
		SLOG(LOG_WARN, TAG_STTC, "[WARNING] Partial result callback is null");
	}   

	if (NULL != client->partial_result)
		free(client->partial_result);

	return EINA_FALSE;
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
		client->partial_result = strdup(data);
		ecore_timer_add(0, __stt_notify_partial_result, client->stt);
	} else {
		SLOG(LOG_WARN, TAG_STTC, "[WARNING] Partial result callback is null");
	}  

	return 0;
}

int __stt_cb_set_state(int uid, int state)
{
	stt_client_s* client = stt_client_get_by_uid(uid);
	if( NULL == client ) {
		SLOG(LOG_ERROR, TAG_STTC, "Handle not found");
		return -1;
	}

	stt_state_e state_from_daemon = (stt_state_e)state;

	if (client->current_state == state_from_daemon) {
		SLOG(LOG_DEBUG, TAG_STTC, "Current state has already been %d", client->current_state);
		return 0;
	}

	client->before_state = client->current_state;
	client->current_state = state_from_daemon;

	ecore_timer_add(0, __stt_notify_state_changed, client->stt);
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

	if (STT_STATE_CREATED != client->current_state) {
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

	if (STT_STATE_CREATED != client->current_state) {
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

	if (STT_STATE_CREATED != client->current_state) {
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

	if (STT_STATE_CREATED != client->current_state) {
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

	if (STT_STATE_CREATED != client->current_state) {
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

	if (STT_STATE_CREATED != client->current_state) {
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

	if (STT_STATE_CREATED != client->current_state) {
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

	if (STT_STATE_CREATED != client->current_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Current state is not 'ready'."); 
		return STT_ERROR_INVALID_STATE;
	}

	client->error_cb = NULL;
	client->error_user_data = NULL;

	return 0;
}

int __get_cmd_line(char *file, char *buf) 
{
	FILE *fp = NULL;
	int i;

	fp = fopen(file, "r");
	if (fp == NULL) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Get command line");
		return -1;
	}

	memset(buf, 0, 256);
	fgets(buf, 256, fp);
	fclose(fp);

	return 0;
}

static bool __stt_is_alive()
{
	DIR *dir;
	struct dirent *entry;
	struct stat filestat;
	
	int pid;
	char cmdLine[256];
	char tempPath[256];

	dir  = opendir("/proc");
	if (NULL == dir) {
		SLOG(LOG_ERROR, TAG_STTC, "process checking is FAILED");
		return FALSE;
	}

	while ((entry = readdir(dir)) != NULL) {
		if (0 != lstat(entry->d_name, &filestat))
			continue;

		if (!S_ISDIR(filestat.st_mode))
			continue;

		pid = atoi(entry->d_name);
		if (pid <= 0) continue;

		sprintf(tempPath, "/proc/%d/cmdline", pid);
		if (0 != __get_cmd_line(tempPath, cmdLine)) {
			continue;
		}
		if ( 0 == strncmp(cmdLine, "[stt-daemon]", strlen("[stt-daemon]")) ||
			0 == strncmp(cmdLine, "stt-daemon", strlen("stt-daemon")) ||
			0 == strncmp(cmdLine, "/usr/bin/stt-daemon", strlen("/usr/bin/stt-daemon"))) {
				SLOG(LOG_DEBUG, TAG_STTC, "stt-daemon is ALIVE !! \n");
				closedir(dir);
				return TRUE;
		}
	}
	SLOG(LOG_DEBUG, TAG_STTC, "THERE IS NO stt-daemon !! \n");

	closedir(dir);
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


static int __check_stt_daemon()
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
		break;
	}

	return 0;
}




