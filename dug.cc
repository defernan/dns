#include "includes.h"

#define MAXLINE 1024
#define MAX_MESSAGE_SIZE 512//65535 //2^16-1
#define DNS_PORT 53
#define NULL_CHAR_SIZE 1
#define DEBUG 1 

//fill in header
void fillDNSHeader( DNSHeader* header ){
    header->id = (unsigned short)htons(getpid());
    header->qr = 0; 
    header->opcode = 0; //standard query
    header->aa = 0; //not authoritative
    header->tc = 0; //not truncated
    header->rd = 1; //recursion Desired
    header->ra = 0; //recursion not available 
    header->z = 0;
    header->rcode = 0;
    header->qdCount = htons(1); //1 question, big endian form
    header->anCount = 0;
    header->nsCount = 0;
    header->arCount = 0;
}

//writes in query name to buffer in correct format ie 7imagine5mines3edu
void writeHostToDNSBuffer(unsigned char* host, unsigned char* buffer){
    int pos = 0;
    int counter = 0;
    int length = strlen((char*)host);
    for(int i = 0; i < length + 1; i++){
        if(host[i] == '.' || i == length ){
           //converts int to char
           *buffer++ = counter; 
           for( int j = pos; j < i; j++){
               *buffer++ = host[j]; 
           }
           pos = i + 1;
           counter = 0;
        }else{
           counter++;
        }
    }
    //null terminate
    *buffer++ = 0x00;
}
void readDNSResponse(unsigned char* buffer, unsigned char* questionName){
    DNSHeader* header;
    DNSQuery* query;
    unsigned char* response;
    
    header = (DNSHeader*)buffer;
    response = &buffer[ sizeof(DNSHeader) + strlen((const char*)questionName) + NULL_CHAR_SIZE + sizeof(DNSQueryInfo) ];
    
    cout << ntohs(header->qdCount) << " questions\n";
    cout << ntohs(header->anCount) << " answers\n";
    cout << ntohs(header->nsCount) << " ns\n";
    cout << ntohs(header->arCount) << " addln\n";
    
    for(int i = 0; i < ntohs(header->qdCount); i++){
        
    }

}
// ***************************************************************************
// * Main
// ***************************************************************************
int main(int argc, char **argv) {
    
    unsigned char* host = (unsigned char*)argv[1];
    const char* serverIP = argv[2];
    unsigned char buffer[MAX_MESSAGE_SIZE]; 
    unsigned char dnsHost[100];
    unsigned char* questionName;

    //dns struct
    DNSHeader* dnsHeader = (DNSHeader*)&buffer;
    //populate dns header info
    fillDNSHeader(dnsHeader);
    //move to buffer spot after header info
    questionName = (unsigned char*)&buffer[sizeof(DNSHeader)];
    writeHostToDNSBuffer(host, questionName);
    DNSQueryInfo* queryInfo = (DNSQueryInfo*)&buffer[sizeof(DNSHeader) + strlen((const char*)questionName) + NULL_CHAR_SIZE ]; //+1 for null char
    //populate query field 
    queryInfo->qtype = htons(1); //A type
    queryInfo->qclass = htons(1); //1 for internet
    //printf("/%s/",qname); 
    if (argc != 3) {
            cout << "useage " << argv[0] << endl;
            exit(-1);
    }
    
    //client socket	
    int clientSock = -1;
    if( (clientSock = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        cout << "Failed to create listening socket " << strerror(errno) << endl;
        exit(-1);
    }
    
      
    /***************************
    * Set up server address to send to
    ****************************/
    struct sockaddr_in	servaddr;
    // Zero the whole thing.
    bzero(&servaddr, sizeof(servaddr));
    // IPv4 Protocol Family
    servaddr.sin_family = AF_INET;
    // assign ip addr in network byte order.
    servaddr.sin_addr.s_addr = inet_addr(serverIP);
    // DNS uses port 53
    servaddr.sin_port = htons(DNS_PORT);
    
    //MAKE SEND CALL
    if(sendto(clientSock, (char*)buffer, sizeof(DNSHeader) + sizeof(DNSQueryInfo)  + strlen((const char*)questionName) + NULL_CHAR_SIZE, 0, (sockaddr*)&servaddr, sizeof(servaddr)) < 0){
        cout << "Error sending" << endl;
        exit(-1);
    }
    
    //MAKE RCV CALL
    int size_of_serv_addr = sizeof(servaddr);
    if(recvfrom(clientSock, (char*)buffer, MAX_MESSAGE_SIZE, 0, (sockaddr*)&servaddr, (socklen_t*)&size_of_serv_addr) < 0){
        cout << "Error receiving." << endl;
        exit(-1);
    }

    //Read 
    readDNSResponse(buffer, questionName);
    //cout << buffer; 
    /**************************************************************************
    * HERE DOWN USE FOR DAEMONIZE REFERENCE
    ***************************************************************************/
    // ********************************************************************
	// * Binding configures the socket with the parameters we have
	// * specified in the servaddr structure.  This step is implicit in
	// * the connect() call, but must be explicitly listed for servers.
	// ********************************************************************
	if (DEBUG)
		cout << "Process has bound fd " << clientSock << " to port " << DNS_PORT << endl;
     
    /*if (bind(listenfd, (sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        cout << "bind() failed: " << strerror(errno) << endl;
        exit(-1);
    }*/
    
    //int
    /*
	// ********************************************************************
    // * Setting the socket to the listening state is the second step
	// * needed to being accepting connections.  This creates a que for
	// * connections and starts the kernel listening for connections.
    // ********************************************************************
	if (DEBUG)
		cout << "We are now listening for new connections" << endl;
    
    int listenq = 1;
    if (listen(listenfd, listenq) < 0) {
        cout << "listen() failed: " << strerror(errno) << endl;
        exit(-1);
    }

	// ********************************************************************
    // * The accept call will sleep, waiting for a connection.  When 
	// * a connection request comes in the accept() call creates a NEW
	// * socket with a new fd that will be used for the communication.
    // ********************************************************************
	set<pthread_t*> threads;
	while (1) {
		if (DEBUG)
			cout << "Calling accept() in master thread." << endl;
		int connfd = -1;
        if ((connfd = accept(listenfd, (sockaddr *) NULL, NULL)) < 0) {
            cout << "accept() failed: " << strerror(errno) << endl;
            exit(-1);
        }
	
		if (DEBUG)
			cout << "Spawing new thread to handled connect on fd=" << connfd << endl;

		pthread_t* threadID = new pthread_t;
		pthread_create(threadID, NULL, processRequest, (void *)connfd);
		threads.insert(threadID);
	}
    close(listenfd);
    */
}

/*
//get filesize in bytes
int getFileSize(string fileName){
    int fileSize = -1; 
    struct stat buff;
    //fill in stat structure and retrieve file size 
    int statReturn = stat(fileName.c_str(), &buff);
    if(statReturn == 0){
        return buff.st_size;
    }

    return fileSize; 
}
//get content type, should fix but this will always be last field in response
char* getContentType(string fileName){
    char contentType[MAXLINE];
    if((fileName.find("jpeg") != std::string::npos) || (fileName.find("jpg") != std::string::npos) ){
        strcpy(contentType, "\r\nContent-Type: image/jpeg\r\n\r\n");
    }else{
        strcpy(contentType , "\r\nContent-Type: text/html\r\n\r\n");
    }
     
    return contentType;
}
// ***************************************************************************
// * readGetRequest()
// *  This function should read each line of the request sent by the client
// *  and check to see if it contains a GET request (which is the part  we are
// *  interested in).  You know you have read all the lines when you read
// *  a blank line.
// ***************************************************************************
string readGetRequest(int sockfd) {
    char buff[MAXLINE];
    int bytesRead;
    while ( (bytesRead=read(sockfd, buff, MAXLINE)) > 0){
        string getReq = string(buff).substr(0, bytesRead);

        if(getReq.find("GET") != std::string::npos){
            return getReq;
        }
    }
    return "NO GET"; 
}


// ***************************************************************************
// * parseGET()
// *  Parse the GET request line and find the filename.  Since HTTP requests
// *  should always be relitive to a particular directory (so you don't 
// *  accidently expose the whole filesystem) you should prepend a path
// *  as well. In this case prepend "." to make the request relitive to
// *  the current directory. 
// *
// *  Note that a real webserver will include other protections to keep
// *  requests from traversing up the path, including but not limited to
// *  using chroot.  Since we can't do that you must remember that as long
// *  as your program is running anyone who knows what port you are using
// *  could get any of your files.
// ***************************************************************************
string parseGET(string getRequest) {
   
    //set stream = to string 
    stringstream stream;
    stream.str(getRequest);
    string file = ".";
    string fileLoc = "";
    //string parse
    while( stream >> fileLoc ){
        if( fileLoc == "GET" ){
            stream >> fileLoc;
            break;
        }
    }
    
    //clear stream
    stream.str(string());
    stream.clear();

    file = file + fileLoc;
     
    return file;

}

// ***************************************************************************
// * fileExists()
// *  Simple utility function I use to test to see if the file really
// *  exists.  Probably would have been simpler just to put it inline.
// ***************************************************************************
bool fileExists(string filename) {
    ifstream file(filename.c_str());
    if(file){
        file.close();
        return true;
    }
    file.close();
    return false;
}

// ***************************************************************************
// * sendHeader()
// *  Send the content type and rest of the header. For this assignment you
// *  only have to do TXT, HTML and JPG, but you can do others if you want.
// ***************************************************************************
bool sendHeader(int sockfd, string fileName) {
    //status line 
    char statusLine[MAXLINE] = "\nHTTP/1.0 200 OK\r\n";
    char contentLength[MAXLINE] = "Content-Length: ";
    char fileSize[MAXLINE] ;
    sprintf(fileSize, "%d", getFileSize(fileName));
    //content type must be last, has return line and blank line to end header
    char contentType[MAXLINE];
    strcpy(contentType, getContentType(fileName));
    
    write(sockfd, statusLine, strlen(statusLine)); 
    write(sockfd, contentLength, strlen(contentLength));
    write(sockfd, fileSize, strlen(fileSize));
    write(sockfd, contentType, strlen(contentType)); 
    return true;
}

// ***************************************************************************
// * sendFile(int sockfd,string filename)
// *  Open the file, read it and send it.
// ***************************************************************************
bool sendFile(int sockfd,string filename) {
    int file = open(filename.c_str(), O_RDONLY);
    int bytesRead;
    int written;
    char buff[MAXLINE];
     
    while((bytesRead = read(file, buff, MAXLINE))> 0){
        char* p = buff; 
        while(bytesRead > 0){
            written = write(sockfd, p, bytesRead);
            bytesRead = bytesRead - written;
            p += written;
        }
    }
    close(file);
    return true;
}

// ***************************************************************************
// * send404(int sockfd)
// *  Send the whole error page.  I can really say anything you like.
// ***************************************************************************
bool send404(int sockfd) {
    char statusLine[MAXLINE] = "\nHTTP/1.0 404 NOT FOUND\r\n";
    char contentLength[MAXLINE] = "Content-Length: 25\r\n";
    char contentType[MAXLINE] = "Content-Type: text/html\r\n\r\n"; //insert blank line at end to end header
    
    
    char message[MAXLINE] = "Requested file not found.";
    
    write(sockfd, statusLine, strlen(statusLine));
    write(sockfd, contentLength, strlen(contentLength)); 
    write(sockfd, contentType, strlen(contentType)); 

    write(sockfd, message, strlen(message));

    return true;
}


// ***************************************************************************
// * processRequest()
// *  Master function for processing thread.
// *  !!! NOTE - the IOSTREAM library and the cout varibables may or may
// *      not be thread safe depending on your system.  I use the cout
// *      statments for debugging when I know there will be just one thread
// *      but once you are processing multiple rquests it might cause problems.
// ***************************************************************************
void* processRequest(void *arg) {


	// *******************************************************
	// * This is a little bit of a cheat, but if you end up
	// * with a FD of more than 64 bits you are in trouble
	// *******************************************************
	int sockfd = (long)arg;
	if (DEBUG)
		cout << "We are in the thread with fd = " << sockfd << endl;
    
    
	// *******************************************************
	// * Now we need to find the GET part of the request.
	// *******************************************************
	string getRequest = readGetRequest(sockfd);
	if (DEBUG)
		cout << "Get request is " << getRequest << endl;

	// *******************************************************
	// * Find the path/file part of the request.
	// *******************************************************
	string requestedFile = parseGET(getRequest);
	if (DEBUG)
		cout << "The file they want is " << requestedFile << endl;
    // *******************************************************
	// * Send the file
	// *******************************************************
	if (fileExists(requestedFile)) {
		
        // *******************************************************
		// * Build & send the header.
		// *******************************************************
		sendHeader(sockfd, requestedFile);
		if (DEBUG)
			cout << "Header sent" << endl;

		// *******************************************************
		// * Send the file
		// *******************************************************
		sendFile(sockfd,requestedFile);
		if (DEBUG)
			cout << "File sent" << endl;

	} else {
		// *******************************************************
		// * Send an error message 
		// *******************************************************
		if (DEBUG)
			cout << "File " << requestedFile << " does not exist." << endl;
		send404(sockfd);
		if (DEBUG)
			cout << "Error message sent." << endl;
	}

	if (DEBUG)
		cout << "Thread terminating" << endl;
    close(sockfd);
}
*/


