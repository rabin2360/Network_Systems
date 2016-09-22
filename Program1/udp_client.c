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

  /******************
	  Here we populate a sockaddr_in struct with
	  information regarding where we'd like to send our packet 
	  i.e the Server.
  ******************/
  bzero(&remote,sizeof(remote));               //zero the struct
  remote.sin_family = AF_INET;                 //address family
  remote.sin_port = htons(atoi(argv[2]));      //sets port to network byte order
  remote.sin_addr.s_addr = inet_addr(argv[1]); //sets remote IP address

  //Causes the system to create a generic socket of type UDP (datagram)
  if ((sock = socket(AF_INET, SOCK_DGRAM,IPPROTO_UDP) ) < 0)
    {
      printf("unable to create socket");
    }
  else
    {
      printf("socket created\n");
    }

  /******************
	  sendto() sends immediately.  
	  it will report an error if the message fails to leave the computer
	  however, with UDP, there is no error if the message is lost in the network once it leaves the computer.
  ******************/

  char command[MAXBUFSIZE];
	
  remote_length = sizeof(remote);

  while(1){

    //prompting the user
    printf("Please enter msg: ");

    //resetting the buffer
    bzero(command, MAXBUFSIZE);
    fgets(command, MAXBUFSIZE, stdin);

    remote_length = sizeof(remote);
    
    printf("Client: command %s\n", command);
    nbytes = sendto(sock, command, strlen(command),0,(struct sockaddr *) &remote, remote_length);

    if(nbytes <  0)
      printf("Client: Error sending the message\n");
	  
    // Blocks till bytes are received
    struct sockaddr_in from_addr;
    int addr_length = sizeof(struct sockaddr);
    bzero(buffer,sizeof(buffer));
    nbytes = recvfrom(sock, buffer, MAXBUFSIZE,0, (struct sockaddr *)&remote,&remote_length);

    //interpreting the input
    char *pos;
    if((pos = strchr(command, '\n'))!= NULL)
      *pos = '\0';
	  
    char * tokens;
    int i = 0;
    char **tokenArray = malloc(MAXBUFSIZE *sizeof(char));
    tokens = strtok(command, " ");

    while(tokens !=NULL)
      {
	tokenArray[i++] = tokens;
	tokens = strtok(NULL," ");
      }

    if(nbytes < 0)
      printf("Client: Error receiving the message\n");
    else{
      if(strcmp(tokenArray[0], "exit") == 0)
	{
	  printf("Bye bye!\n");
	  break;
	}

      else if(strcmp(tokenArray[0], "ls")==0)
	{
	  printf("Server says %s\n", buffer);     
	}

      else if(strcmp(tokenArray[0], "get") ==0)
	{
	  FILE *fp;
	  fp = fopen("clientReceived", "w");

	  if(!fp)
	    {
	      printf("Error writing\n");
	    }

	  if(strcmp(buffer, "error")==0)
	    {
	      printf("No such file or folder. Please check the name you've entered!\n");
	      continue;
	    }
	  else{
	  size_t writtenVals = fwrite(buffer, sizeof(char),nbytes/sizeof(char),fp);
	  printf("written %zu\n", nbytes/sizeof(char));
	  fclose(fp);

	  //calcuate the md5 of the file received
	  char * calculatedStrMd5 = str2md5(buffer, nbytes/sizeof(char));

	  //waiting to receive the md5 for the file downloaded
	  char * receivedStrMd5 = (char *) malloc(33);
	  nbytes = recvfrom(sock, receivedStrMd5, MAXBUFSIZE,0, (struct sockaddr *)&remote,&remote_length);

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
	}

      else if(strcmp(tokenArray[0], "post") == 0)
	{
	  if(strcmp(buffer, "ready") ==0)
	    {
	      printf("%s\n", tokenArray[0]);
	      printf("%s\n", tokenArray[1]);
	      
	      FILE *fp = fopen(tokenArray[1],"r");
	      char c;
	      int i = 0;

	      if(!fp)
		{
		  printf("ERROR: File not found in the local directory\n");
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
		printf("error allocating file buffer to read. Try again!\n");
		continue;
	      }

	      size_t readVals = fread(fileBuf, sizeof(char), numbytes, fp);
	      printf("read %zu\n", readVals);
	      fclose(fp);

	      //---- for debugging purposes-----
	      fp = fopen("sendingToServer", "w");

	      if(!fp)
		{
		  printf("Client: Error writing\n");
		}

	      size_t writtenVals = fwrite(fileBuf, sizeof(char),numbytes,fp);
	      printf("written %zu\n", writtenVals);
	      fclose(fp);
	      //---- for debugging purposes-----

	      memcpy(buffer, fileBuf, numbytes);
	      nbytes = sendto(sock, fileBuf, numbytes,0,(struct sockaddr *) &remote, remote_length);

	      if(nbytes <  0)
		  printf("Client: Error sending the file\n");

	      
	      char *strMd5 = str2md5(fileBuf, writtenVals);
	      printf("%s\n", strMd5);
	      nbytes = sendto(sock, strMd5, strlen(strMd5),0,(struct sockaddr *) &remote, remote_length);
	    }

	  else
	    printf("Client: Error sending file to the server!\n");
	}

      else
	{
	  printf("No idea what you said %s\n", tokenArray[0]);
	}
	
    }//else statement for reading the number of bytes received
	
  }//infinite loop end
	
  close(sock);

}

