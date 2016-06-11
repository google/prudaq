#!/bin/bash

# Run this script before the selftest binary so that the GPIO test pins
# will be enabled and set as outputs.  Then you can run 'sudo ./selftest'
# as many times as you want until you reboot.

if [[ $EUID -ne 0 ]] ; then
  echo Try again as root
  exit 1
fi

# This table gives us header pin -> GPIO number
# http://exploringbeaglebone.com/wp-content/uploads/resources/BBBP9Header.pdf

# These GPIO pins let us pull the 8 inputs up or down so we can verify
# the analog switches and ADC can route and read each input.
# P9_11 = GPIO 30
# P9_12 = GPIO 60
# P9_13 = GPIO 31
# P9_14 = GPIO 50
# P9_15 = GPIO 48
# P9_16 = GPIO 51
# P9_23 = GPIO 49
# P9_24 = GPIO 15

echo Setting up each of our self test pins for GPIO output and initializing
echo to low.
echo '(You might see a harmless write error if you already ran this script)'
for GPIO in 30 60 31 50 48 51 49 15 ; do
  echo "  Setting up GPIO $GPIO for output"
  echo $GPIO > /sys/class/gpio/export
  echo out   > /sys/class/gpio/gpio$GPIO/direction
  echo 1     > /sys/class/gpio/gpio$GPIO/value
done
