#pragma once
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>


enum dir {
	STOPPED,
	UP,
	RIGHT,
	DOWN,
	LEFT
};

struct Cell {
	int state = 0;
	bool food = false;
	dir tail = STOPPED;
};

typedef std::vector<std::vector<Cell>> Grid;

struct Snake {
	int headX;
	int headY;

	dir facing;

	int length;

	Grid grid;
};

// Grid
Grid& createGrid(int width, int height) {
	static Grid grid(height, std::vector<Cell>(width));
	return grid;
}

void debugPrintGrid(Grid& grid) {
	for (std::vector<Cell> vec : grid) {
		for (Cell c : vec) {
			std::cout << c.state;
		}
		std::cout << std::endl;
	}

	std::cout << std::endl;

	for (std::vector<Cell> vec : grid) {
		for (Cell c : vec) {
			switch (c.tail) {
			case UP:
				std::cout << '^';
				break;
			case RIGHT:
				std::cout << '>';
				break;
			case DOWN:
				std::cout << 'V';
				break;
			case LEFT:
				std::cout << '<';
				break;
			case STOPPED:
			default:
				std::cout << '0';
				break;
			}
		}
		std::cout << std::endl;
	}
}

int getState(Grid& grid, int x, int y) {
	try {
		return grid.at(y).at(x).state;
	}
	catch (const std::out_of_range&) {
		return -1;
	}
}

dir getTail(Grid& grid, int x, int y) {
	try {
		return grid.at(y).at(x).tail;
	}
	catch (const std::out_of_range&) {
		return STOPPED;
	}
}

bool getFood(Grid& grid, int x, int y) {
	try {
		return grid.at(y).at(x).food;
	}
	catch (const std::out_of_range&) {
		return false;
	}
}

void setState(Grid& grid, int state, int x, int y) {
	grid.at(y).at(x).state = state;
}

void setTail(Grid& grid, dir tail, int x, int y) {
	grid.at(y).at(x).tail = tail;
}

void setFood(Grid& grid, bool food, int x, int y) {
	grid.at(y).at(x).food = food;
}

void resolveTail(Grid& grid, int x, int y) {
	int state = getState(grid, x, y);
	if (state == 0 || state == -1) return;
	
	if (state == 1) {
		setTail(grid, STOPPED, x, y);
	}
	setState(grid, state - 1, x, y);
	
	switch (getTail(grid, x, y)) {
	case UP:
		resolveTail(grid, x, y - 1);
		break;
	case RIGHT:
		resolveTail(grid, x + 1, y);
		break;
	case DOWN:
		resolveTail(grid, x, y + 1);
		break;
	case LEFT:
		resolveTail(grid, x - 1, y);
		break;
	case STOPPED:
	default:
		break;
	}
}

// Food
void createFood(Grid& grid) {
	// Collect Cells
	std::vector<std::pair<int, int>> emptyCells;
	for (int i = 0; i < grid.size(); ++i) {
		for (int j = 0; j < grid.at(i).size(); ++j) {
			if (getState(grid, j, i) == 0) emptyCells.push_back(std::make_pair(j, i));
		}
	}

	// Set Random food
	if (!emptyCells.size()) return;
	int index = rand() % emptyCells.size();
	std::pair<int, int> coord = emptyCells.at(index);
	setFood(grid, true, coord.first, coord.second);
}

// Snake
Snake& createSnake(int gridWidth, int gridHeight) {
	static struct Snake s;
	s.facing = RIGHT;
	s.headX = gridWidth / 2;
	s.headY = gridHeight / 2;
	s.length = 3;
	s.grid = createGrid(gridWidth, gridHeight);

	setState(s.grid, s.length, s.headX, s.headY);
	setFood(s.grid, true, s.headX + s.length, s.headY);
	return s;
}

void setFacing(Snake& snake, dir facing) {
	snake.facing = facing;

}

bool takeStep(Snake& snake) { // returns false if the snake dies
	int nextX, nextY;
	dir tail;
	switch (snake.facing) {
	case UP:
		nextX = snake.headX;
		nextY = snake.headY - 1;
		tail = DOWN;
		debugLog("[Snake] Stepping Up!");
		break;
	case RIGHT:
		nextX = snake.headX + 1;
		nextY = snake.headY;
		tail = LEFT;
		debugLog("[Snake] Stepping Right!");
		break;
	case DOWN:
		nextX = snake.headX;
		nextY = snake.headY + 1;
		tail = UP;
		debugLog("[Snake] Stepping Down!");
		break;
	case LEFT:
		nextX = snake.headX - 1;
		nextY = snake.headY;
		tail = RIGHT;
		debugLog("[Snake] Stepping Left!");
		break;
	case STOPPED:
	default:
		return true;
	}

	int nextState = getState(snake.grid, nextX, nextY);
	if (nextState != 0) {
		return false; // If next cell is occupied, the snake dies
	}

	int eatFood = getFood(snake.grid, nextX, nextY);
	if (eatFood) {
		snake.length += 3;
		setFood(snake.grid, false, nextX, nextY);
		createFood(snake.grid);
	}

	setState(snake.grid, snake.length, snake.headX, snake.headY);
	resolveTail(snake.grid, snake.headX, snake.headY);
	snake.headX = nextX;
	snake.headY = nextY;
	setState(snake.grid, snake.length, snake.headX, snake.headY);
	setTail(snake.grid, tail, snake.headX, snake.headY);

	return true;
}