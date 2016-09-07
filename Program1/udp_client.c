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

#define MAXBUFSIZE 30000

/* You will have to modify the program below */

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
	printf("Please enter msg: ");
	bzero(command, MAXBUFSIZE);
	fgets(command, MAXBUFSIZE, stdin);
	
	//char command[] = "apple";
	remote_length = sizeof(remote);
	nbytes = sendto(sock, command, strlen(command),0,(struct sockaddr *) &remote, remote_length);

	if(nbytes <  0)
	  printf("Client: Error sending the message\n");

	// Blocks till bytes are received
	struct sockaddr_in from_addr;
	int addr_length = sizeof(struct sockaddr);
	bzero(buffer,sizeof(buffer));
	nbytes = recvfrom(sock, buffer, MAXBUFSIZE,0, (struct sockaddr *)&remote,&remote_length);

	if(nbytes < 0)
	  printf("Client: Error receiving the message\n");

	printf("Server says %s\n", buffer);

	FILE *fp;
	fp = fopen("testingClient", "w");

	if(!fp)
	{
	   printf("Error writing\n");
	}

	size_t writtenVals = fwrite(buffer, sizeof(char),nbytes/sizeof(char),fp);
	printf("written %zu\n", nbytes/sizeof(char));
	      fclose(fp);

	
	if(strcmp(buffer, "Bye bye!\n")==0)
	  {
	    printf("Bye bye!\n");
	    break;
	  }
	
	}
	close(sock);

}

