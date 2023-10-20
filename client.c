#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <signal.h>
#include <assert.h>
#include <fcntl.h>

#define PORT 3490

static atomic_bool rx_should_run;
static atomic_bool client_should_run;
static int socket_fd;

void *rx_thread_main(void *arg)
{
    char buffer[1024];
    
    while (!rx_should_run) {
    
    }
    
    while (rx_should_run) {
        const int read = recv(socket_fd, buffer, sizeof(buffer), 0);
         if (read > 0) {
            buffer[read] = 0;
            puts(buffer);
        } else if (read < 0 && errno != EWOULDBLOCK) {
            rx_should_run = false;
        }
        usleep(10 * 1000);
    }
    
    return NULL;
}

static void signal_handler(int i)
{
    client_should_run = false;
}

int main(int argc, char *argv[])
{
    client_should_run = true;
    rx_should_run = false;
    
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = signal_handler;
    assert(sigaction(SIGINT, &act, NULL) != -1);
    
    int num;
    
    char buffer[1024];
    char buff[1024];

    if (argc != 2) {
        fprintf(stderr, "Uso: nome do host cliente\n");
        exit(1);
    }

    struct hostent *he = gethostbyname(argv[1]);
    if (!he) {
        fprintf(stderr, "Nao foi possivel pegar o nome do host\n");
        exit(1);
    }

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        fprintf(stderr, "Socket Failure!!\n");
        exit(1);
    }
    
    struct sockaddr_in server_info;
    memset(&server_info, 0, sizeof(server_info));
    server_info.sin_family = AF_INET;
    server_info.sin_port = htons(PORT);
    server_info.sin_addr = *((struct in_addr *)he->h_addr);
    
    pthread_t rx_thread;
    if (pthread_create(&rx_thread, NULL, rx_thread_main, NULL) < 0) {
        perror("pthread_create");
        exit(1);
    }
    
    if (connect(socket_fd, (struct sockaddr *)&server_info, sizeof(struct sockaddr)) < 0) {
        perror("connect");
        exit(1);
    }
    
    int flags = fcntl(socket_fd, F_GETFL, 0);
    if (flags < 0) {
        perror("fcntl");
        exit(1);
    }
    
    flags |= O_NONBLOCK;
    
    flags = fcntl(socket_fd, F_SETFL, flags);
    if (flags < 0) {
        perror("fcntl");
        exit(1);
    }
    
    rx_should_run = true;
    while (client_should_run) {
        printf("Digite uma mensagem:\n");
        fgets(buffer, sizeof(buffer) - 1, stdin);
        
        if (send(socket_fd, buffer, strlen(buffer), 0) == -1) {
            fprintf(stderr, "Failure Sending Message\n");
            client_should_run = false;
            continue;
        } 
        
        if (!rx_should_run) {
            // If this flag is false, recv has failed.
            client_should_run = false;
        }
    }
    
    rx_should_run = false;
    pthread_join(rx_thread, NULL);
    
    close(socket_fd);
    return 0;   
}

