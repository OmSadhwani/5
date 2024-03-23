
#define MAX_MTP_SOCKETS 25



// Assume MTPSocketInfo struct is defined in mtp_socket.h
#include "msocket.h"

int m_socket(int domain, int type, int protocol) {
    printf("Reached here\n");
    // Check if the requested socket type is SOCK_MTP
    if (type != SOCK_MTP) {
        errno = EINVAL; 
        return -1;
    }
    key_t key = 6;
    int shmid = shmget(key, MAX_MTP_SOCKETS * sizeof(struct MTPSocketInfo), 0666 );
    if (shmid == -1) {
       
        return -1;
    }

    // Attach shared memory segment
    struct MTPSocketInfo *SM = (struct MTPSocketInfo *)shmat(shmid, NULL, 0);
    if (SM == (struct MTPSocketInfo *)(-1)) {
        return -1;
    }

    key = 5;
    shmid = shmget(key, sizeof(struct SOCK_INFO), 0666 );
    if (shmid == -1) {
       
        return -1;
    }
    struct SOCK_INFO *sock_info;
    // Attach shared memory segment
    sock_info = (struct SOCK_INFO *)shmat(shmid, NULL, 0);
    if (sock_info == (struct SOCK_INFO *)(-1)) {
        shmdt(SM);
       
        return -1;
    }
    sem_t* sem3= sem_open(ne,0);
    sem_wait(sem3);
  
    printf("hello\n");
    // Find a free entry in shared memory
    int free_entry_index = -1;
    for (int i = 0; i < MAX_MTP_SOCKETS; i++) {
        if (SM[i].is_allocated==0) {
            free_entry_index = i;
            break;
        }
    }
    if (free_entry_index == -1) {
        errno = ENOBUFS; // No buffer space available
        shmdt(SM);
        shmdt(sock_info);
        return -1;
    }
    

    sem_t* sem1=sem_open("/semaphore1",0);
    sem_t* sem2= sem_open("/semaphore2",0);
    printf("hello1\n");
     // Signal on Sem1

    sem_post(sem1);

    // Wait on Sem2
    sem_wait(sem2);

    if(sock_info->sock_id==-1){
        errno=sock_info->err_;
        sock_info->sock_id = 0;
        strcpy(sock_info->IP, "");
        sock_info->port = 0;
        sock_info->err_ = 0;
        shmdt(SM);
        shmdt(sock_info);
        return -1;
    }

    
    // Initialize the shared memory entry for the new MTP socket
    SM[free_entry_index].is_allocated = 1; // Mark the socket as allocated
    SM[free_entry_index].udp_socket_id = sock_info->sock_id;
    SM[free_entry_index].process_id=getpid();
    printf("idx:%d, socket:%d\n", free_entry_index, SM[free_entry_index].udp_socket_id);
    sock_info->sock_id = 0;
    strcpy(sock_info->IP, "");
    sock_info->port = 0;
    sock_info->err_ = 0;


    
    // Initialize other fields as needed
    sem_post(sem3);
    if(shmdt(SM)==-1){
        return -1;
    }
    if(shmdt(sock_info)==-1){
        return -1;
    }
    sem_close(sem1);
    sem_close(sem2);
    sem_close(sem3);

    printf("mtp socket creation succesfull\n");
    return free_entry_index;
}
// int err = m_bind(sockfd, htonl(INADDR_ANY), htons(10010), inet_addr("127.0.0.1"), htons(10020));
int m_bind(int sockfd, char source_ip[50], unsigned short source_port, char dest_ip[50], unsigned short dest_port) {

    

    key_t key = 6;
    int shmid = shmget(key, MAX_MTP_SOCKETS * sizeof(struct MTPSocketInfo), 0666 );
    if (shmid == -1) {
        
        return -1;
    }

    // Attach shared memory segment
    struct MTPSocketInfo *SM = (struct MTPSocketInfo *)shmat(shmid, NULL, 0);
    if (SM == (struct MTPSocketInfo *)(-1)) {
     
        exit(1);
    }

    
    key = 5;
    shmid = shmget(key, sizeof(struct SOCK_INFO), 0666 );
    if (shmid == -1) {
        shmdt(SM);
        return -1;
    }
    struct SOCK_INFO *sock_info;
    // Attach shared memory segment

    sock_info = (struct SOCK_INFO *)shmat(shmid, NULL, 0);
    if (sock_info == (struct SOCK_INFO *)(-1)) {
        shmdt(SM);
        exit(1);
    }
    sem_t* sem3= sem_open(ne,0);
    sem_wait(sem3);
    // Check if the socket is already bound
    if (SM[sockfd].udp_socket_id == -1) {
        errno = EINVAL; // Invalid argument
        shmdt(SM);
        shmdt(sock_info);
        return -1;
    }
    if (sockfd < 0 || sockfd >= MAX_MTP_SOCKETS || SM[sockfd].is_allocated==0) {
        errno = EBADF; // Bad file descriptor
        shmdt(SM);
        shmdt(sock_info);
        return -1;
    } // Update SM with destination IP and destination port

    sock_info->port=source_port;
    sock_info->sock_id=SM[sockfd].udp_socket_id;
    strcpy(sock_info->IP,source_ip);
    printf("hellop\n");
    printf("%d\n",sock_info->sock_id);
    

    sem_t* sem1=sem_open("/semaphore1",0);
    sem_t* sem2= sem_open("/semaphore2",0);

     // Signal on Sem1
    sem_post(sem1);

    // Wait on Sem2
    sem_wait(sem2);
    printf("hello\n");

    

    if(sock_info->sock_id==-1){
        errno=sock_info->err_;
        sock_info->sock_id = 0;
        strcpy(sock_info->IP, "");
        sock_info->port = 0;
        sock_info->err_ = 0;
        shmdt(SM);
        shmdt(sock_info);
        return -1;
    }

  


    strcpy(SM[sockfd].other_end_ip,dest_ip);
    SM[sockfd].other_end_port = dest_port;


    sock_info->sock_id = 0;
    strcpy(sock_info->IP, "");
    sock_info->port = 0;
    sock_info->err_ = 0;

    sem_post(sem3);
    // Update SM with destination IP and destination port
    if(shmdt(SM)==-1){

        return -1;
    }
    if(shmdt(sock_info)==-1){
        return -1;
    }
    sem_close(sem1);
    sem_close(sem2);
    sem_close(sem3);



    return 0;
}




