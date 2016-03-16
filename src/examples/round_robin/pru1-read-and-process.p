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

/* This code runs on PRU1 and retrieves the samples generated as PRU0
 * switches back and forth between inputs 0 and 4, and inputs 1 and 5.
 *
 * It also downsamples the data, calculating the min and max values on
 * each input over a number of samples and reporting that back to the
 * host CPU.
 *
 * Sample rate is limited by the worst case number of cycles it takes to compute
 * the min/max, write to memory, and set up the loop.  That worst case is on
 * the low half of the ADC clock cycle, 29 PRU cycles plus an undocumented
 * number of extra cycles in the case of a busy shared memory bus.
 *
 * In pru0-round-robin.p, the default half-cycle duration is 25 PRU cycles,
 * so in the worst case, the clock goes low, we do our 30ish cycles worth
 * of work, and by the time we get back to the wbs the clock has already been
 * high for a few cycles so we don't have to wait.  That's fine because
 * we still have plenty of its 25 cycles left over to do the work for its half
 * of the cycle.
 */

.origin 0
.entrypoint TOP

#include "shared_header.h"

// We get more throughput if we write to main memory in larger chunks.
// AMPLITUDES_START and AMPLITUDES_LEN will tell the sbbo instruction to
// write the two 32-bit registers in one go.
#define AMPLITUDES_START r13
#define AMPLITUDES_LEN 8
// .w0 means to use the bottom half of the register.
#define INPUT0_AMPLITUDE r13.w0
// .w2 means to use the top half of the register.
#define INPUT1_AMPLITUDE r13.w2
#define INPUT4_AMPLITUDE r14.w0
#define INPUT5_AMPLITUDE r14.w2

// Registers for storing the max and min values on each channel
#define INPUT0_REG r15
#define INPUT0_MAX INPUT0_REG.w2
#define INPUT0_MIN INPUT0_REG.w0
#define INPUT1_REG r16
#define INPUT1_MAX INPUT1_REG.w2
#define INPUT1_MIN INPUT1_REG.w0
#define INPUT4_REG r17
#define INPUT4_MAX INPUT4_REG.w2
#define INPUT4_MIN INPUT4_REG.w0
#define INPUT5_REG r18
#define INPUT5_MAX INPUT5_REG.w2
#define INPUT5_MIN INPUT5_REG.w0

#define RING_POINTER r19
#define RING_END r20

#define AMPLITUDE_SAMPLES r21
// How many samples over which to track min and max value
#define AMPLITUDE_SAMPLE_COUNT 20

#define SAMPLE_REG r22

#define SHARED_RAM r27
#define DDR_SIZE r28
#define DDR r29

// Stores the mask that lets us exclude the unused GPIO bits
#define MASK_REG r12

#define nop add r0, r0, 0

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

  mov RING_POINTER, DDR
  add RING_END, DDR, DDR_SIZE

  /* We will preserve the 10 data bits and bit 10 which tells us which input is
     selected by the analog switch right now (which will be different from
     the input selected 3 cycles ago when the sample entered the ADC's
     internal pipeline. (This gets ANDed with the sample below). */
  mov MASK_REG, 0x000007ff

// Loop over the shared ring buffer
MAIN_LOOP:
  // Update the write pointer where the reader can see it
  // This should take 2 cycles
  sbbo RING_POINTER, SHARED_RAM, OFFSET(Params.shared_ptr), SIZE(Params.shared_ptr)

  // Initialize max and min values to 0 and 0xffff
  mov INPUT0_REG, 0x0000ffff
  mov INPUT1_REG, 0x0000ffff
  mov INPUT4_REG, 0x0000ffff
  mov INPUT5_REG, 0x0000ffff

  /* Because each time through the loop does either inputs 1 and 4, or 2 and 5,
     we need to go through 2n times for 1, 2, 4, and 5 for each to be
     considered n times */
  mov AMPLITUDE_SAMPLES, AMPLITUDE_SAMPLE_COUNT * 2
AMPLITUDE_LOOP:

  // When clock is high we can read channel 0.
  // There are 8 or 9 cycles starting with this wbs and ending at the wbc.
  wbs r31, 11
  /*
   * The clock is high, and it'll be about 11ns before the data is valid.
   * Additionally, PRU0 will be switching the mux in about 10ns.  Waiting
   * 15ns here leaves time for both of those things to happen. */
  nop
  nop
  nop
  // Read and mask GPIO in the same op
  and SAMPLE_REG, r31, MASK_REG
  /* If bit 10 (one of the selector lines for the analog switch) is set,
   * the sample we just read corresponds with input 1 */
  qbbs CHANNEL0_SECOND_INPUT, SAMPLE_REG, 10
CHANNEL0_FIRST_INPUT:
  max INPUT0_MAX, INPUT0_MAX, SAMPLE_REG
  min INPUT0_MIN, INPUT0_MIN, SAMPLE_REG
  qba CHANNEL0_MINMAX_DONE
CHANNEL0_SECOND_INPUT:
  max INPUT1_MAX, INPUT1_MAX, SAMPLE_REG
  min INPUT1_MIN, INPUT1_MIN, SAMPLE_REG
CHANNEL0_MINMAX_DONE:


  // After clock goes low we can read channel 1
  // Up to 29 cycles before we roll over and get to the wbs in the worst
  // case where we roll over the ring buffer.  A stall on the write to
  // shared memory could add a few more.
  wbc r31, 11
  nop
  nop
  nop
  and SAMPLE_REG, r31, MASK_REG
  qbbs CHANNEL1_SECOND_INPUT, SAMPLE_REG, 10
CHANNEL1_FIRST_INPUT:
  max INPUT4_MAX, INPUT4_MAX, SAMPLE_REG
  min INPUT4_MIN, INPUT4_MIN, SAMPLE_REG
  qba CHANNEL1_MINMAX_DONE
CHANNEL1_SECOND_INPUT:
  max INPUT5_MAX, INPUT5_MAX, SAMPLE_REG
  min INPUT5_MIN, INPUT5_MIN, SAMPLE_REG
CHANNEL1_MINMAX_DONE:

  sub AMPLITUDE_SAMPLES, AMPLITUDE_SAMPLES, 1
  qbne AMPLITUDE_LOOP, AMPLITUDE_SAMPLES, 0

  /* Okay, we've found the min and max values over AMPLITUDE_SAMPLE_COUNT
     samples.  Now we write to shared RAM and start over again. */

  sub INPUT0_AMPLITUDE, INPUT0_MAX, INPUT0_MIN
  sub INPUT1_AMPLITUDE, INPUT1_MAX, INPUT1_MIN
  sub INPUT4_AMPLITUDE, INPUT4_MAX, INPUT4_MIN
  sub INPUT5_AMPLITUDE, INPUT5_MAX, INPUT5_MIN

  // 3 cycles, or more in the case of bus collision
  sbbo AMPLITUDES_START, RING_POINTER, 0, AMPLITUDES_LEN

  add RING_POINTER, RING_POINTER, AMPLITUDES_LEN

  /* If it isn't time to roll over the ring buffer, keep looping */
  qblt MAIN_LOOP, RING_END, RING_POINTER

  /* Else reset to the beginning of the ring and then loop */
  mov RING_POINTER, DDR
  qba MAIN_LOOP

  // We should never get here

  // Interrupt the host
  mov  r31.b0, 19 + 16

// Don't forget to halt!
halt
