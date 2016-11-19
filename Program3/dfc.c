#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>
#include <memory.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <time.h>
#include <openssl/md5.h>
#include <ctype.h>

#define READ_BUFFER 1024
#define MAXBUFSIZE 1024
#define SERVERS 4
#define FILES 4
#define FILENAME 100

//variables used across functions
char *serverRoots[SERVERS];
char *serverAddr[SERVERS];
char *fileSizes[SERVERS];
char fileChunkNames[SERVERS][FILENAME];
char recevFileChunks[SERVERS][FILENAME];
int recvFileCompleteness[SERVERS];
bool recvCompleteFile;
char *username;
char *password;

//char files[SERVERS][FILENAME];
char dfs1Files[FILENAME][FILENAME];
char dfs2Files[FILENAME][FILENAME];
char dfs3Files[FILENAME][FILENAME];
char dfs4Files[FILENAME][FILENAME];
char listCompFiles[FILENAME][FILENAME];
char listIncompFiles[FILENAME][FILENAME];
int dfs1Size;
int dfs2Size;
int dfs3Size;
int dfs4Size;
int listCompFilesSize;
int listIncompFilesSize;

int sendOrder[FILES];

char commands[3][FILENAME];
int commandsNum;
int fileHashVal;

//bit XOR encryption
int encryptDecriptFile(char * filename, char * key)
{
  char *outputFileName = malloc(FILENAME);
  strcpy(outputFileName, "e_");
  strcat(outputFileName, filename);

  int bytesRead, writtenBytes;
  int cumulativeBytesRead = 0;
  char *fileContent;
  int fd, outputFd;

  fd = open(filename, O_RDONLY);

  if(fd == -1)
      return -1;

  fileContent = malloc(READ_BUFFER);

  char * outputFileContent = malloc(READ_BUFFER);

  outputFd = open(outputFileName, O_RDWR | O_CREAT, 0777);

  //read the input file and write the encrypted file out
  while((bytesRead = read(fd, fileContent, READ_BUFFER))>0)
    {
      for(int i = 0; i<bytesRead; i++)
	{
	  outputFileContent[i] = fileContent[i]^key[i%(sizeof(key)/sizeof(char))];
	}

      writtenBytes = write(outputFd, outputFileContent, bytesRead);
      cumulativeBytesRead += bytesRead;
    }

  close(fd);
  close(outputFd);
  return 1;
}

void checkIntegrity(char *file1, char *file2)
{

}

