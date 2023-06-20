#ifndef __TRT_MODEL_HPP__
#define __TRT_MODEL_HPP__

#include <memory>
#include <vector>
#include <string>
#include "NvInfer.h"
#include "logger.hpp"

#define WORKSPACESIZE 1<<28
/*
    // 我们希望根据onnx目录进行寻找engine文件。如果有就直接load，如果没有就直接build。并且根据不同的模型类型指定不同的build方式
    classifier = model("resnet18.onnx", Model::CLASSIFICATION);
    detector   = model("yolov8.onnx", Model::DETECTION);
    segmenter  = model("unet.onnx", Model::SEGMENTATION);

    // 三种不同类型的model在获取image的时候操作是一样的，都是分配pinned memory，分配device memory这些
    classifier.load_image(...);
    detector.load_image(...);
    segmenter.load_image(...);
    // 这里需要注意的是不同的task的后处理是不一样的。但仍然是有可以复用的部分，比如说enqueue这些
    claffier.infer_classifier(...);
    detector.infer_detector(...);
    segmenter.infer_segmentor(...);
    
    // 我们总结一下Model可以复用的部分
    1. model初始化的build engine和load engine部分
    2. model读取图片并分配内存的部分
    3. model进行推理是创建binding，enqueue这些部分

    //其余的地方需要根据不同的task进行不同的优化设计
*/


class Model {

public:
    enum task_type {
        CLASSIFICATION,
        DETECTION,
        SEGMENTATION,
    };

    enum device {
        CPU,
        GPU
    };

    typedef struct Params {
        int width;
        int heigth;
        int channel;
        int num_classes;
        device dev;
        Params(int w, int h, int c, int num, device dev) : width(w), heigth(h), channel(c), num_classes(num) , dev(dev){};
    };

public:
    Model(std::string onnx_path, Logger::Level level, Params params);
    virtual ~Model() {};
    void init_model();
    bool load_image(std::string image_path);
    bool inference();

public:
    bool build_engine();
    bool load_engine();
    void save_plan(nvinfer1::IHostMemory& plan);
    void print_network(nvinfer1::INetworkDefinition &network, bool optimized);

    // 这里的dnn推理部分，只要设定好了m_bindings的话，不同的task的infer_dnn的实现都是一样的
    bool enqueue_bindings();

    // setup负责分配host/device的memory, bindings, 以及创建推理所需要的上下文。
    // 由于不同task的input/output的tensor不一样，所以这里的setup需要在子类实现
    virtual void setup(nvinfer1::IRuntime& runtime, void const* data, std::size_t size) = 0;

    // 不同的task的前处理/后处理是不一样的，所以具体的实现放在子类
    virtual bool preprocess_cpu()      = 0;
    virtual bool preprocess_gpu()      = 0;
    virtual bool postprocess_cpu()     = 0;
    virtual bool postprocess_gpu()     = 0;



public:
    
    std::string m_imagePath;
    std::string m_onnxPath;
    std::string m_enginePath;

    Logger* m_logger;
    Params* m_params;

    int m_workspaceSize;
    float* m_bindings[2];
    float* m_inputMemory[2];
    float* m_outputMemory[2];

    nvinfer1::Dims m_inputDims;
    nvinfer1::Dims m_outputDims;
    cudaStream_t m_stream;
    std::shared_ptr<nvinfer1::ICudaEngine> m_engine;
    std::shared_ptr<nvinfer1::IExecutionContext> m_context;
    std::unique_ptr<nvinfer1::INetworkDefinition> m_network;
};

#endif //__TRT_MODEL_HPP__
