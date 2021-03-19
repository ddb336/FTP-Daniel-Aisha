#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdbool.h>

#define NUM_USERS 2
#define MAX_SOCKETS 30

struct serverUser
{
	char *userName;
	char *password;
	int state; // 0 is not yet userrequested, 1 is userrequested (waiting for pass), 2 is authenticated
	int sock_num;
};

void serve_client(int client_sd, struct serverUser *auth_users, int *sock_array);
void close_client(int client_fd, struct serverUser *auth_users, int *sock_array);

int main()
{
	//0. User data
	struct serverUser auth_users[NUM_USERS];
	auth_users[0].userName = "Aisha";
	auth_users[0].password = "AHPass";
	auth_users[0].state = 0;
	auth_users[1].userName = "Daniel";
	auth_users[1].password = "DBPass";
	auth_users[1].state = 0;

	//1. sockets
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	int sock_array[MAX_SOCKETS] = {0};

	if (server_fd < 0)
	{
		perror("Socket: ");
		return (-1);
	}

	struct sockaddr_in server_address;
	memset(&server_address, 0, sizeof(server_address));

	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(9000);
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);

	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
	{
		perror("Setsockopt:");
		return -1;
	}

	//2. bind
	if (bind(server_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
	{
		perror("Bind: ");
		return -1;
	}

	//3. listen
	if (listen(server_fd, 5) < 0)
	{
		perror("Listen: ");
		return -1;
	}

	//4. accept
	struct sockaddr_in client_address;				 //we to pass this to accept method to get client info
	int client_address_len = sizeof(client_address); // accept also needs client_address length

	char client_name[50];

	fd_set readyfd;

	while (1)
	{
		// Clearing the sock set
		FD_ZERO(&readyfd);

		// Adding master socket to set
		FD_SET(server_fd, &readyfd);
		int max_fd = server_fd;

		// Adding child sockets to set
		for (int i = 0; i < MAX_SOCKETS; i++)
		{
			if (sock_array[i] > 0)
			{
				FD_SET(sock_array[i], &readyfd);
			}
			if (sock_array[i] > max_fd)
			{
				max_fd = sock_array[i];
			}
		}

		if (select(max_fd + 1, &readyfd, NULL, NULL, NULL) < 0)
		{
			perror("Select\n");
			return -1;
		}

		for (int i = 0; i <= max_fd; i++)
		{
			if (FD_ISSET(i, &readyfd))
			{
				if (i == server_fd)
				{
					int client_fd = accept(server_fd, NULL, NULL);
					FD_SET(client_fd, &readyfd);
					if (client_fd > max_fd)
						max_fd = client_fd;
					for (int i = 0; i < MAX_SOCKETS; i++)
					{
						if (sock_array[i] == 0)
						{
							sock_array[i] = client_fd;
							printf("New client connected on socket %d\n",
								   client_fd);
							break;
						}
					}
					break;
				}
				else
				{
					serve_client(i, auth_users, sock_array);
					FD_CLR(i, &readyfd);
				}
			}
		}
	}

	close(server_fd);
}

/*
 * This function serves the client passed in as client_fd.
 */
void serve_client(int client_fd, struct serverUser *auth_users, int *sock_array)
{

	bool user_authenticated = false;

	for (int i = 0; i < NUM_USERS; i++)
	{
		if (auth_users[i].sock_num == client_fd && auth_users[i].state == 2)
		{
			user_authenticated = true;
		}
	}

	char request[128];
	memset(request, 0, sizeof(request));

	char response[256];
	memset(response, 0, sizeof(response));

	char delim[] = " ";

	if (recv(client_fd, request, sizeof(request) - 1, 0) > 0)
	{
		char *command = strtok(request, delim);

		if (strcmp(command, "USER") == 0)
		{
			if (user_authenticated)
			{
				strcat(response, "User already logged in.");
				send(client_fd, response, strlen(response), 0);
				printf("User %s already logged in.\n", command);
			}

			command = strtok(NULL, delim);

			printf("User %s attempting to authenticate.\n", command);

			bool found = false;

			for (int i = 0; i < NUM_USERS; i++)
			{
				if (strcmp(auth_users[i].userName, command) == 0)
				{
					if (auth_users[i].state == 0)
					{
						strcat(response, "Username OK, password required!");
						auth_users[i].state = 1;
						auth_users[i].sock_num = client_fd;
						send(client_fd, response, strlen(response), 0);
						printf("User %s exists. Waiting for PASS.\n", command);
					}
					else
					{
						strcat(response, "User already logged in.");
						send(client_fd, response, strlen(response), 0);
						printf("User %s already logged in.\n", command);
					}
					found = true;
				}
			}

			if (!found)
			{
				strcat(response, "Username does not exist!");
				send(client_fd, response, strlen(response), 0);
				printf("User %s does not exist.\n", command);
			}
		}
		else if (strcmp(command, "PASS") == 0)
		{
			if (user_authenticated)
			{
				strcat(response, "User already logged in.");
				send(client_fd, response, strlen(response), 0);
				printf("User %s already logged in.\n", command);
			}

			command = strtok(NULL, delim);

			for (int i = 0; i < NUM_USERS; i++)
			{
				if (auth_users[i].sock_num == client_fd)
				{
					if (strcmp(command, auth_users[i].password) == 0)
					{
						strcat(response, "Authentication complete!");
						send(client_fd, response, strlen(response), 0);
						printf("User %s authenticated.\n",
							   auth_users[i].userName);
						auth_users[i].state = 2;
					}
					else
					{
						strcat(response, "Incorrect password!");
						send(client_fd, response, strlen(response), 0);
						printf("User %s gave incorrect password.\n",
							   auth_users[i].userName);
					}
				}
			}
		}
		else if (user_authenticated)
		{
			if (strcmp(command, "PUT") == 0)
			{
				//forking

				command = strtok(NULL, delim);

				printf("File %s is pending to be transfered.\n", command);

				int put_server_fd = socket(AF_INET, SOCK_STREAM, 0);
				if (put_server_fd < 0)
				{
					perror("Socket: ");
					return;
				}

				struct sockaddr_in put_server_address;
				memset(&put_server_address, 0, sizeof(put_server_address));

				//generate rand port number between 1,024–49,151
				int port_num = (rand() % (49151 - 1024 + 1)) + 1024;
				// printf("port num is %i\n", port_num);

				send(client_fd, &port_num, sizeof(port_num), 0);

				put_server_address.sin_family = AF_INET;
				put_server_address.sin_port = htons(port_num);
				put_server_address.sin_addr.s_addr = htonl(INADDR_ANY);
				printf("Opening port %i for file transfer. \n", put_server_address.sin_port);

				//2. bind
				if (bind(put_server_fd, (struct sockaddr *)&put_server_address, sizeof(put_server_address)) < 0)
				{
					perror("Bind: ");
					return;
				}
				//3. listen
				if (listen(put_server_fd, 2) < 0)
				{
					perror("Listen: ");
					return;
				}
				//4. accept
				struct sockaddr_in put_client_address;				 //we to pass this to accept method to get client info
				int client_address_len = sizeof(put_client_address); // accept also needs client_address length

				char put_client_name[50];

				int put_client_fd = accept(put_server_fd, (struct sockaddr *)&put_client_address, (socklen_t *)&client_address_len);
				char message[100];
				inet_ntop(AF_INET, &put_client_address.sin_addr, put_client_name, sizeof(put_client_name));

				if (put_client_fd < 0)
				{
					perror("Accept: ");
					return;
				}

				int pid = fork();
				if (pid == 0)
				{

					char filename[50];
					int file_size = 0;
					char server_file[50];
					strcpy(server_file, "Server-");
					strcat(server_file, command);

					printf("Serverfile : %s \n", server_file);
					strcpy(filename, server_file);

					recv(put_client_fd, &file_size, sizeof(file_size), 0);

					printf("Creating a file : %s \n", filename);
					FILE *file;
					if (!(file = fopen(filename, "a")))
					{
						perror("Sorry, this file can't be created.");
						return;
					}
					else
					{

						char line[256];
						memset(line, 0, sizeof(line));

						int ctr = 0;

						while (recv(put_client_fd, line, sizeof(line), 0) >= 0)
						{

							int write_file = fwrite(line, 1, sizeof(line), file);

							if (write_file < 0)
							{
								perror("Error when writing into the file. Try again.\n");
								return;
							}

							ctr += sizeof(line);

							if (ctr >= file_size)
								break;
							memset(line, 0, sizeof(line));
						}

						fclose(file);
					}

					fflush(stdout);

					strcat(response, "Transfer of the file done.");
					send(put_client_fd, response, strlen(response), 0);

					printf("PUT function completed.\n");
					send(put_client_fd, "PUT function completed. \n", strlen("PUT function completed. \n"), 0);

					close(put_client_fd);
				}

				close(put_server_fd);
				return;

			} //ending PUT

			// staring GET function
			else if (strcmp(command, "GET") == 0)
			{
				command[strcspn(command, "\n")] = 0;

				char filename[50];
				command = strtok(NULL, delim);
				strcpy(filename, command);

				printf("Requesting file : %s \n", filename);
				FILE *file;
				if (!(file = fopen(filename, "r")))
				{
					strcpy(response, "Nonexistent");
					send(client_fd, response, sizeof(response), 0);
					perror("File does not exist.\n");
				}
				else
				{
					memset(response, 0, sizeof(response));
					strcpy(response, "Existent");
					send(client_fd, response, sizeof(response), 0);

					int get_server_fd = socket(AF_INET, SOCK_STREAM, 0);
					if (get_server_fd < 0)
					{
						perror("Socket: ");
						return;
					}

					struct sockaddr_in get_server_address;
					memset(&get_server_address, 0, sizeof(get_server_address));

					//generate rand port number between 1,024–49,151
					int port_num = (rand() % (49151 - 1024 + 1)) + 1024;
					// printf("port num is %i\n", port_num);

					send(client_fd, &port_num, sizeof(port_num), 0);

					get_server_address.sin_family = AF_INET;
					get_server_address.sin_port = htons(port_num);
					get_server_address.sin_addr.s_addr = htonl(INADDR_ANY);
					printf("Opening port %i for file transfer. \n", get_server_address.sin_port);

					//2. bind
					if (bind(get_server_fd, (struct sockaddr *)&get_server_address, sizeof(get_server_address)) < 0)
					{
						perror("Bind: ");
						return;
					}
                    printf("here1\n");
					//3. listen
					if (listen(get_server_fd, 2) < 0)
					{
						perror("Listen: ");
						return;
					}
					printf("here2\n");
					//4. accept
					struct sockaddr_in get_client_address;				 //we to pass this to accept method to get client info
					int client_address_len = sizeof(get_client_address); // accept also needs client_address length

					char get_client_name[50];

					int get_client_fd = accept(get_server_fd, (struct sockaddr *)&get_client_address, (socklen_t *)&client_address_len);
					char message[100];
					inet_ntop(AF_INET, &get_client_address.sin_addr, get_client_name, sizeof(get_client_name));

					printf("here3\n");

					if (get_client_fd < 0)
					{
						perror("Accept: ");
						return;
					}

					int pid = fork();
					if (pid == 0)
					{
						printf("here4\n");
						char line[256];
						while (fgets(line, sizeof(line), file) != NULL) //read the file until NULL
						{
							if (send(get_client_fd, line, sizeof(line), 0) == -1) //send the server response to the client
							{
								perror("Error Sending file..\n");
								break;
							}
							memset(line, 0, sizeof(line));
						}
						printf("here5\n");
						fclose(file);

						//4. close
						// recv(get_client_fd, response, sizeof(response), 0);
						// printf("%s\n", response);
						// printf("here6\n");

						close(get_client_fd);
					}

					close(get_server_fd);
					return;
				}
			}

			// starting LS function
			else if (strcmp(command, "ls") == 0)
			{
				char line[128];

				FILE *file = popen(request, "r");
				if (file)
				{
					while (fgets(line, sizeof(line), file))
					{
						strcat(response, line);
					}
					pclose(file);
				}

				send(client_fd, response, strlen(response), 0);
			}
			else if (strcmp(command, "pwd") == 0)
			{
				char line[128];

				FILE *file = popen(request, "r");
				if (file)
				{
					while (fgets(line, sizeof(line), file))
					{
						strcat(response, line);
					}
					pclose(file);
				}

				send(client_fd, response, strlen(response), 0);
			}
			else
			{
				strcat(response, "Authenticate first.");
				send(client_fd, response, strlen(response), 0);
			}
		}

		else if (strcmp(command, "QUIT") == 0 || strcmp(command, "quit") == 0)
		{
			close_client(client_fd, auth_users, sock_array);
		}
		else
		{
			strcat(response, "Authenticate first.");
			send(client_fd, response, strlen(response), 0);
		}
	}
	else
	{
		close_client(client_fd, auth_users, sock_array);
	}

	memset(request, 0, sizeof(request));
	memset(response, 0, sizeof(response));

	return;
}

void close_client(int client_fd, struct serverUser *auth_users, int *sock_array)
{
	close(client_fd);
	for (int i = 0; i < MAX_SOCKETS; i++)
	{
		if (sock_array[i] == client_fd)
		{
			sock_array[i] = 0;
		}
	}
	for (int i = 0; i < NUM_USERS; i++)
	{
		if (auth_users[i].sock_num == client_fd)
		{
			auth_users[i].state = 0;
			auth_users[i].sock_num = 0;
			printf("User %s disconnected.\n", auth_users[i].userName);
		}
	}
}
