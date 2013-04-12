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


#include "stt_main.h"
#include "stt_dbus.h"
#include "stt_defs.h"

#include <Ecore.h>
#include "stt_client.h"

static int g_waiting_time = 1500;
static int g_waiting_start_time = 2000;

static Ecore_Fd_Handler* g_fd_handler = NULL;

static DBusConnection* g_conn = NULL;


extern int __stt_cb_error(int uid, int reason);

extern int __stt_cb_result(int uid, const char* type, const char** data, int data_count, const char* msg);
	
extern int __stt_cb_partial_result(int uid, const char* data);

extern int __stt_cb_set_state(int uid, int state);

static Eina_Bool listener_event_callback(void* data, Ecore_Fd_Handler *fd_handler)
{
	DBusConnection* conn = (DBusConnection*)data;
	DBusMessage* msg = NULL;
	DBusMessage *reply = NULL;

	if (NULL == conn)
		return ECORE_CALLBACK_RENEW;

	dbus_connection_read_write_dispatch(conn, 50);

	msg = dbus_connection_pop_message(conn);

	/* loop again if we haven't read a message */
	if (NULL == msg) { 
		return ECORE_CALLBACK_RENEW;
	}

	DBusError err;
	dbus_error_init(&err);

	char if_name[64];
	snprintf(if_name, 64, "%s%d", STT_CLIENT_SERVICE_INTERFACE, getpid());

	if (dbus_message_is_method_call(msg, if_name, STTD_METHOD_HELLO)) {
		SLOG(LOG_DEBUG, TAG_STTC, "===== Get Hello");
		int uid = 0;
		int response = -1;

		dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &uid, DBUS_TYPE_INVALID);

		if (uid > 0) {
			SLOG(LOG_DEBUG, TAG_STTC, "<<<< stt get hello : uid(%d) \n", uid);
			
			/* check uid */
			stt_client_s* client = stt_client_get_by_uid(uid);
			if( NULL != client ) 
				response = 1;
			else 
				response = 0;
		} else {
			SLOG(LOG_ERROR, TAG_STTC, "<<<< stt get hello : invalid uid \n");
		}

		reply = dbus_message_new_method_return(msg);
		
		if (NULL != reply) {
			dbus_message_append_args(reply, DBUS_TYPE_INT32, &response, DBUS_TYPE_INVALID);

			if (!dbus_connection_send(conn, reply, NULL))
				SLOG(LOG_ERROR, TAG_STTC, ">>>> stt get hello : fail to send reply");
			else 
				SLOG(LOG_DEBUG, TAG_STTC, ">>>> stt get hello : result(%d)", response);

			dbus_connection_flush(conn);
			dbus_message_unref(reply); 
		} else {
			SLOG(LOG_ERROR, TAG_STTC, ">>>> stt get hello : fail to create reply message");
		}
		
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
	} /* STTD_METHOD_HELLO */

	else if (dbus_message_is_method_call(msg, if_name, STTD_METHOD_SET_STATE)) {
		SLOG(LOG_DEBUG, TAG_STTC, "===== Set State");
		int uid = 0;
		int response = -1;
		int state = -1;

		dbus_message_get_args(msg, &err, 
			DBUS_TYPE_INT32, &uid, 
			DBUS_TYPE_INT32, &state,
			DBUS_TYPE_INVALID);

		if (uid > 0 && state >= 0) {
			SLOG(LOG_DEBUG, TAG_STTC, "<<<< stt set state : uid(%d), state(%d)", uid, state);

			response = __stt_cb_set_state(uid, state);
		} else {
			SLOG(LOG_ERROR, TAG_STTC, "<<<< stt set state : invalid uid or state");
		}

		reply = dbus_message_new_method_return(msg);

		if (NULL != reply) {
			dbus_message_append_args(reply, DBUS_TYPE_INT32, &response, DBUS_TYPE_INVALID);

			if (!dbus_connection_send(conn, reply, NULL))
				SLOG(LOG_ERROR, TAG_STTC, ">>>> stt set state : fail to send reply");
			else 
				SLOG(LOG_DEBUG, TAG_STTC, ">>>> stt set state : result(%d)", response);

			dbus_connection_flush(conn);
			dbus_message_unref(reply); 
		} else {
			SLOG(LOG_ERROR, TAG_STTC, ">>>> stt set state : fail to create reply message");
		}

		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
	} /* STTD_METHOD_SET_STATE */

	else if (dbus_message_is_method_call(msg, if_name, STTD_METHOD_GET_STATE)) {
		SLOG(LOG_DEBUG, TAG_STTC, "===== Get state");
		int uid = 0;
		int response = -1;

		dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &uid, DBUS_TYPE_INVALID);

		if (uid > 0) {
			SLOG(LOG_DEBUG, TAG_STTC, "<<<< stt get state : uid(%d) \n", uid);

			/* check state */
			stt_client_s* client = stt_client_get_by_uid(uid);
			if( NULL != client ) 
				response = client->current_state;
			else 
				SLOG(LOG_ERROR, TAG_STTC, "invalid uid \n");
			
		} else {
			SLOG(LOG_ERROR, TAG_STTC, "<<<< stt get state : invalid uid \n");
		}

		reply = dbus_message_new_method_return(msg);

		if (NULL != reply) {
			dbus_message_append_args(reply, DBUS_TYPE_INT32, &response, DBUS_TYPE_INVALID);

			if (!dbus_connection_send(conn, reply, NULL))
				SLOG(LOG_ERROR, TAG_STTC, ">>>> stt get state : fail to send reply");
			else 
				SLOG(LOG_DEBUG, TAG_STTC, ">>>> stt get state : result(%d)", response);

			dbus_connection_flush(conn);
			dbus_message_unref(reply); 
		} else {
			SLOG(LOG_ERROR, TAG_STTC, ">>>> stt get hello : fail to create reply message");
		}

		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
	} /* STTD_METHOD_GET_STATE */

	else if (dbus_message_is_method_call(msg, if_name, STTD_METHOD_RESULT)) {
		SLOG(LOG_DEBUG, TAG_STTC, "===== Get Result");
		int uid = 0;
		DBusMessageIter args;
			
		dbus_message_iter_init(msg, &args);
		
		/* Get result */
		if (DBUS_TYPE_INT32 == dbus_message_iter_get_arg_type(&args)) {
			dbus_message_iter_get_basic(&args, &uid);
			dbus_message_iter_next(&args);
		}
		
		if (uid > 0) {
			SLOG(LOG_DEBUG, TAG_STTC, "<<<< stt get result : uid(%d) \n", uid);
			char** temp_result;
			char* temp_msg = NULL;
			char* temp_char = NULL;
			char* temp_type = 0;
			int temp_count = 0;

			/* Get recognition type */
			if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&args)) {
				dbus_message_iter_get_basic(&args, &temp_type);
				dbus_message_iter_next(&args);
			}

			if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&args)) {
				dbus_message_iter_get_basic(&args, &(temp_msg) );
				dbus_message_iter_next(&args);
			}

			/* Get voice size */
			if (DBUS_TYPE_INT32 == dbus_message_iter_get_arg_type(&args)) {
				dbus_message_iter_get_basic(&args, &temp_count);
				dbus_message_iter_next(&args);
			}

			if (temp_count <= 0) {
				SLOG(LOG_WARN, TAG_STTC, "Result count is 0");
				__stt_cb_result(uid, temp_type, NULL, 0, temp_msg);
			} else {
				temp_result = g_malloc0(temp_count * sizeof(char*));

				if (NULL == temp_result)	{
					SLOG(LOG_ERROR, TAG_STTC, "Fail : memory allocation error \n");
				} else {
					int i = 0;
					for (i = 0;i < temp_count;i++) {
						dbus_message_iter_get_basic(&args, &(temp_char) );
						dbus_message_iter_next(&args);
						
						if (NULL != temp_char)
							temp_result[i] = strdup(temp_char);
					}
		
					__stt_cb_result(uid, temp_type, (const char**)temp_result, temp_count, temp_msg);

					for (i = 0;i < temp_count;i++) {
						if (NULL != temp_result[i])
							free(temp_result[i]);
					}

					g_free(temp_result);
				}
			}	
		} else {
			SLOG(LOG_ERROR, TAG_STTC, "<<<< stt get result : invalid uid \n");
		} 
		
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
	}/* STTD_METHOD_RESULT */

	else if (dbus_message_is_method_call(msg, if_name, STTD_METHOD_PARTIAL_RESULT)) {
		SLOG(LOG_DEBUG, TAG_STTC, "===== Get Partial Result");
		int uid = 0;
		DBusMessageIter args;

		dbus_message_iter_init(msg, &args);

		/* Get result */
		if (DBUS_TYPE_INT32 == dbus_message_iter_get_arg_type(&args)) {
			dbus_message_iter_get_basic(&args, &uid);
			dbus_message_iter_next(&args);
		}

		if (uid > 0) {
			SLOG(LOG_DEBUG, TAG_STTC, "<<<< stt get partial result : uid(%d) \n", uid);
			char* temp_char = NULL;

			if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&args)) {
				dbus_message_iter_get_basic(&args, &(temp_char) );
				dbus_message_iter_next(&args);
			}

			__stt_cb_partial_result(uid, temp_char);
		} else {
			SLOG(LOG_ERROR, TAG_STTC, "<<<< stt get partial result : invalid uid \n");
		}

		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
	}/* STTD_METHOD_PARTIAL_RESULT */

	else if (dbus_message_is_method_call(msg, if_name, STTD_METHOD_ERROR)) {
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
			SLOG(LOG_ERROR, TAG_STTC, "<<<< stt Get Error message : Get arguments error (%s)\n", err.message);
			dbus_error_free(&err); 
		} else {
			SLOG(LOG_DEBUG, TAG_STTC, "<<<< stt Get Error message : uid(%d), reason(%d), msg(%s)\n", uid, reason, err_msg);
			__stt_cb_error(uid, reason);
		}

		reply = dbus_message_new_method_return(msg);

		if (NULL != reply) {
			if (!dbus_connection_send(conn, reply, NULL))
				SLOG(LOG_ERROR, TAG_STTC, ">>>> stt Error message : fail to send reply");
			else 
				SLOG(LOG_DEBUG, TAG_STTC, ">>>> stt Error message");

			dbus_connection_flush(conn);
			dbus_message_unref(reply); 
		} else {
			SLOG(LOG_ERROR, TAG_STTC, ">>>> stt Error message : fail to create reply message");
		}

		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
	}/* STTD_METHOD_ERROR */

	/* free the message */
	dbus_message_unref(msg);

	return ECORE_CALLBACK_PASS_ON;
}

