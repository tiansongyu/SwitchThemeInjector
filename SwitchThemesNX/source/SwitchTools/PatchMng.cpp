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

namespace {
	const unordered_map<string, SystemVersion> PartsRequiringPatch =
	{
		{"Entrance.szs", {9,0,0} }
	};

	bool HasLatestPatches = true;

	std::string qlaunchBuildID;

	string GetExefsPatchesPath()
	{
		if (fs::cfw::IsAms() || fs::cfw::IsSX())
			return fs::path::CfwFolder() + "exefs_patches/NxThemesInstaller/";
		else if (fs::cfw::IsRnx())
			return fs::path::CfwFolder() + "patches/NxThemesInstaller/";
		else return "";
	}

	const char* InstallWarnStr =
"众所周知，您尝试安装的主题在没有主菜单补丁的情况下会崩溃，而且您似乎没有安装兼容的主题，"
	"它可能会工作，但它可能会在启动时崩溃。 你想继续吗？\n\n"
	"如果启动时崩溃，您可以通过从 SD 卡上的 /atmosphere/contents 手动删除 0100000000001000 文件夹来删除主题（/<your cfw>/titles 表示旧的氛围和其他 CFW）";

}; 

bool PatchMng::Init() {
	try
	{
		qlaunchBuildID = hactool::QlaunchBuildID();
		LOGf("Qlaunch build ID is %s\n", qlaunchBuildID.c_str());
	}
	catch (std::exception& ex)
	{
		LOGf("Qlaunch build ID error %s\n", ex.what());
		return false;
	}

	return true;
}

const std::string& PatchMng::QlaunchBuildId()

{
	return qlaunchBuildID;
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
		return YesNoPage::Ask(InstallWarnStr);
	return true;
}

void PatchMng::RemoveAll()
{
	fs::RecursiveDeleteFolder(GetExefsPatchesPath());
	fs::RecursiveDeleteFolder(fs::path::PatchesDir);
	HasLatestPatches = false;
}

PatchMng::InstallResult PatchMng::EnsureInstalled()
{
	if (HOSVer.major < 9) return InstallResult::Ok;

	auto exefsDir = GetExefsPatchesPath();
	if (exefsDir == "")
	{
		HasLatestPatches = false;
		return InstallResult::UnsupportedCFW;
	}

	if (!fs::Exists(exefsDir))
		fs::CreateDirectory(exefsDir);

	if (qlaunchBuildID == "")
	{
		return InstallResult::SDError;
	}

	auto expectedPatchFile = exefsDir + qlaunchBuildID + ".ips";

	if (!fs::Exists(expectedPatchFile))
	{
		auto pathInRomfs = ASSET("patches/") + qlaunchBuildID + ".ips";

		try {
			if (fs::patches::hasPatchForBuild(qlaunchBuildID))
				fs::WriteFile(expectedPatchFile, fs::patches::OpenPatchForBuild(qlaunchBuildID));
			else if (fs::Exists(pathInRomfs))
				fs::WriteFile(expectedPatchFile, fs::OpenFile(pathInRomfs));
			else return InstallResult::MissingIps;
		}
		catch (...)
		{
			return InstallResult::SDError;
		}
	}
	
	HasLatestPatches = true;
	return InstallResult::Ok;
}

