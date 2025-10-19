#ifndef COSTANTSETTINGS_HPP
#define COSTANTSETTINGS_HPP

#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>


constexpr int SPACE_FILTER = 3;
constexpr int  TIME_FILTER = 3;
constexpr size_t DIM_MULTI_LEVEL = 1;

extern double PROMOTION_PROBABILITY = 0.999;
extern size_t MAX_ITEMS = 100000;
extern int WIDTH = 768;
extern int HEIGHT = 576;
extern int POOL_SIZE = 10;
extern size_t max_items = 100000;
extern int max_window_length = -1;
extern std::string DIRECTORY_VIDEO = "";
extern std::string DIRECTORY_OUT = "";


void load_config(const std::string& path) {
    std::ifstream f(path);
    if (!f) return;
    std::string key; double val; std::string sval;
    while (f >> key) {
        if (key ==  "DIRECTORY_VIDEO"){
            f >> sval;
            DIRECTORY_VIDEO = sval;
        }
        else if (key == "DIRECTORY_OUT"){
            f >> sval;
            DIRECTORY_OUT = sval;
        }
        else{
            f >> val;
            if (key == "WIDTH") WIDTH = static_cast<int>(val);
            else if ( key == "HEIGHT") HEIGHT = static_cast<int>(val);
            else if (key == "POOL_SIZE") POOL_SIZE = static_cast<int>(val);
            else if (key == "PROMOTION_PROBABILITY") PROMOTION_PROBABILITY = val;
            else if (key == "MAX_ITEMS") MAX_ITEMS = static_cast<size_t>(val);
            else if (key == "MAX_WINDOW_LENGTH") max_window_length = static_cast<int>(val);
        }
    }
}

struct ConfigLoader {
    ConfigLoader() { load_config("config.txt"); }
};
static ConfigLoader _auto_load;

#endif // COSTANTSETTINGS_HPP














