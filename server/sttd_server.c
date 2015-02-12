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

#include <sound_manager.h>
#include <wav_player.h>

#include "stt_network.h"
#include "sttd_client_data.h"
#include "sttd_config.h"
#include "sttd_dbus.h"
#include "sttd_engine_agent.h"
#include "sttd_main.h"
#include "sttd_recorder.h"
#include "sttd_server.h"


/*
* STT Server static variable
*/
static double g_processing_timeout = 30;

static double g_recording_timeout = 60;

Ecore_Timer*	g_recording_timer = NULL;
Ecore_Timer*	g_processing_timer = NULL;

static Eina_Bool g_stt_daemon_exist = EINA_TRUE;

static int g_recording_log_count = 0;

/*
* STT Server Callback Functions
*/

Eina_Bool sttd_get_daemon_exist()
{
	return g_stt_daemon_exist;
}

Eina_Bool __stop_by_silence(void *data)
{
	SLOG(LOG_DEBUG, TAG_STTD, "===== Stop by silence detection");

	int uid = 0;

	uid = stt_client_get_current_recognition();

	int ret;
	if (0 != uid) {
		ret = sttd_server_stop(uid); 
		if (0 > ret) {
			return EINA_FALSE;
		}
	
		if (STTD_RESULT_STATE_DONE == ret) {
			ret = sttdc_send_set_state(uid, (int)APP_STATE_PROCESSING);
			if (0 != ret) {
				SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to send state : result(%d)", ret);

				/* Remove client */
				sttd_server_finalize(uid);
				stt_client_unset_current_recognition();
			}
		}
	} else {
		SLOG(LOG_WARN, TAG_STTD, "[Server WARNING] uid is NOT valid");
	}

	SLOG(LOG_DEBUG, TAG_STTD, "=====");
	SLOG(LOG_DEBUG, TAG_STTD, "  ");

	return EINA_FALSE;
}

static void __cancel_recognition_internal()
{
	if (NULL != g_recording_timer)	{
		ecore_timer_del(g_recording_timer);
		g_recording_timer = NULL;
	}

	int uid = 0;
	uid = stt_client_get_current_recognition();

	if (0 != uid) {
		/* cancel engine recognition */
		int ret = sttd_engine_agent_recognize_cancel(uid);
		if (0 != ret) {
			SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to cancel : result(%d)", ret);
		}

		/* change uid state */
		sttd_client_set_state(uid, APP_STATE_READY);
		stt_client_unset_current_recognition();

		ret = sttdc_send_set_state(uid, (int)APP_STATE_READY);
		if (0 != ret) {
			SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to send state change : result(%d)", ret);
		}
	} else {
		SLOG(LOG_WARN, TAG_STTD, "[Server WARNING] uid is NOT valid");
	}
}

Eina_Bool __cancel_by_error(void *data)
{
	SLOG(LOG_DEBUG, TAG_STTD, "===== Cancel by error");

	__cancel_recognition_internal();

	SLOG(LOG_DEBUG, TAG_STTD, "=====");
	SLOG(LOG_DEBUG, TAG_STTD, "  ");

	return EINA_FALSE;
}

int __server_audio_recorder_callback(const void* data, const unsigned int length)
{
	int uid = -1;
	int ret;

	if (NULL == data || 0 == length) {
		SLOG(LOG_WARN, TAG_STTD, "[Server WARNING] Recording data is not valid");
		ecore_timer_add(0, __cancel_by_error, NULL);
		return -1;
	}

	uid = stt_client_get_current_recognition();
	if (0 != uid) {
		ret = sttd_engine_agent_set_recording_data(uid, data, length);
		if (ret < 0) {
			 ecore_timer_add(0, __cancel_by_error, NULL);
			 return -1;
		}
		g_recording_log_count++;
		if (200 <= g_recording_log_count) {
			SECURE_SLOG(LOG_DEBUG, TAG_STTD, "=== Set recording data ===");
			g_recording_log_count = 0;
		}
	} else {
		if (NULL != g_recording_timer)	{
			ecore_timer_del(g_recording_timer);
			g_recording_timer = NULL;
		}
		SLOG(LOG_WARN, TAG_STTD, "[Server WARNING] Current uid in recording is is not valid");
		return -1;
	}

	return 0;
}

void __server_audio_interrupt_callback()
{
	SLOG(LOG_DEBUG, TAG_STTD, "===== Cancel by sound interrupt");

	__cancel_recognition_internal();

	SLOG(LOG_DEBUG, TAG_STTD, "=====");
	SLOG(LOG_DEBUG, TAG_STTD, "  ");
}

