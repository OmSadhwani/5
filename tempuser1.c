// #include "msocket.h"

// #define MAX_MESSAGE_LENGTH 1023

// int main(int argv,char* argc[]){
//     if(argv!=3){
//         printf("Usage: ./user1 <myport> <dest_port>\n");
//         exit(1);
//     }

//     int myport = atoi(argc[1]);
//     int dest_port = atoi(argc[2]);

//     char recv_filename[100];
//     sprintf(recv_filename,"%dr.txt",dest_port);
//     int fpr = open(recv_filename,O_CREAT | O_WRONLY | O_TRUNC , 0666);

//     char send_filename[100];
//     sprintf(send_filename,"%d.txt",myport);
//     int fps = open(send_filename,O_RDONLY,0666);

//     int sockfd = m_socket(AF_INET,SOCK_MTP, 0);
//     m_bind(sockfd,"127.0.0.1",myport,"127.0.0.1",dest_port);
//     struct sockaddr_in serv_addr;
//     memset(&serv_addr,0,sizeof(serv_addr));
//     serv_addr.sin_family=AF_INET;
//     serv_addr.sin_port=htons(dest_port);
//     inet_aton("127.0.0.1",&serv_addr.sin_addr);
//     int len = sizeof(serv_addr);
//     int i=0;

//     char buffer[MAX_MESSAGE_LENGTH+1];
//     size_t bytes_read;

//     int message=0;
//     while ((bytes_read = read(fps, buffer, MAX_MESSAGE_LENGTH)) > 0) {
//         message++;
//         int retval=-1;
//         // char sendm[MAX_MESSAGE_LENGTH];
//         // strcpy(sendm,buffer);
     
//         while(retval<0){
//             retval = m_sendto(sockfd,buffer,bytes_read,0,(struct sockaddr*)&serv_addr,sizeof(serv_addr));
//         }
        
//         // printf("%s",sendm);
//         // i++;
//     }
//     // printf("%d\n",i);

//     // printf("\nDone sending and now receving\n\n");

//     // while(1){
//     //     socklen_t serv_addr_len=sizeof(serv_addr);
//     //     int retval = m_recvfrom(sockfd,buffer,1024,0,(struct sockaddr*)&serv_addr,&serv_addr_len);
//     //     if(retval<0){
//     //         continue;
//     //     }
//     //     buffer[retval]='\0';
//     //     fwrite(buffer,1,strlen(buffer)+1,fpr);
//     // }

//     fprintf(stderr,"Done %d messages \n",message);
    
//     close(fpr);
//     close(fps);

//     while(1){

//     }


//     exit(0);
// }

#include "msocket.h"

#define MAX_MESSAGE_LENGTH 1023

int main(int argv,char* argc[]){
    if(argv!=3){
        printf("Usage: ./user1 <user_port> <destination_port>\n");
        exit(1);
    }


    char buffer[MAX_MESSAGE_LENGTH+1];
    size_t bytes_read;

    int user_port = atoi(argc[1]);
    int destination_port = atoi(argc[2]);

    struct sockaddr_in serv_addr;
    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_port=htons(destination_port);
    inet_aton("127.0.0.1",&serv_addr.sin_addr);
    int len = sizeof(serv_addr);
    int i=0;
    char rfile[100];
    sprintf(rfile,"%dr.txt",destination_port);
    int fp_receive = open(rfile,O_CREAT | O_WRONLY | O_TRUNC , 0666);

    //assuming that the text is read from user_port.txt file
    char sfile[100];
    sprintf(sfile,"%d.txt",user_port);
    int fp_send = open(sfile,O_RDONLY,0666);

    int sockfd = m_socket(AF_INET,SOCK_MTP, 0);
    m_bind(sockfd,"127.0.0.1",user_port,"127.0.0.1",destination_port);

    int message=0;
    while ((bytes_read = read(fp_send, buffer, MAX_MESSAGE_LENGTH)) > 0) {
        message++;
        int retval=-1;
     
        while(retval<0){
            retval = m_sendto(sockfd,buffer,bytes_read,0,(struct sockaddr*)&serv_addr,sizeof(serv_addr));
        }
    }
    char finalblock[]="\r\n";
    // int retval = m_sendto(sockfd,finalblock,sizeof(finalblock),0,(struct sockaddr*)&serv_addr,sizeof(serv_addr));
    while ((bytes_read = m_sendto(sockfd, finalblock, strlen(finalblock), 0, (struct sockaddr *) &serv_addr, sizeof(serv_addr))) < 0) {
        if (errno == ENOBUFS) continue; // Retry if the error is due to lack of buffer space
        else {
            printf("Send failed from user 1\n");
            close(fp_send);
            close(fp_receive);
            exit(EXIT_FAILURE);
        }
    }
    printf("Send Succesfull\n");

    // while(1){
    //     socklen_t addr_len=sizeof(serv_addr);
    //     memset(buffer,'\0',sizeof(buffer));
    //     int retval = m_recvfrom(sockfd,buffer,1024,0,(struct sockaddr*)&serv_addr,&addr_len);
    //     if(retval<0){
    //         continue;
    //     }
    //     if(strcmp(buffer,"\r\n")==0) break;
    //     write(fp_receive,buffer,strlen(buffer));
    //     fsync(fp_receive);
    // }
    // printf("Receive Successful\n");

    while(1);
    close(fp_receive);
    close(fp_send);
    exit(0);
}