#include "mnist.h"
std::vector<std::pair<std::string,std::vector<int>>> mnist::read_MNIST_data(int number_of_images, int number_of_features,std::string filename)
{
    std::vector<std::pair<std::string,std::vector<int>>> vec;
    std::ifstream file;
	file.open(filename, std::ios::binary);
    
    if(!file){
        throw std::runtime_error("Unable to open file");
    }
    if (file.is_open()){
        int magic_number = 0;
        int n_rows = 0;
        int n_cols = 0;
        file.read((char*) &magic_number, sizeof(magic_number));
        magic_number = reverse_int(magic_number);
        int images;
        file.read((char*) &images, sizeof(images));
        file.read((char*) &n_rows, sizeof(n_rows));
        n_rows = reverse_int(n_rows);
        file.read((char*) &n_cols, sizeof(n_cols));
        n_cols = reverse_int(n_cols);
        number_of_features = n_rows * n_cols;
        std::cout << "Number of Images: " << number_of_images << std::endl;
        std::cout << "Number of Features: " << number_of_features << std::endl;
        for(int i = 0; i < number_of_images; ++i){
            std::vector<int> tp;
            for(int r = 0; r < n_rows; ++r)
                for(int c = 0; c < n_cols; ++c){
                    unsigned char temp = 0;
                    file.read((char*) &temp, sizeof(temp));
                    tp.push_back(temp);
                }
            vec.push_back(std::make_pair(std::to_string(i),tp));
        }
    }
    return vec;
}
std::vector<int> mnist::read_MNIST_labels(int number_of_images,std::string filename)
{
    std::vector<int> vec;
    std::ifstream file;
	file.open(filename, std::ios::binary);
    if(!file){
        throw std::runtime_error("Unable to open file");
    }
    if (file.is_open()){
        int magic_number = 0;
        file.read((char*) &magic_number, sizeof(magic_number));
        magic_number = reverse_int(magic_number);
        int images;
        file.read((char*) &images, sizeof(images));
		std::cout << "number of images inside MNIST LAbels: " << number_of_images << std::endl;
        for(int i = 0; i < number_of_images; ++i){
            unsigned char temp = 0;
            file.read((char*) &temp, sizeof(temp));
			//vec.push_back((unsigned long int) temp);
			
                vec.push_back( temp);
        }
    }
	file.close();
    return vec;
}