Eina_Bool __cancel_by_no_record(void *data)
{
	SLOG(LOG_DEBUG, TAG_STTD, "===== Cancel by no record");

	__cancel_recognition_internal();

	SLOG(LOG_DEBUG, TAG_STTD, "=====");
	SLOG(LOG_DEBUG, TAG_STTD, "  ");
	
	return EINA_FALSE;
}

void __server_recognition_result_callback(sttp_result_event_e event, const char* type,
					const char** data, int data_count, const char* msg, void *user_data)
{
	SLOG(LOG_DEBUG, TAG_STTD, "===== Recognition Result Callback");

	/* check uid */
	int uid = stt_client_get_current_recognition();

	app_state_e state;
	if (0 == uid || 0 != sttd_client_get_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] uid is NOT valid ");
		SLOG(LOG_DEBUG, TAG_STTD, "=====");
		SLOG(LOG_DEBUG, TAG_STTD, "  ");
		return;
	}

	SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[Server] uid (%d), event(%d)", uid, event);

	/* send result to client */
	if (STTP_RESULT_EVENT_FINAL_RESULT == event) {
		if (APP_STATE_PROCESSING != state ) {
			SLOG(LOG_WARN, TAG_STTD, "[Server WARNING] Current state is NOT 'Processing'.");
		}
		SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[Server] the size of result from engine is '%d'", data_count);

		/* Delete timer for processing time out */
		if (NULL != g_processing_timer)	{
			ecore_timer_del(g_processing_timer);
			g_processing_timer = NULL;
		}

		sttd_config_time_save();
		sttd_config_time_reset();

		if (NULL == data || 0 == data_count) {
			if (0 != sttdc_send_result(uid, event, NULL, 0, msg)) {
				SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to send result");	
				int reason = (int)STTD_ERROR_OPERATION_FAILED;

				if (0 != sttdc_send_error_signal(uid, reason, "Fail to send recognition result")) {
					SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to send error info . Remove client data");
				}
			}
		} else {
			if (0 != sttdc_send_result(uid, event, data, data_count, msg)) {
				SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to send result");	
				int reason = (int)STTD_ERROR_OPERATION_FAILED;

				if (0 != sttdc_send_error_signal(uid, reason, "Fail to send recognition result")) {
					SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to send error info . Remove client data");
				}
			}
		}

		/* change state of uid */
		sttd_client_set_state(uid, APP_STATE_READY);
		stt_client_unset_current_recognition();

	} else if (STTP_RESULT_EVENT_PARTIAL_RESULT == event) {
		SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[Server] The partial result from engine is event[%d] data_count[%d]", event,  data_count);

		sttd_config_time_save();
		sttd_config_time_reset();

		if (0 != sttdc_send_result(uid, event, data, data_count, msg)) {
			SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to send result");
			int reason = (int)STTD_ERROR_OPERATION_FAILED;

			if (0 != sttdc_send_error_signal(uid, reason, "Fail to send recognition result")) {
				SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to send error info . Remove client data");
			}
		}

	} else if (STTP_RESULT_EVENT_ERROR == event) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] The event of recognition result is ERROR");

		/* Delete timer for processing time out */
		if (NULL != g_processing_timer)	{
			ecore_timer_del(g_processing_timer);
			g_processing_timer = NULL;
		}
		sttd_config_time_reset();

		if (0 != sttdc_send_result(uid, event, NULL, 0, msg)) {
			SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to send result ");

			/* send error msg */
			int reason = (int)STTD_ERROR_INVALID_STATE;	
			if (0 != sttdc_send_error_signal(uid, reason, "[ERROR] Fail to send recognition result")) {
				SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to send error info ");
			}
		}

		/* change state of uid */
		sttd_client_set_state(uid, APP_STATE_READY);
		stt_client_unset_current_recognition();
	} else {
		/* nothing */
	}

	SLOG(LOG_DEBUG, TAG_STTD, "=====");
	SLOG(LOG_DEBUG, TAG_STTD, "  ");

	return;
}

bool __server_result_time_callback(int index, sttp_result_time_event_e event, const char* text, long start_time, long end_time, void* user_data)
{
	SLOG(LOG_DEBUG, TAG_STTD, "[Server] index(%d) event(%d) text(%s) start(%ld) end(%ld)", 
		index, event, text, start_time, end_time);

	if (0 == index) {
		int ret;
		ret = sttd_config_time_add(index, (int)event, text, start_time, end_time);
		if (0 != ret) {
			SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to add time info");
			return false;
		}
	} else {
		return false;
	}

	return true;
}

