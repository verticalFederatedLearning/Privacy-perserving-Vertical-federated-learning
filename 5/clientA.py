from convert import convertHelper
from random import randint
from rpc.python.rpcClient import rpcClient
import numpy as np
import phe.encoding
import math
from phe import *
def getPublicKey()->list:
    return None
def getCipher(ua:list)->list:
    return None
class ExampleEncodedNumber(phe.encoding.EncodedNumber):
    BASE = 256
    LOG2_BASE = math.log(BASE, 2)

client=rpcClient("127.0.0.1",8080)
getPublicKey=client.makeRpcCall(getPublicKey)
client1=rpcClient("127.0.0.1",8081)
getCipher=client1.makeRpcCall(getCipher)
_pubK=getPublicKey()
publicKey=PaillierPublicKey(_pubK)
helper=convertHelper(_pubK)
def encrypt(num):
    return publicKey.encrypt(ExampleEncodedNumber.encode(publicKey, num)) 
import dataset
x,_=dataset.load_mnist('./dataset')
x=x[:,0:784//2]
w=[encrypt(randint(-10,10))/100 for i in range(784//2)]

t=0.01
batchSize=100
batches=10
for i in range(batches):
    print("batch:%d"%(i+1))
    lst=[]
    for j in range(batchSize):
        result=encrypt(0)
        index=i*batchSize+j
        for k in range(784//2):
            result+=w[k]*x[index][k]
        lst.append(result)
    _d=getCipher([helper.paillierToObject(item) for item in lst])
    d=[helper.ObjectToPaillier(item) for item in _d]
    res=[encrypt(0) for i in range(784//2)]
    for j in range(batchSize):
        index=i*batchSize+j
        for k in range(784//2):
            res[k]+=d[j]*x[index][k]
    for k in range(784//2):
        res[k]=res[k]/batchSize
        w[k]-=res[k]*t