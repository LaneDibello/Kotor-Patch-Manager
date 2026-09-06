#pragma once
#include "Common.h"
#include "MemberFunctionThunk.h"
#include "ModOptionFunctions.h"
#include "ModOptionIni.h"
#include "ModOptionsConfig.h"
#include "OptionsEditBox.h"

#include "GameAPI/CExoArrayList.h"
#include "GameAPI/CExoString.h"
#include "GameAPI/CResRef.h"
#include "GameAPI/CSWGuiBorder.h"
#include "GameAPI/CSWGuiBorderParams.h"
#include "GameAPI/CSWGuiButton.h"
#include "GameAPI/CSWGuiButtonToggle.h"
#include "GameAPI/CSWGuiEditBox.h"
#include "GameAPI/CSWGuiEditText.h"
#include "GameAPI/CSWGuiExtent.h"
#include "GameAPI/CSWGuiLabel.h"
#include "GameAPI/CSWGuiListBox.h"
#include "GameAPI/CSWGuiManager.h"
#include "GameAPI/CSWGuiPanel.h"
#include "GameAPI/CSWGuiText.h"
#include "GameAPI/CSWGuiTextParams.h"

#include <cstdlib>
#include <string>
#include <vector>

class OptionsMenu : public CSWGuiPanel {
public:
	ModOptionsConfig config;

	CSWGuiManager* guiManager;

	CSWGuiLabel titleLabel;
	CSWGuiListBox optionsListBox;
	CSWGuiLabel descriptionLabel;
	CSWGuiListBox descriptionListBox;
	CSWGuiButton backButton;
	CSWGuiButton defaultButton;

	// Authoritative option state, indexed like config.options, in INI text form.
	std::vector<std::string> values;


	// Height of a stacked Text Row as a percentage of the provided extent
	static const int TEXT_ROW_HEIGHT_PERCENT = 170;

	// Wrappers for the Text options currently in optionsListBox
	std::vector<OptionsEditBox*> editBoxes;

	// Control the last suppressed SetActiveControl was for

	//Callbacks
	void onBack(void* control) {
		debugLog("[ModOptions] Back Button Pressed");
		releaseKeyboardFocus();
		_HandleInputEvent(CSWGuiControl::BButton, 1);
	}

	void setEditFocus(void* control) {
		releaseKeyboardFocus(control);

		CSWGuiEditBox editBox(control);
		editBox.SetFocus();
	}

	// Drops keyboard focus from any edit box other than `keep`
	void releaseKeyboardFocus(void* keep = nullptr) {
		bool released = false;
		for (OptionsEditBox* box : editBoxes) {
			if (box->IsFocused() && box->GetPtr() != keep) {
				box->ReleaseFocus();
				released = true;
			}
		}

		if (released && !keep) {
			SetActiveControl(&optionsListBox, 0);
		}
	}

	void _SetActiveControl(void* control, int playSound) {
		// HandleMouseMove drives this every mouse-move frame while an edit box is
		// focused. Letting it through would hand active_control back to the list box,
		// and HandleKeyPress only routes keys while active_control IS the focused box.
		if (focusedEditBox() && control == optionsListBox.GetPtr()) {
			return;
		}

		releaseKeyboardFocus(control);

		CSWGuiControl incoming(control);
		SetActiveControl(&incoming, playSound);
	}

	OptionsEditBox* focusedEditBox() {
		for (OptionsEditBox* box : editBoxes) {
			if (box->IsFocused()) {
				return box;
			}
		}
		return nullptr;
	}

	void onDefault(void* control) {
		debugLog("[ModOptions] Default Button Pressed");
		releaseKeyboardFocus();

		// Restore all options to their default states
		for (const ModOption& option : config.options) {
			if (option.HasIni()) {
				WriteOptionValue(option, option.defaultString);
			}
			if (option.HasFunction()) {
				InvokeModOptionHandler(option, option.defaultString);
			}
		}

		populateOptionsListBox();
	}

	void onOption(void* control) {
		releaseKeyboardFocus(control);
		commitOption(control);
	}

