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

#include <dirent.h>
#include <dlfcn.h>
#include <dlog.h>
#include <Ecore.h>
#include <glib.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <vconf.h>

#include "stt_config_mgr.h"
#include "stt_defs.h"
#include "stt_config_parser.h"

#define VCONFKEY_VOICE_INPUT_LANGUAGE     "db/voice_input/language"

typedef struct {
	int	uid;
	stt_config_engine_changed_cb	engine_cb;
	stt_config_lang_changed_cb	lang_cb;
	stt_config_bool_changed_cb	bool_cb;
	void*	user_data;
}stt_config_client_s;


extern const char* stt_tag();

static GSList* g_engine_list = NULL;

static GSList* g_config_client_list = NULL;

static stt_config_s* g_config_info;

static Ecore_Fd_Handler* g_fd_handler_noti = NULL;
static int g_fd_noti;
static int g_wd_noti;

int __stt_config_mgr_print_engine_info();

bool __stt_config_mgr_check_lang_is_valid(const char* engine_id, const char* language)
{
	if (NULL == engine_id || NULL == language) {
		SLOG(LOG_ERROR, stt_tag(), "[ERROR] Input parameter is NULL");
		return false;
	}

	GSList *iter = NULL;
	stt_engine_info_s *engine_info = NULL;

	if (0 >= g_slist_length(g_engine_list)) {
		SLOG(LOG_ERROR, stt_tag(), "[ERROR] There is no engine!!");
		return false;
	}

	/* Get a first item */
	iter = g_slist_nth(g_engine_list, 0);

	while (NULL != iter) {
		engine_info = iter->data;

		if (NULL == engine_info) {
			SLOG(LOG_ERROR, stt_tag(), "[ERROR] Engine info is NULL");
			return false;
		}

		if (0 != strcmp(engine_id, engine_info->uuid)) {
			iter = g_slist_next(iter);
			continue;
		}

		GSList *iter_lang = NULL;
		char* engine_lang;
		if (g_slist_length(engine_info->languages) > 0) {
			/* Get a first item */
			iter_lang = g_slist_nth(engine_info->languages, 0);

			int i = 1;	
			while (NULL != iter_lang) {
				/*Get handle data from list*/
				engine_lang = iter_lang->data;

				SECURE_SLOG(LOG_DEBUG, stt_tag(), "  [%dth] %s", i, engine_lang);

				if (0 == strcmp(language, engine_lang)) {
					return true;
				}

				/*Get next item*/
				iter_lang = g_slist_next(iter_lang);
				i++;
			}
		}
		break;
	}

	return false;
}

int __stt_config_mgr_select_lang(const char* engine_id, char** language)
{
	if (NULL == engine_id || NULL == language) {
		SLOG(LOG_ERROR, stt_tag(), "[ERROR] Input parameter is NULL");
		return false;
	}

	GSList *iter = NULL;
	stt_engine_info_s *engine_info = NULL;

	if (0 >= g_slist_length(g_engine_list)) {
		SLOG(LOG_ERROR, stt_tag(), "[ERROR] There is no engine!!");
		return false;
	}

	/* Get a first item */
	iter = g_slist_nth(g_engine_list, 0);

	while (NULL != iter) {
		engine_info = iter->data;

		if (NULL == engine_info) {
			SLOG(LOG_ERROR, stt_tag(), "engine info is NULL");
			return false;
		}

		if (0 != strcmp(engine_id, engine_info->uuid)) {
			iter = g_slist_next(iter);
			continue;
		}
		
		GSList *iter_lang = NULL;
		char* engine_lang = NULL;
		if (g_slist_length(engine_info->languages) > 0) {
			/* Get a first item */
			iter_lang = g_slist_nth(engine_info->languages, 0);
			
			while (NULL != iter_lang) {
				engine_lang = iter_lang->data;
				if (NULL != engine_lang) {
					/* Default language is STT_BASE_LANGUAGE */
					if (0 == strcmp(STT_BASE_LANGUAGE, engine_lang)) {
						*language = strdup(engine_lang);
						SECURE_SLOG(LOG_DEBUG, stt_tag(), "Selected language : %s", *language);
						return 0;
					}
				}

				iter_lang = g_slist_next(iter_lang);
			}

			/* Not support STT_BASE_LANGUAGE */
			if (NULL != engine_lang) {
				*language = strdup(engine_lang);
				SECURE_SLOG(LOG_DEBUG, stt_tag(), "Selected language : %s", *language);
				return 0;
			}
		}
		break;
	}

	return -1;
}

