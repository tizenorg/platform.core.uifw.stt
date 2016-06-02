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

#include <aul.h>
#include <cynara-client.h>
#include <cynara-error.h>
#include <cynara-session.h>
#include <dirent.h>
#include <Ecore.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <system_info.h>
#include <unistd.h>

#include "stt.h"
#include "stt_client.h"
#include "stt_dbus.h"
#include "stt_config_mgr.h"
#include "stt_main.h"


static void __stt_notify_state_changed(void *data);
static Eina_Bool __stt_notify_error(void *data);

static Ecore_Timer* g_connect_timer = NULL;
static float g_volume_db = 0;

static int g_feature_enabled = -1;

static int g_privilege_allowed = -1;
static cynara *p_cynara = NULL;

static bool g_err_callback_status = false;

const char* stt_tag()
{
	return "sttc";
}

static int __stt_get_feature_enabled()
{
	if (0 == g_feature_enabled) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] STT NOT supported");
		return STT_ERROR_NOT_SUPPORTED;
	} else if (-1 == g_feature_enabled) {
		bool stt_supported = false;
		bool mic_supported = false;
		if (0 == system_info_get_platform_bool(STT_FEATURE_PATH, &stt_supported)) {
			if (0 == system_info_get_platform_bool(STT_MIC_FEATURE_PATH, &mic_supported)) {
				if (false == stt_supported || false == mic_supported) {
					SLOG(LOG_ERROR, TAG_STTC, "[ERROR] STT NOT supported");
					g_feature_enabled = 0;
					return STT_ERROR_NOT_SUPPORTED;
				}

				g_feature_enabled = 1;
			} else {
				SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to get feature value");
				return STT_ERROR_NOT_SUPPORTED;
			}
		} else {
			SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to get feature value");
			return STT_ERROR_NOT_SUPPORTED;
		}
	}

	return 0;
}

static int __check_privilege_initialize()
{
	int ret = cynara_initialize(&p_cynara, NULL);
	if (CYNARA_API_SUCCESS != ret)
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] fail to initialize");
	
	return ret == CYNARA_API_SUCCESS;
}

static int __check_privilege(const char* uid, const char * privilege)
{
	FILE *fp = NULL;
	char smack_label[1024] = "/proc/self/attr/current";

	if (!p_cynara) {
	    return false;
	}

	fp = fopen(smack_label, "r");
	if (fp != NULL) {
	    if (fread(smack_label, 1, sizeof(smack_label), fp) <= 0)
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] fail to fread");

	    fclose(fp);
	}

	pid_t pid = getpid();
	char *session = cynara_session_from_pid(pid);
	int ret = cynara_check(p_cynara, smack_label, session, uid, privilege);
	SLOG(LOG_DEBUG, TAG_STTC, "[Client]cynara_check returned %d(%s)", ret, (CYNARA_API_ACCESS_ALLOWED == ret) ? "Allowed" : "Denied");
	if (session)
	    free(session);

	if (ret != CYNARA_API_ACCESS_ALLOWED)
	    return false;
	return true;
}

static void __check_privilege_deinitialize()
{
	if (p_cynara)
		cynara_finish(p_cynara);
	p_cynara = NULL;
}

static int __stt_check_privilege()
{
	char uid[16];

	if (0 == g_privilege_allowed) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Permission is denied");
		return STT_ERROR_PERMISSION_DENIED;
	} else if (-1 == g_privilege_allowed) {
		if (false == __check_privilege_initialize()){
			SLOG(LOG_ERROR, TAG_STTC, "[ERROR] privilege initialize is failed");
			return STT_ERROR_PERMISSION_DENIED;
		}
		snprintf(uid, 16, "%d", getuid());
		if (false == __check_privilege(uid, STT_PRIVILEGE)) {
			SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Permission is denied");
			g_privilege_allowed = 0;
			__check_privilege_deinitialize();
			return STT_ERROR_PERMISSION_DENIED;
		}
		__check_privilege_deinitialize();
	}

	g_privilege_allowed = 1;
	return STT_ERROR_NONE;	
}

static const char* __stt_get_error_code(stt_error_e err)
{
	switch (err) {
	case STT_ERROR_NONE:			return "STT_ERROR_NONE";
	case STT_ERROR_OUT_OF_MEMORY:		return "STT_ERROR_OUT_OF_MEMORY";
	case STT_ERROR_IO_ERROR:		return "STT_ERROR_IO_ERROR";
	case STT_ERROR_INVALID_PARAMETER:	return "STT_ERROR_INVALID_PARAMETER";
	case STT_ERROR_TIMED_OUT:		return "STT_ERROR_TIMED_OUT";
	case STT_ERROR_RECORDER_BUSY:		return "STT_ERROR_RECORDER_BUSY";
	case STT_ERROR_OUT_OF_NETWORK:		return "STT_ERROR_OUT_OF_NETWORK";
	case STT_ERROR_PERMISSION_DENIED:	return "STT_ERROR_PERMISSION_DENIED";
	case STT_ERROR_NOT_SUPPORTED:		return "STT_ERROR_NOT_SUPPORTED";
	case STT_ERROR_INVALID_STATE:		return "STT_ERROR_INVALID_STATE";
	case STT_ERROR_INVALID_LANGUAGE:	return "STT_ERROR_INVALID_LANGUAGE";
	case STT_ERROR_ENGINE_NOT_FOUND:	return "STT_ERROR_ENGINE_NOT_FOUND";
	case STT_ERROR_OPERATION_FAILED:	return "STT_ERROR_OPERATION_FAILED";
	case STT_ERROR_NOT_SUPPORTED_FEATURE:	return "STT_ERROR_NOT_SUPPORTED_FEATURE";
	default:
		return "Invalid error code";
	}
}

static int __stt_convert_config_error_code(stt_config_error_e code)
{
	if (code == STT_CONFIG_ERROR_NONE)			return STT_ERROR_NONE;
	if (code == STT_CONFIG_ERROR_OUT_OF_MEMORY)		return STT_ERROR_OUT_OF_MEMORY;
	if (code == STT_CONFIG_ERROR_IO_ERROR)			return STT_ERROR_IO_ERROR;
	if (code == STT_CONFIG_ERROR_INVALID_PARAMETER)		return STT_ERROR_INVALID_PARAMETER;
	if (code == STT_CONFIG_ERROR_PERMISSION_DENIED)		return STT_ERROR_PERMISSION_DENIED;
	if (code == STT_CONFIG_ERROR_NOT_SUPPORTED)		return STT_ERROR_NOT_SUPPORTED;
	if (code == STT_CONFIG_ERROR_INVALID_STATE)		return STT_ERROR_INVALID_STATE;
	if (code == STT_CONFIG_ERROR_INVALID_LANGUAGE)		return STT_ERROR_INVALID_LANGUAGE;
	if (code == STT_CONFIG_ERROR_ENGINE_NOT_FOUND)		return STT_ERROR_ENGINE_NOT_FOUND;
	if (code == STT_CONFIG_ERROR_OPERATION_FAILED)		return STT_ERROR_OPERATION_FAILED;

	return code;
}

void __stt_config_lang_changed_cb(const char* before_language, const char* current_language, void* user_data)
{
	SLOG(LOG_DEBUG, TAG_STTC, "Language changed : Before lang(%s) Current lang(%s)",
		before_language, current_language);

	if (0 == strcmp(before_language, current_language)) {
		return;
	}

	GList* client_list = NULL;
	client_list = stt_client_get_client_list();

	GList *iter = NULL;
	stt_client_s *data = NULL;

	if (g_list_length(client_list) > 0) {
		/* Get a first item */
		iter = g_list_first(client_list);

		while (NULL != iter) {
			data = iter->data;
			if (NULL != data->default_lang_changed_cb) {
				SLOG(LOG_DEBUG, TAG_STTC, "Call default language changed callback : uid(%d)", data->uid);
				data->default_lang_changed_cb(data->stt, before_language, current_language,
					data->default_lang_changed_user_data);
			}

			/* Next item */
			iter = g_list_next(iter);
		}
	}

	return;
}

