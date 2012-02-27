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


#ifndef __STTD_RECORDER_H__
#define __STTD_RECORDER_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	STTD_RECORDER_STATE_READY,	/**< Recorder is ready to start */
	STTD_RECORDER_STATE_RECORDING,	/**< In the middle of recording */
	STTD_RECORDER_STATE_PAUSED
} sttd_recorder_state;

typedef enum {
	STTD_RECORDER_PCM_S16,		/**< PCM, signed 16-bit */
	STTD_RECORDER_PCM_U8,		/**< PCM, unsigned 8-bit */
	STTD_RECORDER_AMR		/**< AMR (Callback will be invoked after recording) */
} sttd_recorder_audio_type;

typedef enum {
	STTD_RECORDER_CHANNEL_MONO	= 1,	/**< Mono channel : Default value */
	STTD_RECORDER_CHANNEL_STEREO	= 2	/**< Stereo */
} sttd_recorder_channel;


typedef int (*sttvr_audio_cb)(const void* data, const unsigned int length);
typedef int (*sttvr_volume_data_cb)(const float data);

int sttd_recorder_set(sttd_recorder_audio_type type, sttd_recorder_channel ch, unsigned int sample_rate, unsigned int max_time, sttvr_audio_cb cbfunc);

int sttd_recorder_init();

int sttd_recorder_start();

int sttd_recorder_cancel();

int sttd_recorder_stop();

int sttrecorder_pause();

int sttd_recorder_state_get(sttd_recorder_state* state);

int sttd_recorder_get_volume(float *vol);

int sttd_recorder_destroy();

#ifdef __cplusplus
}
#endif

#endif	/* __STTD_RECORDER_H__ */

