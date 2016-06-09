/*
*  Copyright (c) 2011-2016 Samsung Electronics Co., Ltd All Rights Reserved
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
#include "stt_dbus.h"
#include "stt_defs.h"

#include "stt_client.h"

static int g_waiting_time = 3000;
static int g_waiting_short_time = 500;

static Ecore_Fd_Handler* g_fd_handler = NULL;

static DBusConnection* g_conn_sender = NULL;
static DBusConnection* g_conn_listener = NULL;


extern int __stt_cb_error(int uid, int reason, char* err_msg);

extern int __stt_cb_result(int uid, int event, char** data, int data_count, const char* msg);

extern int __stt_cb_set_state(int uid, int state);

extern int __stt_cb_set_volume(int uid, float volume);

static Eina_Bool listener_event_callback(void* data, Ecore_Fd_Handler *fd_handler)
{
	if (NULL == g_conn_listener)	return ECORE_CALLBACK_RENEW;

	dbus_connection_read_write_dispatch(g_conn_listener, 50);

	while (1) {
		DBusMessage* msg = NULL;
		msg = dbus_connection_pop_message(g_conn_listener);

		if (true != dbus_connection_get_is_connected(g_conn_listener)) {
			SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Connection is disconnected");
			break;
		}

		/* loop again if we haven't read a message */
		if (NULL == msg) {
			break;
		}

		SLOG(LOG_DEBUG, TAG_STTC, "[DEBUG] Message is arrived");

		DBusError err;
		dbus_error_init(&err);

		DBusMessage *reply = NULL;

		char if_name[64];
		snprintf(if_name, 64, "%s%d", STT_CLIENT_SERVICE_INTERFACE, getpid());

		if (dbus_message_is_method_call(msg, if_name, STTD_METHOD_HELLO)) {
			SLOG(LOG_DEBUG, TAG_STTC, "===== Get Hello");
			int uid = 0;
			int response = -1;

			dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &uid, DBUS_TYPE_INVALID);
			if (dbus_error_is_set(&err)) {
				SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Get arguments error (%s)", err.message);
				dbus_error_free(&err);
			}

			if (uid > 0) {
				SLOG(LOG_DEBUG, TAG_STTC, "<<<< stt get hello : uid(%d)", uid);

				/* check uid */
				stt_client_s* client = stt_client_get_by_uid(uid);
				if (NULL != client)
					response = 1;
				else
					response = 0;
			} else {
				SLOG(LOG_ERROR, TAG_STTC, "<<<< stt get hello : invalid uid");
			}

			reply = dbus_message_new_method_return(msg);

			if (NULL != reply) {
				dbus_message_append_args(reply, DBUS_TYPE_INT32, &response, DBUS_TYPE_INVALID);

				if (!dbus_connection_send(g_conn_listener, reply, NULL))
					SLOG(LOG_ERROR, TAG_STTC, ">>>> stt get hello : fail to send reply");
				else
					SLOG(LOG_DEBUG, TAG_STTC, ">>>> stt get hello : result(%d)", response);

				dbus_connection_flush(g_conn_listener);
				dbus_message_unref(reply);
			} else {
				SLOG(LOG_ERROR, TAG_STTC, ">>>> stt get hello : fail to create reply message");
			}

			SLOG(LOG_DEBUG, TAG_STTC, "=====");
			SLOG(LOG_DEBUG, TAG_STTC, " ");
		} /* STTD_METHOD_HELLO */

		else if (dbus_message_is_signal(msg, if_name, STTD_METHOD_SET_STATE)) {
			SLOG(LOG_DEBUG, TAG_STTC, "===== Set State");
			int uid = 0;
			int state = -1;

			dbus_message_get_args(msg, &err,
				DBUS_TYPE_INT32, &uid,
				DBUS_TYPE_INT32, &state,
				DBUS_TYPE_INVALID);

			if (dbus_error_is_set(&err)) {
				SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Get arguments error (%s)", err.message);
				dbus_error_free(&err);
			}

			if (uid > 0 && state >= 0) {
				SLOG(LOG_DEBUG, TAG_STTC, "<<<< stt set state : uid(%d), state(%d)", uid, state);
				__stt_cb_set_state(uid, state);
			} else {
				SLOG(LOG_ERROR, TAG_STTC, "<<<< stt set state : invalid uid or state");
			}

			SLOG(LOG_DEBUG, TAG_STTC, "=====");
			SLOG(LOG_DEBUG, TAG_STTC, " ");
		} /* STTD_METHOD_SET_STATE */

		else if (dbus_message_is_signal(msg, if_name, STTD_METHOD_SET_VOLUME)) {
			SLOG(LOG_DEBUG, TAG_STTC, "===== Set volume : signal");
			int uid = 0;
			float volume = 0;

			dbus_message_get_args(msg, &err,
				DBUS_TYPE_INT32, &uid,
				DBUS_TYPE_INT32, &volume,
				DBUS_TYPE_INVALID);

			if (dbus_error_is_set(&err)) {
				SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Get arguments error (%s)", err.message);
				dbus_error_free(&err);
			}

			SLOG(LOG_DEBUG, TAG_STTC, "<<<< stt set volume : uid(%d), volume(%f)", uid, volume); 
			__stt_cb_set_volume(uid, volume);

			/* SLOG(LOG_DEBUG, TAG_STTC, "====="); */
			/* SLOG(LOG_DEBUG, TAG_STTC, " "); */
		} /* STTD_METHOD_SET_VOLUME */

		else if (dbus_message_is_signal(msg, if_name, STTD_METHOD_RESULT)) {
			SLOG(LOG_DEBUG, TAG_STTC, "===== Get Result : signal");
			int uid = 0;
			DBusMessageIter args;

			dbus_message_iter_init(msg, &args);

			/* Get result */
			if (DBUS_TYPE_INT32 == dbus_message_iter_get_arg_type(&args)) {
				dbus_message_iter_get_basic(&args, &uid);
				dbus_message_iter_next(&args);
			}

			stt_client_s* client = NULL;
			client = stt_client_get_by_uid(uid);
			if (NULL != client) {
				char** temp_result = NULL;
				char* temp_msg = NULL;
				char* temp_char = NULL;
				int temp_event = 0;
				int temp_count = 0;
				int temp_result_id = 0;

				/* Get recognition type */
				if (DBUS_TYPE_INT32 == dbus_message_iter_get_arg_type(&args)) {
					dbus_message_iter_get_basic(&args, &temp_event);
					dbus_message_iter_next(&args);
				}

				if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&args)) {
					dbus_message_iter_get_basic(&args, &(temp_msg));
					dbus_message_iter_next(&args);
				}

				/* Get voice size */
				if (DBUS_TYPE_INT32 == dbus_message_iter_get_arg_type(&args)) {
					dbus_message_iter_get_basic(&args, &temp_count);
					dbus_message_iter_next(&args);
				}

				/* Get result id */
				if (DBUS_TYPE_INT32 == dbus_message_iter_get_arg_type(&args)) {
					dbus_message_iter_get_basic(&args, &temp_result_id);
					dbus_message_iter_next(&args);
				}

				SLOG(LOG_DEBUG, TAG_STTC, "<<<< stt get result : uid(%d) event(%d) message(%s) count(%d) result id(%d)",
					uid, temp_event, temp_msg, temp_count, temp_result_id);

				if (temp_count <= 0) {
					__stt_cb_result(uid, temp_event, NULL, 0, temp_msg);
				} else {
					temp_result = (char**)calloc(temp_count, sizeof(char*));

					if (NULL == temp_result)	{
						SLOG(LOG_ERROR, TAG_STTC, "Fail : memory allocation error");
					} else {
						int i = 0;
						for (i = 0; i < temp_count; i++) {
							dbus_message_iter_get_basic(&args, &(temp_char));
							dbus_message_iter_next(&args);

							if (NULL != temp_char) {
								temp_result[i] = strdup(temp_char);
								SLOG(LOG_DEBUG, TAG_STTC, "result[%d] : %s", i, temp_result[i]);
							}
						}

						__stt_cb_result(uid, temp_event, temp_result, temp_count, temp_msg);

						for (i = 0; i < temp_count; i++) {
							if (NULL != temp_result[i])
								free(temp_result[i]);
						}

						free(temp_result);
					}
				}
			} else {
				SLOG(LOG_ERROR, TAG_STTC, "<<<< stt get result : invalid uid");
			}

			SLOG(LOG_DEBUG, TAG_STTC, "=====");
			SLOG(LOG_DEBUG, TAG_STTC, " ");
		} /* STTD_METHOD_RESULT */

		else if (dbus_message_is_signal(msg, if_name, STTD_METHOD_ERROR)) {
			SLOG(LOG_DEBUG, TAG_STTC, "===== Get Error");
			int uid;
			int reason;
			char* err_msg;

			dbus_message_get_args(msg, &err,
				DBUS_TYPE_INT32, &uid,
				DBUS_TYPE_INT32, &reason,
				DBUS_TYPE_STRING, &err_msg,
				DBUS_TYPE_INVALID);

			if (dbus_error_is_set(&err)) {
				SLOG(LOG_ERROR, TAG_STTC, "<<<< stt Get Error message : Get arguments error (%s)", err.message);
				dbus_error_free(&err);
			} else {
				SLOG(LOG_DEBUG, TAG_STTC, "<<<< stt Get Error message : uid(%d), reason(%d), err_msg(%s)", uid, reason, (NULL == err_msg) ? "NULL" : err_msg);
				__stt_cb_error(uid, reason, err_msg);
			}

			SLOG(LOG_DEBUG, TAG_STTC, "=====");
			SLOG(LOG_DEBUG, TAG_STTC, " ");
		} /* STTD_METHOD_ERROR */

		else {
			SLOG(LOG_DEBUG, TAG_STTC, "Message is NOT valid");
			dbus_message_unref(msg);
			break;
		}

		/* free the message */
		dbus_message_unref(msg);
	} /* while */

	return ECORE_CALLBACK_RENEW;
}

