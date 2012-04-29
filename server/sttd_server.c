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


#include "sttd_main.h"
#include "sttd_server.h"

#include "sttd_client_data.h"
#include "sttd_engine_agent.h"
#include "sttd_config.h"
#include "sttd_recorder.h"
#include "sttd_network.h"
#include "sttd_dbus.h"

/*
* STT Server static variable
*/
static bool g_is_engine;

/*
* STT Server Callback Functions											`				  *
*/

int audio_recorder_callback(const void* data, const unsigned int length)
{
	if (0 != sttd_engine_recognize_audio(data, length)) {
		
		/* send message for stop */
		SLOG(LOG_DEBUG, TAG_STTD, "===== Fail to set recording data ");
			
		int uid = sttd_client_get_current_recording();

		app_state_e state;
		if (0 != sttd_client_get_state(uid, &state)) {
			SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] uid is not valid "); 
			return -1;
		}

		if (APP_STATE_RECORDING != state) {
			SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Current state is not recording"); 
			return -1;
		}

		if (0 != sttd_send_stop_recognition_by_daemon(uid)) {
			SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail "); 
		} else {
			SLOG(LOG_DEBUG, TAG_STTD, "[Server SUCCESS] <<<< stop message : uid(%d)", uid); 
		}

		SLOG(LOG_DEBUG, TAG_STTD, "=====");
		SLOG(LOG_DEBUG, TAG_STTD, "  ");
		
		return -1;
	}
	
	

	return 0;
}

void sttd_server_recognition_result_callback(sttp_result_event_e event, const char* type, 
					const char** data, int data_count, const char* msg, void *user_data)
{
	SLOG(LOG_DEBUG, TAG_STTD, "===== Recognition Result Callback");

	/* check uid */
	int *uid = (int*)user_data;

	SLOG(LOG_DEBUG, TAG_STTD, "[Server] uid (%d), event(%d)", uid, event); 

	app_state_e state;
	if (0 != sttd_client_get_state(*uid, &state)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] uid is NOT valid "); 
		SLOG(LOG_DEBUG, TAG_STTD, "=====");
		SLOG(LOG_DEBUG, TAG_STTD, "  ");
		return;
	}

	/* send result to client */
	if (STTP_RESULT_EVENT_SUCCESS == event && 0 < data_count && NULL != data) {

		if (APP_STATE_PROCESSING == state ) {
			SLOG(LOG_DEBUG, TAG_STTD, "[Server] the size of result from engine is '%d' %s", data_count); 

			if (0 != sttdc_send_result(*uid, type, data, data_count, msg)) {
				SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to send result"); 	
				int reason = (int)STTD_ERROR_OPERATION_FAILED;

				if (0 != sttdc_send_error_signal(*uid, reason, "Fail to send recognition result")) {
					SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to send error info . Remove client data"); 

					/* clean client data */
					sttd_client_delete(*uid);
				}
			}
		} else {
			SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Current state is NOT thinking"); 

			int reason = (int)STTD_ERROR_INVALID_STATE;		
			if (0 != sttdc_send_error_signal(*uid, reason, "Client state is NOT thinking. Client don't receive recognition result in current state.")) {
				SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to send error info. Remove client data "); 

				/* clean client data */
				sttd_client_delete(*uid);		
			}
		}
	} else if (STTP_RESULT_EVENT_NO_RESULT == event) {

		if (APP_STATE_PROCESSING == state ) {
			if (0 != sttdc_send_result(*uid, NULL, NULL, 0, NULL)) {
				SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to send result "); 

				/* send error msg */
				int reason = (int)STTD_ERROR_INVALID_STATE;	
				if (0 != sttdc_send_error_signal(*uid, reason, "[ERROR] Fail to send recognition result")) {
					SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to send error info "); 
					sttd_client_delete(*uid);
				}
			}
		} else {
			SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Current state is NOT thinking "); 

			int reason = (int)STTD_ERROR_INVALID_STATE;		

			if (0 != sttdc_send_error_signal(*uid, reason, "Client state is NOT thinking. Client don't receive recognition result in current state.")) {
				SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to send error info. Remove client data"); 

				/* clean client data */
				sttd_client_delete(*uid);		
			}
		}
	} else if (STTP_RESULT_EVENT_ERROR == event) {
		int reason = (int)STTD_ERROR_OPERATION_FAILED;

		if (0 != sttdc_send_error_signal(*uid, reason, "STT Engine ERROR : Recognition fail")) {
			SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to send error info"); 
			sttd_client_delete(*uid);
		}	
	} else {
		/* nothing */
	}

	/* change state of uid */
	sttd_client_set_state(*uid, APP_STATE_READY);

	if (NULL != user_data)	
		free(user_data);

	SLOG(LOG_DEBUG, TAG_STTD, "=====");
	SLOG(LOG_DEBUG, TAG_STTD, "  ");

	return;
}

