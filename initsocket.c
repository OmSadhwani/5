#include "msocket.h"



sem_t *sem1, *sem2,*sem3;
struct MTPSocketInfo *SM;
struct SOCK_INFO *sock_info;
int glshmid,glshm;
int max(int a,int b){
    if(a>b)return a;
    return b;
}

void signal_handler(int signum) {
    printf("\nReceived Ctrl+C. Detaching shared memory and quitting.\n");
    int shmid = shmget(6, MAX_MTP_SOCKETS * sizeof(struct MTPSocketInfo), 0666);
    int shmid2 = shmget(5, sizeof(struct SOCK_INFO), 0666);
    // Detach the shared memory segments
    if (shmdt(SM) == -1) {
        perror("shmdt");
        exit(EXIT_FAILURE);
    }
    if (shmdt(sock_info) == -1) {
        perror("shmdt");
        exit(EXIT_FAILURE);
    }
    // Destroy the semaphores
    sem_destroy(sem1);
    sem_destroy(sem2);
    sem_destroy(sem3);
    printf("Shared memory and semaphores detached and destroyed successfully.\n");
    shmctl(shmid,IPC_RMID,0);
    shmctl(shmid2,IPC_RMID,0);
    exit(EXIT_SUCCESS);
}





void get_ip_port(const struct sockaddr_in *client_addr, char *ip_str, size_t ip_str_len, unsigned short *port) {
    // Get the IP address
    inet_ntop(AF_INET, &(client_addr->sin_addr), ip_str, ip_str_len);

    // Get the port
    *port = ntohs(client_addr->sin_port);
}

void send_ack(int i){
    printf("entered sendack\n");
    key_t key = 6;
    int shmid = shmget(key, MAX_MTP_SOCKETS * sizeof(struct MTPSocketInfo), 0666);
    if (shmid == -1) {
        perror("Error creating shared memory");
        exit(1);
    }

    // Attach shared memory segment
    SM = (struct MTPSocketInfo *)shmat(shmid, NULL, 0);
    if (SM == (struct MTPSocketInfo *)(-1)) {
        perror("shmat");
        exit(1);
    }
    

    int number= SM[i].receivers_window.next_sequence_number;
    for(int j=0;j<5;j++){
        if(SM[i].receivers_window.receive_messages[j].num==-1)break;
        else {
            number++;
            number%=16;
            if(number==0)number=1;
        }
    }
    
    struct message_send sen_msg;
    sen_msg.header.is_ack=1;
    sen_msg.header.number=number-1;
    
    sprintf(sen_msg.data, "%d", SM[i].receivers_window.window_size);

    struct sockaddr_in client_addr;
    int client_addrlen=sizeof(client_addr);
    inet_pton(AF_INET, SM[i].other_end_ip, &(client_addr.sin_addr));

    // Set the port
    client_addr.sin_port = htons(SM[i].other_end_port);

    // Set address family
    client_addr.sin_family = AF_INET;
    if (sendto(SM[i].udp_socket_id, &sen_msg, sizeof(sen_msg), 0, (struct sockaddr *)&client_addr, client_addrlen) == -1) {
        perror("sendto failed");
            // Handle error
    }

    printf("exited sendack \n");
    
    
}




