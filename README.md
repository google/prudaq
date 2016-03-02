# PRUDAQ
*This is not an official Google product.*

PRUDAQ is an open source 40MSPS (20MSPS \* 2 channels) Data Acquisition board for the BeagleBone Black and BeagleBone Green. This repo includes the Eagle CAD electrical circuit board design and parts, as well as the code required to interface with the board and run a sample analog capture script.

The board is built around the AD9201 analog to digital converter, which samples two inputs simultaneously at up to 20MSPS per channel. The code consists of embedded software for the programmable real-time units (PRU) onboard the Beaglebone as well as software for pulling the data to the CPU for processing and storage.

# Build

### 1 - Connect to Beaglebone
Attach the PRUDAQ cape before powering on the Beaglebone, then connect it to your computer with a microUSB cable. The Beaglebone should boot up and connect using ethernet over USB with IP address `192.168.7.2` by default.

*Beaglebone also serves a webpage by default, this can be verified by opening [http://192.168.7.2](http://192.168.7.2) on a browser.*

### 2 - Build and Install
Clone the repo locally and copy it over to the home directory on the Beaglebone:

    scp -r daq-beaglebone/ debian@192.168.7.2:~

*Note: The default password for the debian user is: temppwd*

Then ssh into the Beaglebone to build and install the code:

    ssh debian@192.168.7.2
    
    cd ~/daq-beaglebone/src
      
    make
    sudo make install 
    
    // Init script that needs to be run once every time beaglebone is started up
    sudo ./setup.sh

### 3 - Set Clock input
On the board is a 3 pin header labeled **clock source** that lets you choose where the clock signal comes from. Clock can come from 3 places, selected by jumper: 
 * Onboard 20MHz oscillator
 * External via SMA jack
 * GPIO pin controlled by PRU0

For this demo we will use the GPIO clock and provide a clock signal from PRU0 via `pru0-gpioclk.p`. 
Use a 2-pin jumper and jump the middle pin to the pin closest to the label **GPIO Clock**.

# Run

An example script is included that loads binaries onto the two PRUs and logs analog data to stdout.

    prudaq_capture usage: pru_capture pru0_code.bin pru1_code.bin

This program loads the PRU binaries `*.bin` passed as arguments into the two PRUs. It then gets the pointer to the shared memory allocated by uio_pruss kernel module, keeps the virtual address for itself and sticks the physical address in the PRU shared memory for PRU1 to find. Then the main loop is reading the ring buffer. This goes on until the user Ctrl-C’s to halt the program.

Running `prudaq_capture` with the example binaries (with a pipe to hexdump and head for testing):

    ./prudaq_capture pru0-gpioclk.bin channel-0.bin | hexdump -d | head
    
    262144B of shared DDR available.
     Physical (PRU-side) address:9f500000
    Virtual (linux-side) address: 0xb6d63000
    
    Actual GPIO clock speed is 1e+06
    0000000   01023   01023   01023   01023   00000   00001   00003   00002
    0000010   00004   01023   01023   01023   01023   01023   00000   00001
    0000020   00003   00004   00003   01023   01023   01023   01023   01023
    0000030   00000   00001   00003   00004   00004   01023   01023   01023
    0000040   01023   01023   00000   00000   00003   00003   00003   01023
    0000050   01023   01023   01023   01023   00000   00001   00003   00003
    0000060   00004   01023   01023   01023   01023   01023   00000   00000
    0000070   00002   00003   00003   00255   00255   00255   00255   00255
    0000080   00000   00000   00003   00000   00003   00255   00255   00255
    0000090   00255   00255   00000   00001   00002   00003   00004   00255


Remove the pipe to head for a live stream of all 8 inputs, try bridging a resistor between each yellow and black input pair to see the values change, or better yet connecting to a signal generator source of some sort.

## Troubleshooting 

* If you got `prussdrv_open() failed` then you need to run `sudo ./setup.sh` again, we need to do this just once, every time the Beaglebone is restarted.


* If you only see the initialization output but no data after:

      262144B of shared DDR available.
       Physical (PRU-side) address:9f5c0000
      Virtual (linux-side) address: 0xb6da4000
  
  This is probably because the clock signal isn't being received. Check to make sure J1 is set to use the GPIO clock or onboard clock
