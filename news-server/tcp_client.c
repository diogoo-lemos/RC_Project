#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>

#define BUFLEN 1024
#define _ANY 0

void erro(char *msg);
void handle_sigint(int sig);

int multicast_recieve = 1;
int connection_state = 1;

int main(int argc, char *argv[])
{
    char endServer[100];
    int fd;
    struct sockaddr_in addr, addr_m;
    struct hostent *hostPtr;

    signal(SIGINT, handle_sigint);

    if (argc != 3)
    {
        printf("cliente <host> <port> \n");
        exit(-1);
    }

    strcpy(endServer, argv[1]);
    if ((hostPtr = gethostbyname(endServer)) == 0)
        erro("Não consegui obter endereço");

    bzero((void *)&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ((struct in_addr *)(hostPtr->h_addr))->s_addr;
    addr.sin_port = htons((short)atoi(argv[2]));

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        erro("socket");
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        erro("Connect");

    char msg[BUFLEN];
    char buffer[BUFLEN];
    char cmd[20];

     while (connection_state)
    {
        // Receive message from server
        if(read(fd, buffer, BUFLEN) < 0)
        {
            printf("Receive failed");
            connection_state = 0;
        }
        else
        {
            printf("%s", buffer);
        }

        // Send message to server
        fgets(msg, BUFLEN, stdin);
        sscanf(msg, "%s", cmd);

        if(strcmp(msg, "QUIT\n") == 0)
        {
            write(fd, msg, strlen(msg));
            memset(msg, '\0', BUFLEN);
            connection_state = 0;
        }

        if(strcmp(msg, "LIST_TOPICS\n") == 0)
        {
            write(fd, msg, strlen(msg));
            memset(msg, '\0', BUFLEN);
            for (int i = 0; i < 5; i++)
            {
                printf("%d - ", i);
                if(read(fd, buffer, BUFLEN) < 0)
                {
                    printf("Receive failed");
                }
                printf("%s", buffer);
            }
        }

        if(strcmp(cmd, "SUBSCRIBE_TOPIC") == 0)
        {
            write(fd, msg, strlen(msg));
            if(read(fd, buffer, BUFLEN) < 0)
            {
                printf("Receive failed");
            }
            else
            {
                if(fork() == 0)
                {
                    // create a UDP socket
                    int sock;
                    socklen_t addrlen = sizeof(struct sockaddr_in);
                    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
                        perror("socket");
                        exit(1);
                    }

                    // set up the multicast address structure
                    memset(&addr_m, 0, sizeof(addr_m));
                    addr_m.sin_family = AF_INET;
                    addr_m.sin_addr.s_addr = _ANY;
                    addr_m.sin_port = htons(5000);

                    // bind the socket to the port
                    if (bind(sock, (struct sockaddr *)&addr_m, sizeof(addr_m)) < 0) {
                        perror("bind");
                        exit(1);
                    }

                    // join the multicast group
                    struct ip_mreq mreq;
                    mreq.imr_multiaddr.s_addr = inet_addr(buffer);
                    mreq.imr_interface.s_addr = INADDR_ANY;
                    if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
                        perror("setsockopt");
                        exit(1);
                    }

                    // receive the multicast message
                    int nbytes;
                    while(multicast_recieve)
                    {
                        if ((nbytes = recvfrom(sock, msg, sizeof(msg), 0, (struct sockaddr *)&addr, (socklen_t *)&addrlen)) < 0) {
                            perror("recvfrom");
                            exit(1);
                        }
                        printf("Noticia recebida: %s\n", msg);
                    }

                    // leave the multicast group
                    if (setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
                        perror("setsockopt");
                    }

                    // close the socket
                    close(sock);
                    }
                    memset(buffer, '\0', BUFLEN);
                }
            }

        if(connection_state)
        {
            write(fd, msg, strlen(msg));
            memset(msg, '\0', BUFLEN);
        }
    }
    close(fd);
}

void handle_sigint(int sig)
{
    multicast_recieve = 0;
    connection_state = 0;
    signal(sig, SIG_DFL);
    kill(getpid(), SIGINT);
}

void erro(char *msg)
{
    printf("Erro: %s\n", msg);
    exit(-1);
}
