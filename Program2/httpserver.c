#include<netinet/in.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<signal.h>
#include<fcntl.h>

#define MAX_CONNECTIONS 1000
#define MAX_SUPPORTED_TYPES 250
#define BUFFER_SIZE 1024
#define MAX_RETRIES 100
#define MAX_URI 255

int connected_clients[MAX_CONNECTIONS];
char *supported_types[MAX_SUPPORTED_TYPES]; //stores values such as gif, png, html, etc
char *content_types[MAX_SUPPORTED_TYPES];  //stores the corresponding content type strings such as text/html, etc
char *PORT;
char *DOC_ROOT;

char *get_filename_ext(char *filename) {
    char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}

char *getContentTypeString(char *extension) {
	int i = 0;
	int index;
	for(i; i<MAX_SUPPORTED_TYPES; i++){
		if(strcmp(extension, supported_types[i]) == 0){
			index = i;
			break;
		}
	}
	return content_types[index];
}

void respond(int n) {
	char *root;
	char *extension;
	root = getenv("PWD");
	char *document_root;
	document_root = malloc(100);
	strcpy(document_root, root);
	strcat(document_root, "/www");

    char mesg[99999], *request_str[3], data_to_send[BUFFER_SIZE], path[99999];
    int request, fd, bytes_read;

    memset((void*)mesg, (int)'\0', 99999);

    request=recv(connected_clients[n], mesg, 99999, 0);

    printf("HERE: %s\n", mesg);
    
    if (request<0)    // receive error
        fprintf(stderr,("recv() error\n"));
    else if (request==0)    // receive socket closed
        fprintf(stderr,"Client disconnected upexpectedly.\n");
    else    // message received
    {
        request_str[0] = strtok(mesg, " \t\n");
        if (strncmp(request_str[0], "GET\0", 4)==0)
        {
            request_str[1] = strtok (NULL, " \t");
            request_str[2] = strtok (NULL, " \t\n");
            /* 400 Error */
            if ( strncmp( request_str[2], "HTTP/1.0", 8)!=0 && strncmp( request_str[2], "HTTP/1.1", 8)!=0 )
            {
            	char *bad_request_str;
            	bad_request_str = malloc(80);
            	strcpy(bad_request_str, "HTTP/1.1 400 Bad Request: Invalid HTTP-Version: ");
            	strcat(bad_request_str, request_str[2]);
            	strcat(bad_request_str, "\n");

            	//char *bad_request_str = "HTTP/1.1 400 Bad Request: Invalid HTTP-Version: %s\n", request_str[2];
            	int bad_request_strlen = strlen(bad_request_str);
                write(connected_clients[n], bad_request_str, bad_request_strlen);
                //free(bad_request_str);
            }
            else
            {
                if ( strncmp(request_str[1], "/\0", 2)==0 )
                    request_str[1] = "/index.html"; //Client is requesting the root directory, send it index.html

                extension = get_filename_ext(request_str[1]);
                int type_found = 0;
                int i = 0;
                for(i; strcmp(supported_types[i],"") != 0; i++){
                	if(strcmp(supported_types[i], extension) == 0){
                		type_found = 1;
                		break;
                	}
                }
                /* 501 Error */
                if(type_found != 1){
                	char *not_imp_str;
                	not_imp_str = malloc(80);
                	strcpy(not_imp_str, "HTTP/1.1 501 Not Implemented: ");
                	strcat(not_imp_str, path);
                	strcat(not_imp_str, "\n");
                		                	
					int not_imp_strlen = strlen(not_imp_str);
					write(connected_clients[n], not_imp_str, not_imp_strlen);
					//free(not_imp_str);               	
                }

                strcpy(path, document_root);
                strcpy(&path[strlen(document_root)], request_str[1]); //creating an absolute filepath

                /* 400 Error */
                if (strlen(path) > MAX_URI){

                	char *invalid_uri_str;
                	invalid_uri_str = malloc(80);
                	strcpy(invalid_uri_str, "HTTP/1.1 400 Bad Request: Invalid URI: ");
                	strcat(invalid_uri_str, path);
                	strcat(invalid_uri_str, "\n");
                		                	
                	int invalid_uri_strlen = strlen(invalid_uri_str);
                	write(connected_clients[n], invalid_uri_str, invalid_uri_strlen);
                	//free(invalid_uri_str);               	
                }

                else{
	                if ( (fd=open(path, O_RDONLY))!=-1 )
	                {

	                	char *content_type_header;
	                	content_type_header = malloc(50);
	                	strcpy(content_type_header, "Content-Type: ");
	                	strcat(content_type_header, getContentTypeString(extension));
	                	strcat(content_type_header, "\n");
	                	int content_type_header_len = strlen(content_type_header);

	                	int fsize = lseek(fd, 0, SEEK_END);
	                	lseek(fd, 0, SEEK_SET);
	                	char fsize_str[20];
	                	sprintf(fsize_str, "%d", fsize);
	                	char *content_length_header;
	                	content_length_header = malloc(50);
	                	strcpy(content_length_header, "Content-Length: ");
	                	strcat(content_length_header, fsize_str);
	                	strcat(content_length_header, "\n");
	                	int content_length_header_len = strlen(content_length_header);

	                    send(connected_clients[n], "HTTP/1.1 200 OK\n", 16, 0);
	                    send(connected_clients[n], content_type_header, content_type_header_len, 0);
	                    send(connected_clients[n], content_length_header, content_length_header_len, 0);
	                    send(connected_clients[n], "Connection: keep-alive\n\n", 24, 0);
	                    while ( (bytes_read=read(fd, data_to_send, BUFFER_SIZE))>0 )
	                        write (connected_clients[n], data_to_send, bytes_read);   

	                    //free(content_type_header);                 	
	                    //free(content_length_header);                 	
	                }
	                /* 404 Error */
	                else{
	                	char *not_found_str;
	                	not_found_str = malloc(80);
	                	strcpy(not_found_str, "HTTP/1.1 404 Not Found: ");
	                	strcat(not_found_str, path);
	                	strcat(not_found_str, "\n");

	                	int not_found_strlen = strlen(not_found_str);
	                	write(connected_clients[n], not_found_str, not_found_strlen); //file not found, send 404 error
	                	//free(not_found_str);
	                }   
            }
            }
        }
    }

    shutdown (connected_clients[n], SHUT_RDWR);
    close(connected_clients[n]);
    connected_clients[n]=-1;
}

