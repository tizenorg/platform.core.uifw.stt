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

	if (NULL != reply) {
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
	bool profanity_supported = false;
	bool punctuation_supported = false;

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
		SLOG(LOG_DEBUG, TAG_STTD, "[IN] stt initialize : pid(%d), uid(%d)", pid , uid); 
		ret =  sttd_server_initialize(pid, uid, &silence_supported, &profanity_supported, &punctuation_supported);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		dbus_message_append_args(reply, 
			DBUS_TYPE_INT32, &ret, 
			DBUS_TYPE_INT32, &silence_supported,
			DBUS_TYPE_INT32, &profanity_supported,
			DBUS_TYPE_INT32, &punctuation_supported,
			DBUS_TYPE_INVALID);

		if (0 == ret) {
			SLOG(LOG_DEBUG, TAG_STTD, "[OUT SUCCESS] Result(%d), silence(%d), profanity(%d), punctuation(%d)", 
				ret, silence_supported, profanity_supported, punctuation_supported); 
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
		SLOG(LOG_DEBUG, TAG_STTD, "[IN] stt finalize : uid(%d)", uid); 
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

int sttd_dbus_server_get_support_lang(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int uid;
	int ret = STTD_ERROR_OPERATION_FAILED;
	GList* lang_list = NULL;

	dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &uid, DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_STTD, ">>>>> STT Get supported langs");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_STTD, "[IN ERROR] stt supported langs : get arguments error (%s)", err.message);
		dbus_error_free(&err); 
		ret = STTD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTD, "[IN] stt supported langs : uid(%d)", uid); 
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
			int size = g_list_length(lang_list);

			if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &(size))) {
				SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Fail to append type"); 
				ret = STTD_ERROR_OPERATION_FAILED;
			} else {
				GList *iter = NULL;
				char* temp_lang;

				iter = g_list_first(lang_list);

				while (NULL != iter) {
					temp_lang = iter->data;

					dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &(temp_lang) );
					
					if (NULL != temp_lang)
						free(temp_lang);
					
					lang_list = g_list_remove_link(lang_list, iter);

					iter = g_list_first(lang_list);
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
	char* lang;
	int ret = STTD_ERROR_OPERATION_FAILED;

	dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &uid, DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_STTD, ">>>>> STT Get default langs");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_STTD, "[IN ERROR] stt get default lang : get arguments error (%s)", err.message);
		dbus_error_free(&err); 
		ret = STTD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTD, "[IN] stt get default lang : uid(%d)", uid); 
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

			if (NULL != lang) {
				g_free(lang);
			}
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

	SLOG(LOG_DEBUG, TAG_STTD, "<<<<<");
	SLOG(LOG_DEBUG, TAG_STTD, "  ");

	return 0;
}

int sttd_dbus_server_is_partial_result_supported(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int uid;
	int support = -1;
	int ret = STTD_ERROR_OPERATION_FAILED;

	dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &uid, DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_STTD, ">>>>> STT is partial result supported");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_STTD, "[IN ERROR] stt is partial result supported : get arguments error (%s)", err.message);
		dbus_error_free(&err); 
		ret = STTD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTD, "[IN] stt is partial result supported : uid(%d)", uid); 
		ret = sttd_server_is_partial_result_supported(uid, &support);
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
			SLOG(LOG_DEBUG, TAG_STTD, "[OUT SUCCESS] Result(%d), Support(%s)", ret, support ? "true" : "false"); 
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

