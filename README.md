##README:
* Program 1: UDP program to send files between client and server. Implements functions like GET, PUT, etc. 
* Program 2: HTTP web server. Handles GET and POST requests. Handles concurrent requests.
* Program 3: Distributed File system - has distributed client and distributed server. Handles concurrent requests. Files are chunked into 4 pieces encrypted (XOR encryption) and pust in the servers. There is 'LIST' function implemente that shows all the files on the servers.
While listing the files, [incomplete] tag is shown besides files that don't have all their file chunks. Md5sum is used to decide which server gets a file chunk. On getting the file chunks from the server, the chunks are put together and decrypted. File integrity checked using md5sum hash function.
* Program 4: Forward explicit HTTP web proxy. Handles only GET requests. Concurrent requests are supported and the web content is cached.
