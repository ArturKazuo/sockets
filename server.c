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

#define PORT 3490
#define BACKLOG 10
#define MAXSIZE 1024

int main()
{
    struct sockaddr_in server;
    struct sockaddr_in dest;
    int status,socket_fd, client_fd,num;
    socklen_t size;

    char buffer[10241];
    char *buff;
//  memset(buffer,0,sizeof(buffer));
    int yes =1;



    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0))== -1) {
        fprintf(stderr, "falha no socket!!\n");
        exit(1);
    }

    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt");
        exit(1);
    }
    memset(&server, 0, sizeof(server));
    memset(&dest,0,sizeof(dest));
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = INADDR_ANY; 
    if ((bind(socket_fd, (struct sockaddr *)&server, sizeof(struct sockaddr )))== -1)    { //sizeof(struct sockaddr) 
        fprintf(stderr, "falha no bind\n");
        exit(1);
    }

    if ((listen(socket_fd, BACKLOG))== -1){
        fprintf(stderr, "falha ao criar servidor\n");
        exit(1);
    }

    while(1) {

        size = sizeof(struct sockaddr_in);

        if ((client_fd = accept(socket_fd, (struct sockaddr *)&dest, &size))==-1 ) {
            perror("accept");
            exit(1);
        }
        printf("Conectado ao cliente %s\n", inet_ntoa(dest.sin_addr));

        while(1) {

                if ((num = recv(client_fd, buffer, 1024,0))== -1) {
                        perror("recv");
                        exit(1);
                }
                else if (num == 0) {
                        printf("conexao terminada\n");
                        //So I can now wait for another client
                        break;
                }
                buffer[num] = '\0';
                printf("\nMensagem recebida do cliente: %s\n", buffer);
                printf("Digite uma mensagem:\n");
                fgets(buffer,MAXSIZE-1,stdin);
                if ((send(client_fd,buffer, strlen(buffer),0))== -1) 
                {
                     fprintf(stderr, "Erro ao mandar mensagem\n");
                     close(client_fd);
                     break;
                }

                // printf("Server:Msg being sent: %s\nNumber of bytes sent: %ld\n",buffer, strlen(buffer));

        } //End of Inner While...
        //Close Connection Socket
        close(client_fd);
    } //Outer While
    close(socket_fd); 
    return 0;
}