#include <experimental/filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "utils.hpp"
#include "NvInfer.h"
#include "trt_logger.hpp"


using namespace std;

bool fileExists(const string fileName) {
    if (!experimental::filesystem::exists(
            experimental::filesystem::path(fileName))){
        return false;
    }else{
        return true;
    }
}

bool fileRead(const string &path, vector<unsigned char> &data, size_t &size){
    stringstream trtModelStream;
    ifstream cache(path);

    /* 将engine的内容写入trtModelStream中*/
    trtModelStream.seekg(0, trtModelStream.beg);
    trtModelStream << cache.rdbuf();
    cache.close();

    /* 计算model的大小*/
    trtModelStream.seekg(0, ios::end);
    size = trtModelStream.tellg();

    vector<uint8_t> tmp;
    trtModelStream.seekg(0, ios::beg);
    tmp.resize(size);

    /* 将trtModelStream中的stream通过read函数写入modelMem中*/
    trtModelStream.read((char*)data[0], size);
    return true;
}

vector<unsigned char> loadFile(const string &file){
    ifstream in(file, ios::in | ios::binary);
    if (!in.is_open())
        return {};

    in.seekg(0, ios::end);
    size_t length = in.tellg();

    vector<uint8_t> data;
    if (length > 0){
        in.seekg(0, ios::beg);
        data.resize(length);
        in.read((char*)&data[0], length);
    }
    in.close();
    return data;
}

string printDims(const nvinfer1::Dims dims){
    int n = 0;
    char buff[100];
    string result;

    n += snprintf(buff + n, sizeof(buff) - n, "[ ");
    for (int i = 0; i < dims.nbDims; i++){
        n += snprintf(buff + n, sizeof(buff) - n, "%d", dims.d[i]);
        if (i != dims.nbDims - 1) {
            n += snprintf(buff + n, sizeof(buff) - n, ", ");
        }
    }
    n += snprintf(buff + n, sizeof(buff) - n, " ]");
    result = buff;
    return result;
}

string printTensor(float* tensor, int size){
    int n = 0;
    char buff[100];
    string result;
    n += snprintf(buff + n, sizeof(buff) - n, "[ ");
    for (int i = 0; i < size; i++){
        n += snprintf(buff + n, sizeof(buff) - n, "%8.4lf", tensor[i]);
        if (i != size - 1){
            n += snprintf(buff + n, sizeof(buff) - n, ", ");
        }
    }
    n += snprintf(buff + n, sizeof(buff) - n, " ]");
    result = buff;
    return result;
}

void printTensor(float* data, int size, int chw_size, int hw_size, int w_size){
    printf("[");
    for (int i = 0; i < size; i += chw_size) {
        (i == 0) ? printf("[") : printf(" [");
        for (int j = 0; j < chw_size; j += hw_size) {
            (j == 0) ? printf("[") : printf("  [");
            for (int k = 0; k < hw_size; k += w_size) {
                (k == 0) ? printf("[") : printf("   [");
                for (int p = 0; p < w_size; p ++) {
                        printf("%.8lf", data[i + j + k + p]);

                    if (k != w_size - 1) printf(" ");
                }
                (k == hw_size - w_size) ? printf("]") : printf("]\n");
            }
            (j == chw_size - hw_size) ? printf("]") : printf("]\n\n");
        }
        (i == size - chw_size) ? printf("]") : printf("]\n\n\n");
    }
    printf("]\n");
}

void printTensorCXX(float* data, int b, int c, int h, int w){
    int size     = b * c * h * w;
    int chw_size = c * h * w;
    int hw_size  = h * w;
    int w_size   = w;
    printTensor(data, size, chw_size, hw_size, w_size);
}

void printTensorNPY(cnpy::NpyArray arr){
    float* data = arr.data<float>();
    int size = arr.num_bytes() / sizeof(float);

    int chw_size = arr.shape[1] * arr.shape[2] * arr.shape[3];
    int hw_size  = arr.shape[2] * arr.shape[3];
    int w_size   = arr.shape[3];

    printTensor(data, size, chw_size, hw_size, w_size);
}


string printTensorShape(nvinfer1::ITensor* tensor){
    string str;
    str += "[";
    auto dims = tensor->getDimensions();
    for (int j = 0; j < dims.nbDims; j++) {
        str += to_string(dims.d[j]);
        if (j != dims.nbDims - 1) {
            str += " x ";
        }
    }
    str += "]";
    return str;
}

string getEnginePath(string onnxPath){
    int name_l = onnxPath.rfind("/");
    int name_r = onnxPath.rfind(".");

    int dir_l  = 0;
    int dir_r  = onnxPath.find("/");

    string enginePath;

    enginePath = onnxPath.substr(dir_l, dir_r);
    enginePath += "/engine";
    enginePath += onnxPath.substr(name_l, name_r - name_l);
    
    enginePath += ".engine";
    return enginePath;
}

string getOutputPath(string srcPath, string postfix){
    int pos = srcPath.rfind(".");
    string tarPath;
    tarPath = srcPath.substr(0, pos);
    tarPath += "_" + postfix + ".png";
    return tarPath;
}

string getFileType(string filePath){
    int pos = filePath.rfind(".");
    string suffix;
    suffix = filePath.substr(pos, filePath.length());
    return suffix;
}

string getFileName(string filePath){
    int pos = filePath.rfind("/");
    string suffix;
    suffix = filePath.substr(pos + 1, filePath.length());
    return suffix;
}

string getPrecision(nvinfer1::DataType type) {
    switch(type) {
        case nvinfer1::DataType::kFLOAT:  return "FP32";
        case nvinfer1::DataType::kHALF:   return "FP16";
        case nvinfer1::DataType::kINT32:  return "INT32";
        case nvinfer1::DataType::kINT8:   return "INT8";
        case nvinfer1::DataType::kUINT8:  return "UINT8";
        default:                          return "unknown";
    }
}

void initTensor(float* data, int size, int min, int max, int seed) {
    srand(seed);
    for (int i = 0; i < size; i ++) {
        data[i] = float(rand()) * float(max - min) / RAND_MAX;
    }
}
