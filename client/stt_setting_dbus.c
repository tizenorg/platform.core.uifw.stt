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


#include "stt_main.h"
#include "stt_setting_dbus.h"

static int g_waiting_time = 1500;

static DBusConnection* g_conn = NULL;

int stt_setting_dbus_open_connection()
{
	if( NULL != g_conn ) {
		SLOG(LOG_WARN, TAG_STTC, "already existed connection");
		return 0;
	}

	DBusError err;
	int ret;

	/* initialise the error value */
	dbus_error_init(&err);

	/* connect to the DBUS system bus, and check for errors */
	g_conn = dbus_bus_get_private(DBUS_BUS_SYSTEM, &err);

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_STTC, "Dbus Connection Error (%s)\n", err.message); 
		dbus_error_free(&err); 
	}

	if (NULL == g_conn) {
		SLOG(LOG_ERROR, TAG_STTC, "fail to get dbus connection \n");
		return STT_SETTING_ERROR_OPERATION_FAILED; 
	}

	int pid = getpid();

	char service_name[64];
	memset(service_name, 0, 64);
	snprintf(service_name, 64, "%s%d", STT_SETTING_SERVICE_NAME, pid);

	SLOG(LOG_DEBUG, TAG_STTC, "service name is %s\n", service_name);

	/* register our name on the bus, and check for errors */
	ret = dbus_bus_request_name(g_conn, service_name, DBUS_NAME_FLAG_REPLACE_EXISTING , &err);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_STTC, "Name Error (%s)\n", err.message); 
		dbus_error_free(&err); 
	}

	if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) {
		SLOG(LOG_ERROR, TAG_STTC, "fail dbus_bus_request_name()\n");
		return STT_SETTING_ERROR_OPERATION_FAILED;
	}

	return 0;
}

int stt_setting_dbus_close_connection()
{
	DBusError err;
	dbus_error_init(&err);

	int pid = getpid();

	char service_name[64];
	memset(service_name, 0, 64);
	snprintf(service_name, 64, "%s%d", STT_SETTING_SERVICE_NAME, pid);

	dbus_bus_release_name(g_conn, service_name, &err);

	dbus_connection_close(g_conn);

	g_conn = NULL;

	return 0;
}


int stt_setting_dbus_request_hello()
{
	DBusMessage* msg;

	msg = dbus_message_new_method_call(
		STT_SERVER_SERVICE_NAME, 
		STT_SERVER_SERVICE_OBJECT_PATH, 
		STT_SERVER_SERVICE_INTERFACE, 
		STT_SETTING_METHOD_HELLO);

	if (NULL == msg) { 
		SLOG(LOG_ERROR, TAG_STTC, ">>>> Request setting hello : Fail to make message \n"); 
		return STT_SETTING_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, ">>>> Request setting hello");
	}

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg = NULL;
	int result = 0;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn, msg, 500, &err);

	dbus_message_unref(msg);

	if (NULL != result_msg) {
		dbus_message_unref(result_msg);

		SLOG(LOG_DEBUG, TAG_STTC, "<<<< setting hello");
		result = 0;
	} else {
		SLOG(LOG_ERROR, TAG_STTC, "<<<< setting hello : no response");
		result = -1;
	}

	return result;
}

int stt_setting_dbus_request_initialize()
{
	DBusMessage* msg;

	msg = dbus_message_new_method_call(
		STT_SERVER_SERVICE_NAME, 
		STT_SERVER_SERVICE_OBJECT_PATH, 
		STT_SERVER_SERVICE_INTERFACE, 
		STT_SETTING_METHOD_INITIALIZE);

	if (NULL == msg) { 
		SLOG(LOG_ERROR, TAG_STTC, ">>>> Request setting initialize : Fail to make message \n"); 
		return STT_SETTING_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, ">>>> Request setting initialize");
	}

	int pid = getpid();

	dbus_message_append_args( msg, 
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = STT_SETTING_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn, msg, g_waiting_time, &err);

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err, DBUS_TYPE_INT32, &result, DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) { 
			SLOG(LOG_ERROR, TAG_STTC, "Get arguments error (%s)\n", err.message);
			dbus_error_free(&err); 
			result = STT_SETTING_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);
	}

	if (0 == result) {
		SLOG(LOG_DEBUG, TAG_STTC, "<<<< setting initialize : result = %d", result);
	} else {
		SLOG(LOG_ERROR, TAG_STTC, "<<<< setting initialize : result = %d", result);
	}

	dbus_message_unref(msg);

	return result;
}

