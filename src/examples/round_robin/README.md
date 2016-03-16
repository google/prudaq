This directory has code demonstrating:
 * How to use PRU0 to generate proper timings on the GPIO clock and analog switch select lines to enable alternating between inputs 0 and 4, and inputs 1 and 5, sampling each of the 4 inputs at 2MSPS. (4MHz ADC clock)
 * How to capture that data with PRU1, using the analog switch select line to help keep track of which input is being sampled (taking the 3 cycle ADC pipeline latency into account)
 * How to downsample the input data with PRU1, in this case by measuring amplitude over N samples.
 * Host-side code is a simplified version of ```prudaq_capture``` with "insert code here" for processing the amplitude samples.

Example:
```
$ make
$ sudo ./round-robin pru0-round-robin.bin pru1-read-and-process.bin
$ make && sudo ./round-robin pru0-round-robin.bin pru1-read-and-process.bin
262144B of shared DDR available.
 Physical (PRU-side) address:9e780000
Virtual (linux-side) address: 0xb6dc8000

Processed 1MB
Most recent amplitude for channel 0:0  1:0  4:0  5:0
Processed 2MB
Most recent amplitude for channel 0:0  1:0  4:0  5:0
Processed 3MB
Most recent amplitude for channel 0:0  1:0  4:1  5:0
Processed 4MB
Most recent amplitude for channel 0:0  1:0  4:0  5:0
Processed 5MB
Most recent amplitude for channel 0:0  1:0  4:0  5:0
```
