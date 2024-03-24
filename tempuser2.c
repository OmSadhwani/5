// #include "msocket.h"

// int main(int argv,char* argc[]){
//     if(argv!=3){
//         printf("Usage: ./user2 <myport> <dest_port>\n");
//         exit(1);
//     }
//     int myport = atoi(argc[1]);
//     int dest_port = atoi(argc[2]);

//     char recv_filename[100];
//     sprintf(recv_filename,"%dr.txt",dest_port);
//     int fpr = open(recv_filename,O_CREAT | O_WRONLY | O_TRUNC, 0666);

//     char send_filename[100];
//     sprintf(send_filename,"%d.txt",myport);
//     int fps = open(send_filename, O_RDONLY , 0666);

//     int sockfd = m_socket(AF_INET,SOCK_MTP, 0);
//     m_bind(sockfd,"127.0.0.1",myport,"127.0.0.1",dest_port);
//     struct sockaddr_in serv_addr;
//     memset(&serv_addr,0,sizeof(serv_addr));
//     serv_addr.sin_family=AF_INET;
//     serv_addr.sin_port=htons(myport);
//     inet_aton("127.0.0.1",&serv_addr.sin_addr);
//     // int retval = m_sendto(sockfd,"Hello there",11,0,(struct sockaddr*)&serv_addr,sizeof(serv_addr));
//     char buffer[1024];
//     // memset(buffer,'\0',1025);
//     // sleep(10);
//     int i=0;

//     while(1){
//         socklen_t serv_addr_len=sizeof(serv_addr);
//         memset(buffer,'\0',sizeof(buffer));
//         int retval = m_recvfrom(sockfd,buffer,1024,0,(struct sockaddr*)&serv_addr,&serv_addr_len);
//         if(retval<0){
//             // printf("%d\n",m_errno);
//             continue;
//         }
//         // buffer[1023]='\0';
//         // printf("%s\n",buffer);
//         write(fpr,buffer,strlen(buffer));
//         fsync(fpr);
//     }

//     // printf("\nDone receiving and now sending\n\n");

//     // size_t bytes_read;
//     // while ((bytes_read = fread(buffer, 1, 1024, fps)) > 0) {
//     //     int retval=-1;
//     //     buffer[bytes_read]='\0';
//     //     char sendm[1025];
//     //     strcpy(sendm,buffer);
//     //     sendm[bytes_read]='\0';
//     //     while(retval<0){
//     //         retval = m_sendto(sockfd,sendm,bytes_read+1,0,(struct sockaddr*)&serv_addr,sizeof(serv_addr));
//     //     }
//     //     printf("%d\n",retval);
//     // }
//     fprintf(stderr,"Done\n");
//     close(fpr);
//     close(fps);

//     while(1){

//     }

//     return 0;
// }
#include "msocket.h"

#define MAX_MESSAGE_LENGTH 1023
int main(int argv,char* argc[]){
    if(argv!=3){
        printf("Usage: ./user2 <user_port> <destination_port>\n");
        exit(1);
    }
    size_t bytes_read;
    int destination_port = atoi(argc[2]);
    int user_port = atoi(argc[1]);
    struct sockaddr_in serv_addr;
    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_port=htons(user_port);
    inet_aton("127.0.0.1",&serv_addr.sin_addr);
    char buffer[1024];
    int i=0;

    //the received data  is stored in the file destination_port_r.txt
    char rfile[100];
    sprintf(rfile,"%dr.txt",destination_port);
    int fp_receive = open(rfile,O_CREAT | O_WRONLY | O_TRUNC, 0666);

    char sfile[100];
    sprintf(sfile,"%d.txt",user_port);
    int fp_send = open(sfile, O_RDONLY , 0666);

    int sockfd = m_socket(AF_INET,SOCK_MTP, 0);
    m_bind(sockfd,"127.0.0.1",user_port,"127.0.0.1",destination_port);
    

    while(1){
        socklen_t addr_len=sizeof(serv_addr);
        memset(buffer,'\0',sizeof(buffer));
        int retval = m_recvfrom(sockfd,buffer,1024,0,(struct sockaddr*)&serv_addr,&addr_len);
        if(retval<0){
            continue;
        }
        if(strcmp(buffer,"\r\n")==0) break;
        write(fp_receive,buffer,strlen(buffer));
        fsync(fp_receive);
    }
    printf("Receive Successful\n");
    // int message=0;
    // while ((bytes_read = read(fp_send, buffer, MAX_MESSAGE_LENGTH)) > 0) {
    //     message++;
    //     int retval=-1;
     
    //     while(retval<0){
    //         retval = m_sendto(sockfd,buffer,bytes_read,0,(struct sockaddr*)&serv_addr,sizeof(serv_addr));
    //     }
    // }

    // int retval = m_sendto(sockfd,"\r\n",3,0,(struct sockaddr*)&serv_addr,sizeof(serv_addr));

    // printf("Send Succesfull\n");
    // while(1){}
    close(fp_receive);
    close(fp_send);

    return 0;
}