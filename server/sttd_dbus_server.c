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


#include "sttd_main.h"
#include "sttd_dbus.h"
#include "sttd_dbus_server.h"
#include "sttd_server.h"
#include "sttd_client_data.h"
#include "stt_defs.h"

/*
* Dbus Client-Daemon Server
*/ 

int sttd_dbus_server_hello(DBusConnection* conn, DBusMessage* msg)
{
	SLOG(LOG_DEBUG, TAG_STTD, ">>>>> STT Hello");

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	int status = -1;
	if (EINA_TRUE == sttd_get_daemon_exist()) {
		status = STTD_DAEMON_NORMAL;
	} else {
		status = STTD_DAEMON_ON_TERMINATING;
	}

	if (NULL != reply) {
		dbus_message_append_args(reply, DBUS_TYPE_INT32, &status, DBUS_TYPE_INVALID);

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Fail to create reply message!!"); 
	}

	SLOG(LOG_DEBUG, TAG_STTD, "<<<<<");
	SLOG(LOG_DEBUG, TAG_STTD, "  ");

	return 0;
}

int sttd_dbus_server_initialize(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid;
	int uid;
	bool silence_supported = false;

	int ret = STTD_ERROR_OPERATION_FAILED;

	dbus_message_get_args(msg, &err,
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_INT32, &uid,
		DBUS_TYPE_INVALID);
	
	SLOG(LOG_DEBUG, TAG_STTD, ">>>>> STT Initialize");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_STTD, "[IN ERROR] stt initialize : get arguments error (%s)", err.message);
		dbus_error_free(&err); 
		ret = STTD_ERROR_OPERATION_FAILED;
	} else {
		SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[IN] stt initialize : pid(%d), uid(%d)", pid , uid); 
		ret =  sttd_server_initialize(pid, uid, &silence_supported);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		dbus_message_append_args(reply, 
			DBUS_TYPE_INT32, &ret, 
			DBUS_TYPE_INT32, &silence_supported,
			DBUS_TYPE_INVALID);

		if (0 == ret) {
			SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[OUT SUCCESS] Result(%d), silence(%d)", 
				ret, silence_supported); 
		} else {
			SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Result(%d)", ret); 
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Fail to create reply message!!"); 
	}

	SLOG(LOG_DEBUG, TAG_STTD, "<<<<<");
	SLOG(LOG_DEBUG, TAG_STTD, "  ");

	return 0;
}

int sttd_dbus_server_finalize(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int uid;
	int ret = STTD_ERROR_OPERATION_FAILED;
	dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &uid, DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_STTD, ">>>>> STT Finalize");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_STTD, "[IN ERROR] stt finalize : get arguments error (%s)", err.message);
		dbus_error_free(&err); 
		ret = STTD_ERROR_OPERATION_FAILED;
	} else {
		SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[IN] stt finalize : uid(%d)", uid); 
		ret =  sttd_server_finalize(uid);
	}

	DBusMessage* reply;
	
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

		if (0 == ret) {
			SLOG(LOG_DEBUG, TAG_STTD, "[OUT SUCCESS] Result(%d)", ret); 
		} else {
			SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Result(%d)", ret); 
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Fail to create reply message!!"); 
	}

	SLOG(LOG_DEBUG, TAG_STTD, "<<<<<");
	SLOG(LOG_DEBUG, TAG_STTD, "  ");

	return 0;
}