Eina_Bool stt_config_mgr_inotify_event_cb(void* data, Ecore_Fd_Handler *fd_handler)
{
	SLOG(LOG_DEBUG, stt_tag(), "===== Config changed callback event");

	int length;
	struct inotify_event event;
	memset(&event, '\0', sizeof(struct inotify_event));

	length = read(g_fd_noti, &event, sizeof(struct inotify_event));
	if (0 > length) {
		SLOG(LOG_ERROR, stt_tag(), "[ERROR] Empty Inotify event");
		SLOG(LOG_DEBUG, stt_tag(), "=====");
		SLOG(LOG_DEBUG, stt_tag(), " ");
		return ECORE_CALLBACK_PASS_ON; 
	}

	if (IN_MODIFY == event.mask) {
		char* engine = NULL;
		char* setting = NULL;
		char* lang = NULL;
		int auto_lang = -1;
		int silence = -1;

		GSList *iter = NULL;
		stt_config_client_s* temp_client = NULL;

		if (0 != stt_parser_find_config_changed(&engine, &setting, &auto_lang, &lang, &silence))
			return ECORE_CALLBACK_PASS_ON;

		/* Engine changed */
		if (NULL != engine || NULL != setting) {
			/* engine changed */
			if (NULL != engine) {
				if (NULL != g_config_info->engine_id)	free(g_config_info->engine_id);
				g_config_info->engine_id = strdup(engine);
			}

			if (NULL != setting) {
				if (NULL != g_config_info->setting)	free(g_config_info->setting);
				g_config_info->setting = strdup(setting);
			}

			if (NULL != lang) {
				if (NULL != g_config_info->language)	free(g_config_info->language);
				g_config_info->language = strdup(lang);
			}

			if (-1 != silence)	g_config_info->silence_detection = silence;

			/* Call all callbacks of client*/
			iter = g_slist_nth(g_config_client_list, 0);

			while (NULL != iter) {
				temp_client = iter->data;

				if (NULL != temp_client) {
					if (NULL != temp_client->engine_cb) {
						temp_client->engine_cb(g_config_info->engine_id, g_config_info->setting, g_config_info->language,
							g_config_info->silence_detection, temp_client->user_data);
					}
				}

				iter = g_slist_next(iter);
			}
		}

		if (-1 != auto_lang) {
			g_config_info->auto_lang = auto_lang;
		}

		/* Only language changed */
		if (NULL == engine && NULL != lang) {
			char* before_lang = NULL;
			before_lang = strdup(g_config_info->language);

			if (NULL != lang) {
				if (NULL != g_config_info->language)	free(g_config_info->language);
				g_config_info->language = strdup(lang);
			}

			/* Call all callbacks of client*/
			iter = g_slist_nth(g_config_client_list, 0);

			while (NULL != iter) {
				temp_client = iter->data;

				if (NULL != temp_client) {
					if (NULL != temp_client->lang_cb) {
						temp_client->lang_cb(before_lang, g_config_info->language, temp_client->user_data);
					}
				}

				iter = g_slist_next(iter);
			}

			if (NULL != before_lang) {
				free(before_lang);
			}
		}

		if (-1 != silence) {
			/* silence detection changed */
			g_config_info->silence_detection = silence;

			/* Call all callbacks of client*/
			iter = g_slist_nth(g_config_client_list, 0);

			while (NULL != iter) {
				temp_client = iter->data;

				if (NULL != temp_client) {
					if (NULL != temp_client->bool_cb) {
						temp_client->bool_cb(STT_CONFIG_TYPE_OPTION_SILENCE_DETECTION, silence, temp_client->user_data);
					}
				}

				iter = g_slist_next(iter);
			}
		}

		if (NULL != engine)	free(engine);
		if (NULL != setting)	free(setting);
		if (NULL != lang)	free(lang);
	} else {
		SLOG(LOG_ERROR, stt_tag(), "[ERROR] Undefined event");
	}

	SLOG(LOG_DEBUG, stt_tag(), "=====");
	SLOG(LOG_DEBUG, stt_tag(), " ");

	return ECORE_CALLBACK_PASS_ON;
}

int __stt_config_mgr_register_config_event()
{
	/* get file notification handler */
	int fd;
	int wd;

	fd = inotify_init();
	if (fd < 0) {
		SLOG(LOG_ERROR, stt_tag(), "[ERROR] Fail get inotify fd");
		return -1;
	}
	g_fd_noti = fd;

	wd = inotify_add_watch(fd, STT_CONFIG, IN_MODIFY);
	g_wd_noti = wd;

	g_fd_handler_noti = ecore_main_fd_handler_add(fd, ECORE_FD_READ, (Ecore_Fd_Cb)stt_config_mgr_inotify_event_cb, NULL, NULL, NULL);		
	if (NULL == g_fd_handler_noti) {
		SLOG(LOG_ERROR, stt_tag(), "[ERROR] Fail to get handler_noti");
		return -1;
	}

	return 0;
}

int __stt_config_mgr_unregister_config_event()
{
	/* delete inotify variable */
	ecore_main_fd_handler_del(g_fd_handler_noti);
	inotify_rm_watch(g_fd_noti, g_wd_noti);
	close(g_fd_noti);

	return 0;
}

