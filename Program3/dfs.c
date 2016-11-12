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
#include <pthread.h>

#define READ_BUFFER 1024
#define MAXBUFSIZE 1024
#define USERS 1000
#define CONFIG_FILE "dfs.conf"
#define LISTENQ 100

//variables used across functions
char *rootFolder;
int portNumber;
char * userNames[USERS];
char * password[USERS];
int usersTotal;

//for using stat and directory creation
struct stat st = {0};

void handlePut()
{
  printf("Handling PUT command... \n");
}

void handleGet()
{
  printf("Handling GET command ... \n");
}

void handleList()
{
  printf("Handling LIST command ...\n");
}

void validateUser(char * usernameAndPassword, int bytesLength, int connfd)
{
  int bytesSent;
  char * tempUsernamePassword = malloc(bytesLength);
  char * message = malloc(READ_BUFFER);
  strncpy(tempUsernamePassword, usernameAndPassword, bytesLength);

  char * tokens = strtok(tempUsernamePassword, "&");
  tokens = strtok(NULL, "&");

  char * tempUsername = tokens;
  tokens = strtok(NULL, "&");
  char * tempPassword = tokens;

  //printf("username:%s\n", tempUsername);
  //printf("password:%s\n", tempPassword);

  for(int i = 0; i<usersTotal; i++)
  {
    //printf("Inside loop: username %s, password %s\n", userNames[i], password[i]);
      if(strcmp(tempUsername, userNames[i]) == 0 && strcmp(tempPassword, password[i]) == 0)
      {
        //printf("MSG: Found\n");
        strcpy(message, "valid");
        bytesSent = send(connfd, message, strlen(message), 0);

        if(bytesSent < 0)
          printf("ERROR: Sending valid user message\n");

        return;
      }
  }

  //printf("MSG: Not found\n");
}

void runServer()
{
    
   int listenfd, connfd, n;
  pid_t childpid;
  socklen_t clilen;
  char buf[READ_BUFFER];
  struct sockaddr_in cliaddr, servaddr;

  //Create a socket for the soclet
  //If sockfd<0 there was an error in the creation of the socket
  if ((listenfd = socket (AF_INET, SOCK_STREAM, 0)) <0) {
    perror("ERROR: Problem in creating the socket\n");
    exit(EXIT_FAILURE);
  }
  else
    {
      printf("MSG: Socket created!\n");
    }

  //preparation of the socket address
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(portNumber);

  //reuse the same port - makes testing on the same port easier
  int reuse = 1;
  if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse,sizeof(reuse)) == -1)
    {
      perror("ERROR: Setsockopt failed\n");
    }
  
  //bind the socket
  if(bind (listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
    {
      perror("ERROR: error binding the socket\n");
      exit(EXIT_FAILURE);
    }
  else
    {
      printf("MSG: Binding successful!\n");
    }

  //listen to the socket by creating a connection queue, then wait for clients
  if(listen (listenfd, LISTENQ)<0)
    {
      perror("ERROR: Error listening\n");
      exit(EXIT_FAILURE);
    }
  else
    {
      printf("MSG: Server running...waiting for connections.\n");
    }

     for ( ; ; ) {

    clilen = sizeof(cliaddr);
    //accept a connection
    connfd = accept (listenfd, (struct sockaddr *) &cliaddr, &clilen);
    printf("%s\n","MSG: Received request...");

    if ( (childpid = fork ()) == 0 ) {//if it’s 0, it’s child process

      printf ("%s\n","MSG: Child created for dealing with client requests");

      //close listening socket
      close (listenfd);

      while ( (n = recv(connfd, buf, READ_BUFFER,0)) > 0){

        //printf("client message received %s\n", buf);

        if(strncmp(buf, "valid", 5) == 0)
        {
          validateUser(buf, n, connfd);
        }
        else if(strncmp(buf, "PUT", 3) == 0)
        {
          handlePut();
        }
        else if(strncmp(buf, "GET", 3) == 0)
        {
          handleGet();
        }
        else if(strncmp(buf, "LIST", 4) == 0)
        {
          handleList();
        }
        else
        {
          printf("ERROR: Message not recognized.\n");
        }

        n = send(connfd, buf, strlen(buf), 0);

        if(n < 0)
          printf("ERROR: Sending message\n");

        bzero(buf, READ_BUFFER);
      }

      exit(EXIT_SUCCESS);

    }

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
	
		//printf("username: %s, password: %s\n", userNames[i], password[i]);
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
	//printf("Root folder %s\n", rootFolder);
	//printf("Port # %d\n", portNumber);

	runServer();

	return 0;
} 