int sttd_dbus_server_get_audio_volume(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int uid;
	float current_volume = 0.0;
	int ret = STTD_ERROR_OPERATION_FAILED;

	dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &uid, DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_STTD, ">>>>> STT Get audio volume");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_STTD, "[IN ERROR] stt get audio volume : get arguments error (%s)", err.message);
		dbus_error_free(&err); 
		ret = STTD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTD, "[IN] stt get audio volume : uid(%d)", uid); 
		ret =  sttd_server_get_audio_volume(uid, &current_volume);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		double temp = (double)current_volume;

		dbus_message_append_args(reply, 
			DBUS_TYPE_INT32, &ret, 
			DBUS_TYPE_DOUBLE, &temp, 
			DBUS_TYPE_INVALID);

		if (0 == ret) {
			SLOG(LOG_DEBUG, TAG_STTD, "[OUT SUCCESS] Result(%d), volume - double(%f), float(%f)", ret, temp, current_volume); 
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
	int profanity;
	int punctuation;
	int silence;
	int ret = STTD_ERROR_OPERATION_FAILED;

	dbus_message_get_args(msg, &err, 
		DBUS_TYPE_INT32, &uid, 
		DBUS_TYPE_STRING, &lang,   
		DBUS_TYPE_STRING, &type,
		DBUS_TYPE_INT32, &profanity,
		DBUS_TYPE_INT32, &punctuation,
		DBUS_TYPE_INT32, &silence,
		DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_STTD, ">>>>> STT Start");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_STTD, "[IN ERROR] stt start : get arguments error (%s)", err.message);
		dbus_error_free(&err); 
		ret = STTD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTD, "[IN] stt start : uid(%d), lang(%s), type(%s), profanity(%d), punctuation(%d), silence(%d)"
					, uid, lang, type, profanity, punctuation, silence); 
		ret = sttd_server_start(uid, lang, type, profanity, punctuation, silence);
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
		SLOG(LOG_DEBUG, TAG_STTD, "[IN] stt stop : uid(%d)", uid); 
		ret = sttd_server_stop(uid);
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
		SLOG(LOG_DEBUG, TAG_STTD, "[IN] stt cancel : uid(%d)", uid); 
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


/*
* Dbus Setting-Daemon Server
*/ 

int sttd_dbus_server_setting_initialize(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid;
	int ret = STTD_ERROR_OPERATION_FAILED;

	dbus_message_get_args(msg, &err,
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_STTD, ">>>>> Setting Initialize");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_STTD, "[IN ERROR] setting initializie : get arguments error (%s)", err.message);
		dbus_error_free(&err); 
		ret = STTD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTD, "[IN] setting initializie : uid(%d)", pid); 
		ret =  sttd_server_setting_initialize(pid);
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

int sttd_dbus_server_setting_finalize(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid;
	int ret = STTD_ERROR_OPERATION_FAILED;

	dbus_message_get_args(msg, &err, 
		DBUS_TYPE_INT32, &pid, 
		DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_STTD, ">>>>> Setting Finalize");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_STTD, "[IN ERROR] setting finalize : get arguments error (%s)", err.message);
		dbus_error_free(&err); 
		ret = STTD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTD, "[IN] setting finalize : uid(%d)", pid); 
		ret =  sttd_server_setting_finalize(pid);
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

int sttd_dbus_server_setting_get_engine_list(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid;
	GList* engine_list = NULL;
	int ret = STTD_ERROR_OPERATION_FAILED;

	dbus_message_get_args(msg, &err, 
		DBUS_TYPE_INT32, &pid, 
		DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_STTD, ">>>>> Setting Get Engine List");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_STTD, "[IN ERROR] setting get engine list : get arguments error (%s)", err.message);
		dbus_error_free(&err); 
		ret = STTD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTD, "[IN] setting get engine list : uid(%d)", pid); 
		ret = sttd_server_setting_get_engine_list(pid, &engine_list);
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
			int size = g_list_length(engine_list);
			if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &(size))) {
				ret = STTD_ERROR_OPERATION_FAILED;
			} else {
				GList *iter = NULL;
				engine_s* engine;

				iter = g_list_first(engine_list);

				while (NULL != iter) {
					engine = iter->data;
					SLOG(LOG_DEBUG, TAG_STTD, "engine id : %s, engine name : %s, ug_name, : %s", 
						engine->engine_id, engine->engine_name, engine->ug_name);

					dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &(engine->engine_id) );
					dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &(engine->engine_name) );
					dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &(engine->ug_name) );

					if (NULL != engine->engine_id)
						g_free(engine->engine_id);
					if (NULL != engine->engine_name);
						g_free(engine->engine_name);
					if (NULL != engine->ug_name);
						g_free(engine->ug_name);
					if (NULL != engine);
						g_free(engine);

					engine_list = g_list_remove_link(engine_list, iter);

					iter = g_list_first(engine_list);
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

int sttd_dbus_server_setting_get_engine(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid;
	char* engine_id;
	int ret = STTD_ERROR_OPERATION_FAILED;

	dbus_message_get_args(msg, &err,
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_STTD, ">>>>> Setting Get Engine");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_STTD, "[IN ERROR] setting get engine : get arguments error (%s)", err.message);
		dbus_error_free(&err); 
		ret = STTD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTD, "[IN] setting get engine : uid(%d)", pid); 
		ret = sttd_server_setting_get_engine(pid, &engine_id);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		if (0 == ret) {
			dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_STRING, &engine_id, DBUS_TYPE_INVALID);

			SLOG(LOG_DEBUG, TAG_STTD, "[OUT SUCCESS] Result(%d), Engine id(%s)", ret, engine_id); 
			g_free(engine_id);		
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

	SLOG(LOG_DEBUG, TAG_STTD, "<<<<<");
	SLOG(LOG_DEBUG, TAG_STTD, "  ");

	return 0;
}

