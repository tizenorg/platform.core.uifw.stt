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


#include <vconf.h>

#include "sttd_main.h"
#include "sttd_config.h"


/*
* stt-daemon config
*/

int sttd_config_get_char_type(const char* key, char** value)
{
	if (NULL == key || NULL == value) {
		SLOG(LOG_ERROR, TAG_STTD, "[STTD Config ERROR] Input parameter is NULL");
		return STTD_ERROR_INVALID_PARAMETER;
	} 

	*value = vconf_get_str(key);
	if (NULL == *value) {
		SLOG(LOG_ERROR, TAG_STTD, "[STTD Config ERROR] Fail to get char type from config : key(%s)", key);
		return -1;
	}

	return 0;
}

int sttd_config_set_char_type(const char* key, const char* value)
{
	if (NULL == key || NULL == value) {
		SLOG(LOG_ERROR, TAG_STTD, "[STTD Config ERROR] Input parameter is NULL");
		return STTD_ERROR_INVALID_PARAMETER;
	} 

	if (0 != vconf_set_str(key, value)) {
		SLOG(LOG_ERROR, TAG_STTD, "[STTD Config ERROR] Fail to set char type"); 
		return -1;
	}

	return 0;
}

int sttd_config_get_bool_type(const char* key, bool* value)
{
	if (NULL == key || NULL == value) {
		SLOG(LOG_ERROR, TAG_STTD, "[STTD Config ERROR] Input parameter is NULL");
		return STTD_ERROR_INVALID_PARAMETER;
	} 

	int result ;
	if (0 != vconf_get_int(key, &result)) {
		SLOG(LOG_ERROR, TAG_STTD, "[STTD Config ERROR] Fail to get bool type config : key(%s)", key);
		return -1;
	}

	*value = (bool) result;

	return 0;
}

int sttd_config_set_bool_type(const char* key, const bool value)
{
	if (NULL == key) {
		SLOG(LOG_ERROR, TAG_STTD, "[STTD Config ERROR] Input parameter is NULL");
		return STTD_ERROR_INVALID_PARAMETER;
	} 

	int result = (int)value;
	if (0 != vconf_set_int(key, result)) {
		SLOG(LOG_ERROR, TAG_STTD, "[STTD Config ERROR] Fail to get bool type config : key(%s)", key);
		return -1;
	}

	return 0;
}

/*
* plug-in daemon interface
*/

int __make_key_for_engine(const char* engine_id, const char* key, char** out_key)
{
	int key_size = strlen(STTD_CONFIG_PREFIX) + strlen(engine_id) + strlen(key) + 2; /* 2 means both '/' and '\0'*/

	*out_key = (char*) malloc( sizeof(char) * key_size);

	snprintf(*out_key, key_size, "%s%s/%s", STTD_CONFIG_PREFIX, engine_id, key );

	return 0;
}

int sttd_config_set_persistent_data(const char* engine_id, const char* key, const char* value)
{
	if (NULL == engine_id || NULL == key || NULL == value) {
		SLOG(LOG_ERROR, TAG_STTD, "[STTD Config ERROR] BAD Parameter"); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	char* vconf_key = NULL;
	if (0 != __make_key_for_engine(engine_id, key, &vconf_key)) {
		SLOG(LOG_ERROR, TAG_STTD, "[STTD Config ERROR] Fail __make_key_for_engine()");
		return -1;
	}

	if (NULL == vconf_key)		
		return -1;

	if (0 != vconf_set_str(vconf_key, value)) {
		SLOG(LOG_ERROR, TAG_STTD, "[STTD Config ERROR] Fail to set key, value");

		if (vconf_key != NULL)	
			free(vconf_key);

		return -1;
	}

	SLOG(LOG_DEBUG, TAG_STTD, "[STTD Config DEBUG] sttd_config_set_persistent_data : key(%s), value(%s)", vconf_key, value);

	if (NULL != vconf_key)	
		free(vconf_key);

	return 0;
}

int sttd_config_get_persistent_data(const char* engine_id, const char* key, char** value)
{
	if (NULL == engine_id) {
		SLOG(LOG_ERROR, TAG_STTD, "[STTD Config ERROR] BAD Parameter"); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	char* vconf_key = NULL;

	if (0 != __make_key_for_engine(engine_id, key, &vconf_key)) {
		SLOG(LOG_ERROR, TAG_STTD, "[STTD Config ERROR] Fail __make_key_for_engine()");
		return -1;
	}

	if (NULL == vconf_key)
		return -1;

	char* temp;
	temp = vconf_get_str(vconf_key);
	if (NULL == temp) {
		SLOG(LOG_ERROR, TAG_STTD, "[STTD Config ERROR] Fail to get value");

		if(vconf_key != NULL)	
			free(vconf_key);

		return -1;
	}

	*value = g_strdup(temp);

	SLOG(LOG_DEBUG, TAG_STTD, "[STTD Config DEBUG] sttd_config_get_persistent_data : key(%s), value(%s)", vconf_key, *value);

	if (vconf_key != NULL)	free(vconf_key);
	if (temp != NULL)	free(temp);

	return 0;
}

int sttd_config_remove_persistent_data(const char* engine_id, const char* key)
{
	if (NULL == engine_id || NULL == key) {
		SLOG(LOG_ERROR, TAG_STTD, "[STTD Config ERROR] BAD Parameter"); 
		return STTD_ERROR_INVALID_PARAMETER;
	}

	char* vconf_key = NULL;
	if (0 != __make_key_for_engine(engine_id, key, &vconf_key)) {
		SLOG(LOG_ERROR, TAG_STTD, "[STTD Config ERROR] Fail __make_key_for_engine()");
		return -1;
	}

	if (NULL == vconf_key)		
		return -1;

	if (0 != vconf_unset(vconf_key)) {	
		SLOG(LOG_ERROR, TAG_STTD, "[STTD Config ERROR] Fail to remove key");

		if(vconf_key != NULL)	
			free(vconf_key);

		return -1;
	}

	SLOG(LOG_DEBUG, TAG_STTD, "[STTD Config DEBUG] sttd_config_remove_persistent_data : key(%s)", vconf_key);

	if( NULL != vconf_key )		
		free(vconf_key);

	return 0;
}