void sttd_server_partial_result_callback(sttp_result_event_e event, const char* data, void *user_data)
{
	SLOG(LOG_DEBUG, TAG_STTD, "===== Partial Result Callback");

	/* check uid */
	int *uid = (int*)user_data;

	SLOG(LOG_DEBUG, TAG_STTD, "[Server] uid (%d), event(%d)", uid, event); 

	app_state_e state;
	if (0 != sttd_client_get_state(*uid, &state)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] uid is NOT valid "); 
		SLOG(LOG_DEBUG, TAG_STTD, "=====");
		SLOG(LOG_DEBUG, TAG_STTD, "  ");
		return;
	}

	/* send result to client */
	if (STTP_RESULT_EVENT_SUCCESS == event && NULL != data) {
		if (0 != sttdc_send_partial_result(*uid, data)) {
			SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to send partial result"); 	
		}
	} 

	SLOG(LOG_DEBUG, TAG_STTD, "=====");
	SLOG(LOG_DEBUG, TAG_STTD, "  ");
}

void sttd_server_silence_dectection_callback(void *user_param)
{
	SLOG(LOG_DEBUG, TAG_STTD, "===== Silence Detection Callback");

	int uid = sttd_client_get_current_recording();

	app_state_e state;
	if (0 != sttd_client_get_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] uid is not valid "); 
		return;
	}

	if (APP_STATE_RECORDING != state) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Current state is not recording"); 
		return;
	}

	if (0 != sttd_send_stop_recognition_by_daemon(uid)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail "); 
	} else {
		SLOG(LOG_DEBUG, TAG_STTD, "[Server SUCCESS] <<<< stop message : uid(%d)", uid); 
	}

	SLOG(LOG_DEBUG, TAG_STTD, "=====");
	SLOG(LOG_DEBUG, TAG_STTD, "  ");

	return;
}

/*
* Daemon initialize
*/

int sttd_initialize()
{
	int ret = 0;

	/* recoder init */
	ret = sttd_recorder_init();
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to initialize recorder : result(%d)", ret); 
		return ret;
	}

	/* Engine Agent initialize */
	ret = sttd_engine_agent_init(sttd_server_recognition_result_callback, sttd_server_partial_result_callback, 
				sttd_server_silence_dectection_callback);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to engine agent initialize : result(%d)", ret);
		return ret;
	}
	
	if (0 != sttd_engine_agent_initialize_current_engine()) {
		SLOG(LOG_WARN, TAG_STTD, "[Server WARNING] There is No STT-Engine !!!!!"); 
		g_is_engine = false;
	} else {
		g_is_engine = true;
	}

	SLOG(LOG_DEBUG, TAG_STTD, "[Server SUCCESS] initialize"); 

	return 0;
}

/*
* STT Server Functions for Client
*/