int __stt_config_set_auto_language()
{
	char* value = NULL;
	value = vconf_get_str(VCONFKEY_VOICE_INPUT_LANGUAGE);
	if (NULL == value) {
		SLOG(LOG_ERROR, stt_tag(), "[Config ERROR] Fail to get voice input language");
		return -1;
	}

	char candidate_lang[6] = {'\0', };

	/* Check auto is on or not */
	if (0 == strncmp(value, "auto", 4)) {
		free(value);
	
		value = vconf_get_str(VCONFKEY_LANGSET);
		if (NULL == value) {
			SLOG(LOG_ERROR, stt_tag(), "[Config ERROR] Fail to get display language");
			return -1;
		}

		strncpy(candidate_lang, value, 5);
		free(value);

		/* Check current language */
		if (0 == strncmp(g_config_info->language, candidate_lang, 5)) {
			SECURE_SLOG(LOG_DEBUG, stt_tag(), "[Config] Language is auto. STT language(%s) is same with display lang", g_config_info->language);
			return 0;
		} else {
			SECURE_SLOG(LOG_DEBUG, stt_tag(), "[Config] Display language : %s", candidate_lang);
		}
	} else {
		strncpy(candidate_lang, value, 5);
		SECURE_SLOG(LOG_DEBUG, stt_tag(), "[Config] Voice input language is NOT auto. Voice input language : %s", candidate_lang);

		free(value);
	}

	if (true == __stt_config_mgr_check_lang_is_valid(g_config_info->engine_id, candidate_lang)) {
		/* stt default language change */
		if (NULL == g_config_info->language) {
			SLOG(LOG_ERROR, stt_tag(), "Current config language is NULL");
			return -1;
		}

		char* before_lang = NULL;
		if (0 != stt_parser_set_language(candidate_lang)) {
			SLOG(LOG_ERROR, stt_tag(), "Fail to save default language");
			return -1;
		}

		before_lang = strdup(g_config_info->language);

		free(g_config_info->language);
		g_config_info->language = strdup(candidate_lang);

		SECURE_SLOG(LOG_DEBUG, stt_tag(), "[Config] Language is auto. Set default language(%s)", g_config_info->language);

		/* Call all callbacks of client*/
		GSList *iter = NULL;
		stt_config_client_s* temp_client = NULL;

		iter = g_slist_nth(g_config_client_list, 0);

		while (NULL != iter) {
			temp_client = iter->data;

			if (NULL != temp_client) {
				if (NULL != temp_client->lang_cb) {
					temp_client->lang_cb(before_lang, g_config_info->language, temp_client->user_data);
				}
			}

			iter = g_slist_next(iter);
		}

		if (NULL != before_lang) {
			free(before_lang);
		}
	} else {
		/* Candidate language is not valid */
		char* tmp_language = NULL;
		if (0 != __stt_config_mgr_select_lang(g_config_info->engine_id, &tmp_language)) {
			SLOG(LOG_ERROR, stt_tag(), "[ERROR] Fail to select language");
			return -1;
		}

		if (NULL == tmp_language) {
			SLOG(LOG_ERROR, stt_tag(), "[ERROR] Selected language is NULL");
			return -1;
		}

		if (0 != stt_parser_set_language(tmp_language)) {
			free(tmp_language);
			SLOG(LOG_ERROR, stt_tag(), "[ERROR] Fail to save config");
			return -1;
		}

		SECURE_SLOG(LOG_DEBUG, stt_tag(), "[Config] Language is auto but display lang is not supported. Default language change(%s)", tmp_language);

		/* Call all callbacks of client*/
		GSList *iter = NULL;
		stt_config_client_s* temp_client = NULL;

		iter = g_slist_nth(g_config_client_list, 0);

		while (NULL != iter) {
			temp_client = iter->data;

			if (NULL != temp_client) {
				if (NULL != temp_client->lang_cb) {
					temp_client->lang_cb(g_config_info->language, tmp_language, temp_client->user_data);
				}
			}

			iter = g_slist_next(iter);
		}

		if (NULL != g_config_info->language) {
			free(g_config_info->language);
			g_config_info->language = strdup(tmp_language);
		}

		free(tmp_language);
	}

	return 0;
}

void __stt_config_language_changed_cb(keynode_t *key, void *data)
{
	if (true == g_config_info->auto_lang) {
		/* Get voice input vconf key */
		__stt_config_set_auto_language();
	}

	return;
}