void __stt_config_engine_changed_cb(const char* engine_id, const char* setting, const char* language, bool support_silence, bool need_credential, void* user_data)
{
	stt_h stt = (stt_h)user_data;

	stt_client_s* client = stt_client_get(stt);
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[WARNING] A handle is not valid");
		return;
	}

	if (NULL != engine_id)	SLOG(LOG_DEBUG, TAG_STTC, "Engine id(%s)", engine_id);
	if (NULL != setting)	SLOG(LOG_DEBUG, TAG_STTC, "Engine setting(%s)", setting);
	if (NULL != language)	SLOG(LOG_DEBUG, TAG_STTC, "Language(%s)", language);
	SLOG(LOG_DEBUG, TAG_STTC, "Silence(%s), Credential(%s)", support_silence ? "on" : "off", need_credential ? "need" : "no need");

	/* call callback function */
	if (NULL != client->engine_changed_cb) {
		client->engine_changed_cb(stt, engine_id, language, support_silence, need_credential, client->engine_changed_user_data);
	} else {
		SLOG(LOG_WARN, TAG_STTC, "No registered callback function of supported languages");
	}
	return;
}

static int __stt_check_handle(stt_h stt, stt_client_s** client)
{
	if (NULL == stt) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Input handle is null");
		return STT_ERROR_INVALID_PARAMETER;
	}

	stt_client_s* temp = NULL;
	temp = stt_client_get(stt);

	/* check handle */
	if (NULL == temp) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] A handle is not available");
		return STT_ERROR_INVALID_PARAMETER;
	}
	*client = temp;

	return STT_ERROR_NONE;
}

int stt_create(stt_h* stt)
{
	if (0 != __stt_get_feature_enabled()) {
		return STT_ERROR_NOT_SUPPORTED;
	}
	if (0 != __stt_check_privilege()) {
		return STT_ERROR_PERMISSION_DENIED;
	}

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

	stt_client_s* client = stt_client_get(*stt);
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to create client");
		stt_client_destroy(*stt);
		return STT_ERROR_OPERATION_FAILED;
	}

	int ret = stt_config_mgr_initialize(client->uid);
	ret = __stt_convert_config_error_code(ret);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to init config manager : %s", __stt_get_error_code(ret));
		stt_client_destroy(*stt);
		return ret;
	}

	ret = stt_config_mgr_set_callback(client->uid, __stt_config_engine_changed_cb, __stt_config_lang_changed_cb, NULL, client->stt);
	ret = __stt_convert_config_error_code(ret);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to set config changed : %s", __stt_get_error_code(ret));
		stt_client_destroy(*stt);
		return ret;
	}

	SLOG(LOG_DEBUG, TAG_STTC, "[Success] uid(%d)", (*stt)->handle);

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return STT_ERROR_NONE;
}

int stt_destroy(stt_h stt)
{
	stt_client_s* client = NULL;
	if (0 != __stt_get_feature_enabled()) {
		return STT_ERROR_NOT_SUPPORTED;
	}
	if (0 != __stt_check_privilege()) {
		return STT_ERROR_PERMISSION_DENIED;
	}
	if (0 != __stt_check_handle(stt, &client)) {
		return STT_ERROR_INVALID_PARAMETER;
	}

	SLOG(LOG_DEBUG, TAG_STTC, "===== Destroy STT");

	/* check used callback */
	if (0 != stt_client_get_use_callback(client)) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Cannot destroy in Callback function");
		return STT_ERROR_OPERATION_FAILED;
	}

	stt_config_mgr_finalize(client->uid);

	int ret = -1;

	/* check state */
	switch (client->current_state) {
	case STT_STATE_PROCESSING:
	case STT_STATE_RECORDING:
	case STT_STATE_READY:
		ret = stt_dbus_request_finalize(client->uid);
		if (0 != ret) {
			SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to request finalize : %s", __stt_get_error_code(ret));
		}
	case STT_STATE_CREATED:
		if (NULL != g_connect_timer) {
			SLOG(LOG_DEBUG, TAG_STTC, "Connect Timer is deleted");
			ecore_timer_del(g_connect_timer);
			g_connect_timer = NULL;
		}

		/* Free resources */
		stt_client_destroy(stt);
		break;
	default:
		break;
	}

	if (0 == stt_client_get_size()) {
		if (0 != stt_dbus_close_connection()) {
			SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to close connection");
		}
	}

	stt = NULL;

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return STT_ERROR_NONE;
}

bool __stt_config_supported_engine_cb(const char* engine_id, const char* engine_name,
				      const char* setting, bool support_silence, void* user_data)
{
	stt_h stt = (stt_h)user_data;

	stt_client_s* client = stt_client_get(stt);
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[WARNING] A handle is not valid");
		return false;
	}

	/* call callback function */
	if (NULL != client->supported_engine_cb) {
		return client->supported_engine_cb(stt, engine_id, engine_name, client->supported_engine_user_data);
	} else {
		SLOG(LOG_WARN, TAG_STTC, "No registered callback function of supported engine");
	}

	return false;
}

int stt_foreach_supported_engines(stt_h stt, stt_supported_engine_cb callback, void* user_data)
{
	stt_client_s* client = NULL;
	if (0 != __stt_get_feature_enabled()) {
		return STT_ERROR_NOT_SUPPORTED;
	}
	if (0 != __stt_check_privilege()) {
		return STT_ERROR_PERMISSION_DENIED;
	}
	if (0 != __stt_check_handle(stt, &client)) {
		return STT_ERROR_INVALID_PARAMETER;
	}

	SLOG(LOG_DEBUG, TAG_STTC, "===== Foreach Supported engine");

	if (NULL == callback) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Input parameter is NULL");
		return STT_ERROR_INVALID_PARAMETER;
	}

	if (client->current_state != STT_STATE_CREATED) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid State: Current state(%d) is not CREATED", client->current_state);
		return STT_ERROR_INVALID_STATE;
	}

	client->supported_engine_cb = callback;
	client->supported_engine_user_data = user_data;

	int ret = 0;
	ret = stt_config_mgr_get_engine_list(__stt_config_supported_engine_cb, client->stt);
	ret = __stt_convert_config_error_code(ret);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to get engines : %s", __stt_get_error_code(ret));
	}

	client->supported_engine_cb = NULL;
	client->supported_engine_user_data = NULL;

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return ret;
}

int stt_get_engine(stt_h stt, char** engine_id)
{
	stt_client_s* client = NULL;
	if (0 != __stt_get_feature_enabled()) {
		return STT_ERROR_NOT_SUPPORTED;
	}
	if (0 != __stt_check_privilege()) {
		return STT_ERROR_PERMISSION_DENIED;
	}
	if (0 != __stt_check_handle(stt, &client)) {
		return STT_ERROR_INVALID_PARAMETER;
	}

	SLOG(LOG_DEBUG, TAG_STTC, "===== Get current engine");

	if (NULL == stt || NULL == engine_id) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Input parameter is NULL");
		return STT_ERROR_INVALID_PARAMETER;
	}

	if (client->current_state != STT_STATE_CREATED) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid State: Current state(%d) is not CREATED", client->current_state);
		return STT_ERROR_INVALID_STATE;
	}

	int ret = 0;

	if (NULL != client->current_engine_id) {
		*engine_id = strdup(client->current_engine_id);
		SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Current engine uuid = %s", *engine_id);
	} else {

		ret = stt_config_mgr_get_engine(engine_id);
		ret = __stt_convert_config_error_code(ret);
		if (0 != ret) {
			SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to request get current engine : %s", __stt_get_error_code(ret));
		} else {
			SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Current engine uuid = %s", *engine_id);
		}
	}

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return ret;
}

