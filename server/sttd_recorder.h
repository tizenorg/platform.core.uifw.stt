/*
*  Copyright (c) 2011-2016 Samsung Electronics Co., Ltd All Rights Reserved
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

#include "stte.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef int (*stt_recorder_audio_cb)(const void* data, const unsigned int length);

typedef void (*stt_recorder_interrupt_cb)();

int sttd_recorder_initialize(stt_recorder_audio_cb audio_cb, stt_recorder_interrupt_cb interrupt_cb);

int sttd_recorder_deinitialize();

int sttd_recorder_set_audio_session();

int sttd_recorder_unset_audio_session();

int sttd_recorder_create(stte_audio_type_e type, int channel, unsigned int sample_rate);

int sttd_recorder_destroy();

int sttd_recorder_start(int uid);

int sttd_recorder_stop();


#ifdef __cplusplus
}
#endif

#endif	/* __STTD_RECORDER_H__ */

