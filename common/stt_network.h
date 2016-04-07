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


#ifndef __STT_NETWORK_H_
#define __STT_NETWORK_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef LIBSCL_EXPORT_API
#define LIBSCL_EXPORT_API
#endif // LIBSCL_EXPORT_API


LIBSCL_EXPORT_API int stt_network_initialize();

LIBSCL_EXPORT_API int stt_network_finalize();

LIBSCL_EXPORT_API bool stt_network_is_connected();


#ifdef __cplusplus
}
#endif


#endif	/* __STT_NETWORK_H_ */
