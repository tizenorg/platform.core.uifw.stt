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


static bool g_is_daemon_started = false;

static Ecore_Timer* g_connect_timer = NULL;

static Eina_Bool __stt_notify_state_changed(void *data);
static Eina_Bool __stt_notify_error(void *data);

static int g_stt_daemon_pid = -1;
static int g_count_check_daemon = 0;

const char* stt_tag()
{
	return "sttc";
}

static const char* __stt_get_error_code(stt_error_e err)
{
	switch(err) {
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
	SECURE_SLOG(LOG_DEBUG, TAG_STTC, "Language changed : Before lang(%s) Current lang(%s)", 
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
				SECURE_SLOG(LOG_DEBUG, TAG_STTC, "Call default language changed callback : uid(%d)", data->uid);
				data->default_lang_changed_cb(data->stt, before_language, current_language, 
					data->default_lang_changed_user_data);
			}

			/* Next item */
			iter = g_list_next(iter);
		}
	}

	return; 
}

int stt_create(stt_h* stt)
{
	bool stt_supported = false;
	bool mic_supported = false;
	if (0 == system_info_get_platform_bool(STT_FEATURE_PATH, &stt_supported)) {
		if (0 == system_info_get_platform_bool(STT_MIC_FEATURE_PATH, &mic_supported)) {
			if (false == stt_supported || false == mic_supported) {
				SLOG(LOG_ERROR, TAG_STTC, "[ERROR] STT NOT supported");
				return STT_ERROR_NOT_SUPPORTED;
			}
		}
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

	ret = stt_config_mgr_set_callback(client->uid, NULL, __stt_config_lang_changed_cb, NULL, NULL);
	ret = __stt_convert_config_error_code(ret);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to set config changed : %s", __stt_get_error_code(ret));
		stt_client_destroy(*stt);
		return ret;
	}

	SECURE_SLOG(LOG_DEBUG, TAG_STTC, "[Success] uid(%d)", (*stt)->handle);

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return STT_ERROR_NONE;
}

int stt_destroy(stt_h stt)
{
	bool stt_supported = false;
	bool mic_supported = false;
	if (0 == system_info_get_platform_bool(STT_FEATURE_PATH, &stt_supported)) {
		if (0 == system_info_get_platform_bool(STT_MIC_FEATURE_PATH, &mic_supported)) {
			if (false == stt_supported || false == mic_supported) {
				SLOG(LOG_ERROR, TAG_STTC, "[ERROR] STT NOT supported");
				return STT_ERROR_NOT_SUPPORTED;
			}
		}
	}

	SLOG(LOG_DEBUG, TAG_STTC, "===== Destroy STT");

	if (NULL == stt) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Input handle is null");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_ERROR_INVALID_PARAMETER;
	}
	
	stt_client_s* client = stt_client_get(stt);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] A handle is not available");
		return STT_ERROR_INVALID_PARAMETER;
	}

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

		g_is_daemon_started = false;
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
	bool stt_supported = false;
	bool mic_supported = false;
	if (0 == system_info_get_platform_bool(STT_FEATURE_PATH, &stt_supported)) {
		if (0 == system_info_get_platform_bool(STT_MIC_FEATURE_PATH, &mic_supported)) {
			if (false == stt_supported || false == mic_supported) {
				SLOG(LOG_ERROR, TAG_STTC, "[ERROR] STT NOT supported");
				return STT_ERROR_NOT_SUPPORTED;
			}
		}
	}

	SLOG(LOG_DEBUG, TAG_STTC, "===== Foreach Supported engine");

	if (NULL == stt || NULL == callback) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Input parameter is NULL");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_ERROR_INVALID_PARAMETER;
	}

	stt_client_s* client = stt_client_get(stt);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_ERROR_INVALID_PARAMETER;
	}

	if (client->current_state != STT_STATE_CREATED) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid State: Current state is not CREATE");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
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
	bool stt_supported = false;
	bool mic_supported = false;
	if (0 == system_info_get_platform_bool(STT_FEATURE_PATH, &stt_supported)) {
		if (0 == system_info_get_platform_bool(STT_MIC_FEATURE_PATH, &mic_supported)) {
			if (false == stt_supported || false == mic_supported) {
				SLOG(LOG_ERROR, TAG_STTC, "[ERROR] STT NOT supported");
				return STT_ERROR_NOT_SUPPORTED;
			}
		}
	}

	SLOG(LOG_DEBUG, TAG_STTC, "===== Get current engine");

	if (NULL == stt || NULL == engine_id) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Input parameter is NULL");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_ERROR_INVALID_PARAMETER;
	}

	stt_client_s* client = stt_client_get(stt);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_ERROR_INVALID_PARAMETER;
	}

	if (client->current_state != STT_STATE_CREATED) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid State: Current state is not CREATE");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_ERROR_INVALID_STATE;
	}

	int ret = 0;
	ret = stt_config_mgr_get_engine(engine_id);
	ret = __stt_convert_config_error_code(ret);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to request get current engine : %s", __stt_get_error_code(ret));
	} else {
		SECURE_SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Current engine uuid = %s", *engine_id);
	}

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return ret;
}

int stt_set_engine(stt_h stt, const char* engine_id)
{
	bool stt_supported = false;
	bool mic_supported = false;
	if (0 == system_info_get_platform_bool(STT_FEATURE_PATH, &stt_supported)) {
		if (0 == system_info_get_platform_bool(STT_MIC_FEATURE_PATH, &mic_supported)) {
			if (false == stt_supported || false == mic_supported) {
				SLOG(LOG_ERROR, TAG_STTC, "[ERROR] STT NOT supported");
				return STT_ERROR_NOT_SUPPORTED;
			}
		}
	}

	SLOG(LOG_DEBUG, TAG_STTC, "===== Set current engine");

	if (NULL == stt || NULL == engine_id) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Input parameter is NULL");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_ERROR_INVALID_PARAMETER;
	}

	stt_client_s* client = stt_client_get(stt);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_ERROR_INVALID_PARAMETER;
	}

	/* check state */
	if (client->current_state != STT_STATE_CREATED) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid State: Current state is not CREATE");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_ERROR_INVALID_STATE;
	}

	if (NULL != client->current_engine_id) {
		free(client->current_engine_id);
	}

	client->current_engine_id = strdup(engine_id);
