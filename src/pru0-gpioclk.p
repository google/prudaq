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

#include "shared_header.h"

#define COUNT_H    r20
#define COUNT_L    r21
#define SHARED_RAM r29
TOP:
  // Enable OCP master ports in SYSCFG register
  lbco r0, C4, 4, 4
  clr  r0, r0, 4
  sbco r0, C4, 4, 4

  // The only thing thing we are doing in this program is enabling the ADC.

  // PRU-DAQ 2.0 PCB has pins 6-10 mirrored on the 4734 muxes.  This address
  // line thus goes to the enable pin, and two of the inputs are swapped.
  mov r30, 0x24

  mov SHARED_RAM, SHARED_RAM_ADDRESS
  lbbo COUNT_H, SHARED_RAM, OFFSET(Params.poscnt), SIZE(Params.poscnt)
  lbbo COUNT_L, SHARED_RAM, OFFSET(Params.negcnt), SIZE(Params.negcnt)

  mov r1, 0
  mov r2, COUNT_H
  mov r3, COUNT_L

  mov r4, CYCLE_ODD_L
  QBBS RET_H, r3, 0
  mov r4, CYCLE_EVEN_L

RET_H:
  mov r5, CYCLE_ODD_H
  QBBS CYCLE_ODD_H, r2, 0
  mov r5, CYCLE_EVEN_H
  QBA CYCLE_EVEN_H

CYCLE_ODD_H:
  sub r2, r2, 1
CYCLE_EVEN_H:
  qbbs DONE, r1, 0
WAIT_H:
  sub r2, r2, 2
  qblt WAIT_H, r2, 4

  mov r3, COUNT_L
  clr r30, 0

  JMP r4

CYCLE_ODD_L:
  sub r3, r3, 1
CYCLE_EVEN_L:
  qbbs DONE, r1, 0
WAIT_L:
  sub r3, r3, 2
  qblt WAIT_L, r3, 4

  mov r2, COUNT_H
  set r30, 0

  JMP r5

DONE:
//disable ADC and deselect MUX
  mov r30, 0

// Don't forget to halt!
halt