int sttd_dbus_server_get_support_engines(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int uid;
	int ret = STTD_ERROR_OPERATION_FAILED;
	GSList* engine_list = NULL;

	dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &uid, DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_STTD, ">>>>> STT Get supported engines");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_STTD, "[IN ERROR] stt supported engines : get arguments error (%s)", err.message);
		dbus_error_free(&err); 
		ret = STTD_ERROR_OPERATION_FAILED;
	} else {
		SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[IN] stt supported engines : uid(%d)", uid); 
		ret = sttd_server_get_supported_engines(uid, &engine_list);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		DBusMessageIter args;

		dbus_message_iter_init_append(reply, &args);

		/* Append result*/
		dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &(ret) );

		if (0 == ret) {
			/* Append size */
			int size = g_slist_length(engine_list);
			if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &(size))) {
				ret = STTD_ERROR_OPERATION_FAILED;
			} else {
				GSList *iter = NULL;
				engine_s* engine;

				iter = g_slist_nth(engine_list, 0);

				while (NULL != iter) {
					engine = iter->data;

					if (NULL != engine) {
						if (NULL != engine->engine_id && NULL != engine->engine_name && NULL != engine->ug_name) {
							SECURE_SLOG(LOG_DEBUG, TAG_STTD, "engine id : %s, engine name : %s, ug_name, : %s", 
								engine->engine_id, engine->engine_name, engine->ug_name);

							dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &(engine->engine_id));
							dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &(engine->engine_name));
							/* dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &(engine->ug_name)); */
						} else {
							SLOG(LOG_ERROR, TAG_STTD, "[ERROR] Engine info is NULL");
						}

						if (NULL != engine->engine_id)		free(engine->engine_id);
						if (NULL != engine->engine_name)	free(engine->engine_name);
						if (NULL != engine->ug_name)		free(engine->ug_name);

						free(engine);
					} else {
						SLOG(LOG_ERROR, TAG_STTD, "[ERROR] Engine info is NULL");
					}

					engine_list = g_slist_remove_link(engine_list, iter);

					iter = g_slist_nth(engine_list, 0);
				} 
			}
			SLOG(LOG_DEBUG, TAG_STTD, "[OUT] Result(%d)", ret); 
		} else {
			SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Result(%d)", ret); 
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Fail to create reply message!!"); 
	}

	SLOG(LOG_DEBUG, TAG_STTD, "<<<<<");
	SLOG(LOG_DEBUG, TAG_STTD, "  ");

	return 0;
}

int sttd_dbus_server_set_current_engine(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int uid;
	char* engine_id;
	bool silence_supported = false;
	int ret = STTD_ERROR_OPERATION_FAILED;

	dbus_message_get_args(msg, &err,
		DBUS_TYPE_INT32, &uid,
		DBUS_TYPE_STRING, &engine_id,
		DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_STTD, ">>>>> STT Set current engine");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_STTD, "[IN ERROR] stt set current engine : get arguments error (%s)", err.message);
		dbus_error_free(&err); 
		ret = STTD_ERROR_OPERATION_FAILED;
	} else {
		SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[IN] stt set current engine : uid(%d)", uid); 
		ret = sttd_server_set_current_engine(uid, engine_id, &silence_supported);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		dbus_message_append_args(reply, 
			DBUS_TYPE_INT32, &ret, 
			DBUS_TYPE_INT32, &silence_supported,
			DBUS_TYPE_INVALID);

		if (0 == ret) {
			SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[OUT SUCCESS] Result(%d), silence(%d)", 
				ret, silence_supported);
		} else {
			SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Result(%d) ", ret); 
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Fail to create reply message!!"); 
	}

	SLOG(LOG_DEBUG, TAG_STTD, "<<<<<");
	SLOG(LOG_DEBUG, TAG_STTD, "  ");

	return 0;
}

int sttd_dbus_server_get_current_engine(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int uid;
	char* engine = NULL;
	int ret = STTD_ERROR_OPERATION_FAILED;

	dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &uid, DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_STTD, ">>>>> STT Get current engine");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_STTD, "[IN ERROR] stt get current engine : get arguments error (%s)", err.message);
		dbus_error_free(&err); 
		ret = STTD_ERROR_OPERATION_FAILED;
	} else {
		SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[IN] stt get current engine : uid(%d)", uid); 
		ret = sttd_server_get_current_engine(uid, &engine);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		if (0 == ret) {
			/* Append result and language */
			dbus_message_append_args( reply, 
				DBUS_TYPE_INT32, &ret,
				DBUS_TYPE_STRING, &engine,
				DBUS_TYPE_INVALID);

			SLOG(LOG_DEBUG, TAG_STTD, "[OUT SUCCESS] Result(%d)", ret); 
		} else {
			dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

			SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Result(%d)", ret); 
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Fail to create reply message!!"); 
	}

	if (NULL != engine)	free(engine);

	SLOG(LOG_DEBUG, TAG_STTD, "<<<<<");
	SLOG(LOG_DEBUG, TAG_STTD, "  ");

	return 0;
}

