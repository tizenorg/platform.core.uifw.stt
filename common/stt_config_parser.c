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

#include <dlog.h>
#include <vconf.h>

#include "stt_defs.h"
#include "stt_config_parser.h"


#define STT_TAG_ENGINE_BASE_TAG		"stt-engine"
#define STT_TAG_ENGINE_NAME		"name"
#define STT_TAG_ENGINE_ID		"id"
#define STT_TAG_ENGINE_SETTING		"setting"
#define STT_TAG_ENGINE_AGREEMENT	"agreement"
#define STT_TAG_ENGINE_LANGUAGE_SET	"languages"
#define STT_TAG_ENGINE_LANGUAGE		"lang"
#define STT_TAG_ENGINE_SILENCE_SUPPORT	"silence-detection-support"

#define STT_TAG_CONFIG_BASE_TAG		"stt-config"
#define STT_TAG_CONFIG_ENGINE_ID	"engine"
#define STT_TAG_CONFIG_ENGINE_SETTING	"engine-setting"
#define STT_TAG_CONFIG_AUTO_LANGUAGE	"auto"
#define STT_TAG_CONFIG_LANGUAGE		"language"
#define STT_TAG_CONFIG_SILENCE_DETECTION "silence-detection"


#define STT_TAG_TIME_BASE_TAG		"stt-time"
#define STT_TAG_TIME_INDEX		"index"
#define STT_TAG_TIME_COUNT		"count"
#define STT_TAG_TIME_TIME_INFO		"time-info"
#define STT_TAG_TIME_TEXT		"text-info"
#define STT_TAG_TIME_START		"start"
#define STT_TAG_TIME_END		"end"

extern const char* stt_tag();

static xmlDocPtr g_config_doc = NULL;

