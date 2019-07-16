/*
 * Copyright (c) 2019 Deomid "rojer" Ryabkov
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mgos_rpc_channel_udp.h"

#include "mgos_rpc.h"

bool mgos_rpc_udp_init(void) {
  const struct mgos_config_rpc *sccfg = mgos_sys_config_get_rpc();
  if (mgos_rpc_get_global() == NULL) return true;
  if (sccfg->udp.listen_addr != NULL) {
    struct mg_rpc_channel *ch = mg_rpc_ch_udp_in(sccfg->udp.listen_addr);
    if (ch == NULL) return false;
    mg_rpc_add_channel(mgos_rpc_get_global(), mg_mk_str(NULL), ch);
    ch->ch_connect(ch);
  }
  if (sccfg->udp.dst_addr != NULL) {
    struct mg_rpc_channel *ch = mg_rpc_ch_udp_out(sccfg->udp.dst_addr);
    if (ch == NULL) return false;
    mg_rpc_add_channel(mgos_rpc_get_global(), mg_mk_str(sccfg->udp.dst), ch);
    ch->ch_connect(ch);
  }
  return true;
}
