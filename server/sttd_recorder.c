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


#include <mm_error.h>
#include <mm_player.h>
#include <mm_types.h>
#include <mm_sound.h>
#include <mm_camcorder.h>
#include <mm_session.h>

/* private Header */
#include "sttd_recorder.h"
#include "sttd_main.h"

/* Contant values  */
#define DEF_TIMELIMIT 120
#define DEF_SAMPLERATE 16000
#define DEF_BITRATE_AMR 12200
#define DEF_MAXSIZE 1024 * 1024 * 5
#define DEF_SOURCECHANNEL 1
#define DEF_BUFFER_SIZE 1024

/* Sound buf save */
//#define BUF_SAVE_MODE

typedef struct {
	unsigned int	size_limit;
	unsigned int	time_limit;
	unsigned int	now;
	unsigned int	now_ms;
	unsigned int	frame;
	float		volume;

	/* For MMF */
	unsigned int	bitrate;
	unsigned int	samplerate;    
	sttd_recorder_channel	channel;
	sttd_recorder_state	state;
	sttd_recorder_audio_type	audio_type;

	sttvr_audio_cb	streamcb;

	MMHandleType	rec_handle;
	MMHandleType	ply_handle;
} sttd_recorder_s;


static sttd_recorder_s *g_objRecorer = NULL;
static bool g_init = false;

static char g_temp_file_name[128] = {'\0',};

/* Recorder obj */
sttd_recorder_s *__recorder_getinstance();
void __recorder_state_set(sttd_recorder_state state);

/* MMFW caller */
int __recorder_setup();
int __recorder_run();
int __recorder_pause();
int __recorder_send_buf_from_file();

int __recorder_cancel_to_stop();
int __recorder_commit_to_stop();


/* Event Callback Function */
gboolean _mm_recorder_audio_stream_cb (MMCamcorderAudioStreamDataType *stream, void *user_param)
{
	sttd_recorder_s *pVr = __recorder_getinstance();

	if (stream->length > 0 && stream->data) {
		pVr->frame++;
	
		/* If stream callback is set */
		if (STTD_RECORDER_PCM_S16 == pVr->audio_type || STTD_RECORDER_PCM_U8 == pVr->audio_type) {
#ifdef BUF_SAVE_MODE
			/* write pcm buffer */
			fwrite(stream->data, 1, stream->length, g_pFile);
#else
			if (pVr->streamcb) {
				pVr->streamcb(stream->data, stream->length);
			}
#endif
		} 
	}

	return TRUE;
}


int _camcorder_message_cb (int id, void *param, void *user_param)
{
	MMMessageParamType *m = (MMMessageParamType *)param;

	sttd_recorder_s *pVr = __recorder_getinstance();

	if (pVr) {
		switch(id) {
		case MM_MESSAGE_CAMCORDER_STATE_CHANGED_BY_ASM:
			break;
		case MM_MESSAGE_CAMCORDER_STATE_CHANGED:
			break;
		case MM_MESSAGE_CAMCORDER_MAX_SIZE:
			SLOG(LOG_DEBUG, TAG_STTD, "[Recorder] MM_MESSAGE_CAMCORDER_MAX_SIZE");
			sttd_recorder_stop();
			break;
		case MM_MESSAGE_CAMCORDER_NO_FREE_SPACE:
			SLOG(LOG_DEBUG, TAG_STTD, "[Recorder] MM_MESSAGE_CAMCORDER_NO_FREE_SPACE");
			sttd_recorder_cancel();
			break;
		case MM_MESSAGE_CAMCORDER_TIME_LIMIT:
			SLOG(LOG_DEBUG, TAG_STTD, "[Recorder] MM_MESSAGE_CAMCORDER_TIME_LIMIT");
			sttd_recorder_stop();
			break;
		case MM_MESSAGE_CAMCORDER_ERROR:
			SLOG(LOG_DEBUG, TAG_STTD, "[Recorder] MM_MESSAGE_CAMCORDER_ERROR");
			sttd_recorder_cancel();
			break;
		case MM_MESSAGE_CAMCORDER_RECORDING_STATUS:
			break;
		case MM_MESSAGE_CAMCORDER_CURRENT_VOLUME:
			pVr->volume = m->rec_volume_dB;
			break;
		default:
			SLOG(LOG_DEBUG, TAG_STTD, "[Recorder] Other Message=%d", id);
			break;
		}
	} else {
		return -1;
	}

	return 0;
}


