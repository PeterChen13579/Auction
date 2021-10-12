#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>


#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdbool.h> 

#define BUF_SIZE 128

#define MAX_AUCTIONS 5
#ifndef VERBOSE
#define VERBOSE 0
#endif

#define ADD 0
#define SHOW 1
#define BID 2
#define QUIT 3

/* Auction struct - this is different than the struct in the server program
 */
struct auction_data {
    int sock_fd;
    char item[BUF_SIZE];
    int current_bid;
};

/* Displays the command options available for the user.
 * The user will type these commands on stdin.
 */

void print_menu() {
    printf("The following operations are available:\n");
    printf("    show\n");
    printf("    add <server address> <port number>\n");
    printf("    bid <item index> <bid value>\n");
    printf("    quit\n");
}

/* Prompt the user for the next command 
 */
void print_prompt() {
    printf("Enter new command: ");
    fflush(stdout);
}


/* Unpack buf which contains the input entered by the user.
 * Return the command that is found as the first word in the line, or -1
 * for an invalid command.
 * If the command has arguments (add and bid), then copy these values to
 * arg1 and arg2.
 */
int parse_command(char *buf, int size, char *arg1, char *arg2) {
    int result = -1;
    char *ptr = NULL;
    if(strncmp(buf, "show", strlen("show")) == 0) {
        return SHOW;
    } else if(strncmp(buf, "quit", strlen("quit")) == 0) {
        return QUIT;
    } else if(strncmp(buf, "add", strlen("add")) == 0) {
        result = ADD;
    } else if(strncmp(buf, "bid", strlen("bid")) == 0) {
        result = BID;
    } 
    ptr = strtok(buf, " "); // first word in buf

    ptr = strtok(NULL, " "); // second word in buf
    if(ptr != NULL) {
        strncpy(arg1, ptr, BUF_SIZE);
    } else {
        return -1;
    }
    ptr = strtok(NULL, " "); // third word in buf
    if(ptr != NULL) {
        strncpy(arg2, ptr, BUF_SIZE);
        return result;
    } else {
        return -1;
    }
    return -1;
}

/* Connect to a server given a hostname and port number.
 * Return the socket for this server
 */
int add_server(char *hostname, int port) {
        // Create the socket FD.
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("client: socket");
        exit(1);
    }
    
    // Set the IP and port of the server to connect to.
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    struct addrinfo *ai;
    
    /* this call declares memory and populates ailist */
    if(getaddrinfo(hostname, NULL, NULL, &ai) != 0) {
        close(sock_fd);
        return -1;
    }
    /* we only make use of the first element in the list */
    server.sin_addr = ((struct sockaddr_in *) ai->ai_addr)->sin_addr;

    // free the memory that was allocated by getaddrinfo for this list
    freeaddrinfo(ai);

    // Connect to the server.
    if (connect(sock_fd, (struct sockaddr *)&server, sizeof(server)) == -1) {
        perror("client: connect");
        close(sock_fd);
        return -1;
    }
    if(VERBOSE){
        fprintf(stderr, "\nDebug: New server connected on socket %d.  Awaiting item\n", sock_fd);
    }
    return sock_fd;
}
/* ========================= Add helper functions below ========================
 * Please add helper functions below to make it easier for the TAs to find the 
 * work that you have done.  Helper functions that you need to complete are also
 * given below.
 */

/* Print to standard output information about the auction
 */
void print_auctions(struct auction_data *a, int size) {
    printf("Current Auctions:\n");

    /* TODO Print the auction data for each currently connected 
     * server.  Use the following format string:
     *     "(%d) %s bid = %d\n", index, item, current bid
     * The array may have some elements where the auction has closed and
     * should not be printed.
     */
    for (int i=0;i<size;i++){
        if (a[i].sock_fd >= 0){
            printf("(%d) %s bid = %d\n", i, a[i].item, a[i].current_bid);
            fflush(stdout);
        }
    }
}



/* Process the input that was sent from the auction server at a[index].
 * If it is the first message from the server, then copy the item name
 * to the item field.  (Note that an item cannot have a space character in it.)
 */
void update_auction(char *buf, int size, struct auction_data *a, int index) {
    // fprintf(stderr, "ERROR malformed bid: %s", buf);
    // printf("\nNew bid for %s [%d] is %d (%d seconds left)\n",           );

    //if at auction closed
    if (strncmp(buf, "Auction closed", 14) == 0) {
        if (strlen(a[index].item) > 0) {
            printf("[%d] %s (item: %s)\n", index, buf, a[index].item);
        }else{
            printf("[%d] %s\n", index, buf);
        }
    
        if (close(a[index].sock_fd) < 0) {
            perror("closed");
            exit(1);
        }
        //reset 
        a[index].item[0] = '\0';
        a[index].sock_fd = -1;
        a[index].current_bid = -1;
    }else{
    
        int time = 0;
        char *ptr = NULL;
        char *endptr;

        ptr = strtok(buf, " "); //item name
        if (ptr == NULL || strlen(ptr) < 1) {
            fprintf(stderr, "ERROR malformed bid: %s\n", buf);
        }
        // Check if item name is already set in auction data
        if (strlen(a[index].item) == 0) {
            strncpy(a[index].item, ptr, size);
            a[index].item[strlen(ptr)] = '\0';
        } 
        
        ptr = strtok(NULL, " "); // bid price
        if (ptr != NULL) {
            int temp = strtol(ptr, &endptr, 10);
            if (strlen(endptr) != 0) {
                fprintf(stderr, "ERROR malformed bid: %s\n", buf);
            }else{
                a[index].current_bid = temp;
            }
        }else{
            fprintf(stderr, "ERROR malformed bid: %s\n", buf);
        }

        ptr = strtok(NULL, " "); // time left
        if (ptr != NULL) {
            int temp = strtol(ptr, &endptr, 10);
            if (strlen(endptr) != 0) {
                fprintf(stderr, "ERROR malformed bid: %s\n", buf);
            }else{
                time = temp;
            }
        }else{
            fprintf(stderr, "ERROR malformed bid: %s\n", buf);
        }

        printf("\nNew bid for %s [%d] is %d (%d seconds left)\n", a[index].item, index, a[index].current_bid, time);
    }
}



