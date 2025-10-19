#include <iostream>
#include "IndexedMinHeap.hpp"
#include <random>
#include <chrono>
#include <filesystem>
#include <vector>
#include <string>
#include<unordered_map>


#include"ThreadPool.hpp"
#include"GlobalSettings.hpp"
#include"DataHandler.hpp"
#include"GeometricSampling.hpp"
#include"fast_vector.hpp"
#include"FTK.hpp"

#include"xxhash64.h"

#include <numeric>
#include<atomic>
#include<mutex>
#include<deque>


namespace fs = std::filesystem;

std::vector<std::string> getFilesInDirectory(const std::string& directoryPath) {
    std::vector<std::string> files;

    try {
        for (const auto& entry : fs::directory_iterator(directoryPath)) {
            if (fs::is_regular_file(entry.status())) { // Verifica che sia un file regolare
                files.push_back(entry.path().string()); // Aggiunge il percorso completo
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Errore durante l'accesso alla directory: " << e.what() << std::endl;
    }

    return files;
}
int main() {
    std::string videoPath = DIRECTORY_VIDEO;

    std::string path_occorrenze = DIRECTORY_OUT;

    std::vector<std::string> vettore_file = getFilesInDirectory(videoPath);
    std::vector<std::string> vettore_out(vettore_file.size());

    for( size_t n_file = 0; n_file < vettore_file.size(); n_file++){
        std::string s = vettore_file[n_file];
        for (size_t i = videoPath.size(); i < s.size()-4; i++ ) {
            vettore_out[n_file] += s[i];
        }
    }


    ThreadPool pool(POOL_SIZE);
    int w = max_window_length;
    DataHandler handler= DataHandler(vettore_file[0], pool);
    FTK ftk = FTK(w, handler, pool);
    
    //ftk.random_generator.setSeed(0);

    float sum_iterations = 0;

    auto time_in = std::chrono::high_resolution_clock::now();

    for( size_t n_file = 0; n_file < 10; n_file++){
    // for( size_t n_file = 0; n_file < vettore_file.size(); n_file++){
    //     std::cout << "--------------------" << std::endl;
        if ( n_file >= vettore_file.size()) break;
        std::cout << "Elaborazione del file: " <<n_file<< vettore_file[n_file] << std::endl;
        try {
            handler.new_video(vettore_file[n_file]);
            // qualcosa che può fallire
        } catch (...) {
            continue; // continua al prossimo elemento del ciclo
        }
        ftk.total_iterations = handler.total_iterations;
        size_t frameCount = 0;
        // for (int i = 0; i < TIME_FILTER; i++)   handler.add_frame();
        try {
            for (int i = 0; i < 15; i++)   handler.add_frame();
            // qualcosa che può fallire
        } catch (...) {
            continue; // continua al prossimo elemento del ciclo
        }

        bool not_end = true;
        auto file_start = std::chrono::high_resolution_clock::now();

        while ( not_end ) {
            auto frame_start = std::chrono::high_resolution_clock::now();
            not_end = ftk.time_update();

            if (not_end == false) break;
            //not_end = handler.add_frame();
            auto frame_end = std::chrono::high_resolution_clock::now();

            if (frameCount == 30){
                ++frameCount;
                break;
            }
            if (frameCount%10 ==0) std::cout << "Frame " << frameCount 
                      << " elaborato in :" 
                      << std::chrono::duration_cast<std::chrono::milliseconds>(frame_end - frame_start).count() 
                      << " ms" << std::endl;

            frameCount++;
        }
        std::cout<<(frameCount)<<" "<<ftk.total_iterations<<std::endl;
        sum_iterations += (frameCount)*ftk.total_iterations;
        auto file_end = std::chrono::high_resolution_clock::now();
        std::cout << "Tempo totale per il file " << vettore_file[n_file] << ": " 
                  << std::chrono::duration_cast<std::chrono::milliseconds>(file_end - file_start).count() 
                  << " ms" << std::endl;


    }
    ftk.total_iterations = sum_iterations;
    ftk.time = 1;

    auto time_end = std::chrono::high_resolution_clock::now();
    saveOccorrenzeToCSV(ftk, path_occorrenze, sum_iterations );
    std::cout << "File salvato: " << path_occorrenze << std::endl;


    std::cout << "Tempo totale per la simulazione " 
    << std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_in).count() 
    << " ms" << std::endl;

    return 0;
}
//500 pattern 0.05 W