sttd_recorder_s *__recorder_getinstance()
{
	if (!g_objRecorer) {
		g_objRecorer = NULL;
		g_objRecorer = g_malloc0(sizeof(sttd_recorder_s));

		/* set default value */
		g_objRecorer->time_limit = DEF_TIMELIMIT;
		g_objRecorer->size_limit = DEF_MAXSIZE;    
		g_objRecorer->now        = 0;
		g_objRecorer->now_ms     = 0;
		g_objRecorer->frame      = 0;
		g_objRecorer->volume     = 0.0f;
		g_objRecorer->state      = STTD_RECORDER_STATE_READY;
		g_objRecorer->channel    = STTD_RECORDER_CHANNEL_MONO;
		g_objRecorer->audio_type = STTD_RECORDER_PCM_S16;
		g_objRecorer->samplerate = DEF_SAMPLERATE;
		g_objRecorer->streamcb   = NULL;
	}

	return g_objRecorer;
}

void __recorder_state_set(sttd_recorder_state state)
{
	sttd_recorder_s* pVr = __recorder_getinstance();
	pVr->state = state;
}

void __recorder_remove_temp_file()
{
	/* NOTE: temp file can cause permission problem */
	if (0 != access(g_temp_file_name, R_OK|W_OK)) {
		SLOG(LOG_DEBUG, TAG_STTD, "[Recorder] *** You don't have access right to temp file");
	} else {
		if (0 != remove(g_temp_file_name)) {
			SLOG(LOG_ERROR, TAG_STTD, "[Recorder ERROR] Fail to remove temp file");
		}
	}
}


