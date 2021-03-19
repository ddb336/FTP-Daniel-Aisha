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
bool is_valid_ip(char *ipString);

int main(int argc, char *argv[])
{
    // ----- Dealing with arguments -----

    if (argc != 3)
    {
        printf("Incorrect number of arguments. 3 needed.\n");
        return 0;
    }
    else if (strlen(argv[1]) > 16)
    {
        printf("Argument 1 incorrect. Must be 16 or fewer characters.\n");
        return 0;
    }
    else if (strlen(argv[2]) > 5)
    {
        printf("Argument 2 incorrect. Must be 5 or fewer characters.\n");
        return 0;
    }

    char server_ip_address[16];
    char server_port_number[5];

    strcpy(server_ip_address, argv[1]);
    strcpy(server_port_number, argv[2]);

    if (!is_valid_ip(server_ip_address))
    {
        printf("Invalid ip address.\n");
        return 0;
    }

    int server_port = atoi(server_port_number);

    if (server_port <= 1024)
    {
        printf("Port number too low.");
        return 0;
    }

    // ----- Opening socket -----

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        perror("Socket: ");
        return (-1);
    }

    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(server_port);
    inet_pton(AF_INET, server_ip_address, &(server_address.sin_addr));

    //2 connect
    if (connect(server_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        perror("Could not connect: ");
        return -1;
    }

    printf("Connected to %s\n", server_ip_address);

    // ----- Shell loop -----

    char command[MAX_COMMAND_SIZE];
    char response[256]; //string to hold the server esponse

    while (1)
    {
        memset(response, 0, sizeof(response));

        printf("ftp> ");
        fgets(command, MAX_COMMAND_SIZE, stdin); //more safe but has no \n at the end,
        command[strcspn(command, "\n")] = 0;     // adding the \n

        if (!strncmp(command, "QUIT", 4) || !strncmp(command, "quit", 4))
        {
            send(server_fd, "QUIT", 4, 0);
            close(server_fd);
            break;
        }

        else if (!strncmp(command, "USER", 4))
        {
            command[strcspn(command, "\n")] = 0;
            send(server_fd, command, strlen(command), 0);
            recv(server_fd, response, sizeof(response), 0);
            printf("%s\n", response);
        }

        else if (!strncmp(command, "PASS", 4))
        {
            command[strcspn(command, "\n")] = 0;
            send(server_fd, command, strlen(command), 0);
            recv(server_fd, response, sizeof(response), 0);
            printf("%s\n", response);
        }

        else if (!strncmp(command, "PUT", 3))
        {
            command[strcspn(command, "\n")] = 0;

            char new_command[20];
            strcpy(new_command, command);

            char delim[] = " ";
            char *ptr = strtok(new_command, delim);

            char filename[50];

            ptr = strtok(NULL, delim);
            strcpy(filename, ptr);

            FILE *file;
            if (!(file = fopen(filename, "r")))
            {
                perror("File can't be opened...");
                continue;
            }
            else
            {
                memset(response,0,sizeof(response));
                send(server_fd, command, sizeof(command), 0);
                recv(server_fd, response, sizeof(response), 0);

                if (!strcmp(response, "Ready for put!"))
                {
                    printf("Sending file : %s \n", filename);

                    int put_server_fd = socket(AF_INET, SOCK_STREAM, 0);

                    if (put_server_fd < 0)
                    {
                        perror("Socket: ");
                        return -1;
                    }

                    struct sockaddr_in put_server_address;
                    memset(&put_server_address, 0, sizeof(put_server_address));

                    int port_num;
                    recv(server_fd, &port_num, sizeof(port_num), 0);

                    put_server_address.sin_family = AF_INET;
                    put_server_address.sin_port = htons(port_num);
                    put_server_address.sin_addr.s_addr = htonl(INADDR_ANY);
                    printf("Connecting to port %i for file transfer.\n", put_server_address.sin_port);

                    //2 connect
                    if (connect(put_server_fd, (struct sockaddr *)&put_server_address, sizeof(put_server_address)) < 0)
                    {
                        perror("Connect :");

                        return -1;
                    }

                    // printf("File Server port %i \n", put_server_address.sin_port);
                    char message[100];

                    char line[256];
                    while (fgets(line, sizeof(line), file) != NULL) //read the file until NULL
                    {
                        
                        if (send(put_server_fd, line, sizeof(line), 0) == -1) //send the server response to the client
                        {
                            perror("Error Sending file..\n");
                            break;
                        }
                        memset(line, 0, sizeof(line));
                    }

                    fclose(file);

                    printf("PUT function completed.\n");

                    close(put_server_fd);
                }
                else
                {
                    printf("%s", response);
                }
            }
        }

        else if (!strncmp(command, "GET", 3))
        {
            command[strcspn(command, "\n")] = 0;
            send(server_fd, command, strlen(command), 0);
            memset(response, 0, sizeof(response));
            recv(server_fd, response, sizeof(response), 0);
            if (strcmp(response, "Nonexistent") == 0)
            {
                printf("There's no such file on the server.\n");
            }
            else if (strcmp(response, "Existent") == 0)
            {

                printf("The transfer of the file from server starts...\n");

                int get_server_fd = socket(AF_INET, SOCK_STREAM, 0);

                if (get_server_fd < 0)
                {
                    perror("Socket: ");
                    return -1;
                }

                struct sockaddr_in get_server_address;
                memset(&get_server_address, 0, sizeof(get_server_address));

                int port_num;
                recv(server_fd, &port_num, sizeof(port_num), 0);

                get_server_address.sin_family = AF_INET;
                get_server_address.sin_port = htons(port_num);
                get_server_address.sin_addr.s_addr = htonl(INADDR_ANY);
                printf("Connecting to port %i for file transfer.\n", get_server_address.sin_port);

                //2 connect
                if (connect(get_server_fd, (struct sockaddr *)&get_server_address, sizeof(get_server_address)) < 0)
                {
                    perror("Connect :");

                    return -1;
                }

                // GETTING file from the server
                char delim[] = " ";
                char *filenm = strtok(command, delim);
                filenm = strtok(NULL, delim);

                char filename[50];
                int file_size = 0;
                char client_file[50];

                strcpy(client_file, "Client-");
                strcat(client_file, filenm);

                FILE *file;
                if (!(file = fopen(client_file, "w")))
                {
                    perror("Sorry, this file can't be created.");
                    return 0;
                }
                else
                {
                    char message[256];

                    int myreturn = 0;

                    memset(message, 0, sizeof(message));
                    while ((myreturn = recv(get_server_fd, message, sizeof(message), 0)) > 0)
                    {
                        fputs(message, file);
                        memset(message, 0, sizeof(message));
                    }

                    fclose(file);
                }

                fflush(stdout);

                // char response[100];

                // strcat(response, "Transfer of the file done. Port closed. \n");
                // send(get_server_fd, response, strlen(response), 0);

                printf("Transfer of the file %s done. New file is saved as %s. \nPort closed. \n", filenm, client_file);

                close(get_server_fd);
            }
            else
            {
                printf("%s", response);
            }

            fflush(stdout);
        }

        else if (!strncmp(command, "CD", 2) || !strncmp(command, "LS", 2) || !strncmp(command, "PWD", 3))
        {
            command[strcspn(command, "\n")] = 0;
            send(server_fd, command, strlen(command), 0);
            recv(server_fd, response, sizeof(response), 0);
            fflush(stdout);
            printf("%s", response);
        }

        else if (!strncmp(command, "!LS", 3))
        {
            system("ls");
        }

        else if (!strncmp(command, "!PWD", 4))
        {
            system("pwd");
        }

        else if (!strncmp(command, "!CD", 3))
        {
            char delim[] = " ";
            char *token = strtok(command, delim);
            token = strtok(NULL, delim);
            if (chdir(token) != 0)
                printf("chdir() to %s failed\n", token);
        }

        else
        {
            printf("Invalid ftp command\n");
        }
        fflush(stdout);
    }

    return 0;
}

/*
 * This custom function makes sure that the string IP address given in the argu-
 * ment is in the valid format (four numbers between 0 and 256, with a 0 in 
 * between).
 */
bool is_valid_ip(char *ipString)
{
    int string_size = strlen(ipString);

    if (string_size < 7)
        return false;

    int counter = 0;
    for (int i = 0; i < string_size; i++)
    {
        if (ipString[i] == '.')
            counter++;
    }
    if (counter != 3)
        return false;

    char validChars[] = "0123456789.";
    int validCharsSize = strlen(validChars);
    bool valid;

    for (int i = 0; i < string_size; i++)
    {
        valid = false;
        for (int j = 0; j < validCharsSize; j++)
        {
            if (ipString[i] == validChars[j])
            {
                valid = true;
                break;
            }
        }
        if (!valid)
            return false;
    }

    for (int i = 0; i < string_size; i++)
    {
        if (ipString[i] == '.')
        {
            if (i == 0 || i == string_size - 1)
                return false;
            if (ipString[i - 1] == '.' || ipString[i - 1] == '.')
                return false;
        }
    }

    return true;
}