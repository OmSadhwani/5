#include "msocket.h"


sem_t *sem1, *sem2,*sem3;



struct SOCK_INFO *sock_info;


void *receiver_thread(void *arg) {
    // Implementation of receiver thread behavior
    key_t key = 6;
    int shmid = shmget(key, MAX_MTP_SOCKETS * sizeof(struct MTPSocketInfo), 0666 | IPC_CREAT);
    if (shmid == -1) {
        perror("Error creating shared memory");
        return -1;
    }

    // Attach shared memory segment
    struct MTPSocketInfo *SM = (struct MTPSocketInfo *)shmat(shmid, NULL, 0);
    if (SM == (struct MTPSocketInfo *)(-1)) {
        perror("shmat");
        exit(1);
    }
    
    fd_set readfds;
    FD_ZERO(&readfds);
    int mx=0;
    sem_t* sem3= sem_open("/semaphore3",0);
    sem_wait(sem3);
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

        int activity = select(mx + 1, &readfds, NULL, NULL, NULL);

        if(activity<0){
            perror(select);
            continue;
        }
        if(activity>0){
            sem_wait(sem3);
            for (int i = 0; i < MAX_MTP_SOCKETS; i++) {
                if(SM[i].is_allocated==0 || SM[i].udp_socket_id<0)continue;
                int sd = SM[i].udp_socket_id;

                if (FD_ISSET(sd, &readfds)) {
                    // Handle incoming message
                    struct message_send msg;
                    struct sockaddr_in client_addr;
                    int client_addrlen=sizeof(client_addr);

                    recvfrom(sd, &msg, sizeof(msg), 0, (struct sockaddr *)&client_addr, &client_addrlen);

                    // Process received message
                    if (msg.header.is_ack == 0) {
                        
                        // Data message received
                        // Store in receiver-side message buffer
                        // Send ACK message to the sender

                        int idx= SM[i].receivers_window.index_to_receive;
                        if(SM[i].receivers_window.nospace==1){ //buffer full
                            continue;
                        }
                        int expected_number= SM[i].receivers_window.next_sequence_number;
                        int window_size=SM[i].receivers_window.window_size;
                        int max_allowed= (expected_number+window_size-1+16)%16;
                        if(max_allowed==0)max_allowed=1;
                        int arr[16];
                        int index=0;
                        int cur=expected_number;
                        int flag=0;
                        while(cur!=max_allowed){
                            arr[index++]=cur;
                            if(cur==msg.header.number){
                                flag=1;
                                break;
                            }
                            cur++;
                            
                            cur%=16;
                            if(cur==0)cur=1;

                        }


                        if(flag==0){ // sequence number not in window

                        }
                        struct message_receive m;
                        strcpy(m.data,msg.data);
                        m.num=msg.header.number;
                        SM[i].receivers_window.receive_messages[idx]=m;
                        SM[i].receivers_window.index_to_receive++;
                        if(SM[i].receivers_window.index_to_receive==5){
                            SM[i].receivers_window.nospace=1;
                        }
                        SM[i].receivers_window.window_size--;
                        if(expected_number==msg.header.number){
                            SM[i].receivers_window.next_sequence_number=(SM[i].receivers_window.next_sequence_number+1)%16;
                            if(SM[i].receivers_window.next_sequence_number==0)SM[i].receivers_window.next_sequence_number=1;
                        }
                        struct message_send sen_msg;
                        sen_msg.header.is_ack=1;
                        // sen_msg.header.number=

                        // sendto(SM[i].udp_socket_id,)



                        // Example code:
                        // send_ack(sd, &client_addr, client_addrlen, received_msg.sequence_number);
                    } else {
                        // ACK message received
                        // Update swnd and remove message from sender-side buffer

                        // Example code:
                        // update_swnd(sd, received_msg.sequence_number);
                    }
                }
            }
            sem_post(sem3);
        }
        else if (activity==0){

        }



    }
    return NULL;
}

void *sender_thread(void *arg) {
    // Implementation of sender thread behavior
    return NULL;
}

void *garbage_collector(void *arg) {
    // Implementation of garbage collector behavior
    return NULL;
}

int init() {
    // Create shared memory
    key_t key = 6;
    int shmid = shmget(key, MAX_MTP_SOCKETS * sizeof(struct MTPSocketInfo), 0666 | IPC_CREAT);
    if (shmid == -1) {
        perror("Error creating shared memory");
        return -1;
    }

    // Attach shared memory segment
    struct MTPSocketInfo *SM = (struct MTPSocketInfo *)shmat(shmid, NULL, 0);
    if (SM == (struct MTPSocketInfo *)(-1)) {
        perror("shmat");
        exit(1);
    }

    
    for (int i = 0; i < MAX_MTP_SOCKETS; i++) {
        SM[i].is_allocated = 0; // Mark all sockets as free
        SM[i].senders_window.next_sequence_number=1;
        SM[i].receivers_window.next_sequence_number=1;
        SM[i].receivers_window.nospace=0;
        for(int j=0;j<10;j++){
            if(j<5){
                SM[i].receivers_window.receive_messages[j].num-1;
            }
            SM[i].senders_window.send_messages[j].header.number=-1;
        }
        // Initialize other fields as necessary
    }

    // Start receiver thread
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
    pthread_t garbage_collector_tid;
    if (pthread_create(&garbage_collector_tid, NULL, garbage_collector, NULL) != 0) {
        perror("Error creating garbage collector thread");
        return -1;
    }

    key_t key = 5;
    int shmid = shmget(key, sizeof(struct SOCK_INFO), 0666 | IPC_CREAT);
    if (shmid == -1) {
        perror("Error creating shared memory for SOCK_INFO");
        return -1;
    }

    // Attach shared memory segment
    sock_info = (struct SOCK_INFO *)shmat(shmid, NULL, 0);
    if (sock_info == (struct SOCK_INFO *)(-1)) {
        perror("shmat");
        exit(1);
    }

    // Initialize SOCK_INFO
    sock_info->sock_id = 0;
    strcpy(sock_info->IP, "");
    sock_info->port = 0;
    sock_info->err_ = 0;


    sem1 = sem_open("/semaphore1", O_CREAT, 0666, 0);
    if (sem1 == SEM_FAILED) {
        perror("sem_open");
        return 1;
    }

    sem2 = sem_open("/semaphore2", O_CREAT, 0666, 0);
    if (sem2 == SEM_FAILED) {
        perror("sem_open");
        return 1;
    }
    sem3=sem_open("/semaphore3",O_CREAT,0666,1);
    if(sem3==SEM_FAILED){
        perror("sem open");
        return 1;
    }


    // Main thread waits for m_socket() or m_bind() call
    while (1) {
        sem_wait(sem1); // Wait for semaphore signal

        // Check SOCK_INFO to determine action
        if (sock_info->sock_id == 0 && strcmp(sock_info->IP, "") == 0 && sock_info->port == 0) {
            // m_socket() call
            sock_info->sock_id = socket(AF_INET, SOCK_DGRAM, 0);
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
            if (bind(sock_info->sock_id, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) == -1) {
                sock_info->err_ = errno;
                sock_info->sock_id = -1; // Reset sock_id if bind fails
            }
        }

        sem_post(sem2); // Signal completion
    }

    return 0;
}


