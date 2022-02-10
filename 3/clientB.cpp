#include <iostream>
#include <vector>
#include <sstream>
#include <cmath>

#include <openssl/evp.h>
#include <openssl/ec.h>

#include "rpc/include/rpcServer.h"
#include "bloom/bloom_filter.hpp"
#include "rpc/include/tcpServer.h"
#include "mnist.h"

bloom_parameters paramaters;
#define DROPRATE 0.2
#define SAMPLES 5000
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
auto mnistData=getMNISTData();
std::vector<EC_POINT*> cipherOther,cipher;
std::vector<EC_POINT*> getCipher(std::vector<EC_POINT*> data)
{
    std::cout<<"收到对方密文"<<std::endl;
    for (auto d:data)
        cipherOther.push_back(d);
    auto &[id,_]=mnistData;
    std::vector<EC_POINT*> cipher;
    std::cout<<"正在加密标签"<<std::endl;
    for (auto _id:id)
    {
        EC_POINT *point=EC_POINT_new(group);
        BIGNUM *bn=BN_new();
        BN_set_word(bn,_id);
        EC_POINT_mul(group,point,bn,NULL,NULL,NULL);
        EC_POINT_mul(group,point,NULL,point,privateKey,NULL);
        cipher.push_back(point);
        BN_free(bn);
    }
    std::cout<<"发送密文"<<std::endl;
    return cipher;
}
std::vector<EC_POINT*> getIntersect(std::vector<EC_POINT*> cipherB)
{
    std::cout<<"收到二次加密密文"<<std::endl;
    for (auto *c:cipherOther)
        EC_POINT_mul(group,c,NULL,c,privateKey,NULL);
    std::cout<<"求取交集...."<<std::endl;
    bloom_filter bloom(paramaters);
    for (auto *cb:cipherB)
        bloom.insert((std::string)serialize::doSerialize(cb));
    std::vector<EC_POINT*> res;
    for (auto *c:cipherOther)
        if (bloom.contains((std::string)serialize::doSerialize(c)))
            res.push_back(c);
    int sz=cipherOther.size();
    int p=res.size();
    int q=pow((1.0*sz/p),lm)*p;
    std::cout<<"求解完成，需要加入混淆点的个数为"<<q<<std::endl;
    for (int i=0;i<q;i++)
    {
        EC_POINT *point=EC_POINT_new(group);
        EC_POINT_mul(group,point,rand256(),NULL,NULL,NULL);
        res.push_back(point);
    }
    std::cout<<"发送给对方"<<std::endl;
    return res;
}
std::vector<EC_POINT*> getId(std::vector<EC_POINT*> intersect0)
{
    std::cout<<"接收到一次解密后的交集，进行解密"<<std::endl;
    for (auto &c:intersect0)
        EC_POINT_mul(group,c,NULL,c,keyInv,NULL);
    auto &[id,_]=mnistData;
    bloom_filter bloom(paramaters);
    std::cout<<"求解最终交集"<<std::endl;
    for (auto *i:intersect0)
        bloom.insert((std::string)serialize::doSerialize(i));
    int tot=0;
     for (auto _id:id)
    {
        EC_POINT *point=EC_POINT_new(group);
        BIGNUM *bn=BN_new();
        BN_set_word(bn,_id);
        EC_POINT_mul(group,point,bn,NULL,NULL,NULL);        
        if (bloom.contains((std::string)serialize::doSerialize(point)))
        {
            tot++;
            printf("found %d\n",_id);
        }
        BN_free(bn);
    }
    printf("交集大小：%d\n",tot);
    return intersect0;
}
int main()
{
    std::cout<<"等待交换密文"<<std::endl;
    paramaters.projected_element_count=SAMPLES;
    paramaters.compute_optimal_parameters();
    srand(time(NULL));
    privateKey=rand256();
    keyInv=BN_new();
    BIGNUM *p=BN_new();
    EC_GROUP_get_order(group,p,NULL);
    BN_CTX* ctx=BN_CTX_new();
    BN_mod_inverse(keyInv,privateKey,p,ctx);
    rpcHandler* handler=new rpcHandler();
    handler->AddRpcHandler(getCipher);
    handler->AddRpcHandler(getId);
    handler->AddRpcHandler(getIntersect);
    poolServer *server=new poolServer(handler,8080);
    server->startForever();
}