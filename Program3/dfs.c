#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>
#include <memory.h>
#include <sys/wait.h>
#include <stdbool.h>

#define READ_BUFFER 1024
#define MAXBUFSIZE 1024
#define USERS 1000
#define CONFIG_FILE "dfs.conf"

//variables used across functions
char *rootFolder;
int portNumber;
char * userNames[USERS];
char * password[USERS];
int usersTotal;

//for using stat and directory creation
struct stat st = {0};

bool validateUser(char * username, char * password)
{

	return false;
}

void runServer()
{
	 int sock;                           //This will be our socket
  struct sockaddr_in sin, remote;     //"Internet socket address structure"
  unsigned int remote_length;         //length of the sockaddr_in structure
  int nbytes;                        //number of bytes we receive in our message
  char buffer[MAXBUFSIZE];             //a buffer to store our received message

bzero(&sin,sizeof(sin));                    //zero the struct
  sin.sin_family = AF_INET;                   //address family
  sin.sin_port = htons(portNumber);        //htons() sets the port # to network byte order
  sin.sin_addr.s_addr = INADDR_ANY;           //supplies the IP address of the local machine

 //Causes the system to create a generic socket of type UDP (datagram)
  if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    {
      printf("ERROR: Unable to create socket\n");
      exit(1);
    }

  if (bind(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0)
    {
      printf("ERROR: Unable to bind socket.\n");
      exit(1);
    }
  else
    {
      printf("MSG: Server successfully started!\n");
    }


  remote_length = sizeof(remote);
  char msg[MAXBUFSIZE];

  while(1){

    //resetting the buffers whenever needed
    bzero(buffer,sizeof(buffer));
    bzero(msg, MAXBUFSIZE);

    //receiving message from client
    nbytes = recvfrom(sock,buffer,MAXBUFSIZE,0, (struct sockaddr *) &remote, &remote_length);
    printf("%s", buffer);

     if(nbytes < 0)
      printf("ERROR: Error receiving the message\n");

  	nbytes = sendto(sock, buffer, strlen(buffer), 0, (struct sockaddr *)&remote, remote_length);

  	if(nbytes < 0)
  		printf("ERROR: Error sending the message\n");
}

}

//create directory if not present
void createDirectory(char * directoryName)
{
	if(stat(directoryName, &st) == -1)
	{
		mkdir(directoryName, 0700);
	}
}

void readConfigFile(char *rootFolder)
{
	int bytesRead;
	char *confFileContent = malloc(READ_BUFFER);
	int fd = open(CONFIG_FILE, O_RDONLY);

	usersTotal = 0;

	if(fd != -1)
	{
		while((bytesRead = read(fd, confFileContent, READ_BUFFER)) > 0)
		{
			char * tokens = strtok(confFileContent, " \n");

			while(tokens != NULL)
			{
				userNames[usersTotal] = tokens;
				tokens = strtok(NULL, " \n");

				password[usersTotal] = tokens;
				tokens = strtok(NULL, " \n");

				usersTotal++;
			}
		}
	}
	else
	{
		printf("ERROR: Server config file cannot be found");
		exit(EXIT_FAILURE);
	}


	//creating the sub directories for each user		
	for(int i = 0; i<usersTotal; i++)
	{
		char *subDirectory = malloc(READ_BUFFER);
		strcpy(subDirectory, rootFolder);
		strcat(subDirectory, "/");
	
		printf("username: %s, password: %s\n", userNames[i], password[i]);
		strcat(subDirectory, userNames[i]);
		createDirectory(subDirectory);
	}

}


int main(int argc, char ** argv)
{

	if(argc != 3)
	{
		printf("ERROR: dfc <rootFolderName> <Port Number>\n");
		exit(EXIT_FAILURE);
	}

	//getting and storing root folder
	rootFolder = malloc(READ_BUFFER);
	strcpy(rootFolder, argv[1]);

	//getting and storing the port number
	portNumber = atoi(argv[2]);

	//order of function calls important here
	createDirectory(rootFolder);
	readConfigFile(rootFolder);
	
	//DEBUG
	printf("Root folder %s\n", rootFolder);
	printf("Port # %d\n", portNumber);

	runServer();

	return 0;
} 