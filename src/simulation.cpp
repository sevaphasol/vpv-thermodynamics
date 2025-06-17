#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <algorithm>
#include <random>
#include "simulation.h"
#include "config.h"
#include "types.h"

// === Глобальные переменные для хранения истории графика ===
std::vector<std::pair<float, float>> rayleigh_history;    // {r, cdf}
std::vector<std::pair<float, int>>    histogram_history;  // {r, count}

// === Обновление истории данных ===
void update_histogram_data(const std::vector<Particle>& particles,
                           float mean_free_path,
                           int current_step) {
    histogram_history.clear();
    rayleigh_history.clear();

    const int BIN_COUNT = 100;

    // Вычисляем расстояния и сортируем
    std::vector<float> distances;
    for (const auto& p : particles) {
        distances.push_back(std::sqrt(p.position.x * p.position.x + p.position.y * p.position.y));
    }
    std::sort(distances.begin(), distances.end());

    float max_radius = mean_free_path * sqrt(2 * current_step);
    if (max_radius < 1e-5f) max_radius = 1e-5f;

    // σ^2 = λ² * N
    float sigma_sq = mean_free_path * mean_free_path * current_step;
    if (sigma_sq <= 1e-5f) sigma_sq = 1e-5f;

    for (int i = 0; i < BIN_COUNT; ++i) {
        float r = max_radius * i / (BIN_COUNT - 1);

        // Подсчёт числа частиц до радиуса r
        size_t count = std::upper_bound(distances.begin(), distances.end(), r) - distances.begin();
        histogram_history.emplace_back(r, static_cast<int>(count));

        // Теоретическое значение CDF Рэлея
        float cdf = 1.0f - exp(-r * r / (2 * sigma_sq));
        rayleigh_history.emplace_back(r, cdf);
    }
}

