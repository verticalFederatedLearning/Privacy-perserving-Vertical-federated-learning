/*
*   paillierGmpImp类是基于paillier的加密解密类
*   利用gmp库实现
*/
#ifndef PAILLIERGMPIMP_H
#define PAILLIERGMPIMP_H
#include <gmpxx.h>
#include <iostream>
class paillierGmpImp
{
public:
/*
*   这些的注释参见paillier.h
*/
    using bigInteger=mpz_class;
    using privateKey=std::pair<bigInteger,bigInteger>;
    using publicKey=std::pair<bigInteger,bigInteger>;
    std::pair<privateKey,publicKey> genKey();
    bigInteger encrypt(bigInteger,publicKey&);
    bigInteger decrypt(bigInteger,privateKey&);
    bigInteger add(bigInteger,bigInteger,publicKey&);
    bigInteger mul(bigInteger,bigInteger,publicKey&);
    paillierGmpImp(int bitLength=512);
    ~paillierGmpImp()=default;
protected:
    bigInteger n;
    /*
    *   封装了gmp的gcd，逆元以及求幂函数
    */
    bigInteger gcd(bigInteger a,bigInteger b)
    {
        bigInteger g;
        mpz_gcd(g.get_mpz_t(),a.get_mpz_t(),b.get_mpz_t());
        return g;
    }
    bigInteger invert(bigInteger a,bigInteger module)
    {
        bigInteger g;
        mpz_invert(g.get_mpz_t(),a.get_mpz_t(),module.get_mpz_t());
        return g;
    }
    bigInteger pow(bigInteger a,bigInteger n,bigInteger module)
    {
        bigInteger g;
        mpz_powm(g.get_mpz_t(),a.get_mpz_t(),n.get_mpz_t(),module.get_mpz_t());
        return g;
    }
private:
    int bitLength;
    mpz_class randbits(int bits) //随机生成bit位大素数
    {
        gmp_randclass a(gmp_randinit_default);
        a.seed(rand());
        mpz_class l(bits);
        mpz_class b=a.get_z_bits(l);
        mpz_class ret;
        mpz_nextprime(ret.get_mpz_t(), b.get_mpz_t());
        return ret;
    }
};


#endif