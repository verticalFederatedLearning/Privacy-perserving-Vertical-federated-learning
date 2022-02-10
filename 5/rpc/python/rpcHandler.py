
class rpcHandler:
    def __init__(self) -> None:
        self.rpcMap={}
    def handleRpc(self,rpcmsg):
        return self.rpcMap[rpcmsg["name"]](rpcmsg)
    def addRpcHandler(self,name:str,func):
        def rpcFunc(rpcmsg):
            argnum=func.__code__.co_argcount
            result=func.__call__(*[rpcmsg['param%d'%(argnum-i)] for i in range(argnum)])
            return result
        self.rpcMap[name]=rpcFunc