int sttd_dbus_server_check_app_agreed(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int uid;
	char* appid;
	int ret = STTD_ERROR_OPERATION_FAILED;
	bool available = false;

	dbus_message_get_args(msg, &err, 
		DBUS_TYPE_INT32, &uid, 
		DBUS_TYPE_STRING, &appid, 
		DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_STTD, ">>>>> STT Is engine available");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_STTD, "[IN ERROR] stt Is engine available : get arguments error (%s)", err.message);
		dbus_error_free(&err); 
		ret = STTD_ERROR_OPERATION_FAILED;
	} else {
		SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[IN] stt Is engine available : uid(%d)", uid); 
		ret = sttd_server_check_agg_agreed(uid, appid, &available);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		/* Append result and language */
		dbus_message_append_args(reply, 
			DBUS_TYPE_INT32, &ret,
			DBUS_TYPE_INT32, &available,
			DBUS_TYPE_INVALID);

		if (0 == ret) {
			SLOG(LOG_DEBUG, TAG_STTD, "[OUT SUCCESS] Result(%d)", ret); 
		} else {
			SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Result(%d)", ret); 
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Fail to create reply message!!"); 
	}

	SLOG(LOG_DEBUG, TAG_STTD, "<<<<<");
	SLOG(LOG_DEBUG, TAG_STTD, "  ");

	return 0;
}

int sttd_dbus_server_get_support_lang(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int uid;
	int ret = STTD_ERROR_OPERATION_FAILED;
	GSList* lang_list = NULL;

	dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &uid, DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_STTD, ">>>>> STT Get supported langs");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_STTD, "[IN ERROR] stt supported langs : get arguments error (%s)", err.message);
		dbus_error_free(&err); 
		ret = STTD_ERROR_OPERATION_FAILED;
	} else {
		SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[IN] stt supported langs : uid(%d)", uid); 
		ret = sttd_server_get_supported_languages(uid, &lang_list);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		DBusMessageIter args;
		dbus_message_iter_init_append(reply, &args);

		/* Append result*/
		dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &(ret));

		if (0 == ret) {
			/* Append language size */
			int size = g_slist_length(lang_list);

			if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &(size))) {
				SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Fail to append type"); 
				ret = STTD_ERROR_OPERATION_FAILED;
			} else {
				GSList *iter = NULL;
				char* temp_lang;

				iter = g_slist_nth(lang_list, 0);

				while (NULL != iter) {
					temp_lang = iter->data;

					dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &(temp_lang) );
					
					if (NULL != temp_lang)
						free(temp_lang);
					
					lang_list = g_slist_remove_link(lang_list, iter);

					iter = g_slist_nth(lang_list, 0);
				} 
			}
			SLOG(LOG_DEBUG, TAG_STTD, "[OUT] Result(%d)", ret); 
		} else {
			SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Result(%d)", ret); 
		}
		
		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Fail to create reply message!!"); 
	}

	SLOG(LOG_DEBUG, TAG_STTD, "<<<<<");
	SLOG(LOG_DEBUG, TAG_STTD, "  ");

	return 0;
}

int sttd_dbus_server_get_default_lang(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int uid;
	char* lang = NULL;
	int ret = STTD_ERROR_OPERATION_FAILED;

	dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &uid, DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_STTD, ">>>>> STT Get default langs");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_STTD, "[IN ERROR] stt get default lang : get arguments error (%s)", err.message);
		dbus_error_free(&err); 
		ret = STTD_ERROR_OPERATION_FAILED;
	} else {
		SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[IN] stt get default lang : uid(%d)", uid); 
		ret = sttd_server_get_current_langauage(uid, &lang);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		if (0 == ret) {
			/* Append result and language */
			dbus_message_append_args( reply, 
				DBUS_TYPE_INT32, &ret,
				DBUS_TYPE_STRING, &lang,
				DBUS_TYPE_INVALID);

			SLOG(LOG_DEBUG, TAG_STTD, "[OUT SUCCESS] Result(%d)", ret); 
		} else {
			dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

			SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Result(%d)", ret); 
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Fail to create reply message!!"); 
	}

	if (NULL != lang)	free(lang);

	SLOG(LOG_DEBUG, TAG_STTD, "<<<<<");
	SLOG(LOG_DEBUG, TAG_STTD, "  ");

	return 0;
}