int stt_parser_get_engine_info(const char* path, stt_engine_info_s** engine_info)
{
	if (NULL == path || NULL == engine_info) {
		SLOG(LOG_ERROR, stt_tag(), "[ERROR] Input parameter is NULL");
		return -1;
	}

	xmlDocPtr doc = NULL;
	xmlNodePtr cur = NULL;
	xmlChar *key;

	doc = xmlParseFile(path);
	if (doc == NULL) {
		return -1;
	}

	cur = xmlDocGetRootElement(doc);
	if (cur == NULL) {
		SLOG(LOG_ERROR, stt_tag(), "[ERROR] Empty document");
		xmlFreeDoc(doc);
		return -1;
	}

	if (xmlStrcmp(cur->name, (const xmlChar *)STT_TAG_ENGINE_BASE_TAG)) {
		SLOG(LOG_ERROR, stt_tag(), "[ERROR] The wrong type, root node is NOT 'stt-engine'");
		xmlFreeDoc(doc);
		return -1;
	}

	cur = cur->xmlChildrenNode;
	if (cur == NULL) {
		SLOG(LOG_ERROR, stt_tag(), "[ERROR] Empty document");
		xmlFreeDoc(doc);
		return -1;
	}

	/* alloc engine info */
	stt_engine_info_s* temp;
	temp = (stt_engine_info_s*)calloc(1, sizeof(stt_engine_info_s));
	if (NULL == temp) {
		xmlFreeDoc(doc);
		SLOG(LOG_ERROR, stt_tag(), "[ERROR] Fail to allocate memory");
		return -1;
	}

	temp->name = NULL;
	temp->uuid = NULL;
	temp->setting = NULL;
	temp->agreement = NULL;
	temp->languages = NULL;
	temp->support_silence_detection = false;

	while (cur != NULL) {
		if (0 == xmlStrcmp(cur->name, (const xmlChar *)STT_TAG_ENGINE_NAME)) {
			key = xmlNodeGetContent(cur);
			if (NULL != key) {
				// SLOG(LOG_DEBUG, stt_tag(), "Engine name : %s", (char *)key);
				if (NULL != temp->name)	free(temp->name);
				temp->name = strdup((char*)key);
				xmlFree(key);
			} else {
				SECURE_SLOG(LOG_ERROR, stt_tag(), "[ERROR] <%s> has no content", STT_TAG_ENGINE_NAME);
			}
		} else if (0 == xmlStrcmp(cur->name, (const xmlChar *)STT_TAG_ENGINE_ID)) {
			key = xmlNodeGetContent(cur);
			if (NULL != key) {
				// SLOG(LOG_DEBUG, stt_tag(), "Engine uuid : %s", (char *)key);
				if (NULL != temp->uuid)	free(temp->uuid);
				temp->uuid = strdup((char*)key);
				xmlFree(key);
			} else {
				SECURE_SLOG(LOG_ERROR, stt_tag(), "[ERROR] <%s> has no content", STT_TAG_ENGINE_ID);
			}
		} else if (0 == xmlStrcmp(cur->name, (const xmlChar *)STT_TAG_ENGINE_SETTING)) {
			key = xmlNodeGetContent(cur);
			if (NULL != key) {
				SLOG(LOG_DEBUG, stt_tag(), "Engine setting : %s", (char *)key);
				if (NULL != temp->setting)	free(temp->setting);
				temp->setting = strdup((char*)key);
				xmlFree(key);
			} else {
				SLOG(LOG_ERROR, stt_tag(), "[ERROR] <%s> has no content", STT_TAG_ENGINE_SETTING);
			}
		} else if (0 == xmlStrcmp(cur->name, (const xmlChar *)STT_TAG_ENGINE_AGREEMENT)) {
			key = xmlNodeGetContent(cur);
			if (NULL != key) {
				SLOG(LOG_DEBUG, stt_tag(), "Engine agreement : %s", (char *)key);
				if (NULL != temp->agreement)	free(temp->agreement);
				temp->agreement = strdup((char*)key);
				xmlFree(key);
			} else {
				SLOG(LOG_ERROR, stt_tag(), "[ERROR] <%s> has no content", STT_TAG_ENGINE_AGREEMENT);
			}
		} else if (0 == xmlStrcmp(cur->name, (const xmlChar *)STT_TAG_ENGINE_LANGUAGE_SET)) {
			xmlNodePtr lang_node = NULL;
			char* temp_lang = NULL;

			lang_node = cur->xmlChildrenNode;

			while (lang_node != NULL) {
				if (0 == xmlStrcmp(lang_node->name, (const xmlChar *)STT_TAG_ENGINE_LANGUAGE)){
					key = xmlNodeGetContent(lang_node);
					if (NULL != key) {
						// SLOG(LOG_DEBUG, stt_tag(), "language : %s", (char *)key);
						temp_lang = strdup((char*)key);
						temp->languages = g_slist_append(temp->languages, temp_lang);
						xmlFree(key);
					} else {
						SECURE_SLOG(LOG_ERROR, stt_tag(), "[ERROR] <%s> has no content", STT_TAG_ENGINE_LANGUAGE);
					}
				}

				lang_node = lang_node->next;
			}
		} else if (0 == xmlStrcmp(cur->name, (const xmlChar *)STT_TAG_ENGINE_SILENCE_SUPPORT)) {
			key = xmlNodeGetContent(cur);
			if (NULL != key) {
				//SLOG(LOG_DEBUG, stt_tag(), "silence-detection-support : %s", (char *)key);

				if (0 == xmlStrcmp(key, (const xmlChar *)"true"))
					temp->support_silence_detection = true;
				else
					temp->support_silence_detection = false;
				
				xmlFree(key);
			} else {
				SECURE_SLOG(LOG_ERROR, stt_tag(), "[ERROR] <%s> has no content", STT_TAG_ENGINE_SILENCE_SUPPORT);
			}
		} else {

		}

		cur = cur->next;
	}

	xmlFreeDoc(doc);

	if (NULL == temp->name || NULL == temp->uuid) {
		/* Invalid engine */
		SECURE_SLOG(LOG_ERROR, stt_tag(), "[ERROR] Invalid engine : %s", path);
		stt_parser_free_engine_info(temp);
		return -1;
	}

	*engine_info = temp;

	return 0;
}

int stt_parser_free_engine_info(stt_engine_info_s* engine_info)
{
	if (NULL == engine_info) {
		SLOG(LOG_ERROR, stt_tag(), "[ERROR] Input parameter is NULL");
		return -1;
	}

	if (NULL != engine_info->name)		free(engine_info->name);
	if (NULL != engine_info->uuid)		free(engine_info->uuid);
	if (NULL != engine_info->setting)	free(engine_info->setting);
	if (NULL != engine_info->agreement)	free(engine_info->agreement);

	int count = g_slist_length(engine_info->languages);

	int i ;
	char *temp_lang;

	for (i = 0;i < count ;i++) {
		temp_lang = g_slist_nth_data(engine_info->languages, 0);

		if (NULL != temp_lang) {
			engine_info->languages = g_slist_remove(engine_info->languages, temp_lang);

			if (NULL != temp_lang)
				free(temp_lang);
		} 
	}

	if (NULL != engine_info)	free(engine_info);

	return 0;	
}

