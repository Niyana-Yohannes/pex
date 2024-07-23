#include "pe_out.h"

void write_to_trader(int *exch_wr_trad_rd, char *msg, int *trader_pid, int trader_id) {
	//Write to exchange 1 byte at a time until ;
    size_t bytes_written;
    int write_char = 0;
    if (trader_pid[trader_id] != -1) {
	    while(1) {
	        bytes_written = write(exch_wr_trad_rd[trader_id*2], msg + write_char, 1);
	        if (bytes_written == -1)
	            perror("Failed to write from exchange");
	        if (msg[write_char] == ';')
	            break;
	        write_char++;
	    }
	    //Send kill msg
	    kill(trader_pid[trader_id], SIGUSR1);
	}
}

int check_valid_buysell(char *read_buffer, char *token, char **products, int num_of_products, int trader_id, trader **traders) {
	char read_buffer_cpy[READ_BUFFER] = {0};
	strcpy(read_buffer_cpy, read_buffer);
	int order_id = 0;
	int quantity = 0;
	int price = 0;
	int white_space_counter = 0;
	char product_str[50] = {0};
	//Check for 4 spaces first
	for(int i = 0; read_buffer[i] != '\0' ; ++i){
		if(isspace(read_buffer[i])){
			white_space_counter++;
		}
	}
	if (white_space_counter != 4)
		return 0;
	//Store variables 
	char *token_new;
	//Ignore type
	token_new = strtok(read_buffer_cpy, " ");
	//Order id
	token_new = strtok(NULL, " ");
	order_id = atoi(token_new);
	//product
	token_new = strtok(NULL, " ");
	strcpy(product_str, token_new);
	//quantity
	token_new = strtok(NULL, " ");
	quantity = atoi(token_new);
	//price
	token_new = strtok(NULL, " ");
	price = atoi(token_new);
	//Check if Valid Product
	int valid_prod = 0;
	for (int i = 0; i < num_of_products; ++i) {
		if (!strcmp(products[i], product_str)) {
			valid_prod = 1;
		}
	}
	if (valid_prod == 0)
		return 0;
	//Order id
	int valid_ord = 0;
	if (traders[trader_id]->order_id == -1) {
		if (order_id == 0)
			valid_ord = 1;
	}
	else {
		if (order_id == traders[trader_id]->order_id + 1)
			valid_ord = 1;
	}
	//Quantity
	int valid_quant = 0;
	if (quantity > 0 && quantity <= 999999)
		valid_quant = 1;
	//Price
	int valid_price = 0;
	if (price > 0 && price <= 999999)
		valid_price = 1;
	//Final return	
	if (valid_prod == 1 && valid_ord == 1 && valid_quant == 1 && valid_price == 1)
		return 1;
	else
		return 0;
}

int check_valid_amend(char *read_buffer, char *token, char **products, int num_of_products,
	priority_queue **pq_buy_arr, priority_queue **pq_sell_arr, int trader_id, trader **traders) {

	char read_buffer_cpy[READ_BUFFER] = {0};
	strcpy(read_buffer_cpy, read_buffer);
	int order_id = 0;
	int quantity = 0;
	int price = 0;
	int white_space_counter = 0;
	//Check if 3 spaces
	for(int i = 0; read_buffer[i] != '\0' ; ++i){
		if(isspace(read_buffer[i])){
			white_space_counter++;
		}
	}
	if (white_space_counter != 3)
		return 0;
	//Store variables 
	char *token_new;
	//Ignore type
	token_new = strtok(read_buffer_cpy, " ");
	//Order id
	token_new = strtok(NULL, " ");
	order_id = atoi(token_new);
	//quantity
	token_new = strtok(NULL, " ");
	quantity = atoi(token_new);
	//price
	token_new = strtok(NULL, " ");
	price = atoi(token_new);
	//Check if order exists
	int valid_ord = 0;
	if (traders[trader_id]->order_id == -1) {
			valid_ord = 0;
	}
	else {
		if (order_id <= traders[trader_id]->order_id )
			valid_ord = 1;
	}
	if (valid_ord == 0)
		return 0;
	//Quantity
	int valid_quant = 0;
	if (quantity > 0 && quantity <= 999999)
		valid_quant = 1;
	if (valid_quant == 0)
		return 0;
	//Price
	int valid_price = 0;
	if (price > 0 && price <= 999999)
		valid_price = 1;
	if (valid_price == 0)
		return 0;
	//Check if order is filled
	int valid_amend = 0;
	for (int i = 0; i <  num_of_products; i++) {
		for (int j = 0; j < pq_buy_arr[i]->size; ++j) {
			int trad_id = pq_buy_arr[i]->orders[j]->trader_id;
			int ord_id = pq_buy_arr[i]->orders[j]->order_id;
			int quantity = pq_buy_arr[i]->orders[j]->quantity;
			if (trad_id == trader_id && ord_id == order_id && quantity > 0) {
				valid_amend = 1;
				break;
			}
		}
		if (valid_amend == 1)
			break;
		//Sell array
		for (int j = 0; j < pq_sell_arr[i]->size; ++j) {
			int trad_id = pq_sell_arr[i]->orders[j]->trader_id;
			int ord_id = pq_sell_arr[i]->orders[j]->order_id;
			int quantity = pq_sell_arr[i]->orders[j]->quantity;
			if (trad_id == trader_id && ord_id == order_id && quantity > 0) {
				valid_amend = 1;
				break;
			}
		}
		if (valid_amend == 1)
			break;
	}
	if (valid_amend == 0)
		return 0;
	//Reached here all goods

	return 1;
}

