#ifndef PE_EXCHANGE_H
#define PE_EXCHANGE_H

#include "pe_common.h"

#define LOG_PREFIX "[PEX]"
#define FIFO_LENGTH 30
#define MARKET_OPEN_MSG 12
#define FILL_BUFFER 20 //Max size of message + null-terminator
#define READ_BUFFER 43 //Max size of message + null-terminator
#define NOTIFY_BUFFER 46 //Max size of message + null-terminator
#define ACCEPTED_BUFFER 17 //Max size of message + null-terminator
#define PRODUCT_BUFFER 17 //Max size of product + null-terminating

void sigaction_handler(int sig, siginfo_t* sinf, void* ucontext);
void sigaction_handler_sc(int sig, siginfo_t* sinf, void* ucontext);

typedef struct order {
	int trader_id;
	int order_id;
	int type; //BUY or SELL
	int quantity;
	long price;
	int age;
	int product_id;
} order;

typedef struct trader {
	int id;
	long *quantity;
	long *balance;
	int order_id;
} trader;

typedef struct priority_queue {
	order **orders;
	int size;
} priority_queue;

#endif
