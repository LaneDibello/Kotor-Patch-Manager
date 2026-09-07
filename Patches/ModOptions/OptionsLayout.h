#pragma once
#include "Common.h"
#include "GameAPI/CExoString.h"
#include "GameAPI/CResGFF.h"
#include "GameAPI/CResRef.h"
#include "GameAPI/CSWGuiControl.h"

// A layout GFF held open for a menu's lifetime, so rows can be loaded at any time.
// StopLoadFromLayout releases the panel's own GFF, so anything built after the panel
// finishes loading needs its own.
class OptionsLayout {
public:
	~OptionsLayout() { Close(); }

	bool Open(const char* resref) {
		Close();
		if (!resref) {
			return false;
		}

		CResRef layoutRef(resref);
		gff = new CResGFF(GUI, "GUI ", &layoutRef);
		if (!gff || !gff->GetPtr()) {
			Close();
			return false;
		}

		// Demand is what actually loads the resource; without it every lookup below
		// comes back empty.
		gff->Demand();

		if (!gff->GetTopLevelStruct(&root)) {
			debugLog("[ModOptions] layout `%s` has no top level struct", resref);
			Close();
			return false;
		}
		if (!gff->GetList(&controls, &root, const_cast<char*>("CONTROLS"))) {
			debugLog("[ModOptions] layout `%s` has no CONTROLS list", resref);
			Close();
			return false;
		}

		debugLog("[ModOptions] layout `%s` open, %i controls",
			resref, gff->GetListCount(&controls));
		return true;
	}

	void Close() {
		if (gff) {
			gff->Release();
			delete gff;
			gff = nullptr;
		}
	}

	bool IsOpen() const { return gff != nullptr; }

	// Loads `control` from the CONTROLS entry tagged `tag`. `owner` becomes the
	// control's gui object, which is the panel every mouse path resolves against.
	bool Load(CSWGuiControl* control, CSWGuiObject* owner, const char* tag) {
		if (!gff || !control || !tag) {
			return false;
		}

		CExoString tagString(const_cast<char*>(tag));
		control->LoadFromLayout(owner, gff, &controls, &tagString);

		// A tag no CONTROLS entry carries makes Load_2 a no-op, which would leave a
		// blank zero-sized control in the list. Every template has a real EXTENT, so
		// a height is the signal that the load actually happened.
		if (control->GetExtent().height <= 0) {
			debugLog("[ModOptions] layout has no control tagged `%s`", tag);
			return false;
		}
		return true;
	}

private:
	CResGFF* gff = nullptr;
	CResStruct root{};
	CResList controls{};
};