int stt_setting_dbus_request_finalilze(void)
{
	DBusMessage* msg;

	msg = dbus_message_new_method_call(
		STT_SERVER_SERVICE_NAME, 
		STT_SERVER_SERVICE_OBJECT_PATH, 
		STT_SERVER_SERVICE_INTERFACE, 
		STT_SETTING_METHOD_FINALIZE);

	if (NULL == msg) { 
		SLOG(LOG_ERROR, TAG_STTC, ">>>> Request setting finalize : Fail to make message \n"); 
		return STT_SETTING_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, ">>>> Request setting finalize");
	}

	int pid = getpid();

	dbus_message_append_args(msg, DBUS_TYPE_INT32, &pid, DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = STT_SETTING_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn, msg, g_waiting_time, &err);

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err, DBUS_TYPE_INT32, &result, DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) { 
			SLOG(LOG_ERROR, TAG_STTC, "Get arguments error (%s)\n", err.message);
			dbus_error_free(&err); 
			result = STT_SETTING_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);
	}

	if (0 == result) {
		SLOG(LOG_DEBUG, TAG_STTC, "<<<< setting finallize : result = %d", result);
	} else {
		SLOG(LOG_ERROR, TAG_STTC, "<<<< setting finallize : result = %d", result);
	}

	dbus_message_unref(msg);

	return result;
}

int stt_setting_dbus_request_get_engine_list(stt_setting_supported_engine_cb callback, void* user_data)
{
	if (NULL == callback) {
		SLOG(LOG_ERROR, TAG_STTC, "Input parameter is NULL");
		return -1;
	}

	DBusMessage* msg;

	msg = dbus_message_new_method_call(
		STT_SERVER_SERVICE_NAME, 
		STT_SERVER_SERVICE_OBJECT_PATH, 
		STT_SERVER_SERVICE_INTERFACE, 
		STT_SETTING_METHOD_GET_ENGINE_LIST);

	if (NULL == msg) { 
		SLOG(LOG_ERROR, TAG_STTC, ">>>> Request setting get engine list : Fail to make message \n"); 
		return STT_SETTING_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, ">>>> Request setting get engine list");
	}

	int pid = getpid();

	dbus_message_append_args(msg, DBUS_TYPE_INT32, &pid, DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = STT_SETTING_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn, msg, g_waiting_time, &err);

	if (NULL != result_msg) {
		DBusMessageIter args;
		if (dbus_message_iter_init(result_msg, &args)) {
			/* Get result */
			if (DBUS_TYPE_INT32 == dbus_message_iter_get_arg_type(&args)) {
				dbus_message_iter_get_basic(&args, &result);
				dbus_message_iter_next(&args);
			} 

			if (0 == result) {
				SLOG(LOG_DEBUG, TAG_STTC, "<<<< setting get engine list : result = %d \n", result);

				int size ; 
				char* temp_id;
				char* temp_name;
				char* temp_path;

				/* Get engine count */
				if (DBUS_TYPE_INT32 == dbus_message_iter_get_arg_type(&args)) {
					dbus_message_iter_get_basic(&args, &size);
					dbus_message_iter_next(&args);
				}

				int i=0;
				for (i=0 ; i<size ; i++) {
					dbus_message_iter_get_basic(&args, &(temp_id));
					dbus_message_iter_next(&args);

					dbus_message_iter_get_basic(&args, &(temp_name));
					dbus_message_iter_next(&args);

					dbus_message_iter_get_basic(&args, &(temp_path));
					dbus_message_iter_next(&args);

					if (true != callback(temp_id, temp_name, temp_path, user_data)) {
						break;
					}
				}
			}  else {
				SLOG(LOG_ERROR, TAG_STTC, "<<<< setting get engine list : result = %d \n", result);
			}
		} else {
			SLOG(LOG_ERROR, TAG_STTC, "<<<< setting get engine list : invalid message \n");
		}

		dbus_message_unref(result_msg);
	} else {
		SLOG(LOG_ERROR, TAG_STTC, "<<<< setting get engine list : result message is NULL!! \n");
	}

	dbus_message_unref(msg);

	return result;
}