int m_sendto(int sockfd, const void* data, int len, int flags, const struct sockaddr *servaddr, socklen_t addrlen ) {

    
    key_t key = 6;
    int shmid = shmget(key, MAX_MTP_SOCKETS * sizeof(struct MTPSocketInfo), 0666 );
    if (shmid == -1) {
        perror("shmget");
        return -1;
    }
    
    struct MTPSocketInfo *SM = (struct MTPSocketInfo *)shmat(shmid, NULL, 0);
    if (SM == (struct MTPSocketInfo *)(-1)) {
        perror("shmat");
        exit(1);
    }

    if (sockfd < 0 || sockfd >= MAX_MTP_SOCKETS || SM[sockfd].is_allocated==0) {
        errno = EBADF; 
        shmdt(SM);
        return -1;
    }

    sem_t* sem3= sem_open(ne,0);
    
    if(sem_wait(sem3)==-1){
        shmdt(SM);
        return -1;
    }
    
    struct sockaddr_in *ipv4 = (struct sockaddr_in *)servaddr;

    char ip[50];


    inet_ntop(AF_INET, &(ipv4->sin_addr), ip, INET_ADDRSTRLEN);
    unsigned short port = ntohs(ipv4->sin_port);



  
    int flag=0;
    if (SM[sockfd].is_allocated && strcmp(SM[sockfd].other_end_ip, ip) == 0 && SM[sockfd].other_end_port == port) {
        flag =1;        
    }
    
    

    if (flag == 0) {
        
        errno = ENOTBOUND;
        sem_post(sem3);
        shmdt(SM);
        return -1;
    }


   
    if (SM[sockfd].senders_window.index_to_write == 10) {
        
        errno = ENOBUFS;
        sem_post(sem3);
        shmdt(SM);
        return -1;

    }


    int idx=SM[sockfd].senders_window.index_to_write;
    SM[sockfd].senders_window.send_messages[idx].header.is_ack=0;
    
    

    
    
    

    memset(&(SM[sockfd].senders_window.send_messages[idx].data), 0, sizeof(SM[sockfd].senders_window.send_messages[idx].data));


    SM[sockfd].senders_window.send_messages[SM[sockfd].senders_window.index_to_write].header.number=SM[sockfd].senders_window.next_sequence_number ;

    memcpy(SM[sockfd].senders_window.send_messages[SM[sockfd].senders_window.index_to_write].data,data,len); 
    SM[sockfd].senders_window.next_sequence_number=(SM[sockfd].senders_window.next_sequence_number+1)%16;
    if(SM[sockfd].senders_window.next_sequence_number==0){
        SM[sockfd].senders_window.next_sequence_number=1;
    }


    SM[sockfd].senders_window.is_sent[SM[sockfd].senders_window.index_to_write]=0;
    SM[sockfd].senders_window.index_to_write++;

    if(sem_post(sem3)==-1){
        shmdt(SM);
        return -1;
    }
    if(shmdt(SM)==-1){
        
        return -1;
    }
    
    



    return len;
}


