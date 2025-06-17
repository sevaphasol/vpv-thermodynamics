#ifndef TYPES_H
#define TYPES_H

#include <SFML/Graphics.hpp>

typedef struct Settings {
    int particle_count;
    int mean_free_path;
    int delay;
} Settings;

typedef enum AppState {
    MENU,
    SIMULATION
} AppState;

struct Particle {
    sf::Vector2f position;
    std::vector<sf::Vertex> path;
    sf::Color color;
};

#endif // TYPES_H
