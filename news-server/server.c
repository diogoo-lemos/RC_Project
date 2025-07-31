#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/shm.h>

#include "functions.h"

int main(int argc, char *argv[])
{
	// Verifica os parametros introduzidos no terminal
	if (argc != 4)
	{
		erro("Sintaxe inválida, introduza o comando da seguinte forma:\nnews_server {PORTO_CONFIG} {PORTO_NOTICIAS} {ficheiro configuração}\n");
	}
	// Ler parametros do terminal
	int porto_config = atoi(argv[2]);	// Porto para receção de pacotes UDP
	int porto_noticias = atoi(argv[1]); // Porto para ligações TCP
	char filename[20];
	strcpy(filename, argv[3]);

	int shmid_s, shmid_u, shmid_t;
	mem_struct *aux_data_block;
	shmid_s = shmget(IPC_PRIVATE, sizeof(mem_struct), IPC_CREAT|0666);	// Memory Structure
	shmid_u = shmget(IPC_PRIVATE, sizeof(User)*20, IPC_CREAT|0666);		// Max of 10 users
	shmid_t = shmget(IPC_PRIVATE, sizeof(Topic)*5, IPC_CREAT|0666); 	// Max of 5 topics

	aux_data_block = (mem_struct*) shmat(shmid_s, 0, 0);
	aux_data_block->users = (User*) shmat(shmid_u, 0, 0);
	aux_data_block->topics = (Topic*) shmat(shmid_u, 0, 0);

	FILE *f;

	aux_data_block->num_users = get_users(aux_data_block->users, f, filename); // A função retorna o número de utilizadores existentes

	if (fork() == 0) // Cria um processo que vai funcionar com a socket que recebe pacotes UDP
	{
		mem_struct *udp_data_block;
		struct sockaddr_in si_minha, si_outra;
		int s, recv_len;
		int logged_in = 0;
		int user_found = 0;
		socklen_t slen = sizeof(si_outra);
		char client_message[BUFLEN], aux_client_message[BUFLEN];
		char message[BUFLEN];

		// Attach shared memory
		udp_data_block = (mem_struct*) shmat(shmid_s, 0, 0);
		udp_data_block->users = (User*) shmat(shmid_u, 0, 0);
		udp_data_block->topics = (Topic*) shmat(shmid_t, 0, 0);

		// Limpar buffer
		memset(client_message, '\0', sizeof(client_message));
		memset(message, '\0', sizeof(message));

		// Socket para receção de pacotes UDP
		if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		{
			erro("Erro na criação do socket");
		}

		// Preenchimento da socket address structure
		si_minha.sin_family = AF_INET;
		si_minha.sin_port = htons(porto_config);
		si_minha.sin_addr.s_addr = htonl(INADDR_ANY);

		// Associa o socket à informação de endereço
		if (bind(s, (struct sockaddr *)&si_minha, sizeof(si_minha)) == -1)
		{
			erro("Erro no bind");
		}

		int socket_on = 0;
		// Recebe os 5 pacotes UDP da utilização do comando do netcat
		for (int i = 0; i < 5; i++)
		{
			if ((recv_len = recvfrom(s, client_message, BUFLEN, 0, (struct sockaddr *)&si_outra, (socklen_t *)&slen)) == -1)
			{
				erro("Erro no recvfrom");
			}
		}
		snprintf(message, BUFLEN, "SERVER: Bem vindo, introduza as suas credênciais de administrador:\n");
		send_udp(message, s, si_outra, slen);
		memset(message, '\0', sizeof(message));
		while (socket_on == 0)
		{
			if ((recv_len = recvfrom(s, client_message, BUFLEN, 0, (struct sockaddr *)&si_outra, (socklen_t *)&slen)) == -1)
			{
				erro("Erro no recvfrom");
			}
			user_found = 0;
			for (int i = 0; i < udp_data_block->num_users; i++)
			{
				printf("client_message before tokens: %s\n", client_message);
				strcpy(aux_client_message, client_message); // Copia do input para uma string auxiliar para nao existir perda de dados numa proxima iteracao do loop
				char *token;
				token = strtok(aux_client_message, " \n");
				// Verifica username
				if(strcmp((udp_data_block->users + i)->name, token) == 0)
				{
					snprintf(message, BUFLEN, "SERVER: Utilizador encontrado.\n");
					user_found = 1;
					send_udp(message, s, si_outra, slen);
					memset(message, '\0', sizeof(message));
					token = strtok(NULL, " \n");
					// Verifica se o utilizador é administrador
					if(strcmp((udp_data_block->users + i)->type, "administrador") == 0)
					{
						printf("yo1\n");
						// Verifica a password associada ao utilizador
						if(strcmp((udp_data_block->users + i)->password, token) == 0)
						{
							snprintf(message, BUFLEN, "SERVER: Autenticação efetuada com sucesso, bem vindo/a %s!\n", (udp_data_block->users + i)->name);
							send_udp(message, s, si_outra, slen);
							i = udp_data_block->num_users;
							memset(message, '\0', sizeof(message));
							logged_in = 1;
							// Consola do admin
							while(logged_in == 1)
							{
								// Recebe comando
								if ((recv_len = recvfrom(s, client_message, BUFLEN, 0, (struct sockaddr *)&si_outra, (socklen_t *)&slen)) == -1)
								{
									erro("Erro no recvfrom");
								}
								logged_in = admin_menu(udp_data_block, client_message, message, s, si_outra, slen, &socket_on);
								memset(message, '\0', sizeof(message));
							}
						}
						else
						{
							snprintf(message, BUFLEN, "SERVER: Password incorreta.\n");
							send_udp(message, s, si_outra, slen);
							i = udp_data_block->num_users;
						}
					}
					else
					{
						snprintf(message, BUFLEN, "SERVER: O utilizador introduzido não é administrador do sistema.\n");
						send_udp(message, s, si_outra, slen);
						i = udp_data_block->num_users;
					}
				}

			}
			if(user_found == 0){
				snprintf(message, BUFLEN, "SERVER: O utilizador introduzido não existe.\n");
				send_udp(message, s, si_outra, slen);	
			}

			memset(client_message, '\0', sizeof(client_message));
			memset(message, '\0', sizeof(message));
		}
		close(s);
	}
	else
	{	
		// Receção de pacotes TCP
		int fd, client;
 	 	struct sockaddr_in addr, client_addr;
  		int client_addr_size;
		pthread_t thread_id;

		bzero((void *)&addr, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
		addr.sin_port = htons(porto_noticias);

		if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
			erro("Erro na funcao socket");
		if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
			erro("Erro na funcao bind");
		if (listen(fd, 5) < 0)
			erro("Erro na funcao listen");
		client_addr_size = sizeof(client_addr);
		while (1){
			while (waitpid(-1, NULL, WNOHANG) > 0);
			// Espera por uma nova conexão
			client = accept(fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_size);
			if (client > 0){
				if(fork() == 0){
					close(fd);
					// Attach shared memory
					mem_struct *tcp_data_block;
					tcp_data_block = shmat(shmid_s, 0, 0);
					tcp_data_block->users = shmat(shmid_u, 0, 0);
					tcp_data_block->topics = shmat(shmid_t, 0, 0);			
					process_client(tcp_data_block->users, tcp_data_block->topics, client, tcp_data_block->num_users, &tcp_data_block->num_topics);
				}
			}
		}
	}

	return 0;
}