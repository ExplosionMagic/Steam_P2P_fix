#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <windows.h>
#include <vector>
#include <map>

namespace fs = std::filesystem;

// Define game info struct
struct GameInfo
{
    std::string name;                       // Game name
    std::string appId;                      // APPID
    std::vector<std::string> targetSubDirs; // List of target subdirectories
};

// Get Steam installation path from registry
std::string getSteamPath()
{
    HKEY hKey;
    LPCSTR subKey = "SOFTWARE\\Wow6432Node\\Valve\\Steam"; // 64-bit
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, subKey, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
    {
        subKey = "SOFTWARE\\Valve\\Steam"; // 32-bit
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

// Replace substring (used for VDF path escaping)
void replaceAll(std::string &str, const std::string &from, const std::string &to)
{
    if (from.empty())
        return;
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos)
    {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
}

// Parse libraryfolders.vdf to get the library path for a given AppID
std::string getLibraryPath(const std::string &steamRoot, const std::string &appId)
{
    fs::path vdfPath = fs::path(steamRoot) / "steamapps" / "libraryfolders.vdf";
    if (!fs::exists(vdfPath))
        return "";

    std::ifstream file(vdfPath);
    std::string line;
    std::string currentPath = "";
    std::string targetAppIdStr = "\"" + appId + "\"";

    while (std::getline(file, line))
    {
        if (line.find("\"path\"") != std::string::npos)
        {
            size_t firstQuote = line.find('"', line.find("\"path\"") + 6);
            if (firstQuote != std::string::npos)
            {
                size_t secondQuote = line.find('"', firstQuote + 1);
                if (secondQuote != std::string::npos)
                {
                    currentPath = line.substr(firstQuote + 1, secondQuote - firstQuote - 1);
                    replaceAll(currentPath, "\\\\", "\\");
                }
            }
        }
        if (line.find(targetAppIdStr) != std::string::npos)
        {
            return currentPath;
        }
    }
    return "";
}

// Parse appmanifest.acf to get the game's installation folder name
std::string getInstallDir(const std::string &libraryPath, const std::string &appId)
{
    fs::path acfPath = fs::path(libraryPath) / "steamapps" / ("appmanifest_" + appId + ".acf");
    if (!fs::exists(acfPath))
        return "";

    std::ifstream file(acfPath);
    std::string line;

    while (std::getline(file, line))
    {
        if (line.find("\"installdir\"") != std::string::npos)
        {
            size_t firstQuote = line.find('"', line.find("\"installdir\"") + 12);
            if (firstQuote != std::string::npos)
            {
                size_t secondQuote = line.find('"', firstQuote + 1);
                if (secondQuote != std::string::npos)
                {
                    return line.substr(firstQuote + 1, secondQuote - firstQuote - 1);
                }
            }
        }
    }
    return "";
}

// Copy the DLL to the target folder (assumes the folder already exists)
bool copyDll(const fs::path &source, const fs::path &targetFolder)
{
    try
    {
        fs::path targetFile = targetFolder / source.filename();
        std::cout << "Copying to: " << targetFolder.string() << std::endl;
        fs::copy_file(source, targetFile, fs::copy_options::overwrite_existing);
        return true;
    }
    catch (const fs::filesystem_error &e)
    {
        std::cerr << "Error: Failed to copy file to " << targetFolder.string() << " (" << e.what() << ")" << std::endl;
        return false;
    }
}

int main()
{
    // ================Configuration Area==================
    // Add new games here
    // Format: {"Game Name", "AppID", {"target_subfolder1", "target_subfolder2"}}
    // Note: empty subfolder "" means copy directly to game root directory
    // ==========================================
    std::vector<GameInfo> games = {
        {"Deep Rock Galactic", "548430", {"FSD\\Binaries\\Win64"}},
        {"Warhammer: Vermintide 2", "552500", {"binaries", "binaries_dx12"}},
        {"Risk of Rain 2", "632360", {""}},
        {"Far Far West", "3124540", {"FarFarWest\\Binaries\\Win64"}},
        {"Stardew Valley", "413150", {""}},
        {"Street Fighter 6", "1364780", {""}},
    };

    while (true)
    {
        system("cls");
        std::cout << "=============================================\n";
        std::cout << "      Steam Game P2P Connection Fix Script\n";
        std::cout << "=============================================\n\n";
        std::cout << "Please select the game to fix:\n\n";
        for (size_t i = 0; i < games.size(); ++i)
        {
            std::cout << "[" << (i + 1) << "] " << games[i].name << "\n";
        }
        std::cout << "[0] Exit script\n\n";

        std::cout << "Enter a number (0-" << games.size() << "): ";
        std::string choice;
        std::getline(std::cin, choice);

        if (choice == "0")
        {
            break;
        }

        // Convert choice to index
        int idx;
        try
        {
            idx = std::stoi(choice) - 1;
        }
        catch (...)
        {
            std::cout << "\nError: Invalid input, please enter a number.\n";
            system("pause");
            continue;
        }

        if (idx < 0 || idx >= static_cast<int>(games.size()))
        {
            std::cout << "\nError: Selection out of range.\n";
            system("pause");
            continue;
        }

        GameInfo selectedGame = games[idx];

        system("cls");
        std::cout << "=============================================\n";
        std::cout << "          Fixing: " << selectedGame.name << "\n";
        std::cout << "=============================================\n\n";

        // 1. Get Steam installation path
        std::cout << "[1/3] Getting Steam installation path...\n";
        std::string steamRoot = getSteamPath();
        if (steamRoot.empty())
        {
            std::cerr << "Error: Steam installation path not found in registry\n\n";
            system("pause");
            continue;
        }
        std::cout << "Steam installation path: " << steamRoot << "\n\n";

        // 2. Get game installation path
        std::cout << "[2/3] Getting " << selectedGame.name << " installation path...\n";
        std::string libraryPath = getLibraryPath(steamRoot, selectedGame.appId);
        if (libraryPath.empty())
        {
            std::cerr << "Error: AppID " << selectedGame.appId << " not found in any Steam library folder\n\n";
            system("pause");
            continue;
        }

        std::string installDir = getInstallDir(libraryPath, selectedGame.appId);
        if (installDir.empty())
        {
            std::cerr << "Error: Unable to read installdir from acf file\n\n";
            system("pause");
            continue;
        }

        fs::path finalPath = fs::path(libraryPath) / "steamapps" / "common" / installDir;
        std::cout << "Game installation path: " << finalPath.string() << "\n\n";

        // 3. Check and copy files
        std::cout << "[3/3] Checking and copying files...\n";
        fs::path sourceFile = fs::path(steamRoot) / "steamwebrtc64.dll";

        if (!fs::exists(sourceFile))
        {
            std::cout << "steamwebrtc64.dll not found in Steam directory, trying current directory...\n";
            sourceFile = fs::current_path() / "steamwebrtc64.dll";

            if (!fs::exists(sourceFile))
            {
                std::cerr << "Error: steamwebrtc64.dll not found in Steam directory or current directory\n\n";
                system("pause");
                continue;
            }
            else
            {
                std::cout << "Found fallback DLL file in current directory\n";
            }
        }

        // Copy to all target folders configured for the selected game
        bool allSuccess = true;
        for (const auto &subDir : selectedGame.targetSubDirs)
        {
            fs::path targetFolder = finalPath;
            if (!subDir.empty())
            {
                targetFolder /= subDir;
            }
            allSuccess &= copyDll(sourceFile, targetFolder);
        }

        if (allSuccess)
        {
            std::cout << "\n=============================================\n";
            std::cout << "               Operation complete\n";
            std::cout << "=============================================\n\n";
        }
        system("pause");
    }

    return 0;
}