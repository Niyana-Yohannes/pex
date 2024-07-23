#include "pe_comp.h"

int find_trader_id_disconnect(int *trader_pid, int* exch_wr_trad_rd, int size, int file_descriptor) {
	for (int i = 0; i < 2*size; i++) {
		//If found, replace pid with -1 to indicate trader no longer exists
		if (exch_wr_trad_rd[i] == file_descriptor) {
			int trader_indx = (i-1) / 2;
			trader_pid[trader_indx] = -1;
			return trader_indx;
		}
	}
	//if not found, return -1
	return -1;
}

int find_trader_id(int *trader_pid, int num_of_traders, int sig_pid) {
	for (int i = 0; i < num_of_traders; ++i) {
		if (sig_pid == trader_pid[i])
			return i;
	}
	//If does not find, that means message sent from trader disconnected
	return 0;
}

void enqueue_buy(priority_queue **pq_buy_arr, char *read_buffer, char **products, int num_of_products, int trader_id, trader **traders) {
	char read_buffer_cpy[READ_BUFFER] = {0};
	strcpy(read_buffer_cpy, read_buffer);
	//Store variables 
	char *token;
	int order_id, quantity, price;
	char product_str[50];
	//Dont care about first word
	token = strtok(read_buffer_cpy, " ");
	//Order id
	token = strtok(NULL, " ");
	order_id = atoi(token);
	//product	
	token = strtok(NULL, " ");
	strcpy(product_str, token);
	//Quantity
	token = strtok(NULL, " ");
	quantity = atoi(token);
	//price
	token = strtok(NULL, " ");
	price = atoi(token);
	//Get the priority_queue for the correct product
	int product = 0;
	for (int i = 0; i < num_of_products; ++i) {
		if (!strcmp(products[i], product_str)) {
			product = i;
		}
	}
	priority_queue *pq = pq_buy_arr[product];

	int pq_old_size = pq->size;
	pq->orders = (order**)realloc(pq->orders, (pq->size + 1)*sizeof(order*));
	pq->size++;

	int return_indx = -1;
	//If first order, place at the front
	if (pq->size == 1) 
		return_indx = 0;
	// Find the appropriate index to insert the order
	for (int i = 0; i < pq->size - 1; ++i) {
	    if (price <= pq->orders[i]->price) {
	        return_indx = i;
	        break;
	    }
	}
	// If the order should be placed at the end
	if (return_indx == -1 && price > pq->orders[pq_old_size - 1]->price) {
	    return_indx = pq_old_size;
	}
	// Shift orders to the right of the insertion index
	if (return_indx != pq_old_size) {
		for (int i = pq_old_size - 1; i >= return_indx; i--) {
		    pq->orders[i + 1] = pq->orders[i];
		}
	}
	// Create and insert the new order at the appropriate index
	pq->orders[return_indx] = (order*)calloc(1, sizeof(order));
	pq->orders[return_indx]->trader_id = trader_id;
	pq->orders[return_indx]->order_id = order_id;
	pq->orders[return_indx]->quantity = quantity;
	pq->orders[return_indx]->type = 0;
	pq->orders[return_indx]->price = price;
	pq->orders[return_indx]->product_id = product;
	// Update trader order_id
	traders[trader_id]->order_id++;
}

void enqueue_sell(priority_queue **pq_sell_arr, char *read_buffer, char **products, int num_of_products, int trader_id, trader **traders) {
	//Copy of string
	char read_buffer_cpy[READ_BUFFER] = {0};
	strcpy(read_buffer_cpy, read_buffer);
	//Store variables 
	char *token;
	int order_id, quantity, price;
	char product_str[50];
	//Dont care about first word
	token = strtok(read_buffer_cpy, " ");
	//Order id
	token = strtok(NULL, " ");
	order_id = atoi(token);
	//product
	token = strtok(NULL, " ");
	strcpy(product_str, token);
	//Quantity
	token = strtok(NULL, " ");
	quantity = atoi(token);
	//price
	token = strtok(NULL, " ");
	price = atoi(token);
	//Get priority queue for correct product
	int product = 0;
	for (int i = 0; i < num_of_products; ++i) {
		if (!strcmp(products[i], product_str)) {
			product = i;
		}
	}
	priority_queue *pq = pq_sell_arr[product];

	int pq_old_size = pq->size;
	pq->orders = (order**)realloc(pq->orders, (pq->size + 1)*sizeof(order*));
	pq->size++;
	
	int return_indx = -1;
	//If first order, place at the front
	if (pq->size == 1) {
		return_indx = 0;
	}
	// Find the appropriate index to insert the order
	for (int i = 0; i < pq->size - 1; ++i) {
	    if (price >= pq->orders[i]->price) {
	        return_indx = i;
	        break;
	    }
	}
	// If the order should be placed at the end
	if (return_indx == -1 && price < pq->orders[pq_old_size - 1]->price) {
	    return_indx = pq_old_size;
	}
	//Shift all orders to the right of i
	if (return_indx != pq_old_size) {
		for (int i = pq_old_size-1; i >= return_indx; i--) {
			pq->orders[i+1] = pq->orders[i];
		}
	}
	// Create and insert the new order at the appropriate index
	pq->orders[return_indx] = (order*)calloc(1, sizeof(order));
	pq->orders[return_indx]->trader_id = trader_id;
	pq->orders[return_indx]->order_id = order_id;
	pq->orders[return_indx]->quantity = quantity;
	pq->orders[return_indx]->type = 0;
	pq->orders[return_indx]->price = price;
	pq->orders[return_indx]->product_id = product;
	//Update trader order_id
	traders[trader_id]->order_id++;
}