void __server_silence_dectection_callback(sttp_silence_type_e type, void *user_param)
{
	SLOG(LOG_DEBUG, TAG_STTD, "===== Silence Detection Callback");

	int uid = stt_client_get_current_recognition();
	if (0 != uid) {
		app_state_e state;
		if (0 != sttd_client_get_state(uid, &state)) {
			SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] uid is not valid ");
			return;
		}

		if (APP_STATE_RECORDING != state) {
			SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Current state is not recording");
			return;
		}

		if (STTP_SILENCE_TYPE_NO_RECORD_TIMEOUT == type) {
			SECURE_SLOG(LOG_DEBUG, TAG_STTD, "Silence Detection type - No Record");
			ecore_timer_add(0, __cancel_by_no_record, NULL);
		} else if (STTP_SILENCE_TYPE_END_OF_SPEECH_DETECTED == type) {
			SECURE_SLOG(LOG_DEBUG, TAG_STTD, "Silence Detection type - End of Speech");
			ecore_timer_add(0, __stop_by_silence, NULL);
		}
	} else {
		SLOG(LOG_WARN, TAG_STTD, "[Server WARNING] Current recogntion uid is not valid ");
	}

	SLOG(LOG_DEBUG, TAG_STTD, "=====");
	SLOG(LOG_DEBUG, TAG_STTD, "  ");

	return;
}

void __sttd_server_engine_changed_cb(const char* engine_id, const char* language, bool support_silence, void* user_data)
{
	if (NULL == engine_id) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Engine id is NULL");
		return;
	} else {
		SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[Server] New default engine : %s", engine_id);
	}

	/* need to change state of app to ready */
	int uid;
	uid = stt_client_get_current_recognition();

	if (0 != uid) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Server] Set ready state of uid(%d)", uid);

		sttd_server_cancel(uid);
		sttdc_send_set_state(uid, (int)APP_STATE_READY);

		stt_client_unset_current_recognition();
	}

	/* set engine */
	int ret = sttd_engine_agent_set_default_engine(engine_id);
	if (0 != ret)
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to set engine : result(%d)", ret);

	if (NULL != language) {
		ret = sttd_engine_agent_set_default_language(language);
		if (0 != ret)
			SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to set default lang : result(%d)", ret);
	}
	
	ret = sttd_engine_agent_set_silence_detection(support_silence);
	if (0 != ret)
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to Result(%d)", ret);

	return;
}

void __sttd_server_language_changed_cb(const char* language, void* user_data)
{
	if (NULL == language) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] language is NULL");
		return;
	} else {
		SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[Server] Get language changed : %s", language);
	}

	int ret = sttd_engine_agent_set_default_language(language);
	if (0 != ret)
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to set default lang : result(%d)", ret);

	return;
}

void __sttd_server_silence_changed_cb(bool value, void* user_data)
{
	SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[Server] Get silence detection changed : %s", value ? "on" : "off");

	int ret = 0;
	ret = sttd_engine_agent_set_silence_detection(value);
	if (0 != ret)
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to Result(%d)", ret);

	return;
}

/*
* Daemon function
*/

int sttd_initialize()
{
	int ret = 0;

	if (sttd_config_initialize(__sttd_server_engine_changed_cb, __sttd_server_language_changed_cb, 
		__sttd_server_silence_changed_cb, NULL)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to initialize config.");
	}

	ret = sttd_recorder_initialize(__server_audio_recorder_callback, __server_audio_interrupt_callback);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to initialize recorder : result(%d)", ret);
	}

	/* Engine Agent initialize */
	ret = sttd_engine_agent_init(__server_recognition_result_callback, __server_result_time_callback,
		__server_silence_dectection_callback);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to engine agent initialize : result(%d)", ret);
		return ret;
	}
	
	/* Update engine list */
	ret = sttd_engine_agent_initialize_engine_list();
	if (0 != ret) {
		if (STTD_ERROR_ENGINE_NOT_FOUND == ret) {
			SLOG(LOG_WARN, TAG_STTD, "[Server WARNING] There is no stt engine");
		} else {
			SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to update engine list : %d", ret);
			return ret;
		}
	}

	SLOG(LOG_DEBUG, TAG_STTD, "[Server SUCCESS] initialize");

	return 0;
}