//combine files to produce one single file
void combineFiles(char *filename)
{
  char * combinedFile = malloc(FILENAME);
  strcpy(combinedFile, "com_");
  strcat(combinedFile, filename);

  if(remove(combinedFile) != 0)
  {
    printf("ERROR: Removing file %s\n", combinedFile);
  }

  int combinedFileFd = open(combinedFile, O_RDWR | O_CREAT, 0777);

  if(combinedFileFd != -1)
    {
      printf("SUCCESS: No error for combined file.\n");
    }
  else
    {
      printf("ERROR: Error creating combined file.\n");
    }

  int chunkFd;
  int writtenToLargerFile;
  int bytesRead = 0;
  char * confFileContent = malloc(READ_BUFFER);

  for(int i = 0; i<FILES; i++)
    {

      printf("Filename: %s\n", recevFileChunks[i]);
      chunkFd = open(recevFileChunks[i], O_RDONLY);

      while((bytesRead = read(chunkFd, confFileContent, READ_BUFFER))> 0)
	{
	  writtenToLargerFile = write(combinedFileFd, confFileContent, bytesRead);
	}

      close(chunkFd);

      //remove the file chunk
      if(remove(recevFileChunks[i]) != 0)
      {
        printf("ERROR: Deleting chunk %s\n", recevFileChunks[i]);
      }
    }
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

//getting the hash of the input file
int getMd5Hash(char *filename)
{
  int bytesRead;
  int cumulativeBytesRead = 0;
  char *fileContent;
  int fileSize;
  int fd;

  fd = open(filename, O_RDONLY);
  fileSize =  lseek(fd, 0, SEEK_END);
  lseek(fd, 0, SEEK_SET);
  fileContent = malloc(fileSize);

  while((bytesRead = read(fd, fileContent, fileSize))>0)
    {
      cumulativeBytesRead += bytesRead;
    }

  close(fd);
	
  //calculate md5
  char * calculatedMd5 = str2md5(fileContent, cumulativeBytesRead/sizeof(char));
  printf("Md5 Hash %s\n", calculatedMd5);
	
  unsigned long result = 0;
  int length = strlen(calculatedMd5);

  //convert the hash string to int
  //strcpy(calculatedMd5, "bcdef0aa");
  int charVal = 0;
  for(int i = 0; i<length; i++)
    {

      charVal = calculatedMd5[i]- '0';
      
      if(isalpha(calculatedMd5[i]))
      {
        charVal -= 39;
        //printf("char: %c, a-val: %d\n", calculatedMd5[i], charVal);
      }
      else if(isdigit(calculatedMd5[i]))
      {
        //printf("char: %c, d-val: %d\n", calculatedMd5[i], charVal);
      }

      result = result*16 + (charVal);
    }

  printf("result: %lu\n", result);
  result = result % SERVERS;
  //printf("result: %d\n", result);

  return result;
}

//split the input file into 4 chunks
//split the file in x-chunks
int splitFile(char * filename)
{

  int bytesRead;
  char * confFileContent = malloc(READ_BUFFER);

  int fd = open(filename, O_RDONLY);
  long smallerFileSize = lseek(fd, 0, SEEK_END);
  smallerFileSize = smallerFileSize/FILES;
  lseek(fd,0, SEEK_SET);

  long cumulativeSize = 0;
  long shortFileWritten = 0;
  int smallerFileFd;
  int currentFileNum = 0;
  char smallFileName[FILENAME];

  strcpy(filename, filename+2);
  //strcpy(filename, filename++)
  sprintf(smallFileName, "%s.%d", filename, currentFileNum+1);
  //sprintf(smallFileName, "%d",currentFileNum);
  printf("%s\n", smallFileName);

  if(fd != -1)
    {

      strcpy(fileChunkNames[currentFileNum],smallFileName);
      printf("TESTING INSIDE %s\n", fileChunkNames[currentFileNum]);

      smallerFileFd = open(smallFileName, O_RDWR | O_CREAT, 0777);

      if(smallerFileFd == -1)
	       printf("ERROR: Opening the file %s\n", smallFileName);

  while((bytesRead = read(fd, confFileContent, READ_BUFFER)) > 0)
	{
	  cumulativeSize += bytesRead;

	  shortFileWritten = write(smallerFileFd, confFileContent, bytesRead);

	  if(shortFileWritten == -1)
	    printf("ERROR: Writing to file %s\n", smallFileName);


	  if(cumulativeSize >= smallerFileSize)
	    {
	      close(smallerFileFd);
	      currentFileNum++;

	      sprintf(smallFileName, "%s.%d",filename, currentFileNum+1);
	      printf("%s\n", smallFileName);

        strcpy(fileChunkNames[currentFileNum],smallFileName);
    
	      smallerFileFd = open(smallFileName, O_RDWR | O_CREAT, 0777);

	      if(smallerFileFd == -1)
		      printf("ERROR: Opening the file %s\n", smallFileName);

	      cumulativeSize = 0;
	    }
			
	}

      //printf("Outside\n");
      close(smallerFileFd);

      close(fd);
    }
  else
    {
      printf("ERROR: Opening the file %s\n", filename);
      return -1;
    }

    printf("TESTING %s\n",fileChunkNames[2]);

    return 1;
}

void processPUT(int sock, char * message, char * destinationFolder)
{

  char server_reply[READ_BUFFER];
  int fileFd;

  //printf("SENDING %s\n", message);
  fileFd = open(message, O_RDONLY);

  if(fileFd == -1)
    {
      printf("ERROR: PUT cannot open the file %s\n", message);
      close(sock);
      return;
    }

  //sending the file chunk name
  if(send(sock, message, strlen(message), 0)<0)
    {
      printf("ERROR: PUT cannot send filename %s.\n", message);
      close(sock);
      return;
    }

  //resetting the buffer
  bzero(message, sizeof(message));
  
  //awaiting ack from the server
  if(recv(sock, message, READ_BUFFER, 0)<0)
  {
    if(errno != EAGAIN && errno != EWOULDBLOCK)
      printf("ERROR: PUT recv failed for acknowledging filename.\n");
    else
      printf("ERROR: PUT recv timeout.\n");
       
    close(sock);
    return;    
  }
  //printf("%s\n", message);

  char *fileContent = malloc(READ_BUFFER);
  int bytesRead;
  int cumReadSize = lseek(fileFd, 0, SEEK_END);
  lseek(fileFd, 0, SEEK_SET);

  //printf("Cum size: %d\n", cumReadSize);

  //resetting the buffer
  bzero(message,sizeof(message));
	
  //sending the destination folder, if need be		
  if(commandsNum == 3)
  {
    sprintf(message, "%s", destinationFolder);
  }
  else
  {
    sprintf(message, "%s", "null");
  }

  if(send(sock, message, strlen(message), 0)<0)
  {
    printf("ERROR: PUT cannot send the destination folder name.\n");
    close(sock);
    return;
  }

  printf("DESTINATION FOLDER %s\n", message);

  //resetting the filecontent buffer and the send message buffer
  bzero(fileContent, READ_BUFFER);
  bzero(message, strlen(message));


  cumReadSize = 0;
  int sentData = 0;
  int cumSent = 0;

  while((bytesRead = read(fileFd, fileContent, READ_BUFFER)) > 0){
    cumReadSize += bytesRead;
    if((sentData = send(sock, fileContent, bytesRead, 0)<0))
      {
	       printf("ERROR: PUT sending data\n");
      }

      //printf("%s\n", fileContent);
    /*
      else
      {
      cumSent += sentData;
      printf("Sent so far: %d\n", sentData);
      }
    */
    bzero(fileContent, READ_BUFFER);
  }

  //printf("\n\n");
  //printf("Final cum size: %d cum sent: %d\n", cumReadSize, cumSent);
  close(fileFd);

  bzero(&(server_reply), sizeof(server_reply));
  sleep(0.5);
  printf("Closing\n");
  close(sock); 
}

void processGET(int sock, char *filename, char *destinationFolder)
{
  //printf("Client handling GET\n");
  
  int bytesSent;
  int bytesRead;
  char* sendBuffer = malloc(READ_BUFFER);
  char* readBuffer = malloc(READ_BUFFER);
  char* fileContent = malloc(READ_BUFFER);

  int numberOfFilesFound = 0;

  strcpy(sendBuffer, filename);
  //printf("MSG: Sending filename %s\n", filename);
	
  //sending the filename
  if((bytesSent = send(sock, filename, READ_BUFFER, 0)) <0)
    {
      printf("ERROR: Sending message - process GET\n");
      return;
    }

    bzero(readBuffer, READ_BUFFER);
    bzero(sendBuffer, READ_BUFFER);

  //sending the destination folder, if need be    
  if(commandsNum == 3)
  {
    sprintf(sendBuffer, "%s", destinationFolder);
  }
  else
  {
    sprintf(sendBuffer, "%s", "null");
  }

  if(send(sock, sendBuffer, strlen(sendBuffer), 0)<0)
  {
    printf("ERROR: GET cannot send the folder name.\n");
    return;
  }

  //how many files are found
  if((bytesRead = recv(sock, readBuffer, READ_BUFFER, 0))<0)
  {
    printf("ERROR: Error receiving whether file found or not\n");
    return;
  }

  /*if(strncmp(readBuffer, "0", 1) == 0)
  {
    printf("File not found\n");
    return;
  }
  else
  {
    //bzero(sendBuffer, READ_BUFFER);

    //strncpy(sendBuffer, readBuffer, bytesRead);
    //printf("File(s) found %s\n", sendBuffer);
  }*/

  //number of substrings found in the server matching
  //input string
  numberOfFilesFound = atoi(readBuffer);

  if(numberOfFilesFound != 0)
  {
    printf("Files found %d\n", numberOfFilesFound);
  }
  else
  {
    printf("Files not found\n");
    return;
  }

  /*
  //names of the files
  bzero(readBuffer, READ_BUFFER);
  if((bytesRead = recv(sock, readBuffer, READ_BUFFER, 0))<0)
  {
    printf("ERROR: Error receiving the file name\n");
    return;
  }

  printf("Found name received %s\n", readBuffer);
  */

  for(int i = 0; i<numberOfFilesFound; i++)
  {
    bzero(readBuffer, READ_BUFFER);
    if((bytesRead = recv(sock, readBuffer, READ_BUFFER, 0))<0)
    {
      printf("ERROR: Error receiving the file chunk name\n");
      continue;
    }

    printf("Found file chunk named %s\n", readBuffer);
    //strcpy(recevFileChunks[i],readBuffer);

    bzero(sendBuffer, READ_BUFFER);
    strcpy(sendBuffer, "OK");
    //printf("MSG: Sending filename %s\n", filename);
  
    //acknowledging the file name receipt
    if((bytesSent = send(sock, sendBuffer, strlen(sendBuffer), 0)) <0)
    {
      printf("ERROR: acknowledging file chunk receipt\n");
      continue;
    }

    char *recvFileChunkName = malloc(FILENAME);
    strcpy(recvFileChunkName, "._chunk_");
    strcat(recvFileChunkName, readBuffer);

    //get the chunk #
    //printf("Chunk # %c\n", recvFileChunkName[strlen(recvFileChunkName)-1]);
    int arrayIndex = recvFileChunkName[strlen(recvFileChunkName)-1] - '0';
    arrayIndex--;

    //at the index calculated, if it is empty then copy at the same
    //index the name of the file opened
    if(recvFileCompleteness[arrayIndex] != 1)
    {
      recvFileCompleteness[arrayIndex] = 1;
      strcpy(recevFileChunks[arrayIndex],recvFileChunkName);
    }

    int chunkFd = open(recvFileChunkName, O_RDWR | O_CREAT, 0777);

    while((bytesRead = recv(sock, readBuffer, READ_BUFFER, 0)) > 0)
    {
      if((write(chunkFd, readBuffer, bytesRead))<0)
      {
        printf("ERROR: Writing to the file chunk %s", recvFileChunkName);
        continue;
      }
    }

    close(chunkFd);

    bzero(sendBuffer, READ_BUFFER);
    strcpy(sendBuffer, "OK");
    //printf("MSG: Sending filename %s\n", filename);
  
    //acknowledging the file name receipt
    if((bytesSent = send(sock, sendBuffer, strlen(sendBuffer), 0)) <0)
    {
      printf("ERROR: acknowledging file chunk receipt\n");
      continue;
    }
  }

}

void processLIST(int sock, char * destinationFolder)
{
  int bytesRead;
  int bytesSent;
  char * messageRecvd = malloc(READ_BUFFER);
  char * messageSent = malloc(READ_BUFFER);

  char *folderToSearch = malloc(FILENAME);
  strcpy(folderToSearch, "NULL");

  if(strncmp(destinationFolder, "NULL", 3)  != 0)
  {
    printf("Destination folder %s\n", destinationFolder);
    strcpy(folderToSearch, destinationFolder);
  }

  //sending the folder to search
  if((bytesSent = send(sock, folderToSearch, strlen(folderToSearch), 0))<0)
  {
    printf("ERROR: Sending the folder to search for LIST command\n");
    close(sock);
    return;
  }

  bzero(messageRecvd, READ_BUFFER);

  if((bytesRead = read(sock, messageRecvd, READ_BUFFER))<0)
  {
    printf("ERROR: Receiving the file list for LIST command\n");
    close(sock);
    return;
  }

  char *tokens = strtok(messageRecvd, "&\n");
  int listIndex = tokens[(strlen(tokens)-1)]-'0';
  //printf("token %s, arrayIndex %d, char: %c\n", tokens, listIndex, tokens[strlen(tokens-1)]);
  tokens = strtok(NULL, "&\n");

  int elementIndex = 0;

  while(tokens != NULL)
  {
    tokens++;
      //printf("TOKENS: %s\n", tokens);

      if(listIndex == 1)
      {
        strcpy(dfs1Files[dfs1Size], tokens);
        dfs1Size++;
      }
      else if(listIndex == 2)
      {
        strcpy(dfs2Files[dfs2Size], tokens);
        dfs2Size++;
      }
      else if(listIndex == 3)
      {
        strcpy(dfs3Files[dfs3Size],tokens);
        dfs3Size++;
      }
      else if(listIndex == 4)
      {
        strcpy(dfs4Files[dfs4Size], tokens);
        dfs4Size++;
      }

    tokens = strtok(NULL, "&\n");
  }
  //printf("LIST:\n%s\n\n", messageRecvd);

  close(sock);
}

bool validateUser(int sock)
{
  char * usernameAndPassword = malloc(FILENAME);
  char * server_reply = malloc(FILENAME);

  bzero(usernameAndPassword, FILENAME);
  strcpy(usernameAndPassword, "valid&");
  strcat(usernameAndPassword, username);
  strcat(usernameAndPassword, "&");
  strcat(usernameAndPassword, password);

  bzero(server_reply, FILENAME);

  if( send(sock, usernameAndPassword, strlen(usernameAndPassword), 0) < 0)
    {
      printf("ERROR: Sending username and password\n");
      return false;
    }
  else
    {
      //server reply
      if( recv(sock , server_reply , FILENAME , 0) < 0)
        {
	         if(errno != EAGAIN && errno != EWOULDBLOCK)
	           printf("ERROR: Recv failed for user authorization.\n");
	         else
	           printf("ERROR: Timeout\n");
	         
           return false;
        }
      else
        {
	         if(strncmp(server_reply, "valid",5) == 0)
	         {
	           //printf("%s\n", server_reply);
	           return true;
	         }
	         
	         else
	           printf("Server %s\n", server_reply);
	         
        }
    }

  return false;
}

bool checkCompleteness()
{
  for(int i = 0; i<SERVERS; i++)
  {
    if(recvFileCompleteness[i] == 0)
      return false;
  }

  return true;
}

void resetCompleteness()
{
    //no chunks have been received
  for(int i = 0; i<SERVERS; i++)
  {
    //remove(recevFileChunks[i]);
    recvFileCompleteness[i] = 0;
  }
}

int returnChunkNum(char * string)
{
  int chunkNum = string[strlen(string)-1] - '0';
  chunkNum--;

  return chunkNum;
}

bool checkIncompleteFileList(char * input)
{
  char *element = malloc(FILENAME);
  strcpy(element, input);
  element[strlen(element)-1] = 0;
  element[strlen(element)-1] = 0;
  
  for(int i =0; i<listIncompFilesSize; i++)
  {
    if(strcmp(listIncompFiles[i], element) == 0){
      //printf("ENTERING INC: %s, %s\n", listIncompFiles[i], element);
      return true;
    }
  }

  return false;

}

bool checkCompleteFileList(char *input)
{
  char *element = malloc(FILENAME);
  strcpy(element, input);
  element[strlen(element)-1] = 0;
  element[strlen(element)-1] = 0;

  for(int i =0; i<listCompFilesSize; i++)
  {
    if(strcmp(listCompFiles[i], element) == 0){
      //printf("ENTERING COMP: %s, %s\n", listCompFiles[i], element);
      return true;
    }
  }

  return false;
}

void findElement(char * input)
{

  if(checkCompleteFileList(input))
    return;

  if(checkIncompleteFileList(input))
    return;

  //printf("Passes first checks\n");

  char *firstElement = malloc(FILENAME);
  char *secondElement = malloc(FILENAME);

  strcpy(firstElement, input);

  //printf("Element: %s\n\n ", element);
  recvFileCompleteness[returnChunkNum(firstElement)] = 1;
  firstElement[strlen(firstElement)-1] = 0;
  //printf("Stripped element %s\n", firstElement);

  //check in list1
  for(int i = 0; i<dfs1Size; i++)
  {
    
    strcpy(secondElement, dfs1Files[i]);
    int index = returnChunkNum(secondElement);
    secondElement[strlen(secondElement)-1] = 0;

    //printf("First Element %s, Second Element %s\n", firstElement, secondElement);

    if(strcmp(firstElement, secondElement) == 0)
    {
      recvFileCompleteness[index] = 1;
    }
  }

  /*printf("List 1 %s\n", input);
  for(int i = 0; i<4; i++)
  {
    printf("%d ", recvFileCompleteness[i]);
  }

  printf("\n");
*/
  //check if all the chunk found
  if(checkCompleteness())
    return;

    //check in list2
  for(int i = 0; i<dfs2Size; i++)
  {
    
    strcpy(secondElement, dfs2Files[i]);
    int index = returnChunkNum(secondElement);
    secondElement[strlen(secondElement)-1] = 0;

    //printf("First Element %s, Second Element %s\n", firstElement, secondElement);

    if(strcmp(firstElement, secondElement) == 0)
    {
      recvFileCompleteness[index] = 1;
    }
  }

  /*printf("List 2 %s\n", input);
  for(int i = 0; i<4; i++)
  {
    printf("%d ", recvFileCompleteness[i]);
  }

  printf("\n");
*/

  //check if all the chunk found
  if(checkCompleteness())
    return;

    //check in list3
  for(int i = 0; i<dfs3Size; i++)
  {
    
    strcpy(secondElement, dfs3Files[i]);
    int index = returnChunkNum(secondElement);
    secondElement[strlen(secondElement)-1] = 0;

    //printf("First Element %s, Second Element %s\n", firstElement, secondElement);

    if(strcmp(firstElement, secondElement) == 0)
    {
      //printf("My index %d %s\n", index, secondElement);
      recvFileCompleteness[index] = 1;
    }
  }

  /*printf("List 3 %s\n", input);
  for(int i = 0; i<4; i++)
  {
    printf("%d ", recvFileCompleteness[i]);
  }

  printf("\n");
*/

  //check if all the chunk found
  if(checkCompleteness())
    return;

      //check in list4
  for(int i = 0; i<dfs4Size; i++)
  {
    
    strcpy(secondElement, dfs4Files[i]);
    int index = returnChunkNum(secondElement);
    secondElement[strlen(secondElement)-1] = 0;

    //printf("First Element %s, Second Element %s\n", firstElement, dfs4Files[i]);

    if(strcmp(firstElement, secondElement) == 0)
    {
      recvFileCompleteness[index] = 1;
    }
  }

  /*printf("List 4 %s\n", input);
  for(int i = 0; i<4; i++)
  {
    printf("%d ", recvFileCompleteness[i]);
  }

  printf("\n");
*/


  //check if all the chunk found
  if(checkCompleteness())
    return;

}

void connectToServer(char * serverIP, char* portNum, char * messageTag, char * message, char * destinationFolder)
{

  int sock;
  struct sockaddr_in server;
  //char message[READ_BUFFER];
  int read_size;
  bool loopControl;
    
  fd_set fdset;
  struct timeval tv;
  long arg;
  socklen_t lon;
  int valopt;

  tv.tv_sec = 1;
  tv.tv_usec = 0;


  //Create socket
  sock = socket(AF_INET , SOCK_STREAM , 0);
  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(struct timeval));
  arg = fcntl(sock, F_GETFL, NULL);
  arg |= O_NONBLOCK;
  fcntl(sock, F_SETFL, arg);

  if (sock == -1)
    {
      printf("ERROR: Could not create socket %s:%s\n\n", serverIP, portNum);
      close(sock);
      return;
    }

  printf("MSG: Socket created %s:%s\n", serverIP, portNum);

  server.sin_addr.s_addr = inet_addr(serverIP);
  server.sin_family = AF_INET;
  server.sin_port = htons(atoi(portNum));

  //Connect to remote server
  int res = connect(sock , (struct sockaddr *)&server , sizeof(server));

  if (res < 0)
    {
      if(errno == EINPROGRESS)
    	{
	  //for timeout
	  FD_ZERO(&fdset);
	  FD_SET(sock, &fdset);

	  if(select (sock+1, NULL, &fdset, NULL, &tv) > 0)
	    {
	      lon = sizeof(int);
	      getsockopt(sock, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon);

	      if(valopt)
	    	{
    		  printf("ERROR: Error in connection(). %s:%s\n\n", serverIP, portNum); 
		      close(sock);
		      return;   		
	    	}
	    }
	    
	  else
	    {

	      printf("ERROR: Timeout. %s:%s\n\n", serverIP, portNum);
	      close(sock);
	      return;
	    }
	}
      else
	{
	  printf("ERROR: Error connectiong\n\n");
	  close(sock);
	  return;
	}

    }

  printf("MSG: Connected. %s:%s\n", serverIP, portNum);
  //close(sock);
  //return;


  //makes you non blocking again
  arg = fcntl(sock, F_GETFL, NULL);
  arg &=(~O_NONBLOCK);
  fcntl(sock, F_SETFL, arg);

  loopControl = validateUser(sock);

  if(!loopControl)
  {
    printf("ERROR: Invalid username and/or password. Please check dfc.conf file for %s:%s\n\n", serverIP, portNum);
    close(sock);
    return;
  }
  else
    printf("MSG: Username and password authenticated. %s:%s\n", serverIP, portNum);
        
  //determine if it is put/get or list
  //Send message tag
  if( send(sock , messageTag , strlen(messageTag) , 0) < 0)
    {
      printf("ERROR: Send failed\n");
      //return;
    }
  else
    {
      printf("MSG: Sending message ..\n");
    }

  if(strcmp(messageTag, "PUT") == 0)
  {
    processPUT(sock, message, destinationFolder);
  }
  else if(strcmp(messageTag, "GET") == 0)
  {
    //printf("Searching for %s in %s:%s\n", message, serverIP, portNum);
    processGET(sock, message, destinationFolder);

    for(int i = 0; i<SERVERS; i++)
    {
      printf("Recv index %d, val: %d\n", i, recvFileCompleteness[i]);
    }
  }
  else if(strncmp(messageTag, "LIST", 4) == 0)
  {
    //*destinationFolder = '\0';
    processLIST(sock, destinationFolder);
  }

  printf("\n\n");
}

