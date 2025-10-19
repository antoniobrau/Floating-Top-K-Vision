
/* Fast Vector è una struttura dati che ha gli stessi metodi di vector, con ddei metodi aggiuntivi per
eliminare in O(1) degli elementi sacrificandone l'ordine.
*/
#pragma once
template <typename T>
struct FastVector : public std::vector<T> {
    // Funzione per rimuovere un elemento dato un iteratore
    void fastErase(typename std::vector<T>::iterator &it) {
        if (it == this->end()) return;
        if (this->end() == std::next(it)){
            this->pop_back();
            it = this->end();
            return;
        }
        
        // Scambia l'elemento puntato dall'iteratore con l'ultimo
        std::iter_swap(it, this->rbegin());
        // Rimuovi l'ultimo elemento
        this->pop_back();
    
    }
};

class ReferenceHandler{
    private:
        std::queue<size_t> free_index;
        std::vector<std::pair<uint64_t, int>> buffer;
        CircularMap<int> reference;
    public:
        void delete_element_byindex(size_t index){
            if (--buffer[index].second == 0){
                free_index.push(index);
                reference.erase(buffer[index].first);
            }
        }
        // L'INSERIMENTO DEGLI ELEMENTI è RESO SICURO DAL MAIN CHE METTE UN LOCK SULLA PARTE DI CODICE CHE INSERISCE GLI ELEMENTI
        size_t insert_element(const uint64_t &box){
            auto find = reference.find(box);
            if (find == reference.end()){
                if (free_index.size() > 0){
                    size_t index = free_index.front();
                    free_index.pop();
                    buffer[index] = std::make_pair(box,1);
                    reference[box] = int(index);
                    return index;
                }
                else {
                    buffer.push_back(std::make_pair(box,1));
                    reference[box] = int(buffer.size()) - 1;
                    return int(buffer.size()) - 1;
                }
            }
            size_t index = find->second;
            buffer[index].second++;
            return index;
        }
        size_t size(){
            return buffer.size();
        }
        size_t get_index(const uint64_t &box){
            return reference[box];
        }

        uint64_t get_element(size_t index){
            return buffer[index].first;
        }



};