int amend(priority_queue **pq_buy_arr, priority_queue **pq_sell_arr, char *read_buffer, int num_of_products, int trader_id) {
	//Store variables 
	char *token;
	int order_id = 0;
	int quantity = 0;
	int price = 0;
	char read_buffer_cpy[READ_BUFFER] = {0};
	strcpy(read_buffer_cpy, read_buffer);
	//Ignore type
	token = strtok(read_buffer_cpy, " ");
	//Order id
	token = strtok(NULL, " ");
	order_id = atoi(token);
	//Quantity
	token = strtok(NULL, " ");
	quantity = atoi(token);
	//price
	token = strtok(NULL, " ");
	price = atoi(token);

	//Get where product is
	int product = 0;
	int type = 0;
	int order_pos = 0;
	int found = 0;
	int price_or_qty = 0;
	for (int i = 0; i <  num_of_products; i++) {
		for (int j = 0; j < pq_buy_arr[i]->size; ++j) {
			int trad_id = pq_buy_arr[i]->orders[j]->trader_id;
			int ord_id = pq_buy_arr[i]->orders[j]->order_id;
			int qty = pq_buy_arr[i]->orders[j]->quantity;
			if (trad_id == trader_id && ord_id == order_id && qty > 0) {
				product = i;
				type = 1;
				order_pos = j;
				found = 1;
				if (pq_buy_arr[i]->orders[j]->price == price && qty != quantity)
					price_or_qty = 1;
				break;
			}
		}
		if (found == 1)
			break;
		//Sell array
		for (int j = 0; j < pq_sell_arr[i]->size; ++j) {
			int trad_id = pq_sell_arr[i]->orders[j]->trader_id;
			int ord_id = pq_sell_arr[i]->orders[j]->order_id;
			int qty = pq_sell_arr[i]->orders[j]->quantity;
			if (trad_id == trader_id && ord_id == order_id && qty > 0) {
				product = i;
				type = 2;
				order_pos = j;
				found = 1;
				if (pq_sell_arr[i]->orders[j]->price == price && qty != quantity)
					price_or_qty = 1;
				break;
			}
		}
		if (found == 1)
			break;
	}


	int return_indx = 0;
	//If there isn't don't have to do anything
	if (!price_or_qty) 
		return_indx = amend_price_help(pq_buy_arr, pq_sell_arr, type, order_pos, product, price);
	else if (price_or_qty) 
		return_indx = amend_qty_help(pq_buy_arr, pq_sell_arr, type, order_pos, product, price);

	//Amend product and rearrange array based 
	priority_queue *pq = {0};
	if (type == 1) 
		pq = pq_buy_arr[product];
	
	else if (type == 2) 
		pq = pq_sell_arr[product];

	if (return_indx != -1) {
		//Once got return index, rearrange array appropriately
		if (return_indx != order_pos) {
			if (return_indx < order_pos) {	 
				//Shift all orders to the right of i
				order *temp = pq->orders[order_pos];
				for (int i = order_pos-1; i >= return_indx; --i) {
					pq->orders[i+1] = pq->orders[i];
				}
				pq->orders[return_indx] = temp;
				pq->orders[return_indx]->price = price;
				pq->orders[return_indx]->quantity = quantity;
			}
			else if (return_indx > order_pos) {
				//Shift all orders to the left of i
				order *temp = pq->orders[order_pos];
				for (int i = order_pos; i <= return_indx-1; ++i) {
					pq->orders[i] = pq->orders[i+1];
				}
				pq->orders[return_indx] = temp;
				pq->orders[return_indx]->price = price;
				pq->orders[return_indx]->quantity = quantity;
			}
		}
		else if (return_indx == order_pos) {
			pq->orders[return_indx]->price = price;
			pq->orders[return_indx]->quantity = quantity;
		}
	}

	return product;
}