int stt_config_mgr_initialize(int uid)
{
	GSList *iter = NULL;
	int* get_uid;
	stt_config_client_s* temp_client = NULL;

	if (0 < g_slist_length(g_config_client_list)) {
		/* Check uid */
		iter = g_slist_nth(g_config_client_list, 0);

		while (NULL != iter) {
			get_uid = iter->data;

			if (uid == *get_uid) {
				SECURE_SLOG(LOG_WARN, stt_tag(), "[CONFIG] uid(%d) has already registered", uid);
				return 0;
			}

			iter = g_slist_next(iter);
		}

		temp_client = (stt_config_client_s*)calloc(1, sizeof(stt_config_client_s));
		temp_client->uid = uid;
		temp_client->bool_cb = NULL;
		temp_client->engine_cb = NULL;
		temp_client->lang_cb = NULL;
		temp_client->user_data = NULL;

		/* Add uid */
		g_config_client_list = g_slist_append(g_config_client_list, temp_client);

		SECURE_SLOG(LOG_WARN, stt_tag(), "[CONFIG] Add uid(%d) but config has already initialized", uid);
		return STT_CONFIG_ERROR_NONE;
	}

	/* Get file name from default engine directory */
	DIR *dp = NULL;
	int ret = -1;
	struct dirent entry;
	struct dirent *dirp = NULL;

	g_engine_list = NULL;

	dp  = opendir(STT_DEFAULT_ENGINE_INFO);
	if (NULL != dp) {
		do {
			ret = readdir_r(dp, &entry, &dirp);
			if (0 != ret) {
				SLOG(LOG_ERROR, stt_tag(), "[File ERROR] Fail to read directory");
				break;
			}

			if (NULL != dirp) {
				if (!strcmp(".", dirp->d_name) || !strcmp("..", dirp->d_name))
					continue;

				stt_engine_info_s* info;
				char* filepath;
				int filesize;

				filesize = strlen(STT_DEFAULT_ENGINE_INFO) + strlen(dirp->d_name) + 5;
				filepath = (char*)calloc(filesize, sizeof(char));

				if (NULL != filepath) {
					snprintf(filepath, filesize, "%s/%s", STT_DEFAULT_ENGINE_INFO, dirp->d_name);
				} else {
					SLOG(LOG_ERROR, stt_tag(), "[Config ERROR] Memory not enough!!");
					continue;
				}

				if (0 == stt_parser_get_engine_info(filepath, &info)) {
					g_engine_list = g_slist_append(g_engine_list, info);
				}

				if (NULL != filepath)
					free(filepath);
			}
		} while (NULL != dirp);

		closedir(dp);
	} else {
		SLOG(LOG_WARN, stt_tag(), "[Config WARNING] Fail to open default directory"); 
	}

	__stt_config_mgr_print_engine_info();

	if (0 != stt_parser_load_config(&g_config_info)) {
		SLOG(LOG_ERROR, stt_tag(), "[ERROR] Fail to parse configure information");
		return -1; 
	}

	if (true == g_config_info->auto_lang) {
		/* Check language with display language */
		__stt_config_set_auto_language();
	} else {
		if (false == __stt_config_mgr_check_lang_is_valid(g_config_info->engine_id, g_config_info->language)) {
			/* Default language is not valid */
			char* tmp_language;
			if (0 != __stt_config_mgr_select_lang(g_config_info->engine_id, &tmp_language)) {
				SLOG(LOG_ERROR, stt_tag(), "[ERROR] Fail to select language");
				return -1;
			}

			if (NULL != tmp_language) {
				if (NULL != g_config_info->language) {
					free(g_config_info->language);
					g_config_info->language = strdup(tmp_language);
				}

				free(tmp_language);
			}
		}
	}

	/* print daemon config */
	SLOG(LOG_DEBUG, stt_tag(), "== Daemon config ==");
	SECURE_SLOG(LOG_DEBUG, stt_tag(), " engine : %s", g_config_info->engine_id);
	SECURE_SLOG(LOG_DEBUG, stt_tag(), " setting : %s", g_config_info->setting);
	SECURE_SLOG(LOG_DEBUG, stt_tag(), " auto language : %s", g_config_info->auto_lang ? "on" : "off");
	SECURE_SLOG(LOG_DEBUG, stt_tag(), " language : %s", g_config_info->language);
	SECURE_SLOG(LOG_DEBUG, stt_tag(), " silence detection : %s", g_config_info->silence_detection ? "on" : "off");
	SLOG(LOG_DEBUG, stt_tag(), "===================");

	if (0 != __stt_config_mgr_register_config_event()) {
		SLOG(LOG_ERROR, stt_tag(), "[ERROR] Fail to register config event");
		return STT_CONFIG_ERROR_OPERATION_FAILED;
	}

	/* Register to detect display language change */
	vconf_notify_key_changed(VCONFKEY_LANGSET, __stt_config_language_changed_cb, NULL);
	vconf_notify_key_changed(VCONFKEY_VOICE_INPUT_LANGUAGE, __stt_config_language_changed_cb, NULL);

	temp_client = (stt_config_client_s*)calloc(1, sizeof(stt_config_client_s));
	temp_client->uid = uid;
	temp_client->bool_cb = NULL;
	temp_client->engine_cb = NULL;
	temp_client->lang_cb = NULL;
	temp_client->user_data = NULL;

	/* Add uid */
	g_config_client_list = g_slist_append(g_config_client_list, temp_client);

	return STT_CONFIG_ERROR_NONE;
}

int stt_config_mgr_finalize(int uid)
{
	GSList *iter = NULL;
	stt_config_client_s* temp_client = NULL;

	if (0 < g_slist_length(g_config_client_list)) {
		/* Check uid */
		iter = g_slist_nth(g_config_client_list, 0);

		while (NULL != iter) {
			temp_client = iter->data;

			if (NULL != temp_client) {
				if (uid == temp_client->uid) {
					g_config_client_list = g_slist_remove(g_config_client_list, temp_client);
					free(temp_client);
					temp_client = NULL;
					break;
				}
			}

			iter = g_slist_next(iter);
		}
	}

	if (0 < g_slist_length(g_config_client_list)) {
		SLOG(LOG_DEBUG, stt_tag(), "Client count (%d)", g_slist_length(g_config_client_list));
		return STT_CONFIG_ERROR_NONE;
	}

	stt_engine_info_s *engine_info = NULL;

	if (0 < g_slist_length(g_engine_list)) {

		/* Get a first item */
		iter = g_slist_nth(g_engine_list, 0);

		while (NULL != iter) {
			engine_info = iter->data;

			if (NULL != engine_info) {
				g_engine_list = g_slist_remove(g_engine_list, engine_info);

				stt_parser_free_engine_info(engine_info);
			}

			iter = g_slist_nth(g_engine_list, 0);
		}
	}

	vconf_ignore_key_changed(VCONFKEY_LANGSET, __stt_config_language_changed_cb);
	vconf_ignore_key_changed(VCONFKEY_VOICE_INPUT_LANGUAGE, __stt_config_language_changed_cb);

	__stt_config_mgr_unregister_config_event();

	if (NULL != g_config_info) {
		stt_parser_unload_config(g_config_info);
		g_config_info = NULL;
	}

	return STT_CONFIG_ERROR_NONE;
}

int stt_config_mgr_set_callback(int uid, stt_config_engine_changed_cb engine_cb, stt_config_lang_changed_cb lang_cb, stt_config_bool_changed_cb bool_cb, void* user_data)
{
	GSList *iter = NULL;
	stt_config_client_s* temp_client = NULL;

	/* Call all callbacks of client*/
	iter = g_slist_nth(g_config_client_list, 0);

	while (NULL != iter) {
		temp_client = iter->data;

		if (NULL != temp_client) {
			if (uid == temp_client->uid) {
				temp_client->engine_cb = engine_cb;
				temp_client->lang_cb = lang_cb;
				temp_client->bool_cb = bool_cb;
				temp_client->user_data = user_data;
			}
		}

		iter = g_slist_next(iter);
	}

	return STT_CONFIG_ERROR_NONE;
}

