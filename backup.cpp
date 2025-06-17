#include "simulation.h"
#include "config.h"
#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <string>

// Функция рисует 2D график: x - r, y - доля частиц, + теория Рэлея
void draw_graph_2d(sf::RenderWindow& window, sf::Font& font,
                   const std::vector<float>& histogram_data,
                   float max_radius,
                   int particle_count,
                   float step_size,
                   int current_step) {
    if (histogram_data.empty()) return;

    const float chart_left   = 80.f;
    const float chart_top    = 80.f;
    const float chart_width  = WINDOW_WIDTH - 160.f;
    const float chart_height = WINDOW_HEIGHT - 160.f;

    // Оси
    sf::Vertex axis_x[] = {
        sf::Vertex(sf::Vector2f(chart_left, chart_top + chart_height), sf::Color::White),
        sf::Vertex(sf::Vector2f(chart_left + chart_width, chart_top + chart_height), sf::Color::White)
    };
    sf::Vertex axis_y[] = {
        sf::Vertex(sf::Vector2f(chart_left, chart_top + chart_height), sf::Color::White),
        sf::Vertex(sf::Vector2f(chart_left, chart_top), sf::Color::White)
    };

    window.draw(axis_x, 2, sf::Lines);
    window.draw(axis_y, 2, sf::Lines);

    // Подпись
    sf::Text title("Fraction of particles vs Radius", font, 20);
    title.setFillColor(sf::Color::Yellow);
    title.setPosition(10, 10);
    window.draw(title);

    // Параметры теории Рэлея
    float sigma_squared = step_size * step_size * current_step;
    float max_theory = 0.0f;

    // Найдём максимум для нормализации
    for (size_t i = 0; i < histogram_data.size(); ++i) {
        if (histogram_data[i] > max_theory)
            max_theory = histogram_data[i];
    }

    // Рисуем точки графика
    float bin_size = max_radius / histogram_data.size();

    for (size_t i = 1; i < histogram_data.size(); ++i) {
        float x1 = chart_left + chart_width * (i - 1) / (histogram_data.size() - 1);
        float y1 = chart_top + chart_height - chart_height * histogram_data[i - 1];
        float x2 = chart_left + chart_width * i / (histogram_data.size() - 1);
        float y2 = chart_top + chart_height - chart_height * histogram_data[i];

        sf::Vertex line[2] = {
            sf::Vertex(sf::Vector2f(x1, y1), sf::Color::Cyan),
            sf::Vertex(sf::Vector2f(x2, y2), sf::Color::Cyan)
        };
        window.draw(line, 2, sf::Lines);
    }

    // Рисуем теорию Рэлея
    for (int i = 1; i < histogram_data.size(); ++i) {
        float r1 = bin_size * (i - 1);
        float r2 = bin_size * i;

        float pdf1 = (r1 / sigma_squared) * exp(-r1 * r1 / (2 * sigma_squared));
        float pdf2 = (r2 / sigma_squared) * exp(-r2 * r2 / (2 * sigma_squared));

        float y1 = chart_top + chart_height - chart_height * pdf1 / 4;
        float y2 = chart_top + chart_height - chart_height * pdf2 / 4;

        float x1 = chart_left + chart_width * (i - 1) / (histogram_data.size() - 1);
        float x2 = chart_left + chart_width * i / (histogram_data.size() - 1);

        sf::Vertex theory_line[2] = {
            sf::Vertex(sf::Vector2f(x1, y1), sf::Color::Magenta),
            sf::Vertex(sf::Vector2f(x2, y2), sf::Color::Magenta)
        };
        window.draw(theory_line, 2, sf::Lines);
    }
}

