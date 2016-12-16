/*
  Name: Rabin Ranabhat
  CSCI 5273
  Program 4
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <stdbool.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <pthread.h>
#include <openssl/md5.h>
#include <time.h>
#include <sys/stat.h>

#define MAXLINE 1028
#define CACHE_CONTENT 102400
#define LISTENQ 100
#define READ_BUFFER 1028
#define CACHE_SIZE 10

//variables used across methods
int portNumber;
int cacheTimeOut;
char *ipAddress;
char *requestedURL;
char *httpVersion;
char *requestType;
char *messageBody;

//extracts the GET command
//extracts the local resouce being searched
//extracts the HTTP version
//determines if there is message body
int checkMessageFormat(char *buf)
{
	//malloc variables
	httpVersion = malloc(READ_BUFFER);
	requestedURL = malloc(READ_BUFFER);
	requestType = malloc(READ_BUFFER);
	messageBody = malloc(READ_BUFFER);
	bool hasBody = false;
	strcpy(requestType,"");
	strcpy(requestedURL, "");
	strcpy(httpVersion, "");

	char * messageToken = strtok(buf, " \n\t");

	//getting the request type
	if(messageToken != NULL)
	{
		strcpy(requestType, messageToken);
		//return;
	}
	else
	{
		//printf("%s\n", "ERROR: Missing http request type.");
		return 2;
	}

	messageToken = strtok(NULL, " \n\t");

	if(messageToken != NULL)
	{
		strcpy(requestedURL, messageToken);
	}
	else
	{
		//printf("%s\n", "ERROR: Missing URL.");
		return 2;
	}

	//bzero(messageToken, strlen(messageToken));
	messageToken = strtok(NULL, " \n\t");

	if(messageToken != NULL)
	{
		strcpy(httpVersion, messageToken);

		if(strncmp(httpVersion, "HTTP/1.0", 8) != 0 && strncmp(httpVersion, "HTTP/1.1", 8) != 0)
		{
			strcpy(httpVersion, "");
			return 2;			
		}
	}
	else
	{

		//printf("%s\n", "ERROR: Missing http version.");
		return 2;
	}

	//bzero(messageToken, strlen(messageToken));

	//print the obtained values
	/*printf("Request Type: %s\n", requestType);
	printf("Requested URL: %s\n", requestedURL);
	printf("HTTP Version: %s\n", httpVersion);
	*/

	//printf("message token before split %s\n", messageToken);
	messageToken = strtok(NULL, "\n\r");
	if(messageToken == NULL || strlen(messageToken) < 7)
	{
		//printf("has NO body.\n");
		//printf("has NO body. messageToken length: %lu\n", strlen(messageToken));
		hasBody = false;
	}
	else
	{
		//printf("HAS body. messageToken length: %lu message token: %s\n", strlen(messageToken), messageToken);
		hasBody = true;
	}

	char *urlWithoutHttp = strrchr(requestedURL, ':');
	if(urlWithoutHttp != NULL)
	{
		urlWithoutHttp = urlWithoutHttp+3;
	
		//printf("url without http: %s\n", urlWithoutHttp);

		char *resourcePath = strchr(urlWithoutHttp, '/');
	
		if(resourcePath == NULL)
		{
			resourcePath = malloc(READ_BUFFER);
			strcpy(resourcePath, "/");
		}

		strcpy(requestedURL, resourcePath);
		//printf("After removing http and host name, Resource path: %s\n", requestedURL);

		if(hasBody)
			return 1;
		else
			return 0;
	}
	else
	{
		//printf("This hass %s\n", requestedURL);
		
		char * resourcePath = strchr(requestedURL, '/');
		//printf("resourcePath: %s\n", resourcePath);

		if(resourcePath == NULL)
		{
			requestedURL = malloc(READ_BUFFER);
			strcpy(requestedURL, "/");
			//printf("No http, no resource path. Resource path: %s\n", requestedURL);
			return 0;
		}

		strcpy(requestedURL, resourcePath);

		if(hasBody)
			return 1;
		else
			return 0;	
		
	}

}

//gets the IP address for the host name
char* convertAddresstoIP(char *address)
{
	char *ipAddress = malloc(READ_BUFFER);
	struct hostent *he;
	struct in_addr **addr_list;
	int i;

	if((he= gethostbyname(address)) == NULL)
	{
		printf("ERROR: Getting IP address.\n");
		return NULL;
	}

	//printf("after gethostbyname function.\n");
	addr_list = (struct in_addr **) he->h_addr_list;

	for(i = 0; addr_list[i]!= NULL; i++)
	{
		strcpy(ipAddress, inet_ntoa(*addr_list[i]));
		printf("MSG: IP address %s\n", ipAddress);
		return ipAddress;
	}

	return NULL;
}

