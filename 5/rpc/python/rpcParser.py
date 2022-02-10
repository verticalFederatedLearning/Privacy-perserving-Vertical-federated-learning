rpcMaxLength=7
class rpcMessage():
    def __init__(self,data:str) -> None:
        self.data=data
    def encode(self) -> bytes:
        length=str(len(self.data))
        length='0'*(rpcMaxLength-len(length))+length
        return (length+"\r\n"+self.data+"\r\n").encode()
def parse(data:str):
    length=0
    try:
        length=int(data[:rpcMaxLength])
        assert(data[rpcMaxLength:rpcMaxLength+2]=="\r\n")
        length2=len(data)
        assert(length2-rpcMaxLength-4<=length)
        if (length2-rpcMaxLength-4==length):
            return {"length":length,"message":data[rpcMaxLength+2:-2]}
        else:
            return {"length":length,"message":data[rpcMaxLength:]}
    except:
        raise Exception("invalid request")