int stt_dbus_open_connection()
{
	if (NULL != g_conn) {
		SLOG(LOG_WARN, TAG_STTC, "already existed connection ");
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
		SLOG(LOG_ERROR, TAG_STTC, "Fail to get dbus connection \n");
		return STT_ERROR_OPERATION_FAILED; 
	}

	int pid = getpid();

	char service_name[64];
	memset(service_name, '\0', 64);
	snprintf(service_name, 64, "%s%d", STT_CLIENT_SERVICE_NAME, pid);

	SLOG(LOG_DEBUG, TAG_STTC, "service name is %s\n", service_name);

	/* register our name on the bus, and check for errors */
	ret = dbus_bus_request_name(g_conn, service_name, DBUS_NAME_FLAG_REPLACE_EXISTING , &err);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_STTC, "Name Error (%s)\n", err.message); 
		dbus_error_free(&err); 
	}

	if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) {
		printf("fail dbus_bus_request_name()\n");
		return -2;
	}

	if( NULL != g_fd_handler ) {
		SLOG(LOG_WARN, TAG_STTC, "The handler already exists.");
		return 0;
	}

	char rule[128];
	snprintf(rule, 128, "type='signal',interface='%s%d'", STT_CLIENT_SERVICE_INTERFACE, pid);

	/* add a rule for which messages we want to see */
	dbus_bus_add_match(g_conn, rule, &err); 
	dbus_connection_flush(g_conn);

	if (dbus_error_is_set(&err)) 
	{ 
		SLOG(LOG_ERROR, TAG_STTC, "Match Error (%s)\n", err.message);
		return STT_ERROR_OPERATION_FAILED; 
	}

	int fd = 0;
	if (1 != dbus_connection_get_unix_fd(g_conn, &fd)) {
		SLOG(LOG_ERROR, TAG_STTC, "fail to get fd from dbus \n");
		return STT_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, "Get fd from dbus : %d\n", fd);
	}

	g_fd_handler = ecore_main_fd_handler_add(fd, ECORE_FD_READ, (Ecore_Fd_Cb)listener_event_callback, g_conn, NULL, NULL);

	if (NULL == g_fd_handler) {
		SLOG(LOG_ERROR, TAG_STTC, "fail to get fd handler from ecore \n");
		return STT_ERROR_OPERATION_FAILED;
	}

	return 0;
}

