CC=gcc
CFLAGS=-Wall -Werror -Wvla -O0 -std=c11 -g -fsanitize=address,leak
LDFLAGS=-lm

all: pe_exchange pe_trader

.PHONY: clean
clean:
	rm -f pe_exchange pe_trader

pe_exchange: pe_exchange.c pe_comp.c pe_out.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

pe_trader: pe_trader.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)