int sttd_finalize()
{
	sttd_recorder_deinitialize();

	sttd_config_finalize();

	sttd_engine_agent_release();

	return STTD_ERROR_NONE;
}

Eina_Bool sttd_cleanup_client(void *data)
{
	int* client_list = NULL;
	int client_count = 0;
	int result;
	int i = 0;

	if (0 != sttd_client_get_list(&client_list, &client_count)) {
		if (NULL != client_list)
			free(client_list);

		return EINA_TRUE;
	}

	if (NULL != client_list) {
		SLOG(LOG_DEBUG, TAG_STTD, "===== Clean up client ");

		for (i = 0;i < client_count;i++) {
			result = sttdc_send_hello(client_list[i]);

			if (0 == result) {
				SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[Server] uid(%d) should be removed.", client_list[i]);
				sttd_server_finalize(client_list[i]);
			} else if (-1 == result) {
				SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Hello result has error");
			}
		}

		SLOG(LOG_DEBUG, TAG_STTD, "=====");
		SLOG(LOG_DEBUG, TAG_STTD, "  ");

		free(client_list);
	}

	return EINA_TRUE;
}

/*
* STT Server Functions for Client
*/

int sttd_server_initialize(int pid, int uid, bool* silence)
{
	if (false == sttd_engine_agent_is_default_engine()) {
		/* Update installed engine */
		sttd_engine_agent_initialize_engine_list();
		
		if (false == sttd_engine_agent_is_default_engine()) {
			SLOG(LOG_WARN, TAG_STTD, "[Server WARNING] No stt-engine");
			return STTD_ERROR_ENGINE_NOT_FOUND;
		}
	}

	/* check if uid is valid */
	app_state_e state;
	if (0 == sttd_client_get_state(uid, &state)) {
		SLOG(LOG_WARN, TAG_STTD, "[Server WARNING] uid has already been registered");
		return STTD_ERROR_NONE;
	}

	/* load engine */
	if (0 != sttd_engine_agent_load_current_engine(uid, NULL)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to load default engine");
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (0 != sttd_engine_agent_get_option_supported(uid, silence)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to get engine options supported");
		return STTD_ERROR_OPERATION_FAILED;
	}

	/* Add client information to client manager */
	if (0 != sttd_client_add(pid, uid)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to add client info");
		return STTD_ERROR_OPERATION_FAILED;
	}

	return STTD_ERROR_NONE;
}

static Eina_Bool __quit_ecore_loop(void *data)
{
	ecore_main_loop_quit();
	return EINA_FALSE;
}

int sttd_server_finalize(int uid)
{
	/* check if uid is valid */
	app_state_e state;
	if (0 != sttd_client_get_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] uid is NOT valid ");
		return STTD_ERROR_INVALID_PARAMETER;
	}

	/* release recorder */
	if (APP_STATE_RECORDING == state || APP_STATE_PROCESSING == state) {
		if (APP_STATE_RECORDING == state) {
			if (NULL != g_recording_timer)	{
				ecore_timer_del(g_recording_timer);
				g_recording_timer = NULL;
			}
		}

		if (APP_STATE_PROCESSING == state) {
			if (NULL != g_processing_timer) {
				ecore_timer_del(g_processing_timer);
				g_processing_timer = NULL;
			}
		}

		if (0 != sttd_engine_agent_recognize_cancel(uid)) {
			SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to cancel recognition");
		}

		stt_client_unset_current_recognition();
	}
	
	if (0 != sttd_engine_agent_unload_current_engine(uid)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to unload engine");
	}

	/* Remove client information */
	if (0 != sttd_client_delete(uid)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to delete client");
	}

	/* unload engine, if ref count of client is 0 */
	if (0 == sttd_client_get_ref_count()) {
		g_stt_daemon_exist = EINA_FALSE;
		ecore_timer_add(0, __quit_ecore_loop, NULL);
	}
	
	return STTD_ERROR_NONE;
}

int sttd_server_get_supported_engines(int uid, GSList** engine_list)
{
	/* Check if uid is valid */
	app_state_e state;
	if (0 != sttd_client_get_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] uid is NOT valid ");
		return STTD_ERROR_INVALID_PARAMETER;
	}

	/* Check state of uid */
	if (APP_STATE_READY != state) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] The state of uid(%d) is not Ready", uid);
		return STTD_ERROR_INVALID_STATE;
	}

	int ret;
	ret = sttd_engine_agent_get_engine_list(engine_list);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to get engine list");
		return STTD_ERROR_OPERATION_FAILED;
	}

	return STTD_ERROR_NONE;
}

