#include "includes.h"

#define MAX_MESSAGE_SIZE 512//65535 //2^16-1
#define DNS_PORT 53
#define NULL_CHAR_SIZE 1
#define POINTER 192 //1100 0000 
#define POINTER_OFFSET 49152 //1100 0000 0000 0000
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
//convert name to readable format ie 7imagine5mines3edu => imagine.mines.edu
void convertDNSHostToNormal(unsigned char* dnsHost){
    unsigned int counter;
    unsigned int pos;
    int length = strlen((const char*)dnsHost);
    for(pos = 0; pos < length; pos++){
        counter = dnsHost[pos];
        for(int j = pos; j <= (pos + (int)counter); j++){
            if(j == ((int)counter + pos) ){
                dnsHost[j] = '.';
            }else{
                dnsHost[j] = dnsHost[j+1];
            }
        }
        //takes counter to next number after i increments
        pos += counter;
    }
    //get rid of last .
    dnsHost[pos-1] = 0x00;
}

unsigned char* readName(unsigned char* buffer, unsigned char* parser, int* octetsMoved){
    unsigned int offset;
    unsigned char* name = (unsigned char*)malloc(256);;
    *octetsMoved = 1;
    int counter = 0;
    bool movedToPointer = false;
    //break when end of name
    while(*parser != 0x00){
        //look for pointer
        if(*parser >= POINTER){
            /********************
             * offset = octet1 + octet2 => 256 * octet1 in order to represent as first octet value
             ********************/
            offset = (*parser)*256 + *(parser+1) - POINTER_OFFSET;
            parser = buffer + offset - 1;
            movedToPointer = true;
        }else{
           name[counter] = *parser;
           counter++;
        }
        parser++;
        if(!movedToPointer) (*octetsMoved)++;
    }
    if(movedToPointer) (*octetsMoved)++;
    //null terminate
    name[counter] = 0x00;
    convertDNSHostToNormal(name);
    return name;
}
void readDNSResponse(unsigned char* buffer, unsigned char* questionName){
    DNSHeader* header;
    DNSQuery* query;
    unsigned char* parser;
    int octetsMoved;
    
    header = (DNSHeader*)buffer;
    parser = &buffer[ sizeof(DNSHeader) + strlen((const char*)questionName) + NULL_CHAR_SIZE + sizeof(DNSQueryInfo) ];
    vector<DNSResourceRecord> answers, auth, addl;
    cout << ntohs(header->qdCount) << " questions\n";
    //IF A RETURN, else RECURSIVELYSEARCH
    if( ntohs(header->anCount) > 0 ){
        //answers
        cout <<  ntohs(header->anCount) << " ANSWERS\n";
        for(int i = 0; i < ntohs(header->anCount); i++){
            DNSResourceRecord record;
            record.name = readName(buffer, parser, &octetsMoved);
            parser += octetsMoved;
            unsigned char* lname =  record.name; 
            cout << "Name " << lname << endl;
            record.resourceInfo = (DNSResourceInfo*)parser; 
            //cout << "\nRES " << sizeof(DNSResourceInfo) << endl; 
            parser+= sizeof(DNSResourceInfo);
            //if type A 
            //cout << ntohs(record.resourceInfo->type);
            if(ntohs(record.resourceInfo->type) == 1){
                record.rdata = (unsigned char*)malloc(ntohs(record.resourceInfo->rdLength));
                for(int j = 0; j < ntohs(record.resourceInfo->rdLength); j++){
                    record.rdata[j] = parser[j];
                }
                record.rdata[ntohs(record.resourceInfo->rdLength)] = 0x00;
                parser += ntohs(record.resourceInfo->rdLength);
                //used for printing 
                struct sockaddr_in a;
                long *p;
                p=(long*)record.rdata;
                a.sin_addr.s_addr=(*p); //working without ntohl
                printf("has IPv4 address : %s\n",inet_ntoa(a.sin_addr));

            }else{// if CName
                record.rdata = readName(buffer, parser, &octetsMoved);
                parser += octetsMoved;
                cout << "CName " << record.rdata << endl;
            }
            cout << endl;
            answers.push_back(record);
        }
    }else if( ntohs(header->nsCount) ) { 
        cout << ntohs(header->nsCount) <<  " NS\n";
        for(int i = 0; i < ntohs(header->nsCount); i++){
            DNSResourceRecord record;
            record.name = readName(buffer, parser, &octetsMoved);
            parser += octetsMoved;

            record.resourceInfo = (DNSResourceInfo*)parser;
            parser+= sizeof(DNSResourceInfo);

            record.rdata = readName(buffer, parser, &octetsMoved);
            parser+= octetsMoved;
            cout << "NS name  " << record.rdata << endl;
            auth.push_back(record);
        }

        cout << ntohs(header->arCount) << " addln\n";
        //for(int i = 0; i < ntohs(header->arCount); i++){
        for(int i = 0; i < ntohs(header->nsCount); i++){
            DNSResourceRecord record;

            record.name=readName(buffer, parser, &octetsMoved);
            parser += octetsMoved;

            record.resourceInfo=(DNSResourceInfo*)parser;
            parser += sizeof(DNSResourceInfo);
            cout << record.name;

            if(ntohs(record.resourceInfo->type) == 1 ){
                record.rdata = (unsigned char*)malloc(ntohs(record.resourceInfo->rdLength));
                for(int i = 0; i < ntohs(record.resourceInfo->rdLength); i++){
                    record.rdata[i]=parser[i];

                }
                record.rdata[ntohs(record.resourceInfo->rdLength)] = 0x00;
                parser+=ntohs(record.resourceInfo->rdLength);

                struct sockaddr_in a;
                long *p;
                p=(long*)record.rdata;
                a.sin_addr.s_addr=(*p); //working without ntohl
                printf(" has IPv4 address : %s\n",inet_ntoa(a.sin_addr));

            }
            else{
                cout << "IM GETTING CALLED";
                record.rdata=readName(buffer, parser, &octetsMoved);
                parser += octetsMoved;
            }

           addl.push_back(record);
        }
    }else{

        cout << "NO ANSWERS\n";
    }
    return;
}

