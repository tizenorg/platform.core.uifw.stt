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

#include <audio_io.h>
#include <Ecore.h>
#include <math.h>
#include <sound_manager.h>

#include "stt_defs.h"
#include "sttd_recorder.h"
#include "sttd_main.h"
#include "sttp.h"


#define FRAME_LENGTH 160
#define BUFFER_LENGTH FRAME_LENGTH * 2

typedef enum {
	STTD_RECORDER_STATE_NONE = -1,
	STTD_RECORDER_STATE_READY = 0,	/**< Recorder is ready to start */
	STTD_RECORDER_STATE_RECORDING	/**< In the middle of recording */
} sttd_recorder_state;

typedef struct {
	int			engine_id;
	audio_in_h		audio_h;
	sttp_audio_type_e	audio_type;
}stt_recorder_s;

static GSList *g_recorder_list;

static int g_recording_engine_id;

static stt_recorder_audio_cb	g_audio_cb;

static stt_recorder_interrupt_cb	g_interrupt_cb;

static sttd_recorder_state	g_recorder_state = STTD_RECORDER_STATE_NONE;

static FILE* g_pFile_vol;

static int g_buffer_count;

/* Sound buf save for test */
/*
#define BUF_SAVE_MODE
*/
#ifdef BUF_SAVE_MODE
static char g_temp_file_name[128] = {'\0',};

static FILE* g_pFile;

static int g_count = 1;
#endif 

const char* __stt_get_session_interrupt_code(sound_session_interrupted_code_e code)
{
	switch(code) {
	case SOUND_SESSION_INTERRUPTED_COMPLETED:		return "SOUND_SESSION_INTERRUPTED_COMPLETED";
	case SOUND_SESSION_INTERRUPTED_BY_MEDIA:		return "SOUND_SESSION_INTERRUPTED_BY_MEDIA";
	case SOUND_SESSION_INTERRUPTED_BY_CALL:			return "SOUND_SESSION_INTERRUPTED_BY_CALL";
	case SOUND_SESSION_INTERRUPTED_BY_EARJACK_UNPLUG:	return "SOUND_SESSION_INTERRUPTED_BY_EARJACK_UNPLUG";
	case SOUND_SESSION_INTERRUPTED_BY_RESOURCE_CONFLICT:	return "SOUND_SESSION_INTERRUPTED_BY_RESOURCE_CONFLICT";
	case SOUND_SESSION_INTERRUPTED_BY_ALARM:		return "SOUND_SESSION_INTERRUPTED_BY_ALARM";
	case SOUND_SESSION_INTERRUPTED_BY_EMERGENCY:		return "SOUND_SESSION_INTERRUPTED_BY_EMERGENCY";
	case SOUND_SESSION_INTERRUPTED_BY_NOTIFICATION:		return "SOUND_SESSION_INTERRUPTED_BY_NOTIFICATION";
	default:
		return "Undefined error code";
	}
}

void __sttd_recorder_sound_interrupted_cb(sound_session_interrupted_code_e code, void *user_data)
{
	SLOG(LOG_DEBUG, TAG_STTD, "[Recorder] Get the interrupt code from sound mgr : %s", 
		__stt_get_session_interrupt_code(code));

	if (SOUND_SESSION_INTERRUPTED_COMPLETED == code || SOUND_SESSION_INTERRUPTED_BY_EARJACK_UNPLUG == code)
		return;

	if (NULL != g_interrupt_cb) {
		g_interrupt_cb();
	}
	return;
}

int sttd_recorder_initialize(stt_recorder_audio_cb audio_cb, stt_recorder_interrupt_cb interrupt_cb)
{
	if (NULL == audio_cb || NULL == interrupt_cb) {
		SLOG(LOG_ERROR, TAG_STTD, "[Recorder ERROR] Input param is NOT valid");
		return STTD_ERROR_INVALID_PARAMETER;
	}

	if (STTD_RECORDER_STATE_NONE != g_recorder_state) {
		SLOG(LOG_ERROR, TAG_STTD, "[Recorder ERROR] Current state of recorder is recording");
		return STTD_ERROR_INVALID_STATE;
	}
	
	g_audio_cb = audio_cb;
	g_interrupt_cb = interrupt_cb;
	g_recorder_state = STTD_RECORDER_STATE_NONE;
	g_recording_engine_id = -1;

	if (0 != sound_manager_set_session_type(SOUND_SESSION_TYPE_MEDIA)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Recorder ERROR] Fail to set exclusive session");
	}

	if (0 != sound_manager_set_session_interrupted_cb(__sttd_recorder_sound_interrupted_cb, NULL)) {
		SLOG(LOG_ERROR, TAG_STTD, "[Recorder ERROR] Fail to set sound interrupt callback");
	}

	return 0;
}