#if 0
	if (client->current_state == STT_STATE_READY) {
		int ret = 0;
		bool silence_supported = false;

		ret = stt_dbus_request_set_current_engine(client->uid, engine_id, &silence_supported);

		if (0 != ret) {
			SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to set current engine : %s", __stt_get_error_code(ret));
			return ret;
		} else {
			SECURE_SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Current engine uuid = %s", engine_id);

			/* success to change engine */
			client->silence_supported = silence_supported;
			SECURE_SLOG(LOG_DEBUG, TAG_STTC, "Supported options : silence(%s)", silence_supported ? "true" : "false");
		}
	}
#endif
	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return 0;
}

static int __read_proc(const char *path, char *buf, int size)
{
	int fd;
	int ret;

	if (NULL == buf || NULL == path) {
		return -1;
	}

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		return -1;
	}

	ret = read(fd, buf, size - 1);
	if (0 >= ret) {
		close(fd);
		return -1;
	} else {
		buf[ret] = 0;
	}
	close(fd);
	return ret;
}

static bool __stt_check_daemon_exist()
{
	char buf[128];
	int ret;

	FILE* fp;
	fp = fopen(STT_PID_FILE_PATH, "r");
	if (NULL == fp) {
		return false;
	}

	g_stt_daemon_pid = -1;
	int pid;
	if (0 >= fscanf(fp, "%d", &pid)) {
		SECURE_SLOG(LOG_DEBUG, TAG_STTC, "Fail to read pid");
		fclose(fp);
		return false;
	}

	fclose(fp);
	snprintf(buf, sizeof(buf), "/proc/%d/cmdline", pid);
	ret = __read_proc(buf, buf, sizeof(buf));
	if (0 >= ret) {
		return false;
	} else {
		if (!strcmp(buf, "/usr/bin/stt-daemon")) {
			SECURE_SLOG(LOG_DEBUG, TAG_STTC, "Daemon existed - [%d]%s", pid, buf);
			g_stt_daemon_pid = pid;
			return true;
		}
	}
	return false;
}

static void* __fork_stt_daemon(void* NotUsed)
{
	int pid, i;
	pid = fork();

	switch(pid) {
	case -1:
		SLOG(LOG_DEBUG, TAG_STTC, "[STT ERROR] fail to create STT-DAEMON");
		break;

	case 0:
		setsid();
		for (i = 0;i < _NSIG;i++)
			signal(i, SIG_DFL);

		execl("/usr/bin/stt-daemon", "/usr/bin/stt-daemon", NULL);
		break;

	default:
		break;
	}

	return (void*) 1;
}

static Eina_Bool __stt_connect_daemon(void *data)
{
	stt_client_s* client = (stt_client_s*)data;

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] A handle is not available");
		return EINA_FALSE;
	}

	/* Send hello */
	int ret = -1;
	ret = stt_dbus_request_hello();

	if (STT_DAEMON_NORMAL != ret) {
		if (STT_ERROR_INVALID_STATE == ret) {
			return EINA_FALSE;
		}

		if (STT_DAEMON_ON_TERMINATING == ret) {
			/* Todo - Wait for terminating and do it again*/
			usleep(50);
			return EINA_TRUE;
		} else {
			/* for new daemon */
			bool check = __stt_check_daemon_exist();
			if (true == check) {
				g_count_check_daemon++;
				if (3 < g_count_check_daemon) {
					/* Todo - Kill daemon and restart */
					SECURE_SLOG(LOG_DEBUG, TAG_STTC, "Need to Kill daemon");
				}
				usleep(50);
				return EINA_TRUE;
			} else {
				if (false == g_is_daemon_started) {
					g_is_daemon_started = true;
					
					pthread_t thread;
					int thread_id;
					thread_id = pthread_create(&thread, NULL, __fork_stt_daemon, NULL);
					if (thread_id < 0) {
						SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to make thread");
						g_connect_timer = NULL;
						return EINA_FALSE;
					}

					pthread_detach(thread);
				}

				usleep(50);
				return EINA_TRUE;
			}
		}
	}

	g_connect_timer = NULL;
	SLOG(LOG_DEBUG, TAG_STTC, "===== Connect daemon");

	/* request initialization */

	bool silence_supported = false;

	ret = stt_dbus_request_initialize(client->uid, &silence_supported);

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
		SECURE_SLOG(LOG_DEBUG, TAG_STTC, "Supported options : silence(%s)", silence_supported ? "true" : "false");
	}

	if (NULL != client->current_engine_id) {
		ret = -1;
		int count = 0;
		silence_supported = false;
		while (0 != ret) {
			ret = stt_dbus_request_set_current_engine(client->uid, client->current_engine_id, &silence_supported);
			if (0 != ret) {
				if (STT_ERROR_TIMED_OUT != ret) {
					SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to set current engine : %s", __stt_get_error_code(ret));
					return ret;
				} else {
					SLOG(LOG_WARN, TAG_STTC, "[WARNING] retry");
					usleep(10);
					count++;
					if (10 == count) {
						SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to request");
						return ret;
					}
				}
			} else {
				SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Current engine uuid = %s", client->current_engine_id);

				/* success to change engine */
				client->silence_supported = silence_supported;
				SLOG(LOG_DEBUG, TAG_STTC, "Supported options : silence(%s)", silence_supported ? "true" : "false");
			}
		}
	}

	SECURE_SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] uid(%d)", client->uid);

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
	bool stt_supported = false;
	bool mic_supported = false;
	if (0 == system_info_get_platform_bool(STT_FEATURE_PATH, &stt_supported)) {
		if (0 == system_info_get_platform_bool(STT_MIC_FEATURE_PATH, &mic_supported)) {
			if (false == stt_supported || false == mic_supported) {
				SLOG(LOG_ERROR, TAG_STTC, "[ERROR] STT NOT supported");
				return STT_ERROR_NOT_SUPPORTED;
			}
		}
	}

	SLOG(LOG_DEBUG, TAG_STTC, "===== Prepare STT");

	stt_client_s* client = stt_client_get(stt);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_ERROR_INVALID_PARAMETER;
	}

	/* check state */
	if (client->current_state != STT_STATE_CREATED) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid State: Current state is not 'CREATED'");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_ERROR_INVALID_STATE;
	}

	g_count_check_daemon = 0;
	g_connect_timer = ecore_timer_add(0, __stt_connect_daemon, (void*)client);

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return STT_ERROR_NONE;
}

