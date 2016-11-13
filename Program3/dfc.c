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
#include <stdbool.h>

#define READ_BUFFER 1024
#define MAXBUFSIZE 1024
#define SERVERS 4
#define FILES 2
#define FILENAME 100
#define LOCALHOST "127.0.0.1"

//variables used across functions
char *serverRoots[SERVERS];
char *serverAddr[SERVERS];
char *fileSizes[SERVERS];
char fileChunkNames[SERVERS][FILENAME];
char *username;
char *password;


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
    sprintf(smallFileName, "file%d.txt",currentFileNum);
    printf("%s\n", smallFileName);

	if(fd != -1)
	{

		strcpy(fileChunkNames[currentFileNum],smallFileName);
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

			    sprintf(smallFileName, "file%d.txt",currentFileNum);
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
	}
}

char* processGET(char* path)
{
	char * getCommand = malloc(READ_BUFFER);
	
	return getCommand;
}

void processPUT(char* filepath)
{
	//split the files into chunks
	splitFile(filepath);


}

char* processLIST(char * path)
{

	char * listCommand = malloc(READ_BUFFER);

	return listCommand;
}

bool validateUser(int sock)
{
	char * usernameAndPassword = malloc(FILENAME);
	char * server_reply = malloc(FILENAME);

	strcpy(usernameAndPassword, "valid&");
	strcat(usernameAndPassword, username);
	strcat(usernameAndPassword, "&");
	strcat(usernameAndPassword, password);

	//printf("username and password: %s\n", usernameAndPassword);

	if( send(sock, usernameAndPassword, strlen(usernameAndPassword), 0) < 0)
	{
		return false;
	}
	else
	{
		//server reply
		if( recv(sock , server_reply , FILENAME , 0) < 0)
        {
            printf("recv failed");
            return false;
        }
        else
        {
        	if(strncmp(server_reply, "valid",5) == 0)
        		return true;
        	/*else
        		printf("server reply %s\n", server_reply);
        	*/	
        }
	}

	return false;
}

void connectToServer(char * serverIP, char* portNum, char * messageTag, char * message)
{

	int sock;
    struct sockaddr_in server;
    //char message[READ_BUFFER];
    char server_reply[READ_BUFFER];
    int read_size;
    bool loopControl;

    //Create socket
    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1)
    {
        printf("ERROR: Could not create socket %s:%s\n", serverIP, portNum);
    }
    printf("MSG: Socket created %s:%s\n", serverIP, portNum);

    server.sin_addr.s_addr = inet_addr(serverIP);
    server.sin_family = AF_INET;
    server.sin_port = htons(atoi(portNum));

    //Connect to remote server
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        printf("ERROR: Connect failed. %s:%s\n\n", serverIP, portNum);
        close(sock);
        return;
    }

    printf("MSG: Connected. %s:%s\n", serverIP, portNum);

    loopControl = validateUser(sock);

    if(!loopControl){
    	printf("ERROR: Invalid username and/or password\n. Please check dfc.conf file for %s:%s\n", serverIP, portNum);
    	return;
    }
    else
    	printf("MSG: Username and password authenticated. %s:%s\n", serverIP, portNum);

        
    //printf("Enter message : ");
    //bzero(message, READ_BUFFER);
    //fgets(message, READ_BUFFER, stdin);
         
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

		if(strcmp(messageTag, "PUT") == 0){

			//sending file name

			//open the file
			int fileFd;
			fileFd = open(message, O_RDONLY);
			if(send(sock, message, strlen(message), 0)<0)
			{
				printf("Problem gate\n");
			}
			else{
				printf("Filename sent\n");
			}

			bzero(message, sizeof(message));
			recv(sock, message, READ_BUFFER, 0);
			printf("%s\n", message);

			char *fileContent = malloc(READ_BUFFER);
			int bytesRead;
			int cumReadSize = lseek(fileFd, 0, SEEK_END);
			lseek(fileFd, 0, SEEK_SET);

			printf("Cum size: %d\n", cumReadSize);

			bzero(message,sizeof(message));
			sprintf(message, "%d", cumReadSize);

			if(send(sock, message, strlen(message), 0)<0)
			{
				printf("Problem gate\n");
			}
			else{
				printf("File size sent\n");
			}
			
			bzero(fileContent, READ_BUFFER);
			bzero(message, strlen(message));

			while((bytesRead = read(fileFd, fileContent, READ_BUFFER)) > 0){
				//cumReadSize += bytesRead;
				send(sock, message, strlen(message), 0);
				printf("BytesRead %d\n", bytesRead);
				bzero(fileContent, READ_BUFFER);
				bzero(message, strlen(message));
			}

			close(fileFd);
			/*printf("Before sending END!\n");
			char * endOfMessage = malloc(READ_BUFFER);
			strcpy(endOfMessage, "data for file\n");
			//strcpy(endOfMessage, "DONE\n");

			if(send(sock, endOfMessage, strlen(endOfMessage), 0)<0)
			{
				printf("ERROR: less than zero\n");
			}
			else
			{
				printf("MSG: Message sent");
			}*/
		}
        	//Receive a reply from the server
       /* if( recv(sock , server_reply , READ_BUFFER , 0) < 0)
        {
            printf("ERROR: Recv failed\n");
        }
        else
        {
        	printf("MSG: Server reply :");
        	printf("%s\n",server_reply);
        }
         */

        bzero(&(server_reply), sizeof(server_reply));

        close(sock); 
    }

 void runClient()
 {
 	char *readBuffer = malloc(READ_BUFFER);
 	char *message = malloc(READ_BUFFER);
 	char *messageTag = malloc(READ_BUFFER);

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

 	while(1){

 		printf("Enter message : ");

    	bzero(readBuffer, READ_BUFFER);
    	bzero(messageTag, READ_BUFFER);
    	bzero(message, READ_BUFFER);

    	fgets(readBuffer, READ_BUFFER, stdin);
    	send = true;

    	if(strncmp(readBuffer, "PUT", 3) == 0)
    	{
    		char * tokens = strtok(readBuffer, " \n");
    		tokens = strtok(NULL, " \n");
    		strcpy(messageTag, "PUT");
    		strcpy(message, tokens);
    		splitFile(message);
    	}
    	else if(strncmp(readBuffer, "GET", 3) == 0)
    	{
    		strcpy(messageTag, "GET");
    		*message = '\0';
    	}
    	else if(strncmp(readBuffer, "LIST", 4) == 0)
    	{
    		strcpy(messageTag, "LIST");
    		*message = '\0';
    	}
    	else
    	{
    		printf("ERROR: Unrecognized message %s\n", readBuffer);
    		send = false;
    	}

 		for(int i = 0; i<FILES && send; i++)
 		{
 			//printf("serverAddr:%s, serverPort: %s\n", serverAddrs[i], serverPorts[i]);
 			if(strncmp(readBuffer, "PUT", 3) == 0)
 			{
 				bzero(message, READ_BUFFER);
 				strcpy(message, fileChunkNames[i]);
 			}


 			connectToServer(serverAddrs[i], serverPorts[i], messageTag, message);
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

	//splitFile("wine3.jpg");	
	//splitFile("foo1");

	//combineFiles();
	runClient();

return 0;
} 