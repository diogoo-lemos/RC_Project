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

void erro(char *s)
{
	perror(s);
	exit(1);
}

// Lê os conteúdos do ficheiro de utilizadores
int get_users(User *users, FILE *f, char *filename)
{
	int num_users = 0;
	f = fopen(filename, "r");
	if (f == NULL)
	{
		erro("Erro ao abrir o ficheiro.");
	}

	char line[100];
	while (fgets(line, sizeof(line), f) != NULL)
	{
		char *token;
		// Nome
		token = strtok(line, ";");
		strcpy(users->name, token);
		printf("name: %s\n", users->name);
		// Password
		token = strtok(NULL, ";");
		strcpy(users->password, token);
		printf("pass: %s\n", users->password);
		// Tipo
		token = strtok(NULL, ";\n");
		strcpy(users->type, token);
		printf("type: %s\n", users->type);

		// printf("name: %s, pass: %s, type: %s\n", users->name, users->password, users->type);
		users += 1;
		num_users++;
	}
	fclose(f);
	return num_users;
}

void send_udp(char *message, int s, struct sockaddr_in si_outra, socklen_t slen){
	if(sendto(s, message, BUFLEN, 0, (struct sockaddr*)&si_outra, slen) < 0){
        printf("Unable to send UDP message\n");
    }
}

void send_tcp(int __fd, const void *__buf, size_t __n){
	if(write(__fd, __buf, __n) < 0){
		printf("Unable to send TCP message\n");
	}
}

//Menu admin para introdução de comandos
int admin_menu(mem_struct *shmem, char *client_message, char *message, int s, struct sockaddr_in si_outra, socklen_t slen, int *s_state)
{
	char *token;
	token = strtok(client_message, " \n");
	if(strcmp(token, "QUIT") == 0)
	{
		snprintf(message, BUFLEN, "SERVER: Sessão encerrada. A voltar ao menu de login.\n");
		send_udp(message, s, si_outra, slen);
		return 0;
	}
	if(strcmp(token, "QUIT_SERVER") == 0)
	{
		snprintf(message, BUFLEN, "SERVER: A encerrar servidor...\n");
		send_udp(message, s, si_outra, slen);
		*s_state = 0;
		return 0;
	}
	if(strcmp(token, "LIST") == 0)
	{
		snprintf(message, BUFLEN, "SERVER: Utilizadores:\n");
		send_udp(message, s, si_outra, slen);
		for(int i = 0; i < shmem->num_users; i++){
			memset(message, '\0', BUFLEN);
			snprintf(message, BUFLEN, "User: %s, type: %s\n", (shmem->users + i)->name, (shmem->users + i)->type);
			send_udp(message, s, si_outra, slen);
		}
		return 1;
	}
	if(strcmp(token, "DEL") == 0)
	{
		int found = 0;
		token = strtok(NULL, " \n");
		memset(message, '\0', BUFLEN);
		for(int i = 0; i < shmem->num_users; i++){
			if(strcmp((shmem->users + i)->name, token) == 0){
				found = 1;
				if(i == shmem->num_users - 1){
					strcpy((shmem->users + i)->name, "");
					strcpy((shmem->users + i)->type, "");
					strcpy((shmem->users + i)->password, "");
				}
				else{
					strcpy((shmem->users + i)->name, (shmem->users + shmem->num_users - 1)->name);
					strcpy((shmem->users + i)->type, (shmem->users + shmem->num_users - 1)->type);
					strcpy((shmem->users + i)->password, (shmem->users + shmem->num_users - 1)->password);
				}
				shmem->num_users = shmem->num_users - 1;
				snprintf(message, BUFLEN, "SERVER: User <%s> deleted.\n", token);
				send_udp(message, s, si_outra, slen);
			}
		}
		if(found == 0){
			snprintf(message, BUFLEN, "SERVER: User not found.\n", token);
			send_udp(message, s, si_outra, slen);

		}
		return 1;
	}
	if(strcmp(token, "ADD_USER") == 0)
	{
		if(shmem->num_users < 20){
			token = strtok(NULL, " \n");
			strcpy((shmem->users + shmem->num_users)->name, token);
			token = strtok(NULL, " \n");
			strcpy((shmem->users + shmem->num_users)->password, token);
			token = strtok(NULL, " \n");
			strcpy((shmem->users + shmem->num_users)->type, token);
			shmem->num_users = shmem->num_users + 1;
			snprintf(message, BUFLEN, "SERVER: Utilizador criado.\n");
			send_udp(message, s, si_outra, slen);
		}
		else{
			snprintf(message, BUFLEN, "SERVER: Número de ultilizadores excedido.\n");
			send_udp(message, s, si_outra, slen);
		}
		return 1;
	}
	else
	{
		snprintf(message, BUFLEN, "SERVER: Comando não existente.\n");
		send_udp(message, s, si_outra, slen);
		return 1;
	}
}