int check_valid_cancel(char *read_buffer, char *token, char **products, int num_of_products,
	priority_queue **pq_buy_arr, priority_queue **pq_sell_arr, int trader_id, trader **traders) {

	char read_buffer_cpy[READ_BUFFER] = {0};
	strcpy(read_buffer_cpy, read_buffer);
	int order_id = 0;
	int white_space_counter = 0;
	//Check if 1 spaces
	for(int i = 0; read_buffer[i] != '\0' ; ++i){
		if(isspace(read_buffer[i])){
			white_space_counter++;
		}
	}
	if (white_space_counter != 1)
		return 0;
	//Store variables 
	char *token_new;
	//Ignore type
	token_new = strtok(read_buffer_cpy, " ");
	//Order id
	token_new = strtok(NULL, " ");
	order_id = atoi(token_new);
	//Check if order exists
	int valid_ord = 0;
	if (traders[trader_id]->order_id == -1) {
			valid_ord = 0;
	}
	else {
		if (order_id <= traders[trader_id]->order_id)
			valid_ord = 1;
	}
	if (valid_ord == 0)
		return 0;
	//Check if exists an order with order id and quantity > 0
	//Have to check through both buy and sell arr
	//Also has to be for spcific trader
	int valid_cancel = 0;
	for (int i = 0; i < num_of_products; ++i) {
		//Buy array
		for (int j = 0; j < pq_buy_arr[i]->size; ++j) {
			int trad_id = pq_buy_arr[i]->orders[j]->trader_id;
			int ord_id = pq_buy_arr[i]->orders[j]->order_id;
			int quantity = pq_buy_arr[i]->orders[j]->quantity;
			if (trad_id == trader_id && ord_id == order_id && quantity > 0) {
				valid_cancel = 1;
				break;
			}
		}
		if (valid_cancel == 1) 
			break;
		//Sell array
		for (int j = 0; j < pq_sell_arr[i]->size; ++j) {
			int trad_id = pq_sell_arr[i]->orders[j]->trader_id;
			int ord_id = pq_sell_arr[i]->orders[j]->order_id;
			int quantity = pq_sell_arr[i]->orders[j]->quantity;
			if (trad_id == trader_id && ord_id == order_id && quantity > 0) {
				valid_cancel = 1;
				break;
			}
		}
		if (valid_cancel == 1)
			break;
	}
	if (valid_cancel == 0)
		return 0;

	//Reached here all goods
	return 1;
} 

void return_buysell_msg(char *read_buffer, int trader_id, int *trader_pid, int* exch_wr_trad_rd) {
	char read_buffer_cpy[READ_BUFFER] = {0};
	strcpy(read_buffer_cpy, read_buffer);
	int order_id = 0;
	char accepted_msg[ACCEPTED_BUFFER] = {0};
	//Store variables 
	char *token_new = {0};
	//Order id
	token_new = strtok(read_buffer_cpy, " ");
	token_new = strtok(NULL, " ");
	order_id = atoi(token_new);
    snprintf(accepted_msg, sizeof(accepted_msg), "ACCEPTED %d;", order_id);
    write_to_trader(exch_wr_trad_rd, accepted_msg, trader_pid, trader_id);
}

