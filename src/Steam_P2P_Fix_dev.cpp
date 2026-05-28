#include <iostream>
#include <string>
#include <filesystem>
#include <windows.h>
#include <vector>
#include <shlobj.h>

namespace fs = std::filesystem;

// 设置控制台输出 UTF-8 编码（避免中文乱码）
void initConsole()
{
    SetConsoleOutputCP(CP_UTF8);
}

// 从注册表获取 Steam 安装路径
std::string getSteamPath()
{
    HKEY hKey;
    LPCSTR subKey = "SOFTWARE\\Wow6432Node\\Valve\\Steam"; // 64 位
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, subKey, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
    {
        subKey = "SOFTWARE\\Valve\\Steam"; // 32 位
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, subKey, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        {
            return "";
        }
    }

    char value[MAX_PATH];
    DWORD bufferSize = sizeof(value);
    if (RegQueryValueExA(hKey, "InstallPath", NULL, NULL, (LPBYTE)value, &bufferSize) == ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return std::string(value);
    }

    RegCloseKey(hKey);
    return "";
}

// 复制文件
bool copyDll(const fs::path &source, const fs::path &targetFolder)
{
    try
    {
        fs::path targetFile = targetFolder / source.filename();
        std::cout << "  正在复制 " << source.filename().string() << " 到 " << targetFolder.string() << std::endl;
        fs::copy_file(source, targetFile, fs::copy_options::overwrite_existing);
        return true;
    }
    catch (const fs::filesystem_error &e)
    {
        std::cerr << "  错误：复制 " << source.filename().string() << " 失败 (" << e.what() << ")" << std::endl;
        return false;
    }
}

int main()
{
    initConsole();

    // 检查是否以管理员身份运行
    if (!IsUserAnAdmin())
    {
        std::cerr << "错误：请以管理员身份运行该程序。\n";
        std::cerr << "右键该程序选择“以管理员身份运行”。\n\n";
        system("pause");
        return 1;
    }

    std::cout << "=============================================\n";
    std::cout << "  Steam P2P 修复 - 正在启动...\n";
    std::cout << "=============================================\n\n";

    // 需要复制的 DLL 文件名列表
    const std::vector<std::string> dllNames = {
        "steamwebrtc64.dll",
        "steamwebrtc.dll"};

    // 获取 Steam 安装路径（用于定位源 DLL）
    std::cout << "[1/3] 正在获取 Steam 安装路径...\n";
    std::string steamRoot = getSteamPath();
    if (steamRoot.empty())
    {
        std::cerr << "  注册表中未找到 Steam 安装路径\n";
        std::cerr << "  将尝试从当前路径读取 DLL 文件\n\n";
    }
    else
    {
        std::cout << "  Steam 安装路径 " << steamRoot << "\n\n";
    }

    // 定位源 DLL 文件
    std::cout << "[2/3] 正在查找 DLL 文件...\n";
    std::vector<fs::path> sourceFiles;

    for (const auto &dllName : dllNames)
    {
        fs::path srcPath;
        bool found = false;

        // 优先从 Steam 安装路径获取
        if (!steamRoot.empty())
        {
            srcPath = fs::path(steamRoot) / dllName;
            if (fs::exists(srcPath))
            {
                std::cout << "  在 Steam 安装路径找到 " << dllName << std::endl;
                sourceFiles.push_back(srcPath);
                found = true;
            }
        }

        // 如果 Steam 安装路径没有，尝试在当前路径寻找
        if (!found)
        {
            srcPath = fs::current_path() / dllName;
            if (fs::exists(srcPath))
            {
                std::cout << "  在当前路径找到 " << dllName << std::endl;
                sourceFiles.push_back(srcPath);
                found = true;
            }
        }

        if (!found)
        {
            std::cout << "  未找到 " << dllName << "，跳过" << std::endl;
        }
    }

    if (sourceFiles.empty())
    {
        std::cerr << "\n发生错误：未找到任何 DLL 文件，操作终止。\n\n";
        system("pause");
        return 1;
    }

    // 复制到 C:\Windows
    std::cout << "\n[3/3] 正在将文件复制到目标位置...\n";
    bool allSuccess = true;
    for (const auto &src : sourceFiles)
    {
        if (!copyDll(src, "C:\\Windows"))
        {
            allSuccess = false;
        }
    }

    if (allSuccess)
    {
        std::cout << "\n=============================================\n";
        std::cout << "  操作完成！所有文件已成功复制。\n";
        std::cout << "=============================================\n\n";
    }
    else
    {
        std::cerr << "\n文件复制失败，检查程序是否以管理员身份运行。\n\n";
    }

    system("pause");
    return allSuccess ? 0 : 1;
}
