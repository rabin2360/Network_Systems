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
/* You will have to modify the program below */

#define MAXBUFSIZE 100

void processLS()
{
  pid_t pid;
  int pidStatus;
  
  pid = fork();

  if(pid == 0)
    {
      char *argv[] = {"ls",0};
      execvp(argv[0], argv);
      printf("Command failed! What the fuck happened\n");
    }
  else if (pid < 0)
    printf("Child process could not be created\n");
  else
    waitpid(pid, &pidStatus, 0);
}

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
	  //waits for an incoming message
	  bzero(buffer,sizeof(buffer));
	  bzero(msg, MAXBUFSIZE);
	
	  nbytes = recvfrom(sock,buffer,MAXBUFSIZE,0, (struct sockaddr *) &remote, &remote_length);

	  if(nbytes < 0)
	    printf("Server: Error receiving the message\n");
	  
	  printf("The client says %s\n", buffer);


	  if(strcmp(buffer, "exit\n") == 0)
	    {
	      strncpy(msg, "Bye bye!\n", MAXBUFSIZE);
	      printf("%s",msg);
	      nbytes = sendto(sock,msg,strlen(msg),0,(struct sockaddr *) &remote, remote_length);
	  if(nbytes < 0)
	    printf("Server: Error terminating server\n");
	  
	      break;
	    }
	  else if(strcmp(buffer, "ls\n") == 0)
	    {
	      //processLS();

	      FILE *fp;
	      buffer[strlen(buffer)-1] = 0;
	      strcat(buffer,">file");
	      //printf("Buffer: %s\n",buffer);

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

		  while((c=getc(fp))!=EOF)
		    msg[i++] = c;

		  msg[i] = '\0';
		  //printf("%s\n", msg);
		  fclose(fp);
		}
	      //strncpy(msg, "ls", MAXBUFSIZE);
	    }
	  else if(strcmp(buffer, "get\n") == 0)
	    {
	      strncpy(msg, "get", MAXBUFSIZE);
	    }
	  else if(strcmp(buffer, "post\n") == 0)
	    {
	      strncpy(msg, "post", MAXBUFSIZE);
	    }
	  else
	    {
	      strncpy(msg, "what!!!", MAXBUFSIZE);
	    }

	  
	  nbytes = sendto(sock,msg,strlen(msg),0,(struct sockaddr *) &remote, remote_length);
	  
	  if(nbytes <  0)
	  printf("Server: Error sending the message\n");
	}
	
	close(sock);
}

