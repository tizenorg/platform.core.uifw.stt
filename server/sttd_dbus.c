/*
* Copyright (c) 2011-2014 Samsung Electronics Co., Ltd All Rights Reserved 
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


#include <dbus/dbus.h>
#include <Ecore.h>

#include "sttd_main.h"
#include "sttd_dbus.h"
#include "sttd_client_data.h"
#include "sttd_dbus_server.h"
#include "stt_defs.h"

static DBusConnection* g_conn;
static int g_waiting_time = 3000;

int sttdc_send_hello(int uid)
{
	int pid = sttd_client_get_pid(uid);

	if (0 > pid) {
		SLOG(LOG_ERROR, TAG_STTD, "[Dbus ERROR] pid is NOT valid");
		return -1;
	}

	char service_name[64];
	memset(service_name, 0, 64);
	snprintf(service_name, 64, "%s%d", STT_CLIENT_SERVICE_NAME, pid);

	char target_if_name[128];
	snprintf(target_if_name, sizeof(target_if_name), "%s%d", STT_CLIENT_SERVICE_INTERFACE, pid);

	DBusMessage* msg;

	SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[Dbus] Send hello message : uid(%d)", uid);

	msg = dbus_message_new_method_call(
		service_name, 
		STT_CLIENT_SERVICE_OBJECT_PATH, 
		target_if_name, 
		STTD_METHOD_HELLO);

	if (NULL == msg) { 
		SLOG(LOG_ERROR, TAG_STTD, "[Dbus ERROR] Fail to create message"); 
		return -1;
	}

	dbus_message_append_args(msg, DBUS_TYPE_INT32, &uid, DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = -1;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn, msg, g_waiting_time, &err);
	dbus_message_unref(msg);
	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_STTD, "[ERROR] Send error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err, DBUS_TYPE_INT32, &result, DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) { 
			SLOG(LOG_ERROR, TAG_STTD, "[Dbus] Get arguments error (%s)\n", err.message);
			dbus_error_free(&err); 
			result = -1;
		}

		dbus_message_unref(result_msg);
	} else {
		SLOG(LOG_DEBUG, TAG_STTD, "[Dbus] Result message is NULL. Client is not available");
		result = 0;
	}

	return result;
}

int sttdc_send_get_state(int uid, int* state)
{
	int pid = sttd_client_get_pid(uid);

	if (0 > pid) {
		SLOG(LOG_ERROR, TAG_STTD, "[Dbus ERROR] pid is NOT valid");
		return -1;
	}

	char service_name[64];
	memset(service_name, 0, 64);
	snprintf(service_name, 64, "%s%d", STT_CLIENT_SERVICE_NAME, pid);

	char target_if_name[128];
	snprintf(target_if_name, sizeof(target_if_name), "%s%d", STT_CLIENT_SERVICE_INTERFACE, pid);

	DBusMessage* msg;

	SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[Dbus] Send get state message : uid(%d)", uid);

	msg = dbus_message_new_method_call(
		service_name, 
		STT_CLIENT_SERVICE_OBJECT_PATH, 
		target_if_name, 
		STTD_METHOD_GET_STATE);

	if (NULL == msg) { 
		SLOG(LOG_ERROR, TAG_STTD, "[Dbus ERROR] Fail to create message"); 
		return -1;
	}

	dbus_message_append_args(msg, DBUS_TYPE_INT32, &uid, DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int tmp = -1;
	int result = 0;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn, msg, g_waiting_time, &err);
	dbus_message_unref(msg);
	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_STTD, "[ERROR] Send error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err, DBUS_TYPE_INT32, &tmp, DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) { 
			SLOG(LOG_ERROR, TAG_STTD, "[Dbus] Get arguments error (%s)\n", err.message);
			dbus_error_free(&err); 
			result = -1;
		} else {
			*state = tmp;
			result = 0;
		}

		dbus_message_unref(result_msg);
	} else {
		SLOG(LOG_ERROR, TAG_STTD, "[Dbus] Result message is NULL. Client is not available");
		result = -1;
	}
	
	return result;
}

int sttdc_send_result(int uid, int event, const char** data, int data_count, const char* result_msg)
{
	int pid = sttd_client_get_pid(uid);

	if (0 > pid) {
		SLOG(LOG_ERROR, TAG_STTD, "[Dbus ERROR] pid is NOT valid" );
		return -1;
	}

	char service_name[64];
	memset(service_name, 0, 64);
	snprintf(service_name, 64, "%s%d", STT_CLIENT_SERVICE_NAME, pid);

	char target_if_name[128];
	snprintf(target_if_name, sizeof(target_if_name), "%s%d", STT_CLIENT_SERVICE_INTERFACE, pid);

	DBusMessage* msg;

	SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[Dbus] send result signal : uid(%d), event(%d), result count(%d)", uid, event, data_count);

	msg = dbus_message_new_method_call(
		service_name, 
		STT_CLIENT_SERVICE_OBJECT_PATH, 
		target_if_name, 
		STTD_METHOD_RESULT);

	if (NULL == msg) { 
		SLOG(LOG_ERROR, TAG_STTD, "[Dbus ERROR] Fail to create message"); 
		return -1;
	}

	DBusMessageIter args;
	dbus_message_iter_init_append(msg, &args);

	/* Append uid & type */
	dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &uid);

	char* msg_temp;
	dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &event);

	/* Append result msg */
	if (NULL == result_msg) {
		msg_temp = strdup("None");
		dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &msg_temp);
		SLOG(LOG_WARN, TAG_STTD, "[Dbus] result message is NULL"); 
		free(msg_temp);
	} else {
		SLOG(LOG_DEBUG, TAG_STTD, "[Dbus] result message(%s)", result_msg ); 
		dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &(result_msg));
	}
	
	/* Append result size */
	if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &(data_count))) {
		SLOG(LOG_ERROR, TAG_STTD, "[Dbus] response message : Fail to append result size"); 
		return -1;
	}

	int i;
	SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[Dbus] result size (%d)", data_count); 
	for (i = 0;i < data_count;i++) {
		if (NULL != data[i]) {
			SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[Dbus] result (%d, %s)", i, data[i] ); 

			if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &data[i])) {
				SLOG(LOG_ERROR, TAG_STTD, "[Dbus] response message : Fail to append result data");
				return -1;
			}
		} else {
			int reason = (int)STTD_ERROR_OPERATION_FAILED;

			if (0 != sttdc_send_error_signal(uid, reason, "Fail to get recognition result from engine")) {
				SLOG(LOG_ERROR, TAG_STTD, "[Dbus ERROR] Fail to send error info. Remove client data"); 

				/* clean client data */
				sttd_client_delete(uid);
			}

			SECURE_SLOG(LOG_ERROR, TAG_STTD, "[Dbus ERROR] Result from engine is NULL(%d)", i); 
			return -1;
		}
	}
	
	if (!dbus_connection_send(g_conn, msg, NULL)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Dbus ERROR] Fail to send message : Out Of Memory !"); 
		return -1;
	}

	dbus_connection_flush(g_conn);
	dbus_message_unref(msg);

	return 0;
}