void update_max_fd(struct auction_data *a, int size, int *max_fd) {
    int temp_max = STDIN_FILENO;

    for (int i = 0; i < size; i++) {
        if (a[i].sock_fd > temp_max) {
            temp_max = a[i].sock_fd;
        }
    }
    *max_fd = temp_max;
}

int main(void) {

    char name[BUF_SIZE];
    char arg1[BUF_SIZE], arg2[BUF_SIZE];
    char buf[BUF_SIZE];
    struct auction_data auctions[MAX_AUCTIONS];

    //set everything neccessary
    for (int i = 0; i < MAX_AUCTIONS; i++) {
        auctions[i].sock_fd = -1;
        auctions[i].item[0] = '\0';
        auctions[i].current_bid = -1;
    }

    int options;

    // Get the user to provide a name.
    printf("Please enter a username: ");
    fflush(stdout);
    int num_read = read(STDIN_FILENO, name, BUF_SIZE);
    name[strlen(name)-1] = '\0';

    if(num_read <= 0){
        fprintf(stderr, "ERROR: read from stdin failed\n");
        exit(1);
    }
    print_menu();

    fd_set all_fds;
    FD_ZERO(&all_fds);
    FD_SET(STDIN_FILENO, &all_fds);
    int max_fd = STDIN_FILENO;
    

    while(1) {
        print_prompt();

        memset(arg1, '\0', BUF_SIZE);
        memset(arg2, '\0', BUF_SIZE);
        fd_set listen_fds = all_fds;

        if (select(max_fd + 1, &listen_fds, NULL, NULL, NULL) < 0) {
            perror("select");
            exit(1);
        }

        //USER INPUT
        if (FD_ISSET(STDIN_FILENO, &listen_fds)) {
            num_read = read(STDIN_FILENO, buf, BUF_SIZE);
            if (num_read <= 0) {
                fprintf(stderr, "read stdin failed\n");
                exit(1);
            }

            options = parse_command(buf, BUF_SIZE, arg1, arg2);
            
            if (options == ADD) {
                char* endptr;
                int port = strtol(arg2, &endptr, 10);
                if (endptr[0] != '\n') {
                    fprintf(stderr, "Invalid port\n");
                }

                bool max = false;
                int available_index;
                int sock_fd = -1;
                for (int i = 0; i < MAX_AUCTIONS; i++) {
                    if (auctions[i].sock_fd == -1) {
                        available_index = i;
                        break;
                    }
                    if (i== MAX_AUCTIONS-1){
                        max = true;
                    }
                }

                if (max == false) {
                    sock_fd = add_server(arg1, port);
                }else {
                    fprintf(stderr, "Max number of ports reached. (%d)\n", MAX_AUCTIONS);
                }

                if (sock_fd >= 0) {
                    auctions[available_index].sock_fd = sock_fd;

                    // set fds
                    FD_SET(sock_fd, &all_fds);
                    if (sock_fd > max_fd) {
                        max_fd = sock_fd;
                    }

                    // Write user's name to server
                    if (write(sock_fd, name, strlen(name) +1) == -1) {
                        perror("write");
                        exit(1);
                    }
                }else if(sock_fd == -1) {
                    fprintf(stderr, "Failed add server\n");
                }
            }else if (options == SHOW) {
                print_auctions(auctions, MAX_AUCTIONS);
            }else if (options == BID) {
                // Check if index has open port and place bid if possible
                
                int index = strtol(arg1, NULL, 10);
                if (auctions[index].sock_fd == -1 || index < 0 || index >= MAX_AUCTIONS) {
                    fprintf(stderr, "Unavailable auction at %d\n", index);
                }else {
                    // Write bid string to corresponding auction socket
                    if (write(auctions[index].sock_fd, arg2, BUF_SIZE) == -1) {
                        perror("write");
                        close(auctions[index].sock_fd);
                        exit(1);
                    }
                }
            }else if (options == QUIT) {
                for (int i = 0; i < MAX_AUCTIONS; i++) {
                    if (auctions[i].sock_fd >= 0) {
                        if (close(auctions[i].sock_fd) == -1) {
                            perror("close");
                            exit(1);
                        }
                    }
                }
                exit(0);
            }else{
                fprintf(stderr, "Invalid option\n");
            }
            
            // Clear buffer for next read
            memset(buf, '\0', BUF_SIZE);
        }



        // SERVER INPUT
        for (int i = 0; i < MAX_AUCTIONS; i++) {
            if (FD_ISSET(auctions[i].sock_fd, &listen_fds)) {
                
                int curr_fd = auctions[i].sock_fd;
                int num_read = read(curr_fd, buf, BUF_SIZE);
                if (num_read == -1) {
                    fprintf(stderr, "socket [%d] failed\n", curr_fd);
                    exit(1);
                }
                buf[num_read] = '\0';
                update_auction(buf, BUF_SIZE, auctions, i);

                // check if auction closed
                if (auctions[i].sock_fd == -1) {
                    FD_CLR(curr_fd, &all_fds);
                    if (curr_fd == max_fd) {
                        update_max_fd(auctions, MAX_AUCTIONS, &max_fd);
                    }
                }
                memset(buf, '\0', BUF_SIZE);
            }
        }
    }
    return 0; // Shoud never get here
}
