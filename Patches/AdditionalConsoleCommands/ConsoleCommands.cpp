#include "Common.h"
#include "GameAPI/GameVersion.h"
#include "GameAPI/ConsoleFunc.h"
#include "GameAPI/CExoString.h"
#include "GameAPI/CVirtualMachine.h"
#include "GameAPI/CServerExoApp.h"
#include "GameAPI/CClientExoApp.h"
#include "GameAPI/CClientOptions.h"
#include "GameAPI/CSWSCreature.h"
#include "GameAPI/CSWSCreatureStats.h"
#include "GameAPI/CSWSObject.h"
#include "PathCasing.h"

#include <filesystem>
#include <set>
#include <string>

#ifndef TOML_EXCEPTIONS
#define TOML_EXCEPTIONS 0
#endif
#ifndef TOML_ENABLE_FORMATTERS
#define TOML_ENABLE_FORMATTERS 0
#endif
#include "External/toml.hpp"

void __cdecl runscript(char* script) {
    CExoString scriptFile(script);

    CServerExoApp* server = CServerExoApp::GetInstance();
    if (!server) return;

    DWORD playerId = server->GetPlayerCreatureId();

    CVirtualMachine* vm = CVirtualMachine::GetInstance();
    if (vm) {
        vm->RunScript(&scriptFile, playerId, 1);
        delete vm;
    }

    delete server;
}

void __cdecl teleport(char* location) {
    // Location formatted like "x y"
    int takeStraightLine = 1;

    CServerExoApp* server = CServerExoApp::GetInstance();
    if (!server) {
        return;
    }

    DWORD playerId = server->GetPlayerCreatureId();
    CSWSCreature* serverPlayer = server->GetCreatureByGameObjectID(playerId);
    if (!serverPlayer) {
        delete server;
        return;
    }

    Vector position = serverPlayer->GetPosition();
    Vector orientation = serverPlayer->GetOrientation();
    DWORD areaId = serverPlayer->GetAreaId();

    float x = position.x;
    float y = position.y;
    sscanf_s(location, "%f %f", &x, &y);

    debugLog("[teleport] serverPlayer pointer is %p", serverPlayer->GetPtr());

    int action = 0x41a00000;
    serverPlayer->AddActionToFront(5, 0xffff, 2, &x, 2, &y, 2, &position.z, 3, &areaId, 1, &takeStraightLine, 2, (void *)&action, 2, &orientation.x, 2, &orientation.y, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL);

    debugLog("[teleport] Done");

    delete serverPlayer;
    delete server;
}

static int* GetRenderPointer(const char* pointerName) {
    void* ptr = GameVersion::GetGlobalPointer(pointerName);
    if (!ptr) {
        debugLog("[ConsoleCommands] ERROR: Failed to get pointer for %s", pointerName);
        return nullptr;
    }
    return static_cast<int*>(ptr);
}

void __cdecl walkmeshrender() {
    static int* renderAABB = GetRenderPointer("RENDER_AABB");
    if (renderAABB) {
        *renderAABB = (*renderAABB) ^ 1;
    }
}

void __cdecl guirender() {
    static int* renderGUI = GetRenderPointer("RENDER_GUI");
    if (renderGUI) {
        *renderGUI = (*renderGUI) ^ 1;
    }
}

void __cdecl wireframerender() {
    static int* renderWireframe = GetRenderPointer("RENDER_WIREFRAME");
    if (renderWireframe) {
        *renderWireframe = (*renderWireframe) ^ 1;
    }
}

void __cdecl triggersrender() {
    static int* renderQATriggers = GetRenderPointer("RENDER_QA_TRIGGERS");
    static int* renderTriggers = GetRenderPointer("RENDER_TRIGGERS");
    if (renderQATriggers) {
        *renderQATriggers = (*renderQATriggers) ^ 1;
    }
    if (renderTriggers) {
        *renderTriggers = (*renderTriggers) ^ 1;
    }
}

void __cdecl personalspacerender() {
    static int* renderPersonalSpace = GetRenderPointer("RENDER_PERSONAL_SPACE");
    if (renderPersonalSpace) {
        *renderPersonalSpace = (*renderPersonalSpace) ^ 1;
    }
}

void __cdecl boundingboxesrender() {
    static int* renderGobBBs = GetRenderPointer("RENDER_GOB_BBS");
    if (renderGobBBs) {
        *renderGobBBs = (*renderGobBBs) ^ 1;
    }
}

void __cdecl freecam() {
    CClientExoApp* client = CClientExoApp::GetInstance();
    if (!client) {
        return;
    }

    CClientOptions* options = client->GetClientOptions();
    if (!options) {
        return;
    }

    options->SetCameraMode(7); // Mode 7 is freecam
}

void __cdecl addfeat(int feat) {
    CServerExoApp* server = CServerExoApp::GetInstance();
    if (!server) {
        return;
    }
    
    CSWSCreature* serverCreature = server->GetPlayerCreature();
    if (!serverCreature) {
        delete server;
        return;
    }

    CSWSCreatureStats* stats = serverCreature->GetCreatureStats();
    if (!stats) {
        delete serverCreature;
        delete server;
        return;
    }

    stats->AddFeat((WORD)feat);

    delete serverCreature;
    delete server;
}

static bool ParseCommandType(const std::string& text, funcTypes& outType) {
    if (text == "none") { outType = NO_PARAMS; return true; }
    if (text == "int") { outType = INT_PARAM; return true; }
    if (text == "string") { outType = STRING_PARAM; return true; }
    return false;
}

