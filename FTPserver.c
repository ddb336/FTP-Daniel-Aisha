#include<stdio.h>
#include<string.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<sys/types.h>
 #include <unistd.h>
#include<netinet/in.h>


int main()
{

	//1. socket
	int server_fd = socket(AF_INET,SOCK_STREAM,0);
	// printf("Server IN");
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
	//2. bind
	if(bind(server_fd,(struct sockaddr*) &server_address,sizeof(server_address))<0)
	{
		perror("Bind: ");
		return -1;
	}
	//3. listen
	if(listen(server_fd,2)<0)
	{
		perror("Listen: ");
		return -1;
	}
	//4. accept
	struct sockaddr_in client_address;				//we to pass this to accept method to get client info
	int client_address_len = sizeof(client_address); // accept also needs client_address length
	
	char client_name[50];

	int readyfd, nfds;
	fd_set readfds;
	//writefds, errorfds will be NULL for our task
	// timeout is NULL, meaning we wait forever for response

	// FD_ZERO(&fdset) initializes a descriptor set fdset to the null set.
	FD_ZERO(&readfds); 

	//nfds=max num of fds, -> the descriptors up to and including nfds–1 are tested
	// nfds = server_fd + 1;
	// FD_setsize also works
	nfds = FD_SETSIZE; 

	

	

	while(1)
	{
		
		// int select(int nfds, fd_set *restrict readfds, fd_set *restrict writefds,
  //        fd_set *restrict errorfds, struct timeval *restrict timeout);
		FD_SET(server_fd, &readfds); 
		readyfd = select(nfds, &readfds, NULL, NULL, NULL);

		if (FD_ISSET(server_fd, &readfds)){

			// printf("CLIENT IN");
			int client_fd = accept(server_fd, (struct sockaddr*)&client_address,(socklen_t *)&client_address_len);



		char message[100];
		char command[20];
		char message_back[100];
		

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

		inet_ntop(AF_INET,&client_address.sin_addr,client_name,sizeof(client_name));

		

		if(client_fd<0)
		{
			perror("Accept: ");
			return -1;
		}

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
							    	// printf("USEROK");
							    	// printf("%s: %s \n",ptr, message_back);
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


			
		
	}
	}

	close(server_fd);
	//5. send/receive 
	//6. close the socket
	return 0;
}
