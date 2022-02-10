#ifndef RPCPARSER_H
#define RPCPARSER_H
#include <iostream>

#include "jsonParser.h"
#define RPCMESSAGELENGTH 8
struct  rpcMessage
{
    int length;
    std::string message;
    rpcMessage(int length,std::string message)
    {
        this->length=length;
        this->message=message;
    }
    rpcMessage(std::string message):rpcMessage(message.length(),message)
    {

    }
    rpcMessage(JsonParser message):rpcMessage(std::string(message))
    {

    }
    virtual std::string toString()
    {
        char len[RPCMESSAGELENGTH+2];
        char format[]="%0xd";
        format[2]='0'+RPCMESSAGELENGTH;
        sprintf(len,format,length);
        std::string result(len);
        result+="\r\n"+message+"\r\n";
        return result;
    }
};
class rpcParser
{
private:
    /* data */
public:
    rpcParser(/* args */)=default;
    static rpcMessage parse(std::string rpcRequest);
    ~rpcParser()=default;
};
#endif