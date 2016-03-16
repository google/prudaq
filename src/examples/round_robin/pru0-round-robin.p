// -*- mode: asm -*-
/*
Copyright 2015 Google Inc. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
implied.  See the License for the specific language governing
permissions and limitations under the License.
*/

/* This code runs on PRU0.  It generates a GPIO clock while alternating
 * the analog switches between inputs 0 and 4, and inputs 1 and 5.
 * See doc/Performance.md for more discussion on timing considerations
 * relating to ADC sampling and switching the analog switches. */

.origin 0
.entrypoint TOP

#include "shared_header.h"

// How many PRU clock cycles == half a cycle of our ADC clock
#define HALF_CYCLE_COUNT 25

#define COUNT_H    r20
#define COUNT_L    r21
#define SHARED_RAM r29
#define PAUSE_COUNT r22

.macro NOP
  add r0, r0, 0
.endm

// Pauses for n cycles (+/- 1).  n must be >= 6
.macro PAUSE
.mparam count
  mov PAUSE_COUNT, count
  sub PAUSE_COUNT, PAUSE_COUNT, 3  // Subtract the 3 cycles this init takes
  lsr PAUSE_COUNT, PAUSE_COUNT, 1  // Divide by 2
PAUSE_LOOP:
  sub PAUSE_COUNT, PAUSE_COUNT, 1
  qblt PAUSE_LOOP, PAUSE_COUNT, 0
.endm


TOP:
  // Enable OCP master ports in SYSCFG register
  lbco r0, C4, 4, 4
  clr  r0, r0, 4
  sbco r0, C4, 4, 4

  mov SHARED_RAM, SHARED_RAM_ADDRESS

  /*
  We need to switch channels on the analog muxes before we sample,
  and the MAX4734 datasheet says that takes 25ns.

  The AD9201 has 4ns aperture delay.  So we have to wait that long after
  the rising clock edge before we switch.

  The input RC filter (10 ohms + 100pF) has a corner at 160MHz, so a bit
  less than 10ns.  So we should wait at least that long after switching
  so that the filter has had time to respond.  Also, there's almost
  200pF of capacitance inside the MAX4734.

  So we want to switch at least 35ns before we sample, and after we sample
  we shouldnt switch for 4ns.  So one or two PRU clock cycles after the
  rising ADC clock edge, we should be fine to switch, and that maximizes
  the time we have to switch and overcome capacitance before the next
  rising edge.

  Register 30:
  Bit 0: clock
  Bit 5 and 3: channel 0 input select
  Bit 1 and 2: channel 1 input select
  */

  // Clock low, input 0
  mov r30, 0
  // Wait half a cycle (subtracting 1 for the mov we just did)
  PAUSE HALF_CYCLE_COUNT - 1

REPEAT:
  // Clock goes high, sample inputs 0 and 4
  mov r30, 0x01

  // Wait 10ns for aperture delay just to be safe
  NOP
  NOP

  // Switch muxes to inputs 1 and 5 now that 0 and 4 have been sampled
  mov r30, (1 << 1) | (1 << 3) | 0x01

  // This pause plus the next half ADC clock cycle should be plenty for the amux
  // switching time and for the input filters to adjust
  PAUSE HALF_CYCLE_COUNT - 4

  // Clock goes low (channel 1 soon becomes available on readout pins)
  mov r30, (1 << 1) | (1 << 3) | 0x0

  // Twiddle our thumbs during the low part of the cycle
  PAUSE HALF_CYCLE_COUNT - 1

  // Clock goes high, sampling inputs 1 and 5.
  mov r30, (1 << 1) | (1 << 3) | 0x1

  // Wait 10ns
  NOP
  NOP

  // Clock stays high as we switch muxes back to inputs 0 and 4
  mov r30, 0x1

  PAUSE HALF_CYCLE_COUNT - 4

  // Clock goes low, input 5 
  mov r30, 0x0

  PAUSE HALF_CYCLE_COUNT - 2

  qba REPEAT

// (We should never reach here)
halt
