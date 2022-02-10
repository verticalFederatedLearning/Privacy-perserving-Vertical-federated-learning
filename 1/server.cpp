#include "rpc/include/rpcServer.h"
#include "rpc/include/tcpServer.h"
#include "bloom/bloom_filter.hpp"
#include "paillierGmpImp.h"

#include <future>
#include <iostream>
#include <sstream>
std::promise<std::vector<bloom_filter>> promiseA,promiseB;

std::string big2str(paillierGmpImp::bigInteger integer)
{
    std::stringstream ss;
    ss<<integer;
    return ss.str();
}
std::pair<std::vector<int>,std::map<std::string,std::vector<std::string>>> intersect(std::vector<bloom_filter> bfA,std::vector<bloom_filter> bfB,bool state)
{
    std::vector<int> res;
    int mp[bfA.size()];
    for (int i=0;i<bfA.size();i++)
    {
        res.push_back(i);
        mp[i]=-1;
    }
    if (bfA.size()<bfB.size()||(state&&bfA.size()==bfB.size()))
    {
        auto encryptor=paillierGmpImp();
        auto [privateKey,publicKey]=encryptor.genKey();
        std::vector<std::string> encrypt;
        for (auto fit1:bfA)
        {
            int index=-1;
            int cnt=0;
            for (auto fit2:bfB)
            {
                int a=fit1.getBitCount();
                int b=fit2.getBitCount();
                int h=(fit1|fit2).getBitCount();
                if (2.0*h/(a+b)<1.2)
                {
                    index=cnt;
                    break;
                }
                cnt++;
            }
            if (index==-1)
                encrypt.push_back(big2str(encryptor.encrypt(paillierGmpImp::bigInteger("0"),publicKey)));
            else
                encrypt.push_back(big2str(encryptor.encrypt(paillierGmpImp::bigInteger("1"),publicKey)));
        };
        std::map<std::string,std::vector<std::string>> result;
        result["cipher"]=encrypt;
        result["publicKey"]=std::vector<std::string>{big2str(publicKey.first),big2str(publicKey.second)};
        return std::make_pair(res,result);
    } 
    else
    {
        int tot=0;
        std::vector<int> rnd;
        for (auto fit1:bfA)
        {
            int index=-1;
            int cnt=0;
            for (auto fit2:bfB)
            {
                int a=fit1.getBitCount();
                int b=fit2.getBitCount();
                int h=(fit1|fit2).getBitCount();
                if (2.0*h/(a+b)<1.2)
                {
                    index=cnt;
                    break;
                }
                cnt++;
            }
            if (index!=-1)
                mp[index]=tot;
            else
                rnd.push_back(tot);
            tot++;
        }
        std::random_shuffle(rnd.begin(),rnd.end());
        std::vector<int> ans;
        int ptr=0;
        auto encryptor=paillierGmpImp();
        auto [privateKey,publicKey]=encryptor.genKey();
        std::vector<std::string> encrypt;
        for (int i=0;i<bfA.size();i++)
        {
            if (mp[i]==-1)
            {
                encrypt.push_back(big2str(encryptor.encrypt(paillierGmpImp::bigInteger("0"),publicKey)));
                ans.push_back(rnd[ptr++]);
            }
            else
            {
                encrypt.push_back(big2str(encryptor.encrypt(paillierGmpImp::bigInteger("1"),publicKey)));
                ans.push_back(mp[i]);
            }
        }
        std::map<std::string,std::vector<std::string>> result;
        result["encrypt"]=encrypt;
        result["publicKey"]=std::vector<std::string>{big2str(publicKey.first),big2str(publicKey.second)};
        std::cout<<"服务端计算完成"<<std::endl;
        return std::make_pair(ans,result);
    }
}
auto intersectA(std::vector<bloom_filter> bfA)
{
    std::cout<<"服务端收到A的数据"<<std::endl;
    bloom_parameters parameters;
    parameters.projected_element_count = 16;
    parameters.minimum_number_of_hashes=2;
    parameters.compute_optimal_parameters();
    for (auto &fit:bfA)
        fit.update(parameters);
    promiseA.set_value(bfA);
    auto bfB=promiseB.get_future().get();
    return intersect(bfA,bfB,false);
}
auto intersectB(std::vector<bloom_filter> bfB)
{
    std::cout<<"服务端收到B的数据"<<std::endl;
    bloom_parameters parameters;
    parameters.projected_element_count = 16;
    parameters.minimum_number_of_hashes=2;
    parameters.compute_optimal_parameters();
    for (auto &fit:bfB)
        fit.update(parameters);
    promiseB.set_value(bfB);
    auto bfA=promiseA.get_future().get();
    return intersect(bfB,bfA,true);
}
int main()
{
    srand(time(NULL));
    rpcHandler* handler=new rpcHandler();
    handler->AddRpcHandler(intersectA);
    handler->AddRpcHandler(intersectB);
    poolServer *server=new poolServer(handler,8080);
    server->startForever();
    return 0;
}