int stt_set_engine(stt_h stt, const char* engine_id)
{
	stt_client_s* client = NULL;
	if (0 != __stt_get_feature_enabled()) {
		return STT_ERROR_NOT_SUPPORTED;
	}
	if (0 != __stt_check_privilege()) {
		return STT_ERROR_PERMISSION_DENIED;
	}
	if (0 != __stt_check_handle(stt, &client)) {
		return STT_ERROR_INVALID_PARAMETER;
	}

	SLOG(LOG_DEBUG, TAG_STTC, "===== Set current engine");

	if (NULL == engine_id) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Input parameter is NULL");
		return STT_ERROR_INVALID_PARAMETER;
	}

	/* check state */
	if (client->current_state != STT_STATE_CREATED) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid State: Current state(%d) is not CREATED", client->current_state);
		return STT_ERROR_INVALID_STATE;
	}

	if (NULL != client->current_engine_id) {
		free(client->current_engine_id);
	}

	client->current_engine_id = strdup(engine_id);

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return 0;
}

int stt_set_credential(stt_h stt, const char* credential)
{
	stt_client_s* client = NULL;
	if (0 != __stt_get_feature_enabled()) {
		return STT_ERROR_NOT_SUPPORTED;
	}
	if (0 != __stt_check_privilege()) {
		return STT_ERROR_PERMISSION_DENIED;
	}
	if (0 != __stt_check_handle(stt, &client)) {
		return STT_ERROR_INVALID_PARAMETER;
	}

	SLOG(LOG_DEBUG, TAG_STTC, "===== Set credential");

	if (NULL == credential) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Input parameter is NULL, stt(%s), credential(%a)", stt, credential);
		return STT_ERROR_INVALID_PARAMETER;
	}

	/* check state */
	if (client->current_state != STT_STATE_CREATED && client->current_state != STT_STATE_READY) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid State: Current state(%d) is not CREATED or READY", client->current_state);
		return STT_ERROR_INVALID_STATE;
	}

	client->credential = strdup(credential);

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return STT_ERROR_NONE;
}

int stt_set_private_data(stt_h stt, const char* key, const char* data)
{
	stt_client_s* client = NULL;
	if (0 != __stt_get_feature_enabled()) {
		return STT_ERROR_NOT_SUPPORTED;
	}
	if (0 != __stt_check_privilege()) {
		return STT_ERROR_PERMISSION_DENIED;
	}
	if (0 != __stt_check_handle(stt, &client)) {
		return STT_ERROR_INVALID_PARAMETER;
	}

	SLOG(LOG_DEBUG, TAG_STTC, "===== Set private data");

	if (NULL == key || NULL == data) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid parameter");
		return STT_ERROR_INVALID_PARAMETER;
	}

	/* check state */
	if (STT_STATE_READY != client->current_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid State: Current state(%d) is not READY", client->current_state);
		return STT_ERROR_INVALID_STATE;
	}

	int ret = -1;
	int count = 0;
	while (0 != ret) {
		ret = stt_dbus_request_set_private_data(client->uid, key, data);
		if (0 != ret) {
			if (STT_ERROR_TIMED_OUT != ret) {
				SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to set private data : %s", __stt_get_error_code(ret));
				return ret;
			} else {
				SLOG(LOG_WARN, TAG_STTC, "[WARNING] retry : %s", __stt_get_error_code(ret));
				usleep(10000);
				count++;
				if (STT_RETRY_COUNT == count) {
					SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to request");
					return ret;
				}
			}
		}
	}

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, "");

	return STT_ERROR_NONE;

}
int stt_get_private_data(stt_h stt, const char* key, char** data)
{
	stt_client_s* client = NULL;
	if (0 != __stt_get_feature_enabled()) {
		return STT_ERROR_NOT_SUPPORTED;
	}
	if (0 != __stt_check_privilege()) {
		return STT_ERROR_PERMISSION_DENIED;
	}
	if (0 != __stt_check_handle(stt, &client)) {
		return STT_ERROR_INVALID_PARAMETER;
	}

	SLOG(LOG_DEBUG, TAG_STTC, "===== Get private data");

	if (NULL == key || NULL == data) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid parameter");
		return STT_ERROR_INVALID_PARAMETER;
	}

	/* check state */
	if (STT_STATE_READY != client->current_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid State: Current state(%d) is not READY", client->current_state);
		return STT_ERROR_INVALID_STATE;
	}

	int ret = -1;
	int count = 0;
	while (0 != ret) {
		ret = stt_dbus_request_get_private_data(client->uid, key, data);
		if (0 != ret) {
			if (STT_ERROR_TIMED_OUT != ret) {
				SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to get private data : %s", __stt_get_error_code(ret));
				return ret;
			} else {
				SLOG(LOG_WARN, TAG_STTC, "[WARNING] retry : %s", __stt_get_error_code(ret));
				usleep(10000);
				count++;
				if (STT_RETRY_COUNT == count) {
					SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to request");
					return ret;
				}
			}
		}
	}

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, "");

	return STT_ERROR_NONE;
}
static Eina_Bool __stt_connect_daemon(void *data)
{
	stt_client_s* client = (stt_client_s*)data;

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] A handle is not available");
		g_connect_timer = NULL;
		return EINA_FALSE;
	}

	/* Send hello */
	int ret = -1;
	ret = stt_dbus_request_hello();

	if (0 != ret) {
		if (STT_ERROR_INVALID_STATE == ret) {
			g_connect_timer = NULL;
			return EINA_FALSE;
		}
		return EINA_TRUE;
	}

	g_connect_timer = NULL;
	SLOG(LOG_DEBUG, TAG_STTC, "===== Connect daemon");

	/* request initialization */
	bool silence_supported = false;
	bool credential_needed = false;

	ret = stt_dbus_request_initialize(client->uid, &silence_supported, &credential_needed);

	if (STT_ERROR_ENGINE_NOT_FOUND == ret) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to initialize : %s", __stt_get_error_code(ret));

		client->reason = STT_ERROR_ENGINE_NOT_FOUND;
		ecore_timer_add(0, __stt_notify_error, (void*)client);

		return EINA_FALSE;

	} else if (STT_ERROR_NONE != ret) {
		SLOG(LOG_ERROR, TAG_STTC, "[WARNING] Fail to connection. Retry to connect");
		return EINA_TRUE;
	} else {
		/* success to connect stt-daemon */
		client->silence_supported = silence_supported;
		client->credential_needed = credential_needed;
		SLOG(LOG_DEBUG, TAG_STTC, "Supported options : silence(%s), credential(%s)", silence_supported ? "support" : "no support", credential_needed ? "need" : "no need");
	}

	if (NULL != client->current_engine_id) {
		ret = -1;
		int count = 0;
		silence_supported = false;
		credential_needed = false;
		while (0 != ret) {
			ret = stt_dbus_request_set_current_engine(client->uid, client->current_engine_id, &silence_supported, &credential_needed);
			if (0 != ret) {
				if (STT_ERROR_TIMED_OUT != ret) {
					SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to set current engine : %s", __stt_get_error_code(ret));
					return ret;
				} else {
					SLOG(LOG_WARN, TAG_STTC, "[WARNING] retry");
					usleep(10000);
					count++;
					if (STT_RETRY_COUNT == count) {
						SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to request");
						return ret;
					}
				}
			} else {
				SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Current engine uuid = %s", client->current_engine_id);

				/* success to change engine */
				client->silence_supported = silence_supported;
				SLOG(LOG_DEBUG, TAG_STTC, "Supported options : silence(%s), credential(%s)", silence_supported ? "support" : "no support", credential_needed ? "need" : "no need");
			}
		}
	}

	SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] uid(%d)", client->uid);

	client->before_state = client->current_state;
	client->current_state = STT_STATE_READY;

	if (NULL != client->state_changed_cb) {
		stt_client_use_callback(client);
		client->state_changed_cb(client->stt, client->before_state,
			client->current_state, client->state_changed_user_data);
		stt_client_not_use_callback(client);
		SLOG(LOG_DEBUG, TAG_STTC, "State changed callback is called");
	} else {
		SLOG(LOG_WARN, TAG_STTC, "[WARNING] State changed callback is null");
	}

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, "  ");

	return EINA_FALSE;
}

