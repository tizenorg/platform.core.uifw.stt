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


#include <sys/wait.h>
#include <Ecore.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include "stt_main.h"
#include "stt_setting.h"
#include "stt_setting_dbus.h"


static int __check_setting_stt_daemon();

static bool g_is_daemon_started = false;

static Ecore_Timer* g_setting_connect_timer = NULL;

static stt_setting_state_e g_state = STT_SETTING_STATE_NONE;

static stt_setting_initialized_cb g_initialized_cb;

static void* g_user_data;

static int g_reason;

/* API Implementation */
static Eina_Bool __stt_setting_initialized(void *data)
{
	g_initialized_cb(g_state, g_reason, g_user_data);

	return EINA_FALSE;
}

static Eina_Bool __stt_setting_connect_daemon(void *data)
{
	/* Send hello */
	if (0 != stt_setting_dbus_request_hello()) {
		if (false == g_is_daemon_started) {
			g_is_daemon_started = true;
			__check_setting_stt_daemon();
		}
		return EINA_TRUE;
	}

	SLOG(LOG_DEBUG, TAG_STTC, "===== Connect daemon");

	/* do request initialize */
	int ret = -1;

	ret = stt_setting_dbus_request_initialize();

	if (STT_SETTING_ERROR_ENGINE_NOT_FOUND == ret) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Engine not found");
	} else if (STT_SETTING_ERROR_NONE != ret) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to connection : %d", ret);
	} else {
		/* success to connect stt-daemon */
		g_state = STT_SETTING_STATE_READY;
	}

	g_reason = ret;

	ecore_timer_add(0, __stt_setting_initialized, NULL);

	g_setting_connect_timer = NULL;

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return EINA_FALSE;
}

int stt_setting_initialize ()
{
	SLOG(LOG_DEBUG, TAG_STTC, "===== Initialize STT Setting");

	int ret = 0;
	if (STT_SETTING_STATE_READY == g_state) {
		SLOG(LOG_WARN, TAG_STTC, "[WARNING] STT Setting has already been initialized. \n");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_SETTING_ERROR_NONE;
	}

	if (0 != stt_setting_dbus_open_connection()) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to open connection\n");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_SETTING_ERROR_OPERATION_FAILED;
	}

	/* Send hello */
	if (0 != stt_setting_dbus_request_hello()) {
		__check_setting_stt_daemon();
	}

	/* do request */
	int i = 1;
	while(1) {
		ret = stt_setting_dbus_request_initialize();

		if (STT_SETTING_ERROR_ENGINE_NOT_FOUND == ret) {
			SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Engine not found");
			break;
		} else if(ret) {
			sleep(1);
			if (i == 3) {
				SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Connection Time out");
				ret = STT_SETTING_ERROR_TIMED_OUT;			    
				break;
			}
			i++;
		} else {
			/* success to connect stt-daemon */
			break;
		}
	}

	if (STT_SETTING_ERROR_NONE == ret) {
		g_state = STT_SETTING_STATE_READY;
		SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Initialize");
	}

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return ret;
}

int stt_setting_initialize_async(stt_setting_initialized_cb callback, void* user_data)
{
	SLOG(LOG_DEBUG, TAG_STTC, "===== Initialize STT Setting");

	if (STT_SETTING_STATE_READY == g_state) {
		SLOG(LOG_WARN, TAG_STTC, "[WARNING] STT Setting has already been initialized. \n");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_SETTING_ERROR_NONE;
	}

	if( 0 != stt_setting_dbus_open_connection() ) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to open connection\n ");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_SETTING_ERROR_OPERATION_FAILED;
	}

	g_initialized_cb = callback;
	g_user_data = user_data;

	g_setting_connect_timer = ecore_timer_add(0, __stt_setting_connect_daemon, NULL);

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return STT_SETTING_ERROR_NONE;
}

int stt_setting_finalize ()
{
	SLOG(LOG_DEBUG, TAG_STTC, "===== Finalize STT Setting");
	
	int ret = 0;

	if (STT_SETTING_STATE_READY == g_state) {
		ret = stt_setting_dbus_request_finalilze();
		if (0 != ret) {
			SLOG(LOG_ERROR, TAG_STTC, "Fail : finialize(%d)", ret);
		}
	}

	if (NULL != g_setting_connect_timer) {
		SLOG(LOG_DEBUG, TAG_STTC, "Setting Connect Timer is deleted");
		ecore_timer_del(g_setting_connect_timer);
	}

	g_is_daemon_started = false;

	if (0 != stt_setting_dbus_close_connection()) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to close connection");
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Finalize");
	}

	g_state = STT_SETTING_STATE_NONE;

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return STT_SETTING_ERROR_NONE;
}

