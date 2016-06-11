# can run `make Q=` to see commands as they run
Q := @

CFLAGS += --std=gnu99 -O2 -Wall

CC := $(Q)$(CC)
RM := $(Q)$(RM)
PASM := $(Q)pasm -DBUILD_WITH_PASM=1
DTC := $(Q)dtc

.PHONY: all clean install

TARGETS := selftest pru0.bin pru1.bin

all: $(TARGETS)

clean:
	$(RM) $(TARGETS) *.o

%.bin: %.p
	$(PASM) -b $^

selftest: selftest.o
	$(CC) -o $@ $^ -l prussdrv
