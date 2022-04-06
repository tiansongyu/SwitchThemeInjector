#include "CfwSelectPage.hpp"
#include "../ViewFunctions.hpp"
#include "../UI/UIManagement.hpp"

using namespace std;

CfwSelectPage::CfwSelectPage(const vector<string>& folders) : Folders(folders) {}

CfwSelectPage::~CfwSelectPage()
{

}

static const int BtnWidth = 500;
static const int XCursorBtn = SCR_W / 2 - BtnWidth / 2;

void CfwSelectPage::Render(int X, int Y)
{
	Utils::ImGuiSetupPageFullscreen("选择页面", 10, 10);
	ImGui::SetWindowFocus();

	if (Folders.size() == 0)
	{
		ImGui::PushFont(font30);
		Utils::ImGuiCenterString("找不到任何 cfw 文件夹.");
		ImGui::PopFont();
		ImGui::NewLine();
		ImGui::TextWrapped(
			"确保您的 SD 卡根目录中有 \"atmosphere\"、\"reinx\" 或 \"sxos\" 文件夹。\n\n"
			"某些 cfw 不会自动创建此文件夹，因此您应该手动创建。\n"
			"如果您确实有 cfw 文件夹但仍然看到此屏幕，请确保它写入正确，没有空格和全部小写。");
		
	}	
	else {
		Utils::ImGuiCenterString("检测到多个 cfw 文件夹，您想使用哪个？");

		ImGui::PushFont(font30);
		ImGui::SetCursorPos({ (float)XCursorBtn, ImGui::GetCursorPosY() + 30 });

		int count = 0;
		for (const auto& e : Folders)
		{
			ImGui::SetCursorPosX((float)XCursorBtn);
			if (ImGui::Button(e.c_str(), { BtnWidth, 50 }))
			{
				fs::cfw::SetFolder(e);
				PopPage(this);
			}
			count++;
		}

		ImGui::PopFont();
	}

	ImGui::NewLine();
	Utils::ImGuiCenterString("如果您的 cfw 不受支持，请在 Github 上提出问题。");
	if (Utils::ImGuiCenterButton("关闭此应用程序"))
		App::Quit();

	Utils::ImGuiSetWindowScrollable();
	Utils::ImGuiCloseWin();
}

void CfwSelectPage::Update()
{
	if (gamepad.buttons[GLFW_GAMEPAD_BUTTON_START])
		App::Quit();
}


