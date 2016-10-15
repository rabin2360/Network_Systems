#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define MAXLINE 4096 /*max text line length*/
#define LISTENQ 8 /*maximum number of client connections*/
#define READ_BUFFER 1024
#define DIRECTORY_INDEX_SIZE 3
#define SUPPORTED_CONTENT_TYPES 9

#define SERVER_CONF_FILE "ws.conf"

//variables used across functions
int portNumber;
char *documentRoot;
char *directoryIndexes[DIRECTORY_INDEX_SIZE];
char *contentType[SUPPORTED_CONTENT_TYPES];
char *extensionType[SUPPORTED_CONTENT_TYPES];

char * getExtensionOfFile(char * filepath)
{
  char * extension;
  const char ch = '.';

  extension = strrchr(filepath, ch);

  printf("extension %s\n", extension);

  return extension;
}

char * getContentType(char * extension)
{
  char * content_type = malloc(READ_BUFFER);

  int i = 0;

  for(i; i<SUPPORTED_CONTENT_TYPES; i++)
    {
      //printf("OUTSIDE %d, %s, %s\n", strlen(extension), extension, contentType[i]);
      
      if(strncmp(extension, extensionType[i], strlen(extension))==0)
	{
	  content_type = contentType[i];
	  break;
	}
    }

  return content_type;
}

void getProcessing(char * message, int connfd)
{
  //request type
  char * tokens = strtok(message, " \n");
  char * httpRequestType = malloc(strlen(tokens));
  strncpy(httpRequestType, tokens, strlen(tokens));
  printf("httpRequestType %s\n", httpRequestType);

  //www - file path
  tokens = strtok(NULL, " \n");

  char * filepath;
  if(strlen(tokens) != 1){
      filepath = malloc(strlen(tokens)+3);
      strncpy(filepath, "www", 3);
      strcat(filepath, tokens);
      printf("File path %s, length: %d\n", filepath, strlen(filepath));
    }
    else
      {
	filepath = "www/index.html";
	printf("File path %s\n", filepath);
      }

  if(strncmp(filepath, "www/index.htm",13) == 0)
    {
      filepath = "www/index.html";
      printf("File path %s \n", filepath);
    }
  
  //http version
  tokens = strtok(NULL, " \n");
  char * httpVersion = malloc(strlen(tokens));
  strncpy(httpVersion, tokens, strlen(tokens));
  printf("HTTP Version %s\n\n", httpVersion);
  

  //check http version
  if(strncmp(httpVersion, "HTTP/1.1",8) != 0 && strncmp(httpVersion, "HTTP/1.0",8) != 0)
    {
      printf("ERROR: Not supported http version -%s\n", httpVersion);
      //call for not supported http version method here
      return;
    }

  //get content type, need the string after the last occurence of . in the filepath
  char * fileExtension = getExtensionOfFile(filepath);

  //search the array for the file extension and get content type
  char * contentType = getContentType(fileExtension);

  //get the file
  int indexFd = open(filepath,O_RDONLY);

  //send the response
  if(indexFd  != -1)
    {
      char *contentHeader;
      contentHeader = malloc(50);
      strcpy(contentHeader, "Content-Type: ");
      strcat(contentHeader, contentType);
      strcat(contentHeader, "\n");

      int contentHeaderLength = strlen(contentHeader);

      int sizeOfFile = lseek(indexFd, 0, SEEK_END);
      lseek(indexFd,0, SEEK_SET);
      char fileSizeStr[20];
      sprintf(fileSizeStr, "%d", sizeOfFile);

      char *lengthOfContent;
      lengthOfContent = malloc(50);
      strcpy(lengthOfContent, "Content-Length: ");
      strcat(lengthOfContent, fileSizeStr);
      strcat(lengthOfContent, "\n");
      int lengthOfContentLength = strlen(lengthOfContent);

	send(connfd, "HTTP/1.1 200 OK\n",16,0);
	send(connfd, contentHeader, contentHeaderLength, 0);
	send(connfd, lengthOfContent, lengthOfContentLength, 0);
	send(connfd, "Connection: keep-alive\n\n",24,0);
	    
	    
	int bytesRead;
	char *indexDataToSend = malloc(READ_BUFFER);
	while((bytesRead = read(indexFd, indexDataToSend, READ_BUFFER))>0)
	write(connfd, indexDataToSend, bytesRead);
    }
  else
    {
      printf("ERROR: File not found %s\n", filepath);
      //call method here to say file not found in the server
      return;
    }

  close(indexFd);

  
  //printf("GET processing:\n %s\n", message);

}

