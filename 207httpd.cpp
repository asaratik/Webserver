#include "common.h"

using namespace std;

string docRoot;

const char *response_404 = "HTTP/1.1 404 Not Found\nContent-Type: text/html; charset=utf-8\n\n<html><body><i>Requested resouce not found on the server..Please check the URL !!</i></body></html>";


//  Get MIME Type  
const char * http_get_mime(const char *fname) {
const char *extf = strrchr(fname, '.');
if (extf == NULL) {
		return "application/octet-stream";
	} else if (strcmp(extf, ".html") == 0) {
		return "text/html";
	} else if (strcmp(extf, ".jpg") == 0) {
		return "image/jpeg";
	} else if (strcmp(extf, ".gif") == 0) {
		return "image/gif";
	} else {
		return "application/octet-stream";
	}
}


//  Get the source file size  
int http_get_filesize(FILE *fp) {
	int fsize;
	fseek(fp, 0, SEEK_END);
	fsize = ftell(fp);
	rewind(fp);
	return fsize;
}


//Thread initiating method
void *serveRequest(void *clientID)
{
	string path="",fileType,fileContents,fileLine;
	char buffer[8096],bufferResp[8096];
	int i=4,fileSize,client = *(int*)clientID;
	FILE *inFile;
	size_t pos = 0;
	vector<char> imageBuffer;

	//Clearing or initalizing the buffers to null
	memset(buffer,0,sizeof(buffer));
    memset(bufferResp,0,sizeof(bufferResp));

	//Receiving the client request
    recv(client, buffer, sizeof(buffer), 0);
    
    //Getting the file path from the GET request
    while(buffer[i] != ' ')
    {
    	path = path + buffer[i];
    	i++;
    }
    
    if(path != "/favicon.ico")
    {
    	if(path == "/")
    		path = docRoot + path + "index.html";
    	else
    		path = docRoot + path;
	   
	    path = path + '\0';

    	while ((pos = path.find("images", pos)) != string::npos) {
         	path.replace(pos, 6, "img");
         	pos += 6;
         }

         while ((pos = path.find("//", pos)) != string::npos) {
         	path.replace(pos, 2, "/");
         	pos += 2;
         }


	    //Converting path into a constant character pointer
		const char * filePath = path.c_str();
	    
	    inFile = fopen(filePath,"r");
	    if(inFile == NULL)
		    {
		    	cout << "Error opening the file.." << endl;
		    	send(client, response_404, strlen(response_404), 0);
		    	close(client);
		    }
		    else
		    {
				fileSize = http_get_filesize(inFile);
			    fclose(inFile);

			    fileType = http_get_mime(filePath);

			    if(fileType == "text/html")
			    {
			    	ifstream outFile(filePath);

					if(outFile.is_open())
					{
						while(getline(outFile,fileLine))
						{
							fileContents = fileContents+fileLine;
						}		
					}
					
					string response = "HTTP/1.1 200 OK\r\n"
					"Server: 207httpd/0.0.1\r\n"
					"Connection: close\r\n"
					"Content-Type: "+fileType+"\r\n"
					"Content-Size: "+to_string(fileSize)+" bytes\r\n"
					"\r\n"+fileContents+"\0";

					for(i=0;i<response.size();i++){
				        bufferResp[i]=response[i];
				    }

					
					if(send(client, bufferResp, sizeof(bufferResp), 0)<0){
						cout << "Failed responding to the client..terminating connection.." << endl;
						close(client);
					}
				}
				    else
				    {
				    	memset(bufferResp,0,sizeof(bufferResp));	
					    
						//Header response
					    string response = "HTTP/1.1 200 OK\r\n"
						"Server: 207httpd/0.0.1\r\n"
						"Connection: close\r\n"
						"Content-Type: "+fileType+"\r\n"
						"Content-Size: "+to_string(fileSize)+" bytes\r\n"
						"Content-Transfer-Encoding: binary\r\n"
						"\r\n";

						for(i=0;i<response.size();i++){
					        bufferResp[i]=response[i];
					    }

						char data_to_send[fileSize];

						inFile = fopen(filePath, "rb");
						imageBuffer.resize(fileSize);
						//Reading image data into vector buffer
						fread(&imageBuffer[0], 1, fileSize, inFile);

						for(i=0;i<=fileSize;i++)
						{
							data_to_send[i]=imageBuffer[i];
						}
						//Sending header data 
						send(client, bufferResp, sizeof(bufferResp), 0);
						//Sending the image file
						send(client, data_to_send, sizeof(data_to_send), 0);
				    	fclose(inFile);
				    }

				//Clearing the buffers and closing the connection with client
			    memset(buffer,0,sizeof(buffer));
			    memset(bufferResp,0,sizeof(bufferResp));
			    close(client);
		    }

	    }

    else{
    	cout <<"Refused the favicon png request..!!"<< endl;
    	close(client);
    }
}


int main(int argc, char *argv[]) {

	//Checking for the input arguments
	if (argc < 3) {
		cout << "Expected execution : ./server port documentRoot" << endl;
		return 0;
	}

	int portNumber = atoi(argv[1]), enable = 1,sckt,client;
	docRoot = argv[2];
	struct sockaddr_in server_addr,client_addr;
	socklen_t slen;
	pthread_attr_t ta;
	pthread_t thread;

	// Creating a socket for the server
	sckt = socket(AF_INET, SOCK_STREAM, 0);
	if (sckt < 0) {
		perror("Error establishing a socket..");
		return 0;
	}

	// Setting up to re-use the port
	if (setsockopt(sckt, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0) {
	perror("Failed to set sock option SO_REUSEADDR");
	}


	// Setting the sockaddr structure
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htons(INADDR_ANY);
	server_addr.sin_port = htons(portNumber);


	// Binding the socket
	if ((bind(sckt, (struct sockaddr*) &server_addr, sizeof(server_addr))) < 0) {
		perror("Error binding the connection !!");
		return 0;
	}

	
	//Listening for clients
	if(listen(sckt,100) < 0){
		perror("Failed listening to the port");
		return 0;
	}

	// Looping for the clients and creating threads
	
	(void) pthread_attr_init(&ta);
	(void) pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);


	cout << "Server successfully started..waiting for clients to connect..!!"<<endl;

	while (true) {
    	slen = sizeof(client_addr);
    	client = accept(sckt, (struct sockaddr *)&client_addr, &slen);
    	int *pclient = new int;
        *pclient = client;
    	if (client < 0) {
        	if (errno == EINTR)
            	continue;
        perror("Error on accepting the connection !");
        return 0;
    	}
    if (pthread_create(&thread, &ta, (void * (*)(void *))serveRequest,pclient) < 0)
        perror("pthread_create: serve request \n");  
	}

}