#pragma once

#include <vector>
#include <string>
#include <sstream>
#include <cstring>
#include <cstddef>
#include "xxhash64.h"
#include"ThreadPool.hpp"
#include"GlobalSettings.hpp"

#include <unordered_map>
#include <array>
#include <shared_mutex>
#include <mutex>

template <typename T, size_t S, size_t t>
struct CircularVector {
    std::vector<T> data;  // Array di dimensioni fisse
    int S2 = S*S;
    int TIME_FILTER = t;
    int front_index = 0;
    int len = S*S*t;

    CircularVector() : data(S*S*t) {} 

    // Avanza l'indice in avanti (circolare)

    // Avanza l'indice indietro (circolare)
    void push_back_index() {
        front_index -= S2;
        if (front_index < 0) front_index += len;
    }

    T* front_get(){
        return &data[front_index];
    }

    // Ottieni una riferenza all'elemento "back"
    T* back_get() {
        int index = front_index - S2;
        if (index < 0) index = index + len;
        return &data[index];
    }
    T* get(int index){
        index = front_index + index * S2;
        if (index >= len) index = index - len;
        return &data[index];
    }
    // Confronta due CircularVector usando memcmp
    std::string to_string() const {
        std::ostringstream oss;
        oss << "[";
        const T* ptr = this->data.data() + this->front_index;
        for (size_t i = 0; i < len - front_index; ++i) {
            if (i > 0) oss << ", ";
            oss << ((ptr[i] == 0) ? "0" : "1");
        }
        ptr = data.data();
        for (size_t i = 0; i < front_index; ++i) {
            oss << ", ";
            oss << ((ptr[i] == 0) ? "0" : "1");
        }

        oss << "]";
        return oss.str();
    }
    bool compare(const CircularVector<T, S, t>& other) const {
        if (this->len != other.len) {
            return false; // I vettori devono avere la stessa lunghezza
        }

        int N = this->len;
        int diff;
        int max ;
        const T *ptr1 ;
        const T *ptr2 ;
        const T *begin1;
        const T *begin2;

        // pt1 ha il front index più alto
        if (this->front_index > other.front_index){
            diff = (this->front_index - other.front_index);
            max = this->front_index;
            ptr1 =  this->data.data() + this->front_index ;
            begin1 = this->data.data();
            ptr2 = other.data.data() + other.front_index ;
            begin2 = other.data.data();
        }
        else{
            diff = -(this->front_index - other.front_index);
            max = other.front_index;
            ptr2 =  this->data.data() + this->front_index ;
            begin2 = this->data.data();
            ptr1 = other.data.data() + other.front_index ;
            begin1 = other.data.data();
        }

        int part = N - max;

        // for (int i = 0; i< part; i++) std::cout<<ptr1[i];
        // std::cout<<std::endl;
        // for (int i = 0; i< part; i++) std::cout<<ptr2[i];
        // std::cout<<std::endl;

        if (std::memcmp(ptr1, ptr2, part) != 0 ) return false;

        ptr1 = begin1;
        ptr2 += part;

        part = diff;

        // for (int i = 0; i< part; i++) std::cout<<ptr1[i];
        // std::cout<<std::endl;
        // for (int i = 0; i< part; i++) std::cout<<ptr2[i];
        // std::cout<<std::endl;

        if (std::memcmp(ptr1, ptr2, part) != 0 ) return false;


        ptr1 += sizeof(T)*part;
        ptr2 = begin2;
        part = sizeof(T)*(max - diff);

        if (std::memcmp(ptr1, ptr2, part) != 0) return false;

        return true;
    }
    uint64_t hash() const{
        XXHash64 myhash(0);

        // Aggiungi i dati dalla posizione front_index fino alla fine del vettore
        myhash.add(data.data() + front_index, (len - front_index) * sizeof(T));

        // Aggiungi i dati dall'inizio del vettore fino alla posizione front_index
        myhash.add(data.data(), front_index * sizeof(T));

        return myhash.hash();
    }
};

// Comparatore personalizzato per CircularVector
template <typename T, size_t S, size_t t>
struct CircularVector_Equal {
    bool operator()(const CircularVector<T, S, t>& lhs, const CircularVector<T, S, t>& rhs) const {
        return lhs.compare(rhs);  // Usa il metodo compare definito nella struttura
    }
};

template <typename T, size_t S, size_t t>
struct CircularVector_Hash {
    uint64_t operator()(const CircularVector<T, S, t>& cv) const {
        XXHash64 myhash(0);

        // Aggiungi i dati dalla posizione front_index fino alla fine del vettore
        myhash.add(cv.data.data() + cv.front_index, (cv.len - cv.front_index) * sizeof(T));

        // Aggiungi i dati dall'inizio del vettore fino alla posizione front_index
        myhash.add(cv.data.data(), cv.front_index * sizeof(T));

        return myhash.hash();
    }
};


class SynchronizedArray {
public:
    std::vector<int> data;  
    int treshold;
    // Costruttore che inizializza l'array con `size` elementi
    SynchronizedArray(size_t size, int treshold) 
        : data(size, 0), mutexes(size), treshold(treshold), size(size) {}

    void initialize(size_t from_index, int to_index){
        std::fill(data.begin() + from_index, data.begin() + to_index,0);
    }
    // Funzione per leggere un elemento all'indice `index`
    void add(size_t value) {
        size_t index = value % size;
        if (data[index] < treshold){
            std::lock_guard<std::mutex> lock(mutexes[index]); 
            ++data[index];
        }
    }
    bool is_over_treshold(size_t value){
        return data[value % size] >= treshold;
    }

    void parallel_initialize(ThreadPool & pool, size_t n_task) {
        std::mutex mtx;
        std::unique_lock<std::mutex> lock(mtx);
        std::condition_variable cv;
        std::atomic<int> task_mancanti = int(n_task);
        size_t chunk_size = data.size() / n_task;
        std::vector<std::thread> threads;

        for (size_t t = 0; t < n_task; ++t) {
            size_t start = t * chunk_size;
            size_t end = (t == n_task - 1) ? data.size() : start + chunk_size;

            pool.enqueue([&,this, start, end]{
                std::fill(data.begin() + start, data.begin() + end, 0);
                task_mancanti.fetch_sub(1, std::memory_order_relaxed);
                if (task_mancanti.load() == 0) {
                    cv.notify_all();
                }
            });
        }

        cv.wait(lock, [&] { return task_mancanti.load() == 0; });

    }
private:
     // Array di interi
    size_t size;
    std::vector<std::mutex> mutexes;  // Array di mutex
};

// template <typename Value>
// using CircularMap = std::unordered_map<
//     CircularVector<uint8_t, SPACE_FILTER, TIME_FILTER>, 
//     Value, 
//     CircularVector_Hash<uint8_t, SPACE_FILTER, TIME_FILTER>, 
//     CircularVector_Equal<uint8_t, SPACE_FILTER, TIME_FILTER>>;

template <typename Value>
using CircularMap = std::unordered_map<uint64_t, Value>;