int sttd_dbus_server_setting_set_engine(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid;
	char* engine_id;
	int ret = STTD_ERROR_OPERATION_FAILED;

	dbus_message_get_args(msg, &err,
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_STRING, &engine_id,
		DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_STTD, ">>>>> Setting Set Engine");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_STTD, "[IN ERROR] setting set engine : get arguments error (%s)", err.message);
		dbus_error_free(&err); 
		ret = STTD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTD, "[IN] setting set engine : uid(%d)", pid); 
		ret = sttd_server_setting_set_engine(pid, engine_id);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

		if (0 == ret) {
			SLOG(LOG_DEBUG, TAG_STTD, "[OUT SUCCESS] Result(%d) ", ret); 
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

int sttd_dbus_server_setting_get_language_list(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid;
	GList* lang_list = NULL;
	char* engine_id;
	int ret = STTD_ERROR_OPERATION_FAILED;

	dbus_message_get_args(msg, &err,
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_STTD, ">>>>> Setting Get Language List");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_STTD, "[IN ERROR] setting get language list : get arguments error (%s)", err.message);
		dbus_error_free(&err); 
		ret = STTD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTD, "[IN] setting get language list : uid(%d)", pid); 
		ret = sttd_server_setting_get_lang_list(pid, &engine_id, &lang_list);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		DBusMessageIter args;
		dbus_message_iter_init_append(reply, &args);

		/* Append result*/
		dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &ret);

		if (0 == ret) {
			dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &(engine_id));

			int size = g_list_length(lang_list);
			SLOG(LOG_ERROR, TAG_STTD, "[OUT DEBUG] Count(%d) ", size); 

			/* Append size */
			if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &(size))) {
				ret = STTD_ERROR_OPERATION_FAILED;
				SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Result(%d) ", ret); 
			} else {
				GList *iter = NULL;
				char* temp_lang;

				iter = g_list_first(lang_list);

				while (NULL != iter) {
					temp_lang = iter->data;

					dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &(temp_lang) );
					
					if (NULL != temp_lang)
						free(temp_lang);
					
					lang_list = g_list_remove_link(lang_list, iter);

					iter = g_list_first(lang_list);
				} 
				SLOG(LOG_DEBUG, TAG_STTD, "[OUT SUCCESS] Result(%d) ", ret); 
			}
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

int sttd_dbus_server_setting_get_default_language(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid;
	char* language;
	int ret = STTD_ERROR_OPERATION_FAILED;

	dbus_message_get_args(msg, &err,
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_STTD, ">>>>> Setting Get Default Language");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_STTD, "[IN ERROR] setting get default language : get arguments error (%s)", err.message);
		dbus_error_free(&err); 
		ret = STTD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTD, "[IN] setting get default language : uid(%d)", pid); 
		ret = sttd_server_setting_get_default_language(pid, &language);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		if (0 == ret) {
			dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_STRING, &language, DBUS_TYPE_INVALID);
			SLOG(LOG_DEBUG, TAG_STTD, "[OUT SUCCESS] Result(%d), Default Language(%s)", ret, language); 
			free(language);		
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

	SLOG(LOG_DEBUG, TAG_STTD, "<<<<<");
	SLOG(LOG_DEBUG, TAG_STTD, "  ");

	return 0;
}

int sttd_dbus_server_setting_set_default_language(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid;
	char* language;
	int ret = STTD_ERROR_OPERATION_FAILED;

	dbus_message_get_args(msg, &err,
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_STRING, &language,
		DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_STTD, ">>>>> Setting Set Default Language");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_STTD, "[IN ERROR] setting set default language : get arguments error (%s)", err.message);
		dbus_error_free(&err); 
		ret = STTD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTD, "[IN] setting set default language : uid(%d), language(%s)", pid, language); 
		ret = sttd_server_setting_set_default_language(pid, language);
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

