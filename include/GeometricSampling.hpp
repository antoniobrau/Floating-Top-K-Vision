#pragma once

#include <iostream>
#include <random>

class RandomGeometric {
private:
    std::mt19937 generator;                     // Generatore di numeri casuali
    std::geometric_distribution<> distribution; // Distribuzione geometrica

public:
    RandomGeometric() 
        : generator(std::random_device{}()), distribution(0.5) {}
    // Costruttore: accetta il parametro della distribuzione geometrica (probabilità p)
    explicit RandomGeometric(double p) 
        : generator(std::random_device{}()), distribution(p) {}

    // Genera un numero casuale dalla distribuzione
    int generate() {
        return distribution(generator);
    }

    // Imposta un nuovo valore per il parametro p
    void setProbability(double p) {
        distribution = std::geometric_distribution<>(p);
    }
    // Reimposta il seed del generatore
    void setSeed(unsigned int seed) {
        generator.seed(seed);
    }

};