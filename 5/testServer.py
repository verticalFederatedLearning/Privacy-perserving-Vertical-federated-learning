from convert import convertHelper
import math
import phe.encoding
from phe import paillier
from phe import *

from rpc.python.rpcHandler import rpcHandler
from rpc.python.rpcServer import rpcServer
class ExampleEncodedNumber(phe.encoding.EncodedNumber):
    BASE = 64
    LOG2_BASE = math.log(BASE, 2)
public_key, private_key = paillier.generate_paillier_keypair()
a=3
b=4
encoded_a = ExampleEncodedNumber.encode(public_key, a)
encoded_b = ExampleEncodedNumber.encode(public_key, b)

encrypted_a = public_key.encrypt(encoded_a)
encrypted_b = public_key.encrypt(encoded_b)


helper=convertHelper(public_key.n)
encrypta=helper.ObjectToPaillier(helper.paillierToObject(encrypted_a))
print(private_key.decrypt(encrypta/4))