void *receiver_thread(void *arg) {
    // Implementation of receiver thread behavior
    time_t last_ack_sent[MAX_MTP_SOCKETS]={0};
    
    key_t key = 6;
    int shmid = shmget(key, MAX_MTP_SOCKETS * sizeof(struct MTPSocketInfo), 0666);
    if (shmid == -1) {
        perror("Error creating shared memory");
        exit(1);
    }

    // Attach shared memory segment
    SM = (struct MTPSocketInfo *)shmat(shmid, NULL, 0);
    if (SM == (struct MTPSocketInfo *)(-1)) {
        perror("shmat");
        exit(1);
    }
    struct timeval timeout;
    timeout.tv_sec = 5;          // seconds
    timeout.tv_usec = 0;    // microseconds
    
    fd_set readfds;
    FD_ZERO(&readfds);
    int mx=0;
    
    sem_wait(sem3);
    printf("hhh\n");
    for (int i = 0; i < MAX_MTP_SOCKETS; i++) {
            // If socket descriptor is valid, add to read list
            if (SM[i].is_allocated && SM[i].udp_socket_id > 0) {
                // Add socket descriptor to set
                FD_SET(SM[i].udp_socket_id, &readfds);

                // Update max_sd if needed
                if (SM[i].udp_socket_id > mx)
                    mx = SM[i].udp_socket_id;
            }
        }
    sem_post(sem3);

        // Wait for activity on one of the sockets
    while(1){

        int activity = select(mx + 1, &readfds, NULL, NULL, &timeout);

        if(activity<0){
        
            printf("select error\n");
            continue;
        }
        if(activity>0){
            sem_wait(sem3);
            printf("entered R thread again\n");

            printf("detected\n");
            for (int i = 0; i < MAX_MTP_SOCKETS; i++) {
                if(SM[i].is_allocated==0 || SM[i].udp_socket_id<0)continue;
                int sd = SM[i].udp_socket_id;

                if (FD_ISSET(sd, &readfds)) {
                    // Handle incoming message
                    printf("%d index is set\n",i);
                    struct message_send msg;
                    struct sockaddr_in client_addr;
                    int client_addrlen=sizeof(client_addr);
                    recvfrom(sd, &msg, sizeof(msg), 0, (struct sockaddr *)&client_addr, &client_addrlen);
                    if(dropMessage(probability)){
                        printf("Message Dropped\n");
                        continue;
                    }
                    printf("message received\n");
                    char ip_address[50] ;

                 
                    unsigned short port_number;

                    get_ip_port(&client_addr, ip_address, sizeof(ip_address), &port_number);

                    if(port_number!=SM[i].other_end_port || strcmp(ip_address,SM[i].other_end_ip)!=0){
                        continue;
                    }

                    
                    if (msg.header.is_ack == 0) {
                        printf("the message is of type data\n");
    
                        
                        if(SM[i].receivers_window.nospace==1){ //buffer full
                            printf("the buffer is full\n");
                            continue;
                        }
                        int expected_number= SM[i].receivers_window.next_sequence_number;
                        int window_size=SM[i].receivers_window.window_size;
                        int max_allowed= (expected_number+5)%16;
                        if(max_allowed==0)max_allowed=1;
                   
                        int index=0;
                        int cur=expected_number;
                        int flag=0;
                        while(cur!=max_allowed){
                            
                            if(cur==msg.header.number){
                                flag=1;
                                break;
                            }
                            index++;
                            cur++;
                            
                            cur%=16;
                            if(cur==0)cur=1;

                        }


                        if(flag==0){ 
                            printf("%d sequence number not in window\n",msg.header.number);
                            send_ack(i);
                            last_ack_sent[i]=time(NULL);
                            continue;
                        }
                        else if(flag==1 && SM[i].receivers_window.receive_messages[index].num!=-1){
                            printf("duplicated message\n");
                            
                            send_ack(i);
                            last_ack_sent[i]=time(NULL);
                            printf("the ack is sent for duplicated\n");
                            continue;
                        }
                        struct message_receive m;
                        memcpy(m.data,msg.data,1024);
                        m.num=msg.header.number;
                        SM[i].receivers_window.receive_messages[index]=m;
                        SM[i].receivers_window.receive_messages[index].num=m.num;
                        int fr=5;
                        for(int j=0;j<5;j++){
                            if(SM[i].receivers_window.receive_messages[j].num==-1){
                                fr=j;
                                break;
                            }
                        }
                        SM[i].receivers_window.window_size=5-fr;
                        // SM[i].receivers_window.window_size--;
                        if(SM[i].receivers_window.window_size==0){
                            SM[i].receivers_window.nospace=1;
                        }
                        
                        printf("sending ack\n");
                        send_ack(i);
                        printf("the ack is sent\n");
                        last_ack_sent[i]=time(NULL);



                    } else {
                        printf("the message received is ack for sequence number %d \n",msg.header.number);
                        printf("The sequence numbers present in the buffer are \n");
                        for(int k=0;k<5;k++){
                            printf("%d\n",SM[i].senders_window.send_messages[k].header.number);
                        }

                        int index=-1;
                        int window_sz= atoi(msg.data);
                        
                        for(int j=0;j<SM[i].senders_window.window_size;j++){
                            if(SM[i].senders_window.send_messages[j].header.number==msg.header.number){
                                index=j;
                                break;
                            }
                        }
                        if(index==-1){
                            SM[i].senders_window.window_size=window_sz;
                            continue;
                        }
                        SM[i].senders_window.index_to_write-=(index+1);
                        index++;
                        printf("the messages in the send buffer of %d are\n",i);
                        for(int j=0;j<10;j++){
                            printf("j: \n");
                            printf("%s \n",SM[i].senders_window.send_messages[j]);
                        }
                        for(int j=0;j<index;j++){
                            SM[i].senders_window.send_messages[j].header.number=-1;
                            SM[i].senders_window.time[j]=NULL;
                            memset(SM[i].senders_window.send_messages[j].data,'\0',sizeof(SM[i].senders_window.send_messages[j].data));
                        }
                        int j=0;
                        while(index<10){
                            memset(SM[i].senders_window.send_messages[j].data,'\0',sizeof(SM[i].senders_window.send_messages[j].data));
                            memcpy(SM[i].senders_window.send_messages[j].data,SM[i].senders_window.send_messages[index].data,1024);
                            memset(SM[i].senders_window.send_messages[index].data,'\0',sizeof(SM[i].senders_window.send_messages[index].data));
                            SM[i].senders_window.send_messages[j].header.is_ack=SM[i].senders_window.send_messages[index].header.is_ack;
                            SM[i].senders_window.send_messages[j].header.number=SM[i].senders_window.send_messages[index].header.number;
                            SM[i].senders_window.time[j]=SM[i].senders_window.time[index];
                            SM[i].senders_window.send_messages[index].header.number=-1;
                            SM[i].senders_window.time[index]=NULL;

                            index++;
                            j++;
                        }
                        printf("The sequence numbers present in the buffer are \n");
                        for(int k=0;k<5;k++){
                            printf("%d\n",SM[i].senders_window.send_messages[k].header.number);
                        }
                        printf("the messages in the send buffer after of %d are\n",i);
                        for(int j=0;j<10;j++){
                            printf("j: \n");
                            printf("%s \n",SM[i].senders_window.send_messages[j]);
                        }
                      
                    }
                }
            }
            printf("exited from R thread \n");
            sem_post(sem3);
        }
        else if (activity==0){
            mx=0;
            FD_ZERO(&readfds);
            timeout.tv_sec=T;
            timeout.tv_usec=0;
            sem_wait(sem3);
            for(int i=0;i<25;i++){
                if(SM[i].is_allocated && SM[i].udp_socket_id>0){
                    FD_SET(SM[i].udp_socket_id,&readfds);
                    mx=max(mx,SM[i].udp_socket_id);
                    printf("%d added to the set\n",SM[i].udp_socket_id);
                    if((SM[i].receivers_window.nospace==1 && SM[i].receivers_window.window_size>0)){
                        SM[i].receivers_window.nospace=0;
                        
                        send_ack(i);
                        last_ack_sent[i]=time(NULL);

                    }
                    if(difftime(time(NULL),last_ack_sent[i])>30){
                        send_ack(i);
                        last_ack_sent[i]=time(NULL);
                    }
                }  
                else if(SM[i].is_allocated==0){
                    SM[i].receivers_window.nospace=0;
                } 
            }
            printf("exited R timeout thread\n");
            sem_post(sem3);
        }



    }
    shmdt(SM);
    return NULL;
}

