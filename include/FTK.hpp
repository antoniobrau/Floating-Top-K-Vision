#pragma once
#include <iostream>
#include "IndexedMinHeap.hpp"
#include <random>
#include <chrono>

#include<unordered_map>


#include"ThreadPool.hpp"
#include"GlobalSettings.hpp"
#include"DataHandler.hpp"
#include"GeometricSampling.hpp"
#include"fast_vector.hpp"

#include"xxhash64.h"

#include <numeric>
#include<atomic>
#include<mutex>
#include<deque>


class FTK{
    private:

        RandomGeometric random_generator;
        DataHandler & handler;
        ThreadPool &pool;
        std::condition_variable cv;
        std::mutex mtx;
        std::atomic<int> task_mancanti;
        std::vector<size_t> topc_sum_level;

    public:
        std::vector<std::pair<uint64_t, int>> occorrenze;
        int DimensioneFinestra;
        int time = 0;
        size_t k;
        size_t total_iterations;
        CircularMap<int> Reference;
        std::vector<std::deque<FastVector<std::pair<MultiLevel,int>>>> circular_topc;
        std::vector<IndexedMinHeap<MultiLevel>> topc;
        std::vector<ReferenceHandler> referenceHandler;

        FTK(int DimensioneFinestra, DataHandler& handler, ThreadPool& pool)
            : handler(handler), pool(pool), task_mancanti(0), DimensioneFinestra(DimensioneFinestra), k(MAX_ITEMS){
            random_generator = RandomGeometric(1 - PROMOTION_PROBABILITY);
            total_iterations = handler.total_iterations;
            topc.resize(POOL_SIZE);
            circular_topc.resize(POOL_SIZE);
            referenceHandler.resize(POOL_SIZE);
        }

        FTK(const FTK&) = delete;
        FTK& operator=(const FTK&) = delete;
        FTK(FTK&&) = delete;
        FTK& operator=(FTK&&) = delete;



        bool time_update(){
            if ( handler.add_frame() == false ) return false;
            {
            time++;
            std::unique_lock<std::mutex> lock(mtx);
            task_mancanti.store(POOL_SIZE);

            for (int task = 0; task < POOL_SIZE; task++){
                std::mutex mtx;
                pool.enqueue([this, task, &mtx]{
                    auto &buffer_binari = handler.vec;

                    CircularMap<MultiLevel> current_topc_task;

                    for (int idx = task; idx < total_iterations; idx += POOL_SIZE){
                        auto & box = buffer_binari[idx];

                        MultiLevel levels;
                        int sum = 0;
                        for ( size_t _i = 0; _i < DIM_MULTI_LEVEL; ++_i){
                            int value = 1 + random_generator.generate();
                            levels.levels[_i] = value;
                            levels.sum_levels += value;
                        }

                        current_topc_task[box].aggiorna(levels);
                        
                    }
                    auto &topc_task = topc[task];
                    auto &referenceHandler_task = referenceHandler[task];
                    auto &circular_topc_task = circular_topc[task];

                    circular_topc_task.push_back({});
                    auto & last = circular_topc_task.back();

                    
                    for (auto it = current_topc_task.begin(); it != current_topc_task.end(); it++){
                        size_t index = referenceHandler_task.insert_element(it->first);
                        last.push_back(std::make_pair(it->second, index));
                    }
                    std::sort(last.begin(), last.end(), std::greater<std::pair<MultiLevel,int>>());
                    
                    if (last.size() > k) {
                        auto size = last.size();
                        for (size_t ind = k; ind < size; ind++) referenceHandler_task.delete_element_byindex(last[ind].second);
                        last.resize(k);
                    }
                    topc_task.inizializza(referenceHandler_task.size(), k);
                    for ( int _ind = last.size() - 1; _ind >= 0; _ind--){
                        topc_task.aggiorna(last[_ind].second,last[_ind].first);
                    }
                    
                    if ( DimensioneFinestra != -1 & circular_topc_task.size() > DimensioneFinestra ) {
                        auto &front = circular_topc_task.front();
                        size_t size = front.size();
                        for ( size_t ind = 0; ind < size; ++ind){
                            referenceHandler_task.delete_element_byindex(front[ind].second);
                        }
                        circular_topc_task.pop_front();
                    }
                    auto begin = circular_topc_task.rbegin();
                    if (begin != circular_topc_task.rend()){
                        begin = std::next(begin);
                        auto circular_end = circular_topc_task.rend();
                        for (auto it = begin; it != circular_end; it++){
                            auto it2 = it->begin();
                            auto end = it->end();
                            while ( it2 != end ){
                                bool all_over = topc_task.aggiorna(it2->second,it2->first);
                                if (all_over){
                                    referenceHandler_task.delete_element_byindex(it2->second);
                                    it->fastErase(it2);
                                    end = it->end();
                                }
                                else {
                                    ++it2;
                                }
                            }
                            it->shrink_to_fit();
                        }
                    }

                    // Rimuovi i vector vuoti dalla deque
                    circular_topc_task.erase(
                        std::remove_if(circular_topc_task.begin(), circular_topc_task.end(),
                                    [](const FastVector<std::pair<MultiLevel, int>>& v) { return v.empty(); }),
                        circular_topc_task.end());

                    
                    task_mancanti.fetch_sub(1, std::memory_order_relaxed);
                    if (task_mancanti.load() == 0) {
                        cv.notify_all();

                    }
            });
        }
        cv.wait(lock, [&] { return task_mancanti.load() == 0; });
        }
        return true;
    }
    CircularMap<std::pair<int,int>> get_topc(){
        CircularMap<std::pair<int,int>> map;
        for ( size_t task = 0; task < POOL_SIZE; task++){
            auto &heap = topc[task].heap;
            size_t size = heap.size();
            for ( size_t i = 0; i < size; ++i){
                int sum = heap[i].first;
                size_t index = heap[i].second;
                auto box = referenceHandler[task].get_element(index);
                //std::cout<<index<<" "<<box.to_string()<<" "<<topc[task].indexed_map[index].second<<std::endl;
                std::pair<int,int> &pair = map[box];
                pair.first += sum;
                pair.second += DIM_MULTI_LEVEL;
            }
        }
        return map;
    }
    
};



void saveOccorrenzeToCSV(FTK &ftk, const std::string& filename, float iterations) {
    auto occorrenze = ftk.get_topc();
    // Scrittura del file in UTF-8
    std::ofstream file(filename, std::ios::out | std::ios::binary);
    // file.imbue(std::locale(std::locale::classic(), new std::codecvt_utf8<char>));

    file<<"# Total_Iterations; SpaceFilter; TimeFilter; POOL_SIZE; DIM_MULTI_LEVEL\n";
    int time = ftk.time;
    if (time > ftk.DimensioneFinestra & ftk.DimensioneFinestra != -1) time = ftk.DimensioneFinestra;
    file<<"#"<<iterations<<";"<<SPACE_FILTER<<";"<<TIME_FILTER<<";"<<POOL_SIZE<<";"<<DIM_MULTI_LEVEL<<";\n";

    file<<"Pattern;";
    file<<"Sum_level;";
    file<<"Count_level\n";

    for (const auto& pair : occorrenze) {
        file << pair.first << ";" << pair.second.first << ";" << pair.second.second << "\n";
    }

    file.close();
}

