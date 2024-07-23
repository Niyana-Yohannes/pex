/**
 * comp2017 - assignment 3
 * <Niyana Yohannes>
 * <nyoh0352>
 */

#include "pe_exchange.h"
#include "pe_comp.h"
#include "pe_out.h"

volatile sig_atomic_t signal_pid = 0;
volatile sig_atomic_t trader_disconnect = 0;

int main(int argc, char **argv) {

    //Needs to be at least 3 args
    if (argc < 3) {
        printf("Not enough arguments\n");
        return 1;
    }


    printf("%s Starting\n", LOG_PREFIX);

    ////////////////////Storing products to trader//////////////////////////
    FILE *fp;
    int num_of_products = 0;
    char temp_str[100];

    fp = fopen(argv[1], "r");
    if (fp == NULL){
        perror("Failed to open file");
        exit(0);    
    }
    fscanf(fp, "%d", &num_of_products);

    char **products = malloc(num_of_products * sizeof(char*));
    if (products == NULL) {
        printf("Failed to allocate memory!\n");
        exit(0);
    }

    int i = 0;
    while (fscanf(fp, "%s", temp_str) != EOF) {
        products[i] = malloc(sizeof(char) * (strlen(temp_str) + 1));
        strcpy(products[i], temp_str);
        i++;
    }

    fclose(fp);
    
    printf("%s Trading %d products:", LOG_PREFIX, num_of_products);
    for (int i = 0; i < num_of_products; ++i) {
        printf(" %s", products[i]);
        if (i == num_of_products - 1)
            printf("\n");
    }

    /////////////////////CREATE 2 NAMED PIPES FOR EACH TRADER//////////////////
    char exchange_fifo[FIFO_LENGTH] = {0};
    char trader_fifo[FIFO_LENGTH] = {0};
    int num_of_traders = argc - 2;
    int trader_count = 0;

    char** fifo_arr = (char**)malloc(sizeof(char*) * 2*num_of_traders);

    mode_t perm = 0600;
    for (int i = 0; i < num_of_traders; ++i) {
        snprintf(exchange_fifo, sizeof(exchange_fifo), FIFO_EXCHANGE, trader_count);
        snprintf(trader_fifo, sizeof(trader_fifo), FIFO_TRADER, trader_count);
        //Check if FIFO exists and unlink if so
        if (access(exchange_fifo, F_OK) == 0) {
            // FIFO already exists, so unlink it
            if (unlink(exchange_fifo) == -1) {
                perror("Unlink did not work");
                exit(0);
            }
        }
        //Check if FIFO exists and unlink if so
        if (access(trader_fifo, F_OK) == 0) {
            // FIFO already exists, so unlink it
            if (unlink(trader_fifo) == -1) {
                perror("Unlink did not work");
                exit(0);
            }
        }
        //make named fifos
        if (mkfifo(exchange_fifo, perm) == -1) {
            perror("error making fifo");
            exit(0);
        }
        if (mkfifo(trader_fifo, perm) == -1) {
            perror("error making fifo");
            exit(0);
        }
        //Allocate memory in fifo_arr for strings and store fifo stirng in arr
        fifo_arr[i*2] = (char*)malloc(FIFO_LENGTH*sizeof(char));
        fifo_arr[i*2+1] = (char*)malloc(FIFO_LENGTH*sizeof(char));
        strcpy(fifo_arr[i*2], exchange_fifo);
        strcpy(fifo_arr[i*2+1], trader_fifo);
        //error checking
        memset(exchange_fifo, 0, sizeof(exchange_fifo));
        memset(trader_fifo, 0, sizeof(trader_fifo));
        trader_count++;
    }

    //Preparing file descriptors array
    int *exch_wr_trad_rd = (int*)malloc(2 * sizeof(int) * num_of_traders);


    ////////////////EXCHANGE SHALL LAUNCH TRADER BINARY AS CHILD////////////////
    int *trader_pid = (int*)malloc(sizeof(int) *  num_of_traders);
    char *market_open = "MARKET OPEN;";
    char trader_id[12]; 
    trader_count = 0;
    
    for (int i = 2; i < argc; ++i) {

        snprintf(trader_id, sizeof(trader_id), "%d", trader_count);
        pid_t pid = fork();

        if (pid == -1) {
            perror("Failed to fork");
            exit(0);
        }

        else if (pid == 0) {
            printf("%s Created FIFO %s\n", LOG_PREFIX, fifo_arr[trader_count*2]);
            printf("%s Created FIFO %s\n", LOG_PREFIX, fifo_arr[trader_count*2+1]);
            printf("%s Starting trader %d (%s)\n", LOG_PREFIX, trader_count, argv[i]);
            int result = execl(argv[i], argv[i], trader_id, NULL);
            if(-1 == result) {
                perror("Failed to run trader program");
                exit(0); 
            }
        }
        else {
            trader_pid[trader_count] = pid;
        }

        //Making file descriptors
        exch_wr_trad_rd[(i-2)*2] = open(fifo_arr[trader_count*2], O_WRONLY);
        if (exch_wr_trad_rd[(i-2)*2] == -1)
            perror("Failed to open pe_exchange in write");
        exch_wr_trad_rd[(i-2)*2 + 1] = open(fifo_arr[trader_count*2+1], O_RDONLY);
        if (exch_wr_trad_rd[(i-2)*2 + 1] == -1)
            perror("Failed to open pe_trader in read");
        //Make a method for printing exchange messages
        printf("%s Connected to %s\n", LOG_PREFIX, fifo_arr[trader_count*2]);
        printf("%s Connected to %s\n", LOG_PREFIX, fifo_arr[trader_count*2+1]);
        memset(trader_id, 0, sizeof(trader_id));
        trader_count++;
    }

    ///////////////////SEND MARKET OPEN MESSAGE///////////////////////
    int j = 0;
    for (int i = 0; i < num_of_traders; ++i) {
        while(1) {
            size_t bytes_written = write(exch_wr_trad_rd[i*2], market_open + j , 1);
            if (bytes_written == -1) {
                perror("Failed to write from exchange");
                exit(0);
            }
            if (market_open[j] == ';')
                break;
            j++;
        }
        j = 0;
        kill(trader_pid[i], SIGUSR1);
    }
    

    ///////////////////Create Signal Handler for SIGUSR and SIGCHILD///////////////////////
    //register signal handler
    struct sigaction s = {0};
    s.sa_sigaction = sigaction_handler;
    s.sa_flags = SA_SIGINFO; //Specifies to use sa_sigaction instead of sa_handler
    int res;
    res = sigaction(SIGUSR1, &s, NULL);
    if (res == -1) {
        perror("Could not install handler");
        exit(0);
    }
    //register signal handler for SIGCHILD
    struct sigaction sc = {0};
    sc.sa_sigaction = sigaction_handler_sc;
    sc.sa_flags = SA_SIGINFO; //Specifies to use sa_sigaction instead of sa_handler
    res = sigaction(SIGCHLD, &sc, NULL);
    if (res == -1) {
        perror("Could not install handler");
        exit(0);
    }


    ///////////////////Create epoll structs///////////////////////
    // Create epoll_event
    struct epoll_event *arr_of_events = (struct epoll_event*)malloc(num_of_traders * sizeof(struct epoll_event));
    int epoll_fd = epoll_create1(0);
        if ( epoll_fd == -1) {
            perror("Failed to create epoll file descriptor");
            exit(0);
        }

    for (int i = 0; i < num_of_traders; ++i) {
        arr_of_events[i].events = EPOLLHUP; 
        arr_of_events[i].data.fd = exch_wr_trad_rd[i*2+1]; //Read side of trader
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, exch_wr_trad_rd[i*2+1], arr_of_events+i)) {
            perror("Failed to add file descriptor to epoll");
            exit(0);
        }
    }

    // Create epoll_event for epoll wait
    struct epoll_event *events = (struct epoll_event*)malloc(num_of_traders * sizeof(struct epoll_event));
    int num_events = 0;

    //////////////////Traders Array and Priority_Queue Buy and SELL arrays///////////////
    trader **traders = calloc(num_of_traders, sizeof(trader*));
    for (int i = 0; i < num_of_traders; ++i) {
        traders[i] = calloc(1, sizeof(trader));
        traders[i]->id = i;
        traders[i]->order_id = -1;
        traders[i]->quantity = calloc(num_of_products, sizeof(long));
        traders[i]->balance = calloc(num_of_products, sizeof(long));
    }
    priority_queue **pq_buy_arr = calloc(num_of_products,  sizeof(priority_queue*));
    for (int i = 0; i < num_of_products; ++i) {
        pq_buy_arr[i] = calloc(1, sizeof(priority_queue));
        pq_buy_arr[i]->size = 0;
    }
    priority_queue **pq_sell_arr = calloc(num_of_products,  sizeof(priority_queue*));
    for (int i = 0; i < num_of_products; ++i) {
        pq_sell_arr[i] = calloc(1, sizeof(priority_queue));
        pq_sell_arr[i]->size = 0;
    }


    ///////////////////Main While Loop///////////////////////

    char read_buffer[READ_BUFFER] = {0};
    char read_buffer_cpy[READ_BUFFER] = {0};
    long exchange_fees = 0; 

    while (1) {

        pause(); 

        //TRADER DISCONNECT
        if (trader_disconnect == 1) {
            num_events = epoll_wait(epoll_fd, events, num_of_traders, -1);
            if (num_events == -1) {
                perror("epoll_wait failed");
                exit(0);
            }
            for (int i = 0; i < num_events; ++i) {
                if (events[i].events & EPOLLHUP) {
                    //Remove from epoll_event
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, NULL) == -1) { 
                        perror("epoll_ctl failed");
                        exit(0);
                    }
                    int index = find_trader_id_disconnect(trader_pid, exch_wr_trad_rd, num_of_traders, events[i].data.fd);
                    printf("%s Trader %d disconnected\n", LOG_PREFIX, index);
                }
            }

            trader_disconnect = 0;
        }

        //TRADER ORDER
        if (signal_pid != 0) {
            //Find which trader sent it
            int trader_id = find_trader_id(trader_pid, num_of_traders, signal_pid);
            //Read message
            memset(read_buffer, 0, sizeof(read_buffer));
            size_t bytes_read = 0;
            int read_buff_p = 0;
            while (1) {
                bytes_read = read(exch_wr_trad_rd[trader_id*2+1], read_buffer + read_buff_p, 1);
                if (bytes_read == -1) {
                    perror("Failed to read from trader");
                    exit(0);
                }
                if (read_buffer[read_buff_p] == ';'){
                    read_buffer[read_buff_p] = '\0';
                    break;
                }
                read_buff_p++;
            }

            //Parse first word
            memset(read_buffer_cpy, 0, sizeof(read_buffer_cpy));
            strcpy(read_buffer_cpy, read_buffer);
            char *token = strtok(read_buffer_cpy, " ");
            int valid = 0;

            //BUY COMMAND
            if (!strcmp(token, "BUY")) {
                valid = check_valid_buysell(read_buffer, token, products, num_of_products, trader_id, traders);
                printf("%s [T%d] Parsing command: <%s>\n", LOG_PREFIX, trader_id, read_buffer);
                if (valid == 1) {
                    //Send accepted to trader 
                    return_buysell_msg(read_buffer, trader_id, trader_pid, exch_wr_trad_rd);
                    //Notify all other traders excluding current & disconnected traders!!!!!!!!!!!!!!!
                    notify_traders_buysell(read_buffer, trader_pid, exch_wr_trad_rd, trader_id, num_of_traders);
                    //Add to product buy orders
                    enqueue_buy(pq_buy_arr, read_buffer, products, num_of_products, trader_id, traders);
                    //CHECK FOR MATCH ORDER AND SEND MSG TO BUYER AND SELLER (exclude disconnected traders!!!!!!!!!!!!!)
                    exchange_fees += match(pq_buy_arr, pq_sell_arr, read_buffer, products, num_of_products, trader_id, traders, num_of_traders, exch_wr_trad_rd, trader_pid, 0, 0);
                    //Print orderbook and positions
                    print_orderbook(pq_buy_arr, pq_sell_arr, products, num_of_products);
                    print_positions(traders, products, num_of_products, num_of_traders);
                }
                //Invalid 
                else {
                    //Send invalid to trader 
                    return_invalid_msg( trader_id, trader_pid, exch_wr_trad_rd); 
                }
            }

            //SELL COMMAND
            else if (!strcmp(token, "SELL")) {
                //SELL METHOD
                valid = check_valid_buysell(read_buffer, token, products, num_of_products, trader_id, traders);
                printf("%s [T%d] Parsing command: <%s>\n", LOG_PREFIX, trader_id, read_buffer);
                if (valid == 1) {
                    //Send accepted to trader 
                    return_buysell_msg(read_buffer, trader_id, trader_pid, exch_wr_trad_rd);
                    //Notify all other traders
                    notify_traders_buysell(read_buffer, trader_pid, exch_wr_trad_rd, trader_id, num_of_traders);
                    //Add to product buy orders
                    enqueue_sell(pq_sell_arr, read_buffer, products, num_of_products, trader_id, traders );
                    //CHECK FOR MATCH ORDER AND SEND MSG TO BUYER AND SELLER
                    exchange_fees += match(pq_buy_arr, pq_sell_arr, read_buffer, products, num_of_products, trader_id, traders, num_of_traders, exch_wr_trad_rd, trader_pid, 0, 0);
                    //Print orderbook and positions
                    print_orderbook(pq_buy_arr, pq_sell_arr, products, num_of_products);
                    print_positions(traders, products, num_of_products, num_of_traders);
                }
                //Invalid
                else {
                    return_invalid_msg( trader_id, trader_pid, exch_wr_trad_rd);
                }
            }

            //AMEND COMMAND
            else if (!strcmp(token, "AMEND")) {
                //AMMEND METHOD
                valid = check_valid_amend(read_buffer, token, products, num_of_products, pq_buy_arr, pq_sell_arr, trader_id, traders);
                printf("%s [T%d] Parsing command: <%s>\n", LOG_PREFIX, trader_id, read_buffer);
                if (valid == 1) {
                    //CHECK FOR MATCH ORDER AND SEND MSG TO BUYER AND SELLER
                    //Send ammended message to trader
                    return_amend_msg(read_buffer, trader_id, trader_pid, exch_wr_trad_rd);
                    //Notify all other traders
                    notify_traders_amend(read_buffer, trader_pid, exch_wr_trad_rd, num_of_traders, trader_id, products, num_of_products, pq_buy_arr, pq_sell_arr);
                    //Amend
                    int amend_prod = amend(pq_buy_arr, pq_sell_arr, read_buffer, num_of_products, trader_id);
                    //Check for match order and send msg
                    exchange_fees += match(pq_buy_arr, pq_sell_arr, read_buffer, products, num_of_products, trader_id, traders, num_of_traders, exch_wr_trad_rd, trader_pid, amend_prod, 1);
                    //Print orderbook and positions
                    print_orderbook(pq_buy_arr, pq_sell_arr, products, num_of_products);
                    print_positions(traders, products, num_of_products, num_of_traders);
                }
                else {
                    return_invalid_msg( trader_id, trader_pid, exch_wr_trad_rd);
                }
            }

            //CANCEL COMMAND 
            else if (!strcmp(token, "CANCEL")) {
                //CANCEL METHOD;
                valid = check_valid_cancel(read_buffer, token, products, num_of_products, pq_buy_arr, pq_sell_arr, trader_id, traders);
                printf("%s [T%d] Parsing command: <%s>\n", LOG_PREFIX, trader_id, read_buffer);
                if (valid == 1) {
                    //Send cancelled message to trader
                    return_cancel_msg(read_buffer, trader_id, trader_pid, exch_wr_trad_rd);
                    //Notify all other traders
                    notify_traders_cancel(read_buffer, trader_pid, exch_wr_trad_rd, num_of_traders, trader_id, products, num_of_products, pq_buy_arr, pq_sell_arr);
                    //CANCEL order
                    cancel(pq_buy_arr, pq_sell_arr, read_buffer, num_of_products, trader_id);
                    //No need to check for match
                    //Print orderbook and positions
                    print_orderbook(pq_buy_arr, pq_sell_arr, products, num_of_products);
                    print_positions(traders, products, num_of_products, num_of_traders);
                }
                //Invalid
                else {
                    return_invalid_msg( trader_id, trader_pid, exch_wr_trad_rd);
                }

            }
            //INVALID
            else
                return_invalid_msg(trader_id, trader_pid, exch_wr_trad_rd);

            signal_pid = 0;
        }

        //If no more file descrpitors remaining 
        int trader_exists = 0;
        for (int i = 0; i < num_of_traders; ++i) {
            if (trader_pid[i] != -1) 
                trader_exists = 1;
        }

        if (trader_exists == 0) {
            printf("%s Trading completed\n", LOG_PREFIX);
            printf("%s Exchange fees collected: $%ld\n", LOG_PREFIX, exchange_fees);
            break;
        }

    }

    //Close all opened pipes
    for (int i = 0; i < 2*num_of_traders; ++i) {
        if (close(exch_wr_trad_rd[i])) {
            perror("Failed to close exchange and trader file descriptors");
            exit(0);
        }
    }
    if (close(epoll_fd)) {
        perror("Failed to close epoll file descriptor");
        exit(0);
    }

    //Clean up fifos
    trader_count = 0;
    for (int i = 0; i < num_of_traders; ++i) {
        if (unlink(fifo_arr[trader_count*2]) == -1) {
            perror("Unlink did not work");
            exit(0);
        }
        if (unlink(fifo_arr[trader_count*2+1]) == -1) {
            perror("Unlink did not work");
            exit(0);
        }
        trader_count++;
    }

    //Free at the end
    for (int j = 0; j < num_of_products; j++) {
        free(products[j]);
    }
    free(products);
    for (int i = 0; i < 2*num_of_traders; ++i) {
        free(fifo_arr[i]);
    }
    free(fifo_arr);
    for (int i = 0; i < num_of_traders; ++i) {
        free(traders[i]->quantity);
        free(traders[i]->balance);
        free(traders[i]);
    }
    free(traders);
    for (int i = 0; i < num_of_products; ++i) {
        for (int j = 0; j < pq_buy_arr[i]->size; ++j)
            free(pq_buy_arr[i]->orders[j]); 
        free(pq_buy_arr[i]->orders);
        free(pq_buy_arr[i]);
    }
    free(pq_buy_arr);
    for (int i = 0; i < num_of_products; ++i) {
        for (int j = 0; j < pq_sell_arr[i]->size; ++j) 
            free(pq_sell_arr[i]->orders[j]);
        free(pq_sell_arr[i]->orders);
        free(pq_sell_arr[i]);
    }
    free(pq_sell_arr);

    free(exch_wr_trad_rd);
    free(trader_pid);
    free(arr_of_events);
    free(events);

}

void sigaction_handler(int sig, siginfo_t* sinf, void* ucontext){
    signal_pid = sinf->si_pid;
}
void sigaction_handler_sc(int sig, siginfo_t* sinf, void* ucontext){
    trader_disconnect = 1;
}