//Menu client para introdução de comandos
int client_menu(Topic *topics, int *num_topics, char *command, char *username, char *type, int client_fd, char *message_tcp){

	char *token;
	token = strtok(command, " \n");
	printf("token before strcmp: %s\n", token);
	// Caso o client seja leitor
	if(strcmp(type, "leitor") == 0){
		if(strcmp(token, "LIST_TOPICS") == 0){
			memset(message_tcp, '\0', BUFLEN);
			snprintf(message_tcp, BUFLEN, "Tópicos disponíveis:\n");
			send_tcp(client_fd, message_tcp, BUFLEN);
			// Lista os tópicos
			for(int i = 0; i < 5; i++){
				memset(message_tcp, '\0', BUFLEN);
				snprintf(message_tcp, BUFLEN, "Título: %s ID: %s\n", (topics + i)->title, (topics + i)->id);
				printf("line sent: %S\n", message_tcp);
				send_tcp(client_fd, message_tcp, BUFLEN);
			}
			printf("exited loop\n");
			return 1;
		}
		else if(strcmp(token, "SUBSCRIBE_TOPIC") == 0){
			token = strtok(NULL, " \n");
			printf("%d", *num_topics);
			for(int i = 0; i < *num_topics; i++){
				printf("id: %s, token: %s\n", (topics + i)->id, token);
				printf("multicast: %s", (topics + i)->multicast);
				if(strcmp(((topics + i)->id), token) == 0){
					memset(message_tcp, '\0', BUFLEN);
					snprintf(message_tcp, BUFLEN, "%s", (topics + i)->multicast);
					send_tcp(client_fd, message_tcp, BUFLEN);
					i = *num_topics;
				}
			}
		}
		else if(strcmp(token, "QUIT") == 0){
			return 0;
		}
		else{
			printf("comando n existe.\n");
			return 1;
		}
	}
	// Caso o client seja jornalista
	else{

		int sock;
		// create a UDP socket
		if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
			perror("socket");
			exit(1);
		}
		if(strcmp(token, "CREATE_TOPIC") == 0){
			if(*num_topics < 10){
				token = strtok(NULL, " \n");
				strcpy((topics + *num_topics)->title, token);
				token = strtok(NULL, " \n");
				strcpy((topics + *num_topics)->id, token);
				memset(message_tcp, '\0', BUFLEN);
				snprintf(message_tcp, BUFLEN, "Tópico %s criado.\n", (topics + *num_topics)->title);
				send_tcp(client_fd, message_tcp, BUFLEN);
				*num_topics = *num_topics + 1;
				printf("num topics: %d", *num_topics);
				return 1;
			}
			else{

			}
		}
		else if(strcmp(token, "SEND_NEWS") == 0){
			token = strtok(NULL, " \n");
			for(int i = 0; i < *num_topics; i++){
				if(strcmp((topics + i)->id, token) == 0){
					
					struct sockaddr_in addr;
					char msg[BUFLEN];

					// set up the multicast address structure
					memset(&addr, 0, sizeof(addr));
					addr.sin_family = AF_INET;
					addr.sin_addr.s_addr = inet_addr((topics + i)->multicast);
					addr.sin_port = htons(5000);

					// enable multicast on the socket
					int enable = 1;
					if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &enable, sizeof(enable)) < 0) {
						perror("setsockopt");
						exit(1);
					}

					token = strtok(NULL, " \n");
					while(token != NULL){
						strcat(msg, token);
						strcat(msg, " ");
						token = strtok(NULL, " \n");
					}

					// send the multicast message
					if (sendto(sock, msg, BUFLEN, 0, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
						perror("sendto");
						exit(1);
					}
					i = *num_topics;
				}
				else{
					//tópico não existe
					memset(message_tcp, '\0', BUFLEN);
					snprintf(message_tcp, BUFLEN, "Tópico não existe.\n");
					send_tcp(client_fd, message_tcp, BUFLEN);
				}
			}
		}
		else if(strcmp(token, "QUIT") == 0){
			return 0;
		}
		else{
			printf("comando n existe.\n");
			return 1;
		}
	}
	return 1;
}

