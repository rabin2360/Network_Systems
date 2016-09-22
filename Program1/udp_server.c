#include <sys/types.h>
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
#include <string.h>
#include "md5.h"

#define MAXBUFSIZE 30000
#define STRMAX 100

int main (int argc, char * argv[] )
{


  int sock;                           //This will be our socket
  struct sockaddr_in sin, remote;     //"Internet socket address structure"
  unsigned int remote_length;         //length of the sockaddr_in structure
  int nbytes;                        //number of bytes we receive in our message
  char buffer[MAXBUFSIZE];             //a buffer to store our received message

  if (argc != 2)
    {
      printf ("USAGE:  <port>\n");
      exit(1);
    }

  /******************
	  This code populates the sockaddr_in struct with
	  the information about our socket
  ******************/
  bzero(&sin,sizeof(sin));                    //zero the struct
  sin.sin_family = AF_INET;                   //address family
  sin.sin_port = htons(atoi(argv[1]));        //htons() sets the port # to network byte order
  sin.sin_addr.s_addr = INADDR_ANY;           //supplies the IP address of the local machine


  //Causes the system to create a generic socket of type UDP (datagram)
  if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    {
      printf("unable to create socket\n");
    }
  else
    {
      printf("Socket created\n");
    }


  /******************
	  Once we've created a socket, we must bind that socket to the 
	  local address and port we've supplied in the sockaddr_in struct
  ******************/
  if (bind(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0)
    {
      printf("unable to bind socket\n");
    }
  else
    {
      printf("binding successful\n");
    }

  remote_length = sizeof(remote);
  char msg[MAXBUFSIZE];
	  
  while(1){

    //resetting the buffers whenever needed
    bzero(buffer,sizeof(buffer));
    bzero(msg, MAXBUFSIZE);
	
    nbytes = recvfrom(sock,buffer,MAXBUFSIZE,0, (struct sockaddr *) &remote, &remote_length);

    if(nbytes < 0)
      printf("Server: Error receiving the message\n");

    //getting rid of the newline character
    char *pos;
    if((pos = strchr(buffer, '\n'))!= NULL)
      *pos = '\0';
	  
    char * tokens;
    int i = 0;
    char **tokenArray = malloc(MAXBUFSIZE *sizeof(char));
    tokens = strtok(buffer, " ");

    while(tokens !=NULL)
      {
	tokenArray[i++] = tokens;
	tokens = strtok(NULL," ");
      }

    if(strcmp(tokenArray[0], "exit") == 0)
      {
	strncpy(msg, "Server signing off!\n", MAXBUFSIZE);
	printf("%s",msg);
	nbytes = sendto(sock,msg,strlen(msg),0,(struct sockaddr *) &remote, remote_length);

	if(nbytes < 0)
	  printf("Server: Error terminating server\n");
	  
	break;
      }
    else if(strcmp(tokenArray[0], "ls") == 0)
      {
	FILE *fp;
	int deleteFile;
	      
	strcat(buffer,">file");

	pid_t pid = vfork();
	int status = 0;

	if(pid == 0)
	  {
	    execlp("sh", "sh", "-c", buffer, (char *)0);
	  }
	else if(pid < 0)
	  {
	    printf("Demon has spawned. Your program won't work\n");
	  }
	else
	  {
	    wait(&status);
	    char c;
	    int i = 0;
	    //open file and send it to the child
	    fp = fopen("file", "r");

	    long numbytes;
	    fseek(fp, 0L, SEEK_END);
	    numbytes = ftell(fp);

	    fseek(fp, 0L, SEEK_SET);
	    
	    while((c=getc(fp))!=EOF)
	      msg[i++] = c;

	    msg[i] = '\0';
	    //printf("%s\n", msg);
	    fclose(fp);

	    //deleteFile = remove("file");
	        //change the buffer size
	    nbytes = sendto(sock,msg,numbytes,0,(struct sockaddr *) &remote, remote_length);

	  }
	//strncpy(msg, "ls", MAXBUFSIZE);
      }
    else if(strcmp(tokenArray[0], "get") == 0)
      {
	printf("%s\n", tokenArray[0]);
	printf("%s\n", tokenArray[1]);
	      
	FILE *fp = fopen(tokenArray[1],"r");
	char c;
	int i = 0;

	if(!fp)
	  {
	    printf("ERROR: File not found in the local directory\n");

	    //send the error message
	    strncpy(msg, "error", STRMAX);
	    printf("%zu\n", strlen(msg));
	    nbytes = sendto(sock,msg,strlen(msg),0,(struct sockaddr *) &remote, remote_length);

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
	  printf("Error: error allocating. Try again!\n");
	  continue;
	}

	size_t readVals = fread(fileBuf, sizeof(char), numbytes, fp);
	printf("read %zu\n", readVals);
	fclose(fp);

	//****debugging
	fp = fopen("sendingToClient", "w");

	if(!fp)
	  {
	    printf("Error writing\n");
	  }

	size_t writtenVals = fwrite(fileBuf, sizeof(char),numbytes,fp);
	printf("written %zu\n", writtenVals);
	fclose(fp);
	//*****bebugging******
	
	memcpy(msg, fileBuf, numbytes);
	    //change the buffer size

	//send the file
	nbytes = sendto(sock,msg,numbytes,0,(struct sockaddr *) &remote, remote_length);

	//send the calculated md5
	char * calculatedMd5 = str2md5(msg, numbytes);
	printf("%s\n", calculatedMd5);
	nbytes = sendto(sock, calculatedMd5, strlen(calculatedMd5), 0, (struct sockaddr *) & remote, remote_length);
	
	
      }
    else if(strcmp(buffer, "post") == 0)
      {
	strncpy(msg, "ready", MAXBUFSIZE);

	nbytes = sendto(sock,msg,MAXBUFSIZE,0,(struct sockaddr *) &remote, remote_length);
		       
	if(nbytes <  0)
	  printf("Server: Error sending the ready message\n");

	//receiving the file
	bzero(buffer,sizeof(MAXBUFSIZE));
	nbytes = recvfrom(sock,buffer,MAXBUFSIZE,0, (struct sockaddr *) &remote, &remote_length);

         if(nbytes <  0)
	   printf("Server: Error receiving the file\n");

	  FILE *fp;
	  fp = fopen("postedFile", "w");

	  if(!fp)
	    {
	      printf("Error writing\n");
	    }

	  size_t writtenVals = fwrite(buffer, sizeof(char),nbytes/sizeof(char),fp);
	  printf("written output %zu\n", nbytes/sizeof(char));
	  fclose(fp);

	  //ensuring that the file received is not corrupted
	  char *calculatedStrMd5 = str2md5(buffer, nbytes/sizeof(char));	  
	  char *receivedStrMd5 = (char*) malloc(33);
	  
	  nbytes = recvfrom(sock,receivedStrMd5,MAXBUFSIZE,0, (struct sockaddr *) &remote, &remote_length);

	  if (strncmp(calculatedStrMd5, receivedStrMd5,32) ==0)
	      printf("File not corrupted!\n");
	  else
	    {
	      printf("File corrupted\n");
	      printf("Client calculated md5 of the sent file: %s \n", receivedStrMd5);
	      printf("Server calculated md5 of the received file: %s\n", calculatedStrMd5);
	    }
	  
	  free(receivedStrMd5);

      }
    else
      {
	strncpy(msg, "what!!!", MAXBUFSIZE);
      }

    //change the buffer size
    //nbytes = sendto(sock,msg,MAXBUFSIZE,0,(struct sockaddr *) &remote, remote_length);
		       
    if(nbytes <  0){
      printf("Server: Error sending the message\n");
    }
    else
      {
	printf("Server: sent %d\n", nbytes);
      }
	    
  }//while loop
	
  close(sock);
}