int stt_setting_dbus_request_get_engine(char** engine_id)
{
	if (NULL == engine_id)	{
		SLOG(LOG_ERROR, TAG_STTC, "Input parameter is NULL");
		return STT_SETTING_ERROR_INVALID_PARAMETER;
	}

	DBusMessage* msg;

	msg = dbus_message_new_method_call(
		STT_SERVER_SERVICE_NAME, 
		STT_SERVER_SERVICE_OBJECT_PATH, 
		STT_SERVER_SERVICE_INTERFACE, 
		STT_SETTING_METHOD_GET_ENGINE);

	if (NULL == msg) { 
		SLOG(LOG_ERROR, TAG_STTC, ">>>> Request setting get engine : Fail to make message \n"); 
		return STT_SETTING_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, ">>>> Request setting get engine ");
	}

	int pid = getpid();

	dbus_message_append_args( msg, 
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = STT_SETTING_ERROR_OPERATION_FAILED;
	char* temp;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn, msg, g_waiting_time, &err);

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err, DBUS_TYPE_INT32, &result, DBUS_TYPE_STRING, &temp, DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) { 
			SLOG(LOG_ERROR, TAG_STTC, "Get arguments error (%s)\n", err.message);
			dbus_error_free(&err); 
			result = STT_SETTING_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);
	}

	if (0 == result) {
		*engine_id = strdup(temp);

		if (NULL == *engine_id) {
			SLOG(LOG_ERROR, TAG_STTC, "<<<< setting get engine : Out of memory \n");
			result = STT_SETTING_ERROR_OUT_OF_MEMORY;
		} else {
			SLOG(LOG_DEBUG, TAG_STTC, "<<<< setting get engine : result(%d), engine id(%s)\n", result, *engine_id);
		}
	} else {
		SLOG(LOG_ERROR, TAG_STTC, "<<<< setting get engine : result(%d) \n", result);
	}

	dbus_message_unref(msg);

	return result;
}

int stt_setting_dbus_request_set_engine(const char* engine_id)
{
	if (NULL == engine_id) {
		SLOG(LOG_ERROR, TAG_STTC, "Input parameter is NULL");
		return STT_SETTING_ERROR_INVALID_PARAMETER;
	}

	DBusMessage* msg;

	msg = dbus_message_new_method_call(
		STT_SERVER_SERVICE_NAME, 
		STT_SERVER_SERVICE_OBJECT_PATH, 
		STT_SERVER_SERVICE_INTERFACE, 
		STT_SETTING_METHOD_SET_ENGINE);

	if (NULL == msg) { 
		SLOG(LOG_ERROR, TAG_STTC, ">>>> Request setting set engine : Fail to make message \n"); 
		return STT_SETTING_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, ">>>> Request setting set engine : engine id(%s)", engine_id);
	}

	int pid = getpid();

	dbus_message_append_args( msg, 
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_STRING, &engine_id,
		DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = STT_SETTING_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn, msg, g_waiting_time, &err);

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err, DBUS_TYPE_INT32, &result, DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) { 
			SLOG(LOG_ERROR, TAG_STTC, "Get arguments error (%s)\n", err.message);
			dbus_error_free(&err); 
			result = STT_SETTING_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);
	}

	if (0 == result) {
		SLOG(LOG_DEBUG, TAG_STTC, "<<<< setting set engine : result(%d) \n", result);
	} else {
		SLOG(LOG_ERROR, TAG_STTC, "<<<< setting set engine : result(%d) \n", result);
	}

	dbus_message_unref(msg);

	return result;
}

