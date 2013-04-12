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

#include <audio_io.h> 
#include <Ecore.h>
#include <math.h>
#include "sttd_recorder.h"
#include "sttd_main.h"
#include "stt_defs.h"


#define FRAME_LENGTH 160
#define BUFFER_LENGTH FRAME_LENGTH * 2

static stt_recorder_audio_cb	g_audio_cb;
static audio_in_h		g_audio_in_h;

static sttd_recorder_state	g_recorder_state = -1;

static sttp_audio_type_e	g_audio_type;

static char g_buffer[BUFFER_LENGTH];

static FILE* g_pFile_vol;

static char g_temp_vol[64];

/* Sound buf save */
/*
#define BUF_SAVE_MODE
*/

#ifdef BUF_SAVE_MODE
static char g_temp_file_name[128] = {'\0',};

static FILE* g_pFile;

static int g_count = 1;
#endif 

int sttd_recorder_create(stt_recorder_audio_cb callback, sttp_audio_type_e type, int channel, unsigned int sample_rate)
{
	if (NULL == callback) {
		SLOG(LOG_ERROR, TAG_STTD, "[Recorder ERROR] Input param is NOT valid");
		return STTD_ERROR_INVALID_PARAMETER;
	}

	audio_channel_e audio_ch;
	audio_sample_type_e audio_type;

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
	ret = audio_in_create(sample_rate, audio_ch, audio_type, &g_audio_in_h);
	if (AUDIO_IO_ERROR_NONE != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Recorder ERROR] Fail to create audio handle : %d", ret);
		return STTD_ERROR_OPERATION_FAILED;
	}

	g_audio_cb = callback;
	g_recorder_state = STTD_RECORDER_STATE_READY;
	g_audio_type = type;

	return 0;
}

int sttd_recorder_destroy()
{
	int ret;
	if (STTD_RECORDER_STATE_RECORDING == g_recorder_state) {
		ret = audio_in_unprepare(g_audio_in_h);
		if (AUDIO_IO_ERROR_NONE != ret) {
			SLOG(LOG_ERROR, TAG_STTD, "[Recorder ERROR] Fail to unprepare audio_in");
		}
	}

	ret = audio_in_destroy(g_audio_in_h);
	if (AUDIO_IO_ERROR_NONE != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Recorder ERROR] Fail to destroy audio_in");
	}

	g_audio_cb = NULL;
	g_recorder_state = -1;

	if (0 == access(STT_AUDIO_VOLUME_PATH, R_OK)) {
		if (0 == remove(STT_AUDIO_VOLUME_PATH)) {
			SLOG(LOG_WARN, TAG_STTD, "[Recorder WARN] Fail to remove volume file"); 
		}
	}

	return 0;
}

static float get_volume_decibel(char* data, int size, sttp_audio_type_e type)
{
	#define MAX_AMPLITUDE_MEAN_16 23170.115738161934
	#define MAX_AMPLITUDE_MEAN_08    89.803909382810

	int i, depthByte;
	int count = 0;

	short* pcm16 = 0;
	char* pcm8 = 0;

	float db = 0.0;
	float rms = 0.0;
	unsigned long long square_sum = 0;

	if (type == STTP_AUDIO_TYPE_PCM_S16_LE)
		depthByte = 2;
	else
		depthByte = 1;

	for (i = 0; i < size; i += (depthByte<<1)) {
		if (depthByte == 2) {
			pcm16 = (short*)(data + i);
			square_sum += (*pcm16) * (*pcm16);
		} else {
			pcm8 = (char*)(data +i);
			square_sum += (*pcm8) * (*pcm8);
		}
		count++;
	}

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

	if (STTD_RECORDER_STATE_READY == g_recorder_state) {
		SLOG(LOG_DEBUG, TAG_STTD, "[Recorder] Exit audio reading func");
		return EINA_FALSE;
	}

	read_byte = audio_in_read(g_audio_in_h, g_buffer, BUFFER_LENGTH);
	if (0 > read_byte) {
		SLOG(LOG_WARN, TAG_STTD, "[Recorder WARNING] Fail to read audio : %d", read_byte);
		g_recorder_state = STTD_RECORDER_STATE_READY;
		return EINA_FALSE;
	}

	if (0 != g_audio_cb(g_buffer, read_byte)) {
		SLOG(LOG_WARN, TAG_STTD, "[Recorder WARNING] Fail audio callback");
		sttd_recorder_stop();
		return EINA_FALSE;
	}

	float vol_db = get_volume_decibel(g_buffer, BUFFER_LENGTH, g_audio_type);

	rewind(g_pFile_vol);
	fwrite((void*)(&vol_db), sizeof(vol_db), 1, g_pFile_vol);

#ifdef BUF_SAVE_MODE
	/* write pcm buffer */
	fwrite(g_buffer, 1, BUFFER_LENGTH, g_pFile);
#endif

	return EINA_TRUE;
}

int sttd_recorder_start()
{
	if (STTD_RECORDER_STATE_RECORDING == g_recorder_state)
		return 0;

	int ret = -1; 
	ret = audio_in_prepare(g_audio_in_h);
	if (AUDIO_IO_ERROR_NONE != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Recorder ERROR] Fail to prepare audio in : %d", ret);
		return STTD_ERROR_OPERATION_FAILED;
	}

	/* Add ecore timer to read audio data */
	ecore_timer_add(0, __read_audio_func, NULL);

	g_recorder_state = STTD_RECORDER_STATE_RECORDING;

	g_pFile_vol = fopen(STT_AUDIO_VOLUME_PATH, "wb+");
	if (!g_pFile_vol) {
		SLOG(LOG_ERROR, TAG_STTD, "[Recorder ERROR] Fail to create Volume File");
		return -1;
	}


#ifdef BUF_SAVE_MODE
	g_count++;

	snprintf(g_temp_file_name, sizeof(g_temp_file_name), "/tmp/stt_temp_%d_%d", getpid(), g_count);
	SLOG(LOG_DEBUG, TAG_STTD, "[Recorder] Temp file name=[%s]", g_temp_file_name);

	/* open test file */
	g_pFile = fopen(g_temp_file_name, "wb+");
	if (!g_pFile) {
		SLOG(LOG_ERROR, TAG_STTD, "[Recorder ERROR] File not found!");
		return -1;
	}	
#endif	

	return 0;
}

int sttd_recorder_stop()
{
	if (STTD_RECORDER_STATE_READY == g_recorder_state)
		return 0;

	g_recorder_state = STTD_RECORDER_STATE_READY;

	int ret = STTD_ERROR_OPERATION_FAILED; 
	ret = audio_in_unprepare(g_audio_in_h);
	if (AUDIO_IO_ERROR_NONE != ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Recorder ERROR] Fail to unprepare audio_in : %d", ret);
	}

	fclose(g_pFile_vol);

#ifdef BUF_SAVE_MODE
	fclose(g_pFile);
#endif	

	SLOG(LOG_ERROR, TAG_STTD, "[Recorder] Recorder stop success");
	return 0;
}