// === Функция отрисовки графика CDF ===
void draw_histogram_with_rayleigh(sf::RenderWindow& window, sf::Font& font,
                                  const std::vector<Particle>& particles,
                                  int particle_count,
                                  float mean_free_path,
                                  int current_step) {
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

    // Заголовок
    sf::Text title("CDF vs Radius", font, 20);
    title.setFillColor(sf::Color(160, 120, 140));
    title.setPosition(10, 10);
    window.draw(title);

    if (particles.empty()) return;

    // Собираем расстояния всех частиц от центра
    std::vector<float> distances;
    for (const auto& p : particles) {
        float dist_sq = p.position.x * p.position.x + p.position.y * p.position.y;
        distances.push_back(sqrt(dist_sq));
    }
    std::sort(distances.begin(), distances.end());

    const int BIN_COUNT = 100;
    std::vector<float> histogram_radii(BIN_COUNT);
    std::vector<int> hit_counts(BIN_COUNT, 0);

    float max_radius = distances.empty() ? 0.1f : distances.back();

    for (int i = 0; i < BIN_COUNT; ++i) {
        histogram_radii[i] = max_radius * i / (BIN_COUNT - 1);
    }

    size_t count = 0;
    for (int i = 0; i < BIN_COUNT && count < distances.size(); ++i) {
        while (count < distances.size() && distances[count] <= histogram_radii[i]) {
            ++count;
        }
        hit_counts[i] = static_cast<int>(count);
    }

    // Теоретическая CDF Рэлея
    float sigma_sq = mean_free_path * mean_free_path * current_step;
    if (sigma_sq <= 1e-5f) sigma_sq = 1e-5f;

    std::vector<float> cdf_values(BIN_COUNT);
    for (int i = 0; i < BIN_COUNT; ++i) {
        float r = histogram_radii[i];
        cdf_values[i] = 1.0f - exp(-r * r / (2 * sigma_sq));
    }

    // === Рисуем бирюзовую линию (CDF) как накопленную долю частиц ===
    for (int i = 1; i < BIN_COUNT; ++i) {
        float x1 = chart_left + chart_width * histogram_radii[i - 1] / max_radius;
        float x2 = chart_left + chart_width * histogram_radii[i] / max_radius;
        float y1 = chart_top + chart_height - chart_height * (static_cast<float>(hit_counts[i - 1]) / particle_count);
        float y2 = chart_top + chart_height - chart_height * (static_cast<float>(hit_counts[i]) / particle_count);
        sf::Vertex line[] = {
            sf::Vertex(sf::Vector2f(x1, y1), sf::Color::Cyan),
            sf::Vertex(sf::Vector2f(x2, y2), sf::Color::Cyan)
        };
        window.draw(line, 2, sf::Lines);
    }

    // === Рисуем фиолетовую кривую (теоретическая CDF Рэлея) ===
    for (int i = 1; i < BIN_COUNT; ++i) {
        float x1 = chart_left + chart_width * histogram_radii[i - 1] / max_radius;
        float x2 = chart_left + chart_width * histogram_radii[i] / max_radius;
        float y1 = chart_top + chart_height - chart_height * cdf_values[i - 1];
        float y2 = chart_top + chart_height - chart_height * cdf_values[i];
        sf::Vertex line[] = {
            sf::Vertex(sf::Vector2f(x1, y1), sf::Color::Magenta),
            sf::Vertex(sf::Vector2f(x2, y2), sf::Color::Magenta)
        };
        window.draw(line, 2, sf::Lines);
    }

    // === Рисуем зелёную линию для теоретического RMS радиуса ===
    float theoretical_R = mean_free_path * sqrt(2 * current_step);
    float theoretical_N = 1.0f - exp(-theoretical_R * theoretical_R / (2 * sigma_sq));
    float x_start_theory = chart_left;
    float x_end_theory = chart_left + chart_width * theoretical_R / max_radius;
    float y_N_theory = chart_top + chart_height - chart_height * theoretical_N;
    sf::Vertex hline_theory[] = {
        sf::Vertex(sf::Vector2f(x_start_theory, y_N_theory), sf::Color::Green),
        sf::Vertex(sf::Vector2f(x_end_theory, y_N_theory), sf::Color::Green)
    };
    window.draw(hline_theory, 2, sf::Lines);

    float arrow_length = 10.0f;

    // Стрелочка слева
    sf::Vertex left_arrow[] = {
        sf::Vertex(sf::Vector2f(x_start_theory, y_N_theory), sf::Color::Green),
        sf::Vertex(sf::Vector2f(x_start_theory + arrow_length, y_N_theory - arrow_length), sf::Color::Green),
        sf::Vertex(sf::Vertex(sf::Vector2f(x_start_theory, y_N_theory), sf::Color::Green)),
        sf::Vertex(sf::Vector2f(x_start_theory + arrow_length, y_N_theory + arrow_length), sf::Color::Green)
    };
    window.draw(left_arrow, 4, sf::Lines);

    // Стрелочка справа
    sf::Vertex right_arrow[] = {
        sf::Vertex(sf::Vector2f(x_end_theory, y_N_theory), sf::Color::Green),
        sf::Vertex(sf::Vector2f(x_end_theory - arrow_length, y_N_theory - arrow_length), sf::Color::Green),
        sf::Vertex(sf::Vertex(sf::Vector2f(x_end_theory, y_N_theory), sf::Color::Green)),
        sf::Vertex(sf::Vector2f(x_end_theory - arrow_length, y_N_theory + arrow_length), sf::Color::Green)
    };
    window.draw(right_arrow, 4, sf::Lines);

    // Надпись R, N(R)
    sf::Text label_r_theory("R = " + std::to_string(theoretical_R).substr(0, 5), font, 14);
    label_r_theory.setFillColor(sf::Color::Green);
    label_r_theory.setPosition(WINDOW_WIDTH - 150, 120);
    window.draw(label_r_theory);

    sf::Text label_n_theory("N(R) = " + std::to_string(theoretical_N).substr(0, 5), font, 14);
    label_n_theory.setFillColor(sf::Color::Green);
    label_n_theory.setPosition(WINDOW_WIDTH - 150, 140);
    window.draw(label_n_theory);

    // === Теперь рисуем аналогичную линию для экспериментального RMS радиуса R' ===
    float sum_r_squared = 0.0f;
    for (const auto& p : particles) {
        float r = sqrt(p.position.x * p.position.x + p.position.y * p.position.y);
        sum_r_squared += r * r;
    }
    float R_prime = sqrt(sum_r_squared / particle_count); // Среднее квадратичное отклонение
    float N_of_R_prime = 0.0f;

    // Находим N(R') по экспериментальной CDF
    for (size_t i = 0; i < histogram_radii.size(); ++i) {
        if (histogram_radii[i] >= R_prime) {
            N_of_R_prime = static_cast<float>(hit_counts[i]) / particle_count;
            break;
        }
    }
    if (N_of_R_prime == 0.0f && !hit_counts.empty()) {
        N_of_R_prime = static_cast<float>(hit_counts.back()) / particle_count;
    }

    // Позиции на графике
    float x_start_exp = chart_left;
    float x_end_exp = chart_left + chart_width * R_prime / max_radius;
    float y_N_exp = chart_top + chart_height - chart_height * N_of_R_prime;

    // Горизонтальная линия
    sf::Vertex hline_exp[] = {
        sf::Vertex(sf::Vector2f(x_start_exp, y_N_exp), sf::Color::Yellow),
        sf::Vertex(sf::Vector2f(x_end_exp, y_N_exp), sf::Color::Yellow)
    };
    window.draw(hline_exp, 2, sf::Lines);

    // Стрелочки
    sf::Vertex left_arrow_exp[] = {
        sf::Vertex(sf::Vector2f(x_start_exp, y_N_exp), sf::Color::Yellow),
        sf::Vertex(sf::Vector2f(x_start_exp + arrow_length, y_N_exp - arrow_length), sf::Color::Yellow),
        sf::Vertex(sf::Vertex(sf::Vector2f(x_start_exp, y_N_exp), sf::Color::Yellow)),
        sf::Vertex(sf::Vector2f(x_start_exp + arrow_length, y_N_exp + arrow_length), sf::Color::Yellow)
    };
    window.draw(left_arrow_exp, 4, sf::Lines);

    sf::Vertex right_arrow_exp[] = {
        sf::Vertex(sf::Vector2f(x_end_exp, y_N_exp), sf::Color::Yellow),
        sf::Vertex(sf::Vector2f(x_end_exp - arrow_length, y_N_exp - arrow_length), sf::Color::Yellow),
        sf::Vertex(sf::Vertex(sf::Vector2f(x_end_exp, y_N_exp), sf::Color::Yellow)),
        sf::Vertex(sf::Vector2f(x_end_exp - arrow_length, y_N_exp + arrow_length), sf::Color::Yellow)
    };
    window.draw(right_arrow_exp, 4, sf::Lines);

    // Надпись R', N(R')
    sf::Text label_r_exp("R' = " + std::to_string(R_prime).substr(0, 5), font, 14);
    label_r_exp.setFillColor(sf::Color::Yellow);
    label_r_exp.setPosition(WINDOW_WIDTH - 150, 180);
    window.draw(label_r_exp);

    sf::Text label_n_exp("N(R') = " + std::to_string(N_of_R_prime).substr(0, 5), font, 14);
    label_n_exp.setFillColor(sf::Color::Yellow);
    label_n_exp.setPosition(WINDOW_WIDTH - 150, 200);
    window.draw(label_n_exp);

    // === Подписываем деления по осям ===
    const int TICKS_X = 10;
    const int TICKS_Y = 10;

    for (int i = 0; i <= TICKS_X; ++i) {
        float r = max_radius * i / TICKS_X;
        float x = chart_left + chart_width * i / TICKS_X;
        sf::Vertex tick[] = {
            sf::Vertex(sf::Vector2f(x, chart_top + chart_height), sf::Color::White),
            sf::Vertex(sf::Vector2f(x, chart_top + chart_height + 5), sf::Color::White)
        };
        window.draw(tick, 2, sf::Lines);
        sf::Text label(std::to_string(int(r)), font, 14);
        label.setFillColor(sf::Color::White);
        label.setPosition(x - 10, chart_top + chart_height + 10);
        window.draw(label);
    }

    for (int i = 0; i <= TICKS_Y; ++i) {
        float fraction = i / (float)TICKS_Y;
        float y = chart_top + chart_height - chart_height * fraction;
        sf::Vertex tick[] = {
            sf::Vertex(sf::Vector2f(chart_left, y), sf::Color::White),
            sf::Vertex(sf::Vector2f(chart_left - 5, y), sf::Color::White)
        };
        window.draw(tick, 2, sf::Lines);
        sf::Text label(std::to_string(fraction).substr(0, 3), font, 14);
        label.setFillColor(sf::Color::White);
        label.setPosition(chart_left - 40, y - 10);
        window.draw(label);
    }

    // === Подписи осей ===
    sf::Text label_x("Radius", font, 16);
    label_x.setFillColor(sf::Color::White);
    label_x.setPosition(chart_left + chart_width / 2 - 30, chart_top + chart_height + 40);
    window.draw(label_x);

    sf::Text label_y("CDF", font, 16);
    label_y.setFillColor(sf::Color::White);
    label_y.setRotation(-90);
    label_y.setPosition(chart_left - 50, chart_top + chart_height / 2 - 20);
    window.draw(label_y);
}