int stt_dbus_open_connection()
{
	if (NULL != g_conn_sender && NULL != g_conn_listener) {
		SLOG(LOG_WARN, TAG_STTC, "already existed connection ");
		return 0;
	}

	DBusError err;

	/* initialise the error value */
	dbus_error_init(&err);

	/* connect to the DBUS system bus, and check for errors */
	if (NULL == g_conn_sender) {
		g_conn_sender = dbus_bus_get_private(DBUS_BUS_SESSION, &err);
		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Dbus Connection Error (%s)", err.message);
			dbus_error_free(&err);
		}
	}

	if (NULL == g_conn_sender) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] fail to get dbus connection");
		return STT_ERROR_OPERATION_FAILED;
	}

	dbus_connection_set_exit_on_disconnect(g_conn_sender, false);

	/* connect to the DBUS system bus, and check for errors */
	if (NULL == g_conn_listener) {
		g_conn_listener = dbus_bus_get_private(DBUS_BUS_SESSION, &err);
		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_STTC, "Dbus Connection Error (%s)", err.message);
			dbus_error_free(&err);
		}
	}

	if (NULL == g_conn_listener) {
		SLOG(LOG_ERROR, TAG_STTC, "Fail to get dbus connection");
		return STT_ERROR_OPERATION_FAILED;
	}

	int pid = getpid();

	char service_name[64];
	memset(service_name, '\0', 64);
	snprintf(service_name, 64, "%s%d", STT_CLIENT_SERVICE_NAME, pid);

	SLOG(LOG_DEBUG, TAG_STTC, "service name is %s", service_name);

	/* register our name on the bus, and check for errors */
	dbus_bus_request_name(g_conn_listener, service_name, DBUS_NAME_FLAG_REPLACE_EXISTING , &err);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_STTC, "Name Error (%s)", err.message); 
		dbus_error_free(&err); 
	}

	char rule[128] = {0, };
	snprintf(rule, 128, "type='signal',interface='%s%d'", STT_CLIENT_SERVICE_INTERFACE, pid);

	/* add a rule for which messages we want to see */
	dbus_bus_add_match(g_conn_listener, rule, &err);
	dbus_connection_flush(g_conn_listener);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_STTC, "Match Error (%s)", err.message);
		dbus_error_free(&err);
		return STT_ERROR_OPERATION_FAILED;
	}

	int fd = 0;
	if (true != dbus_connection_get_unix_fd(g_conn_listener, &fd)) {
		SLOG(LOG_ERROR, TAG_STTC, "Fail to get fd from dbus");
		return STT_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, "Get fd from dbus : %d", fd);
	}

	g_fd_handler = ecore_main_fd_handler_add(fd, ECORE_FD_READ, (Ecore_Fd_Cb)listener_event_callback, g_conn_listener, NULL, NULL);
	if (NULL == g_fd_handler) {
		SLOG(LOG_ERROR, TAG_STTC, "fail to get fd handler from ecore");
		return STT_ERROR_OPERATION_FAILED;
	}

	return 0;
}

