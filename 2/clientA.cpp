#include <iostream>
#include <vector>

#include <openssl/md5.h>

#include "bloom/bloom_filter.hpp"
#include "rpc/include/serialization.h"
#include "rpc/include/rpcClient.h"
#include "rpc/include/rpcParser.h"
#include "mnist.h"

bloom_parameters parameters;
std::vector<unsigned char> md5(std::string str)
{
    MD5_CTX ctx;
    unsigned char md[16];
    MD5_Init(&ctx);
    MD5_Update(&ctx,str.c_str(),str.length());
    std::vector<unsigned char> res;
    MD5_Final(md,&ctx);
    for (int i=0;i<16;i++)
        res.push_back(md[i]);
    return res;
} 
std::vector<bloom_filter> genBloom(std::vector<std::string> label)
{
    std::vector<bloom_filter> result;
    for (auto now:label)
    {
        bloom_filter filter(parameters);
        auto vec=md5(now);
        for (auto c:vec)
        {
            std::string temp=" ";
            temp[0]=c;
            filter.insert(temp);
        }
        result.emplace_back(filter);
    }
    return result;
}
#define DROPRATE 0.2
#define SAMPLES 5000
#define FEATURES 784
std::pair<std::vector<std::string>,std::vector<std::vector<int>>> getMNISTData()
{
    std::pair<std::vector<std::string>,std::vector<std::vector<int>>> result;
    auto mData=mnist::read_MNIST_data(SAMPLES,FEATURES);
    auto label=mnist::read_MNIST_labels(SAMPLES);
    for (int i=0;i<SAMPLES;i++)
        mData[i].second.push_back(label[i]);
    std::random_shuffle(mData.begin(),mData.end());
    for (int i=0;i<SAMPLES*(1-DROPRATE);i++)
    {
        result.first.push_back(mData[i].first);
        auto md=mData[i].second.begin();
        auto mde=mData[i].second.end();
        result.second.emplace_back(std::vector<int>(md+FEATURES/2,mde));
    }
    return result;
}
std::pair<std::vector<std::vector<int>>,std::vector<std::vector<int>>> getSharedData(std::vector<std::vector<int>> &data)
{
    std::vector<std::vector<int>> vec1,vec2;
    for (auto &&line:data)
    {
        std::vector<int> tmp1,tmp2;
        for (auto &&d:line)
        {
            int rnd=rand();
            tmp1.emplace_back(rnd);
            tmp2.emplace_back(d-rnd);
        }
        vec1.emplace_back(tmp1);
        vec2.emplace_back(tmp2);
    }
    return std::make_pair(vec1,vec2);
}
using intersectAFunc=std::function<std::pair<std::vector<int>,std::map<std::string,std::vector<std::vector<int>>>>(std::vector<bloom_filter>,std::vector<std::vector<int>>)>;
#define LISTENQ 5
#define MAXLINE 1024
class simpleRPCServer
{
private:
    int listenfd;
public:
    simpleRPCServer(int port,std::string ipAddress="127.0.0.1")
    {
        sockaddr_in servaddr;
        listenfd=socket(AF_INET,SOCK_STREAM,0);
        if (listenfd==-1)
            throw std::runtime_error("socket error");
        bzero(&servaddr,sizeof(servaddr));
        servaddr.sin_family=AF_INET;
        inet_pton(AF_INET,ipAddress.c_str(),&servaddr.sin_addr);
        servaddr.sin_port=htons(port);
        if (bind(listenfd,(sockaddr*)&servaddr,sizeof(servaddr))==-1)
            throw std::runtime_error("bind error");
        listen(listenfd,LISTENQ);
    }
    std::vector<std::vector<int>> sendSharedData(std::vector<std::vector<int>> data)
    {
        sockaddr_in clientaddr;
        socklen_t clientAddrLen=sizeof(clientaddr);
        char buf[MAXLINE];
        auto connfd=accept(listenfd,(sockaddr*)&clientaddr,&clientAddrLen);
        auto n=read(connfd,buf,MAXLINE);
        std::string message(buf,n);
        auto rpcRequest=rpcParser::parse(message);
        if (rpcRequest.length>rpcRequest.message.length())
        {
            std::string now=message;
            int length=rpcRequest.length;
            while (now.length()<length)
            {
                auto n=read(connfd,buf,MAXLINE);
                now+=std::string(buf,n);
            }
            rpcRequest=rpcParser::parse(now);
        }    
        auto msg=JsonParser(&rpcRequest.message);
        auto responseText=rpcMessage(serialize::doSerialize(data)).toString();
        int wrote=write(connfd,responseText.c_str(),responseText.length());
        return serialize::doUnSerialize<std::vector<std::vector<int>>>(msg["param1"]);
    }
    ~simpleRPCServer()
    {
        close(listenfd);
    }
};


int main()
{
    srand(time(NULL));
    parameters.projected_element_count = 16;
    parameters.minimum_number_of_hashes=2;
    parameters.compute_optimal_parameters();
    std::cout<<"加载mnist数据集..."<<std::endl;
    auto [id0,mnistData0]=getMNISTData();
    std::cout<<"刚开始的id:"<<(std::string)serialize::doSerialize(id0)<<std::endl;
    std::cout<<"生成布隆过滤器..."<<std::endl;
    auto filter=genBloom(id0);
    std::cout<<"进行秘密分享..."<<std::endl;
    auto [shared0,shared1]=getSharedData(mnistData0);
    rpcClient client("127.0.0.1",8080);
    std::cout<<"发送秘密份额给对方..."<<std::endl;
    simpleRPCServer server(8001);
    auto sharedOther=server.sendSharedData(shared1);
    std::cout<<"接收到对方秘密份额！"<<std::endl;
    std::cout<<"发送布隆过滤器和秘密份额给服务端..."<<std::endl;
    auto intersect=client.makeRpcCall<intersectAFunc>("intersectA");
    auto res=intersect(filter,shared0);//返回值，前半部分是要求的顺序，后半部分是A的份额
    std::cout<<"接收到服务端返回的数据！"<<std::endl;
    auto &tabelIndex=res.first;
    std::vector<std::string> id;
    std::vector<std::vector<int>> mnistData;
    std::cout<<"调整数据集顺序..."<<std::endl;
    for (auto i:tabelIndex)
        mnistData.emplace_back(sharedOther[i]);
    auto &rnd=res.second["rand"];
    auto &data=res.second["data"];
    int sz1=rnd.size(),sz2=rnd[0].size();
    for (int i=0;i<sz1;i++)
    {
        for (int j=0;j<sz2;j++)
            mnistData[i][j]-=rnd[i][j];
    }
    for (int i=0;i<sz1;i++)
    {
        for (auto &&dat:data[i])
            mnistData[i].push_back(dat);
    }
    std::cout<<"将秘密分享数据集写入datasetA.txt"<<std::endl;
    freopen("datasetA.txt","w",stdout);
    std::cout<<(std::string)serialize::doSerialize(mnistData)<<std::endl;
    std::cerr<<"写入完成"<<std::endl;
}