# can run `make Q=` to see commands as they run
Q := @

CFLAGS += --std=gnu99 -O2 -Wall

CC := $(Q)$(CC)
RM := $(Q)$(RM)
PASM := $(Q)pasm -DBUILD_WITH_PASM=1
DTC := $(Q)dtc

.PHONY: all clean install

TARGETS := prudaq_capture pru0.bin pru1.bin prudaq-00A0.dtbo

all: $(TARGETS)

clean:
	$(RM) $(TARGETS) *.o

install: prudaq-00A0.dtbo
	$(Q)install -v $^ /lib/firmware

%.bin: %.p
	$(PASM) -b $^

prudaq_capture: prudaq_capture.o
	$(CC) -o $@ $^ -l prussdrv

%.dtbo: %.dts
	$(DTC) -I dts -b0 -O dtb -@ -o $@ $^