int stt_dbus_close_connection()
{
	DBusError err;
	dbus_error_init(&err);

	int pid = getpid();

	char service_name[64];
	memset(service_name, '\0', 64);
	snprintf(service_name, 64, "%s%d", STT_CLIENT_SERVICE_NAME, pid);

	dbus_bus_release_name (g_conn, service_name, &err);

	dbus_connection_close(g_conn);

	g_fd_handler = NULL;
	g_conn = NULL;

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
		SLOG(LOG_ERROR, TAG_STTC, ">>>> Request stt hello : Fail to make message \n"); 
		return STT_ERROR_OPERATION_FAILED;
	} 

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg = NULL;
	int result = 0;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn, msg, 500, &err);

	dbus_message_unref(msg);

	if (NULL != result_msg) {
		dbus_message_unref(result_msg);

		SLOG(LOG_DEBUG, TAG_STTC, "<<<< stt hello");
		result = 0;
	} else {
		result = STT_ERROR_OPERATION_FAILED;
	}

	return result;
}


int stt_dbus_request_initialize(int uid, bool* silence_supported, bool* profanity_supported, bool* punctuation_supported)
{
	DBusMessage* msg;

	msg = dbus_message_new_method_call(
		STT_SERVER_SERVICE_NAME, 
		STT_SERVER_SERVICE_OBJECT_PATH, 
		STT_SERVER_SERVICE_INTERFACE, 
		STT_METHOD_INITIALIZE);

	if (NULL == msg) { 
		SLOG(LOG_ERROR, TAG_STTC, ">>>> stt initialize : Fail to make message \n"); 
		return STT_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, ">>>> stt initialize : uid(%d)", uid);
	}

	int pid = getpid();
	dbus_message_append_args( msg, 
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_INT32, &uid,
		DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = STT_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn, msg, g_waiting_time, &err);

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err, 
			DBUS_TYPE_INT32, &result, 
			DBUS_TYPE_INT32, silence_supported,
			DBUS_TYPE_INT32, profanity_supported,
			DBUS_TYPE_INT32, punctuation_supported,
			DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) { 
			SLOG(LOG_ERROR, TAG_STTC, "<<<< Get arguments error (%s)\n", err.message);
			dbus_error_free(&err); 
			result = STT_ERROR_OPERATION_FAILED;
		}

		dbus_message_unref(result_msg);
	} else {
		SLOG(LOG_ERROR, TAG_STTC, "<<<< Result message is NULL \n");
	}

	if (0 == result) {
		SLOG(LOG_DEBUG, TAG_STTC, "<<<< stt initialize : result = %d , silence(%d), profanity(%d), punctuation(%d)", 
			result, *silence_supported, *profanity_supported, *punctuation_supported);
	} else {
		SLOG(LOG_ERROR, TAG_STTC, "<<<< stt initialize : result = %d \n", result);
	}
	
	dbus_message_unref(msg);

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
		SLOG(LOG_ERROR, TAG_STTC, ">>>> stt finalize : Fail to make message \n"); 
		return STT_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, ">>>> stt finalize : uid(%d)", uid);
	}

	dbus_message_append_args(msg, DBUS_TYPE_INT32, &uid, DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = STT_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn, msg, g_waiting_time, &err);

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err, 
				DBUS_TYPE_INT32, &result,
				DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) { 
			printf("<<<< Get arguments error (%s)\n", err.message);
			dbus_error_free(&err); 
			result = STT_ERROR_OPERATION_FAILED;
		}

		dbus_message_unref(result_msg);
	} else {
		SLOG(LOG_ERROR, TAG_STTC, "<<<< Result message is NULL \n");
	}

	if (0 == result) {
		SLOG(LOG_DEBUG, TAG_STTC, "<<<< stt finalize : result = %d \n", result);
	} else {
		SLOG(LOG_ERROR, TAG_STTC, "<<<< stt finalize : result = %d \n", result);
	}

	dbus_message_unref(msg);

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
		SLOG(LOG_ERROR, TAG_STTC, ">>>> stt get supported languages : Fail to make message \n"); 
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

	result_msg = dbus_connection_send_with_reply_and_block(g_conn, msg, g_waiting_time, &err );

	if (NULL != result_msg) {
		if (dbus_message_iter_init(result_msg, &args)) {
			/* Get result */
			if (DBUS_TYPE_INT32 == dbus_message_iter_get_arg_type(&args)) {
				dbus_message_iter_get_basic(&args, &result);
				dbus_message_iter_next(&args);
			}

			if (0 == result) {
				SLOG(LOG_DEBUG, TAG_STTC, "<<<< stt get support languages : result = %d \n", result);

				/* Get voice size */
				int size ; 
				if (DBUS_TYPE_INT32 == dbus_message_iter_get_arg_type(&args)) {
					dbus_message_iter_get_basic(&args, &size);
					dbus_message_iter_next(&args);
				}

				if (0 >= size) {
					SLOG(LOG_ERROR, TAG_STTC, "<<<< stt size of language error : size = %d \n", size);
				} else {
					int i=0;
					char* temp_lang;

					for (i=0 ; i<size ; i++) {
						dbus_message_iter_get_basic(&args, &(temp_lang));
						dbus_message_iter_next(&args);

						if (true != callback(stt, temp_lang, user_data)) {
							break;
						}
					}
				}
			} else {
				SLOG(LOG_ERROR, TAG_STTC, "<<<< stt get support languages : result = %d \n", result);
			}
		} 
		dbus_message_unref(result_msg);
	} else {
		SLOG(LOG_ERROR, TAG_STTC, "<<<< stt get support languages : result message is NULL \n");
	}

	dbus_message_unref(msg);

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
		SLOG(LOG_ERROR, TAG_STTC, ">>>> stt get default language : Fail to make message \n"); 
		return STT_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, ">>>> stt get default language  : uid(%d)", uid);
	}

	dbus_message_append_args( msg, 
		DBUS_TYPE_INT32, &uid,
		DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = STT_ERROR_OPERATION_FAILED;
	char* temp_lang = NULL;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn, msg, g_waiting_time, &err);

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
			DBUS_TYPE_INT32, &result,
			DBUS_TYPE_STRING, &temp_lang,
			DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) { 
			printf("<<<< Get arguments error (%s)\n", err.message);
			dbus_error_free(&err); 
			result = STT_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);
	} else {
		SLOG(LOG_ERROR, TAG_STTC, "<<<< Result message is NULL \n");
	}

	if (0 == result) {
		*language = strdup(temp_lang);
		SLOG(LOG_DEBUG, TAG_STTC, "<<<< stt get default language : result = %d, language = %s \n", result, *language);
	} else {
		SLOG(LOG_ERROR, TAG_STTC, "<<<< stt get default language : result = %d \n", result);
	}

	dbus_message_unref(msg);

	return result;
}

