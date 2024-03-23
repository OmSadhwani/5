#include "msocket.h"

#define MAX_MESSAGE_LENGTH 1023

int main(int argv,char* argc[]){
    if(argv!=3){
        printf("Usage: ./user1 <myport> <dest_port>\n");
        exit(1);
    }

    int myport = atoi(argc[1]);
    int dest_port = atoi(argc[2]);

    char recv_filename[100];
    sprintf(recv_filename,"%dr.txt",dest_port);
    int fpr = open(recv_filename,O_CREAT | O_WRONLY | O_TRUNC , 0666);

    char send_filename[100];
    sprintf(send_filename,"%d.txt",myport);
    int fps = open(send_filename,O_RDONLY,0666);

    printf("Before\n");
    int sockfd = m_socket(AF_INET,SOCK_MTP, 0);
    printf("After\n");
    m_bind(sockfd,"127.0.0.1",myport,"127.0.0.1",dest_port);
    struct sockaddr_in serv_addr;
    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_port=htons(dest_port);
    inet_aton("127.0.0.1",&serv_addr.sin_addr);
    int len = sizeof(serv_addr);
    int i=0;

    char buffer[MAX_MESSAGE_LENGTH+1];
    size_t bytes_read;

    // int i=0;
    while ((bytes_read = read(fps, buffer, MAX_MESSAGE_LENGTH)) > 0) {
        int retval=-1;
        // char sendm[MAX_MESSAGE_LENGTH];
        // strcpy(sendm,buffer);
        printf("here\n");
        while(retval<0){
            retval = m_sendto(sockfd,buffer,bytes_read,0,(struct sockaddr*)&serv_addr,sizeof(serv_addr));
        }
        printf("here2\n");
        // printf("%s",sendm);
        // i++;
    }
    // printf("%d\n",i);

    // printf("\nDone sending and now receving\n\n");

    // while(1){
    //     socklen_t serv_addr_len=sizeof(serv_addr);
    //     int retval = m_recvfrom(sockfd,buffer,1024,0,(struct sockaddr*)&serv_addr,&serv_addr_len);
    //     if(retval<0){
    //         continue;
    //     }
    //     buffer[retval]='\0';
    //     fwrite(buffer,1,strlen(buffer)+1,fpr);
    // }

    fprintf(stderr,"Done\n");
    
    close(fpr);
    close(fps);

    while(1){

    }


    exit(0);
}