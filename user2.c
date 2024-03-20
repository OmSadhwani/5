
#include "msocket.h"

int main(){
    printf("hi\n");
    int sockfd = m_socket(AF_INET,SOCK_MTP, 0);
    m_bind(sockfd,"127.0.0.1",6001,"127.0.0.1",6000);
    struct sockaddr_in serv_addr;
    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_port=htons(6001);
    inet_aton("127.0.0.1",&serv_addr.sin_addr);
    // int retval = m_sendto(sockfd,"Hello there",11,0,(struct sockaddr*)&serv_addr,sizeof(serv_addr));
    char buffer[1024];
    memset(buffer,'\0',1024);
    // sleep(10);
    int i=0;
    while(i<25){
        int retval = m_recvfrom(sockfd,buffer,1024,0,(struct sockaddr*)&serv_addr,sizeof(serv_addr));
        if(retval>0){
            fprintf(stderr,"Received: %s\n",buffer);
            i++;
        }
        
    }

    int retval = m_sendto(sockfd,"Bye\0",5,0,(struct sockaddr*)&serv_addr,sizeof(serv_addr));
    fprintf(stderr,"Sent: %s\n","Bye");

    // int retval = m_recvfrom(sockfd,buffer,1024,0,(struct sockaddr*)&serv_addr,sizeof(serv_addr));
    // fprintf(stderr, "Received: %s\n", buffer);
    return 0;
}