int stt_setting_dbus_request_get_language_list(stt_setting_supported_language_cb callback, void* user_data)
{
	if (NULL == callback) {
		SLOG(LOG_ERROR, TAG_STTC, "Input parameter is NULL");
		return STT_SETTING_ERROR_INVALID_PARAMETER;
	}

	DBusMessage* msg;

	msg = dbus_message_new_method_call(
		STT_SERVER_SERVICE_NAME, 
		STT_SERVER_SERVICE_OBJECT_PATH, 
		STT_SERVER_SERVICE_INTERFACE, 
		STT_SETTING_METHOD_GET_LANG_LIST);

	if (NULL == msg) { 
		SLOG(LOG_ERROR, TAG_STTC, ">>>> Request setting get language list : Fail to make message \n"); 
		return STT_SETTING_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, ">>>> Request setting get language list");
	}

	int pid = getpid();

	dbus_message_append_args( msg, 
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = STT_SETTING_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn, msg, g_waiting_time, &err);

	if (NULL != result_msg) {
		DBusMessageIter args;

		if (dbus_message_iter_init(result_msg, &args)) {
			/* Get result */
			if (DBUS_TYPE_INT32 == dbus_message_iter_get_arg_type(&args)) {
				dbus_message_iter_get_basic(&args, &result);
				dbus_message_iter_next(&args);
			} 

			if (0 == result) {
				SLOG(LOG_DEBUG, TAG_STTC, "<<<< setting get language list : result = %d \n", result);

				int size = 0; 
				char* temp_id = NULL;
				char* temp_lang = NULL;

				/* Get engine id */
				dbus_message_iter_get_basic(&args, &temp_id);
				dbus_message_iter_next(&args);

				if (NULL != temp_id) {
					/* Get language count */
					if (DBUS_TYPE_INT32 == dbus_message_iter_get_arg_type(&args)) {
						dbus_message_iter_get_basic(&args, &size);
						dbus_message_iter_next(&args);
					}

					int i=0;
					for (i=0 ; i<size ; i++) {
						dbus_message_iter_get_basic(&args, &(temp_lang) );
						dbus_message_iter_next(&args);

						if (true != callback(temp_id, temp_lang, user_data)) {
							break;
						}
					}
				} else {
					SLOG(LOG_ERROR, TAG_STTC, "Engine ID is NULL \n");
					result = STT_SETTING_ERROR_OPERATION_FAILED;
				}

			} else {
				SLOG(LOG_ERROR, TAG_STTC, "<<<< setting get language list : result = %d \n", result);
			}
		} 

		dbus_message_unref(result_msg);
	} else {
		SLOG(LOG_ERROR, TAG_STTC, "<<<< setting get language list : Result message is NULL!!");
	}

	dbus_message_unref(msg);
	
	return result;
}

int stt_setting_dbus_request_get_default_language(char** language)
{
	if (NULL == language) {
		SLOG(LOG_ERROR, TAG_STTC, "Input Parameter is NULL");
		return STT_SETTING_ERROR_INVALID_PARAMETER;
	}

	DBusMessage* msg;

	msg = dbus_message_new_method_call(
		STT_SERVER_SERVICE_NAME, 
		STT_SERVER_SERVICE_OBJECT_PATH, 
		STT_SERVER_SERVICE_INTERFACE, 
		STT_SETTING_METHOD_GET_DEFAULT_LANG);

	if (NULL == msg) { 
		SLOG(LOG_ERROR, TAG_STTC, ">>>> Request setting get default language : Fail to make message \n"); 
		return STT_SETTING_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, ">>>> Request setting get default language");
	}

	int pid = getpid();

	dbus_message_append_args( msg, 
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = STT_SETTING_ERROR_OPERATION_FAILED;
	char* temp_char;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn, msg, g_waiting_time, &err);

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err, 
			DBUS_TYPE_INT32, &result, 
			DBUS_TYPE_STRING, &temp_char,
			DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) { 
			SLOG(LOG_ERROR, TAG_STTC, "Get arguments error (%s)\n", err.message);
			dbus_error_free(&err); 
			result = STT_SETTING_ERROR_OPERATION_FAILED;
		}

		dbus_message_unref(result_msg);
	}

	if (0 == result) {
		*language = strdup(temp_char);

		if (NULL == *language) {
			SLOG(LOG_ERROR, TAG_STTC, "<<<< setting get default language : Out of memory \n");
			result = STT_SETTING_ERROR_OUT_OF_MEMORY;
		} else {
			SLOG(LOG_DEBUG, TAG_STTC, "<<<< setting get default language : result(%d), lang(%s)\n", result, *language);
		}
	} else {
		SLOG(LOG_ERROR, TAG_STTC, "<<<< setting get default language : result(%d) \n", result);
	}

	dbus_message_unref(msg);

	return result;
}