int stt_dbus_close_connection()
{
	DBusError err;
	dbus_error_init(&err);

	if (NULL != g_fd_handler) {
		ecore_main_fd_handler_del(g_fd_handler);
		g_fd_handler = NULL;
	}

	int pid = getpid();

	char service_name[64];
	memset(service_name, '\0', 64);
	snprintf(service_name, 64, "%s%d", STT_CLIENT_SERVICE_NAME, pid);

	dbus_bus_release_name(g_conn_listener, service_name, &err);
	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Release name Error (%s)", err.message);
		dbus_error_free(&err);
	}

	dbus_connection_close(g_conn_sender);
	dbus_connection_close(g_conn_listener);

	dbus_connection_unref(g_conn_sender);
	dbus_connection_unref(g_conn_listener);

	g_conn_sender = NULL;
	g_conn_listener = NULL;

	return 0;
}

int stt_dbus_reconnect()
{
	bool sender_connected = dbus_connection_get_is_connected(g_conn_sender);
	bool listener_connected = dbus_connection_get_is_connected(g_conn_listener);
	SLOG(LOG_DEBUG, TAG_STTC, "[DBUS] Sender(%s) Listener(%s)",
		sender_connected ? "Connected" : "Not connected", listener_connected ? "Connected" : "Not connected");

	if (false == sender_connected || false == listener_connected) {
		stt_dbus_close_connection();

		if (0 != stt_dbus_open_connection()) {
			SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to reconnect");
			return -1;
		}

		SLOG(LOG_DEBUG, TAG_STTC, "[DBUS] Reconnect");
	}

	return 0;
}

int stt_dbus_request_hello()
{
	DBusMessage* msg;

	msg = dbus_message_new_method_call(
		STT_SERVER_SERVICE_NAME, 
		STT_SERVER_SERVICE_OBJECT_PATH, 
		STT_SERVER_SERVICE_INTERFACE, 
		STT_METHOD_HELLO);

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_STTC, ">>>> Request stt hello : Fail to make message");
		return STT_ERROR_OPERATION_FAILED;
	}

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg = NULL;
	int result = 0;

	if (g_conn_sender) {
		result_msg = dbus_connection_send_with_reply_and_block(g_conn_sender, msg, g_waiting_short_time, &err);
		dbus_message_unref(msg);
		if (dbus_error_is_set(&err)) {
			dbus_error_free(&err);
		}

		if (NULL != result_msg) {
			dbus_message_unref(result_msg);

			if (dbus_error_is_set(&err)) {
				SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Get arguments error (%s)", err.message);
				dbus_error_free(&err);
			}

			SLOG(LOG_DEBUG, TAG_STTC, "<<<< stt hello");
		} else {
			if (dbus_error_is_set(&err)) {
				SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Get arguments error (%s)", err.message);
				dbus_error_free(&err);
			}

			SLOG(LOG_ERROR, TAG_STTC, "STT_ERROR_TIME_OUT");
			result = STT_ERROR_TIMED_OUT;
		}
	} else {
		SLOG(LOG_WARN, TAG_STTC, "[WARN] dbus connection handle is null (%p)", g_conn_sender);
		result = STT_ERROR_OPERATION_FAILED;
	}

	return result;
}


int stt_dbus_request_initialize(int uid, bool* silence_supported, bool* credential_needed)
{
	DBusMessage* msg;

	msg = dbus_message_new_method_call(
		STT_SERVER_SERVICE_NAME, 
		STT_SERVER_SERVICE_OBJECT_PATH, 
		STT_SERVER_SERVICE_INTERFACE, 
		STT_METHOD_INITIALIZE);

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_STTC, ">>>> stt initialize : Fail to make message");
		return STT_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, ">>>> stt initialize : uid(%d)", uid);
	}

	int pid = getpid();
	dbus_message_append_args(msg,
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_INT32, &uid,
		DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = STT_ERROR_OPERATION_FAILED;

	if (g_conn_sender) {
		result_msg = dbus_connection_send_with_reply_and_block(g_conn_sender, msg, g_waiting_time, &err);
		dbus_message_unref(msg);
		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Send Error (%s)", err.message);
			dbus_error_free(&err);
		}

		if (NULL != result_msg) {
			dbus_message_get_args(result_msg, &err,
				DBUS_TYPE_INT32, &result,
				DBUS_TYPE_INT32, silence_supported,
				DBUS_TYPE_INT32, credential_needed,
				DBUS_TYPE_INVALID);

			if (dbus_error_is_set(&err)) {
				SLOG(LOG_ERROR, TAG_STTC, "<<<< Get arguments error (%s)", err.message);
				dbus_error_free(&err);
				result = STT_ERROR_OPERATION_FAILED;
			}

			if (0 == result) {
				SLOG(LOG_DEBUG, TAG_STTC, "<<<< stt initialize : result = %d, silence(%d), credential(%d)",
					result, *silence_supported, *credential_needed);
			} else {
				SLOG(LOG_ERROR, TAG_STTC, "<<<< stt initialize : result = %d", result);
			}

			dbus_message_unref(result_msg);
		} else {
			SLOG(LOG_ERROR, TAG_STTC, "<<<< Result message is NULL");
			stt_dbus_reconnect();
			result = STT_ERROR_TIMED_OUT;
		}
	} else {
		SLOG(LOG_WARN, TAG_STTC, "[WARN] dbus connection handle is null (%p)", g_conn_sender);
		result = STT_ERROR_OPERATION_FAILED;
	}

	return result;
}

