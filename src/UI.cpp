#include "UI.h"

void UI::Register() {
    if (!SKSEMenuFramework::IsInstalled()) {
        return;
    }
    SKSEMenuFramework::SetSection(MOD_NAME);
    SKSEMenuFramework::AddSectionItem("Config", Main::Render);
}


void __stdcall UI::Main::Render() {
    ImGuiMCP::Text("Your Menu Goes Here");
}