	// Reads a control's current state and writes it through to values/INI/handler.
	void commitOption(void* control) {
		CSWGuiControl option(control);
		size_t index = (size_t)option.GetCustomValue();
		if (index >= config.OptionCount() || index >= values.size()) {
			debugLog("[ModOptions] mod option button has out-of-range custom value %u", (unsigned)index);
			return;
		}

		const ModOption* opt = config.GetOption(index);
		if (!opt) {
			return;
		}

		// Get the state of the control
		std::string value = "";
		switch (opt->type) {
		case ModOptionType::Toggle: {
			const bool wasOn = (values[index] == "1");
			value = wasOn ? "0" : "1";
			CSWGuiButtonToggle toggle(control);
			toggle.SetSelected(wasOn ? 0 : 1);
			break;
		}
		case ModOptionType::Slider:
			// TODO
			return;
		case ModOptionType::List:
			// TODO
			return;
		case ModOptionType::Text: {
			CSWGuiEditBox editBox(control);
			CSWGuiEditText* editText = editBox.GetEditText();
			if (!editText) {
				return;
			}

			CSWGuiTextParams* params = editText->GetTextParams();
			CExoString* displayed = params ? params->GetText() : nullptr;
			value = displayed ? displayed->ToStdString() : std::string();

			delete displayed;
			delete params;
			delete editText;
			break;
		}
		}

		values[index] = value;

		// INI first, so a handler that re-reads its settings sees the new value.
		if (opt->HasIni()) {
			WriteOptionValue(*opt, value);
		}
		if (opt->HasFunction()) {
			InvokeModOptionHandler(*opt, value);
		}
	}

	void SetDescription(void* control) {
		CSWGuiControl hovered(control);

		size_t index = (size_t)hovered.GetCustomValue();
		const ModOption* opt = config.GetOption(index);
		if (!opt) {
			debugLog("[ModOptions] hovered option has out-of-range custom value %u", (unsigned)index);
			return;
		}

		SetControlText(&descriptionLabel, opt->GetDescription());

		descriptionListBox.ClearItems();

		CSWGuiText* labelText = descriptionLabel.GetText();
		if (labelText) {
			CSWGuiExtent labelExtent = descriptionListBox.GetExtent();
			labelExtent.height = labelText->GetIdealHeight();
			descriptionLabel.SetExtent(labelExtent);
			delete labelText;
		}

		// Only the CExoArrayList overload marshals elements to game pointers.
		CExoArrayList<CSWGuiControl*> descriptionItems;
		descriptionItems.Add(&descriptionLabel);
		descriptionListBox.AddControls(&descriptionItems, 0, 0, 0);
		descriptionListBox.SetActiveControl(&descriptionLabel, 0);
	}

	OptionsMenu(CSWGuiManager* manager, const ModOptionsConfig& menuConfig) :
		CSWGuiPanel(manager),
		config(menuConfig),
		guiManager(manager),
		titleLabel(),
		optionsListBox(),
		descriptionLabel(),
		descriptionListBox(),
		backButton(),
		defaultButton()
	{
		if (!config.loaded) {
			debugLog("[ModOptions] OptionsMenu built without a loaded config");
			return;
		}

		// Before any AddEvent: the thunks look us up by game pointer.
		ThunkRegistry::Register(this);

		CResRef guiResref("modoptionmenu");
		this->StartLoadFromLayout(&guiResref);
		CExoString titleTag("LBL_TITLE");
		this->InitControl(&titleLabel, &titleTag, 1);
		CExoString optionsTag("LB_OPTIONS");
		this->InitControl(&optionsListBox, &optionsTag, 1);
		CExoString descriptionTag("LB_DESC");
		this->InitControl(&descriptionListBox, &descriptionTag, 1);
		CExoString backTag("BTN_BACK");
		this->InitControl(&backButton, &backTag, 1);
		CExoString defaultTag("BTN_DEFAULT");
		this->InitControl(&defaultButton, &defaultTag, 1);
		this->StopLoadFromLayout();

		SetControlText(&titleLabel, config.GetName());

		//Description Logic
		CSWGuiControl* descProtoItem = descriptionListBox.GetProtoItem();
		if (descProtoItem) {
			CSWGuiLabel descProto(descProtoItem->GetPtr());
			CSWGuiExtent descExtent = descProto.GetExtent();
			descriptionLabel.Initialize(&descExtent, &descProto, 1.0f);
			delete descProtoItem;
		}

		populateOptionsListBox();

		defaultButton.AddEvent(CSWGuiControl::AButton, this,
			memberThunkAddr<OptionsMenu, &OptionsMenu::onDefault>());
		defaultButton.SetControlBitFlag(2, false);
		backButton.AddEvent(CSWGuiControl::AButton, this,
			memberThunkAddr<OptionsMenu, &OptionsMenu::onBack>());
		backButton.SetControlBitFlag(2, false);

		this->OverrideHandleInputEvent(memberFuncAddr(&OptionsMenu::_HandleInputEvent));
		this->OverrideSetActiveControl(memberFuncAddr(&OptionsMenu::_SetActiveControl));

		SetActiveControl(&optionsListBox, 0);
		CSWGuiControl* first = optionsListBox.GetControl(0);
		optionsListBox.SetActiveControl(first, 0);
	}