//running the server
void runServer()
{
  int listenfd, connfd, n;
  pid_t childpid;
  socklen_t clilen;
  char buf[MAXLINE];
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

    printf("%s\n","Received request...");

    if ( (childpid = fork ()) == 0 ) {//if it’s 0, it’s child process

      printf ("%s\n","Child created for dealing with client requests");

      //close listening socket
      close (listenfd);

      while ( (n = recv(connfd, buf, MAXLINE,0)) > 0)  {
	//printf("%s","String received from and resent to the client:\n");
	//puts(buf);

	char * tempString = malloc(READ_BUFFER);
	strcpy(tempString, buf);
	
	char * getPostTag = strtok(tempString, " \n");
	  
	if(strncmp(getPostTag, "GET", 3)==0)
	  {
	    //do GET processing
	    getProcessing(buf, connfd);
	  }
	else
	  {
	    perror("ERROR: Message received is not a GET request\n");
	  }
	
	//printf("buf has this: %s\n", buf);

	/*
	int indexFd = open("/home/rabin/Documents/Network_Systems/Program2/www/index.html",O_RDONLY);

	if(indexFd  != -1)
	  {
	    char *contentHeader;
	    contentHeader = malloc(50);
	    strcpy(contentHeader, "Content-Type: ");
	    strcat(contentHeader, "text/html\n");

	    int contentHeaderLength = strlen(contentHeader);

	    int sizeOfFile = lseek(indexFd, 0, SEEK_END);
	    lseek(indexFd,0, SEEK_SET);
	    char fileSizeStr[20];
	    sprintf(fileSizeStr, "%d", sizeOfFile);

	    char *lengthOfContent;
	    lengthOfContent = malloc(50);
	    strcpy(lengthOfContent, "Content-Length: ");
	    strcat(lengthOfContent, fileSizeStr);
	    strcat(lengthOfContent, "\n");
	    int lengthOfContentLength = strlen(lengthOfContent);

	    send(connfd, "HTTP/1.1 200 OK\n",16,0);
	    send(connfd, contentHeader, contentHeaderLength, 0);
	    send(connfd, lengthOfContent, lengthOfContentLength, 0);
	    send(connfd, "Connection: keep-alive\n\n",24,0);

	    int bytesRead;
	    char *indexDataToSend = malloc(READ_BUFFER);
	    while((bytesRead = read(indexFd, indexDataToSend, READ_BUFFER))>0)
	      write(connfd, indexDataToSend, bytesRead);

	    close(indexFd);
	  }
	*/
      }

      if (n < 0)
	printf("ERROR: Error receiving the client request.\n");

      exit(0);
    }
    //close socket of the server
    close(connfd);
  }
  
}


//reading server configuration
void parseServerConfFile(char * fileLocation)
{
  int bytesRead, fd;
  char * confFileContent = malloc(READ_BUFFER);

  //reading the file ws.conf
  fd = open(fileLocation, O_RDONLY);

  //initializing the variables before reading values in them
  portNumber = -1;
  documentRoot = malloc(READ_BUFFER);
  //directoryIndexes = malloc(READ_BUFFER);
 
  if(fd != -1)
    {
      //reading the file into the buffer
      while((bytesRead = read(fd, confFileContent, READ_BUFFER)) >0)
	{
	  //tokenizing the read content
	  char * tokens = strtok(confFileContent, " \n");

	  //go through the tokens to parse accordingly
	  while(tokens != NULL)
	    {

	      //parse the read tokens
	      if(strcmp(tokens, "Listen") == 0)
		{
		  tokens = strtok(NULL, " \n");
		  portNumber = atoi(tokens);
		}
	      else if(strcmp(tokens, "DocumentRoot")==0)
		{
		  tokens = strtok(NULL, " \n");
		  documentRoot = tokens;
		}
	      else if(strcmp(tokens, "DirectoryIndex")==0)
		{
		  int i;
		  for(i = 0; i<DIRECTORY_INDEX_SIZE; i++)
		    {
		      tokens = strtok(NULL, " \n");
		      directoryIndexes[i] = tokens;
		    }
		}
	      else if(tokens[0] == '.')
		{
		  int i;
		  for(i = 0; i<SUPPORTED_CONTENT_TYPES;i++)
		    {
		      extensionType[i] = tokens;
		      tokens = strtok(NULL, " \n");

		      contentType[i] = tokens;
		      tokens = strtok(NULL, " \n");
		    }

		}
	      //gets the next token
	      tokens = strtok(NULL, " \n");
	    }
	}

    }
  else
    {
      printf("ERROR: Cannot read the web configuration file - ws.conf. Please check the local directory.\n");
      exit(1);
    }

  //DEBUG
  /*
    printf("Port number %d\n", portNumber);
    printf("documentRoot %s\n", documentRoot);
    printf("directory index %s\n", directoryIndexes[0]);
    printf("directory index %s\n", directoryIndexes[1]);
    printf("directory index %s\n", directoryIndexes[2]);
  
    int i;
    for(int i = 0; i<SUPPORTED_CONTENT_TYPES; i++)
    {
       printf("%s ::", extensionType[i]);
       printf("%s\n", contentType[i]);
   
    }
  */
  
}


int main (int argc, char **argv)
{
  //parse the ws.conf file and acquire appropriate information
  parseServerConfFile(SERVER_CONF_FILE);

  //run server
  runServer();
  

}
