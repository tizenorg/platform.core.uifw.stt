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

#include <Ecore_File.h>
#include "sttd_main.h"
#include "sttd_config.h"


#define CONFIG_FILE_PATH	CONFIG_DIRECTORY"/sttd.conf"
#define CONFIG_DEFAULT		BASE_DIRECTORY_DEFAULT"/sttd.conf"

#define ENGINE_ID	"ENGINE_ID"
#define LANGUAGE	"LANGUAGE"
#define SILENCE		"SILENCE"
#define PROFANITY	"PROFANITY"
#define PUNCTUATION	"PUNCTUATION"


static char*	g_engine_id;
static char*	g_language;
static int	g_silence;
static int	g_profanity;
static int	g_punctuation;

int __sttd_config_save()
{
	if (0 != access(CONFIG_FILE_PATH, R_OK|W_OK)) {
		if (0 == ecore_file_mkpath(CONFIG_DIRECTORY)) {
			SLOG(LOG_ERROR, TAG_STTD, "[Config ERROR ] Fail to create directory (%s)", CONFIG_DIRECTORY);
			return -1;
		}

		SLOG(LOG_WARN, TAG_STTD, "[Config] Create directory (%s)", CONFIG_DIRECTORY);
	}

	FILE* config_fp;
	config_fp = fopen(CONFIG_FILE_PATH, "w+");

	if (NULL == config_fp) {
		// make file and file default
		SLOG(LOG_ERROR, TAG_STTD, "[Config ERROR] Fail to load config (engine id)");
		return -1;
	}

	SLOG(LOG_DEBUG, TAG_STTD, "[Config] Rewrite config file");

	/* Write engine id */
	fprintf(config_fp, "%s %s\n", ENGINE_ID, g_engine_id);

	/* Write language */
	fprintf(config_fp, "%s %s\n", LANGUAGE, g_language);

	/* Write silence detection */
	fprintf(config_fp, "%s %d\n", SILENCE, g_silence);

	/* Write profanity */
	fprintf(config_fp, "%s %d\n", PROFANITY, g_profanity);

	/* Write punctuation */
	fprintf(config_fp, "%s %d\n", PUNCTUATION, g_punctuation);

	fclose(config_fp);

	return 0;
}