int stt_parser_print_engine_info(stt_engine_info_s* engine_info)
{
	if (NULL == engine_info)
		return -1;

	SLOG(LOG_DEBUG, stt_tag(), "== get engine info ==");
	SECURE_SLOG(LOG_DEBUG, stt_tag(), " name : %s", engine_info->name);
	SECURE_SLOG(LOG_DEBUG, stt_tag(), " id   : %s", engine_info->uuid);
	if (NULL != engine_info->setting)	SECURE_SLOG(LOG_DEBUG, stt_tag(), " setting : %s", engine_info->setting);

	SLOG(LOG_DEBUG, stt_tag(), " languages");
	GSList *iter = NULL;
	char* lang;
	if (g_slist_length(engine_info->languages) > 0) {
		/* Get a first item */
		iter = g_slist_nth(engine_info->languages, 0);

		int i = 1;	
		while (NULL != iter) {
			/*Get handle data from list*/
			lang = iter->data;

			SECURE_SLOG(LOG_DEBUG, stt_tag(), "  [%dth] %s", i, lang);

			/*Get next item*/
			iter = g_slist_next(iter);
			i++;
		}
	} else {
		SLOG(LOG_ERROR, stt_tag(), "  language is NONE");
	}
	SECURE_SLOG(LOG_DEBUG, stt_tag(), " silence support : %s", engine_info->support_silence_detection ? "true" : "false");
	SLOG(LOG_DEBUG, stt_tag(), "=====================");

	return 0;
}

int stt_parser_load_config(stt_config_s** config_info)
{
	if (NULL == config_info) {
		SLOG(LOG_ERROR, stt_tag(), "[ERROR] Input parameter is NULL");
		return -1;
	}

	xmlDocPtr doc = NULL;
	xmlNodePtr cur = NULL;
	xmlChar *key;
	bool is_default_open = false;

	doc = xmlParseFile(STT_CONFIG);
	if (doc == NULL) {
		doc = xmlParseFile(STT_DEFAULT_CONFIG);
		if (doc == NULL) {
			SECURE_SLOG(LOG_ERROR, stt_tag(), "[ERROR] Fail to parse file error : %s", STT_DEFAULT_CONFIG);
			return -1;
		}
		is_default_open = true;
	}

	cur = xmlDocGetRootElement(doc);
	if (cur == NULL) {
		SLOG(LOG_ERROR, stt_tag(), "[ERROR] Empty document");
		xmlFreeDoc(doc);
		return -1;
	}

	if (xmlStrcmp(cur->name, (const xmlChar *) STT_TAG_CONFIG_BASE_TAG)) {
		SECURE_SLOG(LOG_ERROR, stt_tag(), "[ERROR] The wrong type, root node is NOT %s", STT_TAG_CONFIG_BASE_TAG);
		xmlFreeDoc(doc);
		return -1;
	}

	cur = cur->xmlChildrenNode;
	if (cur == NULL) {
		SLOG(LOG_ERROR, stt_tag(), "[ERROR] Empty document");
		xmlFreeDoc(doc);
		return -1;
	}

	/* alloc engine info */
	stt_config_s* temp;
	temp = (stt_config_s*)calloc(1, sizeof(stt_config_s));
	if (NULL == temp) {
		xmlFreeDoc(doc);
		SLOG(LOG_ERROR, stt_tag(), "[ERROR] Fail to allocate memory");
		return -1;
	}

	temp->engine_id = NULL;
	temp->setting = NULL;
	temp->language = NULL;

	while (cur != NULL) {
		if (0 == xmlStrcmp(cur->name, (const xmlChar *)STT_TAG_CONFIG_ENGINE_ID)) {
			key = xmlNodeGetContent(cur);
			if (NULL != key) {
				//SLOG(LOG_DEBUG, stt_tag(), "Engine id : %s", (char *)key);
				if (NULL != temp->engine_id)	free(temp->engine_id);
				temp->engine_id = strdup((char*)key);
				xmlFree(key);
			} else {
				SLOG(LOG_ERROR, stt_tag(), "[ERROR] engine id is NULL");
			}
		} else if (0 == xmlStrcmp(cur->name, (const xmlChar *)STT_TAG_CONFIG_ENGINE_SETTING)) {
			key = xmlNodeGetContent(cur);
			if (NULL != key) {
				//SECURE_SLOG(LOG_DEBUG, stt_tag(), "Setting path : %s", (char *)key);
				if (NULL != temp->setting)	free(temp->setting);
				temp->setting = strdup((char*)key);
				xmlFree(key);
			} else {
				SLOG(LOG_ERROR, stt_tag(), "[ERROR] setting path is NULL");
			}

		} else if (0 == xmlStrcmp(cur->name, (const xmlChar *)STT_TAG_CONFIG_AUTO_LANGUAGE)) {
			key = xmlNodeGetContent(cur);
			if (NULL != key) {
				//SECURE_SLOG(LOG_DEBUG, stt_tag(), "Auto language : %s", (char *)key);

				if (0 == xmlStrcmp(key, (const xmlChar *)"on")) {
					temp->auto_lang = true;
				} else if (0 == xmlStrcmp(key, (const xmlChar *)"off")) {
					temp->auto_lang = false;
				} else {
					SLOG(LOG_ERROR, stt_tag(), "Auto voice is wrong");
					temp->auto_lang = true;
				}

				xmlFree(key);
			} else {
				SLOG(LOG_ERROR, stt_tag(), "[ERROR] auto langauge is NULL");
			}
		} else if (0 == xmlStrcmp(cur->name, (const xmlChar *)STT_TAG_CONFIG_LANGUAGE)) {
			key = xmlNodeGetContent(cur);
			if (NULL != key) {
				//SLOG(LOG_DEBUG, stt_tag(), "language : %s", (char *)key);
				if (NULL != temp->language)	free(temp->language);
				temp->language = strdup((char*)key);
				xmlFree(key);
			} else {
				SLOG(LOG_ERROR, stt_tag(), "[ERROR] language is NULL");
			}
		} else if (0 == xmlStrcmp(cur->name, (const xmlChar *)STT_TAG_CONFIG_SILENCE_DETECTION)) {
			key = xmlNodeGetContent(cur);
			if (NULL != key) {
				//SLOG(LOG_DEBUG, stt_tag(), "silence-detection : %s", (char *)key);

				if (0 == xmlStrcmp(key, (const xmlChar *)"on"))
					temp->silence_detection = true;
				else
					temp->silence_detection = false;

				xmlFree(key);
			} else {
				SLOG(LOG_ERROR, stt_tag(), "[ERROR] silence-detection is NULL");
			}
		} else {

		}

		cur = cur->next;
	}

	*config_info = temp;
	g_config_doc = doc;

	if (is_default_open)
		xmlSaveFile(STT_CONFIG, g_config_doc);

	return 0;
}