int stt_unprepare(stt_h stt)
{
	bool supported = false;
	if (0 == system_info_get_platform_bool(STT_FEATURE_PATH, &supported)) {
		if (false == supported) {
			SLOG(LOG_ERROR, TAG_STTC, "[ERROR] STT NOT supported");
			return STT_ERROR_NOT_SUPPORTED;
		}
	}

	SLOG(LOG_DEBUG, TAG_STTC, "===== Unprepare STT");

	stt_client_s* client = stt_client_get(stt);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] A handle is not available");
		return STT_ERROR_INVALID_PARAMETER;
	}

	/* check state */
	if (client->current_state != STT_STATE_READY) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid State: Current state is not 'READY'");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
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
				usleep(10);
				count++;
				if (10 == count) {
					SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to request");
					break;
				}
			}
		}
	}

	g_is_daemon_started = false;

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
	bool stt_supported = false;
	bool mic_supported = false;
	if (0 == system_info_get_platform_bool(STT_FEATURE_PATH, &stt_supported)) {
		if (0 == system_info_get_platform_bool(STT_MIC_FEATURE_PATH, &mic_supported)) {
			if (false == stt_supported || false == mic_supported) {
				SLOG(LOG_ERROR, TAG_STTC, "[ERROR] STT NOT supported");
				return STT_ERROR_NOT_SUPPORTED;
			}
		}
	}

	SLOG(LOG_DEBUG, TAG_STTC, "===== Foreach Supported Language");

	if (NULL == stt || NULL == callback) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Input parameter is NULL");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_ERROR_INVALID_PARAMETER;
	}

	stt_client_s* client = stt_client_get(stt);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
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
	bool stt_supported = false;
	bool mic_supported = false;
	if (0 == system_info_get_platform_bool(STT_FEATURE_PATH, &stt_supported)) {
		if (0 == system_info_get_platform_bool(STT_MIC_FEATURE_PATH, &mic_supported)) {
			if (false == stt_supported || false == mic_supported) {
				SLOG(LOG_ERROR, TAG_STTC, "[ERROR] STT NOT supported");
				return STT_ERROR_NOT_SUPPORTED;
			}
		}
	}

	SLOG(LOG_DEBUG, TAG_STTC, "===== Get Default Language");

	if (NULL == stt || NULL == language) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Input parameter is NULL");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_ERROR_INVALID_PARAMETER;
	}

	stt_client_s* client = stt_client_get(stt);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
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
	bool stt_supported = false;
	bool mic_supported = false;
	if (0 == system_info_get_platform_bool(STT_FEATURE_PATH, &stt_supported)) {
		if (0 == system_info_get_platform_bool(STT_MIC_FEATURE_PATH, &mic_supported)) {
			if (false == stt_supported || false == mic_supported) {
				SLOG(LOG_ERROR, TAG_STTC, "[ERROR] STT NOT supported");
				return STT_ERROR_NOT_SUPPORTED;
			}
		}
	}

	if (NULL == stt || NULL == state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Input parameter is NULL");
		return STT_ERROR_INVALID_PARAMETER;
	}

	stt_client_s* client = stt_client_get(stt);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Get state : A handle is not valid");
		return STT_ERROR_INVALID_PARAMETER;
	}

	*state = client->current_state;

	switch(*state) {
		case STT_STATE_CREATED:		SLOG(LOG_DEBUG, TAG_STTC, "Current state is 'CREATED'");	break;
		case STT_STATE_READY:		SLOG(LOG_DEBUG, TAG_STTC, "Current state is 'Ready'");		break;
		case STT_STATE_RECORDING:	SLOG(LOG_DEBUG, TAG_STTC, "Current state is 'Recording'");	break;
		case STT_STATE_PROCESSING:	SLOG(LOG_DEBUG, TAG_STTC, "Current state is 'Processing'");	break;
		default:			SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid value");		break;
	}

	return STT_ERROR_NONE;
}

int stt_is_recognition_type_supported(stt_h stt, const char* type, bool* support)
{
	bool stt_supported = false;
	bool mic_supported = false;
	if (0 == system_info_get_platform_bool(STT_FEATURE_PATH, &stt_supported)) {
		if (0 == system_info_get_platform_bool(STT_MIC_FEATURE_PATH, &mic_supported)) {
			if (false == stt_supported || false == mic_supported) {
				SLOG(LOG_ERROR, TAG_STTC, "[ERROR] STT NOT supported");
				return STT_ERROR_NOT_SUPPORTED;
			}
		}
	}

	if (NULL == stt || NULL == type || NULL == support) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Input parameter is NULL");
		return STT_ERROR_INVALID_PARAMETER;
	}

	stt_client_s* client = stt_client_get(stt);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] A handle is not valid");
		return STT_ERROR_INVALID_PARAMETER;
	}

	/* check state */
	if (client->current_state != STT_STATE_READY) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid State: Current state is not READY");
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
				usleep(10);
				count++;
				if (10 == count) {
					SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to request");
					return ret;
				}
			}
		} else {
			SECURE_SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] recognition type is %s", *support ? "true " : "false");
			break;
		}
	}

	return STT_ERROR_NONE;
}

