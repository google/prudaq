# can run `make Q=` to see commands as they run
Q := @

CFLAGS += --std=gnu99 -O2 -Wall

CC := $(Q)$(CC)
RM := $(Q)$(RM)
PASM := $(Q)pasm -DBUILD_WITH_PASM=1
DTC := $(Q)dtc

.PHONY: all clean install

TARGETS := round-robin pru0-round-robin.bin pru1-read-and-process.bin

all: $(TARGETS)

clean:
	$(RM) $(TARGETS) *.o

%.bin: %.p
	$(PASM) -b $^

round-robin: round-robin.o
	$(CC) -o $@ $^ -l prussdrv