int sttdc_send_error_signal(int uid, int reason, const char *err_msg)
{
	if (NULL == err_msg) {
		SLOG(LOG_ERROR, TAG_STTD, "[Dbus ERROR] Input parameter is NULL"); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	int pid = sttd_client_get_pid(uid);

	if (0 > pid) {
		SLOG(LOG_ERROR, TAG_STTD, "[Dbus ERROR] pid is NOT valid" );
		return -1;
	}

	char service_name[64];
	memset(service_name, 0, 64);
	snprintf(service_name, 64, "%s%d", STT_CLIENT_SERVICE_NAME, pid);

	char target_if_name[128];
	snprintf(target_if_name, sizeof(target_if_name), "%s%d", STT_CLIENT_SERVICE_INTERFACE, pid);

	DBusMessage* msg;
	SLOG(LOG_DEBUG, TAG_STTD, "[Dbus] send error signal : reason(%d), Error Msg(%s)", reason, err_msg);

	msg = dbus_message_new_method_call(
		service_name, 
		STT_CLIENT_SERVICE_OBJECT_PATH, 
		target_if_name, 
		STTD_METHOD_ERROR);

	if (NULL == msg) { 
		SLOG(LOG_ERROR, TAG_STTD, "[Dbus ERROR] Fail to create message"); 
		return -1;
	}

	dbus_message_append_args(msg, 
		DBUS_TYPE_INT32, &uid, 
		DBUS_TYPE_INT32, &reason, 
		DBUS_TYPE_STRING, &err_msg,
		DBUS_TYPE_INVALID);
	
	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn, msg, g_waiting_time, &err);
	dbus_message_unref(msg);
	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_STTD, "[ERROR] Send error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_unref(result_msg);
	} else {
		SLOG(LOG_DEBUG, TAG_STTD, "[Dbus] Result message is NULL.");
	}

	return 0;
}