int m_recvfrom(int sockfd, void *buffer, int len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) {
    
    key_t key = 6;
    int shmid = shmget(key, MAX_MTP_SOCKETS * sizeof(struct MTPSocketInfo), 0666 );
    if (shmid == -1) {
        
        return -1;
    }

    // Attach shared memory segment
    struct MTPSocketInfo *SM = (struct MTPSocketInfo *)shmat(shmid, NULL, 0);
    if (SM == (struct MTPSocketInfo *)(-1)) {
     
        exit(1);
    }

    if (sockfd < 0 || sockfd >= MAX_MTP_SOCKETS || SM[sockfd].is_allocated==0) {
        errno = EBADF; // Bad file descriptor
        shmdt(SM);
        return -1;
    }

    sem_t* sem3= sem_open(ne,0);
    // printf("hello in receive \n");
    if(sem_wait(sem3)==-1){
        shmdt(SM);
        return -1;
    }

    // Check if there are any messages received in the buffer
    
    struct MTPSocketInfo soc = SM[sockfd];
    if (soc.receivers_window.receive_messages[0].num==-1) {
        errno = ENOMSG; // No message available
        shmdt(SM);
        sem_post(sem3);
        return -1;
    }
    // printf("hello here in recv\n");

    // Extract the first message from the receiver-side message buffer
    struct message_receive recv_msg = soc.receivers_window.receive_messages[0];

    // Copy the message data to the user-provided buffer
    memcpy(buffer, recv_msg.data, len);
    SM[sockfd].receivers_window.next_sequence_number=(recv_msg.num+1)%16;
    if(SM[sockfd].receivers_window.next_sequence_number==0){
        SM[sockfd].receivers_window.next_sequence_number=1;
    }
   // Port of the other end of the MTP socket
    // printf("idhar\n");
    // Set the source address if provided
    if (src_addr != NULL) {
        // Convert IP address and port to sockaddr structure
        struct sockaddr_in *src_in = (struct sockaddr_in *)src_addr;
        src_in->sin_family = AF_INET;
        src_in->sin_port = htons(SM[sockfd].other_end_port);
        // printf("h\n");
        inet_pton(AF_INET, SM[sockfd].other_end_ip, &(src_in->sin_addr));
        // printf("h\n");
        // *addrlen = sizeof(struct sockaddr_in);
    }
    // printf("here in recv\n");
    for(int j=0;j<4;j++){
        if(SM[sockfd].receivers_window.receive_messages[j+1].num==-1){
            SM[sockfd].receivers_window.receive_messages[j].num=-1;
            memset(SM[sockfd].receivers_window.receive_messages[j].data,'\0',sizeof((SM[sockfd].receivers_window.receive_messages[j].data)));
            
        }
        else{
            memset(SM[sockfd].receivers_window.receive_messages[j].data,'\0',sizeof((SM[sockfd].receivers_window.receive_messages[j].data)));
            memcpy(SM[sockfd].receivers_window.receive_messages[j].data,SM[sockfd].receivers_window.receive_messages[j+1].data,1024);
            SM[sockfd].receivers_window.receive_messages[j].num=SM[sockfd].receivers_window.receive_messages[j+1].num;
            
        }
    }
    // printf("here in recv\n");
    SM[sockfd].receivers_window.receive_messages[4].num=-1;
    memset(SM[sockfd].receivers_window.receive_messages[4].data,'\0',sizeof((SM[sockfd].receivers_window.receive_messages[4].data)));
    SM[sockfd].receivers_window.window_size++;

    if(sem_post(sem3)==-1){
        shmdt(SM);
        return -1;
    }

    printf("receive successful\n");
 
    shmdt(SM);
    return len; // Return the length of the received message
}

int dropMessage(float p){
    
    float random = (float)((float)(rand()) / (float)RAND_MAX);
    
  
    if (random < p) {
        return 1;
    } else {
        return 0;
    }
}
int m_close(int sockfd){
    key_t key = 6;
    int shmid = shmget(key, MAX_MTP_SOCKETS * sizeof(struct MTPSocketInfo), 0666 );
    if (shmid == -1) {
        
        return -1;
    }
    // Attach shared memory segment
    struct MTPSocketInfo *SM = (struct MTPSocketInfo *)shmat(shmid, NULL, 0);
    if (SM == (struct MTPSocketInfo *)(-1)) {
     
        exit(1);
    }

    if (sockfd < 0 || sockfd >= MAX_MTP_SOCKETS || SM[sockfd].is_allocated!=1) {
        errno = EBADF; // Bad file descriptor
        shmdt(SM);
        return -1;
    }
    sem_t* sem3= sem_open(ne,0);
    sem_wait(sem3);
    SM[sockfd].is_allocated=-1;
    sem_post(sem3);
    return 0;
}
