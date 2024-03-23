#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/select.h>
#include <time.h>
#include <sys/stat.h>
#include <signal.h>

#define probability 0.05
#define MAX_MTP_SOCKETS 25
#define SOCK_MTP 3 // Custom socket type for MTP
#define ENOTBOUND 177
// Define a time period using timespec structure
#include <time.h>
#define T 5
// Define a constant time period of 2 seconds
// const struct timespec T = { .tv_sec = 2, .tv_nsec = 0 };
#define ne "/semmm9"


struct message_header_send{
    int is_ack; // 1 for ack 0 for data
    int number; // sequence number
    
};


struct message_send{
    char data[1024];
    struct message_header_send header;
};

struct message_receive{
    char data[1024];
    int num;
};



struct swnd{
        int window_size; // Window size
        time_t time[10];
        int is_sent[10];
        struct message_send send_messages[10]; // Sequence numbers of messages sent but not acknowledged
        int next_sequence_number;
        int index_to_write;
    }; // Sender window

struct rwnd{
        int window_size; // Window size
        struct message_receive receive_messages[5]; // Sequence numbers of messages received but not acknowledged
        int index_to_receive;
        int next_sequence_number;
        int nospace;
    };

struct MTPSocketInfo {
    int is_allocated; // Flag indicating if the MTP socket is free or allocated
    int process_id; // Process ID for the process that created the MTP socket
    int udp_socket_id; // UDP socket ID
    char other_end_ip[16]; // IP address of the other end of the MTP socket
    unsigned short other_end_port; // Port of the other end of the MTP socket
    
    
    struct swnd senders_window;
    struct rwnd receivers_window;
  
};
struct SOCK_INFO {
    int sock_id;
    char IP[50];
    unsigned short port;
    int err_;
};

int m_socket(int domain, int type, int protocol);
int m_bind(int sockfd,char source_ip[50],unsigned short source_port,char dest_ip[50],unsigned short dest_port);
int m_sendto(int sockfd, const void* data, int len, int flags, const struct sockaddr *servaddr, socklen_t addrlen );
int m_recvfrom(int sockfd, void *buffer, int len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
int dropMessage(float p);
int m_close(int sockid);