void *sender_thread(void *arg) {
    key_t key = 6;
    int shmid = shmget(key, MAX_MTP_SOCKETS * sizeof(struct MTPSocketInfo), 0666);
    if (shmid == -1) {
        perror("Error creating shared memory");
        exit(1);
    }

    // Attach shared memory segment
    SM = (struct MTPSocketInfo *)shmat(shmid, NULL, 0);
    if (SM == (struct MTPSocketInfo *)(-1)) {
        perror("shmat");
        exit(1);
    }
    
    while (1) {
        // Sleep for some time less than T/2
        sleep(T / 2);

        sem_wait(sem3);

        // Iterate over MTP sockets
        for (int i = 0; i < MAX_MTP_SOCKETS; i++) {
            // Check if MTP socket is active
            if (SM[i].is_allocated && SM[i].udp_socket_id>=0) {
                // Check message timeout for this MTP socket
                int flag=0;
                // printf("helloooo\n");
                if(SM[i].senders_window.send_messages[0].header.number==-1)continue;
                // printf("%d here\n",i);
                if(SM[i].senders_window.time[0]!=NULL && difftime(time(NULL),SM[i].senders_window.time[0])>=T){
                    for(int j=0;j<SM[i].senders_window.window_size;j++){
                        printf("Time out occured on index %d\n",i);
                        printf("The sequence numbers present in the buffer are \n");
                        for(int k=0;k<5;k++){
                            printf("%d\n",SM[i].senders_window.send_messages[k].header.number);
                        }
                        printf("i=%d, j= %d\n",i,j);

                        if(SM[i].senders_window.send_messages[j].header.number==-1)break;
                        struct sockaddr_in client_addr;
                        int client_addrlen=sizeof(client_addr);
                        inet_pton(AF_INET, SM[i].other_end_ip, &(client_addr.sin_addr));

                        // Set the port
                        client_addr.sin_port = htons(SM[i].other_end_port);

                        // Set address family
                        client_addr.sin_family = AF_INET;
                        if (sendto(SM[i].udp_socket_id, &SM[i].senders_window.send_messages[j], sizeof(SM[i].senders_window.send_messages[j]), 0, (struct sockaddr *)&client_addr, client_addrlen) == -1) {
                            perror("sendto failed");
                                // Handle error
                        }
                        printf("message sent to %d\n",i);
                        SM[i].senders_window.time[j]=time(NULL);
                        
                    }
                }
                for(int j=0;j<SM[i].senders_window.window_size;j++){
                    // printf("jsecond= %d\n",j);
                    if(SM[i].senders_window.send_messages[j].header.number==-1)break;
                    if(SM[i].senders_window.send_messages[j].header.number!=-1 && SM[i].senders_window.time[j]==NULL){
                        struct sockaddr_in client_addr;
                        int client_addrlen=sizeof(client_addr);
                        inet_pton(AF_INET, SM[i].other_end_ip, &(client_addr.sin_addr));

                        // Set the port
                        client_addr.sin_port = htons(SM[i].other_end_port);

                        // Set address family
                        client_addr.sin_family = AF_INET;
                        printf("%d \n",SM[i].udp_socket_id);
                        if (sendto(SM[i].udp_socket_id, &SM[i].senders_window.send_messages[j], sizeof(SM[i].senders_window.send_messages[j]), 0, (struct sockaddr *)&client_addr, client_addrlen) == -1) {
                            perror("sendto failed");
                                // Handle error
                        }
                        printf("message sent to %d for the first time\n",i);
                        SM[i].senders_window.time[j]=time(NULL);
                    }
                }

                
            }
        }
        sem_post(sem3);
    }
    shmdt(SM);

    return NULL;
}

