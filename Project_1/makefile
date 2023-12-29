CC = gcc
CFLAGS = -Wall -pthread

TARGETS = pdispersa hdispersa

all: $(TARGETS)

pdispersa hdispersa: % : %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS)

