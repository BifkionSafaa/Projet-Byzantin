CC ?= gcc
CFLAGS ?= -Wall -Wextra -Werror -std=c11 -D_POSIX_C_SOURCE=200809L
CPPFLAGS ?=
LOG_DIR := logs
TARGETS := main server test_client

.PHONY: all clean logs test

all: logs $(TARGETS)

logs:
	mkdir -p $(LOG_DIR)

test: all
	bash scripts/run_test_suite.sh

main: logs src/main.c src/log.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ src/main.c src/log.c

server: logs src/server.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ src/server.c

test_client: logs src/test_client.c src/log.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ src/test_client.c src/log.c

clean:
	rm -f $(TARGETS)
	rm -f $(LOG_DIR)/*.log $(LOG_DIR)/*.png $(LOG_DIR)/*.dot
