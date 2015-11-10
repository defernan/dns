#include "includes.h"

#define MAX_MESSAGE_SIZE 1024//65535 //2^16-1
#define DNS_PORT 53
#define PORT 9004
#define NULL_CHAR_SIZE 1
#define POINTER 192 //1100 0000 
#define POINTER_OFFSET 49152 //1100 0000 0000 0000

bool DEBUG = false;
bool SERVER_MODE = false;
bool answerFound = false;
bool badResponse = false;
unsigned char digBuffer[MAX_MESSAGE_SIZE];
unsigned char* host; 
int clientSocket = 0;
int dnsSocket = 0;
unsigned short digID = 0;
int sizeOfMessage = 0;
vector<DNSResourceRecord> answers;
//avoid endless searching
set<string> serversSearched;
//fill in dns header
void fillDNSHeaderReq( DNSHeader* header );
void fillDNSHeaderResponse( DNSHeader* header, unsigned short answers );

//writes in query name to buffer in correct format ie 7imagine5mines3edu
void writeHostToDNSBuffer(unsigned char* host, unsigned char* buffer);

//convert name to readable format ie 7imagine5mines3edu => imagine.mines.edu
void convertDNSHostToNormal(unsigned char* dnsHost);
 
//reads a name from a dns response
unsigned char* readName(unsigned char* buffer, unsigned char* parser, int* octetsMoved);

//populates a dns resource record from dns response, returns parser so i can keep my location
unsigned char* populateResourceRecord(unsigned char* buffer, unsigned char* parser, DNSResourceRecord &record);

//prints answers
void printAnswers(vector<DNSResourceRecord> &answers);

//recursively search if no answers found
void recursivelySearch(vector<DNSResourceRecord> &auth, vector<DNSResourceRecord> &addl);

//checks if addl record is one of the authority records provided
bool recordsCorrespond(DNSResourceRecord addl, vector<DNSResourceRecord> &auth);

//checks if bad response ie bad domain name server error
void checkForBadResponse(DNSHeader* header);

//read dns response from buffer
void readDNSResponse(unsigned char* buffer, unsigned char* questionName);