int sttdc_send_set_state(int uid, int state)
{
	int pid = sttd_client_get_pid(uid);

	if (0 > pid) {
		SLOG(LOG_ERROR, TAG_STTD, "[Dbus ERROR] pid is NOT valid");
		return -1;
	}

	char service_name[64];
	memset(service_name, 0, 64);
	snprintf(service_name, 64, "%s%d", STT_CLIENT_SERVICE_NAME, pid);

	char target_if_name[128];
	snprintf(target_if_name, sizeof(target_if_name), "%s%d", STT_CLIENT_SERVICE_INTERFACE, pid);

	DBusMessage* msg;

	SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[Dbus] Send change state message : uid(%d), state(%d)", uid, state);

	msg = dbus_message_new_method_call(
		service_name, 
		STT_CLIENT_SERVICE_OBJECT_PATH, 
		target_if_name, 
		STTD_METHOD_SET_STATE);

	if (NULL == msg) { 
		SLOG(LOG_ERROR, TAG_STTD, "[Dbus ERROR] Fail to create message"); 
		return -1;
	}

	dbus_message_append_args(msg, 
		DBUS_TYPE_INT32, &uid, 
		DBUS_TYPE_INT32, &state, 
		DBUS_TYPE_INVALID);

	if (!dbus_connection_send(g_conn, msg, NULL)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Dbus ERROR] Fail to send message : Out Of Memory !"); 
		return -1;
	}

	dbus_connection_flush(g_conn);
	dbus_message_unref(msg);

	return 0;
}

static Eina_Bool listener_event_callback(void* data, Ecore_Fd_Handler *fd_handler)
{
	DBusConnection* conn = (DBusConnection*)data;
	DBusMessage* msg = NULL;

	if (NULL == conn)
		return ECORE_CALLBACK_RENEW;

	dbus_connection_read_write_dispatch(conn, 50);
	
	msg = dbus_connection_pop_message(conn);

	if (true != dbus_connection_get_is_connected(conn)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Dbus ERROR] Conn is disconnected");
		return ECORE_CALLBACK_RENEW;
	}

	/* loop again if we haven't read a message */
	if (NULL == msg) { 
		return ECORE_CALLBACK_RENEW;
	}

	/* client event */
	if (dbus_message_is_method_call(msg, STT_SERVER_SERVICE_INTERFACE, STT_METHOD_HELLO))
		sttd_dbus_server_hello(conn, msg);

	else if (dbus_message_is_method_call(msg, STT_SERVER_SERVICE_INTERFACE, STT_METHOD_INITIALIZE))
		sttd_dbus_server_initialize(conn, msg);
	
	else if (dbus_message_is_method_call(msg, STT_SERVER_SERVICE_INTERFACE, STT_METHOD_FINALIZE))
		sttd_dbus_server_finalize(conn, msg);

	else if (dbus_message_is_method_call(msg, STT_SERVER_SERVICE_INTERFACE, STT_METHOD_SET_CURRENT_ENGINE))
		sttd_dbus_server_set_current_engine(conn, msg);

	else if (dbus_message_is_method_call(msg, STT_SERVER_SERVICE_INTERFACE, STT_METHOD_CHECK_APP_AGREED))
		sttd_dbus_server_check_app_agreed(conn, msg);

	else if (dbus_message_is_method_call(msg, STT_SERVER_SERVICE_INTERFACE, STT_METHOD_GET_SUPPORT_LANGS))
		sttd_dbus_server_get_support_lang(conn, msg);

	else if (dbus_message_is_method_call(msg, STT_SERVER_SERVICE_INTERFACE, STT_METHOD_GET_CURRENT_LANG))
		sttd_dbus_server_get_default_lang(conn, msg);

	else if (dbus_message_is_method_call(msg, STT_SERVER_SERVICE_INTERFACE, STT_METHOD_IS_TYPE_SUPPORTED))
		sttd_dbus_server_is_recognition_type_supported(conn, msg);


	else if (dbus_message_is_method_call(msg, STT_SERVER_SERVICE_INTERFACE, STT_METHOD_SET_START_SOUND))
		sttd_dbus_server_set_start_sound(conn, msg);

	else if (dbus_message_is_method_call(msg, STT_SERVER_SERVICE_INTERFACE, STT_METHOD_UNSET_START_SOUND))
		sttd_dbus_server_unset_start_sound(conn, msg);

	else if (dbus_message_is_method_call(msg, STT_SERVER_SERVICE_INTERFACE, STT_METHOD_SET_STOP_SOUND))
		sttd_dbus_server_set_stop_sound(conn, msg);

	else if (dbus_message_is_method_call(msg, STT_SERVER_SERVICE_INTERFACE, STT_METHOD_UNSET_STOP_SOUND))
		sttd_dbus_server_unset_stop_sound(conn, msg);


	else if (dbus_message_is_method_call(msg, STT_SERVER_SERVICE_INTERFACE, STT_METHOD_START)) 
		sttd_dbus_server_start(conn, msg);
	
	else if (dbus_message_is_method_call(msg, STT_SERVER_SERVICE_INTERFACE, STT_METHOD_STOP)) 
		sttd_dbus_server_stop(conn, msg);

	else if (dbus_message_is_method_call(msg, STT_SERVER_SERVICE_INTERFACE, STT_METHOD_CANCEL)) 
		sttd_dbus_server_cancel(conn, msg);


	/* free the message */
	dbus_message_unref(msg);

	return ECORE_CALLBACK_RENEW;
}

