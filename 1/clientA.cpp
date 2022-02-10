#include <iostream>
#include <vector>

#include <openssl/md5.h>

#include "bloom/bloom_filter.hpp"
#include "rpc/include/serialization.h"
#include "rpc/include/rpcClient.h"
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
#define SAMPLES 1000
#define FEATURES 784
std::pair<std::vector<std::string>,std::vector<std::vector<double>>> getMNISTData()
{
    std::pair<std::vector<std::string>,std::vector<std::vector<double>>> result;
    auto mData=mnist::read_MNIST_data(SAMPLES,FEATURES);
    auto label=mnist::read_MNIST_labels(SAMPLES);
    for (int i=0;i<SAMPLES;i++)
        mData[i].second.push_back(label[i]);
    std::random_shuffle(mData.begin(),mData.end());
    for (int i=0;i<SAMPLES*(1-DROPRATE);i++)
    {
        result.first.push_back(mData[i].first);
        result.second.emplace_back(mData[i].second);
    }
    return result;
}

using intersectAFunc=std::function<std::pair<std::vector<int>,std::map<std::string,std::vector<std::string>>>(std::vector<bloom_filter>)>;

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
    rpcClient client("127.0.0.1",8080);
    std::cout<<"发送给服务端..."<<std::endl;
    auto intersect=client.makeRpcCall<intersectAFunc>("intersectA");
    auto res=intersect(filter);
    std::cout<<"接收到服务端返回的数据！"<<std::endl;
    auto &tabelIndex=res.first;
    std::vector<std::string> id;
    std::vector<std::vector<double>> mnistData;
    std::cout<<"调整数据集顺序..."<<std::endl;
    for (auto i:tabelIndex)
    {
        id.emplace_back(id0[i]);
        mnistData.emplace_back(mnistData0[i]);
    }
    std::cout<<"调整后的数据集id:"<<(std::string)serialize::doSerialize(id)<<std::endl;
    std::cout<<"同态加密结果已写入cipherA.txt"<<std::endl;
    freopen("cipherA.txt","w",stdout);
    std::cout<<(std::string)serialize::doSerialize(res.second)<<std::endl;
}