int stt_set_silence_detection(stt_h stt, stt_option_silence_detection_e type)
{
	bool stt_supported = false;
	bool mic_supported = false;
	if (0 == system_info_get_platform_bool(STT_FEATURE_PATH, &stt_supported)) {
		if (0 == system_info_get_platform_bool(STT_MIC_FEATURE_PATH, &mic_supported)) {
			if (false == stt_supported || false == mic_supported) {
				SLOG(LOG_ERROR, TAG_STTC, "[ERROR] STT NOT supported");
				return STT_ERROR_NOT_SUPPORTED;
			}
		}
	}

	if (NULL == stt) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Input parameter is NULL");
		return STT_ERROR_INVALID_PARAMETER;
	}

	stt_client_s* client = stt_client_get(stt);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Get state : A handle is not valid");
		return STT_ERROR_INVALID_PARAMETER;
	}

	/* check state */
	if (client->current_state != STT_STATE_READY) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid State: Current state is not READY");
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
	bool stt_supported = false;
	bool mic_supported = false;
	if (0 == system_info_get_platform_bool(STT_FEATURE_PATH, &stt_supported)) {
		if (0 == system_info_get_platform_bool(STT_MIC_FEATURE_PATH, &mic_supported)) {
			if (false == stt_supported || false == mic_supported) {
				SLOG(LOG_ERROR, TAG_STTC, "[ERROR] STT NOT supported");
				return STT_ERROR_NOT_SUPPORTED;
			}
		}
	}

	SLOG(LOG_DEBUG, TAG_STTC, "===== STT SET START SOUND");

	if (NULL == stt || NULL == filename) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Input parameter is NULL");
		return STT_ERROR_INVALID_PARAMETER;
	}

	stt_client_s* client = stt_client_get(stt);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Get state : A handle is not valid");
		return STT_ERROR_INVALID_PARAMETER;
	}

	/* check state */
	if (client->current_state != STT_STATE_READY) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid State: Current state is not READY");
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
				usleep(10);
				count++;
				if (10 == count) {
					SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to request");
					return ret;
				}
			}
		} else {
			SECURE_SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Set start sound : %s", filename);
			break;
		}
	}

	return STT_ERROR_NONE;
}

int stt_unset_start_sound(stt_h stt)
{
	bool stt_supported = false;
	bool mic_supported = false;
	if (0 == system_info_get_platform_bool(STT_FEATURE_PATH, &stt_supported)) {
		if (0 == system_info_get_platform_bool(STT_MIC_FEATURE_PATH, &mic_supported)) {
			if (false == stt_supported || false == mic_supported) {
				SLOG(LOG_ERROR, TAG_STTC, "[ERROR] STT NOT supported");
				return STT_ERROR_NOT_SUPPORTED;
			}
		}
	}

	SLOG(LOG_DEBUG, TAG_STTC, "===== STT UNSET START SOUND");

	if (NULL == stt) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Input parameter is NULL");
		return STT_ERROR_INVALID_PARAMETER;
	}

	stt_client_s* client = stt_client_get(stt);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Get state : A handle is not valid");
		return STT_ERROR_INVALID_PARAMETER;
	}

	/* check state */
	if (client->current_state != STT_STATE_READY) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid State: Current state is not READY");
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
				usleep(10);
				count++;
				if (10 == count) {
					SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to request");
					return ret;
				}
			}
		} else {
			SECURE_SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Unset start sound");
			break;
		}
	}

	return STT_ERROR_NONE;
}

int stt_set_stop_sound(stt_h stt, const char* filename)
{
	bool stt_supported = false;
	bool mic_supported = false;
	if (0 == system_info_get_platform_bool(STT_FEATURE_PATH, &stt_supported)) {
		if (0 == system_info_get_platform_bool(STT_MIC_FEATURE_PATH, &mic_supported)) {
			if (false == stt_supported || false == mic_supported) {
				SLOG(LOG_ERROR, TAG_STTC, "[ERROR] STT NOT supported");
				return STT_ERROR_NOT_SUPPORTED;
			}
		}
	}

	SLOG(LOG_DEBUG, TAG_STTC, "===== STT SET STOP SOUND");

	if (NULL == stt || NULL == filename) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Input parameter is NULL");
		return STT_ERROR_INVALID_PARAMETER;
	}

	stt_client_s* client = stt_client_get(stt);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Get state : A handle is not valid");
		return STT_ERROR_INVALID_PARAMETER;
	}

	/* check state */
	if (client->current_state != STT_STATE_READY) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid State: Current state is not READY");
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
				usleep(10);
				count++;
				if (10 == count) {
					SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to request");
					return ret;
				}
			}
		} else {
			SECURE_SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Set stop sound : %s", filename);
			break;
		}
	}

	return STT_ERROR_NONE;
}

