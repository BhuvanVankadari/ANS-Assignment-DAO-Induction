/*
 * Copyright (c) 2017, RISE SICS
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

#include "contiki.h"
#include "net/routing/rpl-lite/rpl.h"
#include "net/routing/rpl-lite/rpl-icmp6.h"
#include "net/routing/rpl-lite/rpl-dag.h"
#include "sys/clock.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "RPL BR"
#define LOG_LEVEL LOG_LEVEL_INFO

/*
 * DAO induction detection.
 *
 * The root tracks the last DAO sequence number seen from each node.
 * If a node's seqno changes without the root having recently incremented
 * its own DTSN, that DAO was not legitimately solicited and is flagged.
 */
#ifndef DAO_DETECTION_TABLE_SIZE
#define DAO_DETECTION_TABLE_SIZE 16
#endif

/* How long after a root DTSN increment we consider DAO seqno changes normal. */
#ifndef DAO_DETECTION_GRACE
#define DAO_DETECTION_GRACE (30 * CLOCK_SECOND)
#endif

typedef struct {
  uint8_t      used;
  uip_ipaddr_t addr;
  uint8_t      last_seqno;
} dao_detection_entry_t;

static dao_detection_entry_t dao_detection_table[DAO_DETECTION_TABLE_SIZE];

/*---------------------------------------------------------------------------*/
static void
on_dao_received(const uip_ipaddr_t *from, uint8_t seqno)
{
  int i;
  int free_idx = -1;
  clock_time_t now = clock_time();
  clock_time_t last_root_update = rpl_dag_last_root_dtsn_update();
  int root_recently_triggered =
    (last_root_update != 0) &&
    ((now - last_root_update) <= DAO_DETECTION_GRACE);

  for(i = 0; i < DAO_DETECTION_TABLE_SIZE; i++) {
    if(dao_detection_table[i].used) {
      if(uip_ipaddr_cmp(&dao_detection_table[i].addr, from)) {
        if(!root_recently_triggered && dao_detection_table[i].last_seqno != seqno) {
          LOG_WARN("DAO induction suspected: unexpected seqno change from ");
          LOG_WARN_6ADDR(from);
          LOG_WARN_(" (seqno %u -> %u) while root did not recently increment DTSN\n",
                   dao_detection_table[i].last_seqno, seqno);
        }
        dao_detection_table[i].last_seqno = seqno;
        return;
      }
    } else if(free_idx < 0) {
      free_idx = i;
    }
  }

  /* First DAO from this node — record it, never flag the first. */
  if(free_idx >= 0) {
    dao_detection_table[free_idx].used = 1;
    uip_ipaddr_copy(&dao_detection_table[free_idx].addr, from);
    dao_detection_table[free_idx].last_seqno = seqno;
  }
}

/* Declare and auto-start this file's process */
PROCESS(contiki_ng_br, "Contiki-NG Border Router");
AUTOSTART_PROCESSES(&contiki_ng_br);

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(contiki_ng_br, ev, data)
{
  PROCESS_BEGIN();

#if BORDER_ROUTER_CONF_WEBSERVER
  PROCESS_NAME(webserver_nogui_process);
  process_start(&webserver_nogui_process, NULL);
#endif /* BORDER_ROUTER_CONF_WEBSERVER */

  rpl_icmp6_set_dao_callback(on_dao_received);

  LOG_INFO("Contiki-NG Border Router started\n");

  PROCESS_END();
}
