/*
  Name: Rabin Ranabhat
  PA3
  Distributed Server
*/
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
#include <dirent.h>

#define READ_BUFFER 1024
#define NAME 100
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
char * clientUsername;
char * clientPassword;
char foundFileChunks[USERS][NAME];
pthread_mutex_t lock;
pthread_mutex_t lock2;

//for using stat and directory creation
struct stat st = {0};

//create directory if not present
void createDirectory(char * directoryName)
{
  if(stat(directoryName, &st) == -1)
  {
    mkdir(directoryName, 0700);
  }
}

void handlePut(int connfd)
{
  printf("\nMSG: Handling PUT command... \n");

 //pthread_mutex_lock(&lock);
 //get the file name
  int bytesRead;
  char buf[READ_BUFFER];
  char endOfMessage[READ_BUFFER];
  
  char *filename = malloc(READ_BUFFER);
  int fileFd2;
  int writtenBytes;

  char *fileLocation = malloc(READ_BUFFER);

  //receiving file name
  if((bytesRead = recv(connfd, buf, READ_BUFFER,0))<0)
  {
    printf("ERROR: Did not receive the filename - PUT.\n");
    close(connfd);
    return;
  }

  strncpy(filename, buf, bytesRead);

  bzero(buf, READ_BUFFER);
  strcpy(buf, "OK");

  if(send(connfd, buf, strlen(buf), 0)<0)
  {
    printf("ERROR: Did not ack for received filename %s\n.", filename);
    close(connfd);
    return;
  }

  //receiving folder
  if((bytesRead = recv(connfd, buf, READ_BUFFER,0))<0)
  {
    printf("ERROR: Did not receive the folder to put the file.\n");
    close(connfd);
    return;
  }

  char *folderName = malloc(READ_BUFFER);
  strncpy(folderName, buf, bytesRead);

    strcpy(fileLocation, rootFolder);
    strcat(fileLocation, "/");
    strcat(fileLocation, clientUsername);

  //int fileSizeBuffer = atoi(buf);
  if(strncmp(folderName, "null", 3) != 0)
  {
    //printf("Inside: %s %d\n", buf, bytesRead);
    char * tempDirectory = malloc(READ_BUFFER);
    strcpy(tempDirectory, fileLocation);
    strcat(tempDirectory, "/");
    strcat(tempDirectory, folderName);
    //create folder
    createDirectory(tempDirectory);

    //create a path name to open a file there
    //printf("%s\n", tempDirectory);
    //strcpy(fileLocation, "./");

    strcpy(fileLocation, tempDirectory);
    strcat(fileLocation, "/.");
  }
  else
  {
      //strcpy(fileLocation, "./");
    strcat(fileLocation, "/.");
  }

  strcat(fileLocation, filename);
  printf("MSG: Putting file in location: %s\n", fileLocation);

  remove(fileLocation);
  fileFd2 = open(fileLocation, O_RDWR | O_CREAT, 0777);

  if(fileFd2 == -1)
  {
    printf("ERROR opening file\n");
    return;
  }
  //printf("File Location: %s\n", fileLocation);
  

  char *fileContent = malloc(READ_BUFFER);
  //printf("Before file writing\n");

  int cumBytesRead = 0;
  //printf("RECEIVING: %s\n", fileLocation);
  //printf("OUTSIDE LOOP\n");
  while((bytesRead = recv(connfd, fileContent, READ_BUFFER, 0))>0){

  //printf("Bytes received %d\n", bytesRead);
  //writtenBytes = write(fileFd, fileContent, strlen(fileContent));
  cumBytesRead += bytesRead;

  if((writtenBytes = write(fileFd2, fileContent, bytesRead))<0)
  {
      printf("ERROR: Writing to file %s\n", fileLocation);
  }
  //bzero(fileContent, bytesRead);
  }

  //printf("END OF THE FILE\n");
  close(fileFd2);
  //printf("After file writing\n");


  //pthread_mutex_unlock(&lock);

  printf("MSG: Finished put at %s.\n\n", rootFolder);
  close(connfd);
}

