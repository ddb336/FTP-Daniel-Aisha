#include<stdio.h>
#include<string.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<sys/types.h>
 #include <unistd.h>
#include<netinet/in.h>
#include<stdlib.h>


void serve_client(int client_sd);


int main()
{
	//1. socket
	int server_fd = socket(AF_INET,SOCK_STREAM,0);
	
	if(server_fd<0)
	{
		perror("Socket: ");
		return (-1);
	}

	struct sockaddr_in server_address;
	memset(&server_address,0,sizeof(server_address));

	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(9000);
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);

	if(setsockopt(server_fd,SOL_SOCKET,SO_REUSEADDR,&(int){1},sizeof(int))<0)
	{
		perror("Setsockopt:");
		return -1;
	}

	//2. bind
	if(bind(server_fd,(struct sockaddr*) &server_address,sizeof(server_address))<0)
	{
		perror("Bind: ");
		return -1;
	}
	//3. listen
	if(listen(server_fd,5)<0)
	{
		perror("Listen: ");
		return -1;
	}

	//4. accept
	struct sockaddr_in client_address;				//we to pass this to accept method to get client info
	int client_address_len = sizeof(client_address); // accept also needs client_address length
	
	char client_name[50];

	int nfds;
	fd_set readfds, readyfd;
	//writefds, errorfds will be NULL for our task
	// timeout is NULL, meaning we wait forever for response

	// FD_ZERO(&fdset) initializes a descriptor set fdset to the null set.
	FD_ZERO(&readfds); 
	FD_SET(server_fd,&readfds);

	//nfds=max num of fds, -> the descriptors up to and including nfds–1 are tested
	int max_fd = server_fd; 
	// FD_setsize also works
	// nfds = FD_SETSIZE; 
	

	while(1)
	{

		readyfd = readfds;
        if(select(max_fd+1,&readyfd,NULL,NULL,NULL)<0)
        {
            perror("Select");
            return -1;
        }

        for(int i=0; i<=max_fd;i++)		// In the end of the lab our loop terminating condition was i<max_fd that is why our server was not monitoring all sockets
        {
            if(FD_ISSET(i,&readyfd))
            {

                if(i==server_fd)
                {
                    int client_fd = accept(server_fd,NULL,NULL);
                    FD_SET(client_fd,&readyfd);

                    
                    if(client_fd>max_fd) max_fd = client_fd;
                }
                else
                {
                    serve_client(i);
                    FD_CLR(i,&readfds);
                }
            }
        }

}
close(server_fd);
	//5. send/receive 
	//6. close the socket
	
}

/*
 * This function serves the client passed in as client_fd.
 */	
void serve_client(int client_fd){

	struct key_value
			{
			    char* key;
			    char* value;
			};

		int users_len = 2;
		struct key_value auth_users[users_len];

		auth_users[0].key = "Aisha";
		auth_users[0].value = "AHPass";
		auth_users[1].key = "Daniel";
		auth_users[1].value = "DBPass";


	char message[100];
	char command[20];
	char message_back[100];

		int pass_index = -1;

			int user=0;
			
			while(1){				

				memset(command,0,sizeof(command));
				memset(message_back,0,sizeof(message_back));
				
				if(recv(client_fd,command,sizeof(command)-1,0)>0)
				{
					// printf("%s \n",command);
					int init_size = strlen(command);
					char delim[] = " ";

					char *ptr = strtok(command, delim);

					if(strcmp(ptr,"USER")==0)
						{
							user=0;
							ptr = strtok(NULL, delim);

							printf("User %s attempting to authenticate.\n", ptr);

							int i = 0;	

							while(i<users_len) {

						        if(strcmp(auth_users[i].key, ptr) == 0) {
									
						        	strcat(message_back,"Username OK, password required!");
									
							    	pass_index = i;
							    	
							    	send(client_fd,message_back,strlen(message_back),0);

									printf("User %s exists. Waiting for PASS.\n", ptr);

						            user = 2;
						        }
						        i++;
						        
						    }

						    if(user ==0)
						    {
						    	strcat(message_back,"Username does not exist!");

						    	send(client_fd,message_back,strlen(message_back),0);

								printf("User %s does not exist.\n", ptr);
								break;
						    }
							
						}
						if(strcmp(ptr,"PASS")==0){

						ptr = strtok(NULL, delim);

					 	if(user==2){

					 		if(strcmp(auth_users[pass_index].value, ptr) == 0)
					 		{
					 			strcat(message_back,"Authentication complete!");
					 			
							    send(client_fd,message_back,strlen(message_back),0);

								printf("User authenticated.\n");
					 		}
					 		else{
					 			strcat(message_back,"Wrong password!");
					 			
							    send(client_fd,message_back,strlen(message_back),0);

								printf("User attempted incorrect password.\n");
					 		}
					 	}
					 	else{
					 		strcat(message_back,"Set USER first!");

						    send(client_fd,message_back,strlen(message_back),0);
					 	}
						break;
					 	}


							
					}



					}

close(client_fd);		
			
}	

		