int amend_qty_help(priority_queue **pq_buy_arr, priority_queue **pq_sell_arr, int type, int order_pos, int product, int price) {

	//If only quantity changed, have to rearrange array to fullfil time-priority
	priority_queue *pq = {0};
	int return_indx = -1;
	if (type == 1) {
		pq = pq_buy_arr[product];
		//Find appropriate index to insert order;
		//Find last order to right that has same price
		for (int i = order_pos+1; i < pq->size; ++i) {
			if (pq->orders[i]->price == pq->orders[order_pos]->price) 
				return_indx = i;
		}
	}
	else if (type == 2) {
		pq = pq_sell_arr[product];
		//Find appropriate index to insert order;
		//Find closest order to left that has same price
		for (int i = order_pos+1; i < pq->size; ++i) {
			if (pq->orders[i]->price == pq->orders[order_pos]->price) 
				return_indx = i;
		}
	}

	return return_indx;
}

int amend_price_help(priority_queue **pq_buy_arr, priority_queue **pq_sell_arr, int type, int order_pos, int product, int price) {
	//Amend product and rearrange array based 
	priority_queue *pq = {0};
	int return_indx = -1;
	if (type == 1) {
		pq = pq_buy_arr[product];
		// Find the appropriate index to insert the order
		for (int i = 0; i < pq->size - 1; ++i) {
		    if (price <= pq->orders[i]->price) {
		        return_indx = i;
		        break;
		    }
		}
		// If amend bigger than greatest
		if (return_indx == -1 && price > pq->orders[pq->size - 1]->price) {
		    return_indx = pq->size-1;
		}
		//If amend equals greatest
		if (return_indx == -1 && price == pq->orders[pq->size - 1]->price) {
		    return_indx = pq->size-2;
		}
	}
	else if (type == 2) {
		pq = pq_sell_arr[product];
		// Find the appropriate index to insert the order
		for (int i = 0; i < pq->size - 1; ++i) {
		    if (price >= pq->orders[i]->price) {
		        return_indx = i;
		        break;
		    }
		}
		// If amend smaller than smallest
		if (return_indx == -1 && price < pq->orders[pq->size - 1]->price) {
		    return_indx = pq->size-1;
		}
		//If amend equals smallest
		if (return_indx == -1 && price == pq->orders[pq->size - 1]->price) {
		    return_indx = pq->size-2;
		}
	}

	return return_indx;
}

void cancel(priority_queue **pq_buy_arr, priority_queue **pq_sell_arr, char *read_buffer, int num_of_products, int trader_id) {
	//Store variables 
	char *token;
	int order_id = 0;
	char read_buffer_cpy[READ_BUFFER] = {0};
	strcpy(read_buffer_cpy, read_buffer);
	//Ignore type
	token = strtok(read_buffer_cpy, " ");
	//Order id
	token = strtok(NULL, " ");
	order_id = atoi(token);
	for (int i = 0; i < num_of_products; ++i) {
		//Buy array
		for (int j = 0; j < pq_buy_arr[i]->size; ++j) {
			int trad_id = pq_buy_arr[i]->orders[j]->trader_id;
			int ord_id = pq_buy_arr[i]->orders[j]->order_id;
			int quantity = pq_buy_arr[i]->orders[j]->quantity;
			if (trad_id == trader_id && ord_id == order_id && quantity > 0) 
				pq_buy_arr[i]->orders[j]->quantity = 0;
		}
		//Sell array
		for (int j = 0; j < pq_sell_arr[i]->size; ++j) {
			int trad_id = pq_sell_arr[i]->orders[j]->trader_id;
			int ord_id = pq_sell_arr[i]->orders[j]->order_id;
			int quantity = pq_buy_arr[i]->orders[j]->quantity;
			if (trad_id == trader_id && ord_id == order_id && quantity > 0) 
				pq_sell_arr[i]->orders[j]->quantity = 0;
		}
	}
}

