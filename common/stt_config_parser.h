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

 
#ifndef __STT_CONFIG_PARSER_H_
#define __STT_CONFIG_PARSER_H_

#include <glib.h>
#include <libxml/parser.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	char*	name;
	char*	uuid;
	char*	setting;
	char*	agreement;
	GSList*	languages;
	bool	support_silence_detection;
}stt_engine_info_s;

typedef struct {
	char*	engine_id;
	char*	setting;
	bool	auto_lang;
	char*	language;
	bool	silence_detection;
}stt_config_s;

typedef struct {
	int	index;
	int	event;
	char*	text;
	long	start_time;
	long	end_time;
}stt_result_time_info_s;

/* Get engine information */
int stt_parser_get_engine_info(const char* path, stt_engine_info_s** engine_info);

int stt_parser_free_engine_info(stt_engine_info_s* engine_info);


int stt_parser_load_config(stt_config_s** config_info);

int stt_parser_unload_config(stt_config_s* config_info);

int stt_parser_set_engine(const char* engine_id, const char* setting, const char* language, bool silence);

int stt_parser_set_language(const char* language);

int stt_parser_set_auto_lang(bool value);

int stt_parser_set_silence_detection(bool value);

int stt_parser_find_config_changed(char** engine, char** setting, int* auto_lang, char** language, int* silence);


/* Time info */
int stt_parser_set_time_info(GSList* time_list);

int stt_parser_get_time_info(GSList** time_list);

int stt_parser_clear_time_info();


#ifdef __cplusplus
}
#endif

#endif /* __STT_CONFIG_PARSER_H_ */