void return_amend_msg(char *read_buffer, int trader_id, int *trader_pid, int* exch_wr_trad_rd) {
	char read_buffer_cpy[READ_BUFFER] = {0};
	strcpy(read_buffer_cpy, read_buffer);
	int order_id = 0;
	char accepted_msg[ACCEPTED_BUFFER] = {0};
	//Store variables 
	char *token_new = {0};
	//Order id
	token_new = strtok(read_buffer_cpy, " ");
	token_new = strtok(NULL, " ");
	order_id = atoi(token_new);
    snprintf(accepted_msg, sizeof(accepted_msg), "AMENDED %d;", order_id);
    write_to_trader(exch_wr_trad_rd, accepted_msg, trader_pid, trader_id);
}

void return_cancel_msg(char *read_buffer, int trader_id, int *trader_pid, int* exch_wr_trad_rd) {
	char read_buffer_cpy[READ_BUFFER] = {0};
	strcpy(read_buffer_cpy, read_buffer);
	int order_id = 0;
	char accepted_msg[ACCEPTED_BUFFER] = {0};
	//Store variables 
	char *token_new = {0};
	//Order id
	token_new = strtok(read_buffer_cpy, " ");
	token_new = strtok(NULL, " ");
	order_id = atoi(token_new);
	snprintf(accepted_msg, sizeof(accepted_msg), "CANCELLED %d;", order_id);
	write_to_trader(exch_wr_trad_rd, accepted_msg, trader_pid, trader_id);
}

void return_invalid_msg(int trader_id, int *trader_pid, int* exch_wr_trad_rd) {
	char accepted_msg[ACCEPTED_BUFFER] = {0};
	strcpy(accepted_msg, "INVALID;");
	write_to_trader(exch_wr_trad_rd, accepted_msg, trader_pid, trader_id);
}

void notify_traders_buysell(char *read_buffer, int *trader_pid, int* exch_wr_trad_rd, int trader_id, int num_of_traders) {
	char read_buffer_cpy[READ_BUFFER] = {0};
	strcpy(read_buffer_cpy, read_buffer);
	char notify_msg[NOTIFY_BUFFER];
	//Store variables 
	char *token_new;
	int quantity, price;
	char product_str[PRODUCT_BUFFER], buy_sell[5];
	token_new = strtok(read_buffer_cpy, " ");
	strcpy(buy_sell, token_new);
	//Order id not used
	token_new = strtok(NULL, " ");
	//product	
	token_new = strtok(NULL, " ");
	strcpy(product_str, token_new);
	//Quantity
	token_new = strtok(NULL, " ");
	quantity = atoi(token_new);
	//price
	token_new = strtok(NULL, " ");
	price = atoi(token_new);
	//Combine the message
    snprintf(notify_msg, sizeof(notify_msg), "MARKET %s %s %d %d;", buy_sell, product_str, quantity, price);
    for (int i = 0; i < num_of_traders; ++i) {
	    if (i != trader_id && trader_pid[i] != -1) 
			write_to_trader(exch_wr_trad_rd, notify_msg, trader_pid, i);
	}
}   