int stt_dbus_request_is_partial_result_supported(int uid, bool* partial_result)
{
	if (NULL == partial_result) {
		SLOG(LOG_ERROR, TAG_STTC, "Input parameter is NULL");
		return STT_ERROR_INVALID_PARAMETER;
	}

	DBusMessage* msg;

	msg = dbus_message_new_method_call(
		   STT_SERVER_SERVICE_NAME, 
		   STT_SERVER_SERVICE_OBJECT_PATH, 
		   STT_SERVER_SERVICE_INTERFACE, 
		   STT_METHOD_IS_PARTIAL_SUPPORTED);

	if (NULL == msg) { 
		SLOG(LOG_ERROR, TAG_STTC, ">>>> stt is partial result supported : Fail to make message \n"); 
		return STT_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, ">>>> stt is partial result supported : uid(%d)", uid);
	}

	dbus_message_append_args( msg, 
		DBUS_TYPE_INT32, &uid,
		DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = STT_ERROR_OPERATION_FAILED;
	int support = -1;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn, msg, g_waiting_time, &err);

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
			DBUS_TYPE_INT32, &result,
			DBUS_TYPE_INT32, &support,
			DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) { 
			printf("<<<< Get arguments error (%s)\n", err.message);
			dbus_error_free(&err); 
			result = STT_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);
	} else {
		SLOG(LOG_ERROR, TAG_STTC, "<<<< Result message is NULL \n");
	}

	if (0 == result) {
		*partial_result = (bool)support;
		SLOG(LOG_DEBUG, TAG_STTC, "<<<< stt is partial result supported : result = %d, support = %s \n", result, *partial_result ? "true" : "false");
	} else {
		SLOG(LOG_ERROR, TAG_STTC, "<<<< stt is partial result supported : result = %d \n", result);
	}

	dbus_message_unref(msg);

	return result;
}