int stt_prepare(stt_h stt)
{
	stt_client_s* client = NULL;
	if (0 != __stt_get_feature_enabled()) {
		return STT_ERROR_NOT_SUPPORTED;
	}
	if (0 != __stt_check_privilege()) {
		return STT_ERROR_PERMISSION_DENIED;
	}
	if (0 != __stt_check_handle(stt, &client)) {
		return STT_ERROR_INVALID_PARAMETER;
	}

	/* check state */
	if (client->current_state != STT_STATE_CREATED) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid State: Current state(%d) is not 'CREATED'", client->current_state);
		return STT_ERROR_INVALID_STATE;
	}

	g_connect_timer = ecore_timer_add(0, __stt_connect_daemon, (void*)client);

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return STT_ERROR_NONE;
}

int stt_unprepare(stt_h stt)
{
	stt_client_s* client = NULL;
	if (0 != __stt_get_feature_enabled()) {
		return STT_ERROR_NOT_SUPPORTED;
	}
	if (0 != __stt_check_privilege()) {
		return STT_ERROR_PERMISSION_DENIED;
	}
	if (0 != __stt_check_handle(stt, &client)) {
		return STT_ERROR_INVALID_PARAMETER;
	}

	SLOG(LOG_DEBUG, TAG_STTC, "===== Unprepare STT");

	/* check state */
	if (client->current_state != STT_STATE_READY) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid State: Current state(%d) is not 'READY'", client->current_state);
		return STT_ERROR_INVALID_STATE;
	}

	int ret = -1;
	int count = 0;
	while (0 != ret) {
		ret = stt_dbus_request_finalize(client->uid);
		if (0 != ret) {
			if (STT_ERROR_TIMED_OUT != ret) {
				SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to request finalize : %s", __stt_get_error_code(ret));
				break;
			} else {
				SLOG(LOG_WARN, TAG_STTC, "[WARNING] retry");
				usleep(10000);
				count++;
				if (STT_RETRY_COUNT == count) {
					SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to request");
					break;
				}
			}
		}
	}

	client->internal_state = STT_INTERNAL_STATE_NONE;

	client->before_state = client->current_state;
	client->current_state = STT_STATE_CREATED;

	if (NULL != client->state_changed_cb) {
		stt_client_use_callback(client);
		client->state_changed_cb(client->stt, client->before_state,
			client->current_state, client->state_changed_user_data);
		stt_client_not_use_callback(client);
	} else {
		SLOG(LOG_WARN, TAG_STTC, "[WARNING] State changed callback is null");
	}

	if (g_connect_timer) {
		ecore_timer_del(g_connect_timer);
		g_connect_timer = NULL;
	}

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return STT_ERROR_NONE;
}

bool __stt_config_supported_language_cb(const char* engine_id, const char* language, void* user_data)
{
	stt_h stt = (stt_h)user_data;

	stt_client_s* client = stt_client_get(stt);
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[WARNING] A handle is not valid");
		return false;
	}

	/* call callback function */
	if (NULL != client->supported_lang_cb) {
		return client->supported_lang_cb(stt, language, client->supported_lang_user_data);
	} else {
		SLOG(LOG_WARN, TAG_STTC, "No registered callback function of supported languages");
	}

	return false;
}

int stt_foreach_supported_languages(stt_h stt, stt_supported_language_cb callback, void* user_data)
{
	stt_client_s* client = NULL;
	if (0 != __stt_get_feature_enabled()) {
		return STT_ERROR_NOT_SUPPORTED;
	}
	if (0 != __stt_check_privilege()) {
		return STT_ERROR_PERMISSION_DENIED;
	}
	if (0 != __stt_check_handle(stt, &client)) {
		return STT_ERROR_INVALID_PARAMETER;
	}

	SLOG(LOG_DEBUG, TAG_STTC, "===== Foreach Supported Language");

	if (NULL == callback) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Input parameter is NULL");
		return STT_ERROR_INVALID_PARAMETER;
	}

	int ret;
	char* current_engine_id = NULL;

	if (NULL == client->current_engine_id) {
		ret = stt_config_mgr_get_engine(&current_engine_id);
		ret = __stt_convert_config_error_code(ret);
		if (0 != ret) {
			SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to get default engine id : %s", __stt_get_error_code(ret));
			return ret;
		}
	} else {
		current_engine_id = strdup(client->current_engine_id);
		if (NULL == current_engine_id) {
			SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to allocate memory");
			return STT_ERROR_OUT_OF_MEMORY;
		}
	}

	client->supported_lang_cb = callback;
	client->supported_lang_user_data = user_data;

	ret = stt_config_mgr_get_language_list(current_engine_id, __stt_config_supported_language_cb, client->stt);
	ret = __stt_convert_config_error_code(ret);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to get languages : %s", __stt_get_error_code(ret));
	}

	if (NULL != current_engine_id) {
		free(current_engine_id);
	}

	client->supported_lang_cb = NULL;
	client->supported_lang_user_data = NULL;

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return ret;
}

int stt_get_default_language(stt_h stt, char** language)
{
	stt_client_s* client = NULL;
	if (0 != __stt_get_feature_enabled()) {
		return STT_ERROR_NOT_SUPPORTED;
	}
	if (0 != __stt_check_privilege()) {
		return STT_ERROR_PERMISSION_DENIED;
	}
	if (0 != __stt_check_handle(stt, &client)) {
		return STT_ERROR_INVALID_PARAMETER;
	}

	SLOG(LOG_DEBUG, TAG_STTC, "===== Get Default Language");

	if (NULL == language) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Input parameter is NULL");
		return STT_ERROR_INVALID_PARAMETER;
	}

	int ret = 0;
	ret = stt_config_mgr_get_default_language(language);
	ret = __stt_convert_config_error_code(ret);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to get default language : %s", __stt_get_error_code(ret));
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Current language = %s", *language);
	}

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return ret;
}

int stt_get_state(stt_h stt, stt_state_e* state)
{
	stt_client_s* client = NULL;
	if (0 != __stt_get_feature_enabled()) {
		return STT_ERROR_NOT_SUPPORTED;
	}
	if (0 != __stt_check_privilege()) {
		return STT_ERROR_PERMISSION_DENIED;
	}
	if (0 != __stt_check_handle(stt, &client)) {
		return STT_ERROR_INVALID_PARAMETER;
	}

	if (NULL == state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Input parameter is NULL");
		return STT_ERROR_INVALID_PARAMETER;
	}

	*state = client->current_state;

	switch (*state) {
	case STT_STATE_CREATED:		SLOG(LOG_DEBUG, TAG_STTC, "Current state is 'CREATED'");	break;
	case STT_STATE_READY:		SLOG(LOG_DEBUG, TAG_STTC, "Current state is 'Ready'");		break;
	case STT_STATE_RECORDING:	SLOG(LOG_DEBUG, TAG_STTC, "Current state is 'Recording'");	break;
	case STT_STATE_PROCESSING:	SLOG(LOG_DEBUG, TAG_STTC, "Current state is 'Processing'");	break;
	default:			SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid value");		break;
	}

	return STT_ERROR_NONE;
}

