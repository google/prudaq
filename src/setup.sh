#!/bin/bash

# Copyright 2015 Google Inc. All Rights Reserved.

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

#    http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
# implied.  See the License for the specific language governing
# permissions and limitations under the License.

if [[ $EUID -ne 0 ]]; then
  echo "Must be root. Try again with sudo."
  exit 1
fi

DEVICE_OVERLAY=/lib/firmware/prudaq-00A0.dtbo
if [[ ! -f $DEVICE_OVERLAY ]] ; then
  echo -n "$DEVICE_OVERLAY not found, so we can't enable it."
  echo -n " (Did you run 'make install' to generate the .dtbo"
  echo -n " file from the .dts and copy it to /lib/firmware?)"
  echo
  echo
  exit 2
fi

if echo prudaq > /sys/devices/bone_capemgr.9/slots ; then
  echo 'Device overlay successfully loaded.'
else
  echo
  echo -n "Couldn't enable the prudaq-00A0.dtbo device overlay."
  echo -n " (If the error was 'File exists', then the overlay was already"
  echo -n " enabled.)"
  echo
  exit 3
fi