int stt_unset_stop_sound(stt_h stt)
{
	bool stt_supported = false;
	bool mic_supported = false;
	if (0 == system_info_get_platform_bool(STT_FEATURE_PATH, &stt_supported)) {
		if (0 == system_info_get_platform_bool(STT_MIC_FEATURE_PATH, &mic_supported)) {
			if (false == stt_supported || false == mic_supported) {
				SLOG(LOG_ERROR, TAG_STTC, "[ERROR] STT NOT supported");
				return STT_ERROR_NOT_SUPPORTED;
			}
		}
	}

	SLOG(LOG_DEBUG, TAG_STTC, "===== STT UNSET STOP SOUND");

	if (NULL == stt) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Input parameter is NULL");
		return STT_ERROR_INVALID_PARAMETER;
	}

	stt_client_s* client = stt_client_get(stt);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Get state : A handle is not valid");
		return STT_ERROR_INVALID_PARAMETER;
	}

	/* check state */
	if (client->current_state != STT_STATE_READY) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid State: Current state is not READY");
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
				usleep(10);
				count++;
				if (10 == count) {
					SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to request");
					return ret;
				}
			}
		} else {
			SECURE_SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Unset stop sound");
			break;
		}
	}

	return STT_ERROR_NONE;
}

int stt_start(stt_h stt, const char* language, const char* type)
{
	bool stt_supported = false;
	bool mic_supported = false;
	if (0 == system_info_get_platform_bool(STT_FEATURE_PATH, &stt_supported)) {
		if (0 == system_info_get_platform_bool(STT_MIC_FEATURE_PATH, &mic_supported)) {
			if (false == stt_supported || false == mic_supported) {
				SLOG(LOG_ERROR, TAG_STTC, "[ERROR] STT NOT supported");
				return STT_ERROR_NOT_SUPPORTED;
			}
		}
	}

	SLOG(LOG_DEBUG, TAG_STTC, "===== STT START");

	if (NULL == stt) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Input parameter is NULL");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_ERROR_INVALID_PARAMETER;
	}

	stt_client_s* client = stt_client_get(stt);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_ERROR_INVALID_PARAMETER;
	}

	/* check state */
	if (client->current_state != STT_STATE_READY) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid State: Current state is not READY");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_ERROR_INVALID_STATE;
	}

	if (STT_INTERNAL_STATE_NONE != client->internal_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid State : Internal state is NOT none : %d", client->internal_state);
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_ERROR_INVALID_STATE;
	}

	char appid[128] = {0, };
	aul_app_get_appid_bypid(getpid(), appid, sizeof(appid));
	
	if (0 == strlen(appid)) {
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

	int ret = -1;
	/* do request */
	int count = 0;
	while (0 != ret) {
		ret = stt_dbus_request_start(client->uid, temp, type, client->silence, appid);
		if (0 > ret) {
			/* Failure */			
			if (STT_ERROR_TIMED_OUT != ret) {
				SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to start : %s", __stt_get_error_code(ret));
				if (NULL != temp)	free(temp);
				return ret;
			} else {
				SLOG(LOG_WARN, TAG_STTC, "[WARNING] retry to start");
				usleep(10);
				count++;
				if (10 == count) {
					SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to request");
					if (NULL != temp)	free(temp);
					return ret;
				}
			}
		} else {
			/* Success */
			if (NULL != temp)	free(temp);

			if (STT_RESULT_STATE_DONE == ret) {
				SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Start is done : %d", ret);
				client->before_state = client->current_state;
				client->current_state = STT_STATE_RECORDING;

				if (NULL != client->state_changed_cb) {
					stt_client_use_callback(client);
					client->state_changed_cb(client->stt, client->before_state, 
						client->current_state, client->state_changed_user_data);
					stt_client_not_use_callback(client);
					SLOG(LOG_DEBUG, TAG_STTC, "State changed callback is called");
				} else {
					SLOG(LOG_WARN, TAG_STTC, "[WARNING] State changed callback is null");
				}
			} else if (STT_RESULT_STATE_NOT_DONE == ret) {
				SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Start is not done : %d", ret);
				client->internal_state = STT_INTERNAL_STATE_STARTING;
			} else {
				SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid result : %d", ret);
			}

			ret = STT_ERROR_NONE;
			break;
		}
	}

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return ret;
}

int stt_stop(stt_h stt)
{
	bool stt_supported = false;
	bool mic_supported = false;
	if (0 == system_info_get_platform_bool(STT_FEATURE_PATH, &stt_supported)) {
		if (0 == system_info_get_platform_bool(STT_MIC_FEATURE_PATH, &mic_supported)) {
			if (false == stt_supported || false == mic_supported) {
				SLOG(LOG_ERROR, TAG_STTC, "[ERROR] STT NOT supported");
				return STT_ERROR_NOT_SUPPORTED;
			}
		}
	}

	SLOG(LOG_DEBUG, TAG_STTC, "===== STT STOP");

	if (NULL == stt) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Input parameter is NULL");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_ERROR_INVALID_PARAMETER;
	}

	stt_client_s* client = stt_client_get(stt);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_ERROR_INVALID_PARAMETER;
	}   
	
	/* check state */
	if (client->current_state != STT_STATE_RECORDING) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid State : Current state is NOT RECORDING");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_ERROR_INVALID_STATE;
	}

	if (STT_INTERNAL_STATE_NONE != client->internal_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid State : Internal state is NOT none : %d", client->internal_state);
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_ERROR_INVALID_STATE;
	}

	int ret = -1;
	/* do request */
	int count = 0;
	while (0 != ret) {
		ret = stt_dbus_request_stop(client->uid);
		if (0 > ret) {
			/* Failure */			
			if (STT_ERROR_TIMED_OUT != ret) {
				SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to stop : %s", __stt_get_error_code(ret));
				return ret;
			} else {
				SLOG(LOG_WARN, TAG_STTC, "[WARNING] retry stop");
				usleep(10);
				count++;
				if (10 == count) {
					SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to request");
					return ret;
				}
			}
		} else {
			if (STT_RESULT_STATE_DONE == ret) {
				SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Stop is done : %d", ret);
				client->before_state = client->current_state;
				client->current_state = STT_STATE_PROCESSING;

				if (NULL != client->state_changed_cb) {
					stt_client_use_callback(client);
					client->state_changed_cb(client->stt, client->before_state, 
						client->current_state, client->state_changed_user_data); 
					stt_client_not_use_callback(client);
					SLOG(LOG_DEBUG, TAG_STTC, "State changed callback is called");
				} else {
					SLOG(LOG_WARN, TAG_STTC, "[WARNING] State changed callback is null");
				}
			} else if (STT_RESULT_STATE_NOT_DONE == ret) {
				SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Stop is not done : %d", ret);
				client->internal_state = STT_INTERNAL_STATE_STOPING;
			} else {
				SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid result : %d", ret);
			}
			ret = STT_ERROR_NONE;
			break;
		}
	}

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return ret;
}


