1. Describe how your exchange works.

The exchange communicates with its children, the traders, through the use of signals. Specifically, it uses sigaction and a signal handler for SIGUSR1 signal and SIGCHILD signal. In the main while loop, the exchange passively polls through the use of pause. The exchange detects trader disconnections through the use of epoll. An epoll file descriptor is created which has access to the fifos with the flag EPOLLHUP which detects when the process associated to a file descriptor closes. Using this and epoll wait, in the main loop, the exchange checks the epoll events to detect if a trader has disconnected. 

For buy and sell orders, a priority queue data structure is used. The priority queue struct contains an array of orders (also a struct with relavent info about order) and the size of the priority queue. For buy orders, the priority queue is a max priority queue and for the sell orders, the priority queue is a min priority queue. This is so when matching the price matching is already done. Also, it turns out that, time matching is also accomplished with priority queues. A buy pq and a sell pq for each product is created. Also, a struct called traders is created which contains information on each traders' id, balance and quantities (of which balance and quantity are arrays of size number of products).


2. Describe your design decisions for the trader and how it's fault-tolerant.

The trader has a signal handler which detects if a SIGUSR1 signal has been recieved from the exchange. When this occurs, it reads from the fifo and if it's a MARKET SELL message, immediately sends a MARKET BUY message to the exchange. Following this, it also sends a SIGUSR1 signal. After this, it waits until it recieves a ACCEPTED message along with a SIGUSR1 signal. If it hasn't recieved this after a while, that means a race condition has most likely occured. Thus, the trader resends the SIGUSR1 signal and waits again for the accepted message.



3. Describe your tests and how to run them.

"# PEX" 