int stt_dbus_request_start(int uid, const char* lang, const char* type, int profanity, int punctuation, int silence)
{
	if (NULL == lang || NULL == type) {
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
		SLOG(LOG_ERROR, TAG_STTC, ">>>> stt start : Fail to make message \n"); 
		return STT_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, ">>>> stt start : uid(%d), language(%s), type(%s)", uid, lang, type);
	}

	dbus_message_append_args( msg, 
		DBUS_TYPE_INT32, &uid, 
		DBUS_TYPE_STRING, &lang,   
		DBUS_TYPE_STRING, &type,
		DBUS_TYPE_INT32, &profanity,
		DBUS_TYPE_INT32, &punctuation,
		DBUS_TYPE_INT32, &silence,
		DBUS_TYPE_INVALID);
	
	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = STT_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn, msg, g_waiting_start_time, &err);

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
			DBUS_TYPE_INT32, &result,
			DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) { 
			printf("<<<< Get arguments error (%s)\n", err.message);
			dbus_error_free(&err); 
			result = STT_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, "<<<< Result Message is NULL");
	}

	if (0 == result) {
		SLOG(LOG_DEBUG, TAG_STTC, "<<<< stt start : result = %d ", result);
	} else {
		SLOG(LOG_ERROR, TAG_STTC, "<<<< stt start : result = %d ", result);
	}

	dbus_message_unref(msg);

	return result;
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
		SLOG(LOG_ERROR, TAG_STTC, ">>>> stt stop : Fail to make message \n"); 
		return STT_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, ">>>> stt stop : uid(%d)", uid);
	}

	dbus_message_append_args(msg, 
		DBUS_TYPE_INT32, &uid, 
		DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = STT_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn, msg, g_waiting_time, &err);

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
			DBUS_TYPE_INT32, &result,
			DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) { 
			printf("<<<< Get arguments error (%s)\n", err.message);
			dbus_error_free(&err); 
			result = STT_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, "<<<< Result Message is NULL");
	}

	if (0 == result) {
		SLOG(LOG_DEBUG, TAG_STTC, "<<<< stt stop : result = %d ", result);
	} else {
		SLOG(LOG_ERROR, TAG_STTC, "<<<< stt stop : result = %d ", result);
	}

	dbus_message_unref(msg);

	return result;
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
		SLOG(LOG_ERROR, TAG_STTC, ">>>> stt cancel : Fail to make message \n"); 
		return STT_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, ">>>> stt cancel : uid(%d)", uid);
	}

	dbus_message_append_args(msg, 
		DBUS_TYPE_INT32, &uid, 
		DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = STT_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn, msg, g_waiting_time, &err);

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
			DBUS_TYPE_INT32, &result,
			DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) { 
			printf("<<<< Get arguments error (%s)\n", err.message);
			dbus_error_free(&err); 
			result = STT_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, "<<<< Result Message is NULL");
	}

	if (0 == result) {
		SLOG(LOG_DEBUG, TAG_STTC, "<<<< stt cancel : result = %d ", result);
	} else {
		SLOG(LOG_ERROR, TAG_STTC, "<<<< stt cancel : result = %d ", result);
	}

	dbus_message_unref(msg);

	return result;
}