// Registers each [[commands]] entry found in <game>\commands\*.toml. The owning patch
// DLL is already loaded by now, as <manifest id>.dll. Names already in `taken` are skipped.
static void RegisterCommandsFromToml(std::set<std::string>& taken) {
    std::filesystem::path directory;
    if (!PathCasing::FindDirectory(".", "commands", directory)) {
        debugLog("[ConsoleCommands] no `commands` directory beside the game");
        return;
    }

    std::error_code ec;
    std::filesystem::directory_iterator entries(directory, ec);
    if (ec) {
        debugLog("[ConsoleCommands] cannot read `%s`: %s", directory.string().c_str(), ec.message().c_str());
        return;
    }

    for (const auto& entry : entries) {
        if (!entry.is_regular_file() || !PathCasing::HasExtension(entry.path(), ".toml")) {
            continue;
        }

        const std::string sourceName = entry.path().string();
        toml::parse_result result = toml::parse_file(sourceName);
        if (!result) {
            debugLog("[ConsoleCommands] %s: TOML parse error: %s", sourceName.c_str(),
                std::string(result.error().description()).c_str());
            continue;
        }

        const toml::array* commands = result.table()["commands"].as_array();
        if (!commands) {
            debugLog("[ConsoleCommands] %s: no [[commands]] entries found", sourceName.c_str());
            continue;
        }

        for (size_t i = 0; i < commands->size(); ++i) {
            const toml::table* command = commands->get(i)->as_table();
            if (!command) {
                debugLog("[ConsoleCommands] %s: command %u is not a table; skipping", sourceName.c_str(), (unsigned)i);
                continue;
            }

            std::string name = (*command)["name"].value_or<std::string>("");
            std::string patch = (*command)["patch"].value_or<std::string>("");
            std::string function = (*command)["function"].value_or<std::string>("");
            std::string typeText = (*command)["type"].value_or<std::string>("");
            if (name.empty() || patch.empty() || function.empty() || typeText.empty()) {
                debugLog("[ConsoleCommands] %s: command %u needs `name`, `patch`, `function` and `type`; skipping",
                    sourceName.c_str(), (unsigned)i);
                continue;
            }

            funcTypes type;
            if (!ParseCommandType(typeText, type)) {
                debugLog("[ConsoleCommands] %s: command `%s` has unknown type `%s`; skipping",
                    sourceName.c_str(), name.c_str(), typeText.c_str());
                continue;
            }

            // ConsoleFunc holds the name in a char[80]
            if (name.size() >= 80) {
                debugLog("[ConsoleCommands] %s: command `%s` is 80 characters or longer; skipping",
                    sourceName.c_str(), name.c_str());
                continue;
            }

            if (taken.count(name)) {
                debugLog("[ConsoleCommands] %s: command `%s` is already registered; skipping",
                    sourceName.c_str(), name.c_str());
                continue;
            }

            const std::string moduleName = patch + ".dll";
            HMODULE module = GetModuleHandleA(moduleName.c_str());
            if (!module) {
                debugLog("[ConsoleCommands] %s: `%s` is not loaded, cannot register `%s`",
                    sourceName.c_str(), moduleName.c_str(), name.c_str());
                continue;
            }

            void* handler = (void*)GetProcAddress(module, function.c_str());
            if (!handler) {
                debugLog("[ConsoleCommands] %s: `%s` does not export `%s`",
                    sourceName.c_str(), moduleName.c_str(), function.c_str());
                continue;
            }

            new ConsoleFunc(name.c_str(), handler, type);
            taken.insert(name);
            debugLog("[ConsoleCommands] registered `%s` from %s", name.c_str(), moduleName.c_str());
        }
    }
}

extern "C" void __cdecl InitializeAdditionalCommands()
{
    new ConsoleFunc("runscript", (void*)&runscript, STRING_PARAM);
    new ConsoleFunc("teleport", (void*)&teleport, STRING_PARAM);
    new ConsoleFunc("walkmeshrender", (void*)&walkmeshrender, NO_PARAMS);
    new ConsoleFunc("guirender", (void*)&guirender, NO_PARAMS);
    new ConsoleFunc("wireframerender", (void*)&wireframerender, NO_PARAMS);
    new ConsoleFunc("triggersrender", (void*)&triggersrender, NO_PARAMS);
    new ConsoleFunc("personalspacerender", (void*)&personalspacerender, NO_PARAMS);
    new ConsoleFunc("boundingboxesrender", (void*)&boundingboxesrender, NO_PARAMS);
    new ConsoleFunc("freecam", (void*)&freecam, NO_PARAMS);
    new ConsoleFunc("addfeat", (void*)&addfeat, INT_PARAM);

    // Built-ins win any name clash with a TOML-registered command
    std::set<std::string> taken = {
        "runscript", "teleport", "walkmeshrender", "guirender", "wireframerender",
        "triggersrender", "personalspacerender", "boundingboxesrender", "freecam", "addfeat",
    };
    RegisterCommandsFromToml(taken);

    // Note we never free these values, as they're present up until the game closes anyway, so 
    // memory is of minimal practical concern. May consider hooking an additional function to free
    // these in the future
}

// DLL Entry Point
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        // Initialize GameVersion system (reads from KOTOR_VERSION_SHA env var and addresses.toml)
        if (!GameVersion::Initialize()) {
            OutputDebugStringA("[AdditionalConsoleCommands] ERROR: GameVersion::Initialize() failed\n");
            return FALSE;
        }
        OutputDebugStringA("[AdditionalConsoleCommands] GameVersion initialized successfully\n");
        break;

    case DLL_PROCESS_DETACH:
        GameVersion::Reset();
        break;
    }
    return TRUE;
}