int stt_parser_unload_config(stt_config_s* config_info)
{
	if (NULL != g_config_doc)	xmlFreeDoc(g_config_doc);
	if (NULL != config_info) {
		if (NULL != config_info->engine_id)	free(config_info->engine_id);
		if (NULL != config_info->setting)	free(config_info->setting);
		if (NULL != config_info->language)	free(config_info->language);

		free(config_info);
	}

	return 0;
}

int stt_parser_set_engine(const char* engine_id, const char* setting, const char* language, bool silence)
{
	if (NULL == g_config_doc || NULL == engine_id)
		return -1;

	xmlNodePtr cur = NULL;
	cur = xmlDocGetRootElement(g_config_doc);
	if (cur == NULL) {
		SLOG(LOG_ERROR, stt_tag(), "[ERROR] Empty document");
		return -1;
	}

	if (xmlStrcmp(cur->name, (const xmlChar *) STT_TAG_CONFIG_BASE_TAG)) {
		SECURE_SLOG(LOG_ERROR, stt_tag(), "[ERROR] The wrong type, root node is NOT %s", STT_TAG_CONFIG_BASE_TAG);
		return -1;
	}

	cur = cur->xmlChildrenNode;
	if (cur == NULL) {
		SLOG(LOG_ERROR, stt_tag(), "[ERROR] Empty document");
		return -1;
	}

	while (cur != NULL) {
		if (0 == xmlStrcmp(cur->name, (const xmlChar *)STT_TAG_CONFIG_ENGINE_ID)) {
			xmlNodeSetContent(cur, (const xmlChar *)engine_id);
		}

		if (0 == xmlStrcmp(cur->name, (const xmlChar *)STT_TAG_CONFIG_LANGUAGE)) {
			xmlNodeSetContent(cur, (const xmlChar *)language);
		}

		if (0 == xmlStrcmp(cur->name, (const xmlChar *)STT_TAG_CONFIG_ENGINE_SETTING)) {
			xmlNodeSetContent(cur, (const xmlChar *)setting);
		}

		if (0 == xmlStrcmp(cur->name, (const xmlChar *)STT_TAG_CONFIG_SILENCE_DETECTION)) {
			if (true == silence)
				xmlNodeSetContent(cur, (const xmlChar *)"on");
			else 
				xmlNodeSetContent(cur, (const xmlChar *)"off");
		}

		cur = cur->next;
	}

	int ret = xmlSaveFile(STT_CONFIG, g_config_doc);
	SLOG(LOG_DEBUG, stt_tag(), "Save result : %d", ret);

	return 0;
}

int stt_parser_set_language(const char* language)
{
	if (NULL == g_config_doc || NULL == language)
		return -1;

	xmlNodePtr cur = NULL;
	cur = xmlDocGetRootElement(g_config_doc);
	if (cur == NULL) {
		SLOG(LOG_ERROR, stt_tag(), "[ERROR] Empty document");
		return -1;
	}

	if (xmlStrcmp(cur->name, (const xmlChar *) STT_TAG_CONFIG_BASE_TAG)) {
		SECURE_SLOG(LOG_ERROR, stt_tag(), "[ERROR] The wrong type, root node is NOT %s", STT_TAG_CONFIG_BASE_TAG);
		return -1;
	}

	cur = cur->xmlChildrenNode;
	if (cur == NULL) {
		SLOG(LOG_ERROR, stt_tag(), "[ERROR] Empty document");
		return -1;
	}

	while (cur != NULL) {
		if (0 == xmlStrcmp(cur->name, (const xmlChar *)STT_TAG_CONFIG_LANGUAGE)) {
			xmlNodeSetContent(cur, (const xmlChar *)language);
		} 

		cur = cur->next;
	}

	int ret = xmlSaveFile(STT_CONFIG, g_config_doc);
	SLOG(LOG_DEBUG, stt_tag(), "Save result : %d", ret);


	return 0;
}