int stt_cancel(stt_h stt)
{
	bool stt_supported = false;
	bool mic_supported = false;
	if (0 == system_info_get_platform_bool(STT_FEATURE_PATH, &stt_supported)) {
		if (0 == system_info_get_platform_bool(STT_MIC_FEATURE_PATH, &mic_supported)) {
			if (false == stt_supported || false == mic_supported) {
				SLOG(LOG_ERROR, TAG_STTC, "[ERROR] STT NOT supported");
				return STT_ERROR_NOT_SUPPORTED;
			}
		}
	}

	SLOG(LOG_DEBUG, TAG_STTC, "===== STT CANCEL");

	if (NULL == stt) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Input handle is null");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_ERROR_INVALID_PARAMETER;
	}

	stt_client_s* client = stt_client_get(stt);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_ERROR_INVALID_PARAMETER;
	} 	

	/* check state */
	if (STT_STATE_RECORDING != client->current_state && STT_STATE_PROCESSING != client->current_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid state : Current state is 'Ready'");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_ERROR_INVALID_STATE;
	}

	if (STT_INTERNAL_STATE_NONE != client->internal_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Invalid State : Internal state is NOT none : %d", client->internal_state);
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_ERROR_INVALID_STATE;
	}

	int ret = -1;
	/* do request */
	int count = 0;
	while (0 != ret) {
		ret = stt_dbus_request_cancel(client->uid);
		if (0 != ret) {	
			/* Failure */			
			if (STT_ERROR_TIMED_OUT != ret) {
				SLOG(LOG_DEBUG, TAG_STTC, "[ERROR] Fail to cancel : %s", __stt_get_error_code(ret));
				return ret;
			} else {
				SLOG(LOG_WARN, TAG_STTC, "[WARNING] retry");
				usleep(10);
				count++;
				if (10 == count) {
					SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to request");
					return ret;
				}
			}
		} else {
			SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS]");

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
			ret = STT_ERROR_NONE;
			break;
		}
	}

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return ret;
}

static int __stt_get_audio_volume(float* volume)
{
	FILE* fp = fopen(STT_AUDIO_VOLUME_PATH, "rb");
	if (!fp) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to open Volume File");
		return STT_ERROR_OPERATION_FAILED;
	}

	int readlen = fread((void*)volume, sizeof(*volume), 1, fp);
	fclose(fp);

	if (0 == readlen)
		*volume = 0.0f;

	return 0;
}

int stt_get_recording_volume(stt_h stt, float* volume)
{
	bool stt_supported = false;
	bool mic_supported = false;
	if (0 == system_info_get_platform_bool(STT_FEATURE_PATH, &stt_supported)) {
		if (0 == system_info_get_platform_bool(STT_MIC_FEATURE_PATH, &mic_supported)) {
			if (false == stt_supported || false == mic_supported) {
				SLOG(LOG_ERROR, TAG_STTC, "[ERROR] STT NOT supported");
				return STT_ERROR_NOT_SUPPORTED;
			}
		}
	}

	if (NULL == stt || NULL == volume) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Input parameter is NULL");
		return STT_ERROR_INVALID_PARAMETER;
	}

	stt_client_s* client = stt_client_get(stt);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] A handle is not available");
		return STT_ERROR_INVALID_PARAMETER;
	} 
	
	if (STT_STATE_RECORDING != client->current_state) {
		SLOG(LOG_DEBUG, TAG_STTC, "[ERROR] Invalid state : NO 'Recording' state");
		return STT_ERROR_INVALID_STATE;
	}    
	
	int ret = 0;
	ret = __stt_get_audio_volume(volume);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to get audio volume : %s", __stt_get_error_code(ret));
		return STT_ERROR_OPERATION_FAILED;
	}

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
	bool stt_supported = false;
	bool mic_supported = false;
	if (0 == system_info_get_platform_bool(STT_FEATURE_PATH, &stt_supported)) {
		if (0 == system_info_get_platform_bool(STT_MIC_FEATURE_PATH, &mic_supported)) {
			if (false == stt_supported || false == mic_supported) {
				SLOG(LOG_ERROR, TAG_STTC, "[ERROR] STT NOT supported");
				return STT_ERROR_NOT_SUPPORTED;
			}
		}
	}

	SLOG(LOG_DEBUG, TAG_STTC, "===== STT FOREACH DETAILED RESULT");

	if (NULL == callback) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Input parameter is NULL");
		return STT_ERROR_INVALID_PARAMETER;
	}

	stt_client_s* client = stt_client_get(stt);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail : A handle is not valid");
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

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to notify error : A handle is not valid");
		return EINA_FALSE;
	}

	if (NULL == stt_client_get_by_uid(client->uid))
		return EINA_FALSE;

	if (NULL != client->error_cb) {
		stt_client_use_callback(client);
		client->error_cb(client->stt, client->reason, client->error_user_data); 
		stt_client_not_use_callback(client);
		SLOG(LOG_DEBUG, TAG_STTC, "Error callback is called");
	} else {
		SLOG(LOG_WARN, TAG_STTC, "[WARNING] Error callback is null");
	}

	return EINA_FALSE;
}

