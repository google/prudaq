This selftest verifies that all 8 PRUDAQ inputs are correctly switched
to the ADC.  It uses 8 GPIO pins on the beaglebone to send high or low
values to the 8 PRUDAQ inputs, and verifies that the ADC correctly reads them.

Connect 10k resistors between the following pins:

```
  P9_11 and PRUDAQ input0
  P9_12 and PRUDAQ input1
  P9_13 and PRUDAQ input2
  P9_14 and PRUDAQ input3
  P9_15 and PRUDAQ input4
  P9_16 and PRUDAQ input5
  P9_23 and PRUDAQ input6
  P9_24 and PRUDAQ input7
```

Install the clock jumper for either GPIO or onboard 10MHz clock.  (You may
even wish to run the test twice to verify both clock paths).  Make sure the
50ohm termination jumpers J4, J5 and J6 are *not* installed.

Run 'sudo ./setup.sh' from the prudaq/src directory once per boot to enable
the PRUs.

Run 'sudo ./selftest_setup.sh' from this directory once per boot to set up the
8 GPIO pins as outputs.

Run 'make' to build the self test and PRU binaries.

Then run 'sudo ./selftest' to perform the test.

Example:
```
debian@beaglebone:~/prudaq/src$ sudo ./setup.sh 
Device overlay successfully loaded.
debian@beaglebone:~/prudaq/src$ cd examples/self_test/
debian@beaglebone:~/prudaq/src/examples/self_test$ sudo ./selftest_setup.sh 
Setting up each of our self test pins for GPIO output and initializing
to low.
(You might see a harmless write error if you already ran this script)
  Setting up GPIO 30 for output
  Setting up GPIO 60 for output
  Setting up GPIO 31 for output
  Setting up GPIO 50 for output
  Setting up GPIO 48 for output
  Setting up GPIO 51 for output
  Setting up GPIO 49 for output
  Setting up GPIO 15 for output
debian@beaglebone:~/prudaq/src/examples/self_test$ make clean && make


PRU Assembler Version 0.86
Copyright (C) 2005-2013 by Texas Instruments Inc.


Pass 2 : 0 Error(s), 0 Warning(s)

Writing Code Image of 31 word(s)



PRU Assembler Version 0.86
Copyright (C) 2005-2013 by Texas Instruments Inc.


Pass 2 : 0 Error(s), 0 Warning(s)

Writing Code Image of 26 word(s)
debian@beaglebone:~/prudaq/src/examples/self_test$ sudo ./selftest
Actual GPIO clock speed is 1000.00Hz
  DEBUG: Testing inputs 0 and 4 at zero voltage.
  DEBUG: Got 5 and 6 and input0a=0
  DEBUG: Testing inputs 0 and 4 at full voltage.
  DEBUG: Got 1023 and 1023 and input0a=0
  DEBUG: Testing inputs 1 and 5 at zero voltage.
  DEBUG: Got 5 and 5 and input0a=1
  DEBUG: Testing inputs 1 and 5 at full voltage.
  DEBUG: Got 1023 and 1023 and input0a=1
  DEBUG: Testing inputs 2 and 6 at zero voltage.
  DEBUG: Got 5 and 5 and input0a=0
  DEBUG: Testing inputs 2 and 6 at full voltage.
  DEBUG: Got 1023 and 1023 and input0a=0
  DEBUG: Testing inputs 3 and 7 at zero voltage.
  DEBUG: Got 5 and 5 and input0a=1
  DEBUG: Testing inputs 3 and 7 at full voltage.
  DEBUG: Got 1023 and 1023 and input0a=1
SUCCESS: All inputs passed!
```
