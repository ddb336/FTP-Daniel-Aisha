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
		
		// int select(int nfds, fd_set *restrict readfds, fd_set *restrict writefds,
  //        fd_set *restrict errorfds, struct timeval *restrict timeout);

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

							// printf("%s \n",ptr);
					

							int i = 0;	

							// size_t len = sizeof(auth_users) / sizeof(auth_users[0]);
							// size_t len = sizeof(auth_users);
							// printf("%zu \n",len);

							while(i<users_len) {

						        if(strcmp(auth_users[i].key, ptr) == 0) {
									
						        	strcat(message_back,"Username OK, password required!");
									
							    	pass_index = i;
							    	
							    	send(client_fd,message_back,strlen(message_back),0);

				
						            user = 2;
						            // break;
						        }
						        i++;
						        
						    }

						    if(user ==0)
						    {
						    	strcat(message_back,"Username does not exist!");
						    	// printf("passiNOTOK");
							    // printf("%s: %s \n",ptr, message_back);
						    	send(client_fd,message_back,strlen(message_back),0);
						    	
						    	// break;
						    }

									
						}
						if(strcmp(ptr,"PASS")==0){

						ptr = strtok(NULL, delim);

					 	if(user==2){

					 		if(strcmp(auth_users[pass_index].value, ptr) == 0)
					 		{
					 			strcat(message_back,"Authentication complete!\0");
					 			
							    // printf("USEROK");
							    // printf("%s: %s \n",ptr, message_back);
							    send(client_fd,message_back,strlen(message_back),0);
							    //reset the user
							    
					 		}
					 		else{
					 			strcat(message_back,"Wrong password!\n");
					 			
							    // printf("passiNOTOKokok");
							    // printf("%s: %s \n",ptr, message_back);
							    send(client_fd,message_back,strlen(message_back),0);

					 		}

					 	}
					 	else{
					 		strcat(message_back,"Set USER first!\n");
					 			
						    // printf("userNOTOK");
						    // printf(" %d \n",pass_index);
						    // printf("%s: %s \n",ptr, message_back);
						    send(client_fd,message_back,strlen(message_back),0);

					 	}



					 	}


							
					}



					}

close(client_fd);		
			
}	

		

