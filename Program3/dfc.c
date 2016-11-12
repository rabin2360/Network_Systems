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

void runClient()
{

	int sock;
    struct sockaddr_in server;
    char message[READ_BUFFER];
    char server_reply[READ_BUFFER];
    int read_size;
    bool loopControl;

    //Create socket
    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1)
    {
        printf("ERROR: Could not create socket");
    }
    printf("MSG: Socket created.\n");

    server.sin_addr.s_addr = inet_addr(LOCALHOST);
    server.sin_family = AF_INET;
    server.sin_port = htons(10001);

    //Connect to remote server
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        printf("ERROR: connect failed. Error");
        return;
    }

    printf("MSG: Connected.\n");

    loopControl = validateUser(sock);

    if(!loopControl)
    	printf("ERROR: Invalid username and/or password\n. Please check dfc.conf file\n");
    else
    	printf("MSG: Username and password authenticated.\n");

     //keep communicating with server
    while(loopControl)
    {
        printf("Enter message : ");
        bzero(message, READ_BUFFER);
        fgets(message, READ_BUFFER, stdin);
         
        //if(validateUser(sock)){
        //	printf("MSG: Valid user.\n");

        //Send some data
        if( send(sock , message , strlen(message) , 0) < 0)
        {
            printf("Send failed");
            //return;
        }
        else
        {
        	printf("MSG: Sending message ..\n");

        	//Receive a reply from the server
        if( recv(sock , server_reply , READ_BUFFER , 0) < 0)
        {
            printf("ERROR: Recv failed");
        }
        else
        {
        	printf("MSG: Server reply :");
        	printf("%s\n",server_reply);
        }
         
        bzero(&(server_reply), sizeof(server_reply));

        }
	          
        }

        close(sock); 
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