int sttd_server_initialize(int pid, int uid, bool* silence, bool* profanity, bool* punctuation)
{
	if (false == g_is_engine) {
		if (0 != sttd_engine_agent_initialize_current_engine()) {
			SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] No Engine"); 
			g_is_engine = false;
			return STTD_ERROR_ENGINE_NOT_FOUND;
		} else {
			g_is_engine = true;
		}
	}

	/* check if uid is valid */
	app_state_e state;
	if (0 == sttd_client_get_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] uid has already been registered"); 
		return STTD_ERROR_INVALID_PARAMETER;
	}
	
	/* load if engine is unloaded */
	if (0 == sttd_client_get_ref_count()) {
		if (0 != sttd_engine_agent_load_current_engine()) {
			SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to load current engine"); 
			return STTD_ERROR_OPERATION_FAILED;
		}
	}

	/* initialize recorder using audio format from engine */
	sttp_audio_type_e atype;
	int rate;
	int channels;

	if (0 != sttd_engine_get_audio_format(&atype, &rate, &channels)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to get audio format of engine."); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	sttd_recorder_channel	sttchannel;
	sttd_recorder_audio_type	sttatype;

	switch (atype) {
	case STTP_AUDIO_TYPE_PCM_S16_LE:	sttatype = STTD_RECORDER_PCM_S16;	break;
	case STTP_AUDIO_TYPE_PCM_U8:		sttatype = STTD_RECORDER_PCM_U8;	break;
	case STTP_AUDIO_TYPE_AMR:		sttatype = STTD_RECORDER_AMR;		break;
	default:	
		/* engine error */
		sttd_engine_agent_unload_current_engine();
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Invalid Audio Type"); 
		return STTD_ERROR_OPERATION_FAILED;
		break;
	}

	switch (sttchannel) {
	case 1:		sttchannel = STTD_RECORDER_CHANNEL_MONO;	break;
	case 2:		sttchannel = STTD_RECORDER_CHANNEL_STEREO;	break;
	default:	sttchannel = STTD_RECORDER_CHANNEL_MONO;	break;
	}

	if (0 != sttd_recorder_set(sttatype, sttchannel, rate, 60, audio_recorder_callback)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to set recorder"); 
		return STTD_ERROR_OPERATION_FAILED;
	}
	
	SLOG(LOG_DEBUG, TAG_STTD, "[Server] audio type(%d), channel(%d)", (int)atype, (int)sttchannel); 

	/* Add client information to client manager */
	if (0 != sttd_client_add(pid, uid)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to add client info"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (0 != sttd_engine_get_option_supported(silence, profanity, punctuation)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to get engine options supported"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	SLOG(LOG_DEBUG, TAG_STTD, "[Server Success] Initialize"); 

	return STTD_ERROR_NONE;
}

int sttd_server_finalize(const int uid)
{
	/* check if uid is valid */
	app_state_e state;
	if (0 != sttd_client_get_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] uid is NOT valid "); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	/* release recorder */
	app_state_e appstate;
	sttd_client_get_state(uid, &appstate);

	if (APP_STATE_RECORDING == appstate || APP_STATE_PROCESSING == appstate) {
		sttd_recorder_cancel();
		sttd_engine_recognize_cancel();
	}
	
	/* Remove client information */
	if (0 != sttd_client_delete(uid)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to delete client"); 
	}

	/* unload engine, if ref count of client is 0 */
	if (0 == sttd_client_get_ref_count()) {
		if (0 != sttd_engine_agent_unload_current_engine()) {
			SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to unload current engine"); 
		} else {
			SLOG(LOG_DEBUG, TAG_STTD, "[Server SUCCESS] unload current engine"); 
		}
	}
	
	return STTD_ERROR_NONE;
}

int sttd_server_get_supported_languages(const int uid, GList** lang_list)
{
	/* check if uid is valid */
	app_state_e state;
	if (0 != sttd_client_get_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] uid is NOT valid "); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	/* get language list from engine */
	int ret = sttd_engine_supported_langs(lang_list);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to get supported languages"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	SLOG(LOG_DEBUG, TAG_STTD, "[Server SUCCESS] sttd_server_get_supported_languages"); 

	return STTD_ERROR_NONE;
}

int sttd_server_get_current_langauage(const int uid, char** current_lang)
{
	/* check if uid is valid */
	app_state_e state;
	if (0 != sttd_client_get_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] uid is NOT valid "); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (NULL == current_lang) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Input parameter is NULL"); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	/*get current language from engine */
	int ret = sttd_engine_get_default_lang(current_lang);
	if (0 != ret) {	
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to get default language :result(%d)", ret); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	SLOG(LOG_DEBUG, TAG_STTD, "[Server SUCCESS] sttd_engine_get_default_lang"); 

	return STTD_ERROR_NONE;
}

int sttd_server_is_partial_result_supported(int uid, int* partial_result)
{
	/* check if uid is valid */
	app_state_e state;
	if (0 != sttd_client_get_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] uid is NOT valid "); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (NULL == partial_result) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Input parameter is NULL"); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	bool temp;
	int ret = sttd_engine_is_partial_result_supported(&temp);
	if (0 != ret) {	
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to get partial result supported : result(%d)", ret); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	SLOG(LOG_DEBUG, TAG_STTD, "[Server SUCCESS] Partial result supporting is %s", temp ? "true" : "false"); 

	return STTD_ERROR_NONE;
}

int sttd_server_get_audio_volume( const int uid, float* current_volume)
{
	/* check if uid is valid */
	app_state_e state;
	if (0 != sttd_client_get_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] uid is NOT valid "); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	/* check uid state */
	if (APP_STATE_RECORDING != state) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Current state is not recording"); 
		return STTD_ERROR_INVALID_STATE;
	}

	if (NULL == current_volume) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Input parameter is NULL"); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	/* get audio volume from recorder */
	int ret = sttd_recorder_get_volume(current_volume);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to get volume : result(%d)", ret); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	return STTD_ERROR_NONE;
}

