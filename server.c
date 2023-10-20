#define _GNU_SOURCE

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
#include <stdbool.h>
#include <assert.h>
#include <stdatomic.h>
#include <pthread.h>
#include <signal.h>

#define PORT 3490
#define BACKLOG 10
#define MAX_CONNECTIONS 4 

struct connection {
    int client_socket;
    bool is_used;
};

static atomic_bool server_should_run;
static atomic_bool chat_should_run;

static pthread_mutex_t connection_mutex;
static struct connection connections[MAX_CONNECTIONS];

static void add_client(int socket)
{
    for (int i = 0; i < MAX_CONNECTIONS; ++i) {
        if (!connections[i].is_used) {
            connections[i].is_used = true;
            connections[i].client_socket = socket;
            break;
        }
    }
}

static void remove_client(int socket)
{
    for (int i = 0; i < MAX_CONNECTIONS; ++i) {
        if (connections[i].client_socket == socket) {
            connections[i].is_used = false;
            connections[i].client_socket = 0;
            break;
        }
    }
}

static void signal_handler(int i)
{
    server_should_run = false;
}

static void broadcast(int from_client_index, const char* message, int length)
{
    for (int i = 0; i < MAX_CONNECTIONS; ++i) {
        if (i == from_client_index) {
            continue;
        } 
        send(connections[i].client_socket, message, length, 0);
    }
}

void *chat_thread_main(void *arg)
{
    char buffer[1024];
 
    while (!chat_should_run) {
        
    }
    
    while (chat_should_run) {
        pthread_mutex_lock(&connection_mutex);
        
        for (int i = 0; i < MAX_CONNECTIONS; ++i) {
            if (!connections[i].is_used) {
                continue;
            }
            
            struct connection* con = &connections[i];
            const int read = recv(con->client_socket, buffer, sizeof(buffer), 0);
            
            if (read < 0 && errno != EWOULDBLOCK) {
                shutdown(con->client_socket, SHUT_RDWR);
                close(con->client_socket);
                remove_client(con->client_socket);
                fputs("removed client\n", stderr);
            } else if (read > 0) {
                buffer[read] = 0;
                puts(buffer);
                broadcast(i, buffer, read);
            }
        }
        
        pthread_mutex_unlock(&connection_mutex);
        usleep(10 * 1000);
    }
    
    return NULL;
}

int main(void) {
    server_should_run = true;
    chat_should_run = false;
    
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = signal_handler;
    assert(sigaction(SIGINT, &act, NULL) != -1);
    
    memset(&connections, 0, sizeof(connections));
    
    const int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        fprintf(stderr, "falha no socket!!\n");
        exit(1);
    }
    
    struct sockaddr_in server_adr;
    memset(&server_adr, 0, sizeof(server_adr));
    server_adr.sin_family =  AF_INET;
    server_adr.sin_port = htons(PORT);
    server_adr.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(socket_fd, (struct sockaddr *)&server_adr, sizeof(struct sockaddr )) < 0) { 
        fprintf(stderr, "falha no bind\n");
        exit(1);
    }

    if (listen(socket_fd, BACKLOG) < 0) {
        fprintf(stderr, "falha ao criar servidor\n");
        exit(1);
    }
    
    pthread_t chat_thread;
    if (pthread_create(&chat_thread, NULL, chat_thread_main, NULL) < 0) {
        exit(1);
    }
    
    if (pthread_mutex_init(&connection_mutex, NULL) < 0) {
        exit(1);
    }
    
    chat_should_run = true;
    while (server_should_run) {
        struct sockaddr_in client_adr;
        socklen_t size = sizeof(client_adr);
        memset(&client_adr, 0, sizeof(client_adr));
        
        const int client_fd = accept4(socket_fd, (struct sockaddr *)&client_adr, &size, SOCK_NONBLOCK);
        if (client_fd < 0) {
            perror("accept4");
            continue;
        }
        
        pthread_mutex_lock(&connection_mutex);
        add_client(client_fd);
        pthread_mutex_unlock(&connection_mutex);
        
        printf("Conectado ao cliente %s\n", inet_ntoa(client_adr.sin_addr));
        usleep(10 * 1000);
    }
    
    chat_should_run = false;
    pthread_join(chat_thread, NULL);
    
    //struct sockaddr_in dest;
    //int status, client_fd;
    
#if 0 
    int yes =1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt");
        exit(1);
    }
#endif 
    shutdown(socket_fd, SHUT_RDWR);
    close(socket_fd); 
    return 0;
}