int stt_parser_set_auto_lang(bool value)
{
	if (NULL == g_config_doc)
		return -1;

	xmlNodePtr cur = NULL;
	cur = xmlDocGetRootElement(g_config_doc);
	if (cur == NULL) {
		SLOG(LOG_ERROR, stt_tag(), "[ERROR] Empty document");
		return -1;
	}

	if (xmlStrcmp(cur->name, (const xmlChar *) STT_TAG_CONFIG_BASE_TAG)) {
		SLOG(LOG_ERROR, stt_tag(), "[ERROR] The wrong type, root node is NOT %s", STT_TAG_CONFIG_BASE_TAG);
		return -1;
	}

	cur = cur->xmlChildrenNode;
	if (cur == NULL) {
		SLOG(LOG_ERROR, stt_tag(), "[ERROR] Empty document");
		return -1;
	}

	while (cur != NULL) {
		if (0 == xmlStrcmp(cur->name, (const xmlChar *)STT_TAG_CONFIG_AUTO_LANGUAGE)) {
			if (true == value) {
				xmlNodeSetContent(cur, (const xmlChar *)"on");
			} else if (false == value) {
				xmlNodeSetContent(cur, (const xmlChar *)"off");
			} else {
				SLOG(LOG_ERROR, stt_tag(), "[ERROR] The wrong value of auto voice");
				return -1;
			}
			break;
		}
		cur = cur->next;
	}

	int ret = xmlSaveFile(STT_CONFIG, g_config_doc);
	SLOG(LOG_DEBUG, stt_tag(), "Save result : %d", ret);

	return 0;
}

int stt_parser_set_silence_detection(bool value)
{
	if (NULL == g_config_doc)
		return -1;

	xmlNodePtr cur = NULL;
	cur = xmlDocGetRootElement(g_config_doc);
	if (cur == NULL) {
		SLOG(LOG_ERROR, stt_tag(), "[ERROR] Empty document");
		return -1;
	}

	if (xmlStrcmp(cur->name, (const xmlChar *) STT_TAG_CONFIG_BASE_TAG)) {
		SECURE_SLOG(LOG_ERROR, stt_tag(), "[ERROR] The wrong type, root node is NOT %s", STT_TAG_CONFIG_BASE_TAG);
		return -1;
	}

	cur = cur->xmlChildrenNode;
	if (cur == NULL) {
		SLOG(LOG_ERROR, stt_tag(), "[ERROR] Empty document");
		return -1;
	}

	while (cur != NULL) {
		if (0 == xmlStrcmp(cur->name, (const xmlChar *)STT_TAG_CONFIG_SILENCE_DETECTION)) {
			if (true == value)
				xmlNodeSetContent(cur, (const xmlChar *)"on");
			else 
				xmlNodeSetContent(cur, (const xmlChar *)"off");
		} 

		cur = cur->next;
	}

	int ret = xmlSaveFile(STT_CONFIG, g_config_doc);
	SLOG(LOG_DEBUG, stt_tag(), "Save result : %d", ret);

	return 0;
}