int sttd_server_start(const int uid, const char* lang, const char* recognition_type, 
		      int profanity, int punctuation, int silence)
{
	/* check if uid is valid */
	app_state_e state;
	if (0 != sttd_client_get_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] uid is NOT valid "); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	/* check uid state */
	if (APP_STATE_READY != state) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] sttd_server_start : current state is not ready"); 
		return STTD_ERROR_INVALID_STATE;
	}

	if (0 < sttd_client_get_current_recording()) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Current STT Engine is busy because of recording");
		return STTD_ERROR_RECORDER_BUSY;
	}

	if (0 < sttd_client_get_current_thinking()) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Current STT Engine is busy because of thinking");
		return STTD_ERROR_RECORDER_BUSY;
	}

	/* check if engine use network */
	if (true == sttd_engine_agent_need_network()) {
		if (false == sttd_network_is_connected()) {
			SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Disconnect network. Current engine needs to network connection.");
			return STTD_ERROR_OUT_OF_NETWORK;
		}
	}

	/* engine start recognition */
	int* user_data;
	user_data = (int*)malloc( sizeof(int) * 1);
	
	/* free on result callback */
	*user_data = uid;

	SLOG(LOG_DEBUG, TAG_STTD, "[Server] start : uid(%d), lang(%s), recog_type(%s)", *user_data, lang, recognition_type ); 

	int ret = sttd_engine_recognize_start((char*)lang, recognition_type, profanity, punctuation, silence, (void*)user_data);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to start recognition : result(%d)", ret); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	/* recorder start */
	ret = sttd_recorder_start();
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to start recorder : result(%d)", ret); 
		sttd_engine_recognize_cancel();
		return STTD_ERROR_OPERATION_FAILED;
	}

	SLOG(LOG_DEBUG, TAG_STTD, "[Server] start recording"); 

	/* change uid state */
	sttd_client_set_state(uid, APP_STATE_RECORDING);

	return STTD_ERROR_NONE;
}

int sttd_server_stop(const int uid)
{
	/* check if uid is valid */
	app_state_e state;
	if (0 != sttd_client_get_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] uid is NOT valid "); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	/* check uid state */
	if (APP_STATE_RECORDING != state) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Current state is not recording"); 
		return STTD_ERROR_INVALID_STATE;
	}

	/* stop recorder */
	sttd_recorder_stop();

	/* stop engine recognition */
	int ret = sttd_engine_recognize_stop();
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to stop : result(%d)", ret); 
		sttd_client_set_state(uid, APP_STATE_READY);		
	
		return STTD_ERROR_OPERATION_FAILED;
	}

	/* change uid state */
	sttd_client_set_state(uid, APP_STATE_PROCESSING);

	return STTD_ERROR_NONE;
}