int stt_config_mgr_unset_callback(int uid)
{	
	GSList *iter = NULL;
	stt_config_client_s* temp_client = NULL;

	/* Call all callbacks of client*/
	iter = g_slist_nth(g_config_client_list, 0);

	while (NULL != iter) {
		temp_client = iter->data;

		if (NULL != temp_client) {
			if (uid == temp_client->uid) {
				temp_client->engine_cb = NULL;
				temp_client->lang_cb = NULL;
				temp_client->bool_cb = NULL;
				temp_client->user_data = NULL;
			}
		}

		iter = g_slist_next(iter);
	}

	return STT_CONFIG_ERROR_NONE;
}

int stt_config_mgr_get_engine_list(stt_config_supported_engine_cb callback, void* user_data)
{
	if (0 >= g_slist_length(g_config_client_list)) {
		SLOG(LOG_ERROR, stt_tag(), "Not initialized");
		return STT_CONFIG_ERROR_INVALID_STATE;
	}

	if (NULL == callback) {
		return STT_CONFIG_ERROR_INVALID_PARAMETER;
	}

	GSList *iter = NULL;
	stt_engine_info_s *engine_info = NULL;

	if (0 >= g_slist_length(g_engine_list)) {
		SLOG(LOG_WARN, stt_tag(), "[ERROR] Engine list is NULL");
		return STT_CONFIG_ERROR_ENGINE_NOT_FOUND;
	}

	/* Get a first item */
	iter = g_slist_nth(g_engine_list, 0);

	while (NULL != iter) {
		engine_info = iter->data;

		if (NULL == engine_info) {
			SLOG(LOG_ERROR, stt_tag(), " Engine info is NULL");
			return STT_CONFIG_ERROR_OPERATION_FAILED;
		}

		if (false == callback(engine_info->uuid, engine_info->name, 
			engine_info->setting, engine_info->support_silence_detection, user_data)) {
			break;
		}
		
		iter = g_slist_next(iter);
	}

	return STT_CONFIG_ERROR_NONE;
}

int stt_config_mgr_get_engine(char** engine)
{
	if (0 >= g_slist_length(g_config_client_list)) {
		SLOG(LOG_ERROR, stt_tag(), "Not initialized");
		return STT_CONFIG_ERROR_INVALID_STATE;
	}

	if (NULL == engine) {
		return STT_CONFIG_ERROR_INVALID_PARAMETER;
	}

	if (NULL != g_config_info->engine_id) {
		*engine = strdup(g_config_info->engine_id);
	} else {
		SLOG(LOG_ERROR, stt_tag(), " Engine id is NULL");
		return STT_CONFIG_ERROR_ENGINE_NOT_FOUND;
	}

	return STT_CONFIG_ERROR_NONE;
}

int stt_config_mgr_set_engine(const char* engine)
{
	if (0 >= g_slist_length(g_config_client_list)) {
		SLOG(LOG_ERROR, stt_tag(), "Not initialized");
		return STT_CONFIG_ERROR_INVALID_STATE;
	}

	if (NULL == g_config_info) {
		return STT_CONFIG_ERROR_OPERATION_FAILED;
	}

	if (NULL == engine || NULL == g_config_info->engine_id) {
		return STT_CONFIG_ERROR_INVALID_PARAMETER;
	}

	/* Check current engine id with new engine id */
	if (0 == strcmp(g_config_info->engine_id, engine)) {
		return 0;
	}

	SECURE_SLOG(LOG_DEBUG, stt_tag(), "New engine id : %s", engine);

	GSList *iter = NULL;
	stt_engine_info_s *engine_info = NULL;
	bool is_valid_engine = false;

	/* Get a first item */
	iter = g_slist_nth(g_engine_list, 0);

	while (NULL != iter) {
		engine_info = iter->data;

		if (NULL == engine_info) {
			SLOG(LOG_ERROR, stt_tag(), "[ERROR] Engine info is NULL");
			iter = g_slist_next(iter);
			continue;
		}

		/* Check engine id is valid */
		if (0 != strcmp(engine, engine_info->uuid)) {
			iter = g_slist_next(iter);
			continue;
		}

		if (NULL != g_config_info->engine_id)
			free(g_config_info->engine_id);

		g_config_info->engine_id = strdup(engine);

		if (NULL != g_config_info->setting) {
			free(g_config_info->setting);
		}

		if (NULL != engine_info->setting) {
			g_config_info->setting = strdup(engine_info->setting);
		} else {
			g_config_info->setting = NULL;
		}

		/* Engine is valid*/
		GSList *iter_lang = NULL;
		char* lang;
		bool is_valid_lang = false;

		/* Get a first item */
		iter_lang = g_slist_nth(engine_info->languages, 0);

		while (NULL != iter_lang) {
			/*Get handle data from list*/
			lang = iter_lang->data;

			SECURE_SLOG(LOG_DEBUG, stt_tag(), " %s", lang);
			if (NULL != lang) {
				if (0 == strcmp(lang, g_config_info->language)) {
					/* language is valid */
					is_valid_lang = true;

					if (NULL != g_config_info->language) {
						free(g_config_info->language);

						g_config_info->language = strdup(lang);
					}
					break;
				}
			}

			/*Get next item*/
			iter_lang = g_slist_next(iter_lang);
		}

		if (false == is_valid_lang) {
			if (NULL != g_config_info->language) {
				free(g_config_info->language);

				iter_lang = g_slist_nth(engine_info->languages, 0);
				lang = iter_lang->data;

				g_config_info->language = strdup(lang);
			}
		}

		/* Check options */
		if (false == engine_info->support_silence_detection) {
			if (true == g_config_info->silence_detection)
				g_config_info->silence_detection = false;
		}

		is_valid_engine = true;
		break;
	}

	if (true == is_valid_engine) {
		SLOG(LOG_DEBUG, stt_tag(), "[Config] Engine changed");
		SECURE_SLOG(LOG_DEBUG, stt_tag(), "  Engine : %s", g_config_info->engine_id);
		SECURE_SLOG(LOG_DEBUG, stt_tag(), "  Setting : %s", g_config_info->setting);
		SECURE_SLOG(LOG_DEBUG, stt_tag(), "  language : %s", g_config_info->language);
		SECURE_SLOG(LOG_DEBUG, stt_tag(), "  Silence detection : %s", g_config_info->silence_detection ? "on" : "off");

		if ( 0 != stt_parser_set_engine(g_config_info->engine_id, g_config_info->setting, g_config_info->language,
			g_config_info->silence_detection)) {
				SLOG(LOG_ERROR, stt_tag(), " Fail to save config");
				return STT_CONFIG_ERROR_OPERATION_FAILED;
		}
	} else {
		SLOG(LOG_ERROR, stt_tag(), "[ERROR] Engine id is not valid");
		return STT_CONFIG_ERROR_INVALID_PARAMETER;
	}

	return 0;
}

