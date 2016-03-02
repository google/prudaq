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
// This file contains the structure definitions for sharing parameters
// between the PRUs and the application processor.
// The structs must match!
//
// To have them in the same file, the BUILD_WITH_PASM must be defined
// as an option when building .p files
//   pasm -b -DBUILD_WITH_PASM=1 file.p

// TODO: Consider generating the pasm struct from the C

#ifndef BUILD_WITH_PASM

typedef struct {
  // Physical address of the start of the shared main memory buffer.
  // (The PRUs don't go through the virtual memory system, so they
  // see different memory addresses than the linux side does)
  uint32_t physical_addr;
  // Length in bytes of the shared main memory buffer
  uint32_t ddr_len;
  uint32_t shared_ptr;
  uint32_t bytes_written;
  // Used by PRU0 to generate a clock signal.  Measured in
  // PRU clock cycles (200MHz / 5ns).  Each must be >= 6.
  uint32_t high_cycles;
  uint32_t low_cycles;
  // Which input should be selected on each of the two 4:1 analog switches
  // that sit in front of the two ADC input channels.  This is written
  // as-is to r30 on PRU0, setting all of its GPIOs as specified (limited
  // by which pins are enabled in the device tree overlay).
  uint32_t input_select;
} pruparams_t;

#else

#define SHARED_RAM_ADDRESS 0x10000

.struct Params
  .u32 physical_addr
  .u32 ddr_len
  .u32 shared_ptr
  .u32 bytes_written
  .u32 high_cycles
  .u32 low_cycles
  .u32 input_select
.ends

#endif
