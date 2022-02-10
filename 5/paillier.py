from math import gcd
from sympy import mod_inverse
from sympy.ntheory.generate import randprime
class paillier:
    bitLength=0
    def __init__(self,bitLength=128) -> None:
        self.bitLength=bitLength
        self.l=1<<bitLength
        self.r=1<<(bitLength+1)
    def randbits(self):
        return randprime(self.l,self.r)
    def genKey(self):
        while (True):
            p=self.randbits()
            q=self.randbits()
            self.n=p*q
            phi=(p-1)*(q-1)
            if (gcd(self.n,phi)==1):
                break
        g=self.n+1
        lam=phi//gcd(p-1,q-1)
        mu=mod_inverse(lam,self.n)
        return [[lam,mu],[self.n,g]]
    def encrypt(self,m,publicKey):
        r=self.randbits()
        n,g=publicKey
        return (pow(g,m,n*n)*pow(r,n,n*n))%(n*n)
    def decrypt(self,c,privateKey):
        lam,mu=privateKey
        mask=pow(c,lam,self.n*self.n)
        power=(mask-1)//self.n
        return (power*mu)%self.n
    def add(self,cipher,num,publicKey):
        n,_=publicKey
        return (cipher*self.encrypt(num,publicKey))%(n*n)
    def mul(self,cipher,num,publicKey):
        n,_=publicKey
        return pow(cipher,num,n*n)
    def div(self,cipher,_num,publicKey):
        n,_=publicKey
        num=mod_inverse(_num,n)
        return pow(cipher,num,n*n)