int stt_config_mgr_get_engine_agreement(const char* engine, char** agreement)
{
	if (0 >= g_slist_length(g_engine_list)) {
		SLOG(LOG_ERROR, stt_tag(), "There is no engine");
		return STT_CONFIG_ERROR_INVALID_STATE;
	}

	if (NULL == agreement) {
		SLOG(LOG_ERROR, stt_tag(), "Input parameter is NULL");
		return STT_CONFIG_ERROR_INVALID_PARAMETER;
	}

	GSList *iter = NULL;
	stt_engine_info_s *engine_info = NULL;
	char* current_engine = NULL;

	if (NULL == engine) {
		current_engine = strdup(g_config_info->engine_id);
	} else {
		current_engine = strdup(engine);
	}

	/* Get a first item */
	iter = g_slist_nth(g_engine_list, 0);

	while (NULL != iter) {
		engine_info = iter->data;

		if (NULL == engine_info) {
			SLOG(LOG_ERROR, stt_tag(), "[ERROR] engine info is NULL");
			if (NULL != current_engine)	free(current_engine);
			return STT_CONFIG_ERROR_OPERATION_FAILED;
		}

		if (0 != strcmp(current_engine, engine_info->uuid)) {
			iter = g_slist_next(iter);
			continue;
		}

		if (NULL != engine_info->agreement) {
			*agreement = strdup(engine_info->agreement);
		} else {
			SLOG(LOG_WARN, stt_tag(), "[WARNING] engine agreement is not support");
		}
		break;
	}

	if (NULL != current_engine)	free(current_engine);

	return STT_CONFIG_ERROR_NONE;
}

int stt_config_mgr_get_language_list(const char* engine_id, stt_config_supported_langauge_cb callback, void* user_data)
{
	if (0 >= g_slist_length(g_config_client_list)) {
		SLOG(LOG_ERROR, stt_tag(), "Not initialized");
		return STT_CONFIG_ERROR_INVALID_STATE;
	}
	
	if (0 >= g_slist_length(g_engine_list)) {
		SLOG(LOG_ERROR, stt_tag(), "There is no engine");
		return STT_CONFIG_ERROR_ENGINE_NOT_FOUND;
	}

	GSList *iter = NULL;
	stt_engine_info_s *engine_info = NULL;

	/* Get a first item */
	iter = g_slist_nth(g_engine_list, 0);

	while (NULL != iter) {
		engine_info = iter->data;

		if (NULL == engine_info) {
			SLOG(LOG_ERROR, stt_tag(), "[ERROR] engine info is NULL");
			return STT_CONFIG_ERROR_OPERATION_FAILED;
		}

		if (0 != strcmp(engine_id, engine_info->uuid)) {
			iter = g_slist_next(iter);
			continue;
		}

		GSList *iter_lang = NULL;
		char* lang;
		
		/* Get a first item */
		iter_lang = g_slist_nth(engine_info->languages, 0);

		while (NULL != iter_lang) {
			/*Get handle data from list*/
			lang = iter_lang->data;

			SECURE_SLOG(LOG_DEBUG, stt_tag(), " %s", lang);
			if (NULL != lang) {
				if (false == callback(engine_info->uuid, lang, user_data))
					break;
			}
			
			/*Get next item*/
			iter_lang = g_slist_next(iter_lang);
		}
		break;
	}

	return STT_CONFIG_ERROR_NONE;
}

int stt_config_mgr_get_default_language(char** language)
{
	if (0 >= g_slist_length(g_config_client_list)) {
		SLOG(LOG_ERROR, stt_tag(), "Not initialized");
		return STT_CONFIG_ERROR_INVALID_STATE;
	}

	if (NULL == language) {
		return STT_CONFIG_ERROR_INVALID_PARAMETER;
	}

	if (NULL != g_config_info->language) {
		*language = strdup(g_config_info->language);
	} else {
		SLOG(LOG_ERROR, stt_tag(), " language is NULL");
		return STT_CONFIG_ERROR_OPERATION_FAILED;
	}

	return STT_CONFIG_ERROR_NONE;
}

