//separate variable structures out, so we can use strlen on

struct DNSHeader {
    /*unsigned short for 16bit, unsigned char for variable bit*/
    unsigned short id;//identification
    //flags, note order based on big endian form 
    unsigned char rd: 1; //recursion desired
    unsigned char tc: 1; //truncated message
    unsigned char aa: 1; //authoritive answer
    unsigned char opcode: 4; //purpose of message
    
    unsigned char qr: 1; //query/response flag
    unsigned char rcode: 4; //response code
    unsigned char z: 3; //z reserved
    unsigned char ra: 1; //recursion available
    //END FLAGS 
    unsigned short qdCount;//num questions 
    unsigned short anCount;//num answers 
    unsigned short nsCount;//num authority
    unsigned short arCount;//num additional resources
};

struct DNSQueryInfo{
    unsigned short qtype;
    unsigned short qclass;
};
struct DNSQuery{
    unsigned char* name;
    DNSQueryInfo* info;
};
//if used unsined int sizeof is 12 instead of 10
struct DNSResourceInfo{
    //fixed 
    unsigned short type;
    unsigned short class_;
    //unsigned int ttl;
    unsigned short ttl1;
    unsigned short ttl2;
    unsigned short rdLength;
};
struct DNSResourceRecord{
    //variable
    unsigned char* name;
    //fixed info
    DNSResourceInfo* resourceInfo; 
    //variable 
    unsigned char* rdata;
};
struct DNSVarName{
    unsigned char* name;
};
