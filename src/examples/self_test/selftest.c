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
This program is for verifying that a PRUDAQ board works correctly.

It requires that GPIO pins on Beaglebone's P9 header be wired through resistors
to the 8 PRUDAQ inputs.

Then it turns those GPIO pins on and off to verify that the ADC can read high
and low readings from each input.

First run 'sudo ./selftest_setup.sh' to set up the 8 GPIO pins as outputs.

Make sure resistors are connected from GPIO pins to inputs as described
in the README.md.

Then run this program.  First it'll test input0 and input4 by setting
P9_11 and P9_15 low, all the other GPIOs high, then verifying that
it reads values near 0 from the ADC.

Next it'll set P9_11 and P9_15 high, all the other GPIOs low, then verify
that it reads values near 1023 from the ADC.

And so on for inputs 1 and 5, then 2 and 6, then 3 and 7.
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

// Header for sharing info between PRUs and application processor
#include "shared_header.h"


// Used by sig_handler to tell us when to shutdown
static int bCont = 1;

// The PRUs run at 200MHz
#define PRU_CLK 200e6

void sig_handler (int sig) {
  // break out of reading loop
  bCont = 0;
  return;
}

int gpio_set(int channel0_input, int channel1_input, int level) {
  // See selftest_pins.sh for what these mean
  int gpio_numbers[] = {30, 60, 31, 50, 48, 51, 49, 15};
  for (int i = 0; i < 8; i++) {
    int state = 0;
    if (i == channel0_input || i == channel1_input) {
      state = level;
    } else {
      state = !level;
    }

    char *zero_or_one = NULL;
    if (state) {
      zero_or_one = "1";
    } else {
      zero_or_one = "0";
    }

    char fn[255];
    sprintf(fn, "/sys/class/gpio/gpio%d/value", gpio_numbers[i]);
    //fprintf(stderr, "Writing %s to %s\n", zero_or_one, fn);
    FILE *gpio_value_file = fopen(fn, "w");
    if (gpio_value_file == NULL) {
      fprintf(stderr, "Failed to open %s.\n", fn);
      return -1;
    }
    if (!fputs(zero_or_one, gpio_value_file)) {
      fprintf(stderr, "Failed to write %s to %s.\n", zero_or_one, fn);
      return -1;
    }
    fclose(gpio_value_file);
  }
  return 0;
}

int init_and_sample(volatile pruparams_t *pparams,
                    volatile uint32_t *shared_ddr,
                    int channel0_input, int channel1_input,
                    uint16_t *channel0, uint16_t *channel1) {
  // Decide the value that'll get written to PRU0's register r30
  // See the docs for how bits in r30 correspond to the INPUT0A/
  // INPUT0B/INPUT1A/INPUT1B control lines on the analog switches.
  uint32_t pru0r30 = 0;
  if (channel0_input < 0 || channel0_input > 3) return -1;
  switch (channel0_input) {
    case 0: break;
    case 1: pru0r30 |= (1 << 1); break;
    case 2: pru0r30 |= (1 << 2); break;
    case 3: pru0r30 |= (1 << 1) | (1 << 2); break;
  }
  if (channel1_input < 4 || channel1_input > 7) return -1;
  switch (channel1_input) {
    case 4: break;
    case 5: pru0r30 |= (1 << 3); break;
    case 6: pru0r30 |= (1 << 5); break;
    case 7: pru0r30 |= (1 << 3) | (1 << 5); break;
  }
  pparams->input_select = pru0r30;

  // Load the .bin files into PRU0 and PRU1
  prussdrv_exec_program(0, "pru0.bin");
  prussdrv_exec_program(1, "pru1.bin");

  time_t now = time(NULL);
  time_t start_time = now;

  uint32_t write_index = 0;
  // Ignore the first few samples since the AD9201 has 3-cycle
  // pipeline latency.
  while (bCont && (write_index < 8)) {
    uint32_t *write_pointer_virtual = prussdrv_get_virt_addr(pparams->shared_ptr);
    write_index = write_pointer_virtual - shared_ddr;
    now = time(NULL);

    // Give up if it takes longer than 2s for data to arrive
    int max_wait = 2;
    if (now > start_time + max_wait) {
      fprintf(stderr, "Timeout waiting for data from ADC."
                      "  (Did you install the clock jumper?)\n");
      return -1;
    }

    usleep(10000);
  }
  if (!bCont) return -1;

  uint32_t sample = shared_ddr[7];
  *channel0 = sample & 0xffff;
  *channel1 = (sample >> 16) & 0xffff;

  prussdrv_pru_disable(0);
  prussdrv_pru_disable(1);

  return 0;
}

