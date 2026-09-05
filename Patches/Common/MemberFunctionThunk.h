#pragma once
#include "Common.h"
#include "GameAPI/GameAPIObject.h"

#include <map>

// CSWGuiControl::AddEvent (and anything else that marshals a wrapper down to
// GetPtr()) calls back with the game pointer in ECX, not the wrapper, so a plain
// memberFuncAddr member would read its fields out of game memory. Register the
// wrapper and use memberThunkAddr instead.
//
// USAGE:
//   ThunkRegistry::Register(this);                                 // ctor
//   button.AddEvent(evt, this, memberThunkAddr<MyPanel, &MyPanel::OnClick>());
//   ThunkRegistry::Unregister(this);                               // dtor

namespace ThunkRegistry {

	inline std::map<void*, GameAPIObject*>& Objects() {
		static std::map<void*, GameAPIObject*> objects;
		return objects;
	}

	inline void Register(GameAPIObject* object) {
		if (object && object->GetPtr()) {
			Objects()[object->GetPtr()] = object;
		}
	}

	inline void Unregister(GameAPIObject* object) {
		if (object && object->GetPtr()) {
			Objects().erase(object->GetPtr());
		}
	}

	inline GameAPIObject* Lookup(void* gameObject) {
		auto found = Objects().find(gameObject);
		return (found == Objects().end()) ? nullptr : found->second;
	}

}

// __fastcall stands in for the game's __thiscall: object in ECX, one stack argument.
template <typename T, void (T::* Handler)(void*)>
void __fastcall MemberFunctionThunk(void* gameObject, void* /*edx*/, void* param) {
	GameAPIObject* object = ThunkRegistry::Lookup(gameObject);
	if (!object) {
		debugLog("[ThunkRegistry] no wrapper registered for %X", gameObject);
		return;
	}
	(static_cast<T*>(object)->*Handler)(param);
}

template <typename T, void (T::* Handler)(void*)>
inline void* memberThunkAddr() {
	return funcAddr(&MemberFunctionThunk<T, Handler>);
}
