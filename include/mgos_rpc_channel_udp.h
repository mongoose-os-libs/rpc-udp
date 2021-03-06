/*
 * Copyright (c) 2014-2019 Cesanta Software Limited
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <stdbool.h>

#include "mg_rpc_channel.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mg_rpc_channel *mg_rpc_ch_udp_in(const char *listen_addr);

struct mg_rpc_channel *mg_rpc_ch_udp_out(const char *dst_addr);

#ifdef __cplusplus
}
#endif
