#pragma once
#include "Common.h"
#include "MemberFunctionThunk.h"

#include "GameAPI/CExoIni.h"
#include "GameAPI/CExoString.h"
#include "GameAPI/CResRef.h"
#include "GameAPI/CSWGuiBorder.h"
#include "GameAPI/CSWGuiBorderParams.h"
#include "GameAPI/CSWGuiButton.h"
#include "GameAPI/CSWGuiLabel.h"
#include "GameAPI/CSWGuiPanel.h"

#include "snake.h"

#include <cstdlib>
#include <vector>

#define INI "snake.ini"
#define CATEGORY "config"
#define WIDTH "Width"
#define HEIGHT "Height"
#define SPEED "Speed"

#define BLACK "blackfill"
#define BLUE "bluefill"
#define YELLOW "yellowfill"

class SnakePanel : public CSWGuiPanel {
public:
	CSWGuiLabel titleLabel;
	CSWGuiLabel gameLabel;
	CSWGuiButton backButton;

	std::vector<std::vector<CSWGuiBorder*>> grid;

	int cellSize;

	Snake snake;

	bool alive;

	SnakePanel(CSWGuiManager* manager) :
		CSWGuiPanel(manager),
		titleLabel(),
		gameLabel(),
		backButton(),
		grid(getHeight(), std::vector<CSWGuiBorder*>(getWidth(), new CSWGuiBorder())),
		alive(true)
	{
		ThunkRegistry::Register(this);

		CResRef guiResref("snake");
		this->StartLoadFromLayout(&guiResref);
		CExoString titleTag("LBL_TITLE");
		this->InitControl(&titleLabel, &titleTag, 1);
		CExoString gameTag("LBL_GAME");
		this->InitControl(&gameLabel, &gameTag, 1);
		CExoString backTag("BTN_BACK");
		this->InitControl(&backButton, &backTag, 1);
		this->StopLoadFromLayout();

		debugLog("Loaded Snake Panel from layout");

		snake = createSnake(getWidth(), getHeight());

		// cell math
		CSWGuiExtent gameSpace = gameLabel.GetExtent();
		int centerX = gameSpace.left + (gameSpace.width / 2);
		int centerY = gameSpace.top + (gameSpace.height / 2);
		int maxCellWidth = (int)floor((double)gameSpace.width / getWidth());
		int maxCellHeight = (int)floor((double)gameSpace.height / getHeight());

		cellSize = min(maxCellWidth, maxCellHeight);
		int gameWidth = getWidth() * cellSize;
		int gameHeight = getHeight() * cellSize;

		for (int i = 0; i < getHeight(); ++i) {
			for (int j = 0; j < getWidth(); ++j) {
				CSWGuiExtent cellExtent = {
					cellSize * j - (gameWidth / 2),
					cellSize * i - (gameHeight / 2),
					cellSize,
					cellSize
				};
				CSWGuiBorder* cell = grid.at(j).at(i);
				CSWGuiBorderParams* params = cell->GetBorderParams();
				CResRef image(BLACK);
				params->SetFillImage(&image, 0);
				cell->Initialize(&cellExtent, params);
			}
		}

		this->OverrideHandleInputEvent(memberFuncAddr(&SnakePanel::_HandleInputEvent));
		this->OverrideUpdate(memberFuncAddr(&SnakePanel::_Update));
		this->OverrideDraw(memberFuncAddr(&SnakePanel::_Draw));
	}

	~SnakePanel() {
		ThunkRegistry::Unregister(this);
		for (int i = 0; i < getHeight(); ++i) {
			for (int j = 0; j < getWidth(); ++j) {
				delete grid.at(j).at(i);
				grid.at(j).at(i) = nullptr;
			}
		}
	}

	static int getWidth() {
		if (_width > 0) {
			return _width;
		}

		CExoIni ini;
		CExoString width;
		CExoString file(INI);
		CExoString category(CATEGORY);
		CExoString key(WIDTH);
		if (!ini.ReadIniEntry(&width, &file, &category, &key)) {
			_width = 50;
			return _width;
		}

		_width = atoi(width.GetCStr());
		return _width;
	}

	static int getHeight() {
		if (_height > 0) {
			return _height;
		}

		CExoIni ini;
		CExoString height;
		CExoString file(INI);
		CExoString category(CATEGORY);
		CExoString key(HEIGHT);
		if (!ini.ReadIniEntry(&height, &file, &category, &key)) {
			_height = 30;
			return _height;
		}

		_height = atoi(height.GetCStr());
		return _height;
	}

	static int getSpeed() {
		if (_speed > 0) {
			return _speed;
		}

		CExoIni ini;
		CExoString speed;
		CExoString file(INI);
		CExoString category(CATEGORY);
		CExoString key(HEIGHT);
		if (!ini.ReadIniEntry(&speed, &file, &category, &key)) {
			_speed = 1;
			return _speed;
		}

		_speed = atoi(speed.GetCStr());
		return _speed;
	}

private:
	static inline int _width = -1;
	static inline int _height = -1;
	static inline int _speed = -1; // In Cells per second

	double _age = 0.0; // Time in seconds since last Update

	void _HandleInputEvent(int event, int inputPhase) {
		if (inputPhase) {
			switch (event) {
			case CSWGuiControl::UpArrow:
				setFacing(this->snake, UP);
				break;
			case CSWGuiControl::RightArrow:
				setFacing(this->snake, RIGHT);
				break;
			case CSWGuiControl::DownArrow:
				setFacing(this->snake, DOWN);
				break;
			case CSWGuiControl::LeftArrow:
				setFacing(this->snake, LEFT);
				break;
			default:
				break;
			}
		}

		HandleInputEvent(event, inputPhase);
	}

	void _Update(float deltaT) {
		if (!alive) {
			return;
		}

		_age += (double)deltaT;

		if (_age < (1.0 / _speed)) { // 1/speed ~ seconds/Cell
			return;
		}

		alive = takeStep(snake);
	}

	void drawCell(float alpha, int x, int y) {
		int cellState = getState(snake.grid, x, y);
		CSWGuiBorder* cell = grid.at(y).at(x);
		if (getFood(snake.grid, x, y)) {
			CResRef image(YELLOW);
			cell->GetBorderParams()->SetFillImage(&image, 1);
		}
		else if (cellState > 0) {
			CResRef image(BLUE);
			cell->GetBorderParams()->SetFillImage(&image, 1);
		}
		else {
			CResRef image(BLACK);
			cell->GetBorderParams()->SetFillImage(&image, 1);
		}
		cell->Draw(alpha);
	}

	void _Draw(float alpha) {
		for (int i = 0; i < getHeight(); ++i) {
			for (int j = 0; j < getWidth(); ++j) {
				drawCell(alpha, j, i);
			}
		}
		Draw(alpha);
	}
};