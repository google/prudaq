This page describes the theory of operation for the board, and how it's controlled by the BeagleBone.

**Clock selection**

There are 3 ways to provide a clock to the ADC: BeagleBone pin, onboard 10MHz oscillator, and external clock source.

![Clock selection](clock_selection.png?raw=true "Clock selection")

1. Onboard 10MHz oscillator.  Install a jumper on J1 toward the "10MHz osc" label near the 46 pin header.  No jumper on J4.

2. GPIO clock.  Beaglebone pins P9_31 and P8_30 are routed to the ADC clock input.  Install a jumper on J1 toward the "GPIO clock" label.  No jumper on J4.  P9_31 is one of PRU0's high speed GPIO pins, while P8_30 is one of PRU1's.  This allows PRU0 to bit-bang a clock signal while PRU1 collects data, or PRU1 can both generate the clock signal and collect the samples.  Yet another variant is to use P9_31 as a PWM output so that neither PRU has to generate the clock:

```bash
# Use beaglebone PWM features to generate the clock signal instead of the default PRU-based approach
# (Must be root)
echo bone_pwm_P9_31 > /sys/devices/bone_capemgr.9/slots
echo am33xx_pwm > /sys/devices/bone_capemgr.9/slots
sleep 1
# 1000 period ~ 1Mhz
echo 1000 > /sys/devices/ocp.3/pwm_test_P9_14.12/period
# 500 cycles off time
echo 500 > /sys/devices/ocp.3/pwm_test_P9_14.12/duty
```

3. External clock.  Remove jumper from J1 and supply a clock signal either to the middle pin on J1 or via the SMA connector.  Install a jumper on J4 to enable the 50ohm termination resistor.  Clock signal must stay in the range of 0-3.3V.


**"Enabled" LED**

The system control chip that manages bootup on the BeagleBone uses some of the header pins to determine boot options.  Coincidentally, those same pins are used to read the ADC data lines.  So to ensure normal boot, we leave the ADC disabled by default using pullup resistor R20.

Pulling down pin P9_25 enables the ADC and lights the "Enabled" LED.

**Sampling**

As described in the AD9201 datasheet, on the rising clock edge the ADC samples both input channels (I and Q).  About 20ns after the rising edge it's safe to read out the channel I value on the 10 data pins.  And then about 20ns after the falling edge, it's safe to read out channel Q.  

Note that there's a **3 cycle pipeline latency**, so you're really reading out the value sampled 3 cycles ago.

The next section describes how a pair of analog switches are used to select between 4 inputs for each ADC channel.  If you do round-robin sampling of the 8 inputs, the ADC 3 cycle pipeline latency can make it tricky to keep track of which analog switch input a sample came from.  So we routed the channel I input select pins to P8_28 and P9_26 to make it easy for PRU1 to see which input is selected while reading the 10 data bits.  (Unfortunately there weren't enough free input pins to route the channel Q input select pins to PRU1).  Those bits still only tell you what channel was in use for the sample you'll get 3 cycles from now, but at least it's easy to store it along with a sample in 16 bits of data.

**Analog input channels**

The AD9201 ADC has two simultaneously sampled analog input channels.  We have them configured for 0-2V input signals, DC coupled.  There's no overvoltage protection, so the user must take care not to damage the ADC.

The two inputs are routed to the RAW0 and RAW1 SMA connectors.  J6 and J5 allow input signals to be terminated at 50 ohms. 

Additionally, each input is switched through a MAX4734 4:1 analog switch, providing a total of 8 input channels at J2.  Inputs 0..3 go to the first channel (I) and inputs 4..7 go to the second channel (Q).  The analog switches are controlled independently by pins P9 27-30 (PRU0 r30 bits 5,3,1,2).  For example, if those pins were used to select inputs 1 and 7 by asserting P9_29 (low order bit for channel I switch), P9_28 and P9_27 (both bits for channel Q switch), then on the rising edge of the clock inputs 1 and 7 would be simultaneously sampled. 

![Analog Inputs](analog_inputs.png?raw=true "Analog Inputs")

The SMA and MAX4734 inputs are joined at the ADC input through a pair of 10 ohm resistors.  100pF capacitors to ground (C32 and C4) form a low pass filter with a corner at 160MHz, allowing undersampling of fast edges.  For lower noise sampling of slower signals, you may wish to replace R14, R2, R15, R3, C4, or C32 with larger values to lower the cutoff frequency of the input low pass filter.

![Input termination and filtering](input_termination_filtering.png?raw=true "Input termination and filtering")

Note that if J6 or J5 are installed to terminate the SMA connector inputs, they'll present a total of 70 ohms to ground from the analog switch inputs (50 + 10 + 10).

**VREF**

The ADC9201 on this board is configured to map voltages from 0..2V to values between 0..1023.  See the AD9201 datasheet for other possible configurations.  Part of the configuration are the 5.6Kohm resistors R1 and R22 forming a voltage divider from the 2V reference on the VREF pin.  The resulting 1V signal feeds the other half of the differential pair for the two input channels.  

The 1V reference signal is also routed to the 4 lower right pins of J2.  This could be used to AC couple input signals, ensuring that the resulting samples will be centered at midscale (512), but you'll probably want to buffer the signal to avoid drawing current from the VREF pin or affecting the differential inputs.

**Power Supply**

To reduce noise, we use two 3.3V LDO regulators from the 5V BeagleBone supply to separately power the analog and digital sides of the board.  The digital supply is possibly overkill, since the oscillator uses only a few milliamps and the digital side of the AD9201 claims to use less than a milliamp.  The 4-layer board uses separate analog and digital supply planes, and a ground plane joined under the ADC.

J3 provides access to the analog 3.3V supply and ground plane and should be good for tens of milliamps.