int stt_parser_find_config_changed(char** engine, char** setting, int* auto_lang, char** language, int* silence)
{
	if (NULL == engine || NULL == language || NULL == silence) {
		SLOG(LOG_ERROR, stt_tag(), "[ERROR] Input parameter is NULL");
		return -1;
	}

	xmlDocPtr doc = NULL;
	xmlNodePtr cur_new = NULL;
	xmlNodePtr cur_old = NULL;

	xmlChar *key_new;
	xmlChar *key_old;

	doc = xmlParseFile(STT_CONFIG);
	if (doc == NULL) {
		SECURE_SLOG(LOG_ERROR, stt_tag(), "[ERROR] Fail to parse file error : %s", STT_CONFIG);
		return -1;
	}

	cur_new = xmlDocGetRootElement(doc);
	cur_old = xmlDocGetRootElement(g_config_doc);
	if (cur_new == NULL || cur_old == NULL) {
		SLOG(LOG_ERROR, stt_tag(), "[ERROR] Empty document");
		xmlFreeDoc(doc);
		return -1;
	}

	if (xmlStrcmp(cur_new->name, (const xmlChar*)STT_TAG_CONFIG_BASE_TAG) || xmlStrcmp(cur_old->name, (const xmlChar*)STT_TAG_CONFIG_BASE_TAG)) {
		SECURE_SLOG(LOG_ERROR, stt_tag(), "[ERROR] The wrong type, root node is NOT %s", STT_TAG_CONFIG_BASE_TAG);
		xmlFreeDoc(doc);
		return -1;
	}

	cur_new = cur_new->xmlChildrenNode;
	cur_old = cur_old->xmlChildrenNode;
	if (cur_new == NULL || cur_old == NULL) {
		SLOG(LOG_ERROR, stt_tag(), "[ERROR] Empty document");
		xmlFreeDoc(doc);
		return -1;
	}

	*engine = NULL;
	*setting = NULL;
	*language = NULL;

	while (cur_new != NULL && cur_old != NULL) {
		if (0 == xmlStrcmp(cur_new->name, (const xmlChar*)STT_TAG_CONFIG_ENGINE_ID)) {
			if (0 == xmlStrcmp(cur_old->name, (const xmlChar*)STT_TAG_CONFIG_ENGINE_ID)) {
				key_old = xmlNodeGetContent(cur_old);
				if (NULL != key_old) {
					key_new = xmlNodeGetContent(cur_new);
					if (NULL != key_new) {
						if (0 != xmlStrcmp(key_old, key_new)) {
							SECURE_SLOG(LOG_DEBUG, stt_tag(), "Old engine id(%s), New engine(%s)", (char*)key_old, (char*)key_new);
							if (NULL != *engine)	free(*engine);
							*engine = strdup((char*)key_new);
						}
						xmlFree(key_new);
					}
					xmlFree(key_old);
				}
			} else {
				SLOG(LOG_ERROR, stt_tag(), "[ERROR] Old config and new config are different");
			}
		} else if (0 == xmlStrcmp(cur_new->name, (const xmlChar*)STT_TAG_CONFIG_ENGINE_SETTING)) {
			if (0 == xmlStrcmp(cur_old->name, (const xmlChar*)STT_TAG_CONFIG_ENGINE_SETTING)) {
				key_old = xmlNodeGetContent(cur_old);
				if (NULL != key_old) {
					key_new = xmlNodeGetContent(cur_new);
					if (NULL != key_new) {
						if (0 != xmlStrcmp(key_old, key_new)) {
							SLOG(LOG_DEBUG, stt_tag(), "Old engine setting(%s), New engine setting(%s)", (char*)key_old, (char*)key_new);
							if (NULL != *setting)	free(*setting);
							*setting = strdup((char*)key_new);
						}
						xmlFree(key_new);
					}
					xmlFree(key_old);
				}
			} else {
				SLOG(LOG_ERROR, stt_tag(), "[ERROR] Old config and new config are different");
			}
		} else if (0 == xmlStrcmp(cur_new->name, (const xmlChar*)STT_TAG_CONFIG_AUTO_LANGUAGE)) {
			if (0 == xmlStrcmp(cur_old->name, (const xmlChar*)STT_TAG_CONFIG_AUTO_LANGUAGE)) {
				key_old = xmlNodeGetContent(cur_old);
				if (NULL != key_old) {
					key_new = xmlNodeGetContent(cur_new);
					if (NULL != key_new) {
						if (0 != xmlStrcmp(key_old, key_new)) {
							SLOG(LOG_DEBUG, stt_tag(), "Old auto lang(%s), New auto lang(%s)", (char*)key_old, (char*)key_new);
							if (0 == xmlStrcmp((const xmlChar*)"on", key_new)) {
								*auto_lang = (int)true;
							} else {
								*auto_lang = (int)false;
							}
						}

						xmlFree(key_new);
					}
					xmlFree(key_old);
				}
			} else {
				SLOG(LOG_ERROR, stt_tag(), "[ERROR] old config and new config are different");
			}
		} else if (0 == xmlStrcmp(cur_new->name, (const xmlChar*)STT_TAG_CONFIG_LANGUAGE)) {
			if (0 == xmlStrcmp(cur_old->name, (const xmlChar*)STT_TAG_CONFIG_LANGUAGE)) {
				key_old = xmlNodeGetContent(cur_old);
				if (NULL != key_old) {
					key_new = xmlNodeGetContent(cur_new);
					if (NULL != key_new) {
						if (0 != xmlStrcmp(key_old, key_new)) {
							SECURE_SLOG(LOG_DEBUG, stt_tag(), "Old language(%s), New language(%s)", (char*)key_old, (char*)key_new);
							if (NULL != *language)	free(*language);
							*language = strdup((char*)key_new);
						}
						xmlFree(key_new);
					}
					xmlFree(key_old);
				}
			} else {
				SLOG(LOG_ERROR, stt_tag(), "[ERROR] old config and new config are different");
			}
		} else if (0 == xmlStrcmp(cur_new->name, (const xmlChar*)STT_TAG_CONFIG_SILENCE_DETECTION)) {
			if (0 == xmlStrcmp(cur_old->name, (const xmlChar*)STT_TAG_CONFIG_SILENCE_DETECTION)) {
				key_old = xmlNodeGetContent(cur_old);
				if (NULL != key_old) {
					key_new = xmlNodeGetContent(cur_new);
					if (NULL != key_new) {
						if (0 != xmlStrcmp(key_old, key_new)) {
							SECURE_SLOG(LOG_DEBUG, stt_tag(), "Old silence(%s), New silence(%s)", (char*)key_old, (char*)key_new);
							if (0 == xmlStrcmp(key_new, (const xmlChar*)"on")) {
								*silence = 1;
							} else {
								*silence = 0;
							}
						}
						
						xmlFree(key_new);
					}
					xmlFree(key_old);
				}
			} else {
				SLOG(LOG_ERROR, stt_tag(), "[ERROR] old config and new config are different");
			}
		} else {

		}

		cur_new = cur_new->next;
		cur_old = cur_old->next;
	}
	
	xmlFreeDoc(g_config_doc);
	g_config_doc = doc;

	return 0;
}