//extracts the hostname from the given url
char* getHostAddress(const char *requestMessage, char * checkedURL, int getType)
{

  	char *hostname = malloc(READ_BUFFER);

  //extract host name from message body
  if(getType == 1)
  {
  	const char hostTag[10] = "Host";

  	hostname = strstr(requestMessage, hostTag);
  	//printf("hostName: %s\n", hostname);

  	hostname += 6;

  	char *tokens = strtok(hostname, "\r\n");

  	portNumber = 80;
}
//extract host name from request
else
{
	strcpy(hostname, requestMessage);

	char *tokens = strtok(hostname, " ");
	tokens = strtok(NULL, " ");

	strcpy(hostname, tokens);

	//check if it has http
	if(strncmp(hostname, "http://", 7) == 0)
	{
		hostname = hostname + 7;
	}

	tokens = strtok(hostname, "/");

	if(tokens != NULL)
	{
		strcpy(hostname, tokens);
	}
}
  printf("MSG: Hostname: %s\n", hostname);
  return hostname;
}

//calculate the md5 hash (hex) of the input file
//source: stackoverflow
char *str2md5(const char * str, int length)
{
  int n;
  MD5_CTX c;
  unsigned char digest[16];
  char * out = (char*) malloc(33);

  MD5_Init(&c);

  while(length > 0)
    {
      if(length > 512)
	     MD5_Update(&c, str, 512);
      else
	     MD5_Update(&c, str, length);

      length -=512;
      str += 512;
    }

  MD5_Final(digest, &c);

  for(n = 0; n<16; ++n)
    {
      snprintf(&(out[n*2]), 16*2, "%02x", (unsigned int)digest[n]);
    }

  return out;
}

//checks cache to see if the md5sum calculated for the request 
//is present in the cache
int checkCache(char *cacheId)
{
	//check if file is present
	int fd;

    char * tempCacheName = malloc(READ_BUFFER);
    strcpy(tempCacheName, ".");
    strcat(tempCacheName, cacheId);
    
	fd = open(tempCacheName, O_RDONLY);

	if(fd == -1)
	{
		printf("MSG: No data found in cache.\n");
		return 0;
	}

	//if file present, check the modified time
	struct stat attrib;
	stat(tempCacheName, &attrib);
	char date[26];
	strftime(date, 26, "%Y-%m-%d %H:%M:%S", localtime(&(attrib.st_ctime)));

	//create a file to get current time
	int testFd = open("test", O_RDWR | O_CREAT, 0777);
	close(testFd);

	struct stat attribCur;
	stat("test", &attribCur);
	char dateCur[26];
	strftime(dateCur, 26, "%Y-%m-%d %H:%M:%S", localtime(&(attribCur.st_ctime)));

	//printf("The difference is %ld\n", attribCur.st_ctime - attrib.st_ctime);

	remove("test");

	if(( attribCur.st_ctime - attrib.st_ctime)>=cacheTimeOut)
	{
		printf("MSG: Cache data timed out.\n");
		//removing the cached data because the data is not valid anymore
		remove(tempCacheName);
		return 0;
	}
	return 1;
}

//gives the md5sum for the request sent from the client
//www.google.com gets converted to md5sum hash
char* getCacheId(char * buf)
{
	char * tokens = strtok(buf, " ");
	tokens = strtok(NULL, " ");

	char  *md5sum = str2md5(tokens, strlen(tokens));
	//printf("MSG: md5sum calculated %s\n", md5sum);
	return md5sum;
}


