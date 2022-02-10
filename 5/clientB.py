from convert import convertHelper
from random import randint
import numpy as np
import phe.encoding
import math
from phe import *
import traceback
from rpc.python.rpcHandler import rpcHandler
from rpc.python.rpcServer import rpcServer
from rpc.python.rpcClient import rpcClient
def getPublicKey()->list:
    return None
class ExampleEncodedNumber(phe.encoding.EncodedNumber):
    BASE = 256
    LOG2_BASE = math.log(BASE, 2)
client=rpcClient("127.0.0.1",8080)
getPublicKey=client.makeRpcCall(getPublicKey)
_pubK=getPublicKey()
publicKey=PaillierPublicKey(_pubK)
helper=convertHelper(_pubK)
def encrypt(num):
    return publicKey.encrypt(ExampleEncodedNumber.encode(publicKey, num)) 


import dataset
x,y=dataset.load_mnist('./dataset')
x=x[:,784//2:]
t=0.01
y=y//5
w=[encrypt(randint(-10,10))/10 for i in range(784//2)]
i=0
batchSize=100
batches=10
def getCipher(_ua:list):
    try:
        global i,w
        ua=[helper.ObjectToPaillier(item) for item in _ua]
        print("batch:%d"%(i+1))
        ub=[]
        for j in range(batchSize):
            result=encrypt(0)
            index=i*batchSize+j
            for k in range(784//2):
                result+=w[k]*x[index][k]
            ub.append(result)
        u=[x+y for x,y in zip(ua,ub)]
        d=[u[j]/4-y[i*batchSize+j]/2 for j in range(batchSize)]
        res=[encrypt(0) for i in range(784//2)]
        for j in range(batchSize):
            index=i*batchSize+j
            for k in range(784//2):
                res[k]+=d[j]*x[index][k]
        for k in range(784//2):
            res[k]=res[k]/batchSize
            w[k]-=res[k]*t
        i+=1

    except:
        print("exception")
        traceback.print_exc()
    return [helper.paillierToObject(item) for item in d]

handler=rpcHandler()
handler.addRpcHandler("getCipher",getCipher)
server=rpcServer(handler,8081)
server.startForever()