/**
* time function
*/

int stt_parser_set_time_info(GSList* time_list)
{
	if (0 == g_slist_length(time_list)) {
		SLOG(LOG_WARN, stt_tag(), "[WARNING] There is no time info to save");
		return -1;
	}

	if (-1 == remove(STT_TIME_INFO_PATH)) {
		SECURE_SLOG(LOG_WARN, stt_tag(), "[PLAYER WARNING] Fail to remove file(%s)", STT_TIME_INFO_PATH);
	}

	xmlDocPtr doc = NULL;
	xmlNodePtr cur = NULL;

	doc = xmlNewDoc((const xmlChar*)"1.0");
	if (doc == NULL) {
		SECURE_SLOG(LOG_ERROR, stt_tag(), "[ERROR] Fail to make new doc");
		return -1;
	}

	cur = xmlNewNode(NULL, (const xmlChar*)STT_TAG_TIME_BASE_TAG);
	xmlDocSetRootElement(doc, cur);

	xmlNodePtr inode = NULL;
	GSList *iter = NULL;
	stt_result_time_info_s *data = NULL;
	char temp_str[256];

	snprintf(temp_str, 256, "%d", g_slist_length(time_list));

	inode = xmlNewNode(NULL, (const xmlChar*)STT_TAG_TIME_INDEX);
	xmlNewProp(inode, (const xmlChar*)STT_TAG_TIME_COUNT, (const xmlChar*)temp_str);
	xmlAddChild(cur, inode);

	/* Get a first item */
	iter = g_slist_nth(time_list, 0);
	while (NULL != iter) {
		data = iter->data;
		
		xmlNodePtr temp_node = NULL;

		SLOG(LOG_DEBUG, stt_tag(), "[%d] i(%d) t(%s) s(%d) e(%d)", 
			data->index, data->event, data->text, data->start_time, data->end_time);
		
		temp_node = xmlNewNode(NULL, (const xmlChar*)STT_TAG_TIME_TEXT);
		xmlNodeSetContent(temp_node, (const xmlChar*)data->text);
		xmlAddChild(inode, temp_node);
	
		temp_node = xmlNewNode(NULL, (const xmlChar*)STT_TAG_TIME_START);
		snprintf(temp_str, 256, "%ld", data->start_time);
		xmlNodeSetContent(temp_node, (const xmlChar*)temp_str);
		xmlAddChild(inode, temp_node);

		temp_node = xmlNewNode(NULL, (const xmlChar*)STT_TAG_TIME_END);
		snprintf(temp_str, 256, "%ld", data->end_time);
		xmlNodeSetContent(temp_node, (const xmlChar*)temp_str);
		xmlAddChild(inode, temp_node);

		/*Get next item*/
		iter = g_slist_next(iter);
	}

	int ret = xmlSaveFormatFile(STT_TIME_INFO_PATH, doc, 1);
	SLOG(LOG_DEBUG, stt_tag(), "Save result : %d", ret);

	xmlFreeDoc(doc);
	return 0;
}

