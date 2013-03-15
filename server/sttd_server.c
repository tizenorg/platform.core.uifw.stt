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

static double g_state_check_time = 15.5;

/*
* STT Server Callback Functions											`				  *
*/

Eina_Bool __stop_by_silence(void *data)
{
	SLOG(LOG_DEBUG, TAG_STTD, "===== Stop by silence detection");

	int uid = 0;

	uid = sttd_client_get_current_recording();

	if (uid > 0) {
		if (0 != sttd_server_stop(uid))
			return EINA_FALSE;

		int ret = sttdc_send_set_state(uid, (int)APP_STATE_PROCESSING);
		if (0 != ret) {
			SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to send state : result(%d)", ret); 

			/* Remove client */
			sttd_server_finalize(uid);
		}	
	}

	SLOG(LOG_DEBUG, TAG_STTD, "=====");
	SLOG(LOG_DEBUG, TAG_STTD, "  ");

	return EINA_FALSE;
}

int audio_recorder_callback(const void* data, const unsigned int length)
{
	if (0 != sttd_engine_recognize_audio(data, length)) {
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

		ecore_timer_add(0, __stop_by_silence, NULL);
		
		return -1;
	}

	return 0;
}

void sttd_server_recognition_result_callback(sttp_result_event_e event, const char* type, 
					const char** data, int data_count, const char* msg, void *user_data)
{
	SLOG(LOG_DEBUG, TAG_STTD, "===== Recognition Result Callback");

	if (NULL == user_data) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] user data is NULL"); 
		SLOG(LOG_DEBUG, TAG_STTD, "=====");
		SLOG(LOG_DEBUG, TAG_STTD, "  ");
		return;
	}

	/* check uid */
	int *uid = (int*)user_data;

	SLOG(LOG_DEBUG, TAG_STTD, "[Server] uid (%d), event(%d)", *uid, event); 

	app_state_e state;
	if (0 != sttd_client_get_state(*uid, &state)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] uid is NOT valid "); 
		SLOG(LOG_DEBUG, TAG_STTD, "=====");
		SLOG(LOG_DEBUG, TAG_STTD, "  ");
		return;
	}

	/* Delete timer for processing time out */
	Ecore_Timer* timer;
	sttd_cliet_get_timer(*uid, &timer);

	if (NULL != timer)
		ecore_timer_del(timer);

	/* send result to client */
	if (STTP_RESULT_EVENT_SUCCESS == event && 0 < data_count && NULL != data) {

		if (APP_STATE_PROCESSING == state ) {
			SLOG(LOG_DEBUG, TAG_STTD, "[Server] the size of result from engine is '%d' %s", data_count); 

			if (0 != sttdc_send_result(*uid, type, data, data_count, msg)) {
				SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to send result"); 	
				int reason = (int)STTD_ERROR_OPERATION_FAILED;

				if (0 != sttdc_send_error_signal(*uid, reason, "Fail to send recognition result")) {
					SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to send error info . Remove client data"); 
				}
			}
		} else {
			SLOG(LOG_WARN, TAG_STTD, "[Server WARNING] Current state is NOT thinking."); 
		}
	} else if (STTP_RESULT_EVENT_NO_RESULT == event || STTP_RESULT_EVENT_ERROR == event) {

		if (APP_STATE_PROCESSING == state ) {
			if (0 != sttdc_send_result(*uid, type, NULL, 0, msg)) {
				SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to send result "); 

				/* send error msg */
				int reason = (int)STTD_ERROR_INVALID_STATE;	
				if (0 != sttdc_send_error_signal(*uid, reason, "[ERROR] Fail to send recognition result")) {
					SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to send error info "); 
				}
			}
		} else {
			SLOG(LOG_WARN, TAG_STTD, "[Server ERROR] Current state is NOT thinking."); 
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

	ecore_timer_add(0, __stop_by_silence, NULL);

	SLOG(LOG_DEBUG, TAG_STTD, "=====");
	SLOG(LOG_DEBUG, TAG_STTD, "  ");

	return;
}

/*
* Daemon function
*/

int sttd_initialize()
{
	int ret = 0;

	if (sttd_config_initialize()) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server WARNING] Fail to initialize config.");
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

int sttd_finalize()
{
	sttd_config_finalize();

	sttd_engine_agent_release();

	return STTD_ERROR_NONE;
}

Eina_Bool sttd_cleanup_client(void *data)
{
	int* client_list = NULL;
	int client_count = 0;

	if (0 != sttd_client_get_list(&client_list, &client_count)) 
		return EINA_TRUE;
	
	if (NULL == client_list)
		return EINA_TRUE;

	int result;
	int i = 0;

	SLOG(LOG_DEBUG, TAG_STTD, "===== Clean up client ");

	for (i = 0;i < client_count;i++) {
		result = sttdc_send_hello(client_list[i]);

		if (0 == result) {
			SLOG(LOG_DEBUG, TAG_STTD, "[Server] uid(%d) should be removed.", client_list[i]); 
			sttd_server_finalize(client_list[i]);
		} else if (-1 == result) {
			SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Hello result has error"); 
		}
	}

	SLOG(LOG_DEBUG, TAG_STTD, "=====");
	SLOG(LOG_DEBUG, TAG_STTD, "  ");

	free(client_list);

	return EINA_TRUE;
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

		/* set type, channel, sample rate */
		sttp_audio_type_e atype;
		int rate;
		int channels;

		if (0 != sttd_engine_get_audio_format(&atype, &rate, &channels)) {
			SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to get audio format of engine."); 
			return STTD_ERROR_OPERATION_FAILED;
		}

		if (0 != sttd_recorder_create(audio_recorder_callback, atype, channels, rate)) {
			SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to set recorder"); 
			return STTD_ERROR_OPERATION_FAILED;
		}

		SLOG(LOG_DEBUG, TAG_STTD, "[Server] audio type(%d), channel(%d)", (int)atype, (int)channels);
	}

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

static Eina_Bool __quit_ecore_loop(void *data)
{
	ecore_main_loop_quit();
	SLOG(LOG_DEBUG, TAG_STTD, "[Server] quit ecore main loop");
	return EINA_FALSE;
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
	if (APP_STATE_RECORDING == state || APP_STATE_PROCESSING == state) {
		sttd_recorder_stop();
		sttd_engine_recognize_cancel();
	}
	
	/* Remove client information */
	if (0 != sttd_client_delete(uid)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to delete client"); 
	}

	/* unload engine, if ref count of client is 0 */
	if (0 == sttd_client_get_ref_count()) {
		sttd_recorder_destroy();

		ecore_timer_add(0, __quit_ecore_loop, NULL);
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

Eina_Bool __check_recording_state(void *data)
{	
	/* current uid */
	int uid = sttd_client_get_current_recording();
	if (-1 == uid)
		return EINA_FALSE;

	app_state_e state;
	if (0 != sttdc_send_get_state(uid, (int*)&state)) {
		/* client is removed */
		SLOG(LOG_DEBUG, TAG_STTD, "[Server] uid(%d) should be removed.", uid); 
		sttd_server_finalize(uid);
		return EINA_FALSE;
	}

	if (APP_STATE_READY == state) {
		/* Cancel stt */
		SLOG(LOG_DEBUG, TAG_STTD, "[Server] The state of uid(%d) is 'Ready'. The daemon should cancel recording", uid); 
		sttd_server_cancel(uid);
	} else if (APP_STATE_PROCESSING == state) {
		/* Cancel stt and send change state */
		SLOG(LOG_DEBUG, TAG_STTD, "[Server] The state of uid(%d) is 'Processing'. The daemon should cancel recording", uid); 
		sttd_server_cancel(uid);
		sttdc_send_set_state(uid, (int)APP_STATE_READY);
	} else {
		/* Normal state */
		SLOG(LOG_DEBUG, TAG_STTD, "[Server] The states of daemon and client are identical"); 
		return EINA_TRUE;
	}

	return EINA_FALSE;
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

	Ecore_Timer* timer = ecore_timer_add(g_state_check_time, __check_recording_state, NULL);
	sttd_cliet_set_timer(uid, timer);

	return STTD_ERROR_NONE;
}

Eina_Bool __time_out_for_processing(void *data)
{	
	/* current uid */
	int uid = sttd_client_get_current_thinking();
	if (-1 == uid)
		return EINA_FALSE;

	/* Cancel engine */
	int ret = sttd_engine_recognize_cancel();
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to cancel : result(%d)", ret); 
	}

	if (0 != sttdc_send_result(uid, STTP_RECOGNITION_TYPE_FREE, NULL, 0, "Time out not to receive recognition result.")) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to send result "); 

		/* send error msg */
		int reason = (int)STTD_ERROR_TIMED_OUT;	
		if (0 != sttdc_send_error_signal(uid, reason, "[ERROR] Fail to send recognition result")) {
			SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to send error info "); 
		}
	}

	/* Change uid state */
	sttd_client_set_state(uid, APP_STATE_READY);

	return EINA_FALSE;
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
	
	Ecore_Timer* timer;
	sttd_cliet_get_timer(uid, &timer);

	if (NULL != timer)
		ecore_timer_del(timer);

	/* change uid state */
	sttd_client_set_state(uid, APP_STATE_PROCESSING);

	timer = ecore_timer_add(g_state_check_time, __time_out_for_processing, NULL);
	sttd_cliet_set_timer(uid, timer);

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
		sttd_recorder_stop();

	/* cancel engine recognition */
	int ret = sttd_engine_recognize_cancel();
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to cancel : result(%d)", ret); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	Ecore_Timer* timer;
	sttd_cliet_get_timer(uid, &timer);
	ecore_timer_del(timer);

	/* change uid state */
	sttd_client_set_state(uid, APP_STATE_READY);

	return STTD_ERROR_NONE;
}


/******************************************************************************************
* STT Server Functions for setting
*******************************************************************************************/

int sttd_server_setting_initialize(int pid)
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

	/* check whether pid is valid */
	if (true == sttd_setting_client_is(pid)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Setting pid is NOT valid "); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	/* load if engine is unloaded */
	if (0 == sttd_client_get_ref_count()) {
		if (0 != sttd_engine_agent_load_current_engine()) {
			SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to load current engine"); 
			return STTD_ERROR_OPERATION_FAILED;
		}

		/* set type, channel, sample rate */
		sttp_audio_type_e atype;
		int rate;
		int channels;

		if (0 != sttd_engine_get_audio_format(&atype, &rate, &channels)) {
			SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to get audio format of engine."); 
			return STTD_ERROR_OPERATION_FAILED;
		}

		if (0 != sttd_recorder_create(audio_recorder_callback, atype, channels, rate)) {
			SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to set recorder"); 
			return STTD_ERROR_OPERATION_FAILED;
		}

		SLOG(LOG_DEBUG, TAG_STTD, "[Server] audio type(%d), channel(%d)", (int)atype, (int)channels);
	}

	/* Add setting client information to client manager (For internal use) */
	if (0 != sttd_setting_client_add(pid)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to add setting client"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	return STTD_ERROR_NONE;
}

int sttd_server_setting_finalize(int pid)
{
	/* Remove client information */
	if (0 != sttd_setting_client_delete(pid)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to delete setting client"); 
	}

	/* unload engine, if ref count of client is 0 */
	if (0 == sttd_client_get_ref_count()) {
		sttd_recorder_destroy();

		ecore_timer_add(0, __quit_ecore_loop, NULL);
	}

	return STTD_ERROR_NONE;
}

int sttd_server_setting_get_engine_list(int pid, GList** engine_list)
{
	/* check whether pid is valid */
	if (true != sttd_setting_client_is(pid)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Setting pid is NOT valid "); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (NULL == engine_list) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Input parameter is NULL"); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	int ret = sttd_engine_setting_get_engine_list(engine_list); 
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to get engine list : result(%d)", ret); 
		return ret;
	}

	return STTD_ERROR_NONE;
}

int sttd_server_setting_get_engine(int pid, char** engine_id)
{
	/* check whether pid is valid */
	if (true != sttd_setting_client_is(pid)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Setting pid is NOT valid "); 
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
		return ret;
	}

	return STTD_ERROR_NONE;
}

int sttd_server_setting_set_engine(int pid, const char* engine_id)
{
	/* check whether pid is valid */
	if (true != sttd_setting_client_is(pid)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Setting pid is NOT valid "); 
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
		return ret;
	}

	return STTD_ERROR_NONE;
}

int sttd_server_setting_get_lang_list(int pid, char** engine_id, GList** lang_list)
{
	/* check whether pid is valid */
	if (true != sttd_setting_client_is(pid)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Setting pid is NOT valid "); 
		return STTD_ERROR_INVALID_PARAMETER;
	}
	
	if (NULL == lang_list) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] language is NULL"); 
		return STTD_ERROR_INVALID_PARAMETER;
	}
	
	int ret = sttd_engine_setting_get_lang_list(engine_id, lang_list); 
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to get language list : result(%d)", ret); 
		return ret;
	}

	return STTD_ERROR_NONE;
}

