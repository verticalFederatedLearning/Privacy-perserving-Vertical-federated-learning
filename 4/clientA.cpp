#include <iostream>
#include <vector>
#include <sstream>

#include <openssl/evp.h>
#include <openssl/ec.h>

#include "rpc/include/serialization.h"
#include "rpc/include/rpcClient.h"
#include "paillierGmpImp.h"
#include "mnist.h"


#define SAMPLES 100
#define DROPRATE 0.2
#define FEATURES 784
std::pair<std::vector<int>,std::vector<std::vector<int>>> getMNISTData()
{
    std::pair<std::vector<int>,std::vector<std::vector<int>>> result;
    auto mData=mnist::read_MNIST_data(SAMPLES,FEATURES);
    auto label=mnist::read_MNIST_labels(SAMPLES);
    for (int i=0;i<SAMPLES*(1-DROPRATE);i++)
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
auto* group=EC_GROUP_new_by_curve_name(NID_secp256k1);
BIGNUM *rand256()
{
    BIGNUM *bn=BN_new();
    BN_rand(bn,256,-1,1);
    return bn;
}
using getCipherFunc=std::function<std::pair<std::vector<EC_POINT*>,std::pair<std::vector<std::vector<paillierGmpImp::bigInteger>>,paillierGmpImp::privateKey>>(std::pair<std::vector<EC_POINT*>,std::pair<std::vector<std::vector<paillierGmpImp::bigInteger>>,paillierGmpImp::privateKey>>)>;
using exchangeCipherFunc=std::function<std::pair<std::vector<EC_POINT*>,std::vector<std::vector<paillierGmpImp::bigInteger>>>(std::pair<std::vector<EC_POINT*>,std::vector<std::vector<paillierGmpImp::bigInteger>>>)>;
int main()
{
    srand(time(NULL));
    auto privateKey=rand256();
    auto keyInv=BN_new();
    BIGNUM *p=BN_new();
    EC_GROUP_get_order(group,p,NULL);
    BN_CTX* ctx=BN_CTX_new();
    BN_mod_inverse(keyInv,privateKey,p,ctx);
    std::vector<EC_POINT*> cipherid;
    auto [id,mnistData]=getMNISTData();
    std::cout<<"原始标签:"<<(std::string)serialize::doSerialize(id)<<std::endl;
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
    std::cout<<"正在加密样本特征"<<std::endl;
    auto paillier=paillierGmpImp();
    auto [privK,pubK]=paillier.genKey();
    std::vector<std::vector<paillierGmpImp::bigInteger>> cipher;
    for (auto &dataVec:mnistData)
    {
        std::vector<paillierGmpImp::bigInteger> tmp;
        for (auto &data:dataVec)
            tmp.emplace_back(paillier.encrypt(paillierGmpImp::bigInteger(data),pubK));
        cipher.emplace_back(std::move(tmp));
    }
    rpcClient client("127.0.0.1",8080);
    auto getCipher=client.makeRpcCall<getCipherFunc>("getCipher");
    std::cout<<"将密文发送给对方"<<std::endl;
    auto [cipheridOther,enc]=getCipher(std::make_pair(cipherid,std::make_pair(cipher,pubK)));
    std::cout<<"收到对方密文"<<std::endl;
    auto [cipherOther,pubKOther]=enc;
    std::cout<<"继续加密样本ID"<<std::endl;
    for (auto *c:cipheridOther)
        EC_POINT_mul(group,c,NULL,c,privateKey,NULL);
    std::cout<<"样本特征减去随机数"<<std::endl;
    std::vector<std::vector<int>> randData;
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
    std::cout<<"将二次加密的密文发送给对方"<<std::endl;
    auto exchangeCipher=client.makeRpcCall<exchangeCipherFunc>("exchangeCipher");
    auto randDataOther=exchangeCipher(std::make_pair(cipheridOther,cipherOther));
    std::cout<<"收到对方密文"<<std::endl;
    std::cout<<"求取交集密文"<<std::endl;
    auto &[originID,originData]=randDataOther;
    int sz1=originID.size();
    std::map<std::string,std::vector<paillierGmpImp::bigInteger>> mp;
    for (int i=0;i<sz1;i++)
        mp[(std::string)serialize::doSerialize(originID[i])]=originData[i];
    std::vector<std::vector<int>> result;
    int sz=cipheridOther.size();
    int cnt=0;
    for (int i=0;i<sz;i++)
    {
        auto &id0=cipheridOther[i];
        auto &data0=randData[i];
        std::string pb=(std::string)serialize::doSerialize(id0);
        if (mp.find(pb)!=mp.end())
        {
            cnt+=1;
            std::vector<int> tmpVec;
            auto &vecOther=mp[pb];
            for (auto mdata:data0)
            {
                tmpVec.push_back(mdata);
            }
            for (auto mdata:vecOther)
            {
                int result=paillier.decrypt(mdata,privK).get_si();
                tmpVec.push_back(result);
            }
            result.emplace_back(tmpVec);
        }
    }
    std::cout<<"交集个数为"<<cnt<<std::endl;
    std::cout<<"将秘密分享的数据写入datasetA.txt"<<std::endl;
    freopen("datasetA.txt","w",stdout);
    std::cout<<(std::string)serialize::doSerialize(result)<<std::endl;
}