void notify_traders_amend(char *read_buffer, int *trader_pid, int* exch_wr_trad_rd, int num_of_traders, int trader_id,
	char **products, int num_of_products, priority_queue **pq_buy_arr, priority_queue **pq_sell_arr) {
	
	char read_buffer_cpy[READ_BUFFER] = {0};
	strcpy(read_buffer_cpy, read_buffer);
	char notify_msg[NOTIFY_BUFFER];
	//Store variables 
	char *token_new;
	int quantity, price;
	char product_str[PRODUCT_BUFFER], buy_sell[5];

	//Store variables 
	//Ignore type
	token_new = strtok(read_buffer_cpy, " ");
	//Order id
	token_new = strtok(NULL, " ");
	int order_id = atoi(token_new);
	//quantity
	token_new = strtok(NULL, " ");
	quantity = atoi(token_new);
	//price
	token_new = strtok(NULL, " ");
	price = atoi(token_new);
	//Need to get product type
	int order_type = 0;
	int product_type = 0;
	int found = 0;
	for (int i = 0; i < num_of_products; ++i) {
		//Buy array
		for (int j = 0; j < pq_buy_arr[i]->size; ++j) {
			int trad_id = pq_buy_arr[i]->orders[j]->trader_id;
			int ord_id = pq_buy_arr[i]->orders[j]->order_id;
			int quantity = pq_buy_arr[i]->orders[j]->quantity;
			if (trad_id == trader_id && ord_id == order_id && quantity > 0) {
				order_type = 0;
				product_type = i;
				found = 1;
				break;
			}
			if (found == 1)
				break;
		}
		if (found == 1)
			break;
		//Sell array
		for (int j = 0; j < pq_sell_arr[i]->size; ++j) {
			int trad_id = pq_sell_arr[i]->orders[j]->trader_id;
			int ord_id = pq_sell_arr[i]->orders[j]->order_id;
			int quantity = pq_sell_arr[i]->orders[j]->quantity;
			if (trad_id == trader_id && ord_id == order_id && quantity > 0) { 
				order_type = 1;
				product_type = i;
				found = 1;
			}
			if (found == 1)
				break;
		}
		if (found == 1)
			break;
	}
	//Get name of product and type of order
	strcpy(product_str, products[product_type]);
	if (order_type == 0)
		strcpy(buy_sell, "BUY"); 
	else
		strcpy(buy_sell, "SELL"); 
	//Combine the message
    snprintf(notify_msg, sizeof(notify_msg), "MARKET %s %s %d %d;", buy_sell, product_str, quantity, price);
    for (int i = 0; i < num_of_traders; ++i) {
	    if (i != trader_id && trader_pid[i] != -1) 
			write_to_trader(exch_wr_trad_rd, notify_msg, trader_pid, i);
	}
}

void notify_traders_cancel(char *read_buffer, int *trader_pid, int* exch_wr_trad_rd, int num_of_traders, int trader_id,
	char **products, int num_of_products, priority_queue **pq_buy_arr, priority_queue **pq_sell_arr) {

	char read_buffer_cpy[READ_BUFFER] = {0};
	strcpy(read_buffer_cpy, read_buffer);
	char notify_msg[NOTIFY_BUFFER];
	//Store variables 
	char *token_new;
	int quantity, price;
	char product_str[PRODUCT_BUFFER], buy_sell[5];
	//Store variables 
	//Ignore type
	token_new = strtok(read_buffer_cpy, " ");
	//Order id
	token_new = strtok(NULL, " ");
	int order_id = atoi(token_new);
	//Need to get product type
	int order_type = 0;
	int product_type = 0;
	for (int i = 0; i < num_of_products; ++i) {
		//Buy array
		for (int j = 0; j < pq_buy_arr[i]->size; ++j) {
			int trad_id = pq_buy_arr[i]->orders[j]->trader_id;
			int ord_id = pq_buy_arr[i]->orders[j]->order_id;
			int quantity = pq_buy_arr[i]->orders[j]->quantity;
			if (trad_id == trader_id && ord_id == order_id && quantity > 0) {
				order_type = 0;
				product_type = i;
			}
		}
		//Sell array
		for (int j = 0; j < pq_sell_arr[i]->size; ++j) {
			int trad_id = pq_sell_arr[i]->orders[j]->trader_id;
			int ord_id = pq_sell_arr[i]->orders[j]->order_id;
			int quantity = pq_sell_arr[i]->orders[j]->quantity;
			if (trad_id == trader_id && ord_id == order_id && quantity > 0) { 
				order_type = 1;
				product_type = i;
			}
		}
	}
	//Get name of product and type of order
	strcpy(product_str, products[product_type]);
	if (order_type == 0)
		strcpy(buy_sell, "BUY"); 
	else
		strcpy(buy_sell, "SELL"); 
	quantity = 0;
	price = 0;
	//Combine the message
    snprintf(notify_msg, sizeof(notify_msg), "MARKET %s %s %d %d;", buy_sell, product_str, quantity, price);
    for (int i = 0; i < num_of_traders; ++i) {
	    if (i != trader_id && trader_pid[i] != -1) 
			write_to_trader(exch_wr_trad_rd, notify_msg, trader_pid, i);
	}
}

int get_buy_levels(priority_queue *pq_buy, int product) {
	int buy_levels = 0;
	int new_level = 0;

	for (int j = pq_buy->size-1; j >= 0; j--) {
		if (pq_buy->orders[j]->quantity != 0) {
			if (new_level != pq_buy->orders[j]->price) {
				buy_levels++;
				new_level = pq_buy->orders[j]->price;
			}
		}
	}
	return buy_levels;
}

