#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdbool.h> 
#include <stdlib.h>

int MAX_COMMAND_SIZE = 100;
bool is_valid_ip(char* ipString);

int main (int argc, char *argv[]) 
{
    // ----- Dealing with arguments -----
    
    if (argc != 3) {
        printf("Incorrect number of arguments. 3 needed.\n");
        return 0;
    } else if (strlen(argv[1]) > 16) {
        printf("Argument 1 incorrect. Must be 16 or fewer characters.\n");
        return 0; 
    } else if (strlen(argv[2]) > 5) {
        printf("Argument 2 incorrect. Must be 5 or fewer characters.\n");
        return 0; 
    }
    
    char server_ip_address[16];
    char server_port_number[5];

    strcpy(server_ip_address, argv[1]);
    strcpy(server_port_number, argv[2]);

    if (!is_valid_ip(server_ip_address)) {
        printf("Invalid ip address.\n");
        return 0;
    }

    int server_port = atoi(server_port_number);

    if (server_port <= 1024) {
        printf("Port number too low.");
        return 0;
    }

    // ----- Opening socket -----

    int server_fd = socket(AF_INET,SOCK_STREAM,0);
	if(server_fd<0)
	{
		perror("Socket: ");
		return (-1);
	}

    struct sockaddr_in server_address;
	memset(&server_address,0,sizeof(server_address));

	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(server_port);
	inet_pton(AF_INET, server_ip_address, &(server_address.sin_addr));

    //2 connect
	if(connect(server_fd,(struct sockaddr*)&server_address,sizeof(server_address))<0)
	{
		perror("Could not connect: ");
		return -1;
	}

    printf("Connected to %s\n", server_ip_address);

    // ----- Shell loop -----

    char command[MAX_COMMAND_SIZE];
    char response[100];	//string to hold the server esponse

    while (1) {
        memset(response,0,sizeof(response));
        printf("ftp> ");
		fgets(command, MAX_COMMAND_SIZE, stdin); //more safe but has no \n at the end,
		command[strcspn(command,"\n")]=0; // adding the \n

        if (!strncmp(command, "QUIT", 4) || !strncmp(command, "quit", 4)) {
            send(server_fd,"QUIT",4,0);
            close(server_fd);
            break;
        }

        else if (!strncmp(command, "USER", 4)) {
            command[strcspn(command,"\n")]=0;
            send(server_fd,command,strlen(command),0);
            recv(server_fd,response,sizeof(response),0);
            printf("%s\n", response);
        }

        else if (!strncmp(command, "PASS", 4)) {
            command[strcspn(command,"\n")]=0;
            send(server_fd,command,strlen(command),0);
            recv(server_fd,response,sizeof(response),0);
            printf("%s\n", response);
        }

        else if (!strncmp(command, "PUT", 3)) {
            printf("PUT command will be supported in a later release.\n");
        }

        else if (!strncmp(command, "GET", 3)) {
            printf("GET command will be supported in a later release.\n");
        } 
        
        else if(!strncmp(command, "CD", 2) 
        || !strncmp(command, "LS", 2) 
        || !strncmp(command, "PWD", 3)) {
            printf("This command will be supported in a later release.\n");
        } 
        
        else if(!strncmp(command, "!LS", 3) 
        || !strncmp(command, "!PWD", 4)) {
            printf("This command will be supported in a later release.\n");
        }

        else {
            printf("Invalid ftp command\n"); 
        }
    }

    return 0;
}

/*
 * This custom function makes sure that the string IP address given in the argu-
 * ment is in the valid format (four numbers between 0 and 256, with a 0 in 
 * between).
 */
bool is_valid_ip(char* ipString) {
    int string_size = strlen(ipString);

    if (string_size < 7) return false;

    int counter = 0;
    for (int i = 0; i < string_size; i++)
    {
        if (ipString[i] == '.') counter++;
    }
    if (counter != 3) return false;

    char validChars[] = "0123456789.";
    int validCharsSize = strlen(validChars);
    bool valid;

    for (int i = 0; i < string_size; i++)
    {   
        valid = false;
        for (int j = 0; j < validCharsSize; j++)
        {
            if (ipString[i] == validChars[j]) {
                valid = true;
                break;
            }
        }
        if (!valid) return false;
    }

    for (int i = 0; i < string_size; i++)
    {
        if (ipString[i] == '.') {
            if (i == 0 || i == string_size-1) return false;
            if (ipString[i-1] == '.' || ipString[i-1] == '.') return false;
        }
    }

    return true;
}