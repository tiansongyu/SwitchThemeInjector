#include "SettingsPage.hpp"
#include "../ViewFunctions.hpp"
#include "../Platform/Platform.hpp"

using namespace std;

namespace Settings {
	bool UseIcons = true;
	bool UseCommon = true;
};

SettingsPage::SettingsPage() 
{
	Name = "设置";
}

void SettingsPage::Render(int X, int Y)
{
	Utils::ImGuiSetupWin(Name.c_str(), X, Y, DefaultWinFlags);
	ImGui::SetWindowSize(ImVec2(SCR_W - (float)X - 30, SCR_H - (float)Y - 70));
	ImGui::PushFont(font25);

	ImGui::PushFont(font30);
	ImGui::TextUnformatted("NX主题设置");
	ImGui::PopFont();
	ImGui::TextWrapped("这些设置只适用于安装nxthemes，不保存，每次启动这个应用程序时都必须切换回来");
	ImGui::Checkbox("启用自定义图标", &Settings::UseIcons);
	PAGE_RESET_FOCUS;
	ImGui::Checkbox("启用额外的布局（例如 common.szs）", &Settings::UseCommon);
	ImGui::NewLine();

	ImGui::PopFont();
	Utils::ImGuiCloseWin();
}

void SettingsPage::Update()
{	
	if (Utils::PageLeaveFocusInput(true))
	{
		Parent->PageLeaveFocus(this);
		return;
	}
}