int stt_dbus_request_finalize(int uid)
{
	DBusMessage* msg;

	msg = dbus_message_new_method_call(
		STT_SERVER_SERVICE_NAME, 
		STT_SERVER_SERVICE_OBJECT_PATH, 
		STT_SERVER_SERVICE_INTERFACE, 
		STT_METHOD_FINALIZE);

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_STTC, ">>>> stt finalize : Fail to make message");
		return STT_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, ">>>> stt finalize : uid(%d)", uid);
	}

	dbus_message_append_args(msg, DBUS_TYPE_INT32, &uid, DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = STT_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn_sender, msg, g_waiting_short_time, &err);
	dbus_message_unref(msg);
	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Send Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err, 
				DBUS_TYPE_INT32, &result,
				DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_STTC, "<<<< Get arguments error (%s)", err.message);
			dbus_error_free(&err);
			result = STT_ERROR_OPERATION_FAILED;
		}

		dbus_message_unref(result_msg);

		if (0 == result) {
			SLOG(LOG_DEBUG, TAG_STTC, "<<<< stt finalize : result = %d", result);
		} else {
			SLOG(LOG_ERROR, TAG_STTC, "<<<< stt finalize : result = %d", result);
		}
	} else {
		SLOG(LOG_ERROR, TAG_STTC, "<<<< Result message is NULL");
		stt_dbus_reconnect();
		result = STT_ERROR_TIMED_OUT;
	}

	return result;
}

int stt_dbus_request_set_current_engine(int uid, const char* engine_id, bool* silence_supported, bool* credential_needed)
{
	DBusMessage* msg;

	msg = dbus_message_new_method_call(
		STT_SERVER_SERVICE_NAME, 
		STT_SERVER_SERVICE_OBJECT_PATH, 
		STT_SERVER_SERVICE_INTERFACE, 
		STT_METHOD_SET_CURRENT_ENGINE);

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_STTC, ">>>> stt set engine : Fail to make message");
		return STT_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, ">>>> stt set engine : uid(%d)", uid);
	}

	dbus_message_append_args(msg,
		DBUS_TYPE_INT32, &uid,
		DBUS_TYPE_STRING, &engine_id,
		DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = STT_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn_sender, msg, g_waiting_time, &err);
	dbus_message_unref(msg);
	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Send Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err, 
			DBUS_TYPE_INT32, &result, 
			DBUS_TYPE_INT32, silence_supported,
			DBUS_TYPE_INT32, credential_needed,
			DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_STTC, "<<<< Get arguments error (%s)", err.message);
			dbus_error_free(&err); 
			result = STT_ERROR_OPERATION_FAILED;
		}

		dbus_message_unref(result_msg);

		if (0 == result) {
			SLOG(LOG_DEBUG, TAG_STTC, "<<<< stt set engine : result = %d , silence(%d), credential(%d)",
				result, *silence_supported, *credential_needed);
		} else {
			SLOG(LOG_ERROR, TAG_STTC, "<<<< stt set engine : result = %d", result);
		}
	} else {
		SLOG(LOG_ERROR, TAG_STTC, "<<<< Result message is NULL");
		stt_dbus_reconnect();
		result = STT_ERROR_TIMED_OUT;
	}

	return result;
}

int stt_dbus_request_check_app_agreed(int uid, const char* appid, bool* value)
{
	if (NULL == appid || NULL == value) {
		SLOG(LOG_ERROR, TAG_STTC, "Input parameter is NULL");
		return STT_ERROR_INVALID_PARAMETER;
	}

	DBusMessage* msg;

	msg = dbus_message_new_method_call(
		STT_SERVER_SERVICE_NAME, 
		STT_SERVER_SERVICE_OBJECT_PATH, 
		STT_SERVER_SERVICE_INTERFACE, 
		STT_METHOD_CHECK_APP_AGREED);

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_STTC, ">>>> stt check app agreed : Fail to make message");
		return STT_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, ">>>> stt check app agreed : uid(%d) appid(%s)", uid, appid);
	}

	dbus_message_append_args(msg,
		DBUS_TYPE_INT32, &uid,
		DBUS_TYPE_STRING, &appid,
		DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = STT_ERROR_OPERATION_FAILED;
	int available = -1;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn_sender, msg, g_waiting_time, &err);
	dbus_message_unref(msg);
	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Send Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
			DBUS_TYPE_INT32, &result,
			DBUS_TYPE_INT32, &available,
			DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_STTC, "<<<< Get arguments error (%s)", err.message);
			dbus_error_free(&err);
			result = STT_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);

		if (0 == result) {
			*value = (bool)available;
			SLOG(LOG_DEBUG, TAG_STTC, "<<<< stt check app agreed : result = %d, available = %d", result, *value);
		} else {
			SLOG(LOG_ERROR, TAG_STTC, "<<<< stt check app agreed : result = %d", result);
		}
	} else {
		SLOG(LOG_ERROR, TAG_STTC, "<<<< Result message is NULL");
		stt_dbus_reconnect();
		result = STT_ERROR_TIMED_OUT;
	}

	return result;
}