int stt_setting_foreach_supported_engines(stt_setting_supported_engine_cb callback, void* user_data)
{
	SLOG(LOG_DEBUG, TAG_STTC, "===== Foreach supported engines");

	if (STT_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Not initialized");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_SETTING_ERROR_INVALID_STATE;
	}

	if (NULL == callback) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Callback is NULL");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_SETTING_ERROR_INVALID_PARAMETER;
	}

	int ret = stt_setting_dbus_request_get_engine_list(callback, user_data);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Result : %d", ret);
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Foreach supported engines");
	}
	
	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return ret;
}

int stt_setting_get_engine(char** engine_id)
{
	SLOG(LOG_DEBUG, TAG_STTC, "===== Get current engine");

	if (STT_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Not initialized");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_SETTING_ERROR_INVALID_STATE;
	}

	if (NULL == engine_id) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Engine id is NULL");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_SETTING_ERROR_INVALID_PARAMETER;
	}

	int ret = stt_setting_dbus_request_get_engine(engine_id);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Result : %d", ret);
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Get current engine");
	}

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return ret;
}

int stt_setting_set_engine(const char* engine_id)
{
	SLOG(LOG_DEBUG, TAG_STTC, "===== Set current engine");

	if (STT_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Not initialized");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_SETTING_ERROR_INVALID_STATE;
	}

	if (NULL == engine_id) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Engine id is NULL");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_SETTING_ERROR_INVALID_PARAMETER;
	}

	int ret = stt_setting_dbus_request_set_engine(engine_id);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Result : %d", ret);
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Set current engine");
	}

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");
    
	return ret;
}

int stt_setting_foreach_supported_languages(stt_setting_supported_language_cb callback, void* user_data)
{
	SLOG(LOG_DEBUG, TAG_STTC, "===== Foreach supported languages");

	if (STT_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Not initialized");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_SETTING_ERROR_INVALID_STATE;
	}

	if (NULL == callback) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Param is NULL");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_SETTING_ERROR_INVALID_PARAMETER;
	}

	int ret = stt_setting_dbus_request_get_language_list(callback, user_data);

	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Result : %d", ret);
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Foreach supported languages");
	}
	
	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return ret;
}

int stt_setting_get_default_language(char** language)
{
	SLOG(LOG_DEBUG, TAG_STTC, "===== Get default language");

	if (STT_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Not initialized");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_SETTING_ERROR_INVALID_STATE;
	}

	if (NULL == language) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Input parameter is NULL");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_SETTING_ERROR_INVALID_PARAMETER;
	}

	int ret = stt_setting_dbus_request_get_default_language(language);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Result : %d", ret);
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Foreach supported voices");
	}

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return ret;
}

int stt_setting_set_default_language(const char* language)
{
	SLOG(LOG_DEBUG, TAG_STTC, "===== Set default language");

	if (STT_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Not initialized");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_SETTING_ERROR_INVALID_STATE;
	}

	if (NULL == language) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Input parameter is NULL");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_SETTING_ERROR_INVALID_PARAMETER;
	}

	int ret = stt_setting_dbus_request_set_default_language(language);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Result : %d", ret);
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Set default voice");
	}

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return ret;
}

int stt_setting_get_profanity_filter(bool* value)
{
	SLOG(LOG_DEBUG, TAG_STTC, "===== Get profanity filter");

	int ret = 0;

	if (STT_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Not initialized");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_SETTING_ERROR_INVALID_STATE;
	}

	if (NULL == value) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Input parameter is NULL");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_SETTING_ERROR_INVALID_PARAMETER;
	}

	ret = stt_setting_dbus_request_get_profanity_filter(value);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Result (%d)", ret);    
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Get profanity filter(%s)", *value ? "true":"false");
	}
	
	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return ret;
}

int stt_setting_set_profanity_filter(const bool value)
{
	SLOG(LOG_DEBUG, TAG_STTC, "===== Set profanity filter");

	int ret = 0;

	if (STT_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] not initialized");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_SETTING_ERROR_INVALID_STATE;
	}

	ret = stt_setting_dbus_request_set_profanity_filter(value);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Result (%d)", ret);    
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Set profanity filter(%s)", value ? "true":"false");
	}

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return ret;
}

int stt_setting_get_punctuation_override(bool* value)
{
	SLOG(LOG_DEBUG, TAG_STTC, "===== Get punctuation override");

	int ret = 0;

	if (STT_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] not initialized");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_SETTING_ERROR_INVALID_STATE;
	}

	if (NULL == value) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] param is NULL");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_SETTING_ERROR_INVALID_PARAMETER;
	}

	ret = stt_setting_dbus_request_get_punctuation_override(value);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Result (%d)", ret); 
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Get punctuation override(%s)", *value ? "true":"false");
	}

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return ret;
}

int stt_setting_set_punctuation_override(bool value)
{
	SLOG(LOG_DEBUG, TAG_STTC, "===== Set punctuation override");

	int ret = 0;

	if (STT_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] not initialized");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_SETTING_ERROR_INVALID_STATE;
	}

	ret = stt_setting_dbus_request_set_punctuation_override(value);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Result (%d)", ret);    
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Set punctuation override(%s)", value ? "true":"false");
	}

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return ret;
}