int stt_setting_dbus_request_set_default_language(const char* language)
{
	if (NULL == language) {
		SLOG(LOG_ERROR, TAG_STTC, "Input Parameter is NULL");
		return STT_SETTING_ERROR_INVALID_PARAMETER;
	}

	DBusMessage* msg;

	msg = dbus_message_new_method_call(
		STT_SERVER_SERVICE_NAME, 
		STT_SERVER_SERVICE_OBJECT_PATH, 
		STT_SERVER_SERVICE_INTERFACE, 
		STT_SETTING_METHOD_SET_DEFAULT_LANG);

	if (NULL == msg) { 
		SLOG(LOG_ERROR, TAG_STTC, ">>>> Request setting set default language : Fail to make message \n"); 
		return STT_SETTING_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, ">>>> Request setting set default language : lang(%s)\n", language);
	}

	int pid = getpid();

	dbus_message_append_args( msg, 
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_STRING, &language,
		DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = STT_SETTING_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn, msg, g_waiting_time, &err);

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err, 
			DBUS_TYPE_INT32, &result, 
			DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) { 
			SLOG(LOG_ERROR, TAG_STTC, "Get arguments error (%s)\n", err.message);
			dbus_error_free(&err); 
			result = STT_SETTING_ERROR_OPERATION_FAILED;
		}

		dbus_message_unref(result_msg);
	}

	if (0 == result) {
		SLOG(LOG_DEBUG, TAG_STTC, "<<<< setting set default language : result(%d)", result);
	} else {
		SLOG(LOG_ERROR, TAG_STTC, "<<<< setting set default language : result(%d)", result);
	}

	dbus_message_unref(msg);
	
	return result;
}

int stt_setting_dbus_request_get_engine_setting(stt_setting_engine_setting_cb callback, void* user_data)
{
	if (NULL == callback) {
		SLOG(LOG_ERROR, TAG_STTC, "Input Parameter is NULL");
		return STT_SETTING_ERROR_INVALID_PARAMETER;
	}

	DBusMessage* msg;

	msg = dbus_message_new_method_call(
		STT_SERVER_SERVICE_NAME, 
		STT_SERVER_SERVICE_OBJECT_PATH, 
		STT_SERVER_SERVICE_INTERFACE, 
		STT_SETTING_METHOD_GET_ENGINE_SETTING);

	if (NULL == msg) { 
		SLOG(LOG_ERROR, TAG_STTC, ">>>> Request setting get engine setting : Fail to make message \n"); 
		return STT_SETTING_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, ">>>> Request setting get engine setting");
	}

	int pid = getpid();

	dbus_message_append_args( msg, 
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = STT_SETTING_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn, msg, g_waiting_time, &err);

	if (NULL != result_msg) {
		DBusMessageIter args;

		if (dbus_message_iter_init(result_msg, &args)) {
			/* Get result */
			if (DBUS_TYPE_INT32 == dbus_message_iter_get_arg_type(&args)) {
				dbus_message_iter_get_basic(&args, &result);
				dbus_message_iter_next(&args);
			} 

			if (0 == result) {
				SLOG(LOG_DEBUG, TAG_STTC, "<<<< get engine setting : result = %d \n", result);
				int size; 
				char* temp_id = NULL;
				char* temp_key;
				char* temp_value;

				/* Get engine id */
				dbus_message_iter_get_basic(&args, &temp_id);
				dbus_message_iter_next(&args);

				if (NULL != temp_id) {
					/* Get setting count */
					if (DBUS_TYPE_INT32 == dbus_message_iter_get_arg_type(&args)) {
						dbus_message_iter_get_basic(&args, &size);
						dbus_message_iter_next(&args);
					}

					int i=0;
					for (i=0 ; i<size ; i++) {
						dbus_message_iter_get_basic(&args, &(temp_key) );
						dbus_message_iter_next(&args);

						dbus_message_iter_get_basic(&args, &(temp_value) );
						dbus_message_iter_next(&args);

						if (true != callback(temp_id, temp_key, temp_value, user_data)) {
							break;
						}
					} 
				} else {
					SLOG(LOG_ERROR, TAG_STTC, "<<<< get engine setting : result message is invalid \n");
					result = STT_SETTING_ERROR_OPERATION_FAILED;
				}
			} 
		} else {
			SLOG(LOG_ERROR, TAG_STTC, "<<<< get engine setting : result message is invalid \n");
			result = STT_SETTING_ERROR_OPERATION_FAILED;
		}

		dbus_message_unref(result_msg);
	} else {
		SLOG(LOG_ERROR, TAG_STTC, "<<<< get engine setting : Result message is NULL!! \n");
	}	

	dbus_message_unref(msg);

	return result;
}

