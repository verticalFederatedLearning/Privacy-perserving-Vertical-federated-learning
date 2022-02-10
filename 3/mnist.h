#ifndef MNIST_H
#define MNIST_H

#include <fstream>
#include <iostream>
#include <vector>
class mnist
{
private:
    static int reverse_int(int i)
    {
        unsigned char ch1, ch2, ch3, ch4;
        ch1 = i & 255;
        ch2 = (i >> 8) & 255;
        ch3 = (i >> 16) & 255;
        ch4 = (i >> 24) & 255;
        return ((int) ch1 << 24) + ((int) ch2 << 16) + ((int) ch3 << 8) + ch4;
    }
public:

    static std::vector<std::pair<int,std::vector<int>>> read_MNIST_data(int number_of_images, int number_of_features,std::string filename="./dataset/train-images-idx3-ubyte");
    static std::vector<int> read_MNIST_labels(int number_of_images,std::string filename="./dataset/train-labels-idx1-ubyte");
};
#endif