#include "Snake.h"
#include <ncurses.h>
#include <unistd.h>
#include "Gate.h"
#include <thread>
#include <chrono>

Snake::Snake(int startX, int startY, SnakeMap& snakeMap, ScoreBoard& scoreBoard) 
    : snakeMap(snakeMap), scoreBoard(scoreBoard), currentDirection(RIGHT) {
    reset(startX, startY); // 초기화 로직을 reset 메서드로 이동
    gameOver = false;
}

void Snake::reset(int startX, int startY) {
    body.clear();
    body.push_front(std::make_pair(startY, startX));     // 머리
    body.push_back(std::make_pair(startY, startX - 1));  // 몸통 첫 번째 부분
    body.push_back(std::make_pair(startY, startX - 2));  // 몸통 두 번째 부분

    for (const auto& segment : body) {
        snakeMap.setMap(segment.first, segment.second, 4); // Snake body on the map
    }
    snakeMap.setMap(body.front().first, body.front().second, 3); // Snake head on the map
}

// 복사 생성자
Snake::Snake(const Snake& other)
    : snakeMap(other.snakeMap), scoreBoard(other.scoreBoard), body(other.body), currentDirection(other.currentDirection) {}

// 복사 할당 연산자
Snake& Snake::operator=(const Snake& other) {
    if (this != &other) {
        body = other.body;
        currentDirection = other.currentDirection;
    }
    return *this;
}

void Snake::changeDirection(Direction newDirection) {
    if ((currentDirection == UP && newDirection != DOWN) ||
        (currentDirection == DOWN && newDirection != UP) ||
        (currentDirection == LEFT && newDirection != RIGHT) ||
        (currentDirection == RIGHT && newDirection != LEFT)) {
        currentDirection = newDirection;
    }
    else {
        gameOver = true;
    }
}

void Snake::move(Gate& gateManager) {
    auto head = body.front();
    int nextX = head.second;
    int nextY = head.first;

    switch (currentDirection) {
        case UP:
            nextY -= 1; // 한 칸 위로
            break;
        case DOWN:
            nextY += 1; // 한 칸 아래로
            break;
        case LEFT:
            nextX -= 1; // 한 칸 왼쪽으로
            break;
        case RIGHT:
            nextX += 1; // 한 칸 오른쪽으로
            break;
    }

    if (!isValidMove(nextX, nextY)) {
        gameOver = true;
        return; // Invalid move, do not proceed
    }

    int inFrontHead = whatIsInFrontOf(nextX, nextY);

    switch (inFrontHead) {
        case 5: // Growth item
            addBodySegment();
            snakeMap.setMap(nextY, nextX, 0);
            scoreBoard.addScore(20);
            scoreBoard.addGrowthEaten(1);
            scoreBoard.addBodyCurrentLength(1);
            if (scoreBoard.getBodyCurrentLength() > scoreBoard.getBodyLongestLength()) {
                scoreBoard.setBodyLongestLength(scoreBoard.getBodyCurrentLength());
            }
            break;
        case 6: // Poison item
            removeBodySegment();
            snakeMap.setMap(nextY, nextX, 0);
            scoreBoard.addScore(-10);
            scoreBoard.addPoisonEaten(1);
            scoreBoard.addBodyCurrentLength(-1);

            if(scoreBoard.getBodyCurrentLength() < 3)
                gameOver = true;
            break;
        case 7: // Gate
            scoreBoard.addGateEaten(1);
            handleGate(gateManager);
            return; // 게이트로 이동 후 추가 이동 불필요
    }

    body.push_front(std::make_pair(nextY, nextX));
    snakeMap.setMap(body.front().first, body.front().second, 3); // 머리 위치 업데이트
    snakeMap.setMap(body[1].first, body[1].second, 2); // 이전 머리를 몸통으로 업데이트

    auto tail = body.back();
    snakeMap.setMap(tail.first, tail.second, 0); // 꼬리 제거
    body.pop_back();
}

void Snake::handleGate(Gate& gateManager) {
    auto head = body.front();
    int gateIndex = (gateManager.getGateEntry(0) == head) ? 0 : 1;
    auto exitGate = gateManager.getGateExit(gateIndex);
    Direction exitDirection = gateManager.getExitDirection(exitGate, currentDirection, snakeMap);

    int nextX = exitGate.second;
    int nextY = exitGate.first;

    // 게이트 한 칸 앞에서 생성되도록 조정
    switch (exitDirection) {
        case UP:
            nextY -= 1; // 한 칸 위로
            break;
        case DOWN:
            nextY += 1; // 한 칸 아래로
            break;
        case LEFT:
            nextX -= 1; // 한 칸 왼쪽으로
            break;
        case RIGHT:
            nextX += 1; // 한 칸 오른쪽으로
            break;
    }

    // 기존 위치의 뱀을 지우지 않고 각 부분을 하나씩 이동
    for (size_t i = body.size(); i > 0; --i) {
        auto segment = body[i - 1];
        snakeMap.setMap(segment.first, segment.second, 0); // 기존 위치 지우기

        if (i == 1) {
            body[i - 1] = std::make_pair(nextY, nextX); // 새로운 머리 위치
        } else {
            body[i - 1] = body[i - 2]; // 몸통을 앞의 위치로 이동
        }

        snakeMap.setMap(body[i - 1].first, body[i - 1].second, (i == 1) ? 3 : 4); // 새로운 위치 설정
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 이동 속도 조절
        snakeMap.drawMap(3, 1);
    }

    currentDirection = exitDirection; // 새로운 방향 설정
}

void Snake::addBodySegment() {
    auto head = body.front();
    int headX = head.second;
    int headY = head.first;

    // Find the position to add the new body segment
    int newX = headX;
    int newY = headY;

    body.push_front(std::make_pair(newY, newX));
    snakeMap.setMap(body.front().first, body.front().second, 3); // 머리 위치 업데이트
    snakeMap.setMap(body[1].first, body[1].second, 4);
}

void Snake::removeBodySegment() {
    if (body.size() > 1) {
        auto tail = body.back();
        body.pop_back(); // 꼬리 제거
        snakeMap.setMap(tail.first, tail.second, 0); // 맵에서 꼬리 제거
    }
}

std::pair<int, int> Snake::getHeadPosition() const {
    return body.front();
}

const std::deque<std::pair<int, int>>& Snake::getBody() const {
    return body;
}

bool Snake::isGameOver() const
{
    return gameOver;
}

bool Snake::isValidMove(int nextX, int nextY) {
    int mapElement = snakeMap.getMap()[nextY][nextX];
    if (mapElement == 1 || mapElement == 2 || mapElement == 3 || mapElement == 4) {
        return false; // Collision with wall or snake body
    }
    return true;
}

int Snake::whatIsInFrontOf(int nextX, int nextY) {
    return snakeMap.getMap()[nextY][nextX];
}

bool Snake::isCollision(int x, int y) {
    int mapElement = snakeMap.getMap()[y][x];
    return (mapElement == 1 || mapElement == 2 || mapElement == 3 || mapElement == 4);
}