void handleGet(int connfd)
{

  int bytesRead;
  int bytesSent;
  int fileFd;
  char* readBuffer = malloc(READ_BUFFER);
  char* sendBuffer = malloc(READ_BUFFER);
  char* fileContent = malloc(READ_BUFFER);

  char * directoryPath = malloc(NAME);
  char * filePath = malloc(NAME);

  char* filename = malloc(READ_BUFFER);
  char* message = malloc(NAME);
  char* foundSubStr = malloc(NAME);
  int foundFiles = 0;

  printf("\nMSG: Handling GET command ... \n");

  //get the file name from the client
  if((bytesRead = recv(connfd, readBuffer, READ_BUFFER,0))<0)
  {
    printf("ERROR: Receiving file name to handle GET.\n");
    return;
  }

    strcpy(filename, readBuffer);
    //printf("Filename sent %s\n", filename);

    bzero(readBuffer, READ_BUFFER);
    //getting the directory name

    if((bytesRead = recv(connfd, readBuffer, READ_BUFFER,0))<0)
    {
      printf("ERROR: Receiving the folder name GET.\n");
    }

    char *folderName = malloc(READ_BUFFER);
    strncpy(folderName, readBuffer, bytesRead);
    //printf("MSG: Destination folder received %s\n", folderName);

    DIR *d;
    struct dirent *dir;
    strcpy(directoryPath, ".");
    strcat(directoryPath, "/");
    strcat(directoryPath, rootFolder);
    strcat(directoryPath, "/");
    strcat(directoryPath, clientUsername);
    
    if(strncmp(folderName, "null", 3)!= 0)
    {
      strcat(directoryPath, "/");
      strcat(directoryPath, folderName);
    }

   //printf("Directory path %s\n", directoryPath);
   
  d = opendir(directoryPath);
  
  if(d)
  {
      while((dir = readdir(d)) != NULL)
      {
        //printf("%s\n", dir->d_name);
        foundSubStr = strstr(dir->d_name, filename);

        if(foundSubStr != NULL)
        {
          strcpy(foundFileChunks[foundFiles],dir->d_name);
          foundFiles++;

          //printf("Sub string: %s, true string: %s, path %s\n",foundSubStr, dir->d_name, foundFileChunks[foundFiles]);
          
        }
        /*
        else
          printf("Not substring %s\n", dir->d_name);
          */
      }

    closedir(d);
    }
    else if(ENOENT == errno)
    {
      printf("ERROR: Directory %s doesn't exist\n", directoryPath);
    }
    else
    {
      printf("ERROR: Opening the directory %s", directoryPath);
    }

    char *foundFilesStr;
    sprintf(foundFilesStr, "%d", foundFiles);

    printf("Total substrings found in %s: %s\n", directoryPath, foundFilesStr);

    bzero(message, READ_BUFFER);
    strcpy(message, foundFilesStr);

    if((bytesSent = send(connfd, message, strlen(message), 0))<0)
    {
      printf("ERROR: Error sending found message to client\n");
    }


    for(int i = 0; i<foundFiles; i++)
    {
      bzero(message, READ_BUFFER);
      strcpy(message, foundFileChunks[i]);
      //printf("Message sent %s\n", foundFileChunks[i]);

      if((bytesSent = send(connfd, foundFileChunks[i], strlen(foundFileChunks[i]), 0))<0)
      {
        printf("ERROR: Error sending found message to client\n");
      }

      bzero(message, READ_BUFFER);
      if((bytesRead = recv(connfd, message, READ_BUFFER, 0))<0)
      {
        printf("ERROR: Error receiving the file name\n");
      }

      //printf("Got message %s\n", message);

      //add before opening the files
      bzero(filePath, NAME);
      strcpy(filePath, directoryPath);
      strcat(filePath, "/");
      strcat(filePath, foundFileChunks[i]);

      //printf("Opening %s\n", filePath);
      
      int chunkFd = open(filePath, O_RDONLY);

      while((bytesRead = read(chunkFd, fileContent, READ_BUFFER))>0)
      {
        if((bytesSent = send(connfd, fileContent, bytesRead, 0))<0)
        {
          printf("ERROR: Error sending the file chunk\n");
        }
      }

      close(chunkFd);
      
      bzero(message, READ_BUFFER);
      if((bytesRead = recv(connfd, message, READ_BUFFER, 0))<0)
      {
        printf("ERROR: No ack for file chunk\n");
      }

      //printf("Got message %s\n", message);    
    }

  close(connfd); 
}

void handleList(int connfd)
{
  int bytesSent;
  int bytesRead;
  char *messageRecvd = malloc(READ_BUFFER);
  char *messageSent = malloc(READ_BUFFER);
  char *directoryPath = malloc(READ_BUFFER);
  char *directoryRecv = malloc(READ_BUFFER);

  printf("\nMSG: Handling LIST command ...\n");

  if((bytesRead = recv(connfd, messageRecvd, READ_BUFFER, 0))<0)
  {
    printf("ERROR: Receiving message for LIST command\n");
    close(connfd);
    return;
  }

  strncpy(directoryRecv, messageRecvd, bytesRead);

  DIR *d;
  struct dirent *dir;
  strcpy(directoryPath, ".");
  strcat(directoryPath, "/");
  strcat(directoryPath, rootFolder);
  strcat(directoryPath, "/");
  strcat(directoryPath, clientUsername);

  if(strncmp(directoryRecv, "NULL", 4) != 0)
  {
    strcat(directoryPath,"/");
    strcat(directoryPath, messageRecvd);
  }


  printf("MSG: Sending files of directory: %s\n\n", directoryPath);

  bzero(messageSent, READ_BUFFER);
  strcpy(messageSent, rootFolder);
  strcat(messageSent, "&");
  //search directory and then send all those values to the client
  d = opendir(directoryPath);
  
  if(d)
  {
      while((dir = readdir(d)) != NULL)
      {

        if(dir->d_type != DT_DIR)
        {
          //printf("%s is dir\n", dir->d_name);
          strcat(messageSent,dir->d_name);
          strcat(messageSent, "&");
        }          
      }

      closedir(d);
  } 
  else if(ENOENT == errno)
  {
      printf("ERROR: Directory %s doesn't exist\n", directoryPath);
  }
  else
  {
    printf("ERROR: Opening the directory %s\n", directoryPath);
  }

  //printf("Message sent%s\n", messageSent);

  if((bytesSent = send(connfd, messageSent, strlen(messageSent),0))<0)
  {
    printf("ERROR: Sending the list of directories for LIST\n");
    close(connfd);
    return;
  }

  close(connfd);
}