int sttd_recorder_deinitialize()
{
	if (0 != sound_manager_unset_session_interrupted_cb()) {
		SLOG(LOG_ERROR, TAG_STTD, "[Recorder ERROR] Fail to unset sound interrupt callback");
	}

	/* Remove all recorder */
	GSList *iter = NULL;
	stt_recorder_s *recorder = NULL;

	iter = g_slist_nth(g_recorder_list, 0);

	while (NULL != iter) {
		recorder = iter->data;

		if (NULL != recorder) {
			g_recorder_list = g_slist_remove(g_recorder_list, recorder);
			audio_in_destroy(recorder->audio_h);

			free(recorder);
		}

		iter = g_slist_nth(g_recorder_list, 0);
	}

	if (0 == access(STT_AUDIO_VOLUME_PATH, R_OK)) {
		if (0 != remove(STT_AUDIO_VOLUME_PATH)) {
			SLOG(LOG_WARN, TAG_STTD, "[Recorder WARN] Fail to remove volume file"); 
		}
	}

	g_recorder_state = STTD_RECORDER_STATE_NONE;

	return 0;
}

static stt_recorder_s* __get_recorder(int engine_id)
{
	GSList *iter = NULL;
	stt_recorder_s *recorder = NULL;

	iter = g_slist_nth(g_recorder_list, 0);

	while (NULL != iter) {
		recorder = iter->data;

		if (recorder->engine_id == engine_id) {
			return recorder;
		}

		iter = g_slist_next(iter);
	}

	return NULL;
}

int sttd_recorder_set_audio_session()
{
	return 0;
}

int sttd_recorder_unset_audio_session()
{
	return 0;
}

int sttd_recorder_create(int engine_id, sttp_audio_type_e type, int channel, unsigned int sample_rate)
{
	/* Check engine id is valid */
	if (NULL != __get_recorder(engine_id)) {
		SLOG(LOG_WARN, TAG_STTD, "[Recorder WARNING] Engine id is already registered");
		return STTD_ERROR_INVALID_PARAMETER;
	}

	audio_channel_e audio_ch;
	audio_sample_type_e audio_type;
	audio_in_h temp_in_h;

	switch(channel) {
		case 1:	audio_ch = AUDIO_CHANNEL_MONO;		break;
		case 2:	audio_ch = AUDIO_CHANNEL_STEREO;	break;
		default:
			SLOG(LOG_ERROR, TAG_STTD, "[Recorder ERROR] Input channel is not supported");
			return STTD_ERROR_OPERATION_FAILED;
			break;
	}

	switch (type) {
		case STTP_AUDIO_TYPE_PCM_S16_LE:	audio_type = AUDIO_SAMPLE_TYPE_S16_LE;	break;
		case STTP_AUDIO_TYPE_PCM_U8:		audio_type = AUDIO_SAMPLE_TYPE_U8;	break;
		default:	
			SLOG(LOG_ERROR, TAG_STTD, "[Recorder ERROR] Invalid Audio Type"); 
			return STTD_ERROR_OPERATION_FAILED;
			break;
	}

	int ret;
	ret = audio_in_create(sample_rate, audio_ch, audio_type, &temp_in_h);
	if (AUDIO_IO_ERROR_NONE != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Recorder ERROR] Fail to create audio handle : %d", ret);
		return STTD_ERROR_OPERATION_FAILED;
	}

	stt_recorder_s* recorder;
	recorder = (stt_recorder_s*)calloc(1, sizeof(stt_recorder_s));

	recorder->engine_id = engine_id;
	recorder->audio_h = temp_in_h;
	recorder->audio_type = type;
	
	g_recorder_list = g_slist_append(g_recorder_list, recorder);

	g_recorder_state = STTD_RECORDER_STATE_READY;

	return 0;
}