// === Функция отрисовки графика PDF ===
void draw_histogram_pdf(sf::RenderWindow& window, sf::Font& font,
                        const std::vector<Particle>& particles,
                        int particle_count,
                        float mean_free_path,
                        int current_step) {
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

    // Заголовок
    sf::Text title("PDF vs Radius", font, 20);
    title.setFillColor(sf::Color(160, 120, 140));
    title.setPosition(10, 10);
    window.draw(title);

    if (particles.empty()) return;

    // Собираем расстояния всех частиц от центра
    std::vector<float> distances;
    for (const auto& p : particles) {
        distances.push_back(std::sqrt(p.position.x * p.position.x + p.position.y * p.position.y));
    }
    std::sort(distances.begin(), distances.end());

    const int BIN_COUNT = 100;
    std::vector<float> histogram_radii(BIN_COUNT);
    std::vector<int> hit_counts(BIN_COUNT, 0);

    float max_radius = distances.empty() ? 0.1f : distances.back();

    for (int i = 0; i < BIN_COUNT; ++i) {
        histogram_radii[i] = max_radius * i / (BIN_COUNT - 1);
    }

    size_t count = 0;
    for (int i = 0; i < BIN_COUNT && count < distances.size(); ++i) {
        while (count < distances.size() && distances[count] <= histogram_radii[i]) {
            ++count;
        }
        hit_counts[i] = static_cast<int>(count);
    }

    // Подсчёт плотности
    float sigma_sq = mean_free_path * mean_free_path * current_step;
    if (sigma_sq <= 1e-5f) sigma_sq = 1e-5f;

    float dr = max_radius / (BIN_COUNT - 1);
    std::vector<float> empirical_pdf(BIN_COUNT);
    std::vector<float> pdf_values(BIN_COUNT);

    for (int i = 0; i < BIN_COUNT; ++i) {
        float r = histogram_radii[i];
        pdf_values[i] = (r / sigma_sq) * exp(-r * r / (2 * sigma_sq));
        empirical_pdf[i] = (hit_counts[i] - (i > 0 ? hit_counts[i - 1] : 0)) / (dr * particle_count);
    }

    // === Найдём максимумы для масштабирования ===
    float max_empirical = !empirical_pdf.empty() ? *std::max_element(empirical_pdf.begin(), empirical_pdf.end()) : 0.1f;
    float max_theoretical = !pdf_values.empty() ? *std::max_element(pdf_values.begin(), pdf_values.end()) : 0.1f;
    float y_max = std::max(max_empirical, max_theoretical);
    if (y_max < 1e-5f) y_max = 1e-5f;

    // === Рисуем бирюзовую линию (экспериментальная PDF) ===
    for (int i = 1; i < BIN_COUNT; ++i) {
        float x1 = chart_left + chart_width * histogram_radii[i - 1] / max_radius;
        float x2 = chart_left + chart_width * histogram_radii[i] / max_radius;
        float y1 = chart_top + chart_height - chart_height * (empirical_pdf[i - 1] / y_max);
        float y2 = chart_top + chart_height - chart_height * (empirical_pdf[i] / y_max);
        sf::Vertex line[] = {
            sf::Vertex(sf::Vector2f(x1, y1), sf::Color::Cyan),
            sf::Vertex(sf::Vector2f(x2, y2), sf::Color::Cyan)
        };
        window.draw(line, 2, sf::Lines);
    }

    // === Рисуем фиолетовую кривую (теоретическая PDF Рэлея) ===
    for (int i = 1; i < BIN_COUNT; ++i) {
        float x1 = chart_left + chart_width * histogram_radii[i - 1] / max_radius;
        float x2 = chart_left + chart_width * histogram_radii[i] / max_radius;
        float y1 = chart_top + chart_height - chart_height * (pdf_values[i - 1] / y_max);
        float y2 = chart_top + chart_height - chart_height * (pdf_values[i] / y_max);
        sf::Vertex line[] = {
            sf::Vertex(sf::Vector2f(x1, y1), sf::Color::Magenta),
            sf::Vertex(sf::Vector2f(x2, y2), sf::Color::Magenta)
        };
        window.draw(line, 2, sf::Lines);
    }

    // === Теоретический пик (RMS) ===
    float theoretical_R = mean_free_path * sqrt(current_step); // RMS радиус
    float sigma = theoretical_R;
    sigma_sq = sigma * sigma;

    // Поиск пика в теоретическом графике PDF
    auto max_it = std::max_element(pdf_values.begin(), pdf_values.end());
    int peak_index = static_cast<int>(std::distance(pdf_values.begin(), max_it));
    float r_peak_theory = histogram_radii[peak_index];

    // === Поиск пика в экспериментальном графике PDF ===
    auto max_it_exp = std::max_element(empirical_pdf.begin(), empirical_pdf.end());
    int peak_index_exp = static_cast<int>(std::distance(empirical_pdf.begin(), max_it_exp));
    float r_peak_experiment = histogram_radii[peak_index_exp];

    // === Выводим значения пиков справа сверху ===
    sf::Text label_theory("Theory Peak: " + std::to_string(r_peak_theory).substr(0, 5), font, 16);
    label_theory.setFillColor(sf::Color::Green);
    label_theory.setPosition(WINDOW_WIDTH - 220, 20);
    window.draw(label_theory);

    sf::Text label_exp("Experimental Peak: " + std::to_string(r_peak_experiment).substr(0, 5), font, 16);
    label_exp.setFillColor(sf::Color::Yellow);
    label_exp.setPosition(WINDOW_WIDTH - 220, 50);
    window.draw(label_exp);

    // === Подписываем деления по осям ===
    const int TICKS_X = 10;
    const int TICKS_Y = 10;

    for (int i = 0; i <= TICKS_X; ++i) {
        float r = max_radius * i / TICKS_X;
        float x = chart_left + chart_width * i / TICKS_X;
        sf::Vertex tick[] = {
            sf::Vertex(sf::Vector2f(x, chart_top + chart_height), sf::Color::White),
            sf::Vertex(sf::Vector2f(x, chart_top + chart_height + 5), sf::Color::White)
        };
        window.draw(tick, 2, sf::Lines);
        sf::Text label(std::to_string(int(r)), font, 14);
        label.setFillColor(sf::Color::White);
        label.setPosition(x - 10, chart_top + chart_height + 10);
        window.draw(label);
    }

    for (int i = 0; i <= TICKS_Y; ++i) {
        float fraction = i / (float)TICKS_Y;
        float y = chart_top + chart_height - chart_height * fraction;
        sf::Vertex tick[] = {
            sf::Vertex(sf::Vector2f(chart_left, y), sf::Color::White),
            sf::Vertex(sf::Vector2f(chart_left - 5, y), sf::Color::White)
        };
        window.draw(tick, 2, sf::Lines);
        float value = y_max * fraction;
        sf::Text label((value > 0.00001f ? std::to_string(value).substr(0, 7) : "0"), font, 14);
        label.setFillColor(sf::Color::White);
        label.setPosition(chart_left - 70, y - 10);
        window.draw(label);
    }

    // === Подписи осей ===
    sf::Text label_x("Radius", font, 16);
    label_x.setFillColor(sf::Color::White);
    label_x.setPosition(chart_left + chart_width / 2 - 30, chart_top + chart_height + 40);
    window.draw(label_x);

    sf::Text label_y("PDF", font, 16);
    label_y.setFillColor(sf::Color::White);
    label_y.setRotation(-90);
    label_y.setPosition(chart_left - 50, chart_top + chart_height / 2 - 20);
    window.draw(label_y);
}

