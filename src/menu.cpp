#include "menu.h"
#include "config.h"
#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <string>

void show_menu(sf::RenderWindow& window, sf::Font& font, Settings& settings) {
    bool is_dark_theme = true;
    sf::RectangleShape background(sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT));
    background.setFillColor(is_dark_theme ? sf::Color(40, 40, 40) : sf::Color(230, 230, 230));

    const int field_count = 3;
    std::string labels[field_count] = {"N", "L (nm)", "T (mcs)"};
    std::string count_str = std::to_string(settings.particle_count);
    std::string size_str = std::to_string(settings.mean_free_path);
    std::string delay_str = std::to_string(settings.delay);
    std::string* fields[field_count] = {&count_str, &size_str, &delay_str};

    sf::RectangleShape input_boxes[field_count];
    sf::Text input_texts[field_count];
    sf::Text label_objects[field_count];

    float total_height = field_count * 70.f + 100.f;
    float start_y = (WINDOW_HEIGHT - total_height) / 2.f;
    float x = (WINDOW_WIDTH - 340) / 2.f;
    float y = start_y;

    sf::Text title("Random Walks Simulator", font, 40);
    title.setPosition((WINDOW_WIDTH - title.getGlobalBounds().width) / 2.f, start_y - 80);
    title.setFillColor(is_dark_theme ? sf::Color::White : sf::Color::Black);

    for (int i = 0; i < field_count; ++i) {
        label_objects[i] = sf::Text(labels[i], font, 24);
        label_objects[i].setPosition(x, y + 8);
        label_objects[i].setFillColor(is_dark_theme ? sf::Color::White : sf::Color::Black);

        input_boxes[i] = sf::RectangleShape(sf::Vector2f(200.f, 40.f));
        input_boxes[i].setPosition(x + 140, y);
        input_boxes[i].setFillColor(is_dark_theme ? sf::Color(60, 60, 60) : sf::Color::White);
        input_boxes[i].setOutlineThickness(2.f);
        input_boxes[i].setOutlineColor(sf::Color(100, 100, 100));

        input_texts[i] = sf::Text(*fields[i], font, 20);
        input_texts[i].setPosition(x + 150, y + 10);
        input_texts[i].setFillColor(is_dark_theme ? sf::Color::White : sf::Color::Black);

        y += 70;
    }

    sf::RectangleShape cursor(sf::Vector2f(2, 24));
    cursor.setFillColor(sf::Color::Red);

    sf::Text usage(
        "Usage:\n"
        "Enter - Start Simulation\n"
        "H - Hide/Show Controls\n"
        "T - Toggle Theme",
        font, 16
    );
    usage.setFillColor(is_dark_theme ? sf::Color::White : sf::Color::Black);
    usage.setPosition(10, 10);

    bool show_usage = true;
    int active_field = 0;
    bool cursor_visible = true;
    sf::Clock cursor_clock;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::MouseButtonPressed) {
                sf::Vector2i mouse_pos = sf::Mouse::getPosition(window);
                for (int i = 0; i < field_count; ++i) {
                    sf::FloatRect bounds = input_boxes[i].getGlobalBounds();
                    if (bounds.contains(mouse_pos.x, mouse_pos.y)) {
                        active_field = i;
                        break;
                    }
                }
            }

            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Enter) {
                    settings.particle_count = std::max(1, atoi(count_str.c_str()));
                    settings.mean_free_path = atof(size_str.c_str());
                    settings.delay = atof(delay_str.c_str());
                    return;
                }

                if (event.key.code == sf::Keyboard::Up)
                    active_field = (active_field - 1 + field_count) % field_count;

                if (event.key.code == sf::Keyboard::Down)
                    active_field = (active_field + 1) % field_count;

                if (event.key.code == sf::Keyboard::H)
                    show_usage = !show_usage;

                if (event.key.code == sf::Keyboard::T) {
                    is_dark_theme = !is_dark_theme;
                    background.setFillColor(is_dark_theme ? sf::Color(40, 40, 40) : sf::Color(230, 230, 230));

                    for (int i = 0; i < field_count; ++i) {
                        label_objects[i].setFillColor(is_dark_theme ? sf::Color::White : sf::Color::Black);
                        input_boxes[i].setFillColor(is_dark_theme ? sf::Color(60, 60, 60) : sf::Color::White);
                        input_texts[i].setFillColor(is_dark_theme ? sf::Color::White : sf::Color::Black);
                    }

                    usage.setFillColor(is_dark_theme ? sf::Color::White : sf::Color::Black);
                }
            }

            if (event.type == sf::Event::TextEntered && active_field >= 0) {
                std::string& field = *fields[active_field];
                char c = static_cast<char>(event.text.unicode);

                if (c == '\b' && !field.empty())
                    field.pop_back();
                else if ((std::isdigit(c) || c == '.') && field.size() < 7)
                    field += c;

                input_texts[active_field].setString(field);
            }
        }

        if (cursor_clock.getElapsedTime().asSeconds() > 0.5f) {
            cursor_visible = !cursor_visible;
            cursor_clock.restart();
        }

        window.clear();
        window.draw(background);
        window.draw(title);

        if (show_usage)
            window.draw(usage);

        for (int i = 0; i < field_count; ++i) {
            window.draw(label_objects[i]);
            input_boxes[i].setOutlineColor(i == active_field ? sf::Color::Red : sf::Color(100, 100, 100));
            window.draw(input_boxes[i]);
            window.draw(input_texts[i]);

            if (i == active_field && cursor_visible) {
                cursor.setPosition(
                    input_texts[i].findCharacterPos(input_texts[i].getString().getSize()).x + 2,
                    input_texts[i].getPosition().y
                );
                window.draw(cursor);
            }
        }

        window.display();
    }
}
