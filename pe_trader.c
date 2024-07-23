#include "pe_trader.h"

 volatile sig_atomic_t interrupt = 0;

int main(int argc, char ** argv) {

    if (argc < 2) {
        printf("Not enough arguments\n");
        return 1;
    }

    // Setup variables
    int order_id = 0;
    int i = 0;
    char read_buffer[READ_SIZE] = {0};
    char copy_read_buffer[READ_SIZE] = {0};
    char write_buffer[READ_SIZE] = {0};
    char *token;
    char *is_sell_msg[CHECK_IF_SELL];
    char *sell_message_sep[SELL_MSG_SEP];

    // register signal handler
    struct sigaction s = {0};
    s.sa_sigaction = sigaction_handler;
    s.sa_flags = SA_SIGINFO; //Specifies to use sa_sigaction instead of sa_handler
    int res;
    res = sigaction(SIGUSR1, &s, NULL);
    if (res == -1) {
        perror("Could not install handler");
        return -1;
    }

    // connect to named pipes
    int value = atoi(argv[1]);
    char filename1[100];
    char filename2[100];
    snprintf(filename1, sizeof(filename1), FIFO_EXCHANGE, value);
    snprintf(filename2, sizeof(filename2), FIFO_TRADER, value);
    int fd_read = open(filename1, O_RDONLY);
    if (fd_read == -1)
        perror("Open for read failed");
    int fd_write = open(filename2, O_WRONLY);
    if (fd_write == -1)
        perror("Open for write failed");
     
    // Main while loop
    int can_trade = 0;

    while(1) {

        if (interrupt) {

            //Read message and store in array
            size_t bytes_read = read(fd_read, read_buffer, sizeof(read_buffer));
            if (bytes_read == -1)
                perror("Failed to read from exchange");
            read_buffer[bytes_read] = '\0';

            //Store first 2 words in array for checking if SELL
            strcpy(copy_read_buffer, read_buffer);
            token = strtok(copy_read_buffer, " ");
            while ( token != NULL && i < CHECK_IF_SELL) {
                is_sell_msg[i] = token;
                token = strtok(NULL, " ");
                i++;
            }
            i = 0;

            //Check if market open before doing anything
            if( !(strcmp(is_sell_msg[0], "MARKET")) && !(strcmp(is_sell_msg[1], "OPEN;")) ) {
                can_trade = 1;
            }

            //CHECK IF SELL MESSAGE
            else if( !(strcmp(is_sell_msg[0], "MARKET")) && !(strcmp(is_sell_msg[1], "SELL")) ) {
                if (can_trade) {
                    //Store message in array
                    token = strtok(read_buffer, " ");
                    while (token != NULL && i < 5) {
                        sell_message_sep[i] = token;
                        token = strtok(NULL, " ");
                        i++;
                    }
                    i = 0;
                    //Check if quantity above 1000
                    if(atoi(sell_message_sep[3]) > 999) {
                        break;
                    }
                    //Else, return BUY message and SIGUSR signal
                    snprintf(write_buffer, sizeof(write_buffer), "BUY %d %s %s %s", order_id, sell_message_sep[2], sell_message_sep[3], sell_message_sep[4]);
                    //Write to exchange 1 byte at a time until ;
                    size_t bytes_written;
                    while(1) {
                        bytes_written = write(fd_write, &write_buffer[i], 1);
                        if (bytes_written == -1)
                            perror("Failed to write from exchange");
                        if (write_buffer[i] == ';')
                            break;
                        i++;
                    }
                    i = 0;
                    order_id++;
                    //Clear buffers
                    memset(read_buffer, 0, sizeof(read_buffer));
                    memset(write_buffer, 0, sizeof(write_buffer));
                    //Send SIGUSR1 signal
                    kill(getppid(), SIGUSR1);

                    while (1) {

                        if (interrupt == 1) {
                            //Clear buffer;
                            memset(read_buffer, 0, sizeof(read_buffer));
                            //Read message and store in array
                            size_t bytes_read = read(fd_read, read_buffer, sizeof(read_buffer));
                            if (bytes_read == -1)
                                perror("Failed to read from exchange");
                            read_buffer[bytes_read] = '\0';
                            //Store message in array
                            token = strtok(read_buffer, " ");
                            while (token != NULL && i < 2) {
                                is_sell_msg[i] = token;
                                token = strtok(NULL, " ");
                                i++;
                            }
                            i = 0;
                            if (!(strcmp(is_sell_msg[0], "ACCEPTED"))) {
                                interrupt = 0;
                                break;
                            }

                            interrupt = 0;
                        }

                        sleep(1);

                        kill(getppid(), SIGUSR1); 

                    }

                }
            }

            //IF NOT SELL MESSAGE OR MARKET OPEN, DO NOTHING
            else {
                //Do nothing
                memset(read_buffer, 0, sizeof(read_buffer));
            }
        }
    }

    if (close(fd_read))
        perror("Failed to close fd_read");
    if (close(fd_write))
        perror("Failed to close fd_write");

}

void sigaction_handler(int sig, siginfo_t* sinf, void* ucontext){
    interrupt = 1;
}