int sttd_recorder_destroy(int engine_id)
{
	/* Check engine id is valid */
	stt_recorder_s* recorder;
	recorder = __get_recorder(engine_id);
	if (NULL == recorder) {
		SLOG(LOG_WARN, TAG_STTD, "[Recorder WARNING] Engine id is not valid");
		return STTD_ERROR_INVALID_PARAMETER;
	}

	int ret;
	if (STTD_RECORDER_STATE_RECORDING == g_recorder_state) {
		ret = audio_in_unprepare(recorder->audio_h);
		if (AUDIO_IO_ERROR_NONE != ret) {
			SLOG(LOG_ERROR, TAG_STTD, "[Recorder ERROR] Fail to unprepare audioin : %d", ret);
		}

		g_recorder_state = STTD_RECORDER_STATE_READY;
	}

	ret = audio_in_destroy(recorder->audio_h);
	if (AUDIO_IO_ERROR_NONE != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Recorder ERROR] Fail to destroy audioin : %d", ret);
	}

	g_recorder_list = g_slist_remove(g_recorder_list, recorder);

	free(recorder);

	return 0;
}

static float get_volume_decibel(char* data, int size, sttp_audio_type_e type)
{
	#define MAX_AMPLITUDE_MEAN_16 23170.115738161934
	#define MAX_AMPLITUDE_MEAN_08    89.803909382810

	int i, depthByte;
	int count = 0;

	float db = 0.0;
	float rms = 0.0;
	unsigned long long square_sum = 0;

	if (type == STTP_AUDIO_TYPE_PCM_S16_LE)
		depthByte = 2;
	else
		depthByte = 1;

	for (i = 0; i < size; i += (depthByte<<1)) {
		if (depthByte == 2) {
			short pcm16 = 0;
			memcpy(&pcm16, data + i, sizeof(short));
			square_sum += pcm16 * pcm16;
		} else {
			char pcm8 = 0;
			memcpy(&pcm8, data + i, sizeof(char));
			square_sum += pcm8 * pcm8;
		}
		count++;
	}

	if (0 == count)
		rms = 0.0;
	else
		rms = sqrt(square_sum/count);

	if (depthByte == 2)
		db = 20 * log10(rms/MAX_AMPLITUDE_MEAN_16);
	else
		db = 20 * log10(rms/MAX_AMPLITUDE_MEAN_08);

	return db;
}

Eina_Bool __read_audio_func(void *data)
{
	int read_byte = -1;
	static char g_buffer[BUFFER_LENGTH];

	/* Check engine id is valid */
	stt_recorder_s* recorder;
	recorder = __get_recorder(g_recording_engine_id);
	if (NULL == recorder) {
		return EINA_FALSE;
	}

	if (STTD_RECORDER_STATE_READY == g_recorder_state) {
		SLOG(LOG_DEBUG, TAG_STTD, "[Recorder] Exit audio reading func");
		return EINA_FALSE;
	}

	read_byte = audio_in_read(recorder->audio_h, g_buffer, BUFFER_LENGTH);
	if (0 > read_byte) {
		SLOG(LOG_WARN, TAG_STTD, "[Recorder WARNING] Fail to read audio : %d", read_byte);
		g_recorder_state = STTD_RECORDER_STATE_READY;
		return EINA_FALSE;
	}

	if (0 != g_audio_cb(g_buffer, read_byte)) {
		SLOG(LOG_WARN, TAG_STTD, "[Recorder WARNING] Fail audio callback");
		sttd_recorder_stop(g_recording_engine_id);
		return EINA_FALSE;
	}

	float vol_db = get_volume_decibel(g_buffer, BUFFER_LENGTH, recorder->audio_type);

	rewind(g_pFile_vol);

	fwrite((void*)(&vol_db), sizeof(vol_db), 1, g_pFile_vol);

	/* Audio read log */
	if (0 == g_buffer_count % 50) {
		SLOG(LOG_DEBUG, TAG_STTD, "[Recorder][%d] Recording... : read_size(%d)", g_buffer_count, read_byte);
		
		if (100000 == g_buffer_count) {
			g_buffer_count = 0;
		}
	}

	g_buffer_count++;

#ifdef BUF_SAVE_MODE
	/* write pcm buffer */
	fwrite(g_buffer, 1, BUFFER_LENGTH, g_pFile);
#endif

	return EINA_TRUE;
}

