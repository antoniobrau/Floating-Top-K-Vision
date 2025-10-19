#pragma once

#include <iostream>             
#include <vector>               
#include <string>               
#include <atomic>               
#include <mutex>                 
#include <condition_variable>   
#include <chrono>               
#include <opencv2/opencv.hpp>  
#include <thread>
#include <queue>
#include <functional>
#include <atomic>
#include <unordered_map>
#include <stdexcept> 
#include <iostream>   


#include "xxhash64.h"

#include "ThreadPool.hpp" 
#include"CircularVector.hpp"



template <typename Value>
using CircularMap = std::unordered_map<uint64_t, Value>;


void serializeMat(const cv::Mat& mat, uint64_t & pattern) {
    // Verifica che la matrice abbia il formato corretto
    // if (mat.rows != SPACE_FILTER || mat.cols != SPACE_FILTER || mat.type() != CV_8UC1) {
    //     std::cout<<"ciaooo";
    //     throw std::invalid_argument("Matrice di dimensioni o tipo non valido");
    // }

    int idx = 0;

    // Copia i valori della matrice nell'array lineare
    for (int i = 0; i < SPACE_FILTER; ++i) {
        const uint8_t* rowPtr = mat.ptr<uint8_t>(i); // Ottieni il puntatore alla riga
        for (int j = 0; j < SPACE_FILTER; ++j) {
            //std::cout << (rowPtr[j] > 0 ? "1 " : "0 ");
            pattern = ((pattern << 1) | (rowPtr[j] > 0 ? 1 : 0));

        }
    }

    //std::cin.get(); // Attende un input dall'utente prima di procedere

    // std::cout << "[";
    // for (int i = 0; i < mat.rows; ++i) {
    //     const uint8_t* rowPtr = mat.ptr<uint8_t>(i);
    //     for (int j = 0; j < 100; ++j) {
    //         std::cout << static_cast<int>(rowPtr[j]) << " "; // Stampa i valori come interi
    //     }
    // }
    // std::cout << "]\n";
    // std::cin.get();

}

std::mutex coutMutex; 

class DataHandler{
    std::condition_variable frame_completato_cv;
    std::condition_variable next_file_cv;

    cv::Mat frame;

    int currentTime = 0;
    size_t frame_count;
    int n_task;
    uint64_t bit_mask = (1 << (SPACE_FILTER*SPACE_FILTER*TIME_FILTER)) - 1;
    int len_precount_vector;

    size_t height;
    size_t width;

    ThreadPool &pool;
    cv::VideoCapture cap;
public:
    size_t total_iterations;
    std::vector<CircularVector<uint8_t, SPACE_FILTER, TIME_FILTER>> buffer_binari;
    std::vector<uint64_t> vec;

    DataHandler(const std::string& pathVideo, ThreadPool& pool) 
        : pool(pool) {

        OpenVideo(pathVideo);

        total_iterations = (width - SPACE_FILTER + 1) * (height - SPACE_FILTER + 1);

        buffer_binari.resize(total_iterations);
        vec.resize(total_iterations,0);

        n_task = int(pool.get_nthread());
        if (n_task > 10)   n_task = 10;
    }
    
    void new_video(const std::string& pathVideo){
        OpenVideo(pathVideo);

        total_iterations = (width - SPACE_FILTER + 1) * (height - SPACE_FILTER + 1);

        buffer_binari.resize(total_iterations);
        vec.resize(total_iterations,0);
        currentTime = 0;
    }
    void close_video(){
        cap.release();
    }
    bool add_frame() {
        if (cap.read(frame)) {
            if (frame.channels() == 3) { // Se ha 3 canali, convertilo in scala di grigi
                cv::cvtColor(frame, frame, cv::COLOR_BGR2GRAY);
            }
            std::atomic<int> pendingTasks_frame_atomic;
            pendingTasks_frame_atomic.store(0);
            std::mutex mtx;
            std::unique_lock<std::mutex> lock(mtx);

            for (int task = 0; task < n_task; task++){
                int time = currentTime;
                pendingTasks_frame_atomic.fetch_add(1, std::memory_order_relaxed);
                pool.enqueue([this, &pendingTasks_frame_atomic, task, time] {

                    for ( int idx = task; idx < total_iterations; idx += n_task){
                        int i = idx / (height - SPACE_FILTER + 1);
                        int j = idx % (height - SPACE_FILTER+ 1);

                        cv::Mat mat = frame(
                        cv::Range(j, j + SPACE_FILTER), 
                        cv::Range(i, i + SPACE_FILTER));

                        uint64_t &pattern = vec[idx];


                        serializeMat(mat, pattern);
                        pattern = pattern &  bit_mask;

                    }
                    pendingTasks_frame_atomic.fetch_sub(1, std::memory_order_relaxed);
                    if (pendingTasks_frame_atomic.load() == 0) {
                        //Notifica al thread principale che tutti i task sono finiti
                        frame_completato_cv.notify_all();
                    }
                });
            }
            frame_completato_cv.wait(lock, [&] { return pendingTasks_frame_atomic.load() == 0; });
            currentTime++;
            return true;
        } 
    else return false;
    }


    void OpenVideo(const std::string& pathVideo) {
    if (!cap.open(pathVideo)) {
        throw std::runtime_error("Unable to open video: " + pathVideo);
    }

    frame_count = static_cast<size_t>(cap.get(cv::CAP_PROP_FRAME_COUNT));
    width = static_cast<size_t>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
    height = static_cast<size_t>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));

    int format = static_cast<int>(cap.get(cv::CAP_PROP_FORMAT));
        // Leggi il primo frame per verificare i canali
    cv::Mat firstFrame;
    if (!cap.read(firstFrame)) {
        throw std::runtime_error("Unable to read the first frame of the video.");
    }

    // Controlla il numero di canali
    int channels = firstFrame.channels();
    if (channels == 3) {
        std::cout<<"Il video è a colori (3 canali: BGR)." << std::endl;
    } else if (channels == 1) {
        std::cout << "Video in scala di grigi rilevato." << std::endl;
    } else {
        throw std::runtime_error("Formato video sconosciuto o non supportato.");
    }

    // Riposiziona il video all'inizio
    cap.set(cv::CAP_PROP_POS_FRAMES, 0);
}

    ~DataHandler() {
        cap.release();
    }

};