/* MMFW Interface functions */
int __recorder_setup()
{
	sttd_recorder_s *pVr = __recorder_getinstance();

	/* mm-camcorder preset */
	MMCamPreset cam_info;

	int	mmf_ret = MM_ERROR_NONE;
	int	err = 0;
	char*	err_attr_name = NULL;

	cam_info.videodev_type = MM_VIDEO_DEVICE_NONE;

	/* Create camcorder */
	mmf_ret = mm_camcorder_create( &pVr->rec_handle, &cam_info);
	if (MM_ERROR_NONE != mmf_ret) {
		SLOG(LOG_ERROR, TAG_STTD, "[Recorder ERROR] Fail mm_camcorder_create ret=(%X)", mmf_ret);
		return mmf_ret;
	}

	switch (pVr->audio_type) {
	case STTD_RECORDER_PCM_U8:
		SLOG(LOG_DEBUG, TAG_STTD, "[Recorder] STTD_RECORDER_PCM_U8");
		err = mm_camcorder_set_attributes(pVr->rec_handle, 
			&err_attr_name,
			MMCAM_MODE, MM_CAMCORDER_MODE_AUDIO,
			MMCAM_AUDIO_DEVICE, MM_AUDIO_DEVICE_MIC,

			MMCAM_AUDIO_ENCODER, MM_AUDIO_CODEC_AAC, 
			MMCAM_FILE_FORMAT, MM_FILE_FORMAT_3GP, 

			MMCAM_AUDIO_SAMPLERATE, pVr->samplerate,
			MMCAM_AUDIO_FORMAT, MM_CAMCORDER_AUDIO_FORMAT_PCM_U8,
			MMCAM_AUDIO_CHANNEL, pVr->channel,
			MMCAM_AUDIO_INPUT_ROUTE, MM_AUDIOROUTE_CAPTURE_NORMAL,
			NULL );

		if (MM_ERROR_NONE != err) {
			/* Error */
			SLOG(LOG_DEBUG, TAG_STTD, "[Recorder] Fail mm_camcorder_set_attributes ret=(%X)", mmf_ret);
			return err;
		}
		
		break;

	case STTD_RECORDER_PCM_S16:        
		SLOG(LOG_DEBUG, TAG_STTD, "[Recorder] STTD_RECORDER_PCM_S16");
		err = mm_camcorder_set_attributes(pVr->rec_handle, 
			&err_attr_name,
			MMCAM_MODE, MM_CAMCORDER_MODE_AUDIO,
			MMCAM_AUDIO_DEVICE, MM_AUDIO_DEVICE_MIC,
			MMCAM_AUDIO_ENCODER, MM_AUDIO_CODEC_AAC,
			MMCAM_FILE_FORMAT, MM_FILE_FORMAT_3GP,
			MMCAM_AUDIO_SAMPLERATE, pVr->samplerate,
			MMCAM_AUDIO_FORMAT, MM_CAMCORDER_AUDIO_FORMAT_PCM_S16_LE,
			MMCAM_AUDIO_CHANNEL, pVr->channel,
			MMCAM_AUDIO_INPUT_ROUTE, MM_AUDIOROUTE_CAPTURE_NORMAL,
			NULL );

		if (MM_ERROR_NONE != err) {
			/* Error */
			SLOG(LOG_DEBUG, TAG_STTD, "[Recorder] Fail mm_camcorder_set_attributes ret=(%X)", mmf_ret);
			return err;
		}
		break;

	case STTD_RECORDER_AMR:
		SLOG(LOG_DEBUG, TAG_STTD, "[Recorder] STTD_RECORDER_AMR");
		err = mm_camcorder_set_attributes(pVr->rec_handle, 
			&err_attr_name,
			MMCAM_MODE, MM_CAMCORDER_MODE_AUDIO,
			MMCAM_AUDIO_DEVICE, MM_AUDIO_DEVICE_MIC,

			MMCAM_AUDIO_ENCODER, MM_AUDIO_CODEC_AMR,
			MMCAM_FILE_FORMAT, MM_FILE_FORMAT_AMR,

			MMCAM_AUDIO_SAMPLERATE, pVr->samplerate,
			MMCAM_AUDIO_CHANNEL, pVr->channel,

			MMCAM_AUDIO_INPUT_ROUTE, MM_AUDIOROUTE_CAPTURE_NORMAL,
			MMCAM_TARGET_TIME_LIMIT, pVr->time_limit,
			MMCAM_TARGET_FILENAME, g_temp_file_name, strlen(g_temp_file_name)+1,
			NULL );

		if (MM_ERROR_NONE != err) {
			/* Error */
			SLOG(LOG_ERROR, TAG_STTD, "[Recorder ERROR] Fail mm_camcorder_set_attributes ret=(%X)", mmf_ret);
			return err;
		}
		break;

	default:
		SLOG(LOG_DEBUG, TAG_STTD, "[Recorder ERROR]");
		return -1;
		break;
	}

	mmf_ret = mm_camcorder_set_audio_stream_callback(pVr->rec_handle, (mm_camcorder_audio_stream_callback)_mm_recorder_audio_stream_cb, NULL);
	if (MM_ERROR_NONE != err) {
		/* Error */
		SLOG(LOG_ERROR, TAG_STTD, "[Recorder ERROR] Fail mm_camcorder_set_audio_stream_callback ret=(%X)", mmf_ret);
		return err;
	}
	
	mmf_ret = mm_camcorder_set_message_callback(pVr->rec_handle, (MMMessageCallback)_camcorder_message_cb, pVr);
	if (MM_ERROR_NONE != err) {
		/* Error */
		SLOG(LOG_ERROR, TAG_STTD, "[Recorder ERROR] Fail mm_camcorder_set_message_callback ret=(%X)", mmf_ret);
		return err;
	}

	mmf_ret = mm_camcorder_realize(pVr->rec_handle);
	if (MM_ERROR_NONE != err) {
		/* Error */
		SLOG(LOG_DEBUG, TAG_STTD, "[Recorder] Fail mm_camcorder_realize=(%X)", mmf_ret);
		return err;
	}

	/* Camcorder start */
	mmf_ret = mm_camcorder_start(pVr->rec_handle);
	if (MM_ERROR_NONE != mmf_ret) {
		SLOG(LOG_DEBUG, TAG_STTD, "[Recorder] Fail mm_camcorder_start=(%X)", mmf_ret);
		return mmf_ret;
	}

	SLOG(LOG_DEBUG, TAG_STTD, " - size_limit=%3d", pVr->size_limit);
	SLOG(LOG_DEBUG, TAG_STTD, " - time_limit=%3d", pVr->time_limit);
	SLOG(LOG_DEBUG, TAG_STTD, " - Audio Type=%d", pVr->audio_type);
	SLOG(LOG_DEBUG, TAG_STTD, " - Sample rates=%d", pVr->samplerate);
	SLOG(LOG_DEBUG, TAG_STTD, " - channel=%d", pVr->channel);	

	return 0;
}

