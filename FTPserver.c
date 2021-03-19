#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#define NUM_USERS 3
#define MAX_SOCKETS 1024

/*
 * The serverUser struct saves information about a user.
 * Usernames and passwords are stored in a serveruser array in the server.
 * The struct saves their username and password
 * It also saves their state (logged out, username entered, logged in)
 * Finally it stores the socket, so that the server can remember that a 
 * particular socket is logged in
 */
struct serverUser
{
	char *userName;
	char *password;
	int state; // 0 is not yet userrequested, 1 is userrequested (waiting for
			   // pass), 2 is authenticated
	int sock_num;
};

/*
 * The serve client function deals with requests from clients
 */
void serve_client(int client_sd,
				  struct serverUser *auth_users,
				  int *sock_array);

/*
 * The close client function closes a client socket and removes their socket 
 * number from the serveruser array
 */
void close_client(int client_fd,
				  struct serverUser *auth_users,
				  int *sock_array);

int main()
{
	/*
	 * Here we save the user data, setting the username, password, socket number 
	 * and state for each authorized user.
	 */

	struct serverUser auth_users[NUM_USERS];

	auth_users[0].userName = "Aisha";
	auth_users[0].password = "AHPass";
	auth_users[0].state = 0;
	auth_users[0].sock_num = 0;

	auth_users[1].userName = "Daniel";
	auth_users[1].password = "DBPass";
	auth_users[1].state = 0;
	auth_users[1].sock_num = 0;

	auth_users[2].userName = "Khalid";
	auth_users[2].password = "KMPass";
	auth_users[2].state = 0;
	auth_users[2].sock_num = 0;

	/* ---------------------------------------------------------------------- */

	// Creating the server master socket
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0)
	{
		perror("Socket: ");
		return (-1);
	}

	// Creating the array of sockets that will be used for the select() function
	int sock_array[MAX_SOCKETS] = {0};

	// Saving the structure for the port that we would like to bind to
	struct sockaddr_in server_address;
	memset(&server_address, 0, sizeof(server_address));

	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(9000);
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);

	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1},
				   sizeof(int)) < 0)
	{
		perror("Setsockopt:");
		return -1;
	}

	// Binding to the port
	if (bind(server_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
	{
		perror("Bind: ");
		return -1;
	}

	// Listening on the port
	if (listen(server_fd, 5) < 0)
	{
		perror("Listen: ");
		return -1;
	}

	// The socket set that we will use for the select function
	fd_set readyfd;

	while (1)
	{
		// Clearing the socket set so we can add the sockets back to it
		FD_ZERO(&readyfd);

		// Adding main server socket to set
		FD_SET(server_fd, &readyfd);
		int max_fd = server_fd;

		// Adding child sockets to set
		for (int i = 0; i < MAX_SOCKETS; i++)
		{
			// If the socket is valid (non-zero, the array is initialized to 0),
			// we add it to the set
			if (sock_array[i] > 0)
			{
				FD_SET(sock_array[i], &readyfd);
			}
			// Getting the maximum socket for max_fd, so we can save new sockets
			if (sock_array[i] > max_fd)
			{
				max_fd = sock_array[i];
			}
		}

		// Waiting on the select function, this will go through when there's a 
		// request on one of the sockets
		if (select(max_fd + 1, &readyfd, NULL, NULL, NULL) < 0)
		{
			perror("Select\n");
			return -1;
		}

		// Go through the sockets and handle the request
		for (int i = 0; i <= max_fd; i++)
		{
			// If it's the socket chosen by select()
			if (FD_ISSET(i, &readyfd))
			{	
				// If it's a new connect 
				if (i == server_fd)
				{	
					// Accept the connection
					int client_fd = accept(server_fd, NULL, NULL);
					FD_SET(client_fd, &readyfd);
					if (client_fd > max_fd)
						max_fd = client_fd;
					// Set the first 0 in the socket array to the new socket
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
					// Otherwise serve the client
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
 * It takes the main client socket descriptor, the array of users, and the array
 * of sockets.
 * It will implement the outcome of various functionalities that the client 
 * requests.
 */
void serve_client(int client_fd, struct serverUser *auth_users, int *sock_array)
{
	/*
	 * Checking first if the user is authenticated.
	 */
	bool user_authenticated = false;
	for (int i = 0; i < NUM_USERS; i++)
	{	
		// If our connected socket corresponds to a logged in user
		if (auth_users[i].sock_num == client_fd && auth_users[i].state == 2)
		{
			// We are authenticated
			user_authenticated = true;
		}
	}

	// Setting a request string
	char request[256];
	memset(request, 0, sizeof(request));

	// Setting a response string
	char response[256];
	memset(response, 0, sizeof(response));

	// Delimiter for getting substrings
	char delim[] = " ";

	// If we have recieved a message from the client
	if (recv(client_fd, request, sizeof(request) - 1, 0) > 0)
	{
		// Get the command portion of the request 
		char *command = strtok(request, delim);

		// If they are not authenticated
		if (!user_authenticated)
		{
			// Handle the USER command case
			if (strcmp(command, "USER") == 0)
			{
				// Get the username
				command = strtok(NULL, delim);

				printf("User %s attempting to authenticate.\n", command);

				// Try to find the user
				// Pass through all users
				for (int i = 0; i < NUM_USERS; i++)
				{
					// If the username is correct
					if (strcmp(auth_users[i].userName, command) == 0)
					{
						// If they are not yet logged in
						if (auth_users[i].state != 2)
						{
							// Send the response telling that the username 
							// is good
							strcat(response, 
									"331 Username OK, password required!");
							send(client_fd, response, strlen(response), 0);
							// Set the state of the user to 1 (username entered)
							auth_users[i].state = 1;
							// Bind the user to this specific socket
							auth_users[i].sock_num = client_fd;
							printf("User %s exists. Waiting for PASS.\n", command);
						}
						// Otherwise, tell them the user is already logged in
						else
						{
							strcat(response, "User already logged in.");
							send(client_fd, response, strlen(response), 0);
							printf("User %s already logged in.\n", command);
						}
						return;
					}
				}
				// If we never find the specific user, tell the client that the 
				// username given does not exist
				strcat(response, "Username does not exist!");
				send(client_fd, response, strlen(response), 0);
				printf("User %s does not exist.\n", command);
			}
			// Handle the password command
			else if (strcmp(command, "PASS") == 0)
			{
				// Get the password from the command
				command = strtok(NULL, delim);
				
				// Go through all the users
				for (int i = 0; i < NUM_USERS; i++)
				{	
					// Check if their socket number matches the socket that 
					// entered the password
					if (auth_users[i].sock_num == client_fd)
					{	
						// If it matches their password
						if (strcmp(command, auth_users[i].password) == 0)
						{
							// Set them to authenticated
							strcat(response, "Authentication complete!");
							send(client_fd, response, strlen(response), 0);
							printf("User %s authenticated.\n",
								   auth_users[i].userName);
							auth_users[i].state = 2;
						}
						else
						{
							// Otherwise, tell them they entered it wrong
							strcat(response, "Incorrect password!");
							send(client_fd, response, strlen(response), 0);
							printf("User %s gave incorrect password.\n",
								   auth_users[i].userName);
						}
					}
				}
			}
			// If they are not authenticated and they say quit
			else if (strcmp(command, "QUIT") == 0 || strcmp(command, "quit") == 0)
			{
				close_client(client_fd, auth_users, sock_array);
			}
			// Tell them to authenticate
			else
			{
				strcat(response, "Authenticate first.\n");
				send(client_fd, response, strlen(response), 0);
			}
		}
		// If the user is authenticated
		else 
		{	
			// The PUT command
			if (strcmp(command, "PUT") == 0)
			{
				// Respond to USER that we are ready for PUT
				// This effectively means that they are authenticated
				memset(response, 0, sizeof(response));
				strcat(response, "Ready for put!");
				send(client_fd, response, sizeof(response), 0);

				// Forking to receive the data from the user
				int pid = fork();
				if (pid == 0)
				{
					// Getting the file name
					command = strtok(NULL, delim);

					printf("File %s is pending to be transfered.\n", command);

					// Open a new socket
					int put_server_fd = socket(AF_INET, SOCK_STREAM, 0);
					if (put_server_fd < 0)
					{
						perror("Socket: ");
						return;
					}

					// Get all the details together for the port we are 
					// connecting to
					struct sockaddr_in put_server_address;
					memset(&put_server_address, 0, sizeof(put_server_address));

					put_server_address.sin_family = AF_INET;
					// Connecting to socket 0 will allow us to extract an open 
					// port later
					put_server_address.sin_port = htons(0);
					put_server_address.sin_addr.s_addr = htonl(INADDR_ANY);

					// Bind to the port
					if (bind(put_server_fd, 
							(struct sockaddr *)&put_server_address, 
							sizeof(put_server_address)) < 0)
					{
						perror("Bind: ");
						return;
					}

					// We can extract the port
					socklen_t len = sizeof(put_server_address);
					if (getsockname(put_server_fd, 
						(struct sockaddr *)&put_server_address, &len) == -1)
					{
						perror("getsockname");
						return;
					}
					
					// Listen for connections
					if (listen(put_server_fd, 2) < 0)
					{
						perror("Listen: ");
						return;
					}

					// Getting our port number
					int port_num = ntohs(put_server_address.sin_port);
					printf("Connecting to port %i for file transfer.\n", htons(port_num));

					// Sending the port number to the client
					send(client_fd, &port_num, sizeof(port_num), 0);

					// We to pass this to accept method to get client info
					struct sockaddr_in put_client_address;		
					// Accept also needs client_address length		 
					int client_address_len = sizeof(put_client_address); 

					char put_client_name[50];

					// Accepting the connection
					int put_client_fd = accept(put_server_fd, 
					(struct sockaddr *)&put_client_address, 
					(socklen_t *)&client_address_len);

					char message[100];
					inet_ntop(AF_INET, &put_client_address.sin_addr, 
							  put_client_name, sizeof(put_client_name));

					// If there were any errors accepting
					if (put_client_fd < 0)
					{
						perror("Accept: ");
						return;
					}

					// Storing the filename we're gonna save
					char filename[50];
					char server_file[50];
					strcpy(server_file, "Server-");
					strcat(server_file, command);

					printf("Serverfile : %s \n", server_file);
					strcpy(filename, server_file);

					// Create the file on the server side
					printf("Creating a file : %s \n", filename);
					FILE *file;
					if (!(file = fopen(filename, "w")))
					{
						perror("Sorry, this file can't be created.");
						return;
					}
					else
					{
						char message[256];

						// Save the file
						memset(message, 0, sizeof(message));
						while (recv(put_client_fd, message, sizeof(message), 0) > 0)
						{
							fputs(message, file);
							memset(message, 0, sizeof(message));
						}
						
						// Close the file
						fclose(file);

						printf("PUT function completed.\n");
					}

					// Close the ports 
					close(put_client_fd);
					close(put_server_fd);

					// Close this process
					exit(0);
				}

				return;
			}

			// Implementing the GET function
			else if (strcmp(command, "GET") == 0)
			{
				// Getting the file name
				command[strcspn(command, "\n")] = 0;
				char filename[50];
				command = strtok(NULL, delim);
				strcpy(filename, command);

				printf("Requesting file : %s \n", filename);
				FILE *file;

				// Check first if the file exists
				if (!(file = fopen(filename, "r")))
				{
					// Tell client if it doesn't exist
					strcpy(response, "Nonexistent");
					send(client_fd, response, sizeof(response), 0);
					perror("File does not exist.\n");
				}
				else
				{	
					// Open a new process over which we will transfer
					int pid = fork();
					if (pid == 0)
					{
						// Tell client that the file exists
						memset(response, 0, sizeof(response));
						strcpy(response, "Existent");
						send(client_fd, response, sizeof(response), 0);

						// Open new socket
						int get_server_fd = socket(AF_INET, SOCK_STREAM, 0);
						if (get_server_fd < 0)
						{
							perror("Socket: ");
							return;
						}

						// Get the port 
						struct sockaddr_in get_server_address;
						memset(&get_server_address, 0, sizeof(get_server_address));

						get_server_address.sin_family = AF_INET;
						get_server_address.sin_port = htons(0);
						get_server_address.sin_addr.s_addr = htonl(INADDR_ANY);

						if (bind(get_server_fd, (struct sockaddr *)&get_server_address, sizeof(get_server_address)) < 0)
						{
							perror("Bind: ");
							return;
						}

						socklen_t len = sizeof(get_server_address);
						if (getsockname(get_server_fd, (struct sockaddr *)&get_server_address, &len) == -1)
						{
							perror("getsockname");
							return;
						}

						if (listen(get_server_fd, 2) < 0)
						{
							perror("Listen: ");
							return;
						}

						int port_num = ntohs(get_server_address.sin_port);
						printf("Connecting to port %i for file transfer.\n", htons(port_num));

						// sending the port number to the client
						send(client_fd, &port_num, sizeof(port_num), 0);

						//2. bind

						// finding the available port number to send to the client

						//4. accept
						struct sockaddr_in get_client_address;				 //we to pass this to accept method to get client info
						int client_address_len = sizeof(get_client_address); // accept also needs client_address length

						char get_client_name[50];

						int get_client_fd = accept(get_server_fd, (struct sockaddr *)&get_client_address, (socklen_t *)&client_address_len);
						char message[100];
						inet_ntop(AF_INET, &get_client_address.sin_addr, get_client_name, sizeof(get_client_name));

						if (get_client_fd < 0)
						{
							perror("Accept: ");
							return;
						}

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

						fclose(file);

						close(get_client_fd);
						close(get_server_fd);

						exit(0);
					}

					return;
				}
			}

			// starting LS function
			else if (strcmp(command, "LS") == 0)
			{
				char line[128];

				FILE *file = popen("ls", "r");
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
			else if (strcmp(command, "PWD") == 0)
			{
				char line[128];

				FILE *file = popen("pwd", "r");
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
			else if (strcmp(command, "CD") == 0)
			{
				char *token = strtok(NULL, delim);
				if (chdir(token) != 0)
				{
					strcat(response, "Directory change unsuccessful.\n");
					send(client_fd, response, strlen(response), 0);
				}
				else
				{
					strcat(response, "Successfully changed directory.\n");
					send(client_fd, response, strlen(response), 0);
				}
			}
			else if (strcmp(command, "QUIT") == 0 || strcmp(command, "quit") == 0)
			{
				close_client(client_fd, auth_users, sock_array);
			}
			else
			{
				strcat(response, "Authenticate first.\n");
				send(client_fd, response, strlen(response), 0);
			}
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