int stt_dbus_request_get_support_langs(int uid, stt_h stt, stt_supported_language_cb callback, void* user_data)
{
	if (NULL == stt || NULL == callback) {
		SLOG(LOG_ERROR, TAG_STTC, "Input parameter is NULL");
		return STT_ERROR_INVALID_PARAMETER;
	}

	DBusMessage* msg;

	msg = dbus_message_new_method_call(
		STT_SERVER_SERVICE_NAME, 
		STT_SERVER_SERVICE_OBJECT_PATH, 
		STT_SERVER_SERVICE_INTERFACE, 
		STT_METHOD_GET_SUPPORT_LANGS);

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_STTC, ">>>> stt get supported languages : Fail to make message");
		return STT_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, ">>>> stt get supported languages : uid(%d)", uid);
	}

	dbus_message_append_args(msg, DBUS_TYPE_INT32, &uid, DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	DBusMessageIter args;
	int result = STT_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn_sender, msg, g_waiting_time, &err);
	dbus_message_unref(msg);
	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Send Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		if (dbus_message_iter_init(result_msg, &args)) {
			/* Get result */
			if (DBUS_TYPE_INT32 == dbus_message_iter_get_arg_type(&args)) {
				dbus_message_iter_get_basic(&args, &result);
				dbus_message_iter_next(&args);
			}

			if (0 == result) {
				SLOG(LOG_DEBUG, TAG_STTC, "<<<< stt get support languages : result = %d", result);

				/* Get voice size */
				int size = 0;
				if (DBUS_TYPE_INT32 == dbus_message_iter_get_arg_type(&args)) {
					dbus_message_iter_get_basic(&args, &size);
					dbus_message_iter_next(&args);
				}

				if (0 >= size) {
					SLOG(LOG_ERROR, TAG_STTC, "<<<< stt size of language error : size = %d", size);
				} else {
					int i = 0;
					char* temp_lang;

					for (i = 0; i < size; i++) {
						dbus_message_iter_get_basic(&args, &(temp_lang));
						dbus_message_iter_next(&args);

						if (true != callback(stt, temp_lang, user_data)) {
							break;
						}
					}
				}
			} else {
				SLOG(LOG_ERROR, TAG_STTC, "<<<< stt get support languages : result = %d", result);
			}
		}
		dbus_message_unref(result_msg);
	} else {
		SLOG(LOG_ERROR, TAG_STTC, "<<<< stt get support languages : result message is NULL");
		stt_dbus_reconnect();
		result = STT_ERROR_TIMED_OUT;
	}

	return result;
}

int stt_dbus_request_get_default_lang(int uid, char** language)
{
	if (NULL == language) {
		SLOG(LOG_ERROR, TAG_STTC, "Input parameter is NULL");
		return STT_ERROR_INVALID_PARAMETER;
	}

	DBusMessage* msg;

	msg = dbus_message_new_method_call(
		STT_SERVER_SERVICE_NAME, 
		STT_SERVER_SERVICE_OBJECT_PATH, 
		STT_SERVER_SERVICE_INTERFACE, 
		STT_METHOD_GET_CURRENT_LANG);

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_STTC, ">>>> stt get default language : Fail to make message");
		return STT_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, ">>>> stt get default language  : uid(%d)", uid);
	}

	dbus_message_append_args(msg,
		DBUS_TYPE_INT32, &uid,
		DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = STT_ERROR_OPERATION_FAILED;
	char* temp_lang = NULL;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn_sender, msg, g_waiting_time, &err);
	dbus_message_unref(msg);
	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Send Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
			DBUS_TYPE_INT32, &result,
			DBUS_TYPE_STRING, &temp_lang,
			DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_STTC, "<<<< Get arguments error (%s)", err.message);
			dbus_error_free(&err);
			result = STT_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);

		if (0 == result) {
			*language = strdup(temp_lang);
			SLOG(LOG_DEBUG, TAG_STTC, "<<<< stt get default language : result = %d, language = %s", result, *language);
		} else {
			SLOG(LOG_ERROR, TAG_STTC, "<<<< stt get default language : result = %d", result);
		}
	} else {
		SLOG(LOG_ERROR, TAG_STTC, "<<<< Result message is NULL");
		stt_dbus_reconnect();
		result = STT_ERROR_TIMED_OUT;
	}

	return result;
}

int stt_dbus_request_set_private_data(int uid, const char* key, const char* data)
{
	if (NULL == key || NULL == data) {
		SLOG(LOG_ERROR, TAG_STTC, "Input parameter is NULL");
		return STT_ERROR_INVALID_PARAMETER;
	}

	DBusMessage* msg;

	msg = dbus_message_new_method_call(
		STT_SERVER_SERVICE_NAME,
		STT_SERVER_SERVICE_OBJECT_PATH,
		STT_SERVER_SERVICE_INTERFACE,
		STT_METHOD_SET_PRIVATE_DATA);

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_STTC, ">>>> stt set private data : Fail to make message");
		return STT_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, ">>>> stt set private data : uid(%d)", uid);
	}

	if (true != dbus_message_append_args(msg,
			DBUS_TYPE_INT32, &uid,
			DBUS_TYPE_STRING, &key,
			DBUS_TYPE_STRING, &data,
			DBUS_TYPE_INVALID)) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to append args");
		return STT_ERROR_OPERATION_FAILED;
	}

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = STT_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn_sender, msg, g_waiting_time, &err);
	dbus_message_unref(msg);
	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Send Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
			DBUS_TYPE_INT32, &result,
			DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_STTC, "<<<< Get arguments error (%s)", err.message);
			dbus_error_free(&err);
			result = STT_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);

		if (0 == result) {
			SLOG(LOG_DEBUG, TAG_STTC, "<<<< stt set private data : result = %d", result);
		} else {
			SLOG(LOG_ERROR, TAG_STTC, "<<<< stt set private data : result = %d", result);
		}
	} else {
		SLOG(LOG_ERROR, TAG_STTC, "<<<< Result message is NULL");
		stt_dbus_reconnect();
		result = STT_ERROR_TIMED_OUT;
	}

	return result;
}