int sttd_server_set_current_engine(int uid, const char* engine_id, bool* silence)
{
	/* Check if uid is valid */
	app_state_e state;
	if (0 != sttd_client_get_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] uid is NOT valid ");
		return STTD_ERROR_INVALID_PARAMETER;
	}

	/* Check state of uid */
	if (APP_STATE_READY != state) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] The state of uid(%d) is not Ready", uid);
		return STTD_ERROR_INVALID_STATE;
	}

	int ret;
	ret = sttd_engine_agent_load_current_engine(uid, engine_id);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to set engine : %d", ret);
		return STTD_ERROR_OPERATION_FAILED;
	}

	ret = sttd_engine_agent_get_option_supported(uid, silence);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to engine options : %d", ret);
		return STTD_ERROR_OPERATION_FAILED;
	}

	return STTD_ERROR_NONE;
}

int sttd_server_get_current_engine(int uid, char** engine_id)
{
	/* Check if uid is valid */
	app_state_e state;
	if (0 != sttd_client_get_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] uid is NOT valid ");
		return STTD_ERROR_INVALID_PARAMETER;
	}

	/* Check state of uid */
	if (APP_STATE_READY != state) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] The state of uid(%d) is not Ready", uid);
		return STTD_ERROR_INVALID_STATE;
	}
	
	int ret;
	ret = sttd_engine_agent_get_current_engine(uid, engine_id);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to get engine : %d", ret);
		return STTD_ERROR_OPERATION_FAILED;
	}

	return STTD_ERROR_NONE;
}

int sttd_server_check_agg_agreed(int uid, const char* appid, bool* available)
{
	/* Check if uid is valid */
	app_state_e state;
	if (0 != sttd_client_get_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] uid is NOT valid "); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	/* Check state of uid */
	if (APP_STATE_READY != state) {
		SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] The state of uid(%d) is not Ready", uid);
		return STTD_ERROR_INVALID_STATE;
	}

	/* Ask engine available */
	int ret;
	bool temp = false;
	ret = sttd_engine_agent_check_app_agreed(uid, appid, &temp);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to get engine available : %d", ret);
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (true == temp) {
		stt_client_set_app_agreed(uid);
		SLOG(LOG_DEBUG, TAG_STTD, "[Server] App(%s) confirmed that engine is available", appid);
	}

	*available = temp;

	return STTD_ERROR_NONE;
}


int sttd_server_get_supported_languages(int uid, GSList** lang_list)
{
	/* check if uid is valid */
	app_state_e state;
	if (0 != sttd_client_get_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] uid is NOT valid ");
		return STTD_ERROR_INVALID_PARAMETER;
	}

	/* get language list from engine */
	int ret = sttd_engine_agent_supported_langs(uid, lang_list);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to get supported languages");
		return STTD_ERROR_OPERATION_FAILED;
	}

	SLOG(LOG_DEBUG, TAG_STTD, "[Server SUCCESS] sttd_server_get_supported_languages");

	return STTD_ERROR_NONE;
}

int sttd_server_get_current_langauage(int uid, char** current_lang)
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
	int ret = sttd_engine_agent_get_default_lang(uid, current_lang);
	if (0 != ret) {	
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to get default language :result(%d)", ret);
		return STTD_ERROR_OPERATION_FAILED;
	}

	SLOG(LOG_DEBUG, TAG_STTD, "[Server SUCCESS] Get default language");

	return STTD_ERROR_NONE;
}

int sttd_server_is_recognition_type_supported(int uid, const char* type, int* support)
{
	/* check if uid is valid */
	app_state_e state;
	if (0 != sttd_client_get_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] uid is NOT valid ");
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (NULL == type || NULL == support) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Input parameter is NULL");
		return STTD_ERROR_INVALID_PARAMETER;
	}

	bool temp;
	int ret = sttd_engine_agent_is_recognition_type_supported(uid, type, &temp);
	if (0 != ret) {	
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to get recognition type supported : result(%d)", ret);
		return STTD_ERROR_OPERATION_FAILED;
	}
	
	*support = (int)temp;

	SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[Server SUCCESS] recognition type supporting is %s", *support ? "true" : "false");

	return STTD_ERROR_NONE;
}