int stt_config_mgr_set_default_language(const char* language)
{
	if (0 >= g_slist_length(g_config_client_list)) {
		SLOG(LOG_ERROR, stt_tag(), "Not initialized");
		return STT_CONFIG_ERROR_INVALID_STATE;
	}

	if (NULL == language) {
		return STT_CONFIG_ERROR_INVALID_PARAMETER;
	}

	/* Check language is valid */
	if (NULL != g_config_info->language) {
		if (0 != stt_parser_set_language(language)) {
			SLOG(LOG_ERROR, stt_tag(), "Fail to save engine id");
			return STT_CONFIG_ERROR_OPERATION_FAILED;
		}
		free(g_config_info->language);
		g_config_info->language = strdup(language);
	} else {
		SLOG(LOG_ERROR, stt_tag(), " language is NULL");
		return STT_CONFIG_ERROR_OPERATION_FAILED;
	}

	return STT_CONFIG_ERROR_NONE;
}

int stt_config_mgr_get_auto_language(bool* value)
{
	if (0 >= g_slist_length(g_config_client_list)) {
		SLOG(LOG_ERROR, stt_tag(), "Not initialized");
		return STT_CONFIG_ERROR_INVALID_STATE;
	}

	if (NULL == value) {
		return STT_CONFIG_ERROR_INVALID_PARAMETER;
	}

	*value = g_config_info->auto_lang;

	return STT_CONFIG_ERROR_NONE;
}

int stt_config_mgr_set_auto_language(bool value)
{
	if (0 >= g_slist_length(g_config_client_list)) {
		SLOG(LOG_ERROR, stt_tag(), "Not initialized");
		return STT_CONFIG_ERROR_INVALID_STATE;
	}

	if (g_config_info->auto_lang != value) {
		/* Check language is valid */
		if (0 != stt_parser_set_auto_lang(value)) {
			SLOG(LOG_ERROR, stt_tag(), "Fail to save engine id");
			return STT_CONFIG_ERROR_OPERATION_FAILED;
		}
		g_config_info->auto_lang = value;

		if (true == g_config_info->auto_lang) {
			__stt_config_set_auto_language();
		}
	}

	return STT_CONFIG_ERROR_NONE;
}

int stt_config_mgr_get_silence_detection(bool* value)
{
	if (0 >= g_slist_length(g_config_client_list)) {
		SLOG(LOG_ERROR, stt_tag(), "Not initialized");
		return STT_CONFIG_ERROR_INVALID_STATE;
	}

	if (NULL == value)
		return STT_CONFIG_ERROR_INVALID_PARAMETER;

	*value = g_config_info->silence_detection;

	return STT_CONFIG_ERROR_NONE;
}

int stt_config_mgr_set_silence_detection(bool value)
{
	if (0 >= g_slist_length(g_config_client_list)) {
		SLOG(LOG_ERROR, stt_tag(), "Not initialized");
		return STT_CONFIG_ERROR_INVALID_STATE;
	}

	if (0 != stt_parser_set_silence_detection(value)) {
		SLOG(LOG_ERROR, stt_tag(), "Fail to save engine id");
		return STT_CONFIG_ERROR_OPERATION_FAILED;
	}

	g_config_info->silence_detection = value;

	return STT_CONFIG_ERROR_NONE;
}

bool stt_config_check_default_engine_is_valid(const char* engine)
{
	if (0 >= g_slist_length(g_config_client_list)) {
		SLOG(LOG_ERROR, stt_tag(), "Not initialized");
		return false;
	}

	if (NULL == engine) {
		return false;
	}

	if (0 >= g_slist_length(g_engine_list)) 
		return false;

	GSList *iter = NULL;
	stt_engine_info_s *engine_info = NULL;

	/* Get a first item */
	iter = g_slist_nth(g_engine_list, 0);

	while (NULL != iter) {
		engine_info = iter->data;

		if (NULL != engine_info) {
			if (0 == strcmp(engine, engine_info->uuid)) {
				return true;
			}
		}
		iter = g_slist_next(iter);
	}

	return false;
}

bool stt_config_check_default_language_is_valid(const char* language)
{
	if (0 >= g_slist_length(g_config_client_list)) {
		SLOG(LOG_ERROR, stt_tag(), "Not initialized");
		return false;
	}

	if (NULL == language) {
		return false;
	}

	if (NULL == g_config_info->engine_id) {
		SLOG(LOG_ERROR, stt_tag(), "[ERROR] Default engine id is NULL");
		return false;
	}

	if (0 >= g_slist_length(g_engine_list)) 
		return false;

	GSList *iter = NULL;
	stt_engine_info_s *engine_info = NULL;

	/* Get a first item */
	iter = g_slist_nth(g_engine_list, 0);

	while (NULL != iter) {
		engine_info = iter->data;

		if (NULL == engine_info) {
			SLOG(LOG_ERROR, stt_tag(), "[ERROR] Engine info is NULL");
			iter = g_slist_next(iter);
			continue;
		}
		
		if (0 != strcmp(g_config_info->engine_id, engine_info->uuid)) {
			iter = g_slist_next(iter);
			continue;
		}

		GSList *iter_lang = NULL;
		char* lang;

		/* Get a first item */
		iter_lang = g_slist_nth(engine_info->languages, 0);

		while (NULL != iter_lang) {
			lang = iter_lang->data;
			
			if (0 == strcmp(language, lang))
				return true;

			/*Get next item*/
			iter_lang = g_slist_next(iter_lang);
		}
		break;
	}

	return false;
}

