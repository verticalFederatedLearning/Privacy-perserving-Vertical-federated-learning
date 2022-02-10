from rpc.python.serialization import serialization
from rpc.python.rpcHandler import rpcHandler
from rpc.python.rpcParser import parse, rpcMessage
import select
import socket
import queue
from queue import Queue
import traceback

class rpcServer:
    def __init__(self,handler:rpcHandler,port:int) -> None:
        self.port=port
        self.handler=handler
    def startForever(self) -> None:
        server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server.setblocking(False)

        server_address = ('localhost', self.port)
        server.bind(server_address)

        server.listen(5)

        message_queues = {}
        rpcRequests={}

        READ_ONLY = select.POLLIN | select.POLLPRI | select.POLLHUP | select.POLLERR
        READ_WRITE = READ_ONLY | select.POLLOUT

        TIMEOUT = 1000
        poller = select.poll() 
        poller.register(server,READ_ONLY)
        fd_to_socket = { server.fileno(): server,}

        while True:
            events = poller.poll(TIMEOUT)
            for fd, flag in events:
                s = fd_to_socket[fd]
                if flag & (select.POLLIN | select.POLLPRI):
                    if s is server:
                        connection, client_address = s.accept()
                        connection.setblocking(0)
                        fd_to_socket[connection.fileno()] = connection
                        poller.register(connection, READ_ONLY)
                        message_queues[connection] = Queue()
                    else:
                        data = s.recv(1024).decode()
                        if data:
                            req=None
                            if (s in rpcRequests.keys()):

                                if (rpcRequests[s]["length"]!=0):
                                    rpcRequests[s]["length"]-=len(data)
                                    rpcRequests[s]["message"]+=data
                                    if (rpcRequests[s]["length"]>0):
                                        continue
                                    else:                              
                                        req=rpcRequests[s]
                            else:
                                try:
                                    req=parse(data)
                                    if (req["length"]>len(req["message"])):
                                        req["length"]-=len(req["message"])
                                        rpcRequests[s]=req
                                        continue
                                except:
                                    message_queues[s].put(rpcMessage("{status:failed}"))
                                    continue
                            try:
                                result=self.handler.handleRpc(eval(req["message"]))
                                message_queues[s].put(rpcMessage(serialization(result)).encode())
                            except:
                                message_queues[s].put(rpcMessage("{status:failed}").encode())
                            #message_queues[s].put(data)
                            poller.modify(s, READ_WRITE)
                        else:
                            poller.unregister(s)
                            s.close()
                            del message_queues[s]

                elif flag & select.POLLHUP:
                    poller.unregister(s)
                    s.close()

                elif flag & select.POLLOUT:
                    try:
                        next_msg = message_queues[s].get_nowait()
                    except queue.Empty:
                        poller.modify(s, READ_ONLY)
                    else:
                        s.send(next_msg)
                        if (s in rpcRequests.keys()):
                            del rpcRequests[s]
                elif flag & select.POLLERR:
                    poller.unregister(s)
                    s.close()
                    del message_queues[s]
