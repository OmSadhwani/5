
#include "msocket.h"

int main(){
    int sockfd = m_socket(AF_INET,SOCK_MTP, 0);
    printf("hi\n"); 
    m_bind(sockfd,"127.0.0.1",6000,"127.0.0.1",6001);
    struct sockaddr_in serv_addr;
    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_port=htons(6001);
    inet_aton("127.0.0.1",&serv_addr.sin_addr);
    int len = sizeof(serv_addr);
    int i=0;
    // while(i<25){
    //     int retval=-1;
    //     char buffer[1024];
    //     memset(buffer,'\0',1024);
    //     strcpy(buffer,"Hello there");
    //     // Cat i
    //     char num[10];
    //     memset(num,'\0',sizeof(num));
    //     sprintf(num,"%d",i);
    //     strcat(buffer,num);
    //     while(retval<0){
    //     retval = m_sendto(sockfd,buffer,strlen(buffer),0,(struct sockaddr*)&serv_addr,sizeof(serv_addr));

    //     }
    //     fprintf(stderr, "Sent: %s\n", buffer);
    //     i++;
    // }
    FILE* fp= fopen("text.txt","r");
    char buffer[1024];
    memset(buffer,'\0',sizeof(buffer));
    while(fgets(buffer,1023,fp)!=NULL){
        int retval= m_sendto(sockfd,buffer,1024,0,(struct sockaddr*)&serv_addr,sizeof(serv_addr));
        while(retval<0){
            retval=m_sendto(sockfd,buffer,1024,0,(struct sockaddr*)&serv_addr,sizeof(serv_addr));
        }
        printf("%s\n",buffer);
        memset(buffer,'\0',sizeof(buffer));
    }
    

    // char buffer[1024];
    // while(1){
    //     int retval = m_recvfrom(sockfd,buffer,1024,0,(struct sockaddr*)&serv_addr,sizeof(serv_addr));
    //     if(retval>0){
    //         break;
    //     }
    // }
    // printf("%s\n",buffer);

    // int retval = m_sendto(sockfd,"Hello there",11,0,(struct sockaddr*)&serv_addr,sizeof(serv_addr));
    // fprintf(stderr, "Sent: %s\n", "Hello there");
    return 0;
}