int sttd_recorder_start(int engine_id)
{
	if (STTD_RECORDER_STATE_RECORDING == g_recorder_state)
		return 0;

	/* Check engine id is valid */
	stt_recorder_s* recorder;
	recorder = __get_recorder(engine_id);
	if (NULL == recorder) {
		SLOG(LOG_WARN, TAG_STTD, "[Recorder WARNING] Engine id is not valid");
		return STTD_ERROR_INVALID_PARAMETER;
	}

	int ret = -1; 
	ret = audio_in_prepare(recorder->audio_h);
	if (AUDIO_IO_ERROR_NONE != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Recorder ERROR] Fail to start audio : %d", ret);
		return STTD_ERROR_RECORDER_BUSY;
	}

	/* Add ecore timer to read audio data */
	ecore_timer_add(0, __read_audio_func, NULL);

	g_recorder_state = STTD_RECORDER_STATE_RECORDING;
	g_recording_engine_id = engine_id;

	g_pFile_vol = fopen(STT_AUDIO_VOLUME_PATH, "wb+");
	if (!g_pFile_vol) {
		SLOG(LOG_ERROR, TAG_STTD, "[Recorder ERROR] Fail to create Volume File");
		return -1;
	}

	g_buffer_count = 0;

#ifdef BUF_SAVE_MODE
	g_count++;

	snprintf(g_temp_file_name, sizeof(g_temp_file_name), "/tmp/stt_temp_%d_%d", getpid(), g_count);
	SECURE_SLOG(LOG_DEBUG, TAG_STTD, "[Recorder] Temp file name=[%s]", g_temp_file_name);

	/* open test file */
	g_pFile = fopen(g_temp_file_name, "wb+");
	if (!g_pFile) {
		SLOG(LOG_ERROR, TAG_STTD, "[Recorder ERROR] File not found!");
		return -1;
	}	
#endif

	return 0;
}

int sttd_recorder_stop(int engine_id)
{
	if (STTD_RECORDER_STATE_READY == g_recorder_state)
		return 0;

	/* Check engine id is valid */
	stt_recorder_s* recorder;
	recorder = __get_recorder(engine_id);
	if (NULL == recorder) {
		SLOG(LOG_WARN, TAG_STTD, "[Recorder WARNING] Engine id is not valid");
		return STTD_ERROR_INVALID_PARAMETER;
	}

	int ret; 
	ret = audio_in_unprepare(recorder->audio_h);
	if (AUDIO_IO_ERROR_NONE != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Recorder ERROR] Fail to unprepare audioin : %d", ret);
	}

	g_recorder_state = STTD_RECORDER_STATE_READY;
	g_recording_engine_id = -1;

	fclose(g_pFile_vol);

#ifdef BUF_SAVE_MODE
	fclose(g_pFile);
#endif	

	return 0;
}

int sttd_recorder_set_ignore_session(int engine_id)
{
	if (STTD_RECORDER_STATE_READY != g_recorder_state) {
		SLOG(LOG_ERROR, TAG_STTD, "[Recorder ERROR] Record is working.");
		return -1;
	}

	/* Check engine id is valid */
	stt_recorder_s* recorder;
	recorder = __get_recorder(engine_id);
	if (NULL == recorder) {
		SLOG(LOG_WARN, TAG_STTD, "[Recorder WARNING] Engine id is not valid");
		return STTD_ERROR_INVALID_PARAMETER;
	}

	int ret = audio_in_ignore_session(recorder->audio_h);
	if (AUDIO_IO_ERROR_NONE != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Recorder ERROR] Fail to ignore session : %d", ret);
		return STTD_ERROR_OPERATION_FAILED;
	}

	return 0;
}