int sttd_server_set_start_sound(int uid, const char* file)
{
	/* check if uid is valid */
	app_state_e state;
	if (0 != sttd_client_get_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] uid is NOT valid ");
		return STTD_ERROR_INVALID_PARAMETER;
	}

	int ret = sttd_client_set_start_sound(uid, file);
	if (0 != ret) {	
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to set start sound file : result(%d)", ret);
		return STTD_ERROR_OPERATION_FAILED;
	}

	return 0;
}

int sttd_server_set_stop_sound(int uid, const char* file)
{
	/* check if uid is valid */
	app_state_e state;
	if (0 != sttd_client_get_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] uid is NOT valid ");
		return STTD_ERROR_INVALID_PARAMETER;
	}

	int ret = sttd_client_set_stop_sound(uid, file);
	if (0 != ret) {	
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to set start sound file : result(%d)", ret);
		return STTD_ERROR_OPERATION_FAILED;
	}

	return 0;
}

Eina_Bool __check_recording_state(void *data)
{	
	/* current uid */
	int uid = stt_client_get_current_recognition();
	if (0 == uid)
		return EINA_FALSE;

	app_state_e state;
	if (0 != sttdc_send_get_state(uid, (int*)&state)) {
		/* client is removed */
		SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[Server] uid(%d) should be removed.", uid);
		sttd_server_finalize(uid);
		return EINA_FALSE;
	}

	if (APP_STATE_READY == state) {
		/* Cancel stt */
		SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[Server] The state of uid(%d) is 'Ready'. The daemon should cancel recording", uid);
		sttd_server_cancel(uid);
	} else if (APP_STATE_PROCESSING == state) {
		/* Cancel stt and send change state */
		SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[Server] The state of uid(%d) is 'Processing'. The daemon should cancel recording", uid);
		sttd_server_cancel(uid);
		sttdc_send_set_state(uid, (int)APP_STATE_READY);
	} else {
		/* Normal state */
		SLOG(LOG_DEBUG, TAG_STTD, "[Server] The states of daemon and client are identical");
		return EINA_TRUE;
	}

	return EINA_FALSE;
}

Eina_Bool __stop_by_recording_timeout(void *data)
{
	SLOG(LOG_DEBUG, TAG_STTD, "===== Stop by timeout");

	g_recording_timer = NULL;

	int uid = 0;
	uid = stt_client_get_current_recognition();
	if (0 != uid) {
		/* cancel engine recognition */
		int ret = sttd_server_stop(uid);
		if (0 != ret) {
			SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to stop : result(%d)", ret);
		}
	}

	SLOG(LOG_DEBUG, TAG_STTD, "=====");
	SLOG(LOG_DEBUG, TAG_STTD, "  ");

	return EINA_FALSE;
}

void __sttd_server_recorder_start(int uid)
{
	int current_uid = stt_client_get_current_recognition();

	if (uid != current_uid) {
		stt_client_unset_current_recognition();
		return;
	}

	int ret = sttd_engine_agent_recognize_start_recorder(uid);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to start recognition : result(%d)", ret);
		return;
	}

	/* Notify uid state change */
	sttdc_send_set_state(uid, APP_STATE_RECORDING);

	SLOG(LOG_DEBUG, TAG_STTD, "[Server SUCCESS] Start recognition");
}

void __sttd_start_sound_completed_cb(int id, void *user_data)
{
	SLOG(LOG_DEBUG, TAG_STTD, "===== Start sound completed");

	int uid = (int)user_data;
	/* 4. after wav play callback, recorder start */
	__sttd_server_recorder_start(uid);

	SLOG(LOG_DEBUG, TAG_STTD, "=====");
	SLOG(LOG_DEBUG, TAG_STTD, "  ");
	return;
}

