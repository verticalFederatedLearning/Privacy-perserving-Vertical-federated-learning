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
    return trX/256, trY/10
 
