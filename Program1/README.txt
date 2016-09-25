
README:

Client program working functionality:

Socket is created and then the client prompts the user to enter the message. The message shown is “Please enter a command:”.

Client takes the message and tries to verify whether the message is recognized or not. The following commands are recognized:
- ls (shows the files available in the server)
- post (will post a file to the server)
- get (will get a file from the server)
- exit (will shut down the server and will also exit client)

Other commands are not recognized.

ls:
When ls command is entered, the client waits for the server to reply with the list of files. The list of files are then shown on the screen.

post:
When post command is entered along with the file name, the client searches the current directory and if not found, informs the user that the file is not in the current local directory. However, the program does not exit. It will prompt user to enter command again. 

After sending the file, the client also uses the sent file to calculate the md5sum and sends it to the server so that it can be determined if the file was corrupted during the transfer.

get:
When get command is entered, the client wait for the server to reply. When the server responds, the client will received the file and then append “.clientReceived” to the file name. The received file is then stored in the local directory.

After receiving the file, the client waits for the server to send the md5sum. On receiving both file and the md5sum, the client also calculates it’s own md5sum and compares the received md5sum with the locally calculated md5sum. If both of the match, the client displays the following message:
“File not corrupted”.

If the file is corrupted, the client states that the file is corrupted and shows the md5sum that was sent by the server and the locally computed md5sum.

exit:
When exit command is entered, the client shuts down the server and then also itself. Both server and client quit.


Server:
The server recognizes the same set of commands as clients. On establishing connection, the server prompts that server successfully started. The server does not allow the commands to be entered. It waits for the client to send commands to be executed. 

ls:
On receiving the ls command, the server processes the command using execlp command. In order to do this, the server forks a child. The child executes the command and then passes the information to the main thread in server. The main thread in server then sends the list of files to the client. 

post:
When server receives the post command, it responds to client saying it’s ready to receive the file. Then the client sends the file. The server receives the file and then stores it in the server. The received file has ‘.serverReceived’ appended to it’s name and stored.

The server after receiving the file also received the md5sum calculated by the client. On receiving the file, the server also computes it’s own md5sum locally and compares it to the md5sum received. If the md5sum sum match, the server displays the message “File not corrupted”. If the file is corrupted, the server, shows the md5sum computed locally with the md5sum sent by the client and states that the file is corrupted.

get:
When the server receives the get command, the server searches the local folder for the file. It the file is not found, the server responds with “error” as message and client knows that the file is not in the server. Then the client prompts the user accordingly.

After sending the file, the server also computes the md5sum of the file and sends it to the client for verification purposes.

exit:
On receiving the exit command, the server exits. The server also sends the client a message saying “exit”.