int main (int argc, char **argv) {
  double gpiofreq = 1000;

  // Make sure we're root
  if (geteuid() != 0) {
    fprintf(stderr, "Must be root.  Try again with sudo.\n");
    return EXIT_FAILURE;
  }

  // Install signal handler to catch ctrl-C
  if (SIG_ERR == signal(SIGINT, sig_handler)) {
    perror("Warn: signal handler not installed %d\n");
  }

  // This segfaults if we're not root.
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
  volatile uint32_t *shared_ddr = NULL;
  prussdrv_map_extmem((void**)&shared_ddr);
  unsigned int shared_ddr_len = prussdrv_extmem_size();
  unsigned int physical_address = prussdrv_get_phys_addr((void*)shared_ddr);

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

  int passed = 1;
  for (int i = 0; i < 4; i++) {
    uint16_t channel0 = 9999;
    uint16_t channel1 = 9999;

    int channel0_input = i;
    int channel1_input = i + 4;

    for (int level = 0; level < 2; level++) {
      fprintf(stderr, "  DEBUG: Testing inputs %d and %d at %s voltage.\n",
              channel0_input, channel1_input, level ? "full" : "zero");

      int result = gpio_set(channel0_input, channel1_input, level);
      if (result != 0) {
        passed = 0;
        break;
      }

      result = init_and_sample(pparams, shared_ddr, i, i + 4,
                               &channel0, &channel1);
      if (result != 0) {
        passed = 0;
        break;
      }

      // The sample data is in the low 10 bits
      uint16_t sample0 = channel0 & 0x03ff;
      uint16_t sample1 = channel1 & 0x03ff;
      // And bit 10 records the state of input0a right now (which won't
      // affect the sample data for 3 more cycles, but that's okay since
      // we took several samples with this channel enabled).
      uint16_t input0a = (channel0 & 0x0400) ? 1:0;

      fprintf(stderr, "  DEBUG: Got %hd and %hd and input0a=%d\n",
              sample0, sample1, input0a);
      if (input0a != (channel0_input % 2)) {
        fprintf(stderr, "FAIL: Expected input0a to be %d but got %d\n",
                input0a, channel0_input % 2);
        passed = 0;
      }

      if (level == 0) {
        if (sample0 > 20) {
          fprintf(stderr, "FAIL: Expected input %d to be ~0 but got %hd\n",
                  channel0_input, sample0);
          passed = 0;
        }
        if (sample1 > 20) {
          fprintf(stderr, "FAIL: Expected input %d to be ~0 but got %hd\n",
                  channel1_input, sample1);
          passed = 0;
        }
      } else {
        if (sample0 < 1020) {
          fprintf(stderr, "FAIL: Expected input %d to be ~1023 but got %hd\n",
                  channel0_input, sample0);
          passed = 0;
        }
        if (sample1 < 1020) {
          fprintf(stderr, "FAIL: Expected input %d to be ~1023 but got %hd\n",
                  channel1_input, sample1);
          passed = 0;
        }
      }
    }
  }

  prussdrv_exit();

  if (passed) {
    fprintf(stderr, "SUCCESS: All inputs passed!\n");
    return 0;
  }

  fprintf(stderr, "FAILURE: Something went wrong.\n");
  return EXIT_FAILURE;
}