int sttd_dbus_server_setting_get_profanity_filter(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid;
	bool value;
	int ret = STTD_ERROR_OPERATION_FAILED;

	dbus_message_get_args(msg, &err,
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_STTD, ">>>>> Setting Get Profanity Filter");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_STTD, "[IN ERROR] setting get profanity filter : get arguments error (%s)", err.message);
		dbus_error_free(&err); 
		ret = STTD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTD, "[IN] setting get profanity filter : uid(%d)", pid); 
		ret = sttd_server_setting_get_profanity_filter(pid, &value);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		if (0 == ret) {
			dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INT32, &value, DBUS_TYPE_INVALID);
			SLOG(LOG_DEBUG, TAG_STTD, "[OUT SUCCESS] Result(%d) , value(%s)", ret, value ? "true":"false"); 
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

	SLOG(LOG_DEBUG, TAG_STTD, "<<<<<");
	SLOG(LOG_DEBUG, TAG_STTD, "  ");

	return 0;
}

int sttd_dbus_server_setting_set_profanity_filter(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid;
	bool value;
	int ret = STTD_ERROR_OPERATION_FAILED;

	dbus_message_get_args(msg, &err,
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_INT32, &value,
		DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_STTD, ">>>>> Setting Set Profanity Filter");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_STTD, "[IN ERROR] setting set profanity filter : get arguments error (%s)", err.message);
		dbus_error_free(&err); 
		ret = STTD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTD, "[IN] setting set profanity filter : uid(%d), value(%s)", pid, value ? "true":"false"); 
		ret =  sttd_server_setting_set_profanity_filter(pid, (bool)value);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

	if (NULL != reply) {
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

int sttd_dbus_server_setting_get_punctuation_override(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid;
	bool value;
	int ret = STTD_ERROR_OPERATION_FAILED;

	dbus_message_get_args(msg, &err,
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_STTD, ">>>>> Setting Get Punctuation Override");
	
	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_STTD, "[IN ERROR] setting get punctuation override : get arguments error (%s)", err.message);
		dbus_error_free(&err); 
		ret = STTD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTD, "[IN] setting get punctuation override : uid(%d)", pid); 
		ret =  sttd_server_setting_get_punctuation_override(pid, &value);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		if (0 == ret) {
			dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INT32, &value, DBUS_TYPE_INVALID);
			SLOG(LOG_DEBUG, TAG_STTD, "[OUT SUCCESS] Result(%d) , value(%s)", ret, value ? "true":"false"); 
		} else {
			dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);
			SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Result(%d)", ret); 
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Out Of Memory!");
			return -1;
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

int sttd_dbus_server_setting_set_punctuation_override(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid;
	bool value;
	int ret = STTD_ERROR_OPERATION_FAILED;

	dbus_message_get_args(msg, &err,
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_INT32, &value,
		DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_STTD, ">>>>> Setting Set Profanity Filter");
	
	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_STTD, "[IN ERROR] setting set punctuation override : get arguments error (%s)", err.message);
		dbus_error_free(&err); 
		ret = STTD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTD, "[IN] setting set punctuation override : uid(%d), value(%s)", pid, value ? "true":"false"); 
		ret =  sttd_server_setting_set_punctuation_override(pid, value);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

	if (NULL != reply) {
		if (0 == ret) {
			SLOG(LOG_DEBUG, TAG_STTD, "[OUT SUCCESS] Result(%d)", ret); 
		} else {
			SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Result(%d)", ret); 
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Out Of Memory!");
			return -1;
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

int sttd_dbus_server_setting_get_silence_detection(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid;
	bool value;
	int ret = STTD_ERROR_OPERATION_FAILED;

	dbus_message_get_args(msg, &err,
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_STTD, ">>>>> Setting Get Silence Detection");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_STTD, "[IN ERROR] setting get silence detection : get arguments error (%s)", err.message);
		dbus_error_free(&err); 
		ret = STTD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTD, "[IN] setting get silence detection : uid(%d)", pid); 
		ret =  sttd_server_setting_get_silence_detection(pid, &value);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		if (0 == ret) {
			dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INT32, &value, DBUS_TYPE_INVALID);
			SLOG(LOG_DEBUG, TAG_STTD, "[OUT SUCCESS] Result(%d) , Value(%s)", ret, value ? "true":"false"); 
		} else {
			dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);
			SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Result(%d)", ret); 
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Out Of Memory!");
			return -1;
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