int __sttd_config_load()
{
	FILE* config_fp;
	char buf_id[256] = {0};
	char buf_param[256] = {0};
	int int_param = 0;
	bool is_default_open = false;

	config_fp = fopen(CONFIG_FILE_PATH, "r");

	if (NULL == config_fp) {
		SLOG(LOG_WARN, TAG_STTD, "[Config WARNING] Not open file(%s)", CONFIG_FILE_PATH);

		config_fp = fopen(CONFIG_DEFAULT, "r");
		if (NULL == config_fp) {
			SLOG(LOG_WARN, TAG_STTD, "[Config WARNING] Not open original config file(%s)", CONFIG_FILE_PATH);
			__sttd_config_save();
			return 0;
		}
		is_default_open = true;
	}

	/* Read engine id */
	if (EOF == fscanf(config_fp, "%s %s", buf_id, buf_param)) {
		fclose(config_fp);
		SLOG(LOG_WARN, TAG_STTD, "[Config WARNING] Fail to load config (engine id)");
		__sttd_config_save();
		return 0;
	} else {
		if (0 == strncmp(ENGINE_ID, buf_id, strlen(ENGINE_ID))) {
			g_engine_id = strdup(buf_param);
		} else {
			fclose(config_fp);
			SLOG(LOG_WARN, TAG_STTD, "[Config WARNING] Fail to load config (engine id)");
			__sttd_config_save();
			return 0;
		}
	}

	/* Read language */
	if (EOF == fscanf(config_fp, "%s %s", buf_id, buf_param)) {
		fclose(config_fp);
		SLOG(LOG_WARN, TAG_STTD, "[Config WARNING] Fail to load config (language)");
		__sttd_config_save();
		return 0;
	} else {
		if (0 == strncmp(LANGUAGE, buf_id, strlen(LANGUAGE))) {
			g_language = strdup(buf_param);
		} else {
			fclose(config_fp);
			SLOG(LOG_WARN, TAG_STTD, "[Config WARNING] Fail to load config (language)");
			__sttd_config_save();
			return 0;
		}
	}
	
	/* Read silence detection */
	if (EOF == fscanf(config_fp, "%s %d", buf_id, &int_param)) {
		fclose(config_fp);
		SLOG(LOG_WARN, TAG_STTD, "[Config WARNING] Fail to load config (silence)");
		__sttd_config_save();
		return 0;
	} else {
		if (0 == strncmp(SILENCE, buf_id, strlen(SILENCE))) {
			g_silence = int_param;
		} else {
			fclose(config_fp);
			SLOG(LOG_WARN, TAG_STTD, "[Config WARNING] Fail to load config (silence)");
			__sttd_config_save();
			return 0;
		}
	}

	/* Read profanity filter */
	if (EOF == fscanf(config_fp, "%s %d", buf_id, &int_param)) {
		fclose(config_fp);
		SLOG(LOG_WARN, TAG_STTD, "[Config WARNING] Fail to load config (profanity filter)");
		__sttd_config_save();
		return 0;
	} else {
		if (0 == strncmp(PROFANITY, buf_id, strlen(PROFANITY))) {
			g_profanity = int_param;
		} else {
			fclose(config_fp);
			SLOG(LOG_WARN, TAG_STTD, "[Config WARNING] Fail to load config (profanity filter)");
			__sttd_config_save();
			return 0;
		}
	}
	

	/* Read punctuation override */
	if (EOF == fscanf(config_fp, "%s %d", buf_id, &int_param)) {
		fclose(config_fp);
		SLOG(LOG_WARN, TAG_STTD, "[Config WARNING] Fail to load config (punctuation override)");
		__sttd_config_save();
		return 0;
	} else {
		if (0 == strncmp(PUNCTUATION, buf_id, strlen(PUNCTUATION))) {
			g_punctuation = int_param;
		} else {
			fclose(config_fp);
			SLOG(LOG_WARN, TAG_STTD, "[Config WARNING] Fail to load config (punctuation override)");
			__sttd_config_save();
			return 0;
		}
	}
	
	fclose(config_fp);

	SLOG(LOG_DEBUG, TAG_STTD, "[Config] Load config : engine(%s), language(%s), silence(%d), profanity(%d), punctuation(%d)",
		g_engine_id, g_language, g_silence, g_profanity, g_punctuation);

	if (true == is_default_open) {
		if(0 == __sttd_config_save()) {
			SLOG(LOG_DEBUG, TAG_STTD, "[Config] Create config(%s)", CONFIG_FILE_PATH);
		}
	}

	return 0;
}

int sttd_config_initialize()
{
	g_engine_id = NULL;
	g_language = NULL;
	g_silence = 1;
	g_profanity = 1;
	g_punctuation = 0;

	__sttd_config_load();

	return 0;
}

int sttd_config_finalize()
{
	__sttd_config_save();
	return 0;
}

int sttd_config_get_default_engine(char** engine_id)
{
	if (NULL == engine_id)
		return -1;

	*engine_id = strdup(g_engine_id);
	return 0;
}

int sttd_config_set_default_engine(const char* engine_id)
{
	if (NULL == engine_id)
		return -1;

	if (NULL != g_engine_id)
		free(g_engine_id);

	g_engine_id = strdup(engine_id);
	__sttd_config_save();
	return 0;
}

int sttd_config_get_default_language(char** language)
{
	if (NULL == language)
		return -1;

	*language = strdup(g_language);

	return 0;
}

int sttd_config_set_default_language(const char* language)
{
	if (NULL == language)
		return -1;

	if (NULL != g_language)
		free(g_language);

	g_language = strdup(language);

	__sttd_config_save();

	return 0;
}

int sttd_config_get_default_silence_detection(int* silence)
{
	if (NULL == silence)
		return -1;

	*silence = g_silence;

	return 0;
}

int sttd_config_set_default_silence_detection(int silence)
{
	g_silence = silence;
	__sttd_config_save();
	return 0;
}

int sttd_config_get_default_profanity_filter(int* profanity)
{
	if (NULL == profanity)
		return -1;

	*profanity = g_profanity;

	return 0;
}

int sttd_config_set_default_profanity_filter(int profanity)
{
	g_profanity = profanity;
	__sttd_config_save();
	return 0;
}

int sttd_config_get_default_punctuation_override(int* punctuation)
{
	if (NULL == punctuation)
		return -1;

	*punctuation = g_punctuation;

	return 0;
}

int sttd_config_set_default_punctuation_override(int punctuation)
{
	g_punctuation = punctuation;
	__sttd_config_save();
	return 0;
}