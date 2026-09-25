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

#define BLACK "blackfill"
#define BLUE "bluefill"
#define YELLOW "yellowfill"

class SnakePanel : public CSWGuiPanel {
public:
	CSWGuiLabel titleLabel;
	CSWGuiLabel gameLabel;
	CSWGuiButton backButton;

	std::vector<std::vector<CSWGuiBorder>> grid;

	int cellSize;

	Snake snake;

	SnakePanel(CSWGuiManager* manager) :
		CSWGuiPanel(manager),
		titleLabel(),
		gameLabel(),
		backButton(),
		grid(getHeight(), std::vector<CSWGuiBorder>(getWidth(), CSWGuiBorder()))
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
			for (int j = 0; j < getWidth(); ++j)
			{
				CSWGuiExtent cellExtent = {
					cellSize * j - (gameWidth / 2),
					cellSize * i - (gameHeight / 2),
					cellSize,
					cellSize
				};
				CSWGuiBorder cell = grid.at(j).at(i);
				CSWGuiBorderParams* params = cell.GetBorderParams();
				CResRef image(BLACK);
				params->SetFillImage(&image, 0);
				cell.Initialize(&cellExtent, params);
			}
		}

		this->OverrideHandleInputEvent(memberFuncAddr(&SnakePanel::_HandleInputEvent));
		this->OverrideUpdate(memberFuncAddr(&SnakePanel::_Update));
	}

	~SnakePanel() {
		ThunkRegistry::Unregister(this);
	}

	static int getWidth() {
		CExoIni ini;
		CExoString width;
		CExoString file(INI);
		CExoString category(CATEGORY);
		CExoString key(WIDTH);
		if (!ini.ReadIniEntry(&width, &file, &category, &key)) {
			return 50;
		}
		return atoi(width.GetCStr());
	}

	static int getHeight() {
		CExoIni ini;
		CExoString height;
		CExoString file(INI);
		CExoString category(CATEGORY);
		CExoString key(HEIGHT);
		if (!ini.ReadIniEntry(&height, &file, &category, &key)) {
			return 30;
		}
		return atoi(height.GetCStr());
	}

private:
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

	void _Update(float param1) {
		debugLog("Called Update with %.2f", param1);

		// Somewhere in here we'll `takeStep`
	}
};