int stt_setting_dbus_request_set_engine_setting(const char* key, const char* value)
{
	if (NULL == key || NULL == value) {
		SLOG(LOG_ERROR, TAG_STTC, "Input Parameter is NULL");
		return STT_SETTING_ERROR_INVALID_PARAMETER;
	}

	DBusMessage* msg;

	msg = dbus_message_new_method_call(
		STT_SERVER_SERVICE_NAME, 
		STT_SERVER_SERVICE_OBJECT_PATH, 
		STT_SERVER_SERVICE_INTERFACE, 
		STT_SETTING_METHOD_SET_ENGINE_SETTING);

	if (NULL == msg) { 
		SLOG(LOG_ERROR, TAG_STTC, ">>>> Request setting set engine setting : Fail to make message \n"); 
		return STT_SETTING_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, ">>>> Request setting set engine setting : key(%s), value(%s)", key, value);
	}

	int pid = getpid();

	dbus_message_append_args( msg, 
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_STRING, &key,
		DBUS_TYPE_STRING, &value, 
		DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = STT_SETTING_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn, msg, g_waiting_time, &err);

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err, DBUS_TYPE_INT32, &result, DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) { 
			SLOG(LOG_ERROR, TAG_STTC, "<<<< Get arguments error (%s)\n", err.message);
			dbus_error_free(&err); 
			result = -1;
		}
		dbus_message_unref(result_msg);
	}

	if (0 == result) {
		SLOG(LOG_DEBUG, TAG_STTC, "<<<< setting set engine setting : result(%d)", result);
	} else {
		SLOG(LOG_ERROR, TAG_STTC, "<<<< setting set engine setting : result(%d)", result);
	}

	dbus_message_unref(msg);

	return result;
}

int stt_setting_dbus_request_get_profanity_filter(bool* value)
{
	if (NULL == value) {
		SLOG(LOG_ERROR, TAG_STTC, "Input Parameter is NULL");
		return STT_SETTING_ERROR_INVALID_PARAMETER;
	}

	DBusMessage* msg;

	msg = dbus_message_new_method_call(
		STT_SERVER_SERVICE_NAME, 
		STT_SERVER_SERVICE_OBJECT_PATH, 
		STT_SERVER_SERVICE_INTERFACE, 
		STT_SETTING_METHOD_GET_PROFANITY);

	if (NULL == msg) { 
		SLOG(LOG_ERROR, TAG_STTC, ">>>> Request setting set profanity filter : Fail to make message \n"); 
		return STT_SETTING_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, ">>>> Request setting set profanity filter");
	}

	int pid = getpid();

	dbus_message_append_args( msg, 
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = STT_SETTING_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn, msg, g_waiting_time, &err);

	if (NULL != result_msg)	{
		dbus_message_get_args(result_msg, &err, DBUS_TYPE_INT32, &result, DBUS_TYPE_INT32, value, DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) { 
			SLOG(LOG_ERROR, TAG_STTC, "Get arguments error (%s)\n", err.message);
			dbus_error_free(&err); 
			result = STT_SETTING_ERROR_OPERATION_FAILED;
		}

		dbus_message_unref(result_msg);
	} 

	if (0 == result) {
		SLOG(LOG_DEBUG, TAG_STTC, "<<<< setting get profanity filter : result(%d), value(%s)", result, *value ? "true":"false");
	} else {
		SLOG(LOG_ERROR, TAG_STTC, "<<<< setting get profanity filter : result(%d)", result);
	}
	
	dbus_message_unref(msg);

	return result;
}

