#include "PatchMng.hpp"
#include <string>
#include "../fs.hpp"
#include "../SwitchThemesCommon/NXTheme.hpp"
#include <filesystem>
#include "../Platform/Platform.hpp"
#include <unordered_map>
#include "../UI/DialogPages.hpp"
#include "hactool.hpp"
#include "../Dialogs.hpp"

using namespace std;

static const u32 PatchSetVer = 9;
#define LastSupportedVerSTR "13.1"

#define ThemePatchesDir "NxThemesInstaller/"

#define WarnIntro "从 9.0 开始，主菜单的某些部分需要自定义代码补丁（exefs 补丁）才能正常运行。\n"
#define WarnOutro "\n\n如果没有正确的补丁，您仍然可以安装主题，但有些可能会崩溃，当安装已知会导致问题的主题时，您会收到警告"

//Is there even another CFW ?
const char* WarningCFW = WarnIntro "不幸的是，您的 CFW 似乎不支持标题的 ios 补丁。" WarnOutro;

static const char* WarningSX = WarnIntro
"\n看来您使用的是 SX 操作系统，仅在 2.9.4 beta 版中添加了对这些补丁的支持。\n"
"这意味着，如果您运行的是旧版本，您的 CFW 不兼容，您会看到此警告，因为此应用程序无法检测您当前的版本。\n"
"安装锁屏主题时，您将收到有关缺少补丁的警告，如果您确定拥有受支持的版本，则可以安全地安装该主题。\n\n"
"如果您没有正确的版本并安装主题，无论如何您的控制台将在启动时崩溃，安装前的警告还会显示修复它的说明。";

static const char* WarningMissingPatch = WarnIntro
	 "您正在运行此安装程序不支持的固件版本。 此版本发布时的最新固件是“ LastSupportedVerSTR ”。\n"
	 "主页菜单已更新，没有匹配的补丁，请在github上查看更新." WarnOutro;

static const char* WarningSDFail = WarnIntro "访问 sd 卡上的补丁目录时出错，您可能会受到 sd 损坏（可能在 exFat 上）或存档位问题的影响。" WarnOutro;

static const char* ErrorHactool = "无法检测到您正在运行的主菜单版本。 Hactool 无法提取主菜单版本信息。 请在github上打开一个问题。";

const char* PatchMng::InstallWarnStr = 
	"众所周知，您尝试安装的主题在没有主菜单补丁的情况下会崩溃，而且您似乎没有安装兼容的主题，"
	"它可能会工作，但它可能会在启动时崩溃。 你想继续吗？\n\n"
	"如果启动时崩溃，您可以通过从 SD 卡上的 /atmosphere/contents 手动删除 0100000000001000 文件夹来删除主题（/<your cfw>/titles 表示旧的氛围和其他 CFW）";

static const unordered_map<string, SystemVersion> PartsRequiringPatch = 
{
	{"Entrance.szs", {9,0,0} }
};

static bool HasLatestPatches = true;

static string GetExefsPatchesPath()
{
	if (fs::cfw::IsAms() || fs::cfw::IsSX())
		return fs::path::CfwFolder() + "exefs_patches/";
	else if (fs::cfw::IsRnx())
		return fs::path::CfwFolder() + "patches/";
	else return "";
}

bool PatchMng::CanInstallTheme(const string& FileName)
{
	if (HOSVer.major < 9) return true;
	if (!PartsRequiringPatch.count(FileName)) return true;
	
	const auto& ver = PartsRequiringPatch.at(FileName);

	if (HOSVer >= ver)
		return HasLatestPatches;
	else return true;

}

bool PatchMng::ExefsCompatAsk(const std::string& SzsName)
{
	if (!PatchMng::CanInstallTheme(SzsName))
		return YesNoPage::Ask(PatchMng::InstallWarnStr);
	return true;
}

void PatchMng::RemoveAll()
{
	fs::RecursiveDeleteFolder(GetExefsPatchesPath() + ThemePatchesDir);
	HasLatestPatches = false;
}

static bool ExtractPatches() 
{
	auto&& p = GetExefsPatchesPath();

	mkdir(p.c_str(), ACCESSPERMS);
	p += ThemePatchesDir;
	mkdir(p.c_str(), ACCESSPERMS);

	try {
		for (const auto& v : filesystem::directory_iterator(ASSET("patches")))
			fs::WriteFile(p + v.path().filename().string(), fs::OpenFile(v.path().string()));
	}
	catch (...)
	{
		return false;
	}

	FILE* f = fopen((p + "ver.txt").c_str(), "w");
	if (!f)
		return false;
	fprintf(f, "%u", PatchSetVer);
	fclose(f);

	return true;
}

PatchMng::ErrorPage PatchMng::EnsureInstalled()
{
	if (HOSVer.major < 9) return { };

	auto&& outDir = GetExefsPatchesPath();
	if (outDir == "")
	{
		HasLatestPatches = false;
		return {"Warning", WarningCFW };
	}

	FILE* f = fopen((outDir + ThemePatchesDir "ver.txt").c_str(), "r");
	if (!f)
		HasLatestPatches = ExtractPatches();
	else {
		u32 CurVer = 0;
		fscanf(f, "%u", &CurVer);
		fclose(f);

		if (CurVer < PatchSetVer)
			HasLatestPatches = ExtractPatches();
	}
	if (!HasLatestPatches)
		return { "Warning" , WarningSDFail };

	try 
	{
		auto id = hactool::QlaunchBuildID();
		LOGf("Qlaunch build ID is %s\n", id.c_str());

		if (!fs::Exists(std::string(ASSET("patches/")) + id + ".ips"))
			return { "Warning" , WarningMissingPatch };
	}	
	catch (std::exception& ex)
	{
		DialogBlocking(ex.what());
		return { "Error", ErrorHactool };
	}

	if (fs::path::CfwFolder() == SD_PREFIX SX_DIR)
	{
		HasLatestPatches = false;
		return { "Warning", WarningSX };
	}

	return {};
}

