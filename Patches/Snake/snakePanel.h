#pragma once
#include "Common.h"
#include "MemberFunctionThunk.h"

#include "GameAPI/AurGUI.h"
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
		grid(getHeight(), std::vector<CSWGuiBorder*>(getWidth(), nullptr)),
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

		debugLog("[Snake] Loaded Snake Panel from layout");

		snake = createSnake(getWidth(), getHeight());

		debugLog("[Snake] Snake created with grid size: (%i, %i)", getWidth(), getHeight());

		for (int i = 0; i < getHeight(); ++i) {
			for (int j = 0; j < getWidth(); ++j) {
				grid.at(i).at(j) = new CSWGuiBorder();
			}
		}

		// cell math
		CSWGuiExtent gameSpace = gameLabel.GetExtent();
		debugLog("[Snake] gameSpace: {%i, %i, %i, %i}", gameSpace.left, gameSpace.top, gameSpace.width, gameSpace.height);

		int centerX = gameSpace.left + (gameSpace.width / 2);
		int centerY = gameSpace.top + (gameSpace.height / 2);
		int maxCellWidth = (int)floor((double)gameSpace.width / getWidth());
		int maxCellHeight = (int)floor((double)gameSpace.height / getHeight());

		cellSize = min(maxCellWidth, maxCellHeight);
		int gameWidth = getWidth() * cellSize;
		int gameHeight = getHeight() * cellSize;

		debugLog("[Snake] Cell Stats - Cell Size: %i - Center: (%i, %i) - Game Area: (%i, %i)", cellSize, centerX, centerY, gameWidth, gameHeight);

		int frameWidth = getFrameWidth();
		CSWGuiExtent frame = {
			centerX - (gameWidth / 2) - frameWidth,
			centerY - (gameHeight / 2) - frameWidth,
			gameWidth + frameWidth * 2,
			gameHeight + frameWidth * 2
		};
		gameLabel.LayoutExtent(&frame);
		debugLog("[Snake] Frame: {%i, %i, %i, %i} - Border width: %i", frame.left, frame.top, frame.width, frame.height, frameWidth);

		for (int i = 0; i < getHeight(); ++i) {
			for (int j = 0; j < getWidth(); ++j) {
				CSWGuiExtent cellExtent = {
					centerX + cellSize * j - (gameWidth / 2),
					centerY + cellSize * i - (gameHeight / 2),
					cellSize,
					cellSize
				};
				CSWGuiBorder* cell = grid.at(i).at(j);
				CSWGuiBorderParams* params = cell->GetBorderParams();
				CResRef image(BLACK);
				params->SetFillImage(&image, 0);
				params->SetFillStyle(2); // Stretch
				cell->Initialize(&cellExtent, params);
				delete params;
			}
		}

		debugLog("[Snake] Cells Initialized");

		this->OverrideHandleInputEvent(memberFuncAddr(&SnakePanel::_HandleInputEvent));
		this->OverrideUpdate(memberFuncAddr(&SnakePanel::_Update));
		this->OverrideDraw(memberFuncAddr(&SnakePanel::_Draw));

		gameLabel.SetActive(0);
		gameLabel.SetEnabled(0);

		debugLog("[Snake] Constructed");
		debugLog("[Snake] Speed: %i", getSpeed());
		debugLog("[Snake] Head at (%i, %i)", snake.headX, snake.headY);
	}

	~SnakePanel() {
		ThunkRegistry::Unregister(this);
		for (int i = 0; i < getHeight(); ++i) {
			for (int j = 0; j < getWidth(); ++j) {
				delete grid.at(i).at(j);
				grid.at(i).at(j) = nullptr;
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
		CExoString key(SPEED);
		if (!ini.ReadIniEntry(&speed, &file, &category, &key)) {
			_speed = 2;
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
				if (this->snake.facing == DOWN) break;
				setFacing(this->snake, UP);
				debugLog("[Snake] Up!");
				break;
			case CSWGuiControl::RightArrow:
				if (this->snake.facing == LEFT) break;
				setFacing(this->snake, RIGHT);
				debugLog("[Snake] Right!");
				break;
			case CSWGuiControl::DownArrow:
				if (this->snake.facing == UP) break;
				setFacing(this->snake, DOWN);
				debugLog("[Snake] Down!");
				break;
			case CSWGuiControl::LeftArrow:
				if (this->snake.facing == RIGHT) break;
				setFacing(this->snake, LEFT);
				debugLog("[Snake] Left!");
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
		_age = 0.0;
	}

	int getFrameWidth() {
		try {
			CSWGuiBorder labelBorder((char*)gameLabel.GetPtr() + GameVersion::GetOffset("CSWGuiLabel", "border"));
			CSWGuiBorderParams* labelParams = labelBorder.GetBorderParams();
			int dimension = labelParams ? labelParams->GetDimension() : 0;
			delete labelParams;
			if (dimension > 0) {
				return dimension;
			}

			typedef int(__thiscall* GetImageWidthFn)(void* thisPtr);
			GetImageWidthFn getImageWidth = nullptr;
			GameVersion::ResolveFunction(getImageWidth, "CAurGUIImageInternal", "GetImageWidth");
			void* cornerImage = getObjectProperty<void*>(labelBorder.GetPtr(), GameVersion::GetOffset("CSWGuiBorder", "corner_image"));
			return (cornerImage && getImageWidth) ? getImageWidth(cornerImage) : 0;
		}
		catch (const GameVersionException& e) {
			debugLog("[Snake] Frame width lookup failed: %s", e.what());
			return 0;
		}
	}

	void drawCell(float alpha, int x, int y) {
		int cellState = getState(snake.grid, x, y);
		CSWGuiBorder* cell = grid.at(y).at(x);
		CSWGuiBorderParams* params = cell->GetBorderParams();
		if (getFood(snake.grid, x, y)) {
			CResRef image(YELLOW);
			params->SetFillImage(&image, 0);
		}
		else if (cellState > 0) {
			CResRef image(BLUE);
			params->SetFillImage(&image, 0);
		}
		else {
			CResRef image(BLACK);
			params->SetFillImage(&image, 0);
		}
		delete params;
		cell->Draw(alpha);
	}

	void _Draw(float deltaT) {
		Draw(deltaT);

		CSWGuiExtent viewport = GetExtent();
		GetExtentAccountingForPanelOffset(&viewport);
		Vector noBackground = { -1.0f, -1.0f, -1.0f };
		AurGUI::StartLayer();
		if (AurGUI::SetupViewport(viewport.left, viewport.top, viewport.width, viewport.height, &noBackground, false, GetAlpha())) {
			for (int i = 0; i < getHeight(); ++i) {
				for (int j = 0; j < getWidth(); ++j) {
					drawCell(deltaT, j, i);
				}
			}
			AurGUI::CloseViewport();
		}
		AurGUI::StopLayer();
	}
};