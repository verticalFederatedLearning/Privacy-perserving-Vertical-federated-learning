from convert import convertHelper
import phe.encoding
from phe import paillier
from phe import *

from rpc.python.rpcHandler import rpcHandler
from rpc.python.rpcServer import rpcServer
publicKey, privateKey = paillier.generate_paillier_keypair()

helper=convertHelper(publicKey.n)

def getPublicKey():
    return publicKey.n
handler=rpcHandler()
handler.addRpcHandler("getPublicKey",getPublicKey)
server=rpcServer(handler,8080)
server.startForever()