	~OptionsMenu() {
		releaseEditBoxes();
		ThunkRegistry::Unregister(this);
	}

private:
	// Tears down the Text-option wrappers.
	void releaseEditBoxes() {
		// Keyboard mode is global; drop it before the boxes holding it go away.
		releaseKeyboardFocus();

		for (OptionsEditBox* box : editBoxes) {
			box->ReleaseOwnership();
			delete box;
		}
		editBoxes.clear();
	}

	// Reads a caller-owned CResRef wrapper into a string and disposes of both.
	static std::string resRefText(CResRef* ref) {
		std::string out;
		if (!ref) {
			return out;
		}
		char* text = ref->GetCStr();
		if (text) {
			out = text;
			free(text);
		}
		delete ref;
		return out;
	}

	// Swaps a border params object's three images and remembers the originals, so a
	// borrowed proto-item params block can be put back exactly as it was.
	//
	// The proto item's params are shared with every row, and
	// CSWGuiBorder::Initialize copies whatever it is handed into the border -- so
	// without the restore, every toggle built after an edit box inherits the edit
	// box's art (which is also why the highlight border was drawing the checkbox
	// circle: it was still carrying the toggle's fill image).
	struct BorrowedBorderImages {
		BorrowedBorderImages(CSWGuiBorderParams* params, const char* corner,
			const char* edge, const char* fill) : params(params)
		{
			if (!params) {
				return;
			}
			savedCorner = resRefText(params->GetCornerImageResRef());
			savedEdge   = resRefText(params->GetEdgeImageResRef());
			savedFill   = resRefText(params->GetFillImageResRef());
			apply(corner, edge, fill);
		}

		~BorrowedBorderImages() {
			if (params) {
				apply(savedCorner.c_str(), savedEdge.c_str(), savedFill.c_str());
			}
		}

		void apply(const char* corner, const char* edge, const char* fill) {
			CResRef cornerRef(corner);
			CResRef edgeRef(edge);
			CResRef fillRef(fill);
			params->SetCornerImage(&cornerRef, 1);
			params->SetEdgeImage(&edgeRef, 1);
			params->SetFillImage(&fillRef, 1);
		}

		CSWGuiBorderParams* params;
		std::string savedCorner, savedEdge, savedFill;
	};

	// Builds one Text row: thin blue frame at rest, brighter frame when hovered or
	// focused, and the same flat fill for both so neither picks up the toggle art.
	void initializeEditBox(OptionsEditBox* editBox, CSWGuiExtent* extent,
		CSWGuiTextParams* textParams, CSWGuiBorderParams* borderParams,
		CSWGuiBorderParams* hilightParams, const std::string& name,
		const std::string& value)
	{
		{
			BorrowedBorderImages frame(borderParams, "border2", "border1", "dialog3");
			BorrowedBorderImages hilightFrame(hilightParams, "border4", "border3", "dialog3");

			editBox->Initialize(extent, textParams, borderParams, hilightParams, name);
		}

		CSWGuiEditText* editText = editBox->GetEditText();
		if (editText) {
			CExoString textValue(const_cast<char*>(value.c_str()));
			editText->SetText(&textValue);
			delete editText;
		}
	}

	// GetText/GetTextParams hand back caller-owned wrappers.
	template <typename ControlT>
	void SetControlText(ControlT* control, const std::string& text) {
		if (!control) {
			return;
		}

		CSWGuiText* textObject = control->GetText();
		if (!textObject) {
			return;
		}

		CSWGuiTextParams* textParams = textObject->GetTextParams();
		if (textParams) {
			CExoString value(const_cast<char*>(text.c_str()));
			textParams->SetText(&value);
			delete textParams;
		}
		delete textObject;
	}