// === Основной цикл симуляции с шагами распределенными экспоненциально ===
void run_simulation(sf::RenderWindow& window, sf::Font& font, Settings settings) {
    sf::View camera = window.getDefaultView();
    camera.setCenter(0, 0);
    window.setView(camera);

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
    bool show_plot_mode = false;
    int info_mode = 0; // 0: CDF, 1: PDF

    // Controls text
    sf::Text controls(
        "Usage:\n"
        "Q - Back to Menu\n"
        "T - Toggle Theme\n"
        "H - Hide/Show controls\n"
        "Space - Pause\n"
        "R - Reset\n"
        "Z/X - Zoom\n"
        "Tab - Switch plot PDF/CDF\n"
        "Shift - Show plot",
        font, 16
    );
    controls.setFillColor(sf::Color::White);
    controls.setPosition(10, 10);

    float current_zoom = 1.0f;

    // Инициализируем генератор случайных чисел
    std::random_device rd;
    std::mt19937 gen(rd());
    std::exponential_distribution<float> poisson_dist(1.0f / settings.mean_free_path); // λ = 1/l
    std::normal_distribution<float> gaussian_dist(0.0, 1.0);

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
                    info_mode = !info_mode; // Переключает между CDF и PDF
                if (event.key.code == sf::Keyboard::LShift || event.key.code == sf::Keyboard::RShift)
                    show_plot_mode = !show_plot_mode;

                // Камера и зум
                int left    = sf::Keyboard::isKeyPressed(sf::Keyboard::Left );
                int right   = sf::Keyboard::isKeyPressed(sf::Keyboard::Right);
                int up      = sf::Keyboard::isKeyPressed(sf::Keyboard::Up   );
                int down    = sf::Keyboard::isKeyPressed(sf::Keyboard::Down );
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

        // Обновление позиций частиц
        if (!paused && current_step < MAX_STEPS) {
            for (auto& p : particles) {
                float step = poisson_dist(gen);
                float dx = gaussian_dist(gen) * step / sqrt(2);
                float dy = gaussian_dist(gen) * step / sqrt(2);
                p.position.x += dx;
                p.position.y += dy;
                p.path.push_back(sf::Vertex(p.position, p.color));
            }
            current_step++;
            update_histogram_data(particles, settings.mean_free_path, current_step);
            sf::sleep(sf::microseconds(settings.delay));
        }

        window.clear(is_dark_theme ? sf::Color(30, 30, 30) : sf::Color(245, 245, 245));

        if (!show_plot_mode) {
            window.setView(camera);
            float min_x = camera.getCenter().x - camera.getSize().x / 2.f;
            float max_x = camera.getCenter().x + camera.getSize().x / 2.f;
            float min_y = camera.getCenter().y - camera.getSize().y / 2.f;
            float max_y = camera.getCenter().y + camera.getSize().y / 2.f;

            sf::Color grid_color = is_dark_theme ? sf::Color(80, 80, 80) : sf::Color(180, 180, 180);
            sf::Color axis_color = is_dark_theme ? sf::Color::White : sf::Color::Black;

            float grid_step = settings.mean_free_path * 10;
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

            // Оси координат
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

            float radius = 2 * settings.mean_free_path * sqrt(current_step);
            if (radius > 0) {
                sf::CircleShape dynamic_circle(radius);
                dynamic_circle.setOrigin(radius, radius);
                dynamic_circle.setPosition(0.f, 0.f);
                dynamic_circle.setOutlineThickness(pow(current_step, 0.25f) * current_zoom);
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
            if (info_mode == 0)
                draw_histogram_with_rayleigh(window, font, particles, settings.particle_count, settings.mean_free_path, current_step);
            else
                draw_histogram_pdf(window, font, particles, settings.particle_count, settings.mean_free_path, current_step);
        }

        window.display();
    }
}