int stt_dbus_request_get_private_data(int uid, const char* key, char** data)
{
	if (NULL == key || NULL == data) {
		SLOG(LOG_ERROR, TAG_STTC, "Input parameter is NULL");
		return STT_ERROR_INVALID_PARAMETER;
	}

	DBusMessage* msg;

	msg = dbus_message_new_method_call(
		STT_SERVER_SERVICE_NAME,
		STT_SERVER_SERVICE_OBJECT_PATH,
		STT_SERVER_SERVICE_INTERFACE,
		STT_METHOD_GET_PRIVATE_DATA);

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_STTC, ">>>> stt get private data : Fail to make message");
		return STT_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, ">>>> stt get private data : uid(%d)", uid);
	}

	if (true != dbus_message_append_args(msg,
			DBUS_TYPE_INT32, &uid,
			DBUS_TYPE_STRING, &key,
			DBUS_TYPE_INVALID)) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to append args");
		return STT_ERROR_OPERATION_FAILED;
	}

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = STT_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn_sender, msg, g_waiting_time, &err);
	dbus_message_unref(msg);
	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Send Error (%s)", err.message);
		dbus_error_free(&err);
	}

	char* temp = NULL;
	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
			DBUS_TYPE_INT32, &result,
			DBUS_TYPE_STRING, &temp,
			DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_STTC, "<<<< Get arguments error (%s)", err.message);
			dbus_error_free(&err);
			result = STT_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);

		if (0 == result) {
			SLOG(LOG_DEBUG, TAG_STTC, "<<<< stt get private data : result = %d", result);
			if (NULL != temp) {
				*data = strdup(temp);
			}
		} else {
			SLOG(LOG_ERROR, TAG_STTC, "<<<< stt get private data : result = %d", result);
		}
	} else {
		SLOG(LOG_ERROR, TAG_STTC, "<<<< Result message is NULL");
		stt_dbus_reconnect();
		result = STT_ERROR_TIMED_OUT;
	}

	return result;
}



int stt_dbus_request_is_recognition_type_supported(int uid, const char* type, bool* support)
{
	if (NULL == support || NULL == type) {
		SLOG(LOG_ERROR, TAG_STTC, "Input parameter is NULL");
		return STT_ERROR_INVALID_PARAMETER;
	}

	DBusMessage* msg;

	msg = dbus_message_new_method_call(
		   STT_SERVER_SERVICE_NAME, 
		   STT_SERVER_SERVICE_OBJECT_PATH, 
		   STT_SERVER_SERVICE_INTERFACE, 
		   STT_METHOD_IS_TYPE_SUPPORTED);

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_STTC, ">>>> stt is partial result supported : Fail to make message");
		return STT_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, ">>>> stt is recognition type supported : uid(%d) type(%s)", uid, type);
	}

	dbus_message_append_args(msg,
		DBUS_TYPE_INT32, &uid,
		DBUS_TYPE_STRING, &type,
		DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = STT_ERROR_OPERATION_FAILED;
	int result_support = -1;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn_sender, msg, g_waiting_time, &err);
	dbus_message_unref(msg);
	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Send Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
			DBUS_TYPE_INT32, &result,
			DBUS_TYPE_INT32, &result_support,
			DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_STTC, "<<<< Get arguments error (%s)", err.message);
			dbus_error_free(&err);
			result = STT_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);

		if (0 == result) {
			*support = (bool)result_support;
			SLOG(LOG_DEBUG, TAG_STTC, "<<<< stt is recognition type supported : result = %d, support = %s", result, *support ? "true" : "false");
		} else {
			SLOG(LOG_ERROR, TAG_STTC, "<<<< stt is recognition type supported : result = %d", result);
		}
	} else {
		SLOG(LOG_ERROR, TAG_STTC, "<<<< Result message is NULL");
		stt_dbus_reconnect();
		result = STT_ERROR_TIMED_OUT;
	}

	return result;
}

int stt_dbus_request_set_start_sound(int uid, const char* file)
{
	if (NULL == file) {
		SLOG(LOG_ERROR, TAG_STTC, "Input parameter is NULL");
		return STT_ERROR_INVALID_PARAMETER;
	}

	DBusMessage* msg;

	msg = dbus_message_new_method_call(
		   STT_SERVER_SERVICE_NAME, 
		   STT_SERVER_SERVICE_OBJECT_PATH, 
		   STT_SERVER_SERVICE_INTERFACE, 
		   STT_METHOD_SET_START_SOUND);

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_STTC, ">>>> stt set start sound : Fail to make message");
		return STT_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, ">>>> stt set start sound : uid(%d) file(%s)", uid, file);
	}

	dbus_message_append_args(msg,
		DBUS_TYPE_INT32, &uid,
		DBUS_TYPE_STRING, &file,
		DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = STT_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn_sender, msg, g_waiting_time, &err);
	dbus_message_unref(msg);
	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Send Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
			DBUS_TYPE_INT32, &result,
			DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_STTC, "<<<< Get arguments error (%s)", err.message);
			dbus_error_free(&err);
			result = STT_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);

		if (0 == result) {
			SLOG(LOG_DEBUG, TAG_STTC, "<<<< stt set start sound : result = %d", result);
		} else {
			SLOG(LOG_ERROR, TAG_STTC, "<<<< stt set start sound : result = %d", result);
		}
	} else {
		SLOG(LOG_ERROR, TAG_STTC, "<<<< Result message is NULL");
		stt_dbus_reconnect();
		result = STT_ERROR_TIMED_OUT;
	}

	return result;
}