int sttd_dbus_server_setting_set_silence_detection(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid;
	bool value;
	int ret = STTD_ERROR_OPERATION_FAILED;

	dbus_message_get_args(msg, &err,
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_INT32, &value,
		DBUS_TYPE_INVALID);
	SLOG(LOG_DEBUG, TAG_STTD, ">>>>> Setting Set Silence Detection");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_STTD, "[IN ERROR] setting set silence detection : get arguments error (%s)", err.message);
		dbus_error_free(&err); 
		ret = STTD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTD, "[IN] setting set silence detection : uid(%d), value(%s)", pid, value ? "true":"false"); 
		ret =  sttd_server_setting_set_silence_detection(pid, value);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

	if (NULL != reply) {
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


int sttd_dbus_server_setting_get_engine_setting(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid;
	char* engine_id;
	GList* engine_setting_list = NULL;
	int ret = STTD_ERROR_OPERATION_FAILED;

	dbus_message_get_args(msg, &err,
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_STTD, ">>>>> Setting Get Engine Setting");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_STTD, "[IN ERROR] setting get engine setting : get arguments error (%s)", err.message);
		dbus_error_free(&err); 
		ret = STTD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTD, "[IN] setting get engine setting : uid(%d)", pid); 
		ret = sttd_server_setting_get_engine_setting(pid, &engine_id, &engine_setting_list);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		DBusMessageIter args;
		dbus_message_iter_init_append(reply, &args);

		/* Append result*/
		dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &(ret) );

		if (0 == ret) {
			if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &(engine_id))) {
				ret = STTD_ERROR_OPERATION_FAILED;
				SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Fail to add engine id");
			} else {
				if (NULL != engine_id)	free(engine_id);

				/* Append size */
				int size = g_list_length(engine_setting_list);
				if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &(size))) {
					ret = STTD_ERROR_OPERATION_FAILED;
					SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] setting get engine setting : result(%d)", ret);
				} else {
					GList *iter = NULL;
					engine_setting_s* setting;

					iter = g_list_first(engine_setting_list);

					while (NULL != iter) {
						setting = iter->data;

						dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &(setting->key) );
						dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &(setting->value) );

						if (NULL != setting->key)
							g_free(setting->key);
						if (NULL != setting->value)
							g_free(setting->value);
						if (NULL != setting);
							g_free(setting);

						engine_setting_list = g_list_remove_link(engine_setting_list, iter);

						iter = g_list_first(engine_setting_list);
					} 
					SLOG(LOG_DEBUG, TAG_STTD, "[OUT SUCCESS] setting engine setting list : result(%d) \n", ret); 
				}
			}
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

int sttd_dbus_server_setting_set_engine_setting(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid;
	char* key;
	char* value;
	int ret = STTD_ERROR_OPERATION_FAILED;

	dbus_message_get_args(msg, &err,
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_STRING, &key,
		DBUS_TYPE_STRING, &value,
		DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_STTD, ">>>>> Setting Set Engine Setting");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_STTD, "[IN ERROR] setting set engine setting : get arguments error (%s)", err.message);
		dbus_error_free(&err); 
		ret = STTD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_STTD, "[IN] setting set engine setting : uid(%d), key(%s), value(%s)", pid, key, value); 
		ret = sttd_server_setting_set_engine_setting(pid, key, value);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

	if (NULL != reply) {
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

int sttd_dbus_server_stop_by_daemon(DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int uid;

	dbus_message_get_args(msg, &err,
		DBUS_TYPE_INT32, &uid,
		DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_STTD, ">>>>> Stop by daemon");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_STTD, "[IN ERROR] sttd stop by daemon : Get arguments error (%s)", err.message);
		dbus_error_free(&err); 
	} else {
		SLOG(LOG_DEBUG, TAG_STTD, "[IN] sttd stop by daemon : uid(%d)", uid);
		sttd_server_stop(uid);

		/* check silence detection option from config */
		int ret = sttdc_send_set_state(uid, (int)APP_STATE_PROCESSING);
		if (0 == ret) {
			SLOG(LOG_DEBUG, TAG_STTD, "[OUT SUCCESS] Result(%d)", ret); 
		} else {
			SLOG(LOG_ERROR, TAG_STTD, "[OUT ERROR] Result(%d)", ret); 
			/* Remove client */
			sttd_server_finalize(uid);
		}
	}
	
	SLOG(LOG_DEBUG, TAG_STTD, "<<<<<");
	SLOG(LOG_DEBUG, TAG_STTD, "  ");

	return 0;
}
