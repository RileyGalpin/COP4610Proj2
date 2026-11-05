CC := gcc
CFLAGS := -D_GNU_SOURCE -Wall -Wextra -std=c11

.PHONY: all clean

all: producer consumer

producer: producer.c wrappers.h
	$(CC) $(CFLAGS) producer.c -o producer

consumer: consumer.c wrappers.h
	$(CC) $(CFLAGS) consumer.c -o consumer

clean:
	rm -f producer consumer