int __stt_cb_error(int uid, int reason)
{
	stt_client_s* client = stt_client_get_by_uid(uid);
	if( NULL == client ) {
		SLOG(LOG_ERROR, TAG_STTC, "Handle not found\n");
		return -1;
	}

	client->reason = reason;

	if (NULL != client->error_cb) {
		ecore_timer_add(0, __stt_notify_error, client);
	} else {
		SLOG(LOG_WARN, TAG_STTC, "[WARNING] Error callback is null");
	}    

	return 0;
}

static Eina_Bool __stt_notify_state_changed(void *data)
{
	stt_client_s* client = (stt_client_s*)data;

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to notify error : A handle is not valid");
		return EINA_FALSE;
	}

	if (NULL == stt_client_get_by_uid(client->uid))
		return EINA_FALSE;

	if (STT_INTERNAL_STATE_STARTING == client->internal_state && STT_STATE_RECORDING == client->current_state) {
		client->internal_state = STT_INTERNAL_STATE_NONE;
		SLOG(LOG_DEBUG, TAG_STTC, "Internal state change NULL");
	} else if (STT_INTERNAL_STATE_STOPING == client->internal_state && STT_STATE_PROCESSING == client->current_state) {
		client->internal_state = STT_INTERNAL_STATE_NONE;
		SLOG(LOG_DEBUG, TAG_STTC, "Internal state change NULL");
	}

	if (NULL != client->state_changed_cb) {
		stt_client_use_callback(client);
		client->state_changed_cb(client->stt, client->before_state, 
			client->current_state, client->state_changed_user_data); 
		stt_client_not_use_callback(client);
	} else {
		SLOG(LOG_WARN, TAG_STTC, "[WARNING] State changed callback is null");
	}

	return EINA_FALSE;
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
		for (i = 0;i < client->data_count;i++) {
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
			ecore_timer_add(0, __stt_notify_state_changed, client);
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
		return -1;
	}

	if (NULL != msg)	SLOG(LOG_DEBUG, TAG_STTC, "Recognition Result Message = %s", msg);

	int i=0;
	for (i = 0;i < data_count;i++) {
		if (NULL != data[i])	SECURE_SLOG(LOG_DEBUG, TAG_STTC, "Recognition Result[%d] = %s", i, data[i]);
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

			for (i = 0;i < data_count;i++) {
				if(NULL != data[i])
					temp[i] = strdup(data[i]);
				else 
					SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Result data is error");
			}

			client->data_list = temp;
		}

		ecore_timer_add(0, __stt_notify_result, client);
	} else {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] User result callback is null");
	}

	return 0;
}

int __stt_cb_set_state(int uid, int state)
{
	stt_client_s* client = stt_client_get_by_uid(uid);
	if( NULL == client ) {
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

	ecore_timer_add(0, __stt_notify_state_changed, client);
	return 0;
}

int stt_set_recognition_result_cb(stt_h stt, stt_recognition_result_cb callback, void* user_data)
{
	bool stt_supported = false;
	bool mic_supported = false;
	if (0 == system_info_get_platform_bool(STT_FEATURE_PATH, &stt_supported)) {
		if (0 == system_info_get_platform_bool(STT_MIC_FEATURE_PATH, &mic_supported)) {
			if (false == stt_supported || false == mic_supported) {
				SLOG(LOG_ERROR, TAG_STTC, "[ERROR] STT NOT supported");
				return STT_ERROR_NOT_SUPPORTED;
			}
		}
	}

	if (stt == NULL || callback == NULL)
		return STT_ERROR_INVALID_PARAMETER;

	stt_client_s* client = stt_client_get(stt);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] A handle is not available");
		return STT_ERROR_INVALID_PARAMETER;
	}

	if (STT_STATE_CREATED != client->current_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Current state is not 'ready'");
		return STT_ERROR_INVALID_STATE;
	}

	client->recognition_result_cb = callback;
	client->recognition_result_user_data = user_data;

	return 0;
}

int stt_unset_recognition_result_cb(stt_h stt)
{
	bool stt_supported = false;
	bool mic_supported = false;
	if (0 == system_info_get_platform_bool(STT_FEATURE_PATH, &stt_supported)) {
		if (0 == system_info_get_platform_bool(STT_MIC_FEATURE_PATH, &mic_supported)) {
			if (false == stt_supported || false == mic_supported) {
				SLOG(LOG_ERROR, TAG_STTC, "[ERROR] STT NOT supported");
				return STT_ERROR_NOT_SUPPORTED;
			}
		}
	}

	if (NULL == stt)
		return STT_ERROR_INVALID_PARAMETER;

	stt_client_s* client = stt_client_get(stt);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] A handle is not available");
		return STT_ERROR_INVALID_PARAMETER;
	}

	if (STT_STATE_CREATED != client->current_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Current state is not 'ready'");
		return STT_ERROR_INVALID_STATE;
	}

	client->recognition_result_cb = NULL;
	client->recognition_result_user_data = NULL;

	return 0;
}

int stt_set_state_changed_cb(stt_h stt, stt_state_changed_cb callback, void* user_data)
{
	bool stt_supported = false;
	bool mic_supported = false;
	if (0 == system_info_get_platform_bool(STT_FEATURE_PATH, &stt_supported)) {
		if (0 == system_info_get_platform_bool(STT_MIC_FEATURE_PATH, &mic_supported)) {
			if (false == stt_supported || false == mic_supported) {
				SLOG(LOG_ERROR, TAG_STTC, "[ERROR] STT NOT supported");
				return STT_ERROR_NOT_SUPPORTED;
			}
		}
	}

	if (NULL == stt || NULL == callback)
		return STT_ERROR_INVALID_PARAMETER;

	stt_client_s* client = stt_client_get(stt);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] A handle is not available");
		return STT_ERROR_INVALID_PARAMETER;
	}

	if (STT_STATE_CREATED != client->current_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Current state is not 'ready'");
		return STT_ERROR_INVALID_STATE;
	}

	client->state_changed_cb = callback;
	client->state_changed_user_data = user_data;

	return 0;
}