int sttd_server_cancel(const int uid)
{
	/* check if uid is valid */
	app_state_e state;
	if (0 != sttd_client_get_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] uid is NOT valid "); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	/* check uid state */ 
	if (APP_STATE_READY == state) {
		SLOG(LOG_WARN, TAG_STTD, "[Server WARNING] Current state is ready"); 
		return STTD_ERROR_INVALID_STATE;
	}

	/* stop recorder */
	if (APP_STATE_RECORDING == state) 
		sttd_recorder_cancel();

	/* cancel engine recognition */
	int ret = sttd_engine_recognize_cancel();
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to cancel : result(%d)", ret); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	/* change uid state */
	sttd_client_set_state(uid, APP_STATE_READY);

	return STTD_ERROR_NONE;
}


/******************************************************************************************
* STT Server Functions for setting
*******************************************************************************************/

int sttd_server_setting_initialize(int uid)
{
	if (false == g_is_engine) {
		if (0 != sttd_engine_agent_initialize_current_engine()) {
			SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] No Engine"); 
			g_is_engine = false;
			return STTD_ERROR_ENGINE_NOT_FOUND;
		} else {
			g_is_engine = true;
		}
	}

	/* check if uid is valid */
	app_state_e state;
	if (0 == sttd_client_get_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] uid has already been registered"); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	/* load if engine is unloaded */
	if (0 == sttd_client_get_ref_count()) {
		if (0 != sttd_engine_agent_load_current_engine()) {
			SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to load current engine"); 
			return STTD_ERROR_OPERATION_FAILED;
		}
	}

	/* Add client information to client manager (For internal use) */
	if (0 != sttd_client_add(uid, uid)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to add client info"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	return STTD_ERROR_NONE;
}

int sttd_server_setting_finalize(int uid)
{
	/* check if uid is valid */
	app_state_e state;
	if (0 != sttd_client_get_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] uid is NOT valid "); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	/* Remove client information */
	if (0 != sttd_client_delete(uid)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to delete setting client"); 
	}

	/* unload engine, if ref count of client is 0 */
	if (0 == sttd_client_get_ref_count()) {
		if (0 != sttd_engine_agent_unload_current_engine()) {
			SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to unload current engine"); 
		} else {
			SLOG(LOG_DEBUG, TAG_STTD, "[Server SUCCESS] unload current engine"); 
		}
	}

	return STTD_ERROR_NONE;
}

int sttd_server_setting_get_engine_list(int uid, GList** engine_list)
{
	/* check if uid is valid */
	app_state_e state;
	if (0 != sttd_client_get_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] uid is NOT valid "); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (NULL == engine_list) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Input parameter is NULL"); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	int ret = sttd_engine_setting_get_engine_list(engine_list); 
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to get engine list : result(%d)", ret); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	return STTD_ERROR_NONE;
}

int sttd_server_setting_get_engine(int uid, char** engine_id)
{
	app_state_e state;
	if (0 != sttd_client_get_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] uid is NOT valid "); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (NULL == engine_id) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Input parameter is NULL"); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	/* get engine */
	int ret = sttd_engine_setting_get_engine(engine_id); 
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to get engine : result(%d)", ret); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	return STTD_ERROR_NONE;
}

int sttd_server_setting_set_engine(const int uid, const char* engine_id)
{
	/* check if uid is valid */
	app_state_e state;
	if (0 != sttd_client_get_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] uid is NOT valid "); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (NULL == engine_id) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Input parameter is NULL"); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	/* set engine */
	int ret = sttd_engine_setting_set_engine(engine_id); 
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to set engine : result(%d)", ret); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	return STTD_ERROR_NONE;
}

int sttd_server_setting_get_lang_list(int uid, char** engine_id, GList** lang_list)
{
	/* check if uid is valid */
	app_state_e state;
	if (0 != sttd_client_get_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] uid is NOT valid "); 
		return STTD_ERROR_INVALID_PARAMETER;
	}
	
	if (NULL == lang_list) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] language is NULL"); 
		return STTD_ERROR_INVALID_PARAMETER;
	}
	
	int ret = sttd_engine_setting_get_lang_list(engine_id, lang_list); 
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to get language list : result(%d)", ret); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	return STTD_ERROR_NONE;
}

int sttd_server_setting_get_default_language(int uid, char** language)
{
	/* check if uid is valid */
	app_state_e state;
	if (0 != sttd_client_get_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] uid is NOT valid "); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (NULL == language) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] language is NULL"); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	int ret = sttd_engine_setting_get_default_lang(language); 
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to get default language : result(%d)", ret); 
		return STTD_ERROR_OPERATION_FAILED;
	}	

	return STTD_ERROR_NONE;
}

