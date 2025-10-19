#pragma once

#include <vector>
#include <array>
#include <queue>
#include <functional>
#include <atomic>
#include <stdexcept> 
#include <iostream> 

#include"GlobalSettings.hpp"

class MultiLevel{
    public:
        int sum_levels = 0;
        std::array<int, DIM_MULTI_LEVEL> levels = {0};

        MultiLevel(){}

        bool aggiorna( MultiLevel &other){
            bool all_over = true;
            for ( size_t i = 0 ; i < DIM_MULTI_LEVEL; ++i){
                int diff = other.levels[i] - levels[i];
                if ( diff > 0){
                    all_over = false;
                    sum_levels += diff;
                    levels[i] = other.levels[i];
                }
            }
            return all_over;
        }
        // Operatore di confronto per ordinamento
        bool operator<(const MultiLevel& other) const {
            // Lo definisco al contrario in modo che std::sort lo ordini in modo decrescente.
            return sum_levels < other.sum_levels;
        }

};

template <typename T>
class IndexedMinHeap{
    public:
        // IL PRIMO INDICE CONTIENE IL VALORE ASSOCIATO, OSSIA LA MEDIA, MENTRE IL SECONDO L'INDICE
        std::vector<std::pair<int,int>> heap;

        // VETTORE DOVE A OGNI INDICE CORRISPONDE UN OGGETTO E UN INDICE SULLA HEAP
        std::vector<std::pair<T, int>>  indexed_map;

        // Numero massimo di elementi
        size_t max_k;
        size_t size = 0;

        IndexedMinHeap(){}
        IndexedMinHeap( size_t tot_object, size_t max_k) : max_k(max_k){
            indexed_map.resize(tot_object);
            for (size_t i = 0; i < tot_object; i++){
                indexed_map[i].second = -1;
            }
        }
        void inizializza(size_t tot_object, size_t max_k){
            indexed_map.resize(tot_object);
            this->max_k = max_k;
            for (size_t i = 0; i < tot_object; i++){
                indexed_map[i].first = MultiLevel();
                indexed_map[i].second = -1;
            }
            heap.clear();
            this->size = 0;
        }
        void heapifyDown(size_t i) {
            while (true) {
                size_t smallest = i;
                size_t left = 2 * i + 1;
                size_t right = 2 * i + 2;

                if (left < this->size && heap[left].first < heap[smallest].first)
                    smallest = left;
                if (right < this->size && heap[right].first < heap[smallest].first)
                    smallest = right;

                if (smallest == i) break; // Proprietà del min-heap soddisfatta

                std::swap(heap[i], heap[smallest]);
                std::swap(indexed_map[heap[i].second].second, indexed_map[heap[smallest].second].second);
                i = smallest;
            }
        }
        void heapifyUp(size_t i) {
            while (i > 0) {
                size_t parent = (i - 1) / 2;
                if (heap[i].first >= heap[parent].first) break;

                std::swap(heap[i], heap[parent]);
                std::swap(indexed_map[heap[i].second].second, indexed_map[heap[parent].second].second);
                i = parent;
            }
        }
        bool push(T & element, size_t index) {
            if (this->size < this->max_k) {

                // Aggiungi nuovo elemento e mantieni la proprietà del min-heap
                this->heap.push_back(std::make_pair(element.sum_levels, index));
                this->size++;
                indexed_map[index].second = this->size - 1;
                indexed_map[index].first.aggiorna(element);
                heapifyUp(this->size - 1); // Ribilancia dal basso verso l'alto
                return false;
            }
            if ( element.sum_levels > heap[0].first){
                indexed_map[index].first.aggiorna(element);

                indexed_map[heap[0].second].second = -1; // Rimuovi vecchio indice
                indexed_map[heap[0].second].first = MultiLevel();
                // Sostituisci la radice con il nuovo elemento e riequilibra
                heap[0] = std::make_pair(element.sum_levels, index);
                indexed_map[index].second = 0;
                heapifyDown(0); // Ribilancia dall'alto verso il basso
                return false;
            }
            return true;
        }

        bool aggiorna( size_t index, T & element){
            int heap_index = indexed_map[index].second;
            //std::cout<<index<<" "<<element.sum_levels<<" "<<heap_index<<std::endl;
            if ( heap_index != -1){
                bool all_over = indexed_map[index].first.aggiorna(element);
                if (all_over) return true;
                heap[heap_index].first = indexed_map[index].first.sum_levels;
                heapifyDown(heap_index);
                return false;
            }
            int sum = element.sum_levels;
            return push(element, index);
        }
};

