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

.origin 0
.entrypoint TOP

#define DDR r29
#define DDR_SIZE r28
#define SHARED_RAM r27

#include "shared_header.h"

TOP:
  // Enable OCP master ports in SYSCFG register
  lbco r0, C4, 4, 4
  clr  r0, r0, 4
  sbco r0, C4, 4, 4

  mov SHARED_RAM, SHARED_RAM_ADDRESS

  // From shared RAM, grab the address of the shared DDR segment
  lbbo DDR, SHARED_RAM, OFFSET(Params.physical_addr), SIZE(Params.physical_addr)
  // And the size of the segment from SHARED_RAM + 4
  lbbo DDR_SIZE, SHARED_RAM, OFFSET(Params.ddr_len), SIZE(Params.ddr_len)

  mov r12, 0

// Loops over the shared memory segment
BIGLOOP:
  // Start at the beginning of the segment
  mov r10, DDR
  add r11, DDR, DDR_SIZE

// Within the shared memory segment
LOOP0:
  // Update the write pointer where the reader can see it
  sbbo r10, SHARED_RAM, OFFSET(Params.shared_ptr), SIZE(Params.shared_ptr)
  sbbo r12, SHARED_RAM, OFFSET(Params.bytes_written), SIZE(Params.bytes_written)

  // wait for rising clock edge
  wbs r31, 11

  // Wait for the data lines to be valid
  add r19, r19, 0
  add r19, r19, 0
  add r19, r19, 0
  add r19, r19, 0

  // Write 16 bits to DDR
  sbbo r31.w0, r10, 0, 2

  // bytes_written++
  add  r12, r12, 2
  add  r10, r10, 2

  // wait for falling clock edge
  wbc r31, 11

  // XXX: This means r10 < r11, opposite what I expected!
  qblt LOOP0, r11, r10

  qba BIGLOOP

  // Interrupt the host so it knows we're done
  mov  r31.b0, 19 + 16

// Don't forget to halt!
halt
