#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <windows.h>
#include <vector>
#include <map>

namespace fs = std::filesystem;

// 定义游戏信息结构体
struct GameInfo {
    std::string name;// 游戏名
    std::string appId;// APPID
    std::vector<std::string> targetSubDirs; // 目标子目录列表
};

// 设置控制台显示 UTF-8 中文
void initConsole() {
    SetConsoleOutputCP(CP_UTF8);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}

// 从注册表获取 Steam 安装路径
std::string getSteamPath() {
    HKEY hKey;
    LPCSTR subKey = "SOFTWARE\\Wow6432Node\\Valve\\Steam";// 64 位
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, subKey, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        subKey = "SOFTWARE\\Valve\\Steam";// 32 位
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, subKey, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
            return "";
        }
    }

    char value[MAX_PATH];
    DWORD bufferSize = sizeof(value);
    if (RegQueryValueExA(hKey, "InstallPath", NULL, NULL, (LPBYTE)value, &bufferSize) == ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return std::string(value);
    }
    
    RegCloseKey(hKey);
    return "";
}

// 替换字符串中的子串
// 用于转义 VDF 路径
void replaceAll(std::string& str, const std::string& from, const std::string& to) {
    if (from.empty()) return;
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
}

// 解析 libraryfolders.vdf
// 读取 libraryfolders.vdf 里的 path 字段获取库路径
std::string getLibraryPath(const std::string& steamRoot, const std::string& appId) {
    fs::path vdfPath = fs::path(steamRoot) / "steamapps" / "libraryfolders.vdf";
    if (!fs::exists(vdfPath)) return "";

    std::ifstream file(vdfPath);
    std::string line;
    std::string currentPath = "";
    std::string targetAppIdStr = "\"" + appId + "\"";

    while (std::getline(file, line)) {
        if (line.find("\"path\"") != std::string::npos) {
            size_t firstQuote = line.find('"', line.find("\"path\"") + 6);
            if (firstQuote != std::string::npos) {
                size_t secondQuote = line.find('"', firstQuote + 1);
                if (secondQuote != std::string::npos) {
                    currentPath = line.substr(firstQuote + 1, secondQuote - firstQuote - 1);
                    replaceAll(currentPath, "\\\\", "\\");
                }
            }
        }
        if (line.find(targetAppIdStr) != std::string::npos) {
            return currentPath;
        }
    }
    return "";
}

// 3. 解析 appmanifest.acf 获取游戏的文件夹名
// 找到库路径下的 appmanifest.acf
// 读取 appmanifest.acf 里的 installdir 字段获取游戏的文件夹名
std::string getInstallDir(const std::string& libraryPath, const std::string& appId) {
    fs::path acfPath = fs::path(libraryPath) / "steamapps" / ("appmanifest_" + appId + ".acf");
    if (!fs::exists(acfPath)) return "";

    std::ifstream file(acfPath);
    std::string line;

    while (std::getline(file, line)) {
        if (line.find("\"installdir\"") != std::string::npos) {
            size_t firstQuote = line.find('"', line.find("\"installdir\"") + 12);
            if (firstQuote != std::string::npos) {
                size_t secondQuote = line.find('"', firstQuote + 1);
                if (secondQuote != std::string::npos) {
                    return line.substr(firstQuote + 1, secondQuote - firstQuote - 1);
                }
            }
        }
    }
    return "";
}

// 复制文件
bool copyDll(const fs::path& source, const fs::path& targetFolder) {
    try {
        if (!fs::exists(targetFolder)) {
            fs::create_directories(targetFolder);
        }
        fs::path targetFile = targetFolder / source.filename();
        std::cout << "正在复制到: " << targetFolder.string() << std::endl;
        fs::copy_file(source, targetFile, fs::copy_options::overwrite_existing);
        return true;
    }
    catch (const fs::filesystem_error& e) {
        std::cerr << "发生错误：复制文件到 " << targetFolder.string() << " 时出错 (" << e.what() << ")" << std::endl;
        return false;
    }
}