int sttd_server_start(int uid, const char* lang, const char* recognition_type, int silence, const char* appid)
{
	if (NULL == lang || NULL == recognition_type) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Input parameter is NULL");
		return STTD_ERROR_INVALID_PARAMETER;
	}

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

	int ret = 0;
	if (false == stt_client_get_app_agreed(uid)) {
		bool temp = false;
		ret = sttd_engine_agent_check_app_agreed(uid, appid, &temp);
		if (0 != ret) {
			SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to get engine available : %d", ret);
			return STTD_ERROR_OPERATION_FAILED;
		}

		if (false == temp) {
			SLOG(LOG_ERROR, TAG_STTD, "[Server] App(%s) NOT confirmed that engine is available", appid);
			return STTD_ERROR_PERMISSION_DENIED;
		}

		stt_client_set_app_agreed(uid);
	}

	/* check if engine use network */
	if (true == sttd_engine_agent_need_network(uid)) {
		if (false == stt_network_is_connected()) {
			SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Disconnect network. Current engine needs to network connection.");
			return STTD_ERROR_OUT_OF_NETWORK;
		}
	}

	char* sound = NULL;
	if (0 != sttd_client_get_start_sound(uid, &sound)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to get start beep sound");
		return STTD_ERROR_OPERATION_FAILED;
	}

	if (0 != stt_client_set_current_recognition(uid)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Current STT is busy because of recording or processing");
		if (NULL != sound)	free(sound);
		return STTD_ERROR_RECORDER_BUSY;
	}

	/* engine start recognition */
	SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[Server] start : uid(%d), lang(%s), recog_type(%s)", 
			uid, lang, recognition_type);
	if (NULL != sound)
		SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[Server] start sound : %s", sound);

	/* 1. Set audio session */
	ret = sttd_recorder_set_audio_session();
	if (0 != ret) {
		stt_client_unset_current_recognition();
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to set session : %d", ret);
		if (NULL != sound)	free(sound);
		return ret;
	}

	bool is_sound_done = false;

	/* 2. Request wav play */
	if (NULL != sound) {
		int id = 0;
		ret = wav_player_start(sound, SOUND_TYPE_MEDIA, __sttd_start_sound_completed_cb, (void*)uid, &id);
		if (WAV_PLAYER_ERROR_NONE != ret) {
			SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to play wav");
			is_sound_done = true;
		}
		free(sound);
		sound = NULL;
	} else {
		is_sound_done = true;
	}

	/* 3. Create recorder & engine initialize */
	ret = sttd_engine_agent_recognize_start_engine(uid, lang, recognition_type, silence, NULL);
	if (0 != ret) {
		stt_client_unset_current_recognition();
		sttd_recorder_unset_audio_session();
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to start recognition : result(%d)", ret);
		return ret;
	}

	if (0 != strcmp(STTP_RECOGNITION_TYPE_FREE_PARTIAL, recognition_type)) {
		g_recording_timer = ecore_timer_add(g_recording_timeout, __stop_by_recording_timeout, NULL);
	}

	/* change uid state */
	sttd_client_set_state(uid, APP_STATE_RECORDING);

	g_recording_log_count = 0;

	if (true == is_sound_done) {
		SLOG(LOG_DEBUG, TAG_STTD, "[Server] No sound play");

		ret = sttd_engine_agent_recognize_start_recorder(uid);
		if (0 != ret) {
			stt_client_unset_current_recognition();
			sttd_recorder_unset_audio_session();

			sttd_engine_agent_recognize_cancel(uid);
			ecore_timer_del(g_recording_timer);
			sttd_client_set_state(uid, APP_STATE_READY);

			SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to start recorder : result(%d)", ret);
			return STTD_ERROR_OPERATION_FAILED;
		}

		SLOG(LOG_DEBUG, TAG_STTD, "[Server SUCCESS] Start recognition");
		return STTD_RESULT_STATE_DONE;
	}

	SLOG(LOG_DEBUG, TAG_STTD, "[Server] Wait sound finish");

	return STTD_RESULT_STATE_NOT_DONE;
}

Eina_Bool __time_out_for_processing(void *data)
{
	g_processing_timer = NULL;

	/* current uid */
	int uid = stt_client_get_current_recognition();
	if (0 == uid)	return EINA_FALSE;

	/* Cancel engine */
	int ret = sttd_engine_agent_recognize_cancel(uid);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to cancel : result(%d)", ret);
	}

	if (0 != sttdc_send_result(uid, STTP_RESULT_EVENT_FINAL_RESULT, NULL, 0, "Time out not to receive recognition result.")) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to send result ");

		/* send error msg */
		int reason = (int)STTD_ERROR_TIMED_OUT;	
		if (0 != sttdc_send_error_signal(uid, reason, "[ERROR] Fail to send recognition result")) {
			SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to send error info ");
		}
	}

	/* Change uid state */
	sttd_client_set_state(uid, APP_STATE_READY);

	stt_client_unset_current_recognition();

	return EINA_FALSE;
}

