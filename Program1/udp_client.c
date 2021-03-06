#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include "md5.h"

#define MAXBUFSIZE 30000

int main (int argc, char * argv[])
{

  int nbytes;                             // number of bytes send by sendto()
  int sock;                               //this will be our socket
  char buffer[MAXBUFSIZE];

  struct sockaddr_in remote;              //"Internet socket address structure"

  int remote_length;
	
  if (argc < 3)
    {
      printf("USAGE:  <server_ip> <server_port>\n");
      exit(1);
    }

  bzero(&remote,sizeof(remote));               //zero the struct
  remote.sin_family = AF_INET;                 //address family
  remote.sin_port = htons(atoi(argv[2]));      //sets port to network byte order
  remote.sin_addr.s_addr = inet_addr(argv[1]); //sets remote IP address

  //Causes the system to create a generic socket of type UDP (datagram)
  if ((sock = socket(AF_INET, SOCK_DGRAM,IPPROTO_UDP) ) < 0)
    {
      printf("Error: Unable to create socket");
      exit(1);
    }

  char command[MAXBUFSIZE];
  char tempBuffer[MAXBUFSIZE];
  remote_length = sizeof(remote);

  while(1){

    //prompting the user
    printf("Please enter a command: ");

    //resetting the buffer
    bzero(command, MAXBUFSIZE);
    fgets(command, MAXBUFSIZE, stdin);
    remote_length = sizeof(remote);

    char **tokenArray = malloc(MAXBUFSIZE *sizeof(char));
    bzero(tokenArray, MAXBUFSIZE * sizeof(char));
    
    memcpy(tempBuffer, command, MAXBUFSIZE);  
    tokenArray = tokenize(tempBuffer);
    
    //for post commands, check if the file exists before doing anything else
    if(strcmp(tokenArray[0],"post")==0)
      {
	FILE *fp = fopen(tokenArray[1],"r");
	char c;
	int i = 0;

	if(!fp)
	  {
	    printf("ERROR: File not found in the local directory. Please check your local directory.\n");
	    continue;
	  }
      }

    //check if the message is valid before contacting server
    if(strcmp(tokenArray[0], "exit") == 0 ||
       strcmp(tokenArray[0], "ls") == 0 ||
       strcmp(tokenArray[0], "post") == 0 ||
       strcmp(tokenArray[0], "get") == 0)
      {
	nbytes = sendto(sock, command, strlen(command),0,(struct sockaddr *) &remote, remote_length);
        if(nbytes <  0)
	  printf("ERROR: Error sending the message %s\n", command);
	  
	// Blocks till bytes are received
	struct sockaddr_in from_addr;
	int addr_length = sizeof(struct sockaddr);
	bzero(buffer,sizeof(buffer));
	nbytes = recvfrom(sock, buffer, MAXBUFSIZE,0, (struct sockaddr *)&remote,&remote_length);
      }

    if(nbytes < 0)
      printf("ERROR: Error receiving the message\n");
    else{

      if(strcmp(tokenArray[0], "exit") == 0)
	{
	  printf("MSG: Bye bye!\n");
	  break;
	}

      else if(strcmp(tokenArray[0], "ls")==0)
	{
	  printf("MSG: Server says %s\n", buffer);     
	}

      else if(strcmp(tokenArray[0], "get") ==0)
	{

	  printf("tokenArray[1] %s\n", tokenArray[1]);
	  int deleteFile;
	  FILE *fp;

	  char * filename = malloc(strlen(tokenArray[1])+strlen(".clientReceived"));
	  strcpy(filename, tokenArray[1]);
	  strcat(filename, ".clientReceived");
	  
	  fp = fopen(filename, "w");

	  if(!fp)
	    printf("Error writing\n");

	  //server can't find the file
	  if(strcmp(buffer, "error")==0)
	    {
	      fclose(fp);
	      deleteFile = remove(filename);
	      
	      printf("MSG: No such file or folder in the server. Please check the name you've entered!\n");
	      continue;
	    }
	  
	  else{
	    size_t writtenVals = fwrite(buffer, sizeof(char),nbytes/sizeof(char),fp);
	    fclose(fp);

	    printf("MSG: Downloading file from the server...\n");
	    
	    //calcuate the md5 of the file received
	    char * calculatedStrMd5 = str2md5(buffer, nbytes/sizeof(char));

	    //waiting to receive the md5 for the file downloaded
	    char * receivedStrMd5 = (char *) malloc(33);
	    nbytes = recvfrom(sock, receivedStrMd5, MAXBUFSIZE,0, (struct sockaddr *)&remote,&remote_length);

	    if (strncmp(calculatedStrMd5, receivedStrMd5,32) ==0)
	      printf("MSG: File not corrupted!\n");
	    else
	      {
		printf("MSG: File corrupted\n");
		printf("MSG: Client calculated md5 of the sent file: %s \n", receivedStrMd5);
		printf("MSG: Server calculated md5 of the received file: %s\n", calculatedStrMd5);
	      }
	  
	    free(receivedStrMd5);

	  }
	}

      else if(strcmp(tokenArray[0], "post") == 0)
	{
	  if(strcmp(buffer, "ready") ==0)
	    {
	      FILE *fp = fopen(tokenArray[1],"r");
	      char c;
	      int i = 0;

	      if(!fp)
		{
		  printf("ERROR: File not found in the local directory. Please check your local directory.\n");
		  continue;
		}
	      
	      //get the number of bytes
	      long numbytes;
	      fseek(fp, 0L, SEEK_END);
	      numbytes = ftell(fp);

	      //reset to the beginning of the file
	      fseek(fp, 0L, SEEK_SET);

	      char *fileBuf = (char*)calloc(numbytes, sizeof(char));

	      if(fileBuf == NULL){
		printf("ERROR: Cannot allocate buffer to read file. Try again!\n");
		continue;
	      }

	      size_t readVals = fread(fileBuf, sizeof(char), numbytes, fp);
	      //printf("read %zu\n", readVals);
	      fclose(fp);

	      printf("MSG: Uploading %s to the server...\n", tokenArray[1]);
	    
	      memcpy(buffer, fileBuf, numbytes);

	      nbytes = sendto(sock, fileBuf, numbytes,0,(struct sockaddr *) &remote, remote_length);

	      if(nbytes <  0)
		printf("ERROR: Error sending the file to the server. Try again!\n");
	      
	      char *strMd5 = str2md5(fileBuf, numbytes/sizeof(char));
	      nbytes = sendto(sock, strMd5, strlen(strMd5),0,(struct sockaddr *) &remote, remote_length);
	    }

	  else
	    printf("ERROR: Error sending file to the server!\n");
	}

      else
	{
	  printf("ERROR: Command not recognized - %s\n", tokenArray[0]);
	}
	
    }//else statement for reading the number of bytes received
	
  }//infinite loop end
	
  close(sock);

}