int __stt_config_mgr_print_engine_info()
{
	GSList *iter = NULL;
	stt_engine_info_s *engine_info = NULL;

	if (0 >= g_slist_length(g_engine_list)) {
		SLOG(LOG_DEBUG, stt_tag(), "-------------- engine list -----------------");
		SLOG(LOG_DEBUG, stt_tag(), "  No Engine in engine directory");
		SLOG(LOG_DEBUG, stt_tag(), "--------------------------------------------");
		return 0;
	}

	/* Get a first item */
	iter = g_slist_nth(g_engine_list, 0);

	SLOG(LOG_DEBUG, stt_tag(), "--------------- engine list -----------------");

	int i = 1;	
	while (NULL != iter) {
		engine_info = iter->data;

		SECURE_SLOG(LOG_DEBUG, stt_tag(), "[%dth]", i);
		SECURE_SLOG(LOG_DEBUG, stt_tag(), " name : %s", engine_info->name);
		SECURE_SLOG(LOG_DEBUG, stt_tag(), " id   : %s", engine_info->uuid);
		SECURE_SLOG(LOG_DEBUG, stt_tag(), " setting : %s", engine_info->setting);
		SECURE_SLOG(LOG_DEBUG, stt_tag(), " agreement : %s", engine_info->agreement);

		SLOG(LOG_DEBUG, stt_tag(), " languages");
		GSList *iter_lang = NULL;
		char* lang;
		if (g_slist_length(engine_info->languages) > 0) {
			/* Get a first item */
			iter_lang = g_slist_nth(engine_info->languages, 0);

			int j = 1;	
			while (NULL != iter_lang) {
				/*Get handle data from list*/
				lang = iter_lang->data;

				SECURE_SLOG(LOG_DEBUG, stt_tag(), "  [%dth] %s", j, lang);

				/*Get next item*/
				iter_lang = g_slist_next(iter_lang);
				j++;
			}
		} else {
			SLOG(LOG_ERROR, stt_tag(), "  language is NONE");
		}
		SECURE_SLOG(LOG_DEBUG, stt_tag(), " silence support : %s", 
			engine_info->support_silence_detection ? "true" : "false");
		iter = g_slist_next(iter);
		i++;
	}
	SLOG(LOG_DEBUG, stt_tag(), "--------------------------------------------");

	return 0;
}



/**
* time info functions
*/
static GSList* g_time_list = NULL;

int stt_config_mgr_reset_time_info()
{
	/* Remove time info */
	GSList *iter = NULL;
	stt_result_time_info_s *data = NULL;

	/* Remove time info */
	iter = g_slist_nth(g_time_list, 0);
	while (NULL != iter) {
		data = iter->data;

		g_time_list = g_slist_remove(g_time_list, data);
		if (NULL != data) {
			if (NULL == data->text)	free(data->text);
			free(data);
		}

		/*Get next item*/
		iter = g_slist_nth(g_time_list, 0);
	}

	return 0;
}

int stt_config_mgr_add_time_info(int index, int event, const char* text, long start_time, long end_time)
{
	if (NULL == text) {
		SLOG(LOG_ERROR, stt_tag(), "Invalid paramter : text is NULL");
		return -1;
	}

	stt_result_time_info_s *info = (stt_result_time_info_s*)calloc(1, sizeof(stt_result_time_info_s));

	info->index = index;
	info->event = event;
	if (NULL != text) {
		info->text = strdup(text);
	}
	info->start_time = start_time;
	info->end_time = end_time;

	/* Add item to global list */
	g_time_list = g_slist_append(g_time_list, info);

	return 0;
}

int stt_config_mgr_foreach_time_info(stt_config_result_time_cb callback, void* user_data)
{
	if (NULL == callback) {
		SLOG(LOG_ERROR, stt_tag(), "[ERROR] Input paramter is NULL : callback function");
		return STT_CONFIG_ERROR_INVALID_PARAMETER;
	}

	GSList* temp_time = NULL;
	int ret;
	ret = stt_parser_get_time_info(&temp_time);
	if (0 != ret) {
		SLOG(LOG_WARN, stt_tag(), "[WARNING] Fail to get time info : %d", ret);
		return STT_CONFIG_ERROR_OPERATION_FAILED;
	}

	GSList *iter = NULL;
	stt_result_time_info_s *data = NULL;

	/* Get a first item */
	iter = g_slist_nth(temp_time, 0);
	while (NULL != iter) {
		data = iter->data;

		if (false == callback(data->index, data->event, data->text, 
			data->start_time, data->end_time, user_data)) {
			break;
		}

		/*Get next item*/
		iter = g_slist_next(iter);
	}

	/* Remove time info */
	iter = g_slist_nth(temp_time, 0);
	while (NULL != iter) {
		data = iter->data;

		if (NULL != data) {
			temp_time = g_slist_remove(temp_time, data);

			if (NULL == data->text)	free(data->text);
			free(data);
		}

		/*Get next item*/
		iter = g_slist_nth(temp_time, 0);
	}

	return STT_CONFIG_ERROR_NONE;
}

int stt_config_mgr_save_time_info_file()
{
	if (0 == g_slist_length(g_time_list)) {
		SLOG(LOG_WARN, stt_tag(), "[WARNING] There is no time info to save");
		return STT_CONFIG_ERROR_OPERATION_FAILED;
	}

	int ret = 0;
	ret = stt_parser_set_time_info(g_time_list);
	if (0 != ret) {
		SLOG(LOG_ERROR, stt_tag(), "[ERROR] Fail to save time info : %d", ret);
		return STT_CONFIG_ERROR_OPERATION_FAILED;
	}

	return STT_CONFIG_ERROR_NONE;
}

int stt_config_mgr_remove_time_info_file()
{
	stt_parser_clear_time_info();

	return STT_CONFIG_ERROR_NONE;
}