void run_simulation(sf::RenderWindow& window, sf::Font& font, Settings settings) {
    sf::View camera = window.getDefaultView();
    camera.setCenter(0, 0);
    window.setView(camera);

    struct Particle {
        sf::Vector2f position;
        std::vector<sf::Vertex> path;
        sf::Color color;
    };

    std::vector<Particle> particles(settings.particle_count);
    sf::Color colors[] = {
        sf::Color(255, 100, 100),
        sf::Color(100, 255, 100),
        sf::Color(100, 100, 255),
        sf::Color(255, 100, 255),
        sf::Color(100, 255, 255),
        sf::Color(255, 255, 100)
    };

    for (int i = 0; i < settings.particle_count; ++i) {
        Particle p;
        p.position = sf::Vector2f(0, 0);
        p.path.push_back(sf::Vertex(p.position, colors[i % 6]));
        p.color = colors[i % 6];
        particles[i] = p;
    }

    bool paused = false;
    int current_step = 0;
    bool show_controls = true;
    bool is_dark_theme = true;
    bool show_paths = true;
    bool show_info_mode = false;

    sf::Text controls(
        "Usage:\n"
        "Q - Back to Menu\n"
        "T - Toggle Theme\n"
        "H - Hide/Show controls\n"
        "P - Toggle Paths\n"
        "Space - Pause\n"
        "R - Reset\n"
        "Z/X - Zoom\n"
        "Tab - Switch View",
        font, 16
    );

    controls.setFillColor(sf::Color::White);
    controls.setPosition(10, 10);
    float current_zoom = 1.0f;

    const int BIN_COUNT = 1500;
    std::vector<float> histogram_data(BIN_COUNT, 0.0f); // Данные графика

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Q)
                    return;

                if (event.key.code == sf::Keyboard::H)
                    show_controls = !show_controls;

                if (event.key.code == sf::Keyboard::T) {
                    is_dark_theme = !is_dark_theme;
                    controls.setFillColor(is_dark_theme ? sf::Color::White : sf::Color::Black);
                    window.clear(is_dark_theme ? sf::Color(30, 30, 30) : sf::Color(245, 245, 245));
                }

                if (event.key.code == sf::Keyboard::Space)
                    paused = !paused;

                if (event.key.code == sf::Keyboard::R) {
                    for (auto& p : particles) {
                        p.position = sf::Vector2f(0, 0);
                        p.path.clear();
                        p.path.push_back(sf::Vertex(p.position, p.color));
                    }
                    current_step = 0;
                }

                if (event.key.code == sf::Keyboard::P)
                    show_paths = !show_paths;

                if (event.key.code == sf::Keyboard::Tab)
                    show_info_mode = !show_info_mode;

                int left   = sf::Keyboard::isKeyPressed(sf::Keyboard::Left );
                int right  = sf::Keyboard::isKeyPressed(sf::Keyboard::Right);
                int up     = sf::Keyboard::isKeyPressed(sf::Keyboard::Up   );
                int down   = sf::Keyboard::isKeyPressed(sf::Keyboard::Down );
                bool zoom   = sf::Keyboard::isKeyPressed(sf::Keyboard::Z );
                bool reduce = sf::Keyboard::isKeyPressed(sf::Keyboard::X );

                float offset_x = current_zoom * MOVE_CAMERA_FACTOR * (right - left);
                float offset_y = current_zoom * MOVE_CAMERA_FACTOR * (down - up);

                camera.move(offset_x, offset_y);

                if (zoom) {
                    current_zoom *= ZOOM_IN_CAMERA_FACTOR;
                    camera.zoom(ZOOM_IN_CAMERA_FACTOR);
                }

                if (reduce) {
                    current_zoom *= ZOOM_OUT_CAMERA_FACTOR;
                    camera.zoom(ZOOM_OUT_CAMERA_FACTOR);
                }
            }
        }

            if (!paused && current_step < MAX_STEPS) {
            histogram_data.assign(BIN_COUNT, 0.0f);

            float max_radius = settings.step_size * sqrt(2 * current_step);
            float bin_size = max_radius / BIN_COUNT;

            int inside = 0;
            for (auto& p : particles) {
                float dist_sq = p.position.x * p.position.x + p.position.y * p.position.y;
                float radius = sqrt(dist_sq);
                int bin = static_cast<int>(radius / bin_size);
                if (bin < 0) bin = 0; // Защита от отрицательных значений
                if (bin >= BIN_COUNT) bin = BIN_COUNT - 1;
                for (int i = bin; i < BIN_COUNT; ++i) {
                    histogram_data[i] += 1.0f;
                }
            }

            // Нормализуем данные
            for (int i = 0; i < BIN_COUNT; ++i) {
                histogram_data[i] /= settings.particle_count;
            }

            // Обновляем позиции частиц
            for (auto& p : particles) {
                float angle = static_cast<float>(rand()) / RAND_MAX * 2.0f * M_PI;
                float dx = cos(angle) * settings.step_size;
                float dy = sin(angle) * settings.step_size;
                p.position.x += dx;
                p.position.y += dy;
                p.path.push_back(sf::Vertex(p.position, p.color));
            }

            current_step++;
            sf::sleep(sf::microseconds(settings.delay));
        }

        window.clear(is_dark_theme ? sf::Color(30, 30, 30) : sf::Color(245, 245, 245));

        if (!show_info_mode) {
            window.setView(camera);

            float min_x = camera.getCenter().x - camera.getSize().x / 2.f;
            float max_x = camera.getCenter().x + camera.getSize().x / 2.f;
            float min_y = camera.getCenter().y - camera.getSize().y / 2.f;
            float max_y = camera.getCenter().y + camera.getSize().y / 2.f;

            sf::Color grid_color = is_dark_theme ? sf::Color(80, 80, 80) : sf::Color(180, 180, 180);
            sf::Color axis_color = is_dark_theme ? sf::Color::White : sf::Color::Black;
            float grid_step = settings.step_size * 10;

            float start_x = std::floor(min_x / grid_step) * grid_step;
            float end_x   = std::ceil(max_x / grid_step) * grid_step;

            float start_y = std::floor(min_y / grid_step) * grid_step;
            float end_y   = std::ceil(max_y / grid_step) * grid_step;

            for (float x = start_x; x <= end_x; x += grid_step) {
                sf::Vertex line[] = {
                    sf::Vertex(sf::Vector2f(x, min_y), grid_color),
                    sf::Vertex(sf::Vector2f(x, max_y), grid_color)
                };
                window.draw(line, 2, sf::Lines);
            }

            for (float y = start_y; y <= end_y; y += grid_step) {
                sf::Vertex line[] = {
                    sf::Vertex(sf::Vector2f(min_x, y), grid_color),
                    sf::Vertex(sf::Vector2f(max_x, y), grid_color)
                };
                window.draw(line, 2, sf::Lines);
            }

            sf::Vertex axis_x[] = {
                sf::Vertex(sf::Vector2f(-1e6, 0), axis_color),
                sf::Vertex(sf::Vector2f(1e6, 0), axis_color)
            };
            sf::Vertex axis_y[] = {
                sf::Vertex(sf::Vector2f(0, -1e6), axis_color),
                sf::Vertex(sf::Vector2f(0, 1e6), axis_color)
            };

            window.draw(axis_x, 2, sf::Lines);
            window.draw(axis_y, 2, sf::Lines);

            if (show_paths) {
                for (const auto& p : particles) {
                    if (!p.path.empty())
                        window.draw(&p.path[0], p.path.size(), sf::LineStrip);
                }
            }

            for (const auto& p : particles) {
                sf::CircleShape dot(current_zoom);
                dot.setFillColor(sf::Color::Red);
                dot.setPosition(p.position.x - current_zoom, p.position.y - current_zoom);
                window.draw(dot);
            }

            float radius = settings.step_size * sqrt(2 * current_step);
            if (radius > 0) {
                sf::CircleShape dynamic_circle(radius);
                dynamic_circle.setOrigin(radius, radius);
                dynamic_circle.setPosition(0.f, 0.f);
                dynamic_circle.setOutlineThickness(pow(current_step, 0.25) * current_zoom);
                dynamic_circle.setOutlineColor(sf::Color(128, 128, 128));
                dynamic_circle.setFillColor(sf::Color::Transparent);
                window.draw(dynamic_circle);
            }

            window.setView(window.getDefaultView());

            if (show_controls) {
                controls.setFillColor(is_dark_theme ? sf::Color::White : sf::Color::Black);
                window.draw(controls);
            }
        } else {
            // === Режим графика (Tab) ===
            draw_graph_2d(window, font, histogram_data, settings.step_size * sqrt(2 * current_step),
                          settings.particle_count, settings.step_size, current_step);
        }

        window.display();
    }
}
