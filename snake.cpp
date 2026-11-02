#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include <termios.h>
#include <unistd.h>

int main() {

    termios original;
    if (tcgetattr(STDIN_FILENO, &original) == -1) {
        return 1;
    }

    termios settings = original;
    settings.c_lflag &= ~(ICANON | ECHO);
    settings.c_cc[VMIN] = 0;
    settings.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &settings) == -1) {
        return 1;
    }

    const int side = 15;
    const int midx = side / 2;
    const int midy = side / 2;
    std::deque<std::pair<int, int>> snake;
    std::vector<std::string> screen(side, std::string(side, ' '));
    int score = 0;
    int posx = midx;
    int posy = midy;
    bool quit = false;
    bool paused = false;
    bool die = false;

    std::pair<int, int> applepos = {-1, -1};
    std::pair<int, int> direction = {1, 0};
    std::pair<int, int> head = {midy, midx};
    snake.push_back(head);
    std::cout << "\x1b[2J\x1b[H";

    while (true) {
        while (true) {
            screen = std::vector<std::string>(side, std::string(side, ' '));
            std::cout << "\x1b[H";
            std::this_thread::sleep_for(std::chrono::milliseconds(200)); // 5 ticks per second

            for (const auto& segment : snake) {
                if (segment == applepos || applepos.first < 0) {
                    applepos.first = std::rand() % side;
                    applepos.second = std::rand() % side;
                }
            }
            screen[applepos.first][applepos.second] = '$';

            char key;
            if (read(STDIN_FILENO, &key, 1) == 1) {
                tcflush(STDIN_FILENO, TCIFLUSH);
                if (key == 'q') {
                    quit = true;
                    break;
                }
                if (key == 'p') {
                    paused = !paused;
                }
                if (key == 'w' && direction != std::pair<int, int>{1, 0}) {
                    direction = {-1, 0};
                } else if (key == 'a' && direction != std::pair<int, int>{0, 1}) {
                    direction = {0, -1};
                } else if (key == 's' && direction != std::pair<int, int>{-1, 0}) {
                    direction = {1, 0};
                } else if (key == 'd' && direction != std::pair<int, int>{0, -1}) {
                    direction = {0, 1};
                }
            }
            if (paused) continue;

            posy += direction.first;
            posx += direction.second;
            if (posy < 0 || posy >= side || posx < 0 || posx >= side) {
                break;
            }
            head = {posy, posx};
            screen[posy][posx] = '@';


            if (head == applepos) {
                ++score;
            } else {
                snake.pop_back();
            }
            snake.push_front(head);

            for (std::size_t i = 1; i < snake.size(); ++i) {
                screen[snake[i].first][snake[i].second] = 'o';
                if (head == snake[i]) {
                    die = true;
                }
            }

            std::cout << "\n\n\n\n";
            std::cout << std::string(side, '-') << ' ' << score << ' '
                      << std::string(side, '-') << '\n';
            std::cout << std::string(side * 2 + 2, '#') << '\n';
            for (const auto& line : screen) {
                std::cout << '#';
                for (char cell : line) {
                    std::cout << cell << ' ';
                }
                std::cout << "#\n";
            }
            std::cout << std::string(side * 2 + 2, '#') << '\n' << std::flush;
            if (die) break;
        }
        if (quit) break;

        tcflush(STDIN_FILENO, TCIFLUSH); // Do not carry gameplay input into the menu.
        std::cout << "\x1b[" << side + 8 << ";1H\x1b[J"
                  << "Game Over! Score: " << score << '\n'
                  << "Press r to restart or q to quit.\n" << std::flush;

        snake.clear();
        snake.push_back({midy, midx});
        posy = midy;
        posx = midx;
        head = {midy, midx};
        direction = {1, 0};
        applepos = {-1, -1};
        score = 0;
        die = false;
        paused = false;

        while (true) {
            char key;
            if (read(STDIN_FILENO, &key, 1) == 1) {
                tcflush(STDIN_FILENO, TCIFLUSH); // Discard queued extra key presses.
                if (key == 'r') break;
                if (key == 'q') {
                    quit = true;
                    break;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        if (quit) break;
        std::cout << "\x1b[2J\x1b[H" << std::flush;
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &original);
    return 0;
}