int stt_unset_state_changed_cb(stt_h stt)
{
	bool stt_supported = false;
	bool mic_supported = false;
	if (0 == system_info_get_platform_bool(STT_FEATURE_PATH, &stt_supported)) {
		if (0 == system_info_get_platform_bool(STT_MIC_FEATURE_PATH, &mic_supported)) {
			if (false == stt_supported || false == mic_supported) {
				SLOG(LOG_ERROR, TAG_STTC, "[ERROR] STT NOT supported");
				return STT_ERROR_NOT_SUPPORTED;
			}
		}
	}

	if (NULL == stt)
		return STT_ERROR_INVALID_PARAMETER;

	stt_client_s* client = stt_client_get(stt);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] A handle is not available");
		return STT_ERROR_INVALID_PARAMETER;
	}

	if (STT_STATE_CREATED != client->current_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Current state is not 'ready'");
		return STT_ERROR_INVALID_STATE;
	}

	client->state_changed_cb = NULL;
	client->state_changed_user_data = NULL;

	return 0;
}

int stt_set_error_cb(stt_h stt, stt_error_cb callback, void* user_data)
{
	bool stt_supported = false;
	bool mic_supported = false;
	if (0 == system_info_get_platform_bool(STT_FEATURE_PATH, &stt_supported)) {
		if (0 == system_info_get_platform_bool(STT_MIC_FEATURE_PATH, &mic_supported)) {
			if (false == stt_supported || false == mic_supported) {
				SLOG(LOG_ERROR, TAG_STTC, "[ERROR] STT NOT supported");
				return STT_ERROR_NOT_SUPPORTED;
			}
		}
	}

	if (NULL == stt || NULL == callback)
		return STT_ERROR_INVALID_PARAMETER;

	stt_client_s* client = stt_client_get(stt);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] A handle is not available");
		return STT_ERROR_INVALID_PARAMETER;
	}

	if (STT_STATE_CREATED != client->current_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Current state is not 'ready'");
		return STT_ERROR_INVALID_STATE;
	}

	client->error_cb = callback;
	client->error_user_data = user_data;

	return 0;
}

int stt_unset_error_cb(stt_h stt)
{
	bool stt_supported = false;
	bool mic_supported = false;
	if (0 == system_info_get_platform_bool(STT_FEATURE_PATH, &stt_supported)) {
		if (0 == system_info_get_platform_bool(STT_MIC_FEATURE_PATH, &mic_supported)) {
			if (false == stt_supported || false == mic_supported) {
				SLOG(LOG_ERROR, TAG_STTC, "[ERROR] STT NOT supported");
				return STT_ERROR_NOT_SUPPORTED;
			}
		}
	}

	if (NULL == stt)
		return STT_ERROR_INVALID_PARAMETER;

	stt_client_s* client = stt_client_get(stt);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] A handle is not available");
		return STT_ERROR_INVALID_PARAMETER;
	}

	if (STT_STATE_CREATED != client->current_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Current state is not 'ready'");
		return STT_ERROR_INVALID_STATE;
	}

	client->error_cb = NULL;
	client->error_user_data = NULL;

	return 0;
}

int stt_set_default_language_changed_cb(stt_h stt, stt_default_language_changed_cb callback, void* user_data)
{
	bool stt_supported = false;
	bool mic_supported = false;
	if (0 == system_info_get_platform_bool(STT_FEATURE_PATH, &stt_supported)) {
		if (0 == system_info_get_platform_bool(STT_MIC_FEATURE_PATH, &mic_supported)) {
			if (false == stt_supported || false == mic_supported) {
				SLOG(LOG_ERROR, TAG_STTC, "[ERROR] STT NOT supported");
				return STT_ERROR_NOT_SUPPORTED;
			}
		}
	}

	if (NULL == stt || NULL == callback)
		return STT_ERROR_INVALID_PARAMETER;

	stt_client_s* client = stt_client_get(stt);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] A handle is not available");
		return STT_ERROR_INVALID_PARAMETER;
	}

	if (STT_STATE_CREATED != client->current_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Current state is not 'ready'");
		return STT_ERROR_INVALID_STATE;
	}

	client->default_lang_changed_cb = callback;
	client->default_lang_changed_user_data = user_data;

	return 0;
}

int stt_unset_default_language_changed_cb(stt_h stt)
{
	bool stt_supported = false;
	bool mic_supported = false;
	if (0 == system_info_get_platform_bool(STT_FEATURE_PATH, &stt_supported)) {
		if (0 == system_info_get_platform_bool(STT_MIC_FEATURE_PATH, &mic_supported)) {
			if (false == stt_supported || false == mic_supported) {
				SLOG(LOG_ERROR, TAG_STTC, "[ERROR] STT NOT supported");
				return STT_ERROR_NOT_SUPPORTED;
			}
		}
	}

	if (NULL == stt)
		return STT_ERROR_INVALID_PARAMETER;

	stt_client_s* client = stt_client_get(stt);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] A handle is not available");
		return STT_ERROR_INVALID_PARAMETER;
	}

	if (STT_STATE_CREATED != client->current_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Current state is not 'ready'");
		return STT_ERROR_INVALID_STATE;
	}

	client->default_lang_changed_cb = NULL;
	client->default_lang_changed_user_data = NULL;

	return 0;
}