void checkList(int size, char array[FILENAME][FILENAME])
{
  /*for(int i = 0; i<size; i++)
  {
    printf("%s  ", array[i]);
  }

  printf("\n\n");
  */

  if(size > 0)
       {
          for(int i = 0; i<size; i++)
          {
            //printf("Inside the loop of list\n");
            resetCompleteness();
            findElement(array[i]);

            if(checkCompleteFileList(array[i]))
            {
              continue;
            }
            else if(checkIncompleteFileList(array[i]))
            {
              continue;
            }
            else if(checkCompleteness() && !checkCompleteFileList(array[i]))
            {
              char * tempString = malloc(FILENAME);
              strcpy(tempString, array[i]);
              tempString[strlen(tempString)-1] = 0;
              tempString[strlen(tempString)-1] = 0;
              strcpy(listCompFiles[listCompFilesSize], tempString);
              listCompFilesSize++;
              printf("%s\n\n",tempString);

            }
            else if(!checkCompleteness() && !checkIncompleteFileList(array[i]))
            {
              char * tempString = malloc(FILENAME);
              strcpy(tempString, array[i]);
              tempString[strlen(tempString)-1] = 0;
              tempString[strlen(tempString)-1] = 0;
              strcpy(listIncompFiles[listIncompFilesSize], tempString);
              listIncompFilesSize++;
              
              printf("%s[incomplete]\n\n", tempString);
            }
          }
        }
}