int stt_get_error_message(stt_h stt, char** err_msg)
{
	stt_client_s* client = NULL;
	if (0 != __stt_get_feature_enabled()) {
		return STT_ERROR_NOT_SUPPORTED;
	}
	if (0 != __stt_check_privilege()) {
		return STT_ERROR_PERMISSION_DENIED;
	}
	if (0 != __stt_check_handle(stt, &client)) {
		return STT_ERROR_INVALID_PARAMETER;
	}

	if (NULL == err_msg) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Input parameter is NULL");
		return STT_ERROR_INVALID_PARAMETER;
	}

	if (false == g_err_callback_status) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] This callback should be called during an err_callback");
		return STT_ERROR_OPERATION_FAILED;
	}

	if (NULL != client->err_msg) {
		*err_msg = strdup(client->err_msg);
		SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Error msg (%s)", *err_msg);
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Error msg (NULL)");
	}

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return STT_ERROR_NONE;
}

int stt_is_recognition_type_supported(stt_h stt, const char* type, bool* support)
{
	stt_client_s* client = NULL;
	if (0 != __stt_get_feature_enabled()) {
		return STT_ERROR_NOT_SUPPORTED;
	}
	if (0 != __stt_check_privilege()) {
		return STT_ERROR_PERMISSION_DENIED;
	}
	if (0 != __stt_check_handle(stt, &client)) {
		return STT_ERROR_INVALID_PARAMETER;
	}

	if (NULL == type || NULL == support) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Input parameter is NULL");
		return STT_ERROR_INVALID_PARAMETER;
	}

	/* check state */
	if (client->current_state != STT_STATE_READY) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid State: Current state(%d) is not READY", client->current_state);
		return STT_ERROR_INVALID_STATE;
	}

	int ret = -1;
	int count = 0;
	while (0 != ret) {
		ret = stt_dbus_request_is_recognition_type_supported(client->uid, type, support);
		if (0 != ret) {
			if (STT_ERROR_TIMED_OUT != ret) {
				SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to get recognition type supported : %s", __stt_get_error_code(ret));
				return ret;
			} else {
				SLOG(LOG_WARN, TAG_STTC, "[WARNING] retry");
				usleep(10000);
				count++;
				if (STT_RETRY_COUNT == count) {
					SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to request");
					return ret;
				}
			}
		} else {
			SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] recognition type is %s", *support ? "true " : "false");
			break;
		}
	}

	return STT_ERROR_NONE;
}

int stt_set_silence_detection(stt_h stt, stt_option_silence_detection_e type)
{
	stt_client_s* client = NULL;
	if (0 != __stt_get_feature_enabled()) {
		return STT_ERROR_NOT_SUPPORTED;
	}
	if (0 != __stt_check_privilege()) {
		return STT_ERROR_PERMISSION_DENIED;
	}
	if (0 != __stt_check_handle(stt, &client)) {
		return STT_ERROR_INVALID_PARAMETER;
	}

	/* check state */
	if (client->current_state != STT_STATE_READY) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid State: Current state(%d) is not READY", client->current_state);
		return STT_ERROR_INVALID_STATE;
	}

	if (true == client->silence_supported) {
		if (type >= STT_OPTION_SILENCE_DETECTION_FALSE && type <= STT_OPTION_SILENCE_DETECTION_AUTO) {
			client->silence = type;
		} else {
			SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Type is invalid");
			return STT_ERROR_INVALID_PARAMETER;
		}
	} else {
		return STT_ERROR_NOT_SUPPORTED_FEATURE;
	}

	return STT_ERROR_NONE;
}

int stt_set_start_sound(stt_h stt, const char* filename)
{
	stt_client_s* client = NULL;
	if (0 != __stt_get_feature_enabled()) {
		return STT_ERROR_NOT_SUPPORTED;
	}
	if (0 != __stt_check_privilege()) {
		return STT_ERROR_PERMISSION_DENIED;
	}
	if (0 != __stt_check_handle(stt, &client)) {
		return STT_ERROR_INVALID_PARAMETER;
	}

	SLOG(LOG_DEBUG, TAG_STTC, "===== STT SET START SOUND");

	if (NULL == filename) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Input parameter is NULL");
		return STT_ERROR_INVALID_PARAMETER;
	}

	if (0 != access(filename, F_OK)) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] File does not exist");
		return STT_ERROR_INVALID_PARAMETER;
	}

	/* check state */
	if (client->current_state != STT_STATE_READY) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid State: Current state(%d) is not READY", client->current_state);
		return STT_ERROR_INVALID_STATE;
	}

	int ret = -1;
	int count = 0;
	while (0 != ret) {
		ret = stt_dbus_request_set_start_sound(client->uid, filename);
		if (0 != ret) {
			if (STT_ERROR_TIMED_OUT != ret) {
				SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to set start sound : %s", __stt_get_error_code(ret));
				return ret;
			} else {
				SLOG(LOG_WARN, TAG_STTC, "[WARNING] retry");
				usleep(10000);
				count++;
				if (STT_RETRY_COUNT == count) {
					SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to request");
					return ret;
				}
			}
		} else {
			SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Set start sound : %s", filename);
			break;
		}
	}

	return STT_ERROR_NONE;
}

int stt_unset_start_sound(stt_h stt)
{
	stt_client_s* client = NULL;
	if (0 != __stt_get_feature_enabled()) {
		return STT_ERROR_NOT_SUPPORTED;
	}
	if (0 != __stt_check_privilege()) {
		return STT_ERROR_PERMISSION_DENIED;
	}
	if (0 != __stt_check_handle(stt, &client)) {
		return STT_ERROR_INVALID_PARAMETER;
	}

	SLOG(LOG_DEBUG, TAG_STTC, "===== STT UNSET START SOUND");

	/* check state */
	if (client->current_state != STT_STATE_READY) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid State: Current state(%d) is not READY", client->current_state);
		return STT_ERROR_INVALID_STATE;
	}

	int ret = -1;
	int count = 0;
	while (0 != ret) {
		ret = stt_dbus_request_unset_start_sound(client->uid);
		if (0 != ret) {
			if (STT_ERROR_TIMED_OUT != ret) {
				SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to unset start sound : %s", __stt_get_error_code(ret));
				return ret;
			} else {
				SLOG(LOG_WARN, TAG_STTC, "[WARNING] retry");
				usleep(10000);
				count++;
				if (STT_RETRY_COUNT == count) {
					SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to request");
					return ret;
				}
			}
		} else {
			SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Unset start sound");
			break;
		}
	}

	return STT_ERROR_NONE;
}

int stt_set_stop_sound(stt_h stt, const char* filename)
{
	stt_client_s* client = NULL;
	if (0 != __stt_get_feature_enabled()) {
		return STT_ERROR_NOT_SUPPORTED;
	}
	if (0 != __stt_check_privilege()) {
		return STT_ERROR_PERMISSION_DENIED;
	}
	if (0 != __stt_check_handle(stt, &client)) {
		return STT_ERROR_INVALID_PARAMETER;
	}

	SLOG(LOG_DEBUG, TAG_STTC, "===== STT SET STOP SOUND");

	if (NULL == filename) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Input parameter is NULL");
		return STT_ERROR_INVALID_PARAMETER;
	}

	if (0 != access(filename, F_OK)) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] File does not exist");
		return STT_ERROR_INVALID_PARAMETER;
	}

	/* check state */
	if (client->current_state != STT_STATE_READY) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid State: Current state(%d) is not READY", client->current_state);
		return STT_ERROR_INVALID_STATE;
	}

	int ret = -1;
	int count = 0;
	while (0 != ret) {
		ret = stt_dbus_request_set_stop_sound(client->uid, filename);
		if (0 != ret) {
			if (STT_ERROR_TIMED_OUT != ret) {
				SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to set stop sound : %s", __stt_get_error_code(ret));
				return ret;
			} else {
				SLOG(LOG_WARN, TAG_STTC, "[WARNING] retry");
				usleep(10000);
				count++;
				if (STT_RETRY_COUNT == count) {
					SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to request");
					return ret;
				}
			}
		} else {
			SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Set stop sound : %s", filename);
			break;
		}
	}

	return STT_ERROR_NONE;
}