long match(priority_queue **pq_buy_arr, priority_queue **pq_sell_arr, char *read_buffer, char **products, int num_of_products, int trader_id, trader **traders, int num_of_traders, int *exch_wr_trad_rd, int *trader_pid, int amend_product, int amend_or_buysell) {
	//Copy of string
	char read_buffer_cpy[READ_BUFFER] = {0};
	strcpy(read_buffer_cpy, read_buffer);
	//Store variables 
	char *token;
	int type;
	char product_str[50];
	//BUY or SELL
	token = strtok(read_buffer_cpy, " ");
	if (!strcmp(token, "BUY"))
		type = 0;
	else
		type = 1;
	//Dont care about order id
	token = strtok(NULL, " ");
	//product
	int new_product = 0;
	if (amend_or_buysell == 0) {
		token = strtok(NULL, " ");
		strcpy(product_str, token);
		//Get priority_queues for correct product
		for (int i = 0; i < num_of_products; ++i) {
			if (!strcmp(products[i], product_str)) {
				new_product = i;
			}
		}
	}
	else
		new_product = amend_product;

	priority_queue *pq_buy = pq_buy_arr[new_product];
	priority_queue *pq_sell = pq_sell_arr[new_product];

	//Only do below if at least 1 buy order and 1 sell order
	long total_pex_cut = 0;
	if (pq_buy->size > 0 && pq_sell->size > 0) {

		for (int i = pq_buy->size-1; i >= 0; i--) {

			int pq_buy_price = pq_buy->orders[i]->price;
			int pq_buy_qty = pq_buy->orders[i]->quantity;

			for (int j = pq_sell->size-1; j >= 0; j--) {

				int pq_sell_price = pq_sell->orders[j]->price;
				int pq_sell_qty = pq_sell->orders[j]->quantity;

				if (pq_sell_price > pq_buy_price)
					; //Do nothing
				else if (pq_sell_qty == 0 || pq_buy_qty == 0) 
					; //Do nothing
				else {
					long match_price = 0;
					long match_quantity = 0;
					long pex_cut = 0;
					while (pq_sell_qty != 0 && pq_buy_qty != 0) {
						match_quantity++;
						pq_sell_qty--;
						pq_buy_qty--;
					}
					//If only 1 order, match price is not last order but older order
					int buy_orders = 0;
					int sell_orders = 0;
					for (int a = 0; a < pq_buy->size; ++a) {
						if (pq_buy->orders[a]->quantity > 0)
							buy_orders++;
					}
					for (int a = 0; a < pq_sell->size; ++a) {
						if (pq_sell->orders[a]->quantity > 0)
							sell_orders++;
					}
					if (buy_orders == 1 && sell_orders == 1) {
						int buy_trad_id = pq_buy->orders[i]->trader_id;
						int sell_trad_id = pq_sell->orders[j]->trader_id;
						if (trader_id == buy_trad_id) 
							match_price = match_quantity*pq_sell_price;
						else if (trader_id == sell_trad_id)
							match_price = match_quantity*pq_buy_price;
					} 
					//Else, match price follows price-time
					else {
						match_price = match_quantity*pq_buy_price;
					}
					pq_buy->orders[i]->quantity = pq_buy_qty;
					pq_sell->orders[j]->quantity = pq_sell_qty;
					pex_cut = round(match_price*0.01);
					total_pex_cut += pex_cut;
					if (type == 1)
						printf("%s Match: Order %d [T%d], New Order %d [T%d], value: $%ld, fee: $%ld.\n", LOG_PREFIX, pq_buy->orders[i]->order_id, pq_buy->orders[i]->trader_id, pq_sell->orders[j]->order_id, pq_sell->orders[j]->trader_id, match_price, pex_cut);
					else	
						printf("%s Match: Order %d [T%d], New Order %d [T%d], value: $%ld, fee: $%ld.\n", LOG_PREFIX, pq_sell->orders[j]->order_id, pq_sell->orders[j]->trader_id, pq_buy->orders[i]->order_id, pq_buy->orders[i]->trader_id, match_price, pex_cut);
					//Update traders
					for (int z = 0; z < num_of_traders; ++z) { //Needs to know num of traders
						if (traders[z]->id == pq_buy->orders[i]->trader_id) {
							traders[z]->quantity[new_product] += match_quantity;
							traders[z]->balance[new_product] -= match_price;
						}
						else if (traders[z]->id == pq_sell->orders[j]->trader_id) {
							traders[z]->quantity[new_product] -= match_quantity;
							traders[z]->balance[new_product] += match_price;
						}
					}
					//SEND FILL ORDER MESSAGE TO BUYER THAN SELLER TRADERS
					char fill_buffer_buy[FILL_BUFFER] = {0};
					char fill_buffer_sell[FILL_BUFFER] = {0};
					snprintf(fill_buffer_buy, sizeof(fill_buffer_buy), "FILL %d %ld;", pq_buy->orders[i]->order_id, match_quantity);
					snprintf(fill_buffer_sell, sizeof(fill_buffer_sell), "FILL %d %ld;", pq_sell->orders[j]->order_id, match_quantity);
					write_to_trader(exch_wr_trad_rd, fill_buffer_buy, trader_pid, pq_buy->orders[i]->trader_id);
					write_to_trader(exch_wr_trad_rd, fill_buffer_sell, trader_pid, pq_sell->orders[j]->trader_id);
				}
			}
		}
	}

	//Take 1% cut from last order made and add to exchange
	for (int i = 0; i < num_of_traders; i++) {
		if (traders[i]->id == trader_id) {
			traders[i]->balance[new_product] -= total_pex_cut;
		}
	}
	return total_pex_cut;
}

