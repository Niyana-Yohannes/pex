#ifndef PE_TRADER_H
#define PE_TRADER_H

#include "pe_common.h"

#define MAX_EVENTS 1
#define READ_SIZE 500
#define CHECK_IF_SELL 2 
#define SELL_MSG_SEP 5
#define TIME_DELAY 2

void sigaction_handler(int sig, siginfo_t* sinf, void* ucontext);

#endif