int get_sell_levels(priority_queue *pq_sell, int product) {
	int sell_levels = 0;
	int new_level = 0;
	for (int j = pq_sell->size-1; j >= 0; j--) {
		if (pq_sell->orders[j]->quantity != 0) {
			if (new_level != pq_sell->orders[j]->price) {
				sell_levels++;
				new_level = pq_sell->orders[j]->price;
			}
		}
	}
	return sell_levels;
}

void print_orderbook(priority_queue **pq_buy_arr, priority_queue **pq_sell_arr, char **products, int num_of_products) {

	printf("%s\t--ORDERBOOK--\n", LOG_PREFIX);

	//Want to print for each product
	for (int i = 0; i < num_of_products; ++i) {

		priority_queue *pq_buy = pq_buy_arr[i];
		priority_queue *pq_sell = pq_sell_arr[i];
		int buy_levels = get_buy_levels(pq_buy, i);
		int sell_levels = get_sell_levels(pq_sell, i);

		printf("%s\tProduct: %s; Buy levels: %d; Sell levels: %d\n", LOG_PREFIX ,products[i], buy_levels, sell_levels);

		//SELL ORDERS
		//Want to start from beginning of array as sorted by min
		for (int j = 0; j < pq_sell->size; ++j) {

			//Go through each price and see if there are others 
			int orders = 1;
			int quantity = pq_sell->orders[j]->quantity;

			if (quantity != 0) {

				for (int z = j; z < pq_sell->size; ++z) {

					if (pq_sell->orders[z]->product_id == i) {

						if (j != z) {
							if (pq_sell->orders[z]->price == pq_sell->orders[j]->price) { 
								orders++;
								quantity += pq_sell->orders[z]->quantity;
							}
						}
					}
				}
				if (orders == 1)
					printf("%s\t\tSELL %d @ $%ld (%d order)\n", LOG_PREFIX, quantity, pq_sell->orders[j]->price, orders);
				else
					printf("%s\t\tSELL %d @ $%ld (%d orders)\n", LOG_PREFIX, quantity, pq_sell->orders[j]->price, orders);
				//Skip orders already done
				if (orders != 1)
					j += orders-1;
			}
		}
		//BUY ORDERS
		for (int j = pq_buy->size-1; j >= 0; j--) {

			//Go through each price and see if there are others 
			int orders = 1;
			int quantity = pq_buy->orders[j]->quantity;

			if (quantity != 0) {

				for (int z = j; z >= 0; z--) {

					if (pq_buy->orders[z]->product_id == i) {

						if (j != z) {
							if (pq_buy->orders[z]->price == pq_buy->orders[j]->price) { 
								orders++;
								quantity += pq_buy->orders[z]->quantity;
							}
						}
					}
				}
				if (orders == 1)
					printf("%s\t\tBUY %d @ $%ld (%d order)\n", LOG_PREFIX, quantity, pq_buy->orders[j]->price, orders);
				else
					printf("%s\t\tBUY %d @ $%ld (%d orders)\n", LOG_PREFIX, quantity, pq_buy->orders[j]->price, orders);
				//Skip orders already done
				if (orders != 1)
					j -= orders-1;
			}
		}
	}
}

void print_positions(trader **traders, char **products, int num_of_products, int num_of_traders) {

	printf("%s\t--POSITIONS--\n", LOG_PREFIX);

	for (int i = 0; i < num_of_traders; ++i) {
		printf("%s\tTrader %d: ", LOG_PREFIX, traders[i]->id);
		for (int j = 0; j < num_of_products; ++j) {
			if (j == 0 && j == num_of_products-1)
				printf("%s %ld ($%ld)\n", products[j], traders[i]->quantity[j], traders[i]->balance[j]);
			else if (j == 0)
				printf("%s %ld ($%ld)", products[j], traders[i]->quantity[j], traders[i]->balance[j]);
			else if (j == num_of_products-1)
				printf(", %s %ld ($%ld)\n", products[j], traders[i]->quantity[j], traders[i]->balance[j]);
			else
				printf(", %s %ld ($%ld)", products[j], traders[i]->quantity[j], traders[i]->balance[j]);
		}
	}
} 