void __sttd_server_engine_stop(int uid)
{
	/* change uid state */
	sttd_client_set_state(uid, APP_STATE_PROCESSING);

	/* Notify uid state change */
	sttdc_send_set_state(uid, APP_STATE_PROCESSING);

	/* Unset audio session */
	int ret = sttd_recorder_unset_audio_session();
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to unset session : %d", ret);
		return;
	}

	/* Stop engine */
	ret = sttd_engine_agent_recognize_stop_engine(uid);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to stop engine : result(%d)", ret);
		return;
	}

	SLOG(LOG_DEBUG, TAG_STTD, "[Server SUCCESS] Stop recognition");
}

void __sttd_stop_sound_completed_cb(int id, void *user_data)
{
	SLOG(LOG_DEBUG, TAG_STTD, "===== Stop sound completed");

	int uid = (int)user_data;
	/* After wav play callback, engine stop */
	__sttd_server_engine_stop(uid);

	SLOG(LOG_DEBUG, TAG_STTD, "=====");
	SLOG(LOG_DEBUG, TAG_STTD, "  ");
	return;
}

int sttd_server_stop(int uid)
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

	if (NULL != g_recording_timer)	{
		ecore_timer_del(g_recording_timer);
		g_recording_timer = NULL;
	}

	char* sound = NULL;
	if (0 != sttd_client_get_stop_sound(uid, &sound)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to get start beep sound"); 
		return STTD_ERROR_OPERATION_FAILED;
	}

	SLOG(LOG_DEBUG, TAG_STTD, "[Server] stop sound path : %s", sound);

	int ret;
	/* 1. Stop recorder */
	ret = sttd_engine_agent_recognize_stop_recorder(uid);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to stop recorder : result(%d)", ret);
		if (0 != sttd_engine_agent_recognize_cancel(uid)) {
				SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to cancel recognize");
		}
		if (NULL != sound)	free(sound);
		return STTD_ERROR_OPERATION_FAILED;
	}

	/* 2. Request wav play */
	if (NULL != sound) {
		int id = 0;
		ret = wav_player_start(sound, SOUND_TYPE_MEDIA, __sttd_stop_sound_completed_cb, (void*)uid, &id);
		if (WAV_PLAYER_ERROR_NONE != ret) {
			SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to play wav");
		} else {
			SLOG(LOG_DEBUG, TAG_STTD, "[Server] Play wav : %s", sound);
		}

		free(sound);

		g_processing_timer = ecore_timer_add(g_processing_timeout, __time_out_for_processing, NULL);

		return STTD_RESULT_STATE_NOT_DONE;
	} else {
		SLOG(LOG_DEBUG, TAG_STTD, "[Server] No sound play");

		/* Unset audio session */
		ret = sttd_recorder_unset_audio_session();
		if (0 != ret) {
			SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to unset session : %d", ret);
			return STTD_ERROR_OPERATION_FAILED;
		}

		/* Stop engine */
		ret = sttd_engine_agent_recognize_stop_engine(uid);
		if (0 != ret) {
			stt_client_unset_current_recognition();
			SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to stop engine : result(%d)", ret);
			return STTD_ERROR_OPERATION_FAILED;
		}

		/* change uid state */
		sttd_client_set_state(uid, APP_STATE_PROCESSING);

		SLOG(LOG_DEBUG, TAG_STTD, "[Server SUCCESS] Stop recognition");
		
		g_processing_timer = ecore_timer_add(g_processing_timeout, __time_out_for_processing, NULL);

		return STTD_RESULT_STATE_DONE;
	}

	return STTD_ERROR_NONE;
}

int sttd_server_cancel(int uid)
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
		return STTD_ERROR_NONE;
	}

	stt_client_unset_current_recognition();

	if (NULL != g_recording_timer) {
		ecore_timer_del(g_recording_timer);
		g_recording_timer = NULL;
	}

	if (NULL != g_processing_timer) {
		ecore_timer_del(g_processing_timer);
		g_processing_timer = NULL;
	}

	if (APP_STATE_RECORDING == state) {
		/* Unset audio session */
		int ret = sttd_recorder_unset_audio_session();
		if (0 != ret) {
			SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to unset session : %d", ret);
			return STTD_ERROR_OPERATION_FAILED;
		}		
	}

	/* change uid state */
	sttd_client_set_state(uid, APP_STATE_READY);

	/* cancel engine recognition */
	int ret = sttd_engine_agent_recognize_cancel(uid);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Server ERROR] Fail to cancel : result(%d)", ret);
		return STTD_ERROR_OPERATION_FAILED;
	}

	return STTD_ERROR_NONE;
}