void runClient()
{
  char *readBuffer = malloc(READ_BUFFER);
  char *message = malloc(READ_BUFFER);
  char *messageTag = malloc(READ_BUFFER);
  char * destinationFolder = malloc(READ_BUFFER);

  char *serverAddrs[SERVERS];
  char *serverPorts[SERVERS];

  bool send = true;

  for(int i = 0; i<SERVERS; i++)
    {
 		
      char *tokens = strtok(serverAddr[i], ":");
      serverAddrs[i] = tokens;

      tokens = strtok(NULL, "\n");
      serverPorts[i] = tokens;

    }

  //split the file

  while(1)
  {

    printf("Enter message : ");

    //resetting the variables after each loop
    bzero(readBuffer, READ_BUFFER);
    bzero(messageTag, READ_BUFFER);
    bzero(message, READ_BUFFER);
    resetCompleteness();
    *message = '\0';
    *destinationFolder = '\0';
    send = true;
    commandsNum = 0;


    dfs1Size = 0;
    dfs2Size = 0;
    dfs3Size = 0;
    dfs4Size = 0;
    listCompFilesSize = 0;
    listIncompFilesSize = 0;

    fgets(readBuffer, READ_BUFFER, stdin);
    
    char * tokens = strtok(readBuffer, " \n");
     while(tokens != NULL)
     {
      //save
      strcpy(commands[commandsNum], tokens);
      commandsNum++;
      tokens = strtok(NULL, " \n");
     }


    strcpy(messageTag, commands[0]);

    if(strncmp(readBuffer, "PUT", 3) == 0 && commandsNum != 1)
    {

    strcpy(message, commands[1]);

     
    if(commandsNum == 3)
    {
      strcpy(destinationFolder, commands[2]);
      //printf("destination folder %s\n", destinationFolder);
    }

    if(encryptDecriptFile(message, password) == -1)
    {
      printf("ERROR: File %s not found. Please check your local directory.\n", message);
      continue;
    }

    char *encryptedFile = malloc(READ_BUFFER);
    strcpy(encryptedFile, "e_");
    strcat(encryptedFile, message);

      //tokens = strtok(NULL, " \n");
      
	//encrypt file - complete this later
  /*
	encryptDecriptFile(message, password);

	char *testingEncrypt = malloc(READ_BUFFER);
	strcpy(testingEncrypt, "e_");
	strcat(testingEncrypt, message);

	encryptDecriptFile(testingEncrypt, password);
*/
	if(splitFile(encryptedFile) == -1)
  {
      continue;
  }

	fileHashVal = getMd5Hash(message);


	/*printf("Total num %d\n", putCommandsNum);
	for(int i = 0; i<putCommandsNum; i++)
	  {
	    printf("%s\n",putCommands[i]);
	  }
    */

      }
    else if(strncmp(readBuffer, "GET", 3) == 0 && commandsNum != 1)
      {
          //getting the file name
          strcpy(message, commands[1]);

          if(commandsNum == 3)
          {
            strcpy(destinationFolder, commands[2]);
          }
      }
    else if(strncmp(readBuffer, "LIST", 4) == 0)
      {
	       strcpy(messageTag, "LIST");
         *message = '\0';

         if(commandsNum == 2)
         {
          strcpy(destinationFolder, commands[1]);
         }
         else
         {
           strcpy(destinationFolder, "NULL");
         }
      }
      else if(strncmp(readBuffer, "CALC", 4) == 0)
      {
        int val = -1;
        printf("Enter the calcuated value:");
        scanf("%d", &val);

        int startIndex = (SERVERS-val)%SERVERS;
        for(int i = 0; i<SERVERS; i++)
        {
          int firstPart = startIndex;
          int secondPart = (startIndex+1)%SERVERS;
          printf("first %d, second %d\n", sendOrder[firstPart], sendOrder[secondPart]);

          startIndex++;
          startIndex = startIndex%SERVERS;
        }
      }
       else
      {
	       printf("ERROR: Unrecognized message %s\n", readBuffer);
	       send = false;
      }

    int startIndex = (SERVERS-fileHashVal)%SERVERS;
    printf("file val %d\n", fileHashVal);  
    //sending the GET message to all the servers 
    for(int i = 0; i<FILES && send; i++)
      {
       				
      	//printf("serverAddr:%s, serverPort: %s\n", serverAddrs[i], serverPorts[i]);
      	if(strncmp(readBuffer, "PUT", 3) == 0)
      	  {

            bzero(message, READ_BUFFER);
      	    //FIX THIS.... JUST put the value in array and then rotate then by the
      	    //file HashVal
      	    //fileHashVal = 3;
       
            int firstPart = startIndex;
            int secondPart = (startIndex+1)%SERVERS;

      	    strcpy(message, fileChunkNames[firstPart]);
      	    printf("File %s\n", fileChunkNames[firstPart]);
      	    connectToServer(serverAddrs[i], serverPorts[i], messageTag, message, destinationFolder);
      				
      	    bzero(message, READ_BUFFER);
      	    strcpy(message, fileChunkNames[secondPart]);
      	    printf("File %s\n", fileChunkNames[secondPart]);

            startIndex++;
            startIndex = (startIndex)%SERVERS;

      	  }
      	else if (strncmp(readBuffer, "GET", 3) == 0)
      	  {
            //printf("GET message %s\n", message);
      	    //strcpy(message, "foo1.txt");
      	  }

      	connectToServer(serverAddrs[i], serverPorts[i], messageTag, message, destinationFolder);
    
    }

    if(strncmp(readBuffer, "GET",3) == 0 && send){
      if(checkCompleteness()){
        if(strncmp(readBuffer, "GET", 3) == 0)
        {
          printf("Combine files %s and check md5 sum\n", message);
          combineFiles(message);

          char * encryptedRecv = malloc(FILENAME);
          strcat(encryptedRecv, "com_");
          strcat(encryptedRecv, message);

          encryptDecriptFile(encryptedRecv, password);
        }
      }
      else
      {
        printf("MSG: File %s is not complete\n", message);
      }
    }
    else if(strncmp(readBuffer, "PUT",3) == 0 && send)
    {
      for(int i = 0; i<SERVERS; i++)
      {
        printf("File chunk names %s\n", fileChunkNames[i]);
        if(remove(fileChunkNames[i]) != 0)
        {
          printf("ERROR: Removing %s\n", fileChunkNames[i]);
        }
      }
    }
    else if(strncmp(readBuffer, "LIST", 4) == 0 && send)
    {
      //checking for files list in each server directory
      checkList(dfs1Size, dfs1Files);
      checkList(dfs2Size, dfs2Files);
      checkList(dfs3Size, dfs3Files);
      checkList(dfs4Size, dfs4Files);

      if(listIncompFilesSize  == 0 && listCompFilesSize == 0)
      {
        printf("\nMSG: The server folders are either empty or the directory doesn't exist.\n");
      }
    }
    else
    {
      printf("HMmmmm... %s\n", readBuffer);
    }

  }

}
     

