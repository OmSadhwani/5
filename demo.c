#include "msocket.h"


sem_t *sem1, *sem2,*sem3;



struct SOCK_INFO *sock_info;


void *receiver_thread(void *arg) {
    // Implementation of receiver thread behavior
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
        SM[i].senders_window.next_sequence_number=-1;
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