int stt_setting_dbus_request_set_profanity_filter(const bool value)
{
	DBusMessage* msg;

	msg = dbus_message_new_method_call(
		STT_SERVER_SERVICE_NAME, 
		STT_SERVER_SERVICE_OBJECT_PATH, 
		STT_SERVER_SERVICE_INTERFACE, 
		STT_SETTING_METHOD_SET_PROFANITY);

	if (NULL == msg) { 
		SLOG(LOG_ERROR, TAG_STTC, ">>>> Request setting set profanity filter : Fail to make message \n"); 
		return STT_SETTING_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, ">>>> Request setting set profanity filter : value(%s)", value ? "true":"false");
	}

	int pid = getpid();

	dbus_message_append_args( msg, 
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_INT32, &value, 
		DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = STT_SETTING_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn, msg, g_waiting_time, &err);

	if (NULL != result_msg)	{
		dbus_message_get_args(result_msg, &err, DBUS_TYPE_INT32, &result, DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) { 
			SLOG(LOG_ERROR, TAG_STTC, "Get arguments error (%s)\n", err.message);
			dbus_error_free(&err); 
			result = STT_SETTING_ERROR_OPERATION_FAILED;
		}

		dbus_message_unref(result_msg);
	} 

	if (0 == result) {
		SLOG(LOG_DEBUG, TAG_STTC, "<<<< setting set profanity filter : result(%d)", result);
	} else {
		SLOG(LOG_ERROR, TAG_STTC, "<<<< setting set profanity filter : result(%d)", result);
	}

	dbus_message_unref(msg);

	return result;
}

int stt_setting_dbus_request_get_punctuation_override(bool* value)
{
	if (NULL == value) {
		SLOG(LOG_ERROR, TAG_STTC, "Input Parameter is NULL");
		return STT_SETTING_ERROR_INVALID_PARAMETER;
	}

	DBusMessage* msg;

	msg = dbus_message_new_method_call(
		STT_SERVER_SERVICE_NAME, 
		STT_SERVER_SERVICE_OBJECT_PATH, 
		STT_SERVER_SERVICE_INTERFACE, 
		STT_SETTING_METHOD_GET_PUNCTUATION);

	if (NULL == msg) { 
		SLOG(LOG_ERROR, TAG_STTC, ">>>> Request setting get punctuation override : Fail to make message \n"); 
		return STT_SETTING_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, ">>>> Request setting get punctuation override ");
	}

	int pid = getpid();

	dbus_message_append_args( msg, 
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = STT_SETTING_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn, msg, g_waiting_time, &err);

	if (NULL != result_msg)	{
		dbus_message_get_args(result_msg, &err, DBUS_TYPE_INT32, &result, DBUS_TYPE_INT32, value, DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) { 
			SLOG(LOG_ERROR, TAG_STTC, "Get arguments error (%s)\n", err.message);
			dbus_error_free(&err); 
			result = STT_SETTING_ERROR_OPERATION_FAILED;
		}

		dbus_message_unref(result_msg);
	} 

	if (0 == result) {
		SLOG(LOG_DEBUG, TAG_STTC, "<<<< setting get punctuation override : result(%d), value(%s)", result, *value ? "true":"false");
	} else {
		SLOG(LOG_ERROR, TAG_STTC, "<<<< setting get punctuation override : result(%d)", result);
	}

	dbus_message_unref(msg);

	return result;
}

