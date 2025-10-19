#pragma once

#include <vector>
#include <thread>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <codecvt>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <unordered_map>
#include <string>
#include <atomic>
#include <shared_mutex>
#include <fstream>
#include <locale>


/**
 * @brief Pool di thread semplice con coda FIFO di task.
 *
 * Modello: producer-consumer. I thread restano in attesa su una condition
 * e prelevano task dalla coda in ordine di arrivo.
 *
 * @invariants
 * - I worker eseguono al più un task per volta.
 * - La coda è protetta da mutex.
 * - Il distruttore attende la fine dei worker dopo aver bloccato nuove esecuzioni.
 *
 * @thread_safety
 * - enqueue() è thread-safe.
 * - get_nthread() è thread-safe (sola lettura).
 *
 * @limitations
 * - Nessun ritorno di valore: i task non forniscono future/promessa.
 * - Coda non limitata: rischio di crescita non controllata.
 * - stop è valido solo alla distruzione (niente stop “morbido” separato).
 */
class ThreadPool {
    std::vector<std::thread> workers;        // Thread fissi
    std::queue<std::function<void()>> tasks; // Coda dei compiti
    std::mutex queueMutex;
    std::condition_variable condition;
    bool stop = false; // Flag per terminare il pool

public:
    // Costruttore: inizializza i thread
    explicit ThreadPool(size_t threads) {
        for (size_t i = 0; i < threads; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queueMutex);
                        condition.wait(lock, [this] { return stop || !tasks.empty(); });
                        if (stop && tasks.empty()) return; // Esci se il pool è terminato
                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    task(); // Esegui il task
                }
            });
        }
    }
    size_t get_nthread(){
        return workers.size();
    }
    // Aggiungi un compito alla coda
    template <typename F>
    void enqueue(F&& task) {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            tasks.emplace(std::forward<F>(task));
        }
        condition.notify_one(); // Notifica un thread
    }

    // Distruttore: termina tutti i thread
    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread& worker : workers) {
            worker.join();
        }
    }
};
