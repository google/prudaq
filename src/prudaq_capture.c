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
PRUDAQ capture program
Loads .bin files into both PRUs, then reads from the shared
buffer in main memory.
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
  fprintf(stderr, "\nUsage: %s [flags] pru0_code.bin pru1_code.bin\n",
          basename(arg0));

  fprintf(stderr, "\n"
          "  -f freq\t gpio based clock frequency (default: 1000)\n"
          "  -i [0-3]\t channel 0 input select\n"
          "  -q [4-7]\t channel 1 input select\n"
          "  -o output\t output filename (default: stdout)\n\n"
         );
  exit(EXIT_FAILURE);
}


int main (int argc, char **argv) {
  int ch = -1;
  double gpiofreq = 1000;
  int channel0_input = 0;
  int channel1_input = 4;
  char* fname = "-";
  FILE* fout = stdout;

  if (geteuid() != 0) {
    fprintf(stderr, "Must be root.  Try again with sudo.\n");
    return EXIT_FAILURE;
  }

  while (-1 != (ch = getopt(argc, argv, "f:i:q:o:"))) {
    switch (ch) {
    case 'f':
      gpiofreq = strtod(optarg, NULL);
      break;
    case 'i':
      channel0_input = strtol(optarg, NULL, 0);
      if (channel0_input < 0 || channel0_input > 3) {
        fprintf(stderr, "\n-i value must be between 0 and 3\n");
        usage(argv[0]);
      }
      break;
    case 'q':
      channel1_input = strtol(optarg, NULL, 0);
      if (channel1_input < 4 || channel1_input > 7) {
        fprintf(stderr, "\n-q value must be between 4 and 7\n");
        usage(argv[0]);
      }
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
    usage(argv[0]);
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
    fprintf(stderr,
            "prussdrv_open() failed. (Did you forget to run setup.sh?)\n");
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

  // Calculate the GPIO clock high and low cycle counts.
  // Adding 0.5 and truncating is equivalent to rounding
  int cycles = (PRU_CLK/gpiofreq + 0.5);
  fprintf(stderr, "Actual GPIO clock speed is %.2fHz\n", PRU_CLK/((float)cycles));

  if (cycles < 12) {
    fprintf(stderr, "Requested frequency too high (max: 16666666)\n");
    return EXIT_FAILURE;
  }
  pparams->high_cycles = cycles/2;
  pparams->low_cycles  = cycles - pparams->high_cycles;

  // Decide the value that'll get written to PRU0's register r30
  // See the docs for how bits in r30 correspond to the INPUT0A/
  // INPUT0B/INPUT1A/INPUT1B control lines on the analog switches.
  uint32_t pru0r30 = 0;
  switch (channel0_input) {
    case 0: break;
    case 1: pru0r30 |= (1 << 1); break;
    case 2: pru0r30 |= (1 << 2); break;
    case 3: pru0r30 |= (1 << 1) | (1 << 2); break;
  }
  switch (channel1_input) {
    case 4: break;
    case 5: pru0r30 |= (1 << 3); break;
    case 6: pru0r30 |= (1 << 5); break;
    case 7: pru0r30 |= (1 << 3) | (1 << 5); break;
  }
  pparams->input_select = pru0r30;

  // Load the .bin files into PRU0 and PRU1
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