//make a dns query to a server
void makeDNSQuery(unsigned char* host, const char* serverIP);
char* readDIGResponse(unsigned char* buffer, unsigned char* serverIP);
void replyToDIG(int &messageSize);
// ***************************************************************************
// * Main
// ***************************************************************************
int main(int argc, char **argv) {
    
    if( string(argv[1]) == "-d" ) DEBUG = true;
    if( string(argv[1]) == "-f" ) SERVER_MODE = true;
    const char* serverIP; 
    struct sockaddr_in  servaddr;
    
    if(SERVER_MODE){
        serverIP = argv[2];
        unsigned char buff[MAX_MESSAGE_SIZE];  
        //set socket up to listen
        clientSocket = -1; 

        clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
        cout << "Process has bound fd " << clientSocket << " to port " << PORT << endl;
        

        // Zero the whole thing.
        bzero(&servaddr, sizeof(servaddr));
        // IPv4 Protocol Family
        servaddr.sin_family = AF_INET;
        // Let the system pick the IP address.
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        // You pick a random high-numbered port
        servaddr.sin_port = htons(PORT);

        if (bind(clientSocket, (sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
            cout << "bind() failed: " << strerror(errno) << endl;
            exit(-1);
        }

        /* setsockopt: Handy debugging trick that lets 
        * us rerun the server immediately after we kill it; 
        * otherwise we have to wait about 20 secs. 
        * Eliminates "ERROR on binding: Address already in use" error. 
        */
        int optval = 1;
        setsockopt(clientSocket, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));
        while(true){
            //MAKE RCV CALL
            int size_of_serv_addr = sizeof(servaddr);
            
            if(recvfrom(clientSocket, (char*)buff, MAX_MESSAGE_SIZE, 0, (sockaddr*)&servaddr, (socklen_t*)&size_of_serv_addr) < 0){
                cout << "Error receiving." << endl;
                exit(-1);
            }else{
                cout << "Message receieved\n";
                host = (unsigned char*)readDIGResponse(buff, (unsigned char*)serverIP);
                break;
            }
        
        }

    }else if(DEBUG){
        host = (unsigned char*)argv[2];
        serverIP = argv[3];
    }else{
        host = (unsigned char*)argv[1];
        serverIP = argv[2];
    }
        
    //dns socket	
    dnsSocket = -1;
    if( (dnsSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        cout << "Failed to create listening socket " << strerror(errno) << endl;
        exit(-1);
    }
    makeDNSQuery(host, serverIP); 
    if(!answerFound){
        cout << "No Answers Found!";
    }
    close(dnsSocket);
    //printAnswers(answers);  
    if(SERVER_MODE){
        //int sizeOfMessage;
        //replyToDIG( sizeOfMessage); 
         
        //MAKE SEND CALL
        if(sendto(clientSocket, (char*)digBuffer, MAX_MESSAGE_SIZE, 0, (sockaddr*)&servaddr, sizeof(servaddr)) < 0){
            cout << "Error sending daemonize" << endl;
            exit(-1);
        }
        close(clientSocket);
    }
 
}

//make a dns query to a server
void makeDNSQuery(unsigned char* host, const char* serverIP){
    if(DEBUG){
        cout << "/////////////////////////////////////\n";
        cout << "Querying " << serverIP << " for " << host << endl;
        cout << "/////////////////////////////////////\n";
    }
    unsigned char buffer[MAX_MESSAGE_SIZE]; 
    unsigned char* questionName;
    
    //dns struct
    DNSHeader* dnsHeader = (DNSHeader*)&buffer;
    //populate dns header info
    fillDNSHeaderReq(dnsHeader);
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
    if(sendto(dnsSocket, (char*)buffer, sizeof(DNSHeader) + sizeof(DNSQueryInfo)  + strlen((const char*)questionName) + NULL_CHAR_SIZE, 0, (sockaddr*)&servaddr, sizeof(servaddr)) < 0){
        cout << "Error sending" << endl;
        exit(-1);
    }
    
    //MAKE RCV CALL
    int size_of_serv_addr = sizeof(servaddr);
    if(recvfrom(dnsSocket, (char*)buffer, MAX_MESSAGE_SIZE, 0, (sockaddr*)&servaddr, (socklen_t*)&size_of_serv_addr) < 0){
        cout << "Error receiving." << endl;
        exit(-1);
    }

    //Read 
    readDNSResponse(buffer, questionName);
    

}

//fill in header
void fillDNSHeaderReq( DNSHeader* header){
    header->id = (unsigned short)htons(getpid());
    header->qr = 0; 
    header->opcode = 0; //0: standard query 1:standard response
    header->aa = 0; //not authoritative
    header->tc = 0; //not truncated
    header->rd = 0; //recursion Desired
    header->ra = 0; //recursion not available 
    header->z = 0;
    header->rcode = 0;
    header->qdCount = htons(1); //1 question, big endian form
    header->anCount = 0;
    header->nsCount = 0;
    header->arCount = 0;
}
void fillDNSHeaderResponse( DNSHeader* header, unsigned short answerCount ){
    header->id = (unsigned short)htons(getpid());
    header->qr = 1; 
    header->opcode = 0; //0: standard query 
    header->aa = 0; //not authoritative
    header->tc = 0; //not truncated
    header->rd = 0; //recursion Desired
    header->ra = 0; //recursion not available 
    header->z = 0;
    header->rcode = 0;
    header->qdCount = htons(1); 
    header->anCount = htons(answerCount);
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
    int counter;
    int pos;
    int length = (int)strlen((const char*)dnsHost);
    for(pos = 0; pos < (int)strlen((const char*)dnsHost); pos++){
        counter = dnsHost[pos];
        for(int j = 0; j < (int)counter; j++){
            dnsHost[pos] = dnsHost[pos+1];
            pos+= 1;
        }
        dnsHost[pos] = '.';
    }
    //get rid of last .
    dnsHost[pos-1] = 0x00;
}

//read dns response from buffer
void readDNSResponse(unsigned char* buffer, unsigned char* questionName){
    DNSHeader* header;
    DNSQuery* query;
    unsigned char* parser;
    
    header = (DNSHeader*)buffer;
    parser = &buffer[ sizeof(DNSHeader) + strlen((const char*)questionName) + NULL_CHAR_SIZE + sizeof(DNSQueryInfo) ];
    //check response
    checkForBadResponse(header);
    if(badResponse) return;

    vector<DNSResourceRecord> auth, addl;
    //IF A RETURN, else RECURSIVELY SEARCH
    if( ntohs(header->anCount) > 0 ){
        answerFound = true;
        for(int i = 0; i < ntohs(header->anCount); i++){
            DNSResourceRecord record;
            parser = populateResourceRecord(buffer, parser, record);
            if(ntohs(record.resourceInfo->type) == 1 || ntohs(record.resourceInfo->type) == 5 ){
                answers.push_back(record);
            }
        }
        //print answers return
        if(header->aa == 1){
            cout << "Answer is authoritive\n";
        }else {
            cout << "Answer is non authoritive\n";
        }
        printAnswers(answers);
        if(SERVER_MODE){
            //int sizeOfMessage;
            replyToDIG(sizeOfMessage); 
         
        }
    }else if( ntohs(header->nsCount) > 0 ) { 
        for(int i = 0; i < ntohs(header->nsCount); i++){
            DNSResourceRecord record;
            parser = populateResourceRecord(buffer, parser, record);
            if(ntohs(record.resourceInfo->type) == 2){
                auth.push_back(record);
            }
        
        }

        for(int i = 0; i < ntohs(header->arCount); i++){
            DNSResourceRecord record;
            parser = populateResourceRecord(buffer, parser, record);
            if(ntohs(record.resourceInfo->type) == 1){
                addl.push_back(record);
            }
        }
        //recursively search
        recursivelySearch(auth, addl);
    }
    return;
}
//lets response know if bad
void checkForBadResponse(DNSHeader* header){
    if((int)header->rcode == 2){
        badResponse = true; 
        cout << "Error: " <<  (int)header->rcode << ". Server failed to complete request.\n";
    }else if((int)header->rcode == 3){
        badResponse = true;
        cout << "Error: " <<  (int)header->rcode << ". Domain name does not exist.\n";
    }else if( (int)header->rcode == 5){
        badResponse = true;
        cout << "Error: " <<  (int)header->rcode << ", Server refused to answer.\n"; 
    }
}
//populates a dns resource record
//return parser so i can keep my location
unsigned char* populateResourceRecord(unsigned char* buffer, unsigned char* parser, DNSResourceRecord &record){
    int octetsMoved;

    record.name=readName(buffer, parser, &octetsMoved);
    parser += octetsMoved;
    record.resourceInfo = (DNSResourceInfo*)parser;
    parser += sizeof(DNSResourceInfo);
    //a type 
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

    }
    else if(ntohs(record.resourceInfo->type) == 5 || ntohs(record.resourceInfo->type) == 2){//cname or ns
        record.rdata=readName(buffer, parser, &octetsMoved);
        parser += octetsMoved;
    }else{
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
    }
    return parser;
}



//reads a name from a dns response
unsigned char* readName(unsigned char* buffer, unsigned char* parser, int* octetsMoved){
    unsigned int offset;
    unsigned char* name = (unsigned char*)malloc(256);
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

//prints answers
void printAnswers(vector<DNSResourceRecord> &answers){
    cout << "ANSWERS FOR  " << host <<  " ARE:" << endl;
    for(int i = 0; i < answers.size(); i++){
        if(ntohs(answers[i].resourceInfo->type) == 1){
            struct sockaddr_in a;
            long *p;
            p=(long*)answers[i].rdata;
            a.sin_addr.s_addr=(*p); //working without ntohl
            const char* serverIP = inet_ntoa(a.sin_addr);
            cout << "IP: " << serverIP << endl;
        }else if(ntohs(answers[i].resourceInfo->type) == 5){
            cout << "CNAME: " << answers[i].rdata << endl;
        }

    }
    cout << endl;
}

//recursively search if no answers found
void recursivelySearch(vector<DNSResourceRecord> &auth, vector<DNSResourceRecord> &addl){
     
    for(int i = 0; i < addl.size(); i++){
        if(answerFound) break;
            struct sockaddr_in a;
            long *p;
            p=(long*)addl[i].rdata;
            a.sin_addr.s_addr=(*p); //working without ntohl
            const char* serverIP = inet_ntoa(a.sin_addr);
        //check if already searched ip
        if( serversSearched.find(serverIP) == serversSearched.end() ) {
            if(badResponse) return;
            serversSearched.insert(serverIP); 
            makeDNSQuery( host,  serverIP);
        }

    }

}

//read dns response from buffer
char* readDIGResponse(unsigned char* buffer, unsigned char* serverIP){
    DNSHeader* header;
    DNSQuery* query;
    unsigned char* parser;
    int temp = 0;
    header = (DNSHeader*)buffer;
    digID = header->id;
    parser = &buffer[ sizeof(DNSHeader) ];
    char* question = (char*)readName(buffer, parser, &temp);
    return question;
}
//send response to DIG
void replyToDIG(int &sizeOfMessage){
    //unsigned char*pos = buffer;
    unsigned char* questionName;
    int sizeOfAnswers = 0;
    //dns struct
    DNSHeader* dnsHeader = (DNSHeader*)&digBuffer;
    //populate dns header info
    fillDNSHeaderResponse(dnsHeader, answers.size());
    dnsHeader->id = digID;
    //move to buffer spot after header info
    questionName = (unsigned char*)&digBuffer[sizeof(DNSHeader)];
    writeHostToDNSBuffer(host, questionName); 
    DNSQueryInfo* queryInfo = (DNSQueryInfo*)&digBuffer[sizeof(DNSHeader) + strlen((const char*)questionName) + NULL_CHAR_SIZE ]; //+1 for null char
    //populate query field 
    queryInfo->qtype = htons(1); //A type
    queryInfo->qclass = htons(1); //1 for internet
    int position =  sizeof(DNSHeader) + strlen((const char*)questionName) + NULL_CHAR_SIZE + sizeof(DNSQueryInfo);
    DNSResourceRecord* record =(DNSResourceRecord*)&digBuffer[ position  ];
    for(int i = 0; i < answers.size(); i++){
        unsigned char* rec = (unsigned char*)&digBuffer[ position ];
        writeHostToDNSBuffer(answers[i].name, rec); 
        position +=  strlen((const char*)rec) + NULL_CHAR_SIZE;
        DNSResourceInfo* recInfo = (DNSResourceInfo*)&digBuffer[ position ];
        recInfo->type = answers[i].resourceInfo->type;
        recInfo->class_ = answers[i].resourceInfo->class_;
        recInfo->ttl1 = answers[i].resourceInfo->ttl1;
        recInfo->ttl2 = answers[i].resourceInfo->ttl2;
        recInfo->rdLength = answers[i].resourceInfo->rdLength;
        position += sizeof(DNSResourceInfo);
        //cout << sizeof(DNSResourceInfo);
        int rdataSize;
        //cout << "\n " << ntohs(recInfo->type)  << " ^^^ bufh " << ntohs(recInfo->rdLength) << "\n"; 
        if(ntohs(recInfo->type) == 1){
            unsigned char* recrdata = (unsigned char*)&digBuffer[position];
            for(int j = 0; j < ntohs(recInfo->rdLength); j++){
                recrdata[j]=answers[i].rdata[j];

            }
            recrdata[ntohs(recInfo->rdLength)] = 0x00;
            position+=ntohs(recInfo->rdLength);
            rdataSize += ntohs(recInfo->rdLength);
        }else if( ntohs(recInfo -> type) == 5){
            unsigned char* recrdata = (unsigned char*)&digBuffer[position];  
            writeHostToDNSBuffer(answers[i].rdata, recrdata);
            position += strlen((const char*)recrdata) + NULL_CHAR_SIZE;
            rdataSize =  strlen((const char*)recrdata) + NULL_CHAR_SIZE;
        }
        if(i != answers.size() - 1) position += 1;
                
        int sizeOfAnswer;
        sizeOfAnswer = strlen((const char*)rec) + NULL_CHAR_SIZE + rdataSize + sizeof(DNSResourceInfo);
        sizeOfAnswers += sizeOfAnswer;
    }
    sizeOfMessage = sizeof(DNSHeader) + sizeof(DNSQueryInfo)  + strlen((const char*)questionName) + NULL_CHAR_SIZE + sizeOfAnswers;

}
