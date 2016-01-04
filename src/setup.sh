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

echo prudaq > /sys/devices/bone_capemgr.9/slots

#echo bone_pwm_P9_31 > /sys/devices/bone_capemgr.9/slots
#echo am33xx_pwm > /sys/devices/bone_capemgr.9/slots
#sleep 1
# 1000 period ~ 1Mhz
#echo 10 > /sys/devices/ocp.3/pwm_test_P9_14.12/period
# 500 cycles off time
#echo 5 > /sys/devices/ocp.3/pwm_test_P9_14.12/duty
