#pragma once
#include <fstream>
#include <string>

constexpr int SPACE_FILTER = 3;
constexpr int TIME_FILTER  = 3;
constexpr size_t DIM_MULTI_LEVEL = 1;

// variabili globali (una sola definizione grazie a 'inline')
inline double PROMOTION_PROBABILITY = 0.999;
inline size_t MAX_ITEMS = 100000;
inline int WIDTH = 768;
inline int HEIGHT = 576;
inline int POOL_SIZE = 10;
inline int max_window_length = -1;
inline std::string DIRECTORY_VIDEO{};
inline std::string DIRECTORY_OUT{};

// funzione in header: deve essere inline
inline void load_config(const std::string& path) {
    std::ifstream f(path);
    if (!f) return;
    std::string key;
    while (f >> key) {
        if (key == "DIRECTORY_VIDEO")      f >> DIRECTORY_VIDEO;
        else if (key == "DIRECTORY_OUT")   f >> DIRECTORY_OUT;
        else if (key == "WIDTH")           f >> WIDTH;
        else if (key == "HEIGHT")          f >> HEIGHT;
        else if (key == "POOL_SIZE")       f >> POOL_SIZE;
        else if (key == "PROMOTION_PROBABILITY") f >> PROMOTION_PROBABILITY;
        else if (key == "MAX_ITEMS")       f >> MAX_ITEMS;
        else if (key == "MAX_WINDOW_LENGTH") f >> max_window_length;
        else { std::string dump; f >> dump; } // ignora chiave sconosciuta
    }
}

// una sola istanza globale che auto-carica la config (grazie a 'inline')
struct ConfigLoader {
    ConfigLoader() { load_config("config.txt"); }
};
inline ConfigLoader _auto_load;








