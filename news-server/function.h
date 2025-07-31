#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/shm.h>


#include "structures.h"

void erro(char *s);
int admin_menu(mem_struct *shmem, char *client_message, char *message, int s, struct sockaddr_in si_outra, socklen_t slen, int *s_state);
int get_users(User *users, FILE *f, char *filename);
void send_udp(char *message, int s, struct sockaddr_in si_outra, socklen_t slen);
void send_tcp(int __fd, const void *__buf, size_t __n);
void process_client(User *users, Topic *topics, int client_fd, int num_users, int *num_topics);
int client_menu(Topic *topics, int *num_topics, char *command, char *username, char *type, int client_fd, char *message_tcp);