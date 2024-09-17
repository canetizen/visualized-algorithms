#include <SFML/Graphics.hpp>
#include <vector>
#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>

const int DISK_COUNT = 7;

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int DISK_HEIGHT = 20;
const float MAX_DISK_WIDTH = std::max(50.0f, 200.0f - (DISK_COUNT - 1) * 3);
const float DISK_WIDTH_DECREASE = (MAX_DISK_WIDTH - 50.0f) / (DISK_COUNT - 1);
const int TOWER_SPACING = 50 + MAX_DISK_WIDTH;
const int LEFT_MARGIN = (WINDOW_WIDTH - 2 * TOWER_SPACING) / 2;

const int TOWER_WIDTH = 10;
const int TOWER_HEIGHT = (DISK_COUNT + 1) * DISK_HEIGHT;
const int TOWER_BASE_Y = WINDOW_HEIGHT - 100;

const int BAR_HEIGHT = 10;
const int BAR_WIDTH = TOWER_WIDTH + 45;
const int BAR_Y_OFFSET = TOWER_BASE_Y;
const int DISK_Y_OFFSET = BAR_Y_OFFSET - DISK_HEIGHT;

const int DELAY_MILLISEC = 100;

std::mutex mtx;
std::condition_variable cv;
bool ready = false;
bool finished = false;
int iteration = 0;

// Function to draw the towers and disks on the screen
void draw_towers(sf::RenderWindow &window, std::vector<sf::RectangleShape> &tower_a,
                 std::vector<sf::RectangleShape> &tower_b, std::vector<sf::RectangleShape> &tower_c, sf::Font &font) {
    window.clear(sf::Color::White);

    // Draw tower rods
    for (int i = 0; i < 3; ++i) {
        sf::RectangleShape rod(sf::Vector2f(TOWER_WIDTH, TOWER_HEIGHT));
        rod.setFillColor(sf::Color::Black);
        rod.setPosition(LEFT_MARGIN + TOWER_SPACING * i - TOWER_WIDTH / 2, TOWER_BASE_Y - TOWER_HEIGHT);
        window.draw(rod);

        // Draw the horizontal bar beneath the tower
        sf::RectangleShape bar(sf::Vector2f(BAR_WIDTH, BAR_HEIGHT));
        bar.setFillColor(sf::Color::Black);
        bar.setPosition(LEFT_MARGIN + TOWER_SPACING * i - (BAR_WIDTH / 2), BAR_Y_OFFSET);
        window.draw(bar);
    }

    // Draw towers
    for (auto &disk : tower_a) window.draw(disk);
    for (auto &disk : tower_b) window.draw(disk);
    for (auto &disk : tower_c) window.draw(disk);

    // Display the iteration count above the towers
    sf::Text iteration_text;
    iteration_text.setFont(font);
    iteration_text.setString("Iteration: " + std::to_string(iteration));
    iteration_text.setCharacterSize(24);
    iteration_text.setFillColor(sf::Color::Black);
    iteration_text.setPosition(WINDOW_WIDTH / 2 - 50, 20);

    window.draw(iteration_text);
    window.display();
}

// Function to move a disk from one tower to another
void move_disk(std::vector<sf::RectangleShape> &from, std::vector<sf::RectangleShape> &to,
               int to_tower_index) {
    if (!from.empty()) {
        sf::RectangleShape disk = from.back();
        from.pop_back();

        // Calculate the new position of the disk
        float new_x = LEFT_MARGIN + TOWER_SPACING * to_tower_index - disk.getSize().x / 2;
        float new_y = DISK_Y_OFFSET - (to.size() * DISK_HEIGHT); // Ensure no overlap with disks already on the target tower
        disk.setPosition(new_x, new_y);

        to.push_back(disk);

        {
            std::lock_guard<std::mutex> lock(mtx);
            ready = true;
            iteration++;
        }
        cv.notify_one();

        // Wait a bit to visualize the disk movement
        std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MILLISEC));
    }
}

// Recursive Hanoi algorithm
void hanoi(int n, std::vector<sf::RectangleShape> &from, std::vector<sf::RectangleShape> &to,
           std::vector<sf::RectangleShape> &aux, int from_index, int to_index, int aux_index) {
    if (n == 1) {
        move_disk(from, to, to_index);
        return;
    }
    hanoi(n - 1, from, aux, to, from_index, aux_index, to_index);
    move_disk(from, to, to_index);
    hanoi(n - 1, aux, to, from, aux_index, to_index, from_index);
}

int main() {
    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Towers of Hanoi");

    std::vector<sf::RectangleShape> tower_a, tower_b, tower_c;

    sf::Font font;
    font.loadFromFile("../resources/DejaVuSans.ttf");

    // Add disks to the first tower (each disk has a different width)
    for (int i = 0; i < DISK_COUNT; ++i) {
        float disk_width = MAX_DISK_WIDTH - i * DISK_WIDTH_DECREASE;
        sf::RectangleShape disk(sf::Vector2f(disk_width, DISK_HEIGHT));
        disk.setFillColor(sf::Color(50 * i, 100 + 30 * i, 200 - 20 * i));
        float disk_x = LEFT_MARGIN - disk_width / 2;
        float disk_y = DISK_Y_OFFSET - (DISK_HEIGHT * i); // Correct the position to prevent overlap
        disk.setPosition(disk_x, disk_y);
        tower_a.push_back(disk);
    }

    draw_towers(window, tower_a, tower_b, tower_c, font);

    // Start the Hanoi algorithm in a separate thread
    std::thread hanoi_thread([&]() {
        hanoi(DISK_COUNT, tower_a, tower_c, tower_b, 0, 2, 1);
        std::lock_guard<std::mutex> lock(mtx);
        finished = true;
        cv.notify_one();
    });

    // Main loop
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        // Wait for a new move
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, []{ return ready || finished; });
            if (ready) {
                draw_towers(window, tower_a, tower_b, tower_c, font);
                ready = false;
            }
        }

        if (finished) break;
    }

    hanoi_thread.join();

    // Display final state
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }
        draw_towers(window, tower_a, tower_b, tower_c, font);
    }

    return 0;
}
