#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

const int error(const char *msg)
{
    perror(msg);
    return EXIT_FAILURE;
}

const int attack(const char *dest, const int portno)
{
    int sockfd, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char dest_ip[INET_ADDRSTRLEN];

    const int buffer_size = 2;
    char buffer[buffer_size+1];
    const char get_request[] = "GET / HTTP/1.1\r\n\r\n";

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    server = gethostbyname(dest);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        return EXIT_FAILURE;
    }

    inet_ntop(AF_INET, server->h_addr, dest_ip, INET_ADDRSTRLEN);
    printf("Resolved IP: %s\n", dest_ip);

    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy((char *)&serv_addr.sin_addr.s_addr,
        (char *)server->h_addr,
        server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        return error("ERROR connecting");
    n = write(sockfd,get_request,strlen(get_request));
    if (n < 0) 
         error("ERROR writing to socket");

    do
    {
        n = read(sockfd,buffer,buffer_size);
        if (n < 0) 
             return error("ERROR reading from socket");
        else if (n >  0)
        {
            buffer[buffer_size] = '\0';
            printf("%s\n",buffer);
        }
        else
            printf("No more data to read\n");
    } while( n > 0);

    close(sockfd);
    return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(EXIT_FAILURE);
    }

    attack(argv[1], atoi(argv[2]));

    return 0;
}
