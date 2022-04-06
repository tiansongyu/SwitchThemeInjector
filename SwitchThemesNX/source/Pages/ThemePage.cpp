#include "ThemePage.hpp"
#include "../ViewFunctions.hpp"
#include <algorithm>
#include "../Platform/Platform.hpp"

#include "../UI/imgui/imgui_internal.h"

using namespace std;

ThemesPage* ThemesPage::Instance = nullptr;

ThemesPage::ThemesPage() : lblPage("")
{
	if (Instance)
		throw std::runtime_error("ThemePage::Instance should not be set");
	Instance = this;

	if (UseLowMemory)
		LimitLoad = 15;

	Name = "主题";
	lblCommands = CommandsTextNormal;

	RefreshThemesList();
}

void ThemesPage::RefreshThemesList()
{
	DisplayLoading("正在加载主题列表...");
	ClearSelection();
	SetPage(-1);
	CursorMemory.clear();

	ThemeFiles = std::move(fs::theme::ScanThemeFiles());
	std::sort(ThemeFiles.begin(), ThemeFiles.end());

	if (SelectOnRescanTarget != "")
	{
		SelectElementByPath(SelectOnRescanTarget);
		SelectOnRescanTarget = "";
	}
	else SetDir(fs::path::ThemesFolder);
}

void ThemesPage::SelectElementByPath(const std::string& path)
{	
	if (std::find(ThemeFiles.begin(), ThemeFiles.end(), path) == ThemeFiles.end())
		return; // File not in index

	SetDir(fs::GetPath(path));

	auto f = std::find(DirectoryFiles.begin(), DirectoryFiles.end(), path);
	if (f == DirectoryFiles.end())
		return; // Can this ever happen ?

	int index = (int)(f - DirectoryFiles.begin());

	// Find in which page is the file
	int page = 0;
	for (; page < pageCount; page++)
		if (index <= (page + 1) * LimitLoad) // Pages are 0-indexed
			break;

	int pageindex = index - (page * LimitLoad); // Start index of the current page

	SetPage(page, pageindex);
}

ThemesPage::~ThemesPage()
{
	Instance = nullptr;
	SetPage(-1);
}

void ThemesPage::SetDir(const string &dir)
{
	if (pageNum >= 0)
		CursorMemory[CurrentDir] = tuple<int,int>(pageNum, menuIndex);

	CurrentDir = dir;
	if (!StrEndsWith(dir, "/"))
		CurrentDir += "/";
	
	DirectoryFiles.clear();
	for (auto f : ThemeFiles)
		if (fs::GetPath(f) == CurrentDir)
			DirectoryFiles.push_back(f);
	
	pageCount = DirectoryFiles.size() / LimitLoad + 1;
	if (DirectoryFiles.size() % LimitLoad == 0)
		pageCount--;

	pageNum = -1; //force setpage to reload the entries even if in the same page as the path changed
	if (CursorMemory.count(dir))
	{
		const auto& [num, index] = CursorMemory[dir];
		SetPage(num, index);
	}
	else SetPage(0);
}

/*
	Set page number.
	if num < 0 just clear current page and don't load anything
	if index < 0 select last item of the page that's being set
*/
void ThemesPage::SetPage(int num, int index)
{
	ImGui::NavMoveRequestCancel();

	// There are no files, nothing to do here
	if (DirectoryFiles.size() == 0) return;

	// Reset scroll if we're changing the page or the menu index
	ResetScroll = num != pageNum || index != menuIndex;

	if (num < 0) // Just clear the current page
	{
		DisplayEntries.clear();
		pageNum = num;
		lblPage = "";
	}
	else if (num >= 0 && num != pageNum) // If the page is valid and different than the current one load the new entries
	{
		DisplayEntries.clear();
		pageNum = num;

		size_t baseIndex = num * LimitLoad;
		if (baseIndex >= DirectoryFiles.size())
		{
			// This shouldn't happen
			lblPage = "Error page out of bounds";
			return;
		}

		int imax = DirectoryFiles.size() - baseIndex;
		if (imax > LimitLoad) imax = LimitLoad;

		for (int i = 0; i < imax; i++)
			DisplayEntries.push_back(ThemeEntry::FromFile(DirectoryFiles[baseIndex + i]));

		UpdateBottomText();
	}

	// Set the menu index
	menuIndex = index < 0 ? PageItemsCount() - 1 : index;
}

