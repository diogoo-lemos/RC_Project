#define BUFLEN 1024 // Tamanho do buffer

// Formato de dados correspondentes a um utilizador
typedef struct
{
	char name[20];
	char password[20];
	char type[20];
} User;

// Formato de dados de um tópico
typedef struct{
	char title[50];
	char id[15];
	char multicast[16];
} Topic;

// Formato da shared memory
typedef struct{
	int num_users;
	int num_topics;
	User *users;
	Topic *topics;
} mem_struct;