int sttd_server_setting_get_default_language(int pid, char** language)
{
	/* check whether pid is valid */
	if (true != sttd_setting_client_is(pid)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Setting pid is NOT valid "); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (NULL == language) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] language is NULL"); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	int ret = sttd_engine_setting_get_default_lang(language); 
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to get default language : result(%d)", ret); 
		return ret;
	}	

	return STTD_ERROR_NONE;
}

int sttd_server_setting_set_default_language(int pid, const char* language)
{
	/* check whether pid is valid */
	if (true != sttd_setting_client_is(pid)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Setting pid is NOT valid "); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (NULL == language) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] language is NULL"); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	int ret = sttd_engine_setting_set_default_lang((char*)language); 
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to set default lang : result(%d)", ret); 
		return ret;
	}	

	return STTD_ERROR_NONE;
}

int sttd_server_setting_get_profanity_filter(int pid, bool* value)
{
	/* check whether pid is valid */
	if (true != sttd_setting_client_is(pid)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Setting pid is NOT valid "); 
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
		return ret;
	}	

	return STTD_ERROR_NONE;
}

int sttd_server_setting_set_profanity_filter(int pid, bool value)
{
	/* check whether pid is valid */
	if (true != sttd_setting_client_is(pid)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Setting pid is NOT valid "); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	int ret = 0;
	ret = sttd_engine_setting_set_profanity_filter(value);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to set profanity filter: result(%d)", ret); 
		return ret;
	}	

	return STTD_ERROR_NONE;
}

