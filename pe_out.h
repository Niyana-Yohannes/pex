#ifndef PE_OUT_H
#define PE_OUT_H

#include "pe_common.h"
#include "pe_exchange.h"

void write_to_trader(int *exch_wr_trad_rd, char *msg, int *trader_pid, int trader_id);

int check_valid_buysell(char *read_buffer, char *token, char **products, int num_of_products, int trader_id, trader **traders);
int check_valid_amend(char *read_buffer, char *token, char **products, int num_of_products,
	priority_queue **pq_buy_arr, priority_queue **pq_sell_arr, int trader_id, trader **traders);
int check_valid_cancel(char *read_buffer, char *token, char **products, int num_of_products,
	priority_queue **pq_buy_arr, priority_queue **pq_sell_arr, int trader_id, trader **traders);

void return_buysell_msg(char *read_buffer, int trader_id, int *trader_pid, int* exch_wr_trad_rd);
void return_amend_msg(char *read_buffer, int trader_id, int *trader_pid, int* exch_wr_trad_rd);
void return_cancel_msg(char *read_buffer, int trader_id, int *trader_pid, int* exch_wr_trad_rd);
void return_invalid_msg(int trader_id, int *trader_pid, int* exch_wr_trad_rd);

void notify_traders_buysell(char *read_buffer, int *trader_pid, int* exch_wr_trad_rd, int trader_id, int num_of_traders);
void notify_traders_amend(char *read_buffer, int *trader_pid, int* exch_wr_trad_rde, int num_of_traders, int trader_id,
	char **products, int num_of_products, priority_queue **pq_buy_arr, priority_queue **pq_sell_arr);
void notify_traders_cancel(char *read_buffer, int *trader_pid, int* exch_wr_trad_rd, int num_of_traders, int trader_id,
	char **products, int num_of_products, priority_queue **pq_buy_arr, priority_queue **pq_sell_arr);

int get_buy_levels(priority_queue *pq_buy, int product);
int get_sell_levels(priority_queue *pq_sell, int product);
void print_orderbook(priority_queue **pq_buy_arr, priority_queue **pq_sell_arr, char **products, int num_of_products);
void print_positions(trader **traders, char **products, int num_of_products, int num_of_traders);

#endif