int stt_dbus_request_unset_start_sound(int uid)
{
	DBusMessage* msg;

	msg = dbus_message_new_method_call(
		   STT_SERVER_SERVICE_NAME, 
		   STT_SERVER_SERVICE_OBJECT_PATH, 
		   STT_SERVER_SERVICE_INTERFACE, 
		   STT_METHOD_UNSET_START_SOUND);

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_STTC, ">>>> stt unset start sound : Fail to make message");
		return STT_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, ">>>> stt unset start sound : uid(%d)", uid);
	}

	dbus_message_append_args(msg,
		DBUS_TYPE_INT32, &uid,
		DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = STT_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn_sender, msg, g_waiting_time, &err);
	dbus_message_unref(msg);
	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Send Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
			DBUS_TYPE_INT32, &result,
			DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_STTC, "<<<< Get arguments error (%s)", err.message);
			dbus_error_free(&err);
			result = STT_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);

		if (0 == result) {
			SLOG(LOG_DEBUG, TAG_STTC, "<<<< stt unset start sound : result = %d", result);
		} else {
			SLOG(LOG_ERROR, TAG_STTC, "<<<< stt unset start sound : result = %d", result);
		}
	} else {
		SLOG(LOG_ERROR, TAG_STTC, "<<<< Result message is NULL");
		stt_dbus_reconnect();
		result = STT_ERROR_TIMED_OUT;
	}

	return result;
}

int stt_dbus_request_set_stop_sound(int uid, const char* file)
{
	if (NULL == file) {
		SLOG(LOG_ERROR, TAG_STTC, "Input parameter is NULL");
		return STT_ERROR_INVALID_PARAMETER;
	}

	DBusMessage* msg;

	msg = dbus_message_new_method_call(
		   STT_SERVER_SERVICE_NAME, 
		   STT_SERVER_SERVICE_OBJECT_PATH, 
		   STT_SERVER_SERVICE_INTERFACE, 
		   STT_METHOD_SET_STOP_SOUND);

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_STTC, ">>>> stt set stop sound : Fail to make message");
		return STT_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, ">>>> stt set stop sound : uid(%d) file(%s)", uid, file);
	}

	dbus_message_append_args(msg,
		DBUS_TYPE_INT32, &uid,
		DBUS_TYPE_STRING, &file,
		DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = STT_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn_sender, msg, g_waiting_time, &err);
	dbus_message_unref(msg);
	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Send Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
			DBUS_TYPE_INT32, &result,
			DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_STTC, "<<<< Get arguments error (%s)", err.message);
			dbus_error_free(&err);
			result = STT_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);

		if (0 == result) {
			SLOG(LOG_DEBUG, TAG_STTC, "<<<< stt set stop sound : result = %d", result);
		} else {
			SLOG(LOG_ERROR, TAG_STTC, "<<<< stt set stop sound : result = %d", result);
		}
	} else {
		SLOG(LOG_ERROR, TAG_STTC, "<<<< Result message is NULL");
		stt_dbus_reconnect();
		result = STT_ERROR_TIMED_OUT;
	}

	return result;
}

int stt_dbus_request_unset_stop_sound(int uid)
{
	DBusMessage* msg;

	msg = dbus_message_new_method_call(
		   STT_SERVER_SERVICE_NAME, 
		   STT_SERVER_SERVICE_OBJECT_PATH, 
		   STT_SERVER_SERVICE_INTERFACE, 
		   STT_METHOD_UNSET_STOP_SOUND);

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_STTC, ">>>> stt unset stop sound : Fail to make message");
		return STT_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, ">>>> stt unset stop sound : uid(%d)", uid);
	}

	dbus_message_append_args(msg,
		DBUS_TYPE_INT32, &uid,
		DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = STT_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn_sender, msg, g_waiting_time, &err);
	dbus_message_unref(msg);
	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Send Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
			DBUS_TYPE_INT32, &result,
			DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_STTC, "<<<< Get arguments error (%s)", err.message);
			dbus_error_free(&err);
			result = STT_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);

		if (0 == result) {
			SLOG(LOG_DEBUG, TAG_STTC, "<<<< stt unset stop sound : result = %d", result);
		} else {
			SLOG(LOG_ERROR, TAG_STTC, "<<<< stt unset stop sound : result = %d", result);
		}
	} else {
		SLOG(LOG_ERROR, TAG_STTC, "<<<< Result message is NULL");
		stt_dbus_reconnect();
		result = STT_ERROR_TIMED_OUT;
	}

	return result;
}

int stt_dbus_request_start(int uid, const char* lang, const char* type, int silence, const char* appid, const char* credential)
{
	if (NULL == lang || NULL == type || NULL == appid) {
		SLOG(LOG_ERROR, TAG_STTC, "Input parameter is NULL");
		return STT_ERROR_INVALID_PARAMETER;
	}

	DBusMessage* msg;

	/* create a signal & check for errors */
	msg = dbus_message_new_method_call(
		STT_SERVER_SERVICE_NAME,
		STT_SERVER_SERVICE_OBJECT_PATH,
		STT_SERVER_SERVICE_INTERFACE,
		STT_METHOD_START);

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_STTC, ">>>> stt start : Fail to make message");
		return STT_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, ">>>> stt start : uid(%d), language(%s), type(%s)", uid, lang, type);
	}

	char *temp = NULL;
	if (NULL == credential) {
		temp = strdup("NULL");
	} else {
		temp = strdup(credential);
	}

	dbus_message_append_args(msg,
		DBUS_TYPE_INT32, &uid,
		DBUS_TYPE_STRING, &lang,
		DBUS_TYPE_STRING, &type,
		DBUS_TYPE_INT32, &silence,
		DBUS_TYPE_STRING, &appid,
		DBUS_TYPE_STRING, &temp,
		DBUS_TYPE_INVALID);