	void populateOptionsListBox() {
		if (!config.loaded) {
			return;
		}

		// Before ClearItems: each wrapper's destructor writes the game's vtable back
		// into its game object, which ClearItems is about to destroy.
		releaseEditBoxes();

		optionsListBox.ClearItems();
		values.assign(config.OptionCount(), std::string());

		CSWGuiControl* protoItem = optionsListBox.GetProtoItem();
		if (!protoItem) {
			debugLog("[ModOptions] LB_OPTIONS has no proto item");
			return;
		}

		// Will likely make this more generic in the future...
		CSWGuiButtonToggle proto(protoItem->GetPtr());
		delete protoItem;

		// Caller-owned wrappers, and the same for every option, so hoisted out.
		CSWGuiText* protoText = proto.GetText();
		CSWGuiBorder* protoBorder = proto.GetBorder();
		CSWGuiBorder* protoHilight = proto.GetHilight();
		CSWGuiBorder* protoSelectedBorder = proto.GetSelectedBorder();
		CSWGuiBorder* protoHilightSelected = proto.GetHilightSelectedBorder();
		CSWGuiTextParams* textParams = protoText ? protoText->GetTextParams() : nullptr;
		CSWGuiBorderParams* borderParams = protoBorder ? protoBorder->GetBorderParams() : nullptr;
		CSWGuiBorderParams* hilightParams = protoHilight ? protoHilight->GetBorderParams() : nullptr;
		CSWGuiBorderParams* selectedParams = protoSelectedBorder ? protoSelectedBorder->GetBorderParams() : nullptr;
		CSWGuiBorderParams* hilightSelectedParams = protoHilightSelected ? protoHilightSelected->GetBorderParams() : nullptr;

		CSWGuiExtent optionExtent;
		optionExtent.top = 0;
		optionExtent.left = 0;
		optionExtent.width = optionsListBox.GetViewportWidth() - 2 * optionsListBox.GetPadding();
		optionExtent.height = proto.GetExtent().height;

		CExoArrayList<CSWGuiControl*> listOptions;
		const std::vector<ModOption>& options = config.GetOptions();
		for (size_t i = 0; i < options.size(); ++i) {
			const std::string value = ResolveOptionValue(options[i]);
			values[i] = value;

			switch (options[i].type) {
			case ModOptionType::Toggle: {
				CSWGuiButtonToggle* toggle = new CSWGuiButtonToggle();
				toggle->SetOptionsCheckbox();

				toggle->Initialize(&optionExtent, textParams,
					borderParams, hilightParams,
					selectedParams, hilightSelectedParams);
				toggle->SetToggleEvent((CSWGuiControl::GuiEvent)-1);
				toggle->SetSelected((value == "1") ? 1 : 0);
				SetControlText(toggle, options[i].name);

				toggle->AddEvent(CSWGuiControl::AButton, this,
					memberThunkAddr<OptionsMenu, &OptionsMenu::onOption>());
				toggle->AddEvent(CSWGuiControl::HoverEnter, this,
					memberThunkAddr<OptionsMenu, &OptionsMenu::SetDescription>());

				toggle->SetCustomValue((DWORD)i);

				listOptions.Add(toggle);
				break;
			}
			case ModOptionType::Slider:
				// TODO
				break;
			case ModOptionType::List:
				// TODO
				break;
			case ModOptionType::Text: {
				OptionsEditBox* editBox = new OptionsEditBox(this, this);

				// A stacked row needs two lines: the name above the field.
				CSWGuiExtent textExtent = optionExtent;
				textExtent.height = optionExtent.height * TEXT_ROW_HEIGHT_PERCENT / 100;

				initializeEditBox(editBox, &textExtent, textParams, borderParams,
					hilightParams, options[i].name, value);

				editBox->AddEvent(CSWGuiControl::AButton, this,
					memberThunkAddr<OptionsMenu, &OptionsMenu::setEditFocus>());
				editBox->AddEvent(CSWGuiControl::HoverEnter, this,
					memberThunkAddr<OptionsMenu, &OptionsMenu::SetDescription>());

				editBox->SetCustomValue(DWORD(i));

				editBoxes.push_back(editBox);
				listOptions.Add(editBox);
				break;
			}
			}
		}

		delete hilightParams;
		delete borderParams;
		delete textParams;
		delete protoHilight;
		delete protoBorder;
		delete protoText;

		if (listOptions.GetSize() == 0) {
			debugLog("[ModOptions] `%s` produced no usable controls", config.GetName().c_str());
			return;
		}

		debugLog("[ModOptions] `%s` produced %i options", config.GetName().c_str(), listOptions.GetSize());

		// varyItemHeights only matters once Text rows stop matching the toggle height;
		// while they match, the uniform path is the better-trodden one.
		optionsListBox.AddControls(&listOptions, 1, 0,
			TEXT_ROW_HEIGHT_PERCENT != 100 ? 1 : 0);
	}

	void _HandleInputEvent(int event, int doPanelEvents) {
		debugLog("[ModOptions] OptionsMenu _HandleInputEvent (%i, %i)", event, doPanelEvents);
		if (doPanelEvents && guiManager) {
			switch (event) {
			case CSWGuiControl::BButton:
			{
				releaseKeyboardFocus();
				guiManager->PlayGuiSound(0);
				guiManager->PopModalPanel();
				// TODO: properly label these bit flags
				SetBitFlags((GetBitFlags() & ~0x300) | 0x400);
				break;
			}
			default:
				break;
			}
		}

		HandleInputEvent(event, doPanelEvents);
	}
};

// Defined here rather than in OptionsEditBox.h: the edit box only forward-declares
// OptionsMenu, and both headers land in the same translation unit.
inline void OptionsEditBox::CommitToMenu() {
	if (menu) {
		menu->commitOption(GetPtr());
	}
}
