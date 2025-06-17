#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <string>
#include "types.h"
#include "config.h"
#include "menu.h"
#include "simulation.h"

int main() {
    srand(static_cast<unsigned int>(time(0)));
    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Random walks");
    window.setFramerateLimit(60);

    sf::Font font;
    if (!font.loadFromFile("res/DejaVuSans.ttf")) {
        fprintf(stderr, "Error while loading font\n");
        return -1;
    }

    Settings settings;
    settings.particle_count = DEFAULT_PARTICLE_COUNT;
    settings.mean_free_path = DEFAULT_STEP_SIZE;
    settings.delay          = DEFAULT_DELAY;

    AppState state = MENU;

    while (window.isOpen()) {
        if (state == MENU) {
            show_menu(window, font, settings);
            state = SIMULATION;
        } else if (state == SIMULATION) {
            run_simulation(window, font, settings);
            state = MENU;
        }
    }

    return 0;
}
