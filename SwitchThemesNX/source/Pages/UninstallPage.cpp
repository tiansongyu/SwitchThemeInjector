#include "UninstallPage.hpp"
#include "../ViewFunctions.hpp"
#include "../SwitchTools/PatchMng.hpp"

using namespace std;

UninstallPage::UninstallPage()
{
	Name = "卸载主题";
}

void UninstallPage::Render(int X, int Y)
{
	Utils::ImGuiSetupPage(this, X, Y);
	ImGui::PushFont(font30);

	ImGui::TextWrapped("使用它来卸载当前安装的主题。\n如果你有问题，请尝试完全删除 LayeredFS 目录和主菜单补丁，同时按 L+R。");

	ImGui::PushStyleColor(ImGuiCol_Button, u32(0x6B70000ff));
	if (ImGui::Button("卸载"))
	{
		bool FullUninstall = gamepad.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER] && gamepad.buttons[GLFW_GAMEPAD_BUTTON_LEFT_BUMPER];
		PushFunction([FullUninstall]() {
			if (!YesNoPage::Ask("确定吗 ？")) return;
			if (FullUninstall)
			{
				DisplayLoading("清除 LayeredFS 目录...");
				fs::theme::UninstallTheme(true);
				PatchMng::RemoveAll();
				Dialog(
					"完成，与主题相关的所有内容都已删除，重新启动控制台以查看更改。\n"
					"由于这也删除了主菜单补丁，因此您应该在安装任何主题之前重新启动此自制软件."
				);
			}
			else
			{
				DisplayLoading("加载中...");
				fs::theme::UninstallTheme(false);
				Dialog("完成，所有已安装的主题都已删除，重新启动控制台以查看更改");
			}
		});
	}
	PAGE_RESET_FOCUS;
	ImGui::PopStyleColor();

	ImGui::PopFont();
	Utils::ImGuiSetWindowScrollable();
	Utils::ImGuiCloseWin();
}

void UninstallPage::Update()
{	
	if (Utils::PageLeaveFocusInput()){
		Parent->PageLeaveFocus(this);
	}
}




