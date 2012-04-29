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


#include <sys/wait.h>

#include "stt_main.h"
#include "stt_setting.h"
#include "stt_setting_dbus.h"

#define CONNECTION_RETRY_COUNT 2

static bool g_is_setting_initialized = false;

static int __check_stt_daemon();

int stt_setting_initialize ()
{
	SLOG(LOG_DEBUG, TAG_STTC, "===== Initialize STT Setting");

	int ret = 0;
	if (true == g_is_setting_initialized) {
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
		__check_stt_daemon();
	}

	/* do request */
	int i = 1;
	while(1) {
		ret = stt_setting_dbus_request_initialize();

		if (STT_SETTING_ERROR_ENGINE_NOT_FOUND == ret) {
			SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Engine not found");
			break;
		} else if(ret) {
			usleep(1);
			if (i == CONNECTION_RETRY_COUNT) {
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
		g_is_setting_initialized = true;
		SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Initialize");
	}

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return ret;
}


int stt_setting_finalize ()
{
	SLOG(LOG_DEBUG, TAG_STTC, "===== Finalize STT Setting");
	
	int ret = 0;

	if (false == g_is_setting_initialized) {
		SLOG(LOG_WARN, TAG_STTC, "[WARNING] Not initialized");
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return STT_SETTING_ERROR_NONE;
	}

	ret = stt_setting_dbus_request_finalilze();
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_STTC, "Fail : finialize(%d)", ret);
		SLOG(LOG_DEBUG, TAG_STTC, "=====");
		SLOG(LOG_DEBUG, TAG_STTC, " ");
		return ret;
	}
    
	if (0 != stt_setting_dbus_close_connection()) {
		SLOG(LOG_ERROR, TAG_STTC, "[ERROR] Fail to close connection");
	} else {
		SLOG(LOG_DEBUG, TAG_STTC, "[SUCCESS] Finalize");
	}

	g_is_setting_initialized = false;

	SLOG(LOG_DEBUG, TAG_STTC, "=====");
	SLOG(LOG_DEBUG, TAG_STTC, " ");

	return STT_SETTING_ERROR_NONE;
}

int stt_setting_foreach_supported_engines(stt_setting_supported_engine_cb callback, void* user_data)
{
	SLOG(LOG_DEBUG, TAG_STTC, "===== Foreach supported engines");

	if (false == g_is_setting_initialized) {
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

	if (false == g_is_setting_initialized) {
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

	if (false == g_is_setting_initialized) {
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

	if (false == g_is_setting_initialized) {
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

	if (false == g_is_setting_initialized) {
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

	if (false == g_is_setting_initialized) {
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

	if (false == g_is_setting_initialized) {
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

	if (!g_is_setting_initialized) {
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

	if (!g_is_setting_initialized) {
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

	if (!g_is_setting_initialized) {
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

	if (!g_is_setting_initialized) {
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

	if (!g_is_setting_initialized) {
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

	if (!g_is_setting_initialized) {
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

	if (!g_is_setting_initialized) {
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

static bool __stt_is_alive()
{
	FILE *fp = NULL;
	char buff[256] = {'\0',};
	char cmd[256] = {'\0',};

	memset(buff, '\0', sizeof(char) * 256);
	memset(cmd, '\0', sizeof(char) * 256);

	fp = popen("ps", "r");
	if (NULL == fp) {
		SLOG(LOG_DEBUG, TAG_STTC, "[STT SETTING ERROR] popen error \n");
		return FALSE;
	}

	while (fgets(buff, 255, fp)) {
		strcpy(cmd, buff + 26);

		if( 0 == strncmp(cmd, "[stt-daemon]", strlen("[stt-daemon]")) ||
		0 == strncmp(cmd, "stt-daemon", strlen("stt-daemon")) ||
		0 == strncmp(cmd, "/usr/bin/stt-daemon", strlen("/usr/bin/stt-daemon"))) {
			SLOG(LOG_DEBUG, TAG_STTC, "stt-daemon is ALIVE !! \n");
			fclose(fp);
			return TRUE;
		}
	}

	fclose(fp);

	SLOG(LOG_DEBUG, TAG_STTC, "THERE IS NO stt-daemon !! \n");

	return FALSE;
}

static void __my_sig_child(int signo, siginfo_t *info, void *data)
{
	int status = 0;
	pid_t child_pid, child_pgid;

	child_pgid = getpgid(info->si_pid);
	SLOG(LOG_DEBUG, TAG_STTC, "Signal handler: dead pid = %d, pgid = %d\n", info->si_pid, child_pgid);

	while ((child_pid = waitpid(-1, &status, WNOHANG)) > 0) {
		if(child_pid == child_pgid)
			killpg(child_pgid, SIGKILL);
	}

	return;
}

int __check_stt_daemon()
{
	if (TRUE == __stt_is_alive()) 
		return 0;
	
	/* fork-exec stt-daemom */
	int pid = 0, i = 0;
	struct sigaction act, dummy;
	act.sa_handler = NULL;
	act.sa_sigaction = __my_sig_child;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_NOCLDSTOP | SA_SIGINFO;
	
	if (sigaction(SIGCHLD, &act, &dummy) < 0) {
		SLOG(LOG_ERROR, TAG_STTC, "%s\n", "Cannot make a signal handler\n");
		return -1;
	}

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
		sleep(1);
		break;
	}

	return 0;
}