int stt_parser_get_time_info(GSList** time_list)
{
	if (NULL == time_list) {
		SLOG(LOG_ERROR, stt_tag(), "Invalid paramter : text is NULL");
		return -1;
	}

	xmlDocPtr doc = NULL;
	xmlNodePtr cur = NULL;

	doc = xmlParseFile(STT_TIME_INFO_PATH);
	if (doc == NULL) {
		SLOG(LOG_WARN, stt_tag(), "[WARNING] File is not exist");
		return -1;
	}

	cur = xmlDocGetRootElement(doc);
	if (cur == NULL) {
		SLOG(LOG_ERROR, stt_tag(), "[ERROR] Empty document");
		xmlFreeDoc(doc);
		return -1;
	}

	if (xmlStrcmp(cur->name, (const xmlChar *)STT_TAG_TIME_BASE_TAG)) {
		SLOG(LOG_ERROR, stt_tag(), "[ERROR] The wrong type, root node is NOT '%s'", STT_TAG_TIME_BASE_TAG);
		xmlFreeDoc(doc);
		return -1;
	}

	cur = cur->xmlChildrenNode;
	if (cur == NULL) {
		SLOG(LOG_ERROR, stt_tag(), "[ERROR] Empty document");
		xmlFreeDoc(doc);
		return -1;
	}

	/* alloc time info */
	GSList *temp_time_list = NULL;
	xmlChar *key = NULL;
	int index = 0;

	while (cur != NULL) {
		if (0 == xmlStrcmp(cur->name, (const xmlChar *)STT_TAG_TIME_INDEX)) {
			key = xmlGetProp(cur, (const xmlChar*)STT_TAG_TIME_COUNT);
			if (NULL == key) {
				SLOG(LOG_ERROR, stt_tag(), "[ERROR] <%s> has no content", STT_TAG_TIME_COUNT);
				return -1;
			}
			
			SLOG(LOG_DEBUG, stt_tag(), "Count : %s", (char *)key);

			/* Get time count */
			int count = 0;
			count = atoi((char*)key);
			xmlFree(key);

			if (count <= 0) {
				SLOG(LOG_ERROR, stt_tag(), "[ERROR] count is invalid : %d", count);
				break;
			}

			xmlNodePtr time_node = NULL;
			time_node = cur->xmlChildrenNode;

			int i = 0;
			for (i = 0;i < count;i++) {
				/* text */
				time_node = time_node->next;

				stt_result_time_info_s* temp_info;
				temp_info = (stt_result_time_info_s*)calloc(1, sizeof(stt_result_time_info_s));

				if (NULL == temp_info) {
					SLOG(LOG_ERROR, stt_tag(), "[ERROR] Memory alloc error!!");
					return -1;
				}

				temp_info->index = index;
				SLOG(LOG_DEBUG, stt_tag(), "index : %d", temp_info->index);

				if (0 == i)		temp_info->event = 0;
				else if (count -1 == i)	temp_info->event = 2;
				else 			temp_info->event = 1;

				if (0 == xmlStrcmp(time_node->name, (const xmlChar *)STT_TAG_TIME_TEXT)) {
					key = xmlNodeGetContent(time_node);
					if (NULL != key) {
						SLOG(LOG_DEBUG, stt_tag(), "text : %s", (char *)key);
						temp_info->text = strdup((char*)key);
						xmlFree(key);
					} else {
						SECURE_SLOG(LOG_ERROR, stt_tag(), "[ERROR] <%s> has no content", STT_TAG_TIME_TEXT);
						free(temp_info);
						break;
					}
				}

				/* text */
				time_node = time_node->next;
				time_node = time_node->next;

				if (0 == xmlStrcmp(time_node->name, (const xmlChar *)STT_TAG_TIME_START)) {
					key = xmlNodeGetContent(time_node);
					if (NULL != key) {
						SLOG(LOG_DEBUG, stt_tag(), "Start time : %s", (char *)key);
						temp_info->start_time = atoi((char*)key);
						xmlFree(key);
					} else {
						SECURE_SLOG(LOG_ERROR, stt_tag(), "[ERROR] <%s> has no content", STT_TAG_TIME_START);
						if (NULL != temp_info->text)	free(temp_info->text);
						free(temp_info);
						break;
					}
				}
				
				/* text */
				time_node = time_node->next;
				time_node = time_node->next;

				if (0 == xmlStrcmp(time_node->name, (const xmlChar *)STT_TAG_TIME_END)) {
					key = xmlNodeGetContent(time_node);
					if (NULL != key) {
						SLOG(LOG_DEBUG, stt_tag(), "End time : %s", (char *)key);
						temp_info->end_time = atoi((char*)key);
						xmlFree(key);
					} else {
						SECURE_SLOG(LOG_ERROR, stt_tag(), "[ERROR] <%s> has no content", STT_TAG_TIME_END);
						if (NULL != temp_info->text)	free(temp_info->text);
						free(temp_info);
						break;
					}
				}

				time_node = time_node->next;

				temp_time_list = g_slist_append(temp_time_list, temp_info);
			} /* for */
			index++;
		} /* if */
		cur = cur->next;
	}

	xmlFreeDoc(doc);

	*time_list = temp_time_list;

	return 0;
}

int stt_parser_clear_time_info()
{
	if (-1 == remove(STT_TIME_INFO_PATH)) {
		SECURE_SLOG(LOG_WARN, stt_tag(), "[PLAYER WARNING] Fail to remove file(%s)", STT_TIME_INFO_PATH);
	}

	return 0;
}