int stt_dbus_request_start_file_recognition(int uid, const char* filepath, const char* lang, const char* type, int profanity, int punctuation)
{
	if (NULL == filepath || NULL == lang || NULL == type) {
		SLOG(LOG_ERROR, TAG_STTC, "Input parameter is NULL");
		return STT_ERROR_INVALID_PARAMETER;
	}

	DBusMessage* msg;

	/* create a signal & check for errors */
	msg = dbus_message_new_method_call(
		STT_SERVER_SERVICE_NAME,
		STT_SERVER_SERVICE_OBJECT_PATH,	
		STT_SERVER_SERVICE_INTERFACE,	
		STT_METHOD_START_FILE_RECONITION);		

	if (NULL == msg) { 
		SLOG(LOG_ERROR, TAG_STTC, ">>>> stt start file recognition : Fail to make message"); 
		return STT_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, ">>>> stt start file recogntion : uid(%d), filepath(%s) language(%s), type(%s)", 
			uid, filepath, lang, type);
	}

	dbus_message_append_args( msg, 
		DBUS_TYPE_INT32, &uid, 
		DBUS_TYPE_STRING, &filepath,
		DBUS_TYPE_STRING, &lang,
		DBUS_TYPE_STRING, &type,
		DBUS_TYPE_INT32, &profanity,
		DBUS_TYPE_INT32, &punctuation,
		DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = STT_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn, msg, g_waiting_start_time, &err);

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
			DBUS_TYPE_INT32, &result,
			DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) { 
			printf("<<<< Get arguments error (%s)", err.message);
			dbus_error_free(&err); 
			result = STT_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, "<<<< Result Message is NULL");
	}

	if (0 == result) {
		SLOG(LOG_DEBUG, TAG_STTC, "<<<< stt start file recognition : result = %d ", result);
	} else {
		SLOG(LOG_ERROR, TAG_STTC, "<<<< stt start file recognition : result = %d ", result);
	}

	dbus_message_unref(msg);

	return result;
}