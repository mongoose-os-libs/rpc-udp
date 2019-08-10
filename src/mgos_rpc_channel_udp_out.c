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

#include "mgos.h"
#include "mgos_rpc.h"
#include "mgos_sys_config.h"

#include "mongoose.h"

struct mg_rpc_ch_udp_out_data {
  struct mg_str dst_addr;
  struct mg_connection *nc;
};

static void mg_rpc_ch_udp_out_ch_connect(struct mg_rpc_channel *ch) {
  (void) ch;
}

static bool mg_rpc_ch_udp_out_send_frame(struct mg_rpc_channel *ch,
                                         const struct mg_str f) {
  struct mg_rpc_ch_udp_out_data *chd =
      (struct mg_rpc_ch_udp_out_data *) ch->channel_data;
  if (chd->nc == NULL) return false;
  // Can only send one datagram at a time.
  if (chd->nc->send_mbuf.len > 0) return false;
  mg_send(chd->nc, f.p, f.len);
  return true;
}

static void mg_rpc_ch_udp_out_ch_close(struct mg_rpc_channel *ch) {
  struct mg_rpc_ch_udp_out_data *chd =
      (struct mg_rpc_ch_udp_out_data *) ch->channel_data;
  if (chd->nc == NULL) return;
  chd->nc->user_data = NULL;
  chd->nc->flags |= MG_F_SEND_AND_CLOSE;
  chd->nc = NULL;
}

static void mg_rpc_ch_udp_out_ch_destroy(struct mg_rpc_channel *ch) {
  struct mg_rpc_ch_udp_out_data *chd =
      (struct mg_rpc_ch_udp_out_data *) ch->channel_data;
  mg_strfree(&chd->dst_addr);
  free(chd);
  free(ch);
}

static const char *mg_rpc_ch_udp_out_get_type(struct mg_rpc_channel *ch) {
  (void) ch;
  return "UDP_out";
}

static bool mg_rpc_ch_udp_out_get_authn_info(struct mg_rpc_channel *ch,
                                             const char *auth_domain,
                                             const char *auth_file,
                                             struct mg_rpc_authn_info *authn) {
  (void) ch;
  (void) auth_domain;
  (void) auth_file;
  (void) authn;
  return false;
}

static char *mg_rpc_ch_udp_out_get_info(struct mg_rpc_channel *ch) {
  struct mg_rpc_ch_udp_out_data *chd =
      (struct mg_rpc_ch_udp_out_data *) ch->channel_data;
  char buf[64] = {0};
  char *res = NULL;
  if (chd->nc != NULL) {
    mg_conn_addr_to_str(chd->nc, buf, sizeof(buf), MG_SOCK_STRINGIFY_PORT);
  }
  mg_asprintf(&res, 0, "%s", buf);
  return res;
}

static void mg_rpc_ch_udp_out_ev(struct mg_connection *nc, int ev,
                                 void *ev_data, void *user_data) {
  struct mg_rpc_channel *ch = (struct mg_rpc_channel *) user_data;
  if (ch == NULL) return;
  struct mg_rpc_ch_udp_out_data *chd =
      (struct mg_rpc_ch_udp_out_data *) ch->channel_data;
  switch (ev) {
    case MG_EV_CONNECT: {
      if (*((int *) ev_data) == 0) {
        ch->ev_handler(ch, MG_RPC_CHANNEL_OPEN, NULL);
      } else {
        ch->ev_handler(ch, MG_RPC_CHANNEL_CLOSED, NULL);
      }
      break;
    }
    case MG_EV_RECV: {
      struct mg_str f =
          mg_mk_str_n((const char *) nc->recv_mbuf.buf, nc->recv_mbuf.len);
      ch->ev_handler(ch, MG_RPC_CHANNEL_FRAME_RECD, &f);
      mbuf_clear(&nc->recv_mbuf);
      break;
    }
    case MG_EV_SEND: {
      ch->ev_handler(ch, MG_RPC_CHANNEL_FRAME_SENT, NULL);
      break;
    }
    case MG_EV_CLOSE: {
      if (nc != chd->nc) break;
      nc->user_data = NULL;
      chd->nc = NULL;
      ch->ev_handler(ch, MG_RPC_CHANNEL_CLOSED, NULL);
      chd->nc = mg_connect(mgos_get_mgr(), chd->dst_addr.p, mg_rpc_ch_udp_out_ev, ch);
      break;
    }
  }
  (void) ev_data;
}

struct mg_rpc_channel *mg_rpc_ch_udp_out(const char *dst_addr) {
  char *addr = NULL;
  struct mg_rpc_channel *ch = NULL;
  struct mg_rpc_ch_udp_out_data *chd = NULL;
  if (dst_addr == NULL) return NULL;
  if (!mg_str_starts_with(mg_mk_str(dst_addr), mg_mk_str("udp://"))) {
    mg_asprintf(&addr, 0, "udp://%s", dst_addr);
    if (addr == NULL) goto out;
    dst_addr = addr;
  }
  struct mg_connection *nc =
      mg_connect(mgos_get_mgr(), dst_addr, mg_rpc_ch_udp_out_ev, NULL);
  if (nc == NULL) {
    LOG(LL_ERROR, ("Failed to connect to %s", dst_addr));
    goto out;
  }
  chd = (struct mg_rpc_ch_udp_out_data *) calloc(1, sizeof(*chd));
  if (chd == NULL) goto out;
  chd->nc = nc;
  chd->dst_addr = mg_strdup_nul(mg_mk_str(dst_addr));
  ch = (struct mg_rpc_channel *) calloc(1, sizeof(*ch));
  if (ch == NULL) goto out;
  nc->user_data = ch;
  ch->ch_connect = mg_rpc_ch_udp_out_ch_connect;
  ch->send_frame = mg_rpc_ch_udp_out_send_frame;
  ch->ch_close = mg_rpc_ch_udp_out_ch_close;
  ch->ch_destroy = mg_rpc_ch_udp_out_ch_destroy;
  ch->get_type = mg_rpc_ch_udp_out_get_type;
  ch->is_persistent = mg_rpc_channel_true;
  ch->is_broadcast_enabled = mg_rpc_channel_true;
  ch->get_authn_info = mg_rpc_ch_udp_out_get_authn_info;
  ch->get_info = mg_rpc_ch_udp_out_get_info;
  ch->channel_data = chd;
  LOG(LL_INFO, ("%p UDP to %s", ch, dst_addr));

out:
  if (ch == NULL) free(chd);
  free(addr);
  return ch;
}