int sttd_server_setting_get_punctuation_override(int pid, bool* value)
{
	/* check whether pid is valid */
	if (true != sttd_setting_client_is(pid)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Setting pid is NOT valid "); 
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
		return ret;
	}	

	return STTD_ERROR_NONE;
}

int sttd_server_setting_set_punctuation_override(int pid, bool value)
{
	/* check whether pid is valid */
	if (true != sttd_setting_client_is(pid)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Setting pid is NOT valid "); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	int ret = 0;
	ret = sttd_engine_setting_set_punctuation_override(value);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to set punctuation override : result(%d)", ret); 
		return ret;
	}	

	return STTD_ERROR_NONE;
}

int sttd_server_setting_get_silence_detection(int pid, bool* value)
{
	/* check whether pid is valid */
	if (true != sttd_setting_client_is(pid)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Setting pid is NOT valid "); 
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
		return ret;
	}	

	return STTD_ERROR_NONE;
}

int sttd_server_setting_set_silence_detection(int pid, bool value)
{
	/* check whether pid is valid */
	if (true != sttd_setting_client_is(pid)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Setting pid is NOT valid "); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	int ret = 0;
	ret = sttd_engine_setting_set_silence_detection(value);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to Result(%d)", ret); 
		return ret;
	}	

	return STTD_ERROR_NONE;
}

int sttd_server_setting_get_engine_setting(int pid, char** engine_id, GList** lang_list)
{	
	/* check whether pid is valid */
	if (true != sttd_setting_client_is(pid)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Setting pid is NOT valid "); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (0 != sttd_engine_setting_get_engine_setting_info(engine_id, lang_list)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to get engine setting info"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	return STTD_ERROR_NONE;
}

int sttd_server_setting_set_engine_setting(int pid, const char* key, const char* value)
{	
	/* check whether pid is valid */
	if (true != sttd_setting_client_is(pid)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Setting pid is NOT valid "); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (0 != sttd_engine_setting_set_engine_setting(key, value)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to set engine setting info"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	return STTD_ERROR_NONE;
}
