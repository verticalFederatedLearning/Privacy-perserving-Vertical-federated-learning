#include "paillierGmpImp.h"
paillierGmpImp::paillierGmpImp(int bitLength)
{
    this->bitLength=bitLength;
}
std::pair<paillierGmpImp::privateKey,paillierGmpImp::publicKey> paillierGmpImp::genKey()
{
    bigInteger p,q,phi;
    while (true)
    {
        p=randbits(bitLength);
        q=randbits(bitLength);
        n=p*q;
        phi=(p-1)*(q-1);
        if (gcd(n,phi)==1)
            break;
    }    
    bigInteger g=n+1;
    bigInteger lambda = phi/gcd(p-1,q-1);
    bigInteger mu=invert(lambda,n);
    auto publicKey=std::make_pair(n,g);
    auto privateKey=std::make_pair(lambda,mu);
    return std::make_pair(privateKey,publicKey);
}
paillierGmpImp::bigInteger paillierGmpImp::encrypt(paillierGmpImp::bigInteger m,paillierGmpImp::publicKey& key)
{
    bigInteger r=randbits(bitLength);
    auto &&[n,g]=key;
    return (pow(g,m,n*n)*pow(r,n,n*n))%(n*n);
}
paillierGmpImp::bigInteger paillierGmpImp::decrypt(paillierGmpImp::bigInteger c,paillierGmpImp::privateKey& key)
{
    auto &&[lambda,mu]=key;
    bigInteger mask=pow(c,lambda,n*n);
    bigInteger power=(mask-1)/n;
    return (power*mu)%n;
}
paillierGmpImp::bigInteger paillierGmpImp::add(paillierGmpImp::bigInteger cipher,paillierGmpImp::bigInteger num,paillierGmpImp::publicKey& key)
{
    return (cipher*encrypt(num,key))%(n*n);
}
paillierGmpImp::bigInteger paillierGmpImp::mul(paillierGmpImp::bigInteger cipher,paillierGmpImp::bigInteger num,paillierGmpImp::publicKey& key)
{
    return pow(cipher,num,n*n);
}