//attempts to connect to the server to get the web-content
void connectToServer(char *ipAddress, char *hostname, int clientSockFd, char * cacheId)
{
	int socket_desc;
    struct sockaddr_in server;
    char *message = malloc(READ_BUFFER);
    char *server_reply = malloc(READ_BUFFER);
    int bytesRead;
     
    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    struct timeval tv;
    tv.tv_sec = 3;
  	tv.tv_usec = 0;
	setsockopt(socket_desc, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(struct timeval));
    
    if (socket_desc == -1)
    {
        printf("ERROR: Could not create socket to connect to server.\n");
        return;
    }
         
    server.sin_addr.s_addr = inet_addr(ipAddress);
    server.sin_family = AF_INET;
    server.sin_port = htons(80);
 
    //Connect to remote server
    if (connect(socket_desc , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        puts("ERROR: Connect error to the server.\n");
        return;
    }
     
    puts("MSG: Connected to server.\n");
     
    //Send request
    strcpy(message, "GET ");
    strcat(message, requestedURL);
    strcat(message, " HTTP/1.0\n");

    strcat(message, hostname);
    strcat(message, "\n\n");

    printf("MSG: Request being sent to server:\n\n%s", message);

    if( send(socket_desc , message , strlen(message) , 0) < 0)
    {
        puts("ERROR: Send failed.\n");
        return;
    }

    printf("MSG: Data sent successfully to server. Will cache at %s\n", cacheId);
     
    //Receive a reply from the server
    bytesRead = 0;
    char * messageRecv = malloc(READ_BUFFER);
    char * tempCacheName = malloc(READ_BUFFER);
    strcpy(tempCacheName, ".");
    strcat(tempCacheName, cacheId);

    int cacheFd = open(tempCacheName, O_RDWR | O_CREAT, 0777);
    int cacheWrite;

    while((bytesRead = recv(socket_desc, server_reply , READ_BUFFER , 0)) > 0)
    {
        //printf("MSG: Server Reply received.. Sending it to Client ...%d\n", bytesRead);
        cacheWrite = write(cacheFd, server_reply, bytesRead);

        if(cacheWrite < 0)
        {
        	printf("ERROR: Writing to the cacheId %s.\n", cacheId);
        }

        strncpy(messageRecv, server_reply, bytesRead);
    	//puts(messageRecv);
    	if(send(clientSockFd, server_reply, bytesRead,0)<0)
    	{
    		printf("ERROR: Sending to client error.\n");
    	}
    }

    close(cacheFd);
    printf("MSG: Disconnecting from the server.\n");
	shutdown(socket_desc, SHUT_RDWR);
	close(socket_desc);
}

//gets the body of the message (if present) after the request line
//i.e. GET url_request HTTP  <optional message header>, then this method gets
//<optional message header>
char * getMessageBody(char *clientRequest)
{
	char * messageBody = malloc(READ_BUFFER);

	char *tokens = strtok(clientRequest, "\n");

	tokens = strtok(NULL, "\n\r");

	strcpy(messageBody, tokens);
	strcat(messageBody ,"\n");
	tokens = strtok(NULL, "\n");

	while(tokens != NULL)
	{
		strcat(messageBody, tokens);
		strcat(messageBody, "\n");
		tokens = strtok(NULL, "\n");
		
	}

	return messageBody;
}

//fetches data from cache and sends to the client
//sever is not contacted in this case.
void sendFromCache(int connfd, char *cacheId)
{
	int bytesRead;
	int sentData;
	char *cacheContent = malloc(READ_BUFFER);

    char * tempCacheName = malloc(READ_BUFFER);
    strcpy(tempCacheName, ".");
    strcat(tempCacheName, cacheId);

	int cacheFd = open(tempCacheName, O_RDONLY);

	while((bytesRead = read(cacheFd, cacheContent, READ_BUFFER)) > 0){
	    
	    if((sentData = send(connfd, cacheContent, bytesRead, 0)<0))
    	  	{
	       		printf("ERROR: Cache sending data\n");
      		}

	    bzero(cacheContent, READ_BUFFER);
  }

  close(cacheFd);

}

//processing the  GET request
//if check for request type, url, http version passes then this
//method contacts the server to get web content
char* getProcessing(char *buf, int connfd)
{
	bool inCache = false;
	char *httpRequestError = malloc(READ_BUFFER);
	char *hostname = malloc(READ_BUFFER);
	char *resourcePath = malloc(READ_BUFFER);
	char *clientRequest = malloc(READ_BUFFER);
	char *cacheId = malloc(READ_BUFFER);
	char *storeDomainName = malloc(READ_BUFFER);
	char *messageBody = malloc(READ_BUFFER);
	
	//copying to retrieve data for different purposes
	strcpy(cacheId, buf);
	strcpy(clientRequest, buf);
	strcpy(storeDomainName, buf);
	strcpy(messageBody, buf);

	//checking the format of the request received
	if (checkMessageFormat(buf) == 1){

		//gets the Host: <name>
		hostname = getHostAddress(storeDomainName, hostname, 1);	
		messageBody = getMessageBody(messageBody);
	}
	else
	{
		hostname = malloc(READ_BUFFER);
		hostname = getHostAddress(storeDomainName, hostname, 0);
		strcpy(messageBody, "");
	}

	//checking whether request type is valid
	if(strlen(requestType) == 0 || strlen(requestType) == 1 || strcmp(requestType, "GET") != 0)
	{
		strcpy(httpRequestError, "400&invalidmethod");
		//printf("%s\n", httpRequestError);
		return httpRequestError;
	}/*
	else
	{
		printf("MSG: requestType: %s\n", requestType);
	}*/


	//check if there is a http version
	if(strlen(httpVersion) == 0 || strlen(httpVersion) == 1)
	{
		strcpy(httpRequestError, "400&httpinc");
		//printf("%s\n", httpRequestError);
		return httpRequestError;
	}/*
	else
	{	
		printf("RECV: httpVersion: %s\n", httpVersion);
	}
	*/

	if(strncmp(hostname, "invalid", 7) != 0)
	{

		ipAddress = malloc(READ_BUFFER);
		ipAddress = convertAddresstoIP(hostname);
		
		//check if it is present in cache
		cacheId = getCacheId(cacheId);

		if(ipAddress != NULL && checkCache(cacheId) == 0)
		{
			//printf("MSG: Either request not found in cache or cache timed out. Contacting the server.\n");
			connectToServer(ipAddress, messageBody, connfd, cacheId);
		}
		else if(ipAddress == NULL)
		{
			printf("ERROR: Don't connect. No IP address found.\n");
			strcpy(httpRequestError, "400&invalidurl");
			return httpRequestError;
		}
		else if(checkCache(cacheId) == 1)
		{
			sendFromCache(connfd, cacheId);
			printf("MSG: Found in cache at %s. Sending data from cache...\n", cacheId);
		}
		else
			printf("ERROR: Either IP is null or element is in cache.\n");
				
	}

	printf("MSG: Disconnecting with the client.\n");
	shutdown(connfd, SHUT_RDWR);
  	close(connfd);
	
	strcpy(httpRequestError, "200&OK\n");
	return httpRequestError;
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

  //close that session since error has been display to the client
  shutdown(connfd, SHUT_RDWR);
  close(connfd);
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
  else if(strncmp(errorType, "invalidurl", 10) == 0)
    {
      strcpy(errorMessage, "<html><body>ERROR: 400. Invalid URL: ");
    }
  else if(strncmp(errorType, "invalidmethod",13) == 0)
    {
      strcpy(errorMessage, "<html><body>ERROR: 400. Invalid Method: ");
    }
  
  strcat(errorMessage, message);
  strcat(errorMessage, " </body></html>\n"); 

  char *contentHeader;
  contentHeader = malloc(50);
  strcpy(contentHeader, "Content-Type: ");
  strcat(contentHeader, "text/html");
  strcat(contentHeader, "\n");

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

  //shut down the session since client got an error message
  shutdown(connfd, SHUT_RDWR);
  close(connfd);
}


void runProxyServer()
{
  int listenfd, connfd, n;
  pid_t childpid, parent_pid;
  int status;
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
    printf("%s\n","MSG: Received request...");


    if ( (childpid = fork ()) == 0 ) {//if it’s 0, it’s child process

      printf ("%s\n","MSG: Child created for dealing with client requests");

      //close listening socket
      close (listenfd);

      while ( (n = recv(connfd, buf, MAXLINE,0)) > 0)  {
	
		//printf("BUFFER: %s\n", buf);
		char * tempString = malloc(MAXLINE);
		strcpy(tempString, buf);
		char * getPostTag = strtok(tempString, " \n");

		if(n < 0)
	  	{
	    	printf("ERROR: invalid message received\n");
	  	}
		else if(strncmp(getPostTag, "GET", 3)==0)
	  	{
	  		printf("MSG: Client Request:\n %s", buf);
	  		char *getProcessingResponse = malloc(READ_BUFFER);
	  		getProcessingResponse = getProcessing(buf, connfd);
	  		printf("\n");
	  		//printf("GET processing response - %s\n", getProcessingResponse);
	    
	    	//break down the message to see if it error
	    	char * tokens = strtok(getProcessingResponse, "&");

	    	if(strcmp(tokens, "200") != 0)
	    	{
	    		tokens = strtok(NULL, "\n");
	    		//printf("400 error message %s\n", tokens);

	    		error400Message(connfd, tokens, "ERROR: 400 Bad Request.");
	    	}

		    bzero(buf, strlen(buf));
	    	bzero(httpVersion, strlen(httpVersion));
	    	bzero(requestedURL, strlen(requestedURL));
	    	bzero(requestType, strlen(requestType));
	    
	  	}
		else
	 	 {
	    	//printf("ERROR: request is not supported: %s\n", getPostTag);
	    	//printf("ERROR: msg: %s\n", buf);
	    	error501Message(connfd, getPostTag);

		  }


      }

      if (n < 0)
		printf("ERROR: Error receiving the client request.\n");

	//close socket of the server

	  //printf("MSG: exiting child fork.\n");
	  shutdown(connfd, SHUT_RDWR);
	  close(connfd);
      exit(EXIT_SUCCESS);
    }
    else
    {
    	//parent waits
    	//while((parent_pid = wait(&status)) >0);
    }

    printf("MSG: Waiting for a new request ...\n");
    
  }
  
}

int main (int argc, char ** argv)
{
	if(argc != 3)
	{
		printf("%s\n", "Command format: webproxy <Port #> <Timeout in seconds>.\nPlease try again!");
		return (EXIT_FAILURE);
	}

	//getting the port number
	portNumber = atoi(argv[1]);
	cacheTimeOut = atoi(argv[2]);
	runProxyServer();

	return (EXIT_SUCCESS);
}