void makeDNSQuery(unsigned char* host, const char* serverIP, int clientSocket){
    unsigned char buffer[MAX_MESSAGE_SIZE]; 
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
    if(sendto(clientSocket, (char*)buffer, sizeof(DNSHeader) + sizeof(DNSQueryInfo)  + strlen((const char*)questionName) + NULL_CHAR_SIZE, 0, (sockaddr*)&servaddr, sizeof(servaddr)) < 0){
        cout << "Error sending" << endl;
        exit(-1);
    }
    
    //MAKE RCV CALL
    int size_of_serv_addr = sizeof(servaddr);
    if(recvfrom(clientSocket, (char*)buffer, MAX_MESSAGE_SIZE, 0, (sockaddr*)&servaddr, (socklen_t*)&size_of_serv_addr) < 0){
        cout << "Error receiving." << endl;
        exit(-1);
    }

    //Read 
    readDNSResponse(buffer, questionName);

}
// ***************************************************************************
// * Main
// ***************************************************************************
int main(int argc, char **argv) {
    
    unsigned char* host = (unsigned char*)argv[1];
    const char* serverIP = argv[2];

    if (argc != 3) {
            cout << "useage " << argv[0] << endl;
            exit(-1);
    }
    
    //client socket	
    int clientSocket = -1;
    if( (clientSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        cout << "Failed to create listening socket " << strerror(errno) << endl;
        exit(-1);
    }
    makeDNSQuery(host, serverIP, clientSocket); 
    
    close(clientSocket);
    //cout << buffer; 
    /**************************************************************************
    * HERE DOWN USE FOR DAEMONIZE REFERENCE
    ***************************************************************************/
    // ********************************************************************
	// * Binding configures the socket with the parameters we have
	// * specified in the servaddr structure.  This step is implicit in
	// * the connect() call, but must be explicitly listed for servers.
	// ********************************************************************
	//if (DEBUG)
	//	cout << "Process has bound fd " << clientSocket << " to port " << DNS_PORT << endl;
     
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