int stt_unset_stop_sound(stt_h stt)
{
	stt_client_s* client = NULL;
	if (0 != __stt_get_feature_enabled()) {
		return STT_ERROR_NOT_SUPPORTED;
	}
	if (0 != __stt_check_privilege()) {
		return STT_ERROR_PERMISSION_DENIED;
	}
	if (0 != __stt_check_handle(stt, &client)) {
		return STT_ERROR_INVALID_PARAMETER;
	}

	SLOG(LOG_DEBUG, TAG_STTC, "===== STT UNSET STOP SOUND");

	/* check state */
	if (client->current_state != STT_STATE_READY) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid State: Current state(%d) is not READY", client->current_state);
		return STT_ERROR_INVALID_STATE;
	}

	int ret = -1;
	int count = 0;
	while (0 != ret) {
		ret = stt_dbus_request_unset_stop_sound(client->uid);
		if (0 != ret) {
			if (STT_ERROR_TIMED_OUT != ret) {
				SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to unset stop sound : %s", __stt_get_error_code(ret));
				return ret;
			} else {
				SLOG(LOG_WARN, TAG_STTC, "[WARNING] retry");
				usleep(10000);
				count++;
				if (STT_RETRY_COUNT == count) {
					SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to request");
					return ret;
				}
			}
		} else {
			SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Unset stop sound");
			break;
		}
	}

	return STT_ERROR_NONE;
}

int stt_start(stt_h stt, const char* language, const char* type)
{
	stt_client_s* client = NULL;
	if (0 != __stt_get_feature_enabled()) {
		return STT_ERROR_NOT_SUPPORTED;
	}
	if (0 != __stt_check_privilege()) {
		return STT_ERROR_PERMISSION_DENIED;
	}
	if (0 != __stt_check_handle(stt, &client)) {
		return STT_ERROR_INVALID_PARAMETER;
	}

	SLOG(LOG_DEBUG, TAG_STTC, "===== STT START");

	/* check state */
	if (client->current_state != STT_STATE_READY) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid State: Current state(%d) is not READY", client->current_state);
		return STT_ERROR_INVALID_STATE;
	}

	if (STT_INTERNAL_STATE_NONE != client->internal_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid State : Internal state is NOT none : %d", client->internal_state);
		return STT_ERROR_IN_PROGRESS_TO_RECORDING;
	}

	int ret = -1;
	char appid[128] = {0, };
	ret = aul_app_get_appid_bypid(getpid(), appid, sizeof(appid));

	if ((AUL_R_OK != ret) || (0 == strlen(appid))) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to get application ID");
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, "[DEBUG] Current app id is %s", appid);
	}

	char* temp = NULL;
	if (NULL == language) {
		temp = strdup("default");
	} else {
		temp = strdup(language);
	}

	if (true == client->credential_needed && NULL == client->credential) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Do not have app credential for this engine(%s)", client->current_engine_id);
		return STT_ERROR_PERMISSION_DENIED;
	}

	ret = stt_dbus_request_start(client->uid, temp, type, client->silence, appid, client->credential);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to start : %s", __stt_get_error_code(ret));
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Start is successful but not done");
		client->internal_state = STT_INTERNAL_STATE_STARTING;
	}

	if (NULL != temp)	free(temp);

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return ret;
}

int stt_stop(stt_h stt)
{
	stt_client_s* client = NULL;
	if (0 != __stt_get_feature_enabled()) {
		return STT_ERROR_NOT_SUPPORTED;
	}
	if (0 != __stt_check_privilege()) {
		return STT_ERROR_PERMISSION_DENIED;
	}
	if (0 != __stt_check_handle(stt, &client)) {
		return STT_ERROR_INVALID_PARAMETER;
	}

	SLOG(LOG_DEBUG, TAG_STTC, "===== STT STOP");

	/* check state */
	if (client->current_state != STT_STATE_RECORDING) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid State : Current state(%d) is NOT RECORDING", client->current_state);
		return STT_ERROR_INVALID_STATE;
	}

	if (STT_INTERNAL_STATE_NONE != client->internal_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid State : Internal state is NOT none : %d", client->internal_state);
		return STT_ERROR_IN_PROGRESS_TO_PROCESSING;
	}

	int ret = stt_dbus_request_stop(client->uid);

	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to stop : %s", __stt_get_error_code(ret));
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Stop is successful but not done");
		client->internal_state = STT_INTERNAL_STATE_STOPING;
	}

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return ret;
}


int stt_cancel(stt_h stt)
{
	stt_client_s* client = NULL;
	if (0 != __stt_get_feature_enabled()) {
		return STT_ERROR_NOT_SUPPORTED;
	}
	if (0 != __stt_check_privilege()) {
		return STT_ERROR_PERMISSION_DENIED;
	}
	if (0 != __stt_check_handle(stt, &client)) {
		return STT_ERROR_INVALID_PARAMETER;
	}

	SLOG(LOG_DEBUG, TAG_STTC, "===== STT CANCEL");

	/* check state */
	if (STT_STATE_RECORDING != client->current_state && STT_STATE_PROCESSING != client->current_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid state : Current state(%d) is 'Ready'", client->current_state);
		return STT_ERROR_INVALID_STATE;
	}

	if (STT_INTERNAL_STATE_NONE != client->internal_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid State : Internal state is NOT none : %d", client->internal_state);
		return STT_ERROR_IN_PROGRESS_TO_READY;
	}

	int ret = stt_dbus_request_cancel(client->uid);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to cancel : %s", __stt_get_error_code(ret));
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Cancel is successful but not done");
		client->internal_state = STT_INTERNAL_STATE_CANCELING;
	}

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return ret;
}

int __stt_cb_set_volume(int uid, float volume)
{
	stt_client_s* client = NULL;

	client = stt_client_get_by_uid(uid);
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "Handle is NOT valid");
		return STT_ERROR_INVALID_PARAMETER;
	}

	if (STT_STATE_RECORDING != client->current_state) {
		SLOG(LOG_DEBUG, TAG_STTC, "[ERROR] Invalid state : NO 'Recording' state, cur(%d)", client->current_state);
		return STT_ERROR_INVALID_STATE;
	}

	g_volume_db = volume;
	SLOG(LOG_DEBUG, TAG_STTC, "Set volume (%f)", g_volume_db);

	return 0;
}

int stt_get_recording_volume(stt_h stt, float* volume)
{
	stt_client_s* client = NULL;
	if (0 != __stt_get_feature_enabled()) {
		return STT_ERROR_NOT_SUPPORTED;
	}
	if (0 != __stt_check_privilege()) {
		return STT_ERROR_PERMISSION_DENIED;
	}
	if (0 != __stt_check_handle(stt, &client)) {
		return STT_ERROR_INVALID_PARAMETER;
	}

	if (NULL == volume) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Input parameter is NULL");
		return STT_ERROR_INVALID_PARAMETER;
	}

	if (STT_STATE_RECORDING != client->current_state) {
		SLOG(LOG_DEBUG, TAG_STTC, "[ERROR] Invalid state : NO 'Recording' state, cur(%d)", client->current_state);
		return STT_ERROR_INVALID_STATE;
	}

	*volume = g_volume_db;

	return STT_ERROR_NONE;
}

bool __stt_result_time_cb(int index, int event, const char* text, long start_time, long end_time, void* user_data)
{
	stt_client_s* client = (stt_client_s*)user_data;

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to notify error : A handle is not valid");
		return EINA_FALSE;
	}

	if (NULL != client->result_time_cb) {
		SLOG(LOG_DEBUG, TAG_STTC, "(%d) event(%d) text(%s) start(%ld) end(%ld)",
			index, event, text, start_time, end_time);
		client->result_time_cb(client->stt, index, (stt_result_time_event_e)event,
			text, start_time, end_time, client->result_time_user_data);
	} else {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Callback is NULL");
		return false;
	}

	return true;
}

