#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define MAXLINE 4096 /*max text line length*/
#define LISTENQ 100 /*maximum number of client connections*/
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

//from the file path gets the extension of the file being requested
char * getExtensionOfFile(char * filepath)
{
  char * extension;
  const char ch = '.';

  extension = strrchr(filepath, ch);

  printf("extension %s\n", extension);

  return extension;
}

//gets the content type for the response based on the file extension
char * getContentType(char * extension)
{
  char * content_type = malloc(READ_BUFFER);
  content_type = "false";
  
  int i = 0;

  for(i; i<SUPPORTED_CONTENT_TYPES; i++)
    {
      if(strncmp(extension, extensionType[i], strlen(extension))==0)
	{
	  content_type = contentType[i];
	  break;
	}
    }

  return content_type;
}

void error501Message(int connfd, char * notSupported)
{
  char *errorMessage = malloc(100);
  strcpy(errorMessage, "<html><body>ERROR: 501. Not Implemented Support For:");
  strcat(errorMessage, notSupported);
  strcat(errorMessage, "</body></html>\n"); 

  char *contentHeader;
  contentHeader = malloc(50);
  strcpy(contentHeader, "Content-Type: ");
  strcat(contentHeader, "text/html");
  strcat(contentHeader, "\n");

     
  //printf("%s\n", pageNotFound);

  int lengthOfMessage = strlen(errorMessage);
  char strLengthOfMessage[20];

  sprintf(strLengthOfMessage, "%d", lengthOfMessage);
      
  char *lengthOfContent;
  lengthOfContent = malloc(50);
  strcpy(lengthOfContent, "Content-Length: ");
  strcat(lengthOfContent, strLengthOfMessage);
  strcat(lengthOfContent, "\n");
  int lengthOfContentLength = strlen(lengthOfContent);

      
  send(connfd, "HTTP/1.1 501 Not Implemented\n",29,0);
  send(connfd, contentHeader, strlen(contentHeader),0);
  send(connfd, lengthOfContent, lengthOfContentLength, 0);
  send(connfd, "Connection: keep-alive\n\n",24,0);
  send(connfd, errorMessage, lengthOfMessage, 0);

      
  shutdown(connfd, SHUT_RDWR);
  close(connfd);
}

void error404Message(int connfd, char * urlEntered)
{
      char *contentHeader;
      contentHeader = malloc(50);
      strcpy(contentHeader, "Content-Type: ");
      strcat(contentHeader, "text/html");
      strcat(contentHeader, "\n");

      char * strPortNumber = malloc(10);
      sprintf(strPortNumber, "%d", portNumber);
      
      char *pageNotFound = malloc(100);
      strcpy(pageNotFound, "<html><body>ERROR: 404. Not Found Reason URL does not exist: http://localhost:");
      strcat(pageNotFound, strPortNumber);
      strcat(pageNotFound, urlEntered);
      strcat(pageNotFound, "</body></html>\n");
      //printf("%s\n", pageNotFound);

      int lengthOfMessage = strlen(pageNotFound);
      char strLengthOfMessage[20];

      sprintf(strLengthOfMessage, "%d", lengthOfMessage);
      
      char *lengthOfContent;
      lengthOfContent = malloc(50);
      strcpy(lengthOfContent, "Content-Length: ");
      strcat(lengthOfContent, strLengthOfMessage);
      strcat(lengthOfContent, "\n");
      int lengthOfContentLength = strlen(lengthOfContent);

      
      send(connfd, "HTTP/1.1 404 Not Found\n",23,0);
      send(connfd, contentHeader, strlen(contentHeader),0);
      send(connfd, lengthOfContent, lengthOfContentLength, 0);
      send(connfd, "Connection: keep-alive\n\n",24,0);
      send(connfd, pageNotFound, lengthOfMessage, 0);
}

