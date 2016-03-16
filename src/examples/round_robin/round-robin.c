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

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <libgen.h>
#include <string.h>

#include <prussdrv.h>
#include <pruss_intc_mapping.h>

#include <signal.h>

static int bCont = 1;

// header for sharing info between PRUs and application processor
#include "shared_header.h"

// the PRU clock speed used for GPIO clock generation
#define PRU_CLK 200e6

void sig_handler (int sig) {
  // break out of reading loop
  bCont = 0;
  return;
}


int main (int argc, char **argv) {
  if (geteuid() != 0) {
    fprintf(stderr, "Must be root. Try again with sudo.\n");
    return EXIT_FAILURE;
  }

  if (argc != 3) {
    fprintf(stderr, "Usage: %s pru0_code.bin pru1_code.bin\n", argv[0]);
    return EXIT_FAILURE;
  }

  // install signal handler to catch ctrl-C
  if (SIG_ERR == signal(SIGINT, sig_handler)) {
    perror("Warn: signal handler not installed %d\n");
  }

  // If this segfaults, make sure you're executing as root.
  prussdrv_init();
  if (0 != prussdrv_open(PRU_EVTOUT_0)) {
    fprintf(stderr, "prussdrv_open() failed\n");
    return EXIT_FAILURE;
  }

  tpruss_intc_initdata pruss_intc_initdata = PRUSS_INTC_INITDATA;
  prussdrv_pruintc_init(&pruss_intc_initdata);

  // Get pointer into the 8KB of shared PRU DRAM where prudaq expects
  // to share params with prus and the main cpu
  volatile pruparams_t *pparams = NULL;
  prussdrv_map_prumem(PRUSS0_SHARED_DATARAM, (void**)&pparams);

  // Pointer into the DDR RAM mapped by the uio_pruss kernel module.
  volatile uint32_t *shared_ddr = NULL;
  prussdrv_map_extmem((void**)&shared_ddr);
  unsigned int shared_ddr_len = prussdrv_extmem_size();

  unsigned int physical_address = prussdrv_get_phys_addr((void*)shared_ddr);

  fprintf(stderr,
          "%uB of shared DDR available.\n Physical (PRU-side) address:%x\n",
         shared_ddr_len, physical_address);
  fprintf(stderr, "Virtual (linux-side) address: %p\n\n", shared_ddr);

  pparams->physical_addr = physical_address;
  pparams->ddr_len       = shared_ddr_len;

  prussdrv_exec_program(0, argv[1]);
  prussdrv_exec_program(1, argv[2]);

  volatile uint32_t *read_pointer = shared_ddr;
  volatile uint32_t *buffer_end = shared_ddr + (shared_ddr_len / sizeof(*shared_ddr));

  // Dummy variable so compiler doesn't optimize away our data.
  uint32_t foo = 0;
  int64_t bytes_read = 0;
  while (bCont) {
    uint32_t *write_pointer_virtual =
      prussdrv_get_virt_addr(pparams->shared_ptr);

    while (read_pointer != write_pointer_virtual) {
      // Copy to a local array so we're not working in special slow DMA ram
      uint16_t amplitudes[4];
      memcpy(amplitudes, (void *) read_pointer, 8);

      for (int i = 0; i < 4; i++) {
        // Insert code here for doing interesting things with the downsampled
        // amplitude data.
        foo += amplitudes[i];
        bytes_read += sizeof(amplitudes[0]);
      }
      // Occasionally report to stderr
      if (bytes_read % (1048576) == 0) {
        fprintf(stderr, "Processed %lldMB\n", bytes_read / 1048576);
        fprintf(stderr,
                "Most recent amplitude for channel 0:%d  1:%d  4:%d  5:%d\n",
                amplitudes[0], amplitudes[1], amplitudes[2], amplitudes[3]);
      }

      read_pointer += (8 / sizeof(*read_pointer));
      if (read_pointer >= buffer_end) {
        read_pointer = shared_ddr;
      }
    }
    usleep(1000);
  }

  fprintf(stderr, "All done\n");

  prussdrv_pru_disable(0);
  prussdrv_pru_disable(1);
  prussdrv_exit();

  return 0;
}