int stt_foreach_detailed_result(stt_h stt, stt_result_time_cb callback, void* user_data)
{
	stt_client_s* client = NULL;
	if (0 != __stt_get_feature_enabled()) {
		return STT_ERROR_NOT_SUPPORTED;
	}
	if (0 != __stt_check_privilege()) {
		return STT_ERROR_PERMISSION_DENIED;
	}
	if (0 != __stt_check_handle(stt, &client)) {
		return STT_ERROR_INVALID_PARAMETER;
	}

	SLOG(LOG_DEBUG, TAG_STTC, "===== STT FOREACH DETAILED RESULT");

	if (NULL == callback) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Input parameter is NULL");
		return STT_ERROR_INVALID_PARAMETER;
	}

	client->result_time_cb = callback;
	client->result_time_user_data = user_data;

	int ret = -1;
	ret = stt_config_mgr_foreach_time_info(__stt_result_time_cb, client);
	ret = __stt_convert_config_error_code(ret);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to foreach time info : %s", __stt_get_error_code(ret));
	}

	client->result_time_cb = NULL;
	client->result_time_user_data = NULL;

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return ret;
}

static Eina_Bool __stt_notify_error(void *data)
{
	stt_client_s* client = (stt_client_s*)data;

	SLOG(LOG_WARN, TAG_STTC, "[WARNING] Error from sttd");

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to notify error : A handle is not valid");
		return EINA_FALSE;
	}

	if (NULL == stt_client_get_by_uid(client->uid))
		return EINA_FALSE;

	if (NULL != client->error_cb) {
		stt_client_use_callback(client);
		g_err_callback_status = true;
		client->error_cb(client->stt, client->reason, client->error_user_data);
		g_err_callback_status = false;
		stt_client_not_use_callback(client);
		SLOG(LOG_WARN, TAG_STTC, "[WARNING] Error callback is called : reason [%d]", client->reason);
	} else {
		SLOG(LOG_WARN, TAG_STTC, "[WARNING] Error callback is null");
	}

	return EINA_FALSE;
}

int __stt_cb_error(int uid, int reason, char* err_msg)
{
	stt_client_s* client = stt_client_get_by_uid(uid);
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "Handle not found");
		return -1;
	}

	client->reason = reason;
	client->internal_state = STT_INTERNAL_STATE_NONE;
	if (NULL != client->err_msg) {
		free(client->err_msg);
		client->err_msg = NULL;
	}
	client->err_msg = strdup(err_msg);

	SLOG(LOG_INFO, TAG_STTC, "internal state is initialized to 0");

	if (NULL != client->error_cb) {
		ecore_timer_add(0, __stt_notify_error, client);
	} else {
		SLOG(LOG_WARN, TAG_STTC, "[WARNING] Error callback is null");
	}

	return 0;
}

static void __stt_notify_state_changed(void *data)
{
	stt_client_s* client = (stt_client_s*)data;

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to notify error : A handle is not valid");
		return;
	}

	if (NULL == stt_client_get_by_uid(client->uid)) {
		return;
	}

	if (STT_INTERNAL_STATE_STARTING == client->internal_state && STT_STATE_RECORDING == client->current_state) {
		client->internal_state = STT_INTERNAL_STATE_NONE;
		SLOG(LOG_DEBUG, TAG_STTC, "Internal state change to NONE");
	} else if (STT_INTERNAL_STATE_STOPING == client->internal_state && STT_STATE_PROCESSING == client->current_state) {
		client->internal_state = STT_INTERNAL_STATE_NONE;
		SLOG(LOG_DEBUG, TAG_STTC, "Internal state change to NONE");
	} else if (STT_INTERNAL_STATE_CANCELING == client->internal_state && STT_STATE_READY == client->current_state) {
		client->internal_state = STT_INTERNAL_STATE_NONE;
		SLOG(LOG_DEBUG, TAG_STTC, "Internal state change to NONE");
	}

	if (NULL != client->state_changed_cb) {
		stt_client_use_callback(client);
		client->state_changed_cb(client->stt, client->before_state,
			client->current_state, client->state_changed_user_data);
		stt_client_not_use_callback(client);
		SLOG(LOG_DEBUG, TAG_STTC, "State changed callback is called");
	} else {
		SLOG(LOG_WARN, TAG_STTC, "[WARNING] State changed callback is null");
	}

	return;
}

static Eina_Bool __stt_notify_result(void *data)
{
	stt_client_s* client = (stt_client_s*)data;

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to notify error : A handle is not valid");
		return EINA_FALSE;
	}

	if (NULL == stt_client_get_by_uid(client->uid))	{
		return EINA_FALSE;
	}

	if (NULL != client->recognition_result_cb) {
		stt_client_use_callback(client);
		client->recognition_result_cb(client->stt, client->event, (const char**)client->data_list, client->data_count,
			client->msg, client->recognition_result_user_data);
		stt_client_not_use_callback(client);
		SLOG(LOG_DEBUG, TAG_STTC, "client recognition result callback called");
	} else {
		SLOG(LOG_WARN, TAG_STTC, "[WARNING] User recognition result callback is NULL");
	}

	if (NULL != client->msg) {
		free(client->msg);
		client->msg = NULL;
	}

	if (NULL != client->data_list) {
		char **temp = NULL;
		temp = client->data_list;

		int i = 0;
		for (i = 0; i < client->data_count; i++) {
			if (NULL != temp[i]) {
				free(temp[i]);
				temp[i] = NULL;
			} else {
				SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Result data is error");
			}
		}
		free(client->data_list);
		client->data_list = NULL;
	}

	client->data_count = 0;

	stt_config_mgr_remove_time_info_file();

	if (STT_RESULT_EVENT_FINAL_RESULT == client->event || STT_RESULT_EVENT_ERROR == client->event) {
		client->before_state = client->current_state;
		client->current_state = STT_STATE_READY;

		if (NULL != client->state_changed_cb) {
			ecore_main_loop_thread_safe_call_async(__stt_notify_state_changed, client);
		} else {
			SLOG(LOG_WARN, TAG_STTC, "[WARNING] State changed callback is null");
		}
	}

	return EINA_FALSE;
}

int __stt_cb_result(int uid, int event, char** data, int data_count, const char* msg)
{
	stt_client_s* client = NULL;

	client = stt_client_get_by_uid(uid);
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "Handle is NOT valid");
		return STT_ERROR_INVALID_PARAMETER;
	}

	if (NULL != msg)
		SLOG(LOG_DEBUG, TAG_STTC, "Recognition Result Message = %s", msg);

	int i = 0;
	for (i = 0; i < data_count; i++) {
		if (NULL != data[i])
			SLOG(LOG_DEBUG, TAG_STTC, "Recognition Result[%d] = %s", i, data[i]);
	}

	if (NULL != client->recognition_result_cb) {
		client->event = event;
		if (NULL != msg) {
			client->msg = strdup(msg);
		}

		client->data_count = data_count;

		if (data_count > 0) {
			char **temp = NULL;
			temp = (char**)calloc(data_count, sizeof(char*));
			if (NULL == temp) {
				SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to allocate memory");
				return STT_ERROR_OUT_OF_MEMORY;
			}

			for (i = 0; i < data_count; i++) {
				if (NULL != data[i])
					temp[i] = strdup(data[i]);
				else
					SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Result data is error");
			}

			client->data_list = temp;
		}
	} else {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] User result callback is null");
	}

	ecore_timer_add(0, __stt_notify_result, client);

	return STT_ERROR_NONE;
}