int stt_setting_dbus_request_set_punctuation_override(const bool value )
{
	DBusMessage* msg;

	msg = dbus_message_new_method_call(
		STT_SERVER_SERVICE_NAME, 
		STT_SERVER_SERVICE_OBJECT_PATH, 
		STT_SERVER_SERVICE_INTERFACE, 
		STT_SETTING_METHOD_SET_PUNCTUATION);

	if (NULL == msg) { 
		SLOG(LOG_ERROR, TAG_STTC, ">>>> Request setting set punctuation override : Fail to make message \n"); 
		return STT_SETTING_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, ">>>> Request setting set punctuation override : value(%s)", value ? "true":"false");
	}

	int pid = getpid();

	dbus_message_append_args( msg, 
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_INT32, &value, 
		DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = STT_SETTING_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn, msg, g_waiting_time, &err);

	if (NULL != result_msg)	{
		dbus_message_get_args(result_msg, &err, DBUS_TYPE_INT32, &result, DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) { 
			SLOG(LOG_ERROR, TAG_STTC, "Get arguments error (%s)\n", err.message);
			dbus_error_free(&err); 
			result = STT_SETTING_ERROR_OPERATION_FAILED;
		}

		dbus_message_unref(result_msg);
	} 

	if (0 == result) {
		SLOG(LOG_DEBUG, TAG_STTC, "<<<< setting set punctuation override : result(%d)", result);
	} else {
		SLOG(LOG_ERROR, TAG_STTC, "<<<< setting set punctuation override : result(%d)", result);
	}

	dbus_message_unref(msg);

	return result;
}

int stt_setting_dbus_request_get_silence_detection(bool* value)
{
	if (NULL == value) {
		SLOG(LOG_ERROR, TAG_STTC, "Input Parameter is NULL");
		return STT_SETTING_ERROR_INVALID_PARAMETER;
	}

	DBusMessage* msg;

	msg = dbus_message_new_method_call(
		STT_SERVER_SERVICE_NAME, 
		STT_SERVER_SERVICE_OBJECT_PATH, 
		STT_SERVER_SERVICE_INTERFACE, 
		STT_SETTING_METHOD_GET_SILENCE);

	if (NULL == msg) { 
		SLOG(LOG_ERROR, TAG_STTC, ">>>> Request setting get silence detection : Fail to make message \n"); 
		return STT_SETTING_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, ">>>> Request setting get silence detection : value(%s)", value ? "true":"false");
	}

	int pid = getpid();

	dbus_message_append_args( msg, 
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_INT32, &value, 
		DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = STT_SETTING_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn, msg, g_waiting_time, &err);

	if (NULL == result_msg)	{
		dbus_message_unref(msg);
		return STT_SETTING_ERROR_OPERATION_FAILED;
	}

	dbus_message_get_args(result_msg, &err, DBUS_TYPE_INT32, &result, DBUS_TYPE_INT32, value, DBUS_TYPE_INVALID);

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_STTC, "Get arguments error (%s)\n", err.message);
		dbus_error_free(&err); 
		result = STT_SETTING_ERROR_OPERATION_FAILED;
	}

	SLOG(LOG_DEBUG, TAG_STTC, "Get Silence Detection : result(%d), value(%d) \n", result, *value);

	dbus_message_unref(msg);

	return result;
}

int stt_setting_dbus_request_set_silence_detection(const bool value)
{
	DBusMessage* msg;

	msg = dbus_message_new_method_call(
		STT_SERVER_SERVICE_NAME, 
		STT_SERVER_SERVICE_OBJECT_PATH, 
		STT_SERVER_SERVICE_INTERFACE, 
		STT_SETTING_METHOD_SET_SILENCE);

	if (NULL == msg) { 
		SLOG(LOG_ERROR, TAG_STTC, ">>>> Request setting set silence detection : Fail to make message \n"); 
		return STT_SETTING_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, ">>>> Request setting set silence detection : value(%s)", value ? "true":"false");
	}

	int pid = getpid();

	dbus_message_append_args( msg, 
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_INT32, &value, 
		DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = STT_SETTING_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn, msg, g_waiting_time, &err);

	if (NULL != result_msg)	{
		dbus_message_get_args(result_msg, &err, DBUS_TYPE_INT32, &result, DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) { 
			SLOG(LOG_ERROR, TAG_STTC, "Get arguments error (%s)\n", err.message);
			dbus_error_free(&err); 
			result = STT_SETTING_ERROR_OPERATION_FAILED;
		}

		dbus_message_unref(result_msg);
	} 

	if (0 == result) {
		SLOG(LOG_DEBUG, TAG_STTC, "<<<< setting set set silence detection : result(%d)", result);
	} else {
		SLOG(LOG_ERROR, TAG_STTC, "<<<< setting set set silence detection : result(%d)", result);
	}

	dbus_message_unref(msg);

	return result;
}