int stt_setting_get_silence_detection(bool* value)
{
	SLOG(LOG_DEBUG, TAG_STTC, "===== Get silence detection");

	int ret = 0;

	if (STT_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] not initialized");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_SETTING_ERROR_INVALID_STATE;
	}

	if (NULL == value) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] param is NULL");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_SETTING_ERROR_INVALID_PARAMETER;
	}

	ret = stt_setting_dbus_request_get_silence_detection(value);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Result (%d)", ret);    
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Get silence detection(%s)", *value ? "true":"false");
	}

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return ret;
}

int stt_setting_set_silence_detection(bool value)
{
	SLOG(LOG_DEBUG, TAG_STTC, "===== Set silence detection");

	int ret = 0;

	if (STT_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] not initialized");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_SETTING_ERROR_INVALID_STATE;
	}

	ret = stt_setting_dbus_request_set_silence_detection(value);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Result (%d)", ret);    
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Set silence detection(%s)", value ? "true":"false");
	}

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return ret;
}

int stt_setting_foreach_engine_settings(stt_setting_engine_setting_cb callback, void* user_data)
{
	SLOG(LOG_DEBUG, TAG_STTC, "===== Foreach engine setting");

	int ret = 0;

	if (STT_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] not initialized");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_SETTING_ERROR_INVALID_STATE;
	}

	if (NULL == callback) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] parameter is NULL");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_SETTING_ERROR_INVALID_PARAMETER;
	}

	ret = stt_setting_dbus_request_get_engine_setting(callback, user_data);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Result (%d)", ret);    
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Foreach engine setting");
	}

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return ret;
}

int stt_setting_set_engine_setting(const char* key, const char* value)
{
	SLOG(LOG_DEBUG, TAG_STTC, "===== Set engine setting");

	int ret = 0;

	if (STT_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] not initialized");
		return STT_SETTING_ERROR_INVALID_STATE;
	}

	if (NULL == key || NULL == value) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] parameter is NULL");
		return STT_SETTING_ERROR_INVALID_PARAMETER;
	}

	ret = stt_setting_dbus_request_set_engine_setting(key, value);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR]  Result (%d)", ret);    
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Set engine setting(%s)", *value ? "true":"false");
	}

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return ret;
}

int __setting_get_cmd_line(char *file, char *buf) 
{
	FILE *fp = NULL;
	int i;

	fp = fopen(file, "r");
	if (fp == NULL) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Get command line");
		return -1;
	}

	memset(buf, 0, 256);
	fgets(buf, 256, fp);
	fclose(fp);

	return 0;
}

static bool __stt_setting_is_alive()
{
	DIR *dir;
	struct dirent *entry;
	struct stat filestat;
	
	int pid;
	char cmdLine[256];
	char tempPath[256];

	dir  = opendir("/proc");
	if (NULL == dir) {
		SLOG(LOG_ERROR, TAG_STTC, "process checking is FAILED");
		return FALSE;
	}

	while ((entry = readdir(dir)) != NULL) {
		if (0 != lstat(entry->d_name, &filestat))
			continue;

		if (!S_ISDIR(filestat.st_mode))
			continue;

		pid = atoi(entry->d_name);
		if (pid <= 0) continue;

		sprintf(tempPath, "/proc/%d/cmdline", pid);
		if (0 != __setting_get_cmd_line(tempPath, cmdLine)) {
			continue;
		}

		if ( 0 == strncmp(cmdLine, "[stt-daemon]", strlen("[stt-daemon]")) ||
			0 == strncmp(cmdLine, "stt-daemon", strlen("stt-daemon")) ||
			0 == strncmp(cmdLine, "/usr/bin/stt-daemon", strlen("/usr/bin/stt-daemon"))) {
				SLOG(LOG_DEBUG, TAG_STTC, "stt-daemon is ALIVE !! \n");
				closedir(dir);
				return TRUE;
		}
	}
	SLOG(LOG_DEBUG, TAG_STTC, "THERE IS NO stt-daemon !! \n");

	closedir(dir);
	return FALSE;

}

int __check_setting_stt_daemon()
{
	if (TRUE == __stt_setting_is_alive()) 
		return 0;
	
	/* fork-exec stt-daemom */
	int pid = 0, i = 0;

	pid = fork();

	switch(pid) {
	case -1:
		SLOG(LOG_DEBUG, TAG_STTC, "Fail to create stt-daemon");
		break;

	case 0:
		setsid();
		for( i = 0 ; i < _NSIG ; i++ )
			signal(i, SIG_DFL);

		execl("/usr/bin/stt-daemon", "/usr/bin/stt-daemon", NULL);
		break;

	default:
		break;
	}

	return 0;
}