int sttd_dbus_server_is_recognition_type_supported(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int uid;
	char* type = NULL;
	int support = -1;
	int ret = STTD_ERROR_OPERATION_FAILED;

	dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &uid, DBUS_TYPE_STRING, &type, DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_STTD, ">>>>> STT is recognition type supported");

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_STTD, "[IN ERROR] stt is recognition type supported : get arguments error (%s)", err.message);
		dbus_error_free(&err);
		ret = STTD_ERROR_OPERATION_FAILED;
	} else {
		SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[IN] stt is recognition type supported : uid(%d)", uid);
		ret = sttd_server_is_recognition_type_supported(uid, type, &support);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		/* Append result and language */
		dbus_message_append_args(reply, 
			DBUS_TYPE_INT32, &ret,
			DBUS_TYPE_INT32, &support,
			DBUS_TYPE_INVALID);

		if (0 == ret) {
			SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[OUT SUCCESS] Result(%d), Support(%s)", ret, support ? "true" : "false"); 
		} else {
			SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Result(%d)", ret); 
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Fail to create reply message!!"); 
	}

	SLOG(LOG_DEBUG, TAG_STTD, "<<<<<");
	SLOG(LOG_DEBUG, TAG_STTD, "  ");

	return 0;
}

int sttd_dbus_server_set_start_sound(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int uid;
	char* file = NULL;
	int ret = STTD_ERROR_OPERATION_FAILED;

	dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &uid, DBUS_TYPE_STRING, &file, DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_STTD, ">>>>> STT set start sound");

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_STTD, "[IN ERROR] stt set start sound : get arguments error (%s)", err.message);
		dbus_error_free(&err);
		ret = STTD_ERROR_OPERATION_FAILED;
	} else {
		SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[IN] stt set start sound : uid(%d) file(%s)", uid, file);
		ret = sttd_server_set_start_sound(uid, file);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		/* Append result and language */
		dbus_message_append_args(reply, 
			DBUS_TYPE_INT32, &ret,
			DBUS_TYPE_INVALID);

		if (0 == ret) {
			SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[OUT SUCCESS] Result(%d)", ret); 
		} else {
			SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Result(%d)", ret); 
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Fail to create reply message!!"); 
	}

	SLOG(LOG_DEBUG, TAG_STTD, "<<<<<");
	SLOG(LOG_DEBUG, TAG_STTD, "  ");

	return 0;
}

int sttd_dbus_server_unset_start_sound(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int uid;
	int ret = STTD_ERROR_OPERATION_FAILED;

	dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &uid, DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_STTD, ">>>>> STT unset start sound");

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_STTD, "[IN ERROR] stt unset start sound : get arguments error (%s)", err.message);
		dbus_error_free(&err);
		ret = STTD_ERROR_OPERATION_FAILED;
	} else {
		SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[IN] stt unset start sound : uid(%d)", uid);
		ret = sttd_server_set_start_sound(uid, NULL);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		/* Append result and language */
		dbus_message_append_args(reply, 
			DBUS_TYPE_INT32, &ret,
			DBUS_TYPE_INVALID);

		if (0 == ret) {
			SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[OUT SUCCESS] Result(%d)", ret); 
		} else {
			SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Result(%d)", ret); 
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Fail to create reply message!!"); 
	}

	SLOG(LOG_DEBUG, TAG_STTD, "<<<<<");
	SLOG(LOG_DEBUG, TAG_STTD, "  ");

	return 0;
}

int sttd_dbus_server_set_stop_sound(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int uid;
	char* file = NULL;
	int ret = STTD_ERROR_OPERATION_FAILED;

	dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &uid, DBUS_TYPE_STRING, &file, DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_STTD, ">>>>> STT set stop sound");

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_STTD, "[IN ERROR] stt set stop sound : get arguments error (%s)", err.message);
		dbus_error_free(&err);
		ret = STTD_ERROR_OPERATION_FAILED;
	} else {
		SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[IN] stt set stop sound : uid(%d) file(%s)", uid, file);
		ret = sttd_server_set_stop_sound(uid, file);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		/* Append result and language */
		dbus_message_append_args(reply, 
			DBUS_TYPE_INT32, &ret,
			DBUS_TYPE_INVALID);

		if (0 == ret) {
			SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[OUT SUCCESS] Result(%d)", ret); 
		} else {
			SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Result(%d)", ret); 
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Fail to create reply message!!"); 
	}

	SLOG(LOG_DEBUG, TAG_STTD, "<<<<<");
	SLOG(LOG_DEBUG, TAG_STTD, "  ");

	return 0;
}