int __recorder_run()
{
	sttd_recorder_s *pVr = __recorder_getinstance();
	int	mmf_ret = MM_ERROR_NONE;

	/* If recorder already has recording state, cancel */
	if (STTD_RECORDER_STATE_RECORDING == pVr->state) {
		SLOG(LOG_DEBUG, TAG_STTD, "[Recorder] Stop recording first");
		__recorder_cancel_to_stop();        
	}
	
	/* Reset frame number */
	pVr->frame = 0;

	/* Record start */
	mmf_ret = mm_camcorder_record(pVr->rec_handle);
	if(MM_ERROR_NONE != mmf_ret ) {
		/* Error */
		SLOG(LOG_DEBUG, TAG_STTD, "[Recorder] Fail mm_camcorder_record=(%X)", mmf_ret);
		return mmf_ret;        
	}
	SLOG(LOG_DEBUG, TAG_STTD, "[Recorder] Success mm_camcorder_record");

	return 0;
}

int __recorder_pause()
{
	sttd_recorder_s *pVr = __recorder_getinstance();
	int mmf_ret = MM_ERROR_NONE;
	MMCamcorderStateType state_now = MM_CAMCORDER_STATE_NONE;

	/* Get state from MMFW */
	mmf_ret = mm_camcorder_get_state(pVr->rec_handle, &state_now);
	if(mmf_ret != MM_ERROR_NONE ) {
		SLOG(LOG_DEBUG, TAG_STTD, "[Recorder] Fail to get state : mm_camcorder_get_state");
		return mmf_ret;
	}

	/* Check recording state */
	if(MM_CAMCORDER_STATE_RECORDING != state_now) {
		SLOG(LOG_DEBUG, TAG_STTD, "[Recorder] Not recording state");
		return mmf_ret;
	}

	/* Pause recording */
	mmf_ret = mm_camcorder_pause(pVr->rec_handle);
	if(mmf_ret == MM_ERROR_NONE ) {
		SLOG(LOG_DEBUG, TAG_STTD, "[Recorder] mm_camcorder_pause OK");
		return mmf_ret;
	}

	return 0;
}

int __recorder_cancel_to_stop()
{
	sttd_recorder_s *pVr = __recorder_getinstance();
	int	mmf_ret = MM_ERROR_NONE;
	MMCamcorderStateType rec_status = MM_CAMCORDER_STATE_NONE;

	/* Cancel camcorder */
	mmf_ret = mm_camcorder_cancel(pVr->rec_handle);
	if(mmf_ret != MM_ERROR_NONE ) {
		SLOG(LOG_DEBUG, TAG_STTD, "[Recorder] Fail to mm_camcorder_cancel");
		return -1;
	}

	/* Stop camcorder */
	mmf_ret = mm_camcorder_stop(pVr->rec_handle);
	if(mmf_ret != MM_ERROR_NONE ) {
		SLOG(LOG_DEBUG, TAG_STTD, "[Recorder] Fail to mm_camcorder_stop");
		return -1;    
	}

	/* Release resouces */
	mm_camcorder_get_state(pVr->rec_handle, &rec_status);
	if (MM_CAMCORDER_STATE_READY == rec_status) {
		mmf_ret = mm_camcorder_unrealize(pVr->rec_handle);
		SLOG(LOG_DEBUG, TAG_STTD, "[Recorder] Call mm_camcorder_unrealize ret=(%X)", mmf_ret);
	}

	return 0;
}


