# PRUDAQ
*This is not an official Google product.*

PRUDAQ is a fast, [Free](http://www.gnu.org/philosophy/free-sw.html), low-cost ADC board for BeagleBone Black and BeagleBone Green.  You can build the board yourself using the Eagle files in the repo, or buy one pre-built from [GroupGets](https://groupgets.com/).  Features:

* Dual-channel simultaneously-sampled 10-bit ADC
* Up to 20MSPS per channel (40MSPS total) (but see below)
* 0-2V input voltage range (DC coupled)
* 4:1 analog switches in front of each channel provide a total of 8 single-ended analog inputs.  (See [here](doc/BoardModification.md) for differential input)
* SMA jacks for direct access to the 2 ADC channels
* Flexible clock options:
 * External input via SMA jack
 * Internal onboard 10MHz oscillator
 * Programmable clock from BeagleBone GPIO pins

The BeagleBone's programmable real-time units (PRUs) make it possible to capture the 400Mbit/s sample stream into the BeagleBone's 512MB of main memory.  See the [Performance](doc/Performance.md) page for details.  Summary:

* 5MSPS * 2 channels with the sample code in this repo
* 2MSPS * 4 channels with the examples/round-robin sample code
* 10MSPS * 2 channels using PRUDAQ support in the [BeagleLogic](https://github.com/abhishek-kakkar/BeagleLogic) driver with an external clock or the onboard 10MHz clock
* Full-rate 20MSPS * 2 channels using the BeagleLogic clock generator

Note that while we can get 400 megabits per second *into* memory, we don't know any way to store data that quickly on a BeagleBone, whether to onboard flash, an external drive, or by shipping it over a network.  Our high score is currently 112Mbps to a USB flash drive.  See the [Performance](doc/Performance.md) page for details.

## PRUDAQ is not an oscilloscope

If you're looking for an oscilloscope or a hardened data acquisiton system, this board is probably not what you want:

* We haven't spent much time interfacing our board with high level tools like GNU Radio
* Input signals outside the 0-2V range may damage the ADC
* Inputs are DC-coupled
* We provide a 1V reference for mid-scale, but it's unbuffered
* At 40MSPS the 1GHz ARMv7 only has 25 cycles per sample (and at least 25% of that is eaten by DMA overhead)
* Round-robin sampling across the 8 analog inputs is tricky:
 * Switching latency and makes it hard to sample at high rates
 * High sample rates also require a low source impedance (strong input signal) to overcome ~300pF switch and filter capacitance
* As the previous section points out, you can buffer samples into the 512MB of main memory, but there's no way to get the samples into storage or off the board at the full 40MSPS data rate.

But all of that notwithstanding, we think PRUDAQ is pretty neat:
 
* Simultaneously sampled channels are handy for applications like SDR
* Data acquisition boards anywhere close to our sample rate are much larger, cost thousands of dollars, and aren't [Free](http://www.gnu.org/philosophy/free-sw.html)
* Some boards like the Arduino Due and STM32 have ADCs with upwards of 1MSPS, but much less RAM than a BeagleBone, slower CPUs, and major bottlenecks getting data into flash or onto a host PC

## Working with PRUDAQ

We've tried to make PRUDAQ accessible to people who don't want to learn PRU assembly or write C code. [BeagleLogic](https://github.com/abhishek-kakkar/BeagleLogic)'s PRUDAQ driver provides a /dev/beaglelogic device you can pipe into a file or a python program, and their pre-built system image makes it easy to get the software working.

PRUDAQ is also hacker friendly: we've tried to document all the circuit design, firmware and performance considerations.  The stock BeagleBone distro has all the libraries and compiler toolchains needed to compile our sample code, which has lots of comments.  

## Getting Started

Once you have a PRUDAQ board, the [QuickStart](doc/QuickStart.md) will have you reading samples in minutes.

The [Circuit Description](doc/CircuitDescription.md) explains all the jumpers and describes how the board works overall.

The [Input/Output](doc/InputOutput.md) doc explains how the BeagleBone's GPIO pins and realtime units provide clock, input selection, and data transfer into main memory.

The [Performance](doc/Performance.md) doc talks about timing considerations between the clock, input select lines, and data lines.  It also talks about the bottlenecks we encounter getting the data into the main CPU and into storage or out over a network.

The [Board Modification](doc/BoardModification.md) doc describes how you can modify the board to get features like differential input.