#if 1
	if (g_conn_sender) {
		dbus_message_set_no_reply(msg, TRUE);

		if (!dbus_connection_send(g_conn_sender, msg, NULL)) {
			SLOG(LOG_ERROR, TAG_STTC, "[Dbus ERROR] <<<< stt start message : Out Of Memory !");
			if (NULL != temp) {
					free(temp);
					temp = NULL;
			}
			return STT_ERROR_OUT_OF_MEMORY;
		} else {
			dbus_connection_flush(g_conn_sender);
		}

		dbus_message_unref(msg);

	} else {
		SLOG(LOG_WARN, TAG_STTC, "[WARN] dbus connection handle is null (%p)", g_conn_sender);
		if (NULL != temp) {
				free(temp);
				temp = NULL;
		}
		return STT_ERROR_OPERATION_FAILED;
	}

	if (NULL != temp) {
			free(temp);
			temp = NULL;
	}
	return 0;
#else
	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = STT_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn_sender, msg, g_waiting_time, &err);
	dbus_message_unref(msg);
	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Send Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
			DBUS_TYPE_INT32, &result,
			DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_STTC, "<<<< Get arguments error (%s)", err.message);
			dbus_error_free(&err);
			result = STT_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);

		if (0 == result) {
			SLOG(LOG_DEBUG, TAG_STTC, "<<<< stt start : result = %d ", result);
		} else {
			SLOG(LOG_ERROR, TAG_STTC, "<<<< stt start : result = %d ", result);
		}
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, "<<<< Result Message is NULL");
		stt_dbus_reconnect();
		result = STT_ERROR_TIMED_OUT;
	}

	return result;
#endif
}

int stt_dbus_request_stop(int uid)
{
	DBusMessage* msg;

	/* create a signal & check for errors */
	msg = dbus_message_new_method_call(
		STT_SERVER_SERVICE_NAME,
		STT_SERVER_SERVICE_OBJECT_PATH,	
		STT_SERVER_SERVICE_INTERFACE,	
		STT_METHOD_STOP);

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_STTC, ">>>> stt stop : Fail to make message");
		return STT_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, ">>>> stt stop : uid(%d)", uid);
	}

	dbus_message_append_args(msg, 
		DBUS_TYPE_INT32, &uid, 
		DBUS_TYPE_INVALID);
#if 1
	if (g_conn_sender) {
		dbus_message_set_no_reply(msg, TRUE);

		if (!dbus_connection_send(g_conn_sender, msg, NULL)) {
			SLOG(LOG_ERROR, TAG_STTC, "[Dbus ERROR] <<<< stt stop message : Out Of Memory !");
			return STT_ERROR_OUT_OF_MEMORY;
		} else {
			dbus_connection_flush(g_conn_sender);
		}

		dbus_message_unref(msg);
	} else {
		SLOG(LOG_WARN, TAG_STTC, "[WARN] dbus connection handle is null (%p)", g_conn_sender);
		return STT_ERROR_OPERATION_FAILED;
	}

	return 0;
#else
	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = STT_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn_sender, msg, g_waiting_time, &err);
	dbus_message_unref(msg);
	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Send Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
			DBUS_TYPE_INT32, &result,
			DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_STTC, "<<<< Get arguments error (%s)", err.message);
			dbus_error_free(&err);
			result = STT_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);

		if (0 == result) {
			SLOG(LOG_DEBUG, TAG_STTC, "<<<< stt stop : result = %d ", result);
		} else {
			SLOG(LOG_ERROR, TAG_STTC, "<<<< stt stop : result = %d ", result);
		}
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, "<<<< Result Message is NULL");
		stt_dbus_reconnect();
		result = STT_ERROR_TIMED_OUT;
	}

	return result;
#endif
}

int stt_dbus_request_cancel(int uid)
{
	DBusMessage* msg;

	/* create a signal & check for errors */
	msg = dbus_message_new_method_call(
		STT_SERVER_SERVICE_NAME,
		STT_SERVER_SERVICE_OBJECT_PATH,	/* object name of the signal */
		STT_SERVER_SERVICE_INTERFACE,	/* interface name of the signal */
		STT_METHOD_CANCEL);	/* name of the signal */

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_STTC, ">>>> stt cancel : Fail to make message");
		return STT_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, ">>>> stt cancel : uid(%d)", uid);
	}

	dbus_message_append_args(msg, 
		DBUS_TYPE_INT32, &uid, 
		DBUS_TYPE_INVALID);
#if 1
	if (g_conn_sender) {
		dbus_message_set_no_reply(msg, TRUE);

		if (!dbus_connection_send(g_conn_sender, msg, NULL)) {
			SLOG(LOG_ERROR, TAG_STTC, "[Dbus ERROR] <<<< stt stop message : Out Of Memory !");
			return STT_ERROR_OUT_OF_MEMORY;
		} else {
			dbus_connection_flush(g_conn_sender);
		}

		dbus_message_unref(msg);
	} else {
		SLOG(LOG_WARN, TAG_STTC, "[WARN] dbus connection handle is null (%p)", g_conn_sender);
		return STT_ERROR_OPERATION_FAILED;
	}

	return 0;
#else
	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = STT_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn_sender, msg, g_waiting_time, &err);
	dbus_message_unref(msg);
	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Send Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
			DBUS_TYPE_INT32, &result,
			DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_STTC, "<<<< Get arguments error (%s)", err.message);
			dbus_error_free(&err);
			result = STT_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);

		if (0 == result) {
			SLOG(LOG_DEBUG, TAG_STTC, "<<<< stt cancel : result = %d ", result);
		} else {
			SLOG(LOG_ERROR, TAG_STTC, "<<<< stt cancel : result = %d ", result);
		}
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, "<<<< Result Message is NULL");
		stt_dbus_reconnect();
		result = STT_ERROR_TIMED_OUT;
	}

	return result;
#endif
}