void *garbage_collector(void *arg) {
    key_t key = 6;
    int shmid = shmget(key, MAX_MTP_SOCKETS * sizeof(struct MTPSocketInfo), 0666);
    if (shmid == -1) {
        perror("Error creating shared memory");
        return -1;
    }

    // Attach shared memory segment
    SM = (struct MTPSocketInfo *)shmat(shmid, NULL, 0);
    if (SM == (struct MTPSocketInfo *)(-1)) {
        perror("shmat");
        exit(1);
    }
    sem_t* sem3= sem_open(ne,0);
    sem_wait(sem3);
    for(int i=0;i<MAX_MTP_SOCKETS;i++){
        if(SM[i].is_allocated==-1){
            SM[i].is_allocated=0;
            close(SM[i].udp_socket_id);
            SM[i].is_allocated = 0; // Mark all sockets as free
            SM[i].senders_window.next_sequence_number=1;
            SM[i].receivers_window.next_sequence_number=1;
            SM[i].receivers_window.window_size=5;
            SM[i].senders_window.window_size=5;
            SM[i].receivers_window.nospace=0;
            SM[i].senders_window.index_to_write=0;
            for(int j=0;j<10;j++){
                if(j<5){
                    SM[i].receivers_window.receive_messages[j].num=-1;
                }
                SM[i].senders_window.send_messages[j].header.number=-1;
                SM[i].senders_window.time[j]=NULL;
                SM[i].senders_window.is_sent[j]=0;
            }
           
        }
    }
    sem_post(sem3);
    shmdt(SM);
    return NULL;
}

