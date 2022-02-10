#include <iostream>
#include <vector>
#include <sstream>

#include <openssl/evp.h>
#include <openssl/ec.h>

#include "rpc/include/serialization.h"
#include "rpc/include/rpcClient.h"
#include "mnist.h"


#define SAMPLES 5000
#define FEATURES 784
std::pair<std::vector<int>,std::vector<std::vector<int>>> getMNISTData()
{
    std::pair<std::vector<int>,std::vector<std::vector<int>>> result;
    auto mData=mnist::read_MNIST_data(SAMPLES,FEATURES);
    auto label=mnist::read_MNIST_labels(SAMPLES);
    for (int i=0;i<SAMPLES;i++)
        mData[i].second.push_back(label[i]);
    std::random_shuffle(mData.begin(),mData.end());
    for (int i=0;i<SAMPLES;i++)
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
using getCipherFunc=std::function<std::vector<EC_POINT*>(std::vector<EC_POINT*>)>;
using getIntersectFunc=std::function<std::vector<EC_POINT*>(std::vector<EC_POINT*>)>;
using getIdFunc=std::function<std::vector<EC_POINT*>(std::vector<EC_POINT*>)>;
int main()
{
    srand(time(NULL));
    auto privateKey=rand256();
    auto keyInv=BN_new();
    BIGNUM *p=BN_new();
    EC_GROUP_get_order(group,p,NULL);
    BN_CTX* ctx=BN_CTX_new();
    BN_mod_inverse(keyInv,privateKey,p,ctx);
    rpcClient client("127.0.0.1",8080);
    std::vector<EC_POINT*> cipher;
    auto [id,mnistData]=getMNISTData();
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
    auto getCipher=client.makeRpcCall<getCipherFunc>("getCipher");
    auto cipherOther=getCipher(cipher);
    std::cout<<"收到对方密文"<<std::endl;
    std::cout<<"进行二次加密"<<std::endl;
    for (auto *c:cipherOther)
        EC_POINT_mul(group,c,NULL,c,privateKey,NULL);
    std::cout<<"把二次加密密文发送给对方"<<std::endl;
    auto getIntersect=client.makeRpcCall<getIntersectFunc>("getIntersect");
    auto intersect=getIntersect(cipherOther);
    std::cout<<"接收到双方交集，进行第一次解密"<<std::endl;
    for (auto *c:intersect)
        EC_POINT_mul(group,c,NULL,c,keyInv,NULL);
    auto getId=client.makeRpcCall<getIdFunc>("getId");
    auto data=getId(intersect);
    std::cout<<"接收到编码后的交集，交集为："<<std::endl;
    std::cout<<(std::string)serialize::doSerialize(data)<<std::endl;
}