void getUserId(char *userId, int bytesLength, int connfd)
{
  int bytesSent;
  clientUsername = malloc(READ_BUFFER);
  char *message = malloc(READ_BUFFER);

  char *tokens = strtok(userId, "&\n");
  tokens = strtok(NULL, "&\n");
  strncpy(clientUsername, tokens, strlen(tokens));
  //printf("USERNAME: %s\n", clientUsername);

  for(int i = 0; i<usersTotal; i++)
  {
    if(strcmp(clientUsername, userNames[i]) == 0)
    {
      printf("MSG: User %s, authenticated.\n", userNames[i]);
      strcpy(message, "OK");
      break;
    }
    else
    {
      strcpy(message, "Invalid");
    } 
  }
  
  bytesSent = send(connfd, message, strlen(message), 0);

  if(bytesSent < 0)
  {
    printf("ERROR: Sending the ack to receiving userId.\n");
  }
}

void validateUser(char * usernameAndPassword, int bytesLength, int connfd)
{
  int bytesSent;
  char * tempUsernamePassword = malloc(bytesLength);
  char * message = malloc(READ_BUFFER);
  clientUsername = malloc(READ_BUFFER);
  clientPassword = malloc(READ_BUFFER);

  strncpy(tempUsernamePassword, usernameAndPassword, bytesLength);

  char * tokens = strtok(tempUsernamePassword, "&");
  tokens = strtok(NULL, "&");
  strcpy(clientUsername, tokens);
  tokens = strtok(NULL, "&");
  strcpy(clientPassword, tokens);

  //printf("username:%s\n", clientUsername);
  //printf("password:%s\n", clientPassword);

  //sleep(10);
  for(int i = 0; i<usersTotal; i++)
  {
    //printf("Inside loop: username %s, password %s\n", userNames[i], password[i]);
      if(strcmp(clientUsername, userNames[i]) == 0 && strcmp(clientPassword, password[i]) == 0)
      {
        printf("MSG: User %s, authenticated.\n", userNames[i]);
        strcpy(message, "valid");
        //printf("Inside Valid Crap\n");
        break;
      }
      else
      {
        //printf("Inside Invalid Crap\n");
        strcpy(message, "Invalid Username/Password. Please try agains.\n");
      }

  }

      bytesSent = send(connfd, message, strlen(message), 0);

      if(bytesSent < 0)
        printf("ERROR: Sending valid user message\n");
  //printf("MSG: Not found\n");
      //close(connfd);
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
  if ((listenfd = socket (AF_INET, SOCK_STREAM, 0)) <0)
  {
    printf("ERROR: Problem in creating the socket.\n");
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
      perror("ERROR: Setsockopt failed.\n");
    }
  
  //bind the socket
  if(bind (listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
    {
      perror("ERROR: error binding the socket.\n");
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
    printf("\n%s\n","MSG: Received request...");

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
        else if(strncmp(buf, "user", 4) == 0)
        {
          getUserId(buf, n, connfd);
        }
        else if(strncmp(buf, "PUT", 3) == 0)
        {
          handlePut(connfd);
        }
        else if(strncmp(buf, "GET", 3) == 0)
        {
          handleGet(connfd);
        }
        else if(strncmp(buf, "LIST", 4) == 0)
        {
          handleList(connfd);
        }
        else
        {
          printf("ERROR: Message not recognized, %s!. Msg: %s.\n", clientUsername, buf);
        }

        /*n = send(connfd, buf, strlen(buf), 0);

        if(n < 0)
          printf("ERROR: Sending message\n");
        else
          printf("MSG: Sent mesg: %s\n", buf);
        */
        bzero(buf, READ_BUFFER);
      }

      shutdown(connfd, SHUT_RDWR);
      close(connfd);
      //printf("\n");
      exit(EXIT_SUCCESS);

    }

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

  if(pthread_mutex_init(&lock, NULL) != 0)
  {
    printf("Mutex lock failed\n");
    exit(EXIT_FAILURE);
  }

  if(pthread_mutex_init(&lock2, NULL) != 0)
  {
    printf("Mutex lock failed\n");
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