int main() {
    // Create shared memory
    signal(SIGINT,signal_handler);
    key_t key = 6;
    int shmid = shmget(key, MAX_MTP_SOCKETS * sizeof(struct MTPSocketInfo), 0666 | IPC_CREAT);
    if (shmid == -1) {
        perror("Error creating shared memory");
        return -1;
    }

    // Attach shared memory segment
    SM = (struct MTPSocketInfo *)shmat(shmid, NULL, 0);
    if (SM == (struct MTPSocketInfo *)(-1)) {
        perror("shmat");
        exit(1);
    }
    sem1 = sem_open("/semaphore1", O_CREAT , 0666, 0);

        printf("hii\n");
    if (sem1 == SEM_FAILED) {
        perror("sem_open\n");
        return 1;
    }

    sem2 = sem_open("/semaphore2", O_CREAT, 0666, 0);
    if (sem2 == SEM_FAILED) {
        perror("sem_open\n");
        return 1;
    }
    sem3=sem_open(ne,O_CREAT,0666,1);
    if(sem3==SEM_FAILED){
        perror("sem open\n");
        return 1;
    }
    
    for (int i = 0; i < MAX_MTP_SOCKETS; i++) {
        SM[i].is_allocated = 0; // Mark all sockets as free
        SM[i].senders_window.next_sequence_number=1;
        SM[i].receivers_window.next_sequence_number=1;
        SM[i].receivers_window.window_size=5;
        SM[i].senders_window.window_size=5;
        SM[i].receivers_window.nospace=0;
        SM[i].senders_window.index_to_write=0;
        for(int j=0;j<10;j++){
            if(j<5){
                SM[i].receivers_window.receive_messages[j].num=-1;
            }
            SM[i].senders_window.send_messages[j].header.number=-1;
            SM[i].senders_window.time[j]=NULL;
            SM[i].senders_window.is_sent[j]=0;
        }
        // Initialize other fields as necessary
    }

    

    

    key = 5;
    shmid = shmget(key, sizeof(struct SOCK_INFO), 0666 | IPC_CREAT);
    if (shmid == -1) {
        perror("Error creating shared memory for SOCK_INFO\n");
        return -1;
    }

    // Attach shared memory segment
    printf("hello\n");
    sock_info = (struct SOCK_INFO *)shmat(shmid, NULL, 0);
    if (sock_info == (struct SOCK_INFO *)(-1)) {
        perror("shmat\n");
        exit(1);
    }

    // Initialize SOCK_INFO
    sock_info->sock_id = 0;
    strcpy(sock_info->IP, "");
    sock_info->port = 0;
    sock_info->err_ = 0;


    printf("hi\n");
    // sem1 = sem_open("/semaphore1", O_CREAT, 0666, 0);
    


    pthread_t receiver_tid;
    if (pthread_create(&receiver_tid, NULL, receiver_thread, NULL) != 0) {
        perror("Error creating receiver thread");
        return -1;
    }

    // Start sender thread
    pthread_t sender_tid;
    if (pthread_create(&sender_tid, NULL, sender_thread, NULL) != 0) {
        perror("Error creating sender thread");
        return -1;
    }
    // Start garbage collector thread
    // pthread_t garbage_collector_tid;
    // if (pthread_create(&garbage_collector_tid, NULL, garbage_collector, NULL) != 0) {
    //     perror("Error creating garbage collector thread");
    //     return -1;
    // }

    // Main thread waits for m_socket() or m_bind() call

    while (1) {
        sem_wait(sem1); // Wait for semaphore signal

        // Check SOCK_INFO to determine action
        if (sock_info->sock_id == 0 && strcmp(sock_info->IP, "") == 0 && sock_info->port == 0) {
            // m_socket() call
            sock_info->sock_id = socket(AF_INET, SOCK_DGRAM, 0);
            printf("%d hello\n",sock_info->sock_id);
            if (sock_info->sock_id == -1) {
                sock_info->err_ = errno;
            }
        } else {
            // m_bind() call
            struct sockaddr_in bind_addr;
            memset(&bind_addr, 0, sizeof(bind_addr));
            bind_addr.sin_family = AF_INET;
            bind_addr.sin_port = htons(sock_info->port);
            bind_addr.sin_addr.s_addr = inet_addr(sock_info->IP);
            printf("here\n");
            if (bind(sock_info->sock_id, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) == -1) {
                sock_info->err_ = errno;
                sock_info->sock_id = -1; // Reset sock_id if bind fails
            }
            printf("hello5\n");
        }

        sem_post(sem2); // Signal completion
    }

    shmdt(SM);
    shmdt(sock_info);

    return 0;
}