void process_client(User *users, Topic *topics, int client_fd, int num_users, int *num_topics)
{
	int nread = 0;
	int socket_on = 0;
	int logged_in = 0;
	int user_found = 0;
	char buffer[BUFLEN];
	char message_tcp[BUFLEN];
	char *token;

	// Create a multicast IP address for each topic
	strcpy(topics->multicast, "224.0.0.1");
	strcpy((topics + 1)->multicast, "224.0.0.2");
	strcpy((topics + 2)->multicast, "224.0.0.3");
	strcpy((topics + 3)->multicast, "224.0.0.4");
	strcpy((topics + 4)->multicast, "224.0.0.5");

	memset(message_tcp, '\0', sizeof(message_tcp));
	snprintf(message_tcp, BUFLEN, "SERVER: Bem vindo ao porto de noticias do servidor. Introduza as suas credenciais: \n");
	send_tcp(client_fd, message_tcp, BUFLEN);

	while(socket_on == 0){
		// Recebe informações de login
		nread = read(client_fd, buffer, BUFLEN - 1);
		buffer[nread] = '\0';

		printf("Message recieved: %s", buffer);

		token = strtok(buffer, " \n");

		for(int i = 0; i < num_users; i++){
			// Procura e verifica username
			if(strcmp((users + i)->name, token) == 0){
				user_found = 1;
				token = strtok(NULL, " \n");
				// Procura e verifica password
				if(strcmp((users + i)->password, token) == 0){
					logged_in = 1;
					memset(message_tcp, '\0', sizeof(message_tcp));
					snprintf(message_tcp, BUFLEN, "SERVER: Login efetuado com sucesso! Bem vindo/a %s!.\n", (users + i)->name);
					send_tcp(client_fd, message_tcp, BUFLEN);
					while(logged_in == 1){
						//Recebe o comando
						nread = read(client_fd, buffer, BUFLEN - 1);
						buffer[nread] = '\0';
						logged_in = client_menu(topics, num_topics, buffer, (users + i)->name, (users + i)->type, client_fd, message_tcp);
						printf("logged_in = %d\n", logged_in);
					}
					socket_on = 1;
					i = num_users;
				}
				else{
					memset(message_tcp, '\0', sizeof(message_tcp));
					snprintf(message_tcp, BUFLEN, "SERVER: Password incorreta, reintroduza as credênciais.\n", (users + i)->name);
					send_tcp(client_fd, message_tcp, BUFLEN);
				}
			}
		}
		// Utilizador não encontrado
		if(user_found == 0){
			memset(message_tcp, '\0', sizeof(message_tcp));
			snprintf(message_tcp, BUFLEN, "SERVER: Utilizador introduzido não existe.\n");
			send_tcp(client_fd, message_tcp, BUFLEN);
		}
		user_found = 0;
	}
}