void ThemesPage::Render(int X, int Y)
{
	Utils::ImGuiSetupPageFullscreen("ThemesPageContainer", X, Y, DefaultWinFlags | ImGuiWindowFlags_NoBringToFrontOnFocus);
	ImGui::PushFont(font25);

	if (ImGui::GetCurrentWindow()->Appearing && fs::theme::ShouldRescanThemeList())
		PushFunction([this]() { RefreshThemesList(); });

	if (DisplayEntries.size() == 0)
		ImGui::TextWrapped(
			"这里什么都没有，将您的主题复制到 sd 上的主题文件夹中，然后重试。\n"
			"如果您的 sd 中有一个带有主题的 themes 文件夹，请确保名称全部为小写，并且如果您使用的是 mac 或 sd 损坏，则您没有存档位问题，如果您使用 exfat，您可以找到有关这些的更多信息 在 google 上或在 discord 上寻求支持。"
		);

	ImGui::SetCursorPosY(600);
	Utils::ImGuiRightString(lblPage);
	
	if (DisplayEntries.size() == 0)
		goto QUIT_RENDERING;

	ImGui::SetCursorPosY(570);
	ImGui::TextUnformatted(lblCommands.c_str());

	{
		Utils::ImGuiSetupPage("主题列表", X, Y, DefaultWinFlags & ~ImGuiWindowFlags_NoScrollbar);
		int setNewMenuIndex = 0;
		if (ResetScroll || ImGui::GetCurrentWindow()->Appearing)
		{
			setNewMenuIndex = menuIndex;
			ImGui::NavMoveRequestCancel();
			ImGui::SetScrollY(0);
			FocusEvent.Set();
			ResetScroll = false;
		}		
		{
			int count = 0;
			for (auto& e : DisplayEntries)
			{
				bool Selected = IsSelected(e->GetPath());
				if (Selected)
					ImGui::PushStyleColor(ImGuiCol_WindowBg, 0x366e64ff);

				if (e->IsHighlighted())
					menuIndex = count;
				
				auto res = e->Render(Selected);

				if (Selected)
					ImGui::PopStyleColor();
				
				if (count == setNewMenuIndex && FocusEvent.Reset())
					Utils::ImGuiSelectItem(true);
				
				Utils::ImGuiDragWithLastElement();

				if (res == ThemeEntry::UserAction::Preview)
					break;
				else if (res == ThemeEntry::UserAction::Enter)
					PushFunction([count, &e, this]()
						{
							if (e->IsFolder())
								SetDir(e->GetPath());
							else
							{
								if (SelectedFiles.size() == 0)
									e->Install();
								else
								{
									if (menuIndex != count)
										menuIndex = count;
									SelectCurrent();
								}
							}
						});

				count++;
			}
		}

		//Here scrolling is handled by the individual theme entries, Utils::ImGuiSetWindowScrollable is not needed
		Utils::ImGuiCloseWin();
	}
QUIT_RENDERING:
	ImGui::PopFont();
	Utils::ImGuiCloseWin();
}

int ThemesPage::PageItemsCount()
{
	if (pageNum < 0)
		return 0;

	int menuCount = DirectoryFiles.size() - pageNum * LimitLoad;

	return menuCount > LimitLoad ? LimitLoad : menuCount;
}

inline bool ThemesPage::IsSelected(const std::string &fname)
{
	return (std::find(SelectedFiles.begin(), SelectedFiles.end(), fname) != SelectedFiles.end());
}

void ThemesPage::ClearSelection()
{
	SelectedFiles.clear();
	UpdateBottomText();
}

void ThemesPage::UpdateBottomText()
{
	if (pageCount <= 0)
	{
		lblPage = "";
		return;
	}

	std::stringstream ss;

	if (SelectedFiles.size() != 0)
		ss << "(" << SelectedFiles.size() << " selected) ";

	ss << CurrentDir << " - Page " << pageNum + 1 << "/" << pageCount;
	
	lblPage = ss.str();

	lblCommands = (SelectedFiles.size() == 0 ? CommandsTextNormal : CommandsTextSelected);
}

void ThemesPage::SelectCurrent()
{
	if (DisplayEntries[menuIndex]->IsFolder() || !DisplayEntries[menuIndex]->CanInstall()) return;

	auto fname = DisplayEntries[menuIndex]->GetPath();
	auto position = std::find(SelectedFiles.begin(), SelectedFiles.end(), fname);

	if (position != SelectedFiles.end())
		SelectedFiles.erase(position);
	else 
		SelectedFiles.push_back(fname);
	
	UpdateBottomText();
}

void ThemesPage::Update()
{
	int menuCount = PageItemsCount();	
	
	if (NAV_LEFT)
		Parent->PageLeaveFocus(this);
	if (KeyPressed(GLFW_GAMEPAD_BUTTON_B))
	{
		if (CurrentDir != fs::path::ThemesFolder)
			SetDir(fs::GetParentDir(CurrentDir));
		else 
			Parent->PageLeaveFocus(this);
	}
	
	if (menuCount <= 0 || pageNum < 0)
		return;

	if ((NAV_UP && menuIndex <= 0) || KeyPressed(GLFW_GAMEPAD_BUTTON_LEFT_BUMPER))
	{
		if (pageNum != 0) // If not the first page go to the previous
			SetPage(pageNum - 1, -1);
		else if (pageCount > 1) // If the first page and we have more pages go to the last
			SetPage(pageCount - 1 , -1);
		else // If there's only one page go to the last entry
			SetPage(pageNum, -1);
	}
	else if ((NAV_DOWN && menuIndex >= PageItemsCount() - 1) || KeyPressed(GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER))
	{
		if (pageNum + 1 < pageCount) // If we can go to the next page 
			SetPage(pageNum + 1);
		else // Else go to the first page, this happens both if we're at the last one or at the end of the first and only page
			SetPage(0);
	}
	else if (KeyPressed(GLFW_GAMEPAD_BUTTON_Y))
	{
		if (SelectedFiles.size() == 0 && menuIndex >= 0)
			SelectCurrent();
		else
			ClearSelection();
	}
	else if (KeyPressed(GLFW_GAMEPAD_BUTTON_START) && SelectedFiles.size() != 0)
	{
		for (string file : SelectedFiles)
		{
			ThemeEntry::DisplayInstallDialog(file);
			if (!ThemeEntry::FromFile(file)->Install(false))
			{
				Dialog("安装主题失败，进程被取消");
				break;
			}
		}
		ClearSelection();
		SetPage(pageNum);		
	}
}

void ThemesPage::SelectElementOnRescan(const std::string& path)
{
	SelectOnRescanTarget = path;
}