int main() {
    initConsole();

    // =================配置区域==================
    // 此处添加新游戏
    // 格式: {"序号", {"游戏名称", "AppID", {"目标子文件夹1", "目标子文件夹2"}}}
    // 注意: 子文件夹填 "" 表示直接复制到游戏根目录
    // ==========================================
    std::map<std::string, GameInfo> games = {
        {"1", {"深岩银河", "548430", {"FSD\\Binaries\\Win64"}}},
        {"2", {"战锤：末世鼠疫2", "552500", {"binaries", "binaries_dx12"}}},
        {"3", {"雨中冒险2", "632360", {""}}},
        {"4", {"遥遥西土", "3124540", {"FarFarWest\\Binaries\\Win64"}}},
        {"5", {"星露谷物语", "413150", {""}}},
        {"6", {"街霸6", "1364780", {""}}},
        {"7", {"深岩银河：异动核心", "2605790", {"RogueCore\\Binaries\\Win64"}}}
    };

    while (true) {
        system("cls");
        std::cout << "=============================================\n";
        std::cout << "         Steam 游戏 P2P 连接修复脚本\n";
        std::cout << "=============================================\n\n";
        std::cout << "请选择需要修复的游戏:\n\n";
        std::cout << "[1] 深岩银河 (Deep Rock Galactic)\n";
        std::cout << "[2] 战锤：末世鼠疫2 (Warhammer: Vermintide 2)\n";
        std::cout << "[3] 雨中冒险2 (Risk of Rain 2)\n";
        std::cout << "[4] 遥遥西土 (Far Far West)\n";
        std::cout << "[5] 星露谷物语 (Stardew Valley)\n";
        std::cout << "[6] 街霸6 (Street Fighter 6)\n";
        std::cout << "[7] 深岩银河：异动核心 (Deep Rock Galactic: Rogue Core)\n";
        std::cout << "[0] 退出脚本\n\n";
        
        std::cout << "请输入数字选择 (0-7): ";
        std::string choice;
        std::getline(std::cin, choice);

        if (choice == "0") {
            break;
        }
        
        // 验证输入是否有效
        if (games.find(choice) == games.end()) {
            std::cout << "\n发生错误：无效的输入，请重新选择！\n";
            system("pause");
            continue;
        }

        GameInfo selectedGame = games[choice];

        system("cls");
        std::cout << "=============================================\n";
        std::cout << "          正在修复：" << selectedGame.name << "\n";
        std::cout << "=============================================\n\n";

        // 1. 获取 Steam 安装路径
        std::cout << "[1/3] 正在获取 Steam 安装路径...\n";
        std::string steamRoot = getSteamPath();
        if (steamRoot.empty()) {
            std::cerr << "发生错误：注册表中未找到 Steam 安装路径\n\n";
            system("pause");
            continue;
        }
        std::cout << "Steam 安装路径 " << steamRoot << "\n\n";

        // 2. 获取游戏安装路径
        std::cout << "[2/3] 正在获取 " << selectedGame.name << " 安装路径...\n";
        std::string libraryPath = getLibraryPath(steamRoot, selectedGame.appId);
        if (libraryPath.empty()) {
            std::cerr << "发生错误：所有 Steam 库目录下均未找到 AppID: " << selectedGame.appId << " 的安装记录\n\n";
            system("pause");
            continue;
        }

        std::string installDir = getInstallDir(libraryPath, selectedGame.appId);
        if (installDir.empty()) {
            std::cerr << "发生错误：无法在 acf 文件中读取 installdir\n\n";
            system("pause");
            continue;
        }

        fs::path finalPath = fs::path(libraryPath) / "steamapps" / "common" / installDir;
        std::cout << "游戏安装路径 " << finalPath.string() << "\n\n";

        // 3. 检查并复制文件
        std::cout << "[3/3] 正在检查并复制文件...\n";
        fs::path sourceFile = fs::path(steamRoot) / "steamwebrtc64.dll";

        if (!fs::exists(sourceFile)) {
            std::cout << "Steam 目录下未找到 steamwebrtc64.dll，正在尝试从当前目录读取\n";
            sourceFile = fs::current_path() / "steamwebrtc64.dll";
            
            if (!fs::exists(sourceFile)) {
                std::cerr << "发生错误：Steam 目录和当前目录下均未找到 steamwebrtc64.dll\n\n";
                system("pause");
                continue;
            } else {
                std::cout << "已在当前目录找到备用的 DLL 文件\n";
            }
        }

        // 遍历该游戏配置的所有目标文件夹并复制
        bool allSuccess = true;
        for (const auto& subDir : selectedGame.targetSubDirs) {
            fs::path targetFolder = finalPath;
            if (!subDir.empty()) {
                targetFolder /= subDir;
            }
            allSuccess &= copyDll(sourceFile, targetFolder);
        }

        if (allSuccess) {
            std::cout << "\n=============================================\n";
            std::cout << "                   操作完成\n";
            std::cout << "=============================================\n\n";
        }
        system("pause");
    }

    return 0;
}