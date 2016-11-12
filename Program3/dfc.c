#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>
#include <memory.h>
#include <arpa/inet.h>

#define READ_BUFFER 1024
#define MAXBUFSIZE 1024
#define SERVERS 4
#define FILES 4
#define FILENAME 100
#define LOCALHOST "127.0.0.1"

//variables used across functions
char *servers[SERVERS];
char *portNumbers[SERVERS];
char *fileSizes[FILES];
char fileChunkNames[FILES][FILENAME];
char *username;
char *password;

char* generateGET(char* path)
{
	char * getCommand = malloc(READ_BUFFER);
	
	return getCommand;
}

char* generatePUT(char* path)
{
	char * putCommand = malloc(READ_BUFFER);

	return putCommand;
}

char* generateLIST(char * path)
{

	char * listCommand = malloc(READ_BUFFER);

	return listCommand;
}

void runClient()
{

	int sock;
    struct sockaddr_in server;
    char message[READ_BUFFER];
    char server_reply[READ_BUFFER];
    int read_size;

    //Create socket
    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1)
    {
        printf("Could not create socket");
    }
    printf("Socket created");

    server.sin_addr.s_addr = inet_addr(LOCALHOST);
    server.sin_family = AF_INET;
    server.sin_port = htons(10001);

    //Connect to remote server
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("connect failed. Error");
        return;
    }

    printf("Connected\n");

     //keep communicating with server
    while(1)
    {
        printf("Enter message : ");
        bzero(message, READ_BUFFER);

        fgets(message, READ_BUFFER, stdin);
         
        //Send some data
        if( send(sock , message , strlen(message) , 0) < 0)
        {
            printf("Send failed");
            return;
        }
        else
        {
        	printf("Sending message ..\n");
        }
         
         // ( (read_size = recv(client_sock , client_message , READ_BUFFER , 0)) > 0

        //Receive a reply from the server
        if( recv(sock , server_reply , READ_BUFFER , 0) < 0)
        {
            printf("recv failed");
            break;
        }
         
        printf("Server reply :");
        printf("%s\n",server_reply);
        bzero(&(server_reply), sizeof(server_reply));
    }
     
    close(sock);

 /* int nbytes;                             // number of bytes send by sendto()
  int sock;                               //this will be our socket
  char buffer[MAXBUFSIZE];

  struct sockaddr_in remote;              //"Internet socket address structure"

  int remote_length;

  bzero(&remote,sizeof(remote));               //zero the struct
  remote.sin_family = AF_INET;                 //address family
 
  //getting portnum for each of the portNumbers
  char * portNum = strtok(portNumbers[0], ":");
  portNum = strtok(NULL, "\n");
  printf("portNumbers %s\n", portNum);
  
  remote.sin_port = htons(atoi(portNum));      //sets port to network byte order
  remote.sin_addr.s_addr = inet_addr(LOCALHOST); //sets remote IP address

 //Causes the system to create a generic socket of type UDP (datagram)
  if ((sock = socket(AF_INET, SOCK_DGRAM,IPPROTO_UDP) ) < 0)
    {
      printf("Error: Unable to create socket");
      exit(1);
    }

     char command[MAXBUFSIZE];
     remote_length = sizeof(remote);

while(1)
{
	//prompting the user
    printf("Please enter a command: ");

    //resetting the buffer
    bzero(command, MAXBUFSIZE);
    fgets(command, MAXBUFSIZE, stdin);
    remote_length = sizeof(remote);

    nbytes = sendto(sock, command, strlen(command),0,(struct sockaddr *) &remote, remote_length);
        if(nbytes <  0)
	  printf("ERROR: Error sending the message %s\n", command);

	bzero(buffer, sizeof(buffer));
	//wait for response
	nbytes = recvfrom(sock, buffer, MAXBUFSIZE, 0, (struct sockaddr *)&remote, &remote_length);

	printf("Server says %s", buffer);
}
*/
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
					servers[serverIndex] = tokens;

					tokens = strtok(NULL, " \n");
					portNumbers[serverIndex] = tokens;

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
			printf("Server: %s Port: %s\n", servers[i],portNumbers[i]);
		}

		printf("Username: %s\n",username);
		printf("Password: %s\n",password);
		*/
	////
}

