import phe.encoding
from phe import paillier
from phe import *

class convertHelper:
    def __init__(self,n) -> None:
        self.key=PaillierPublicKey(n)
    def paillierToObject(self,num:EncryptedNumber):
        return [num.ciphertext(),num.exponent]
    def ObjectToPaillier(self,obj:list):
        return EncryptedNumber(self.key,obj[0],obj[1])