int __recorder_commit_to_stop()
{
	sttd_recorder_s *pVr = __recorder_getinstance();
	int	mmf_ret = MM_ERROR_NONE;
	MMCamcorderStateType rec_status = MM_CAMCORDER_STATE_NONE;    

	/* Commit camcorder */
	mmf_ret = mm_camcorder_commit(pVr->rec_handle);
	if(mmf_ret != MM_ERROR_NONE ) {
		SLOG(LOG_DEBUG, TAG_STTD, "[Recorder] Fail mm_camcorder_commit=%x", mmf_ret);
	}

	/* Stop camcorder */
	mmf_ret = mm_camcorder_stop(pVr->rec_handle);
	if(mmf_ret != MM_ERROR_NONE ) {
		SLOG(LOG_DEBUG, TAG_STTD, "[Recorder] Fail mm_camcorder_stop=%x", mmf_ret);
	}

	/* Release resouces */
	mm_camcorder_get_state(pVr->rec_handle, &rec_status);
	if (MM_CAMCORDER_STATE_READY == rec_status) {
		mmf_ret = mm_camcorder_unrealize(pVr->rec_handle);
		SLOG(LOG_DEBUG, TAG_STTD, "[Recorder] Call mm_camcorder_unrealize ret=(%X)", mmf_ret);
	}

	return 0;
}


int __recorder_send_buf_from_file()
{
	sttd_recorder_s *pVr = __recorder_getinstance();

	FILE * pFile;
	if (!pVr->streamcb) {
		SLOG(LOG_ERROR, TAG_STTD, "[Recorder ERROR] Return callback is not set");
		return -1;
	}

	if (STTD_RECORDER_AMR != pVr->audio_type) {
#ifndef BUF_SAVE_MODE
		return 0;
#else 
		fclose(g_pFile);
#endif		
	} 

	pFile = fopen(g_temp_file_name, "rb");
	if (!pFile) {
		SLOG(LOG_ERROR, TAG_STTD, "[Recorder ERROR] File not found!");
		return -1;
	}

	char buff[1024];
	size_t read_size = 0;
	int ret = 0;
	
	while (!feof(pFile)) {
		read_size = fread(buff, 1, 1024, pFile);
		if (read_size > 0) {
			ret = pVr->streamcb((void*)buff, read_size);

			if(ret != 0) {
				SLOG(LOG_ERROR, TAG_STTD, "[Recorder ERROR] Fail to set recording");
				break;
			}
		}
	}

	fclose(pFile);

	return 0;
}

int __vr_mmcam_destroy()
{
	int err = 0;
	sttd_recorder_s *pVr = __recorder_getinstance();

	MMCamcorderStateType rec_status = MM_CAMCORDER_STATE_NONE;

	mm_camcorder_get_state(pVr->rec_handle, &rec_status);
	if (rec_status == MM_CAMCORDER_STATE_NULL) {
		err = mm_camcorder_destroy(pVr->rec_handle);

		if (MM_ERROR_NONE == err) {
			SLOG(LOG_DEBUG, TAG_STTD, "[Recorder] mm_camcorder_destroy OK");
			pVr->rec_handle = 0;
		} else {
			SLOG(LOG_ERROR, TAG_STTD, "[Recorder ERROR] Error mm_camcorder_destroy %x", err);            
		}

	}

	return 0;
}


/* External functions */
int sttd_recorder_init()
{
	/* Create recorder instance     */
	sttd_recorder_s *pVr = __recorder_getinstance();
	if (!pVr) {
		SLOG(LOG_DEBUG, TAG_STTD, "[Recorder] Fail to initialize voice recorder!");
		return -1;    
	}
	SLOG(LOG_DEBUG, TAG_STTD, "[Recorder] Voice Recorder Initialized p=%p", pVr);

	/* Set temp file name */
	snprintf(g_temp_file_name, sizeof(g_temp_file_name), "/tmp/stt_temp_%d", getpid());
	SLOG(LOG_DEBUG, TAG_STTD, "[Recorder] Temp file name=[%s]", g_temp_file_name);

	g_init = true;

	return 0;
}

int sttd_recorder_set(sttd_recorder_audio_type type, sttd_recorder_channel ch, unsigned int sample_rate, 
		      unsigned int max_time, sttvr_audio_cb cbfunc)
{
	sttd_recorder_s *pVr = __recorder_getinstance();
	int ret = 0;

	if (STTD_RECORDER_STATE_RECORDING == pVr->state) 
		__recorder_cancel_to_stop();

	if (STTD_RECORDER_STATE_READY != pVr->state) 
		__vr_mmcam_destroy();

	/* Set attributes */
	pVr->audio_type = type;
	pVr->channel    = ch;
	pVr->samplerate = sample_rate;
	pVr->time_limit = max_time;

	/* Stream data Callback function */
	if (cbfunc)
		pVr->streamcb = cbfunc;

	return ret;
}

