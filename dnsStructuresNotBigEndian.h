
struct DNSHeader {
    /*unsigned short for 16bit, unsigned char for variable bit*/
    unsigned short id;//identification
    //flags, note maintain order when calling in code
    unsigned char qr: 1; //query/response flag
    unsigned char opcode: 4; //purpose of message
    unsigned char aa: 1; //authoritive answer
    unsigned char tc: 1; //truncated message
    
    unsigned char rd: 1; //recursion desired
    unsigned char ra: 1; //recursion available
    unsigned char z: 3; //z reserved
    unsigned char rcode: 4; //response code
    //END FLAGS 
    unsigned short qdCount;//num questions 
    unsigned short anCount;//num answers 
    unsigned short nsCount;//num authority
    unsigned short arCount;//num additional resources
};

struct DNSQuery {
    unsigned char* name;
    unsigned short qtype;
    unsigned short qclass;
};

struct DNSResourceRecord{
    //variable
    unsigned char* name;
    //fixed 
    unsigned short type;
    unsigned short class_;
    unsigned int ttl;
    unsigned short rdLength;
    //variable 
    unsigned char* rdata;
};
