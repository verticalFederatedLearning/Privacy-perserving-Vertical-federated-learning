#include "rpc/include/rpcServer.h"
#include "rpc/include/tcpServer.h"
#include "bloom/bloom_filter.hpp"

#include <future>
#include <iostream>
#include <sstream>
std::promise<std::vector<bloom_filter>> promiseA,promiseB;
std::promise<std::vector<std::vector<int>>> promiseA1,promiseB1,promiseRandA,promiseRandB;
std::promise<std::vector<int>> promiseA2,promiseB2;

std::vector<int> intersect(std::vector<bloom_filter> bfA,std::vector<bloom_filter> bfB,bool state)
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
        }
        return res;
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
        for (int i=0;i<bfA.size();i++)
        {
            if (mp[i]==-1)
                ans.push_back(rnd[ptr++]);
            else
                ans.push_back(mp[i]);
        }
        std::cout<<"服务端计算完成"<<std::endl;
        return ans;
    }
}
auto intersectA(std::vector<bloom_filter> bfA,std::vector<std::vector<int>> dataA)
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
    auto orderA=intersect(bfA,bfB,false);
    promiseA2.set_value(orderA);
    auto orderB=promiseB2.get_future().get();
    std::vector<std::vector<int>> rndA;
    std::vector<std::vector<int>> dataA1;
    for (auto i:orderA)
        dataA1.emplace_back(dataA[i]);
    for (auto &line:dataA1)
    {
        std::vector<int> tmp;
        for (auto &d:line)
        {
            int tmpi=rand();
            tmp.push_back(tmpi);
            d+=tmpi;
        }
        rndA.emplace_back(tmp);
    }
    promiseRandA.set_value(rndA);
    auto rndB=promiseRandB.get_future().get();
    std::map<std::string,std::vector<std::vector<int>>> mp;
    mp["data"]=dataA1;
    mp["rand"]=rndB;
    return std::make_pair(orderB,mp);

}
auto intersectB(std::vector<bloom_filter> bfB,std::vector<std::vector<int>> dataB)
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
    auto orderB=intersect(bfB,bfA,true);
    promiseB2.set_value(orderB);
    auto orderA=promiseA2.get_future().get();
    std::vector<std::vector<int>> rndB;
    std::vector<std::vector<int>> dataB1;
    for (auto i:orderB)
        dataB1.emplace_back(dataB[i]);
    for (auto &line:dataB1)
    {
        std::vector<int> tmp;
        for (auto &d:line)
        {
            int tmpi=rand();
            tmp.push_back(tmpi);
            d+=tmpi;
        }
        rndB.emplace_back(tmp);
    }
    promiseRandB.set_value(rndB);
    auto rndA=promiseRandA.get_future().get();
    std::map<std::string,std::vector<std::vector<int>>> mp;
    mp["data"]=dataB1;
    mp["rand"]=rndA;
    return std::make_pair(orderA,mp);
}
auto sendSharedDataA(std::vector<std::vector<int>> dataA)
{
    promiseA1.set_value(dataA);
    auto dataB=promiseB1.get_future().get();
    return dataB;
}
auto sendSharedDataB(std::vector<std::vector<int>> dataB)
{
    promiseB1.set_value(dataB);
    auto dataA=promiseA1.get_future().get();
    return dataA;
}
int main()
{
    srand(time(NULL));
    rpcHandler* handler=new rpcHandler();
    handler->AddRpcHandler(intersectA);
    handler->AddRpcHandler(intersectB);
    handler->AddRpcHandler(sendSharedDataA);
    handler->AddRpcHandler(sendSharedDataB);
    poolServer *server=new poolServer(handler,8080);
    server->startForever();
    return 0;
}