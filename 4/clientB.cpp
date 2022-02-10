#include <iostream>
#include <vector>
#include <sstream>
#include <cmath>

#include <openssl/evp.h>
#include <openssl/ec.h>

#include "rpc/include/rpcServer.h"
#include "paillierGmpImp.h"
#include "rpc/include/tcpServer.h"
#include "mnist.h"

#define DROPRATE 0.2
#define SAMPLES 100
#define FEATURES 784
double lm=0.5;
std::pair<std::vector<int>,std::vector<std::vector<int>>> getMNISTData()
{
    std::pair<std::vector<int>,std::vector<std::vector<int>>> result;
    auto mData=mnist::read_MNIST_data(SAMPLES,FEATURES);
    std::random_shuffle(mData.begin(),mData.end());
    for (int i=0;i<SAMPLES*(1-DROPRATE);i++)
    {
        
        result.first.push_back(mData[i].first);
        auto md=mData[i].second.begin();
        result.second.emplace_back(std::vector<int>(md,md+FEATURES/2));
    }
    return result;
}
auto* group=EC_GROUP_new_by_curve_name(NID_secp256k1);
BIGNUM *rand256()
{
    BIGNUM *bn=BN_new();
    BN_rand(bn,256,-1,1);
    return bn;
}
BIGNUM *privateKey,*keyInv;
auto MnistData=getMNISTData();
std::vector<EC_POINT*> cipheridOther,cipherid;
std::vector<std::vector<paillierGmpImp::bigInteger>> cipherOther,cipher;
std::vector<std::vector<paillierGmpImp::bigInteger>> randDataOther;
paillierGmpImp::publicKey pubKOther,pubK;
auto paillier=paillierGmpImp();
std::vector<std::vector<int>> randData;
auto keys=paillier.genKey();
auto getCipher(std::pair<std::vector<EC_POINT*>,std::pair<std::vector<std::vector<paillierGmpImp::bigInteger>>,paillierGmpImp::privateKey>> data)
{
    cipheridOther=data.first;
    cipherOther=data.second.first;
    pubKOther=data.second.second;
    std::cout<<"收到对方密文"<<std::endl;
    std::vector<EC_POINT*> cipherid;
    auto &[id,mnistData]=MnistData;
    std::cout<<"正在加密标签"<<std::endl;
    for (auto _id:id)
    {
        EC_POINT *point=EC_POINT_new(group);
        BIGNUM *bn=BN_new();
        BN_set_word(bn,_id);
        EC_POINT_mul(group,point,bn,NULL,NULL,NULL);
        EC_POINT_mul(group,point,NULL,point,privateKey,NULL);
        cipherid.push_back(point);
        BN_free(bn);
    }  
    auto &[privK,pubK]=keys;
    std::cout<<"正在加密样本特征"<<std::endl;
    std::vector<std::vector<paillierGmpImp::bigInteger>> cipher;
    for (auto &dataVec:mnistData)
    {
        std::vector<paillierGmpImp::bigInteger> tmp;
        for (auto &data:dataVec)        
            tmp.emplace_back(paillier.encrypt(paillierGmpImp::bigInteger(data),pubK));
        cipher.emplace_back(std::move(tmp));
    }
    std::cout<<"将密文发送给对方"<<std::endl;
    return std::make_pair(cipherid,std::make_pair(cipher,pubK));
}
auto exchangeCipher(std::pair<std::vector<EC_POINT*>,std::vector<std::vector<paillierGmpImp::bigInteger>>> data)
{
    std::cout<<"收到对方密文"<<std::endl;
    std::cout<<"继续加密样本ID"<<std::endl;
    for (auto *c:cipheridOther)
        EC_POINT_mul(group,c,NULL,c,privateKey,NULL);
    auto &[privK,pubK]=keys;
    std::cout<<"样本特征减去随机数"<<std::endl;
    paillierGmpImp paillierOther;
    paillierOther.n=pubKOther.first;
    for (auto &dataVec:cipherOther)
    {
        std::vector<int> tmpVec;
        for (auto &data:dataVec)
        {
            int tmp=-rand();
            data=paillierOther.add(data,paillierGmpImp::bigInteger(-tmp),pubKOther);
            tmpVec.push_back(tmp);
        }
        randData.emplace_back(std::move(tmpVec));
    }
    std::cout<<"求取交集密文"<<std::endl;
    int sz1=cipheridOther.size();
    std::map<std::string,std::vector<int>> mp;
    for (int i=0;i<sz1;i++)
        mp[(std::string)serialize::doSerialize(cipheridOther[i])]=randData[i];
    auto &[originID,originData]=data;
    std::vector<std::vector<int>> result;
    int sz=originID.size();
    int cnt=0;
    for (int i=0;i<sz;i++)
    {
        auto &id0=originID[i];
        auto &data0=originData[i];
        std::string pb=(std::string)serialize::doSerialize(id0);
        if (mp.find(pb)!=mp.end())
        {
            cnt+=1;
            std::vector<int> tmpVec;
            auto &vecOther=mp[pb];
            for (auto mdata:data0)
            {
                int result=paillier.decrypt(mdata,privK).get_si();
                tmpVec.push_back(result);
            }
            for (auto mdata:vecOther)
            {
                tmpVec.push_back(mdata);
            }
            result.emplace_back(tmpVec);
        }
    }
    std::cout<<"交集个数为"<<cnt<<std::endl;
    std::cout<<"将秘密分享的数据写入datasetB.txt"<<std::endl;
    freopen("datasetB.txt","w",stdout);
    std::cout<<(std::string)serialize::doSerialize(result)<<std::endl;
    return std::make_pair(cipheridOther,cipherOther);
}
int main()
{
    std::cout<<"原始标签:"<<(std::string)serialize::doSerialize(MnistData.first)<<std::endl;
    std::cout<<"等待交换密文"<<std::endl;
    srand(time(NULL));
    privateKey=rand256();
    keyInv=BN_new();
    BIGNUM *p=BN_new();
    EC_GROUP_get_order(group,p,NULL);
    BN_CTX* ctx=BN_CTX_new();
    BN_mod_inverse(keyInv,privateKey,p,ctx);
    rpcHandler* handler=new rpcHandler();
    handler->AddRpcHandler(getCipher);
    handler->AddRpcHandler(exchangeCipher);
    poolServer *server=new poolServer(handler,8080);
    server->startForever();
}
