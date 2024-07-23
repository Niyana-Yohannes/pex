#ifndef PE_COMP_H
#define PE_COMP_H

#include "pe_common.h"
#include "pe_exchange.h"
#include "pe_out.h" 

int find_trader_id_disconnect(int *trader_pid, int* exch_wr_trad_rd, int size, int file_descriptor);
int find_trader_id(int *trader_pid, int num_of_traders, int sig_pid);

void enqueue_buy(priority_queue **pq_buy_arr, char *read_buffer, char **products, int num_of_products, int trader_id, trader **traders);
void enqueue_sell(priority_queue **pq_sell_arr, char *read_buffer, char **products, int num_of_products, int trader_id, trader **traders);
int amend(priority_queue **pq_buy_arr, priority_queue **pq_sell_arr, char *read_buffer, int num_of_products, int trader_id);
int amend_qty_help(priority_queue **pq_buy_arr, priority_queue **pq_sell_arr, int type, int order_pos, int product, int price);
int amend_price_help(priority_queue **pq_buy_arr, priority_queue **pq_sell_arr, int type, int order_pos, int product, int price);
void cancel(priority_queue **pq_buy_arr, priority_queue **pq_sell_arr, char *read_buffer, int num_of_products, int trader_id);
long match(priority_queue **pq_buy_arr, priority_queue **pq_sell_arr, char *read_buffer, char **products, int num_of_products, int trader_id, trader **traders, int num_of_traders, int *exch_wr_trad_rd, int *trader_pid, int amend_product, int amend_or_buysell);

#endif