void initializeServer(char *port){

	int create_socket, new_socket;
	socklen_t addrlen;
	char *buffer = malloc(BUFFER_SIZE);
	struct sockaddr_in address;

	int i = 0;
	for(i; i<MAX_CONNECTIONS; i++){
		connected_clients[i] == -1;
	}

	if ((create_socket = socket(AF_INET, SOCK_STREAM, 0)) > 0){
		printf("Socket created successfully\n");
	}

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(8097);

	int yes = 1;
	if ( setsockopt(create_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1 )
	{
	    perror("setsockopt");
	}
	int num_try = 0;
	while(num_try < MAX_RETRIES){
		if (bind(create_socket, (struct sockaddr *) &address, sizeof(address)) == 0){
			printf("Binding socket\n");
			break;
		}
		else{
			num_try++;
			if(num_try == 99){
				printf("Error: max retries reached, could not bind server\n");
				exit(1);
			}
		}		
	}
	if (listen(create_socket, 1000) < 0){
		perror("Server: listen");
		exit(1);
	}
	int slot = 0;
	while (1){
		if ((connected_clients[slot] = accept(create_socket, (struct sockaddr *) &address, &addrlen)) > 0){
				if (fork() == 0){
					respond(slot);
					slot++;
					exit(0);
				}
		}
		else{
			perror("Server: accept");
			exit(1);	
		}
	}
}

void configureServer(char *path){
	int fd, bytes_read;
	char *data_to_send;
	char *file_type;
	char *cont_type;
	data_to_send = malloc(BUFFER_SIZE);

	int i = 0;
	for(i; i<MAX_SUPPORTED_TYPES; i++){
		supported_types[i] = "";
		content_types[i] = "";
	}

	int n = 0;
	if((fd=open(path, O_RDONLY))!=-1){
		 while((bytes_read=read(fd, data_to_send, BUFFER_SIZE))>0){
		 	char *token = strtok(data_to_send, " \t\n");
		 	while(token != NULL){
		 		if(strcmp(token, "Listen") == 0){
		 			token = strtok(NULL, " \t\n");
		 			PORT = token;
		 		}
		 		else if(token[0] == '.'){
		 			char new_token[strlen(token)];
		 			char extension[strlen(token) - 1];
		 			strcpy(new_token, token);
		 			int i = 1;
		 			int j = 0;
		 			for(i;i<strlen(token);i++){
		 				extension[j] = new_token[i];
		 				j++;
		 			}
		 			file_type = malloc(10);
		 			memcpy(file_type, extension, strlen(extension) + 1);
		 			supported_types[n] = file_type;

		 			token = strtok(NULL, " \t\n");
		 			cont_type = malloc(20);
		 			memcpy(cont_type, token, strlen(token) + 1);		 			
		 			content_types[n] = cont_type;
		 			n++;
		 		}
		 		else if(strcmp(token, "DocumentRoot") == 0){
		 			token = strtok(NULL, " \t\n");
		 			DOC_ROOT = token;		 			
		 		}
		 		token = strtok(NULL, " \t\n");
		 	}
		 }
	}
	else{
		printf("Configuration file not found: %s\n", path);
		exit(1);
	}

}

int main(int argc, char* argv[]){
	if(argc > 1){
		char *conf_file = argv[1];
		configureServer(conf_file);
		initializeServer(PORT);
		return 0;	
	}
	else{
		printf("usage: ./main <CONFIGURATION_FILE>\n");
	}
}