void parseConfigFile(char * filename)
{
  int bytesRead, fd;
  char * confFileContent = malloc(READ_BUFFER);
  int serverIndex = 0;
	
  username = malloc(READ_BUFFER);
  password = malloc(READ_BUFFER);

  fd = open(filename, O_RDONLY);

  if(fd != -1)
    {
      while((bytesRead = read(fd, confFileContent, READ_BUFFER)) > 0)
	{
	  char *tokens = strtok(confFileContent, " \n");

	  while(tokens != NULL)
	    {
	      if(strcmp(tokens, "Server") == 0)
		{
		  tokens = strtok(NULL, " \n");
		  serverRoots[serverIndex] = tokens;

		  tokens = strtok(NULL, " \n");
		  serverAddr[serverIndex] = tokens;

		  serverIndex++;
		}
	      else if(strcmp(tokens, "Username:") == 0)
		{
		  tokens = strtok(NULL, " \n");
		  username = tokens;
		}
	      else if(strcmp(tokens, "Password:") == 0)
		{
		  tokens = strtok(NULL, " \n");
		  password = tokens;
		}

	      tokens = strtok(NULL, " \n");
	    }
	}
    }
  else
    {
      printf("ERROR: Cannot find the file %s. Please check your local directory.\n", filename);
    }

  close(fd);

  ////----DEBUG
  /*
    for(int i = 0; i<SERVERS; i++)
    {
    printf("Server: %s Port: %s\n", servers[i],serverAddr[i]);
    }

    printf("Username: %s\n",username);
    printf("Password: %s\n",password);
  */
  ////
}


int main(int argc, char **argv)
{

  if(argc != 2)
    {
      printf("ERROR: Expected dfc <dfc.conf>\n");
      exit(EXIT_FAILURE);
    }


	
  parseConfigFile(argv[1]);
  resetCompleteness();

  for(int i = 0; i<SERVERS; i++)
  {
    sendOrder[i] = i+1;
  }

  //splitFile("wine3.jpg");	
  //splitFile("foo1");

  //combineFiles();
  runClient();

  return 0;
} 
