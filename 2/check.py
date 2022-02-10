import os
import struct
import numpy as np
import gzip
def load_mnist(path):
 
    # trX将加载储存所有60000张灰度图
    fd = open(os.path.join(path, 'train-images-idx3-ubyte'))
    loaded = np.fromfile(file=fd, dtype=np.uint8)
    trX = loaded[16:].reshape((60000, 784)).astype(np.float)
 
    fd = open(os.path.join(path, 'train-labels-idx1-ubyte'))
    loaded = np.fromfile(file=fd, dtype=np.uint8)
    trY = loaded[8:].reshape((60000)).astype(np.float)
    return trX, trY
print("loading dataset...")
[x,y]=load_mnist("./dataset")
a=eval(open('datasetA.txt','r').read())
b=eval(open('datasetB.txt','r').read())
N=len(a)
C=[[int(i) for i in lst.tolist()]for lst in x]
c=[[a[i][j]+b[i][j]for j in range(784)]for i in range(N)]
print("checking...")
label=0
for lst in c:
    try:
    	print("found label:%d"%C.index(lst))
    	label+=1
    except:
        pass
print("found %d valid labels"%label)