int sttd_recorder_start()
{
	int ret = 0;

	__recorder_remove_temp_file();

#ifdef BUF_SAVE_MODE
	sttd_recorder_s *pVr = __recorder_getinstance();
	if (!pVr) {
		SLOG(LOG_DEBUG, TAG_STTD, "[Recorder] Fail to initialize voice recorder!");
		return -1;    
	}

	if (STTD_RECORDER_AMR != pVr->audio_type) {
		/* open test file */
		g_pFile = fopen(g_temp_file_name, "wb+");
		if (!g_pFile) {
			SLOG(LOG_ERROR, TAG_STTD, "[Recorder ERROR] File not found!");
			return -1;
		}	
	}
#endif	

	/* Check if initialized */
	ret = __recorder_setup();
	if (0 != ret) {
		SLOG(LOG_DEBUG, TAG_STTD, "[Recorder] Fail to call __recorder_setup");
		return STTD_ERROR_OPERATION_FAILED;
	}

	/* Start camcorder */
	ret = __recorder_run();
	if (0 != ret) {
		SLOG(LOG_DEBUG, TAG_STTD, "[Recorder] Fail to call __recorder_run");    
		return STTD_ERROR_OPERATION_FAILED;
	}

	__recorder_state_set(STTD_RECORDER_STATE_RECORDING);

	return 0;
}


int sttrecorder_pause()
{
	int ret = 0;

	ret = __recorder_pause();
	if (ret) {
		SLOG(LOG_DEBUG, TAG_STTD, "[Recorder] Fail to call __recorder_pause");
		return -1;
	}

	/* Set state */
	__recorder_state_set(STTD_RECORDER_STATE_PAUSED);    

	return 0;
}

int sttd_recorder_cancel()
{
	int ret = 0;    
	ret = __recorder_cancel_to_stop();
	if (ret) {
		SLOG(LOG_DEBUG, TAG_STTD, "[Recorder] Fail to call __recorder_cancel_to_stop");
		return -1;
	}

	ret = __vr_mmcam_destroy();
	if (ret) {
		SLOG(LOG_DEBUG, TAG_STTD, "[Recorder] Fail to call __vr_mmcam_destroy");
		return -1;
	}      

	/* Set state */
	__recorder_state_set(STTD_RECORDER_STATE_READY);    

	return 0;
}


int sttd_recorder_stop()
{
	int ret = 0;

	ret = __recorder_commit_to_stop();
	if (ret) {
		SLOG(LOG_DEBUG, TAG_STTD, "[Recorder] Fail to call __recorder_commit_to_stop");
		return -1;
	}

	ret = __recorder_send_buf_from_file();
	if (ret) {
		SLOG(LOG_DEBUG, TAG_STTD, "[Recorder] Fail to call __recorder_send_buf_from_file");
		return -1;
	}    

	ret = __vr_mmcam_destroy();
	if (ret) {
		SLOG(LOG_DEBUG, TAG_STTD, "[Recorder] Fail to call __vr_mmcam_destroy");
		return -1;
	}

	__recorder_state_set(STTD_RECORDER_STATE_READY);

	return 0;
}


int sttd_recorder_destroy()
{
	/* Destroy recorder object */
	if (g_objRecorer)
		g_free(g_objRecorer);

	g_objRecorer = NULL;
	
	__recorder_state_set(STTD_RECORDER_STATE_READY);

	return 0;
}


int sttd_recorder_state_get(sttd_recorder_state* state)
{
	sttd_recorder_s *pVr = __recorder_getinstance();
	if (!pVr) {
		SLOG(LOG_DEBUG, TAG_STTD, "[Recorder] Fail to get instance"); 
		return -1;
	}

	*state = pVr->state;

	return 0;
}


int sttd_recorder_get_volume(float *vol)
{
	sttd_recorder_state state;

	sttd_recorder_s *pVr = __recorder_getinstance();
	if (!pVr) {
		SLOG(LOG_DEBUG, TAG_STTD, "[Recorder] Fail to get instance"); 
		return -1;
	}

	sttd_recorder_state_get(&state);
	if (STTD_RECORDER_STATE_RECORDING != state) {
		SLOG(LOG_ERROR, TAG_STTD, "[Recorder ERROR] Not in Recording state");
		return -1;
	}
	*vol = pVr->volume;

	return 0;
}