int __stt_cb_set_state(int uid, int state)
{
	stt_client_s* client = stt_client_get_by_uid(uid);
	if (NULL == client) {
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

	ecore_main_loop_thread_safe_call_async(__stt_notify_state_changed, client);
	return 0;
}

int stt_set_recognition_result_cb(stt_h stt, stt_recognition_result_cb callback, void* user_data)
{
	stt_client_s* client = NULL;
	if (0 != __stt_get_feature_enabled()) {
		return STT_ERROR_NOT_SUPPORTED;
	}
	if (0 != __stt_check_privilege()) {
		return STT_ERROR_PERMISSION_DENIED;
	}
	if (0 != __stt_check_handle(stt, &client)) {
		return STT_ERROR_INVALID_PARAMETER;
	}

	if (callback == NULL)
		return STT_ERROR_INVALID_PARAMETER;

	if (STT_STATE_CREATED != client->current_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Current state(%d) is not 'Created'", client->current_state);
		return STT_ERROR_INVALID_STATE;
	}

	client->recognition_result_cb = callback;
	client->recognition_result_user_data = user_data;

	return 0;
}

int stt_unset_recognition_result_cb(stt_h stt)
{
	stt_client_s* client = NULL;
	if (0 != __stt_get_feature_enabled()) {
		return STT_ERROR_NOT_SUPPORTED;
	}
	if (0 != __stt_check_privilege()) {
		return STT_ERROR_PERMISSION_DENIED;
	}
	if (0 != __stt_check_handle(stt, &client)) {
		return STT_ERROR_INVALID_PARAMETER;
	}

	if (STT_STATE_CREATED != client->current_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Current state(%d) is not 'Created'", client->current_state);
		return STT_ERROR_INVALID_STATE;
	}

	client->recognition_result_cb = NULL;
	client->recognition_result_user_data = NULL;

	return 0;
}

int stt_set_state_changed_cb(stt_h stt, stt_state_changed_cb callback, void* user_data)
{
	stt_client_s* client = NULL;
	if (0 != __stt_get_feature_enabled()) {
		return STT_ERROR_NOT_SUPPORTED;
	}
	if (0 != __stt_check_privilege()) {
		return STT_ERROR_PERMISSION_DENIED;
	}
	if (0 != __stt_check_handle(stt, &client)) {
		return STT_ERROR_INVALID_PARAMETER;
	}

	if (NULL == callback)
		return STT_ERROR_INVALID_PARAMETER;

	if (STT_STATE_CREATED != client->current_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Current state(%d) is not 'Created'", client->current_state);
		return STT_ERROR_INVALID_STATE;
	}

	client->state_changed_cb = callback;
	client->state_changed_user_data = user_data;

	return 0;
}

int stt_unset_state_changed_cb(stt_h stt)
{
	stt_client_s* client = NULL;
	if (0 != __stt_get_feature_enabled()) {
		return STT_ERROR_NOT_SUPPORTED;
	}
	if (0 != __stt_check_privilege()) {
		return STT_ERROR_PERMISSION_DENIED;
	}
	if (0 != __stt_check_handle(stt, &client)) {
		return STT_ERROR_INVALID_PARAMETER;
	}

	if (STT_STATE_CREATED != client->current_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Current state(%d) is not 'Created'", client->current_state);
		return STT_ERROR_INVALID_STATE;
	}

	client->state_changed_cb = NULL;
	client->state_changed_user_data = NULL;

	return 0;
}

int stt_set_error_cb(stt_h stt, stt_error_cb callback, void* user_data)
{
	stt_client_s* client = NULL;
	if (0 != __stt_get_feature_enabled()) {
		return STT_ERROR_NOT_SUPPORTED;
	}
	if (0 != __stt_check_privilege()) {
		return STT_ERROR_PERMISSION_DENIED;
	}
	if (0 != __stt_check_handle(stt, &client)) {
		return STT_ERROR_INVALID_PARAMETER;
	}

	if (NULL == callback)
		return STT_ERROR_INVALID_PARAMETER;

	if (STT_STATE_CREATED != client->current_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Current state(%d) is not 'Created'", client->current_state);
		return STT_ERROR_INVALID_STATE;
	}

	client->error_cb = callback;
	client->error_user_data = user_data;

	return 0;
}

int stt_unset_error_cb(stt_h stt)
{
	stt_client_s* client = NULL;
	if (0 != __stt_get_feature_enabled()) {
		return STT_ERROR_NOT_SUPPORTED;
	}
	if (0 != __stt_check_privilege()) {
		return STT_ERROR_PERMISSION_DENIED;
	}
	if (0 != __stt_check_handle(stt, &client)) {
		return STT_ERROR_INVALID_PARAMETER;
	}

	if (STT_STATE_CREATED != client->current_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Current state(%d) is not 'Created'", client->current_state);
		return STT_ERROR_INVALID_STATE;
	}

	client->error_cb = NULL;
	client->error_user_data = NULL;

	return 0;
}

int stt_set_default_language_changed_cb(stt_h stt, stt_default_language_changed_cb callback, void* user_data)
{
	stt_client_s* client = NULL;
	if (0 != __stt_get_feature_enabled()) {
		return STT_ERROR_NOT_SUPPORTED;
	}
	if (0 != __stt_check_privilege()) {
		return STT_ERROR_PERMISSION_DENIED;
	}
	if (0 != __stt_check_handle(stt, &client)) {
		return STT_ERROR_INVALID_PARAMETER;
	}

	if (NULL == callback)
		return STT_ERROR_INVALID_PARAMETER;

	if (STT_STATE_CREATED != client->current_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Current state(%d) is not 'Created'", client->current_state);
		return STT_ERROR_INVALID_STATE;
	}

	client->default_lang_changed_cb = callback;
	client->default_lang_changed_user_data = user_data;

	return 0;
}

int stt_unset_default_language_changed_cb(stt_h stt)
{
	stt_client_s* client = NULL;
	if (0 != __stt_get_feature_enabled()) {
		return STT_ERROR_NOT_SUPPORTED;
	}
	if (0 != __stt_check_privilege()) {
		return STT_ERROR_PERMISSION_DENIED;
	}
	if (0 != __stt_check_handle(stt, &client)) {
		return STT_ERROR_INVALID_PARAMETER;
	}

	if (STT_STATE_CREATED != client->current_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Current state(%d) is not 'Created'", client->current_state);
		return STT_ERROR_INVALID_STATE;
	}

	client->default_lang_changed_cb = NULL;
	client->default_lang_changed_user_data = NULL;

	return 0;
}

int stt_set_engine_changed_cb(stt_h stt, stt_engine_changed_cb callback, void* user_data)
{
	stt_client_s* client = NULL;
	if (0 != __stt_get_feature_enabled()) {
		return STT_ERROR_NOT_SUPPORTED;
	}
	if (0 != __stt_check_privilege()) {
		return STT_ERROR_PERMISSION_DENIED;
	}
	if (0 != __stt_check_handle(stt, &client)) {
		return STT_ERROR_INVALID_PARAMETER;
	}

	if (NULL == callback)
		return STT_ERROR_INVALID_PARAMETER;

	if (STT_STATE_CREATED != client->current_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Current state(%d) is not 'Created'", client->current_state);
		return STT_ERROR_INVALID_STATE;
	}

	client->engine_changed_cb = callback;
	client->engine_changed_user_data = user_data;

	return 0;
}

int stt_unset_engine_changed_cb(stt_h stt)
{
	stt_client_s* client = NULL;
	if (0 != __stt_get_feature_enabled()) {
		return STT_ERROR_NOT_SUPPORTED;
	}
	if (0 != __stt_check_privilege()) {
		return STT_ERROR_PERMISSION_DENIED;
	}
	if (0 != __stt_check_handle(stt, &client)) {
		return STT_ERROR_INVALID_PARAMETER;
	}

	if (STT_STATE_CREATED != client->current_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Current state(%d) is not 'Created'", client->current_state);
		return STT_ERROR_INVALID_STATE;
	}

	client->engine_changed_cb = NULL;
	client->engine_changed_user_data = NULL;

	return 0;
}