int sttd_dbus_server_unset_stop_sound(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int uid;
	int ret = STTD_ERROR_OPERATION_FAILED;

	dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &uid, DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_STTD, ">>>>> STT unset stop sound");

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_STTD, "[IN ERROR] stt unset stop sound : get arguments error (%s)", err.message);
		dbus_error_free(&err);
		ret = STTD_ERROR_OPERATION_FAILED;
	} else {
		SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[IN] stt unset stop sound : uid(%d)", uid);
		ret = sttd_server_set_stop_sound(uid, NULL);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		/* Append result and language */
		dbus_message_append_args(reply, 
			DBUS_TYPE_INT32, &ret,
			DBUS_TYPE_INVALID);

		if (0 == ret) {
			SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[OUT SUCCESS] Result(%d)", ret); 
		} else {
			SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Result(%d)", ret); 
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Fail to create reply message!!"); 
	}

	SLOG(LOG_DEBUG, TAG_STTD, "<<<<<");
	SLOG(LOG_DEBUG, TAG_STTD, "  ");

	return 0;
}

int sttd_dbus_server_start(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int uid;
	char* lang;
	char* type;
	char* appid;
	int silence;
	int ret = STTD_ERROR_OPERATION_FAILED;

	dbus_message_get_args(msg, &err, 
		DBUS_TYPE_INT32, &uid, 
		DBUS_TYPE_STRING, &lang,   
		DBUS_TYPE_STRING, &type,
		DBUS_TYPE_INT32, &silence,
		DBUS_TYPE_STRING, &appid,
		DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_STTD, ">>>>> STT Start");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_STTD, "[IN ERROR] stt start : get arguments error (%s)", err.message);
		dbus_error_free(&err); 
		ret = STTD_ERROR_OPERATION_FAILED;
	} else {
		SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[IN] stt start : uid(%d), lang(%s), type(%s), silence(%d) appid(%s)"
			, uid, lang, type, silence, appid); 
		ret = sttd_server_start(uid, lang, type, silence, appid);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

		if (0 <= ret) {
			SLOG(LOG_DEBUG, TAG_STTD, "[OUT SUCCESS] Result(%d)", ret); 
		} else {
			SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Result(%d)", ret); 
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Fail to create reply message!!"); 
	}

	SLOG(LOG_DEBUG, TAG_STTD, "<<<<<");
	SLOG(LOG_DEBUG, TAG_STTD, "  ");

	return 0;
}

int sttd_dbus_server_stop(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int uid;
	int ret = STTD_ERROR_OPERATION_FAILED;
	dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &uid, DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_STTD, ">>>>> STT Stop");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_STTD, "[IN ERROR] stt stop : get arguments error (%s)", err.message);
		dbus_error_free(&err); 
		ret = STTD_ERROR_OPERATION_FAILED;
	} else {
		SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[IN] stt stop : uid(%d)", uid); 
		ret = sttd_server_stop(uid);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

		if (0 <= ret) {
			SLOG(LOG_DEBUG, TAG_STTD, "[OUT SUCCESS] Result(%d)", ret); 
		} else {
			SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Result(%d)", ret); 
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Fail to create reply message!!"); 
	}

	SLOG(LOG_DEBUG, TAG_STTD, "<<<<<");
	SLOG(LOG_DEBUG, TAG_STTD, "  ");

	return 0;
}

int sttd_dbus_server_cancel(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int uid;
	int ret = STTD_ERROR_OPERATION_FAILED;
	dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &uid, DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_STTD, ">>>>> STT Cancel");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_STTD, "[IN ERROR] stt cancel : get arguments error (%s)", err.message);
		dbus_error_free(&err); 
		ret = STTD_ERROR_OPERATION_FAILED;
	} else {
		SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[IN] stt cancel : uid(%d)", uid); 
		ret = sttd_server_cancel(uid);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

		if (0 == ret) {
			SLOG(LOG_DEBUG, TAG_STTD, "[OUT SUCCESS] Result(%d)", ret); 
		} else {
			SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Result(%d)", ret); 
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Fail to create reply message!!"); 
	}

	SLOG(LOG_DEBUG, TAG_STTD, "<<<<<");
	SLOG(LOG_DEBUG, TAG_STTD, "  ");

	return 0;
}