int sttd_server_setting_set_default_language(int uid, const char* language)
{
	/* check if uid is valid */
	app_state_e state;
	if (0 != sttd_client_get_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] uid is NOT valid "); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (NULL == language) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] language is NULL"); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	int ret = sttd_engine_setting_set_default_lang((char*)language); 
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to set default lang : result(%d)", ret); 
		return STTD_ERROR_OPERATION_FAILED;
	}	

	return STTD_ERROR_NONE;
}

int sttd_server_setting_get_profanity_filter(int uid, bool* value)
{
	/* check if uid is valid */
	app_state_e state;
	if (0 != sttd_client_get_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] uid is NOT valid "); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (NULL == value) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Input parameter is NULL"); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	int ret = 0;
	ret = sttd_engine_setting_get_profanity_filter(value);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to get profanity filter : result(%d)", ret); 
		return STTD_ERROR_OPERATION_FAILED;
	}	

	return STTD_ERROR_NONE;
}

int sttd_server_setting_set_profanity_filter(int uid, bool value)
{
	/* check if uid is valid */
	app_state_e state;
	if (0 != sttd_client_get_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] uid is NOT valid "); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	int ret = 0;
	ret = sttd_engine_setting_set_profanity_filter(value);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to set profanity filter: result(%d)", ret); 
		return STTD_ERROR_OPERATION_FAILED;
	}	

	return STTD_ERROR_NONE;
}

int sttd_server_setting_get_punctuation_override(int uid, bool* value)
{
	/* check if uid is valid */
	app_state_e state;
	if (0 != sttd_client_get_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] uid is NOT valid "); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (NULL == value) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Input parameter is NULL"); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	int ret = 0;
	ret = sttd_engine_setting_get_punctuation_override(value);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to get punctuation override : result(%d)", ret); 
		return STTD_ERROR_OPERATION_FAILED;
	}	

	return STTD_ERROR_NONE;
}

int sttd_server_setting_set_punctuation_override(int uid, bool value)
{
	/* check if uid is valid */
	app_state_e state;
	if (0 != sttd_client_get_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] uid is NOT valid "); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	int ret = 0;
	ret = sttd_engine_setting_set_punctuation_override(value);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to set punctuation override : result(%d)", ret); 
		return STTD_ERROR_OPERATION_FAILED;
	}	

	return STTD_ERROR_NONE;
}

int sttd_server_setting_get_silence_detection(int uid, bool* value)
{
	/* check if uid is valid */
	app_state_e state;
	if (0 != sttd_client_get_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] uid is NOT valid "); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (NULL == value) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Input parameter is NULL"); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	int ret = 0;
	ret = sttd_engine_setting_get_silence_detection(value);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to get silence detection : result(%d)", ret); 
		return STTD_ERROR_OPERATION_FAILED;
	}	

	return STTD_ERROR_NONE;
}

int sttd_server_setting_set_silence_detection(int uid, bool value)
{
	/* check if uid is valid */
	app_state_e state;
	if (0 != sttd_client_get_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] uid is NOT valid "); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	int ret = 0;
	ret = sttd_engine_setting_set_silence_detection(value);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to Result(%d)", ret); 
		return STTD_ERROR_OPERATION_FAILED;
	}	

	return STTD_ERROR_NONE;
}

int sttd_server_setting_get_engine_setting(int uid, char** engine_id, GList** lang_list)
{	
	/* check if uid is valid */
	app_state_e state;
	if (0 != sttd_client_get_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] uid is NOT valid "); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (0 != sttd_engine_setting_get_engine_setting_info(engine_id, lang_list)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to get engine setting info"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	return STTD_ERROR_NONE;
}

int sttd_server_setting_set_engine_setting(int uid, const char* key, const char* value)
{	
	/* check if uid is valid */
	app_state_e state;
	if (0 != sttd_client_get_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] uid is NOT valid "); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (0 != sttd_engine_setting_set_engine_setting(key, value)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to set engine setting info"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	return STTD_ERROR_NONE;
}