void error400Message(int connfd, char * errorType, char * message)
{
  char *errorMessage = malloc(150);

  if(strncmp(errorType, "filepath",8)==0)
    {
      strcpy(errorMessage, "<html><body>ERROR: 400. Invalid URL:");
    }
  else if(strncmp(errorType, "httpversion",11)==0)
    {
      strcpy(errorMessage, "<html><body>ERROR: 400. Invalid HTTP-Version: ");
    }
  else if(strncmp(errorType, "httpinc", 7) == 0)
    {
      strcpy(errorMessage, "<html><body>ERROR: 400. Invalid HTTP-Version incomplete: ");
    }
  else if(strncmp(errorType, "invalidurl", 7) == 0)
    {
      strcpy(errorMessage, "<html><body>ERROR: 400. Invalid URL: ");
    }
  
  strcat(errorMessage, message);
  strcat(errorMessage, " </body></html>\n"); 

  char *contentHeader;
  contentHeader = malloc(50);
  strcpy(contentHeader, "Content-Type: ");
  strcat(contentHeader, "text/html");
  strcat(contentHeader, "\n");
  //printf("%s\n", pageNotFound);

  int lengthOfMessage = strlen(errorMessage);
  char strLengthOfMessage[20];

  sprintf(strLengthOfMessage, "%d", lengthOfMessage);
      
  char *lengthOfContent;
  lengthOfContent = malloc(50);
  strcpy(lengthOfContent, "Content-Length: ");
  strcat(lengthOfContent, strLengthOfMessage);
  strcat(lengthOfContent, "\n");
  int lengthOfContentLength = strlen(lengthOfContent);

      
  send(connfd, "HTTP/1.1 400 Bad Request\n",25,0);
  send(connfd, contentHeader, strlen(contentHeader),0);
  send(connfd, lengthOfContent, lengthOfContentLength, 0);
  send(connfd, "Connection: keep-alive\n\n",24,0);
  send(connfd, errorMessage, lengthOfMessage, 0);

      
  shutdown(connfd, SHUT_RDWR);
  close(connfd);
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

  //when file path in the received message is null
  if(tokens == NULL)
    {
      printf("ERROR: Filepath is null\n");
      error400Message(connfd, "filepath", "null");
      return;
    }
  //this gets called when it's just a space for file name
  /*else
    {
      error400Message(connfd, "filepath", "null");
      return;
    }
  */
  
  char * filepath;
  char * urlEntered;
  if(strlen(tokens) != 1)
    {
      urlEntered = malloc(strlen(tokens));
      strncpy(urlEntered, tokens, strlen(tokens));
      
      filepath = malloc(strlen(tokens)+3);
      strncpy(filepath, "www", 3);
      strcat(filepath, tokens);
    }
  else if (strlen(tokens) == 1)
      {
	printf("%s\n", tokens);
	filepath = "www/index.html";
      }

  if(strncmp(filepath, "www/index.htm",13) == 0)
    {
      filepath = "www/index.html";
    }

  printf("File path %s\n", filepath);

  //http version
  tokens = strtok(NULL, " \n");

  //check if it's null
  if(tokens == NULL)
    {
      printf("ERROR: HTTP is null\n");
      error400Message(connfd, "httpversion", "null");
      return;
    }
  
  //check if the received message is incomplete
  else if(strlen(tokens)!=9)
    {
      printf("ERROR: Incomplete HTTP %zu\n", strlen(tokens));
      error400Message(connfd, "httpinc", tokens);
      return;
    }
  
  char * httpVersion = malloc(strlen(tokens));
  strncpy(httpVersion, tokens, strlen(tokens));
  printf("HTTP Version %s\n\n", httpVersion);
  
  //check http version
  if(strncmp(httpVersion, "HTTP/1.1",8) != 0 && strncmp(httpVersion, "HTTP/1.0",8) != 0)
    {
      printf("ERROR: Not supported http version -%s\n", httpVersion);
      error501Message(connfd,httpVersion);
      //call for not supported http version method here
      return;
    }
  
  //get content type, need the string after the last occurence of . in the filepath
  char * fileExtension = getExtensionOfFile(filepath);

  if(fileExtension == NULL)
    {
      error400Message(connfd, "invalidurl", filepath);
    }
  
  //search the array for the file extension and get content type
  char * contentType = getContentType(fileExtension);

  if(strncmp(contentType, "false", 5) == 0)
    {
      error501Message(connfd, fileExtension);
      return;
    }
  
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
	  send(connfd, indexDataToSend, bytesRead,0);
    }
  else
    {
      printf("ERROR: File not found %s\n", filepath);
      error404Message(connfd, urlEntered);
     }
  
  shutdown(connfd, SHUT_RDWR);
  close(connfd);
  close(indexFd);

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

	if(n < 0)
	  {
	    printf("ERROR: invalid message received\n");
	  }
	else if(strncmp(getPostTag, "GET", 3)==0)
	  {
	    //do GET processing
	    getProcessing(buf, connfd);
	  }
	else if(strncmp(getPostTag, "POST", 4) == 0)
	  {
	    printf("ERROR: POST is not a GET request\n");
	    error501Message(connfd, getPostTag);
	  }
	
	else
	  {
	    printf("ERROR: request is not supproted: %s, %zu\n", getPostTag, strlen(getPostTag));
	    error501Message(connfd, getPostTag);
	  }

	printf("%s\n", buf);
	
      }

      if (n < 0)
	printf("ERROR: Error receiving the client request.\n");

      exit(EXIT_SUCCESS);
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

		  if(portNumber<1024)
		    {
		      printf("ERROR: Cannot use port numbers less than 1024. Please check ws.conf file!\n");
		      exit(EXIT_FAILURE);
		    }
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
      exit(EXIT_FAILURE);
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
  
  exit(EXIT_SUCCESS);
}