int sttd_dbus_open_connection()
{
	DBusError err;
	dbus_error_init(&err);

	int ret;

	/* connect to the bus and check for errors */
	g_conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_STTD, "[Dbus ERROR] Fail dbus_bus_get : %s", err.message);
		dbus_error_free(&err); 
	}

	if (NULL == g_conn) { 
		SLOG(LOG_ERROR, TAG_STTD, "[Dbus ERROR] Fail to get dbus connection" );
		return -1;
	}

	/* request our name on the bus and check for errors */
	ret = dbus_bus_request_name(g_conn, STT_SERVER_SERVICE_NAME, DBUS_NAME_FLAG_REPLACE_EXISTING , &err);

	if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Dbus ERROR] Fail to be primary owner");
		return -1;
	}

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_STTD, "[Dbus ERROR] dbus_bus_request_name() : %s", err.message);
		dbus_error_free(&err); 
		return -1;
	}

	/* add a rule for getting signal */
	char rule[128];
	snprintf(rule, 128, "type='signal',interface='%s'", STT_SERVER_SERVICE_INTERFACE);

	/* add a rule for which messages we want to see */
	dbus_bus_add_match(g_conn, rule, &err); /* see signals from the given interface */
	dbus_connection_flush(g_conn);

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_STTD, "[Dbus ERROR] dbus_bus_add_match() : %s", err.message);
		return -1; 
	}

	int fd = 0;
	dbus_connection_get_unix_fd(g_conn, &fd);

	if (!ecore_init()) {
		SLOG(LOG_ERROR, TAG_STTD, "[Dbus ERROR] ecore_init()");
		return -1;
	}

	Ecore_Fd_Handler* fd_handler;
	fd_handler = ecore_main_fd_handler_add(fd, ECORE_FD_READ , (Ecore_Fd_Cb)listener_event_callback, g_conn, NULL, NULL);

	if (NULL == fd_handler) {
		SLOG(LOG_ERROR, TAG_STTD, "[Dbus ERROR] Fail to get fd handler");
		return -1;
	}

	return 0;
}

int sttd_dbus_close_connection()
{
	DBusError err;
	dbus_error_init(&err);

	dbus_bus_release_name (g_conn, STT_SERVER_SERVICE_NAME, &err);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Dbus ERROR] dbus_bus_release_name() : %s", err.message); 
		dbus_error_free(&err); 
		return -1;
	}

	return 0;
}
