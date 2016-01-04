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

/*
 * PRUDAQ capture program
 *
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
#include <time.h>


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


void usage (char* arg0) {
  printf("%s\n", basename(arg0));

  printf("\n"
         "  -g freq\t gpio based clock frequency (default: 1MHz)\n"
         //"  -m [0-3]\t mux 1 select (not implemented)\n"
         //"  -n [0-3]\t mux 2 select (not implemented)\n"
	 "  -o output\t output filename (default: stdout)\n"
         );
  exit(EXIT_FAILURE);
}


int main (int argc, char **argv) {
  int ch = -1;
  double gpiofreq = 1e6;
  int mux0 = -1;
  int mux1 = -1;
  char* fname = "-";
  FILE* fout = stdout;

  while (-1 != (ch = getopt(argc, argv, "g:m:n:o:"))) {
    switch (ch) {
    case 'g':
      gpiofreq = strtod(optarg, NULL);
      break;
    case 'm':
      mux0 = strtol(optarg, NULL, 0);
      break;
    case 'n':
      mux1 = strtol(optarg, NULL, 0);
      break;
    case 'o':
      fname = optarg;
      break;
    default:
      usage(argv[0]);
      break;
    }
  }

  if (argc - optind != 2) {
    fprintf(stderr, "Usage: %s pru0_code.bin pru1_code.bin\n", argv[0]);
    return EXIT_FAILURE;
  }

  argc -= optind;
  argv += optind;


  if (0 != strcmp(fname, "-")) {
    fout = fopen(fname, "w");
    if (NULL == fout) {
      perror("unable to open output file");
    }
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
  volatile uint16_t *shared_ddr = NULL;
  prussdrv_map_extmem((void**)&shared_ddr);
  unsigned int shared_ddr_len = prussdrv_extmem_size();
  unsigned int physical_address = prussdrv_get_phys_addr((void*)shared_ddr);

  fprintf(stderr,
          "%uB of shared DDR available.\n Physical (PRU-side) address:%x\n",
         shared_ddr_len, physical_address);
  fprintf(stderr, "Virtual (linux-side) address: %p\n\n", shared_ddr);

  // We'll use the first 8 bytes of PRU memory to tell it where the
  // shared segment of system memory is.
  pparams->physical_addr = physical_address;
  pparams->ddr_len       = shared_ddr_len;

  // adding 0.5 and truncating is equivalent to rounding
  int cycles = (PRU_CLK/gpiofreq + 0.5);
  fprintf(stderr, "Actual GPIO clock speed is %g\n", PRU_CLK/((float)cycles));

  pparams->poscnt        = cycles/2;
  pparams->negcnt        = cycles - pparams->poscnt;

  prussdrv_exec_program(0, argv[0]);
  prussdrv_exec_program(1, argv[1]);

  uint32_t max_index = shared_ddr_len / sizeof(uint16_t);
  uint32_t read_index = 0;
  time_t now = time(NULL);
  time_t start_time = now;

  while (bCont) {
    uint16_t *write_pointer_virtual = prussdrv_get_virt_addr(pparams->shared_ptr);
    uint32_t write_index = write_pointer_virtual - shared_ddr;

    // if nothing available sleep for 1ms and look again
    if (read_index == write_index) {
      usleep(1000);
      continue;
    }

    uint16_t sample = shared_ddr[read_index];
    read_index = (read_index + 1) % max_index;

    unsigned int bytes_written = pparams->bytes_written;

    // clear the clock and mux bits
    sample &= 0x3ff;
    fwrite(&sample, sizeof(sample), 1, fout);
    //fflush(fout);

    if (now != time(NULL)) {
      now = time(NULL);
      fprintf(stderr, "\t%ld bytes / second\n", bytes_written / (now - start_time));
    }
  }

  // Wait for the PRU to let us know it's done
  //prussdrv_pru_wait_event(PRU_EVTOUT_0);
  fprintf(stderr, "All done\n");

  prussdrv_pru_disable(0);
  prussdrv_pru_disable(1);
  prussdrv_exit();

  if (stdout != fout) {
    fclose(fout);
  }

  return 0;
}