void checkIntegrity(char *file1, char *file2)
{

}

//combine files to produce one single file
void combineFiles()
{
	int combinedFileFd = open("combinedFile.jpg", O_RDWR | O_CREAT, 0777);

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

		printf("Filename: %s\n", fileChunkNames[i]);
		chunkFd = open(fileChunkNames[i], O_RDONLY);

		while((bytesRead = read(chunkFd, confFileContent, READ_BUFFER))> 0)
		{
			writtenToLargerFile = write(combinedFileFd, confFileContent, bytesRead);
		}

		close(chunkFd);
	}
}

//split the input file into 4 chunks
//split the file in x-chunks
void splitFile(char * filename)
{

	int bytesRead;
	char * confFileContent = malloc(READ_BUFFER);

	int fd = open(filename, O_RDONLY);

	int smallerFileSize = lseek(fd, 0, SEEK_END);
	smallerFileSize = smallerFileSize/FILES;

    lseek(fd,0, SEEK_SET);

    long cumulativeSize = 0;
    long shortFileWritten = 0;
    int smallerFileFd;
    int currentFileNum = 0;
    char smallFileName[FILENAME];
    sprintf(smallFileName, "file%d.jpg",currentFileNum);
    printf("%s\n", smallFileName);

	if(fd != -1)
	{

		strcpy(fileChunkNames[currentFileNum],smallFileName);
		smallerFileFd = open(smallFileName, O_RDWR | O_CREAT, 0777);

		if(smallerFileFd != -1)
			printf("No errro opening the file\n");
		else
			printf("Error opening the file\n");

		while((bytesRead = read(fd, confFileContent, READ_BUFFER)) > 0)
		{
			cumulativeSize += bytesRead;

			shortFileWritten = write(smallerFileFd, confFileContent, bytesRead);

			if(shortFileWritten == -1)
				printf("Error writing\n");


			if(cumulativeSize >= smallerFileSize)
			{
				close(smallerFileFd);
				currentFileNum++;

			    sprintf(smallFileName, "file%d.jpg",currentFileNum);
    			printf("%s\n", smallFileName);
				strcpy(fileChunkNames[currentFileNum],smallFileName);
		
    			smallerFileFd = open(smallFileName, O_RDWR | O_CREAT, 0777);

			if(smallerFileFd != -1)
				printf("No errro opening the file\n");
			else
				printf("Error opening the file\n");

				cumulativeSize = 0;
			}
			
		}

		//printf("Outside\n");
		close(smallerFileFd);

		close(fd);
	}
	//get the file size of the larger size
	//read must be binary to get bytes
	/*FILE *fpLargeFile = fopen(filename, "rb");
	long filesize = -1;

	//determining the size of the larger file
	if(fpLargeFile)
	{
		fseek(fpLargeFile, 0, SEEK_END);
		filesize = ftell(fpLargeFile)+1;
		fclose(fpLargeFile);
	}

	printf("File size %ld \n", filesize);

	FILE *fpSmallerFile;
	long smallerFileSize = filesize;
	char smallFileName[300];
	char readBuffer[READ_BUFFER];

	fpLargeFile = fopen(filename, "r");

	long currentFileSize = 0;

	printf("File size: %ld\n", smallerFileSize);

	if(fpLargeFile)
	{

		for(int i = 0; i<1; i++)
		{
			currentFileSize = 0;
			sprintf(smallFileName, "file.jpg",i);
			fpSmallerFile = fopen(smallFileName, "wb");

			if(fpSmallerFile)
			{
				while(currentFileSize <= smallerFileSize && fgets(readBuffer, READ_BUFFER, fpLargeFile))
				{
					currentFileSize += strlen(readBuffer);
					fputs(readBuffer, fpSmallerFile);
				}



				printf("file size written: %ld \n", currentFileSize);
				fclose(fpSmallerFile);
			}
		}

	fclose(fpLargeFile);
	}
*/

}

int main(int argc, char **argv)
{

	if(argc != 2)
	{
		printf("ERROR: Expected dfc <dfc.conf>\n");
		exit(EXIT_FAILURE);
	}

	
	parseConfigFile(argv[1]);

	//splitFile("wine3.jpg");	
	//splitFile("foo1");

	//combineFiles();
	runClient();

return 0;
} 