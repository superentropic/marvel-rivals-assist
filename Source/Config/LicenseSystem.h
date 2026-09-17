#pragma once
#include <string>
#include <fstream>
#include <filesystem>
#include "../../global.h"

namespace LicenseSystem {

    inline std::string GetLicensePath() {
        char path[MAX_PATH];
        if (GetModuleFileNameA(NULL, path, MAX_PATH)) {
            std::string dir = std::filesystem::path(path).parent_path().string();
            return dir + "\\license.ini";
        }
        return ".\\license.ini";
    }

    inline bool SaveLicense(const char* user, const char* pass) {
        try {
            std::string filepath = GetLicensePath();
            std::ofstream file(filepath);
            if (!file.is_open()) return false;
            file << "save=1" << std::endl;
            file << "user=" << user << std::endl;
            file << "pass=" << pass << std::endl;
            file.close();
            strncpy_s(mods::licenseUser, user, sizeof(mods::licenseUser) - 1);
            strncpy_s(mods::licensePass, pass, sizeof(mods::licensePass) - 1);
            mods::bLicenseSaved = true;
            return true;
        }
        catch (...) { return false; }
    }

    inline bool LoadLicense() {
        try {
            std::string filepath = GetLicensePath();
            std::ifstream file(filepath);
            if (!file.is_open()) return false;

            std::string line;
            bool hasSave = false;
            while (std::getline(file, line)) {
                if (line.substr(0, 5) == "save=") {
                    hasSave = (line.substr(5) == "1");
                }
                else if (line.substr(0, 5) == "user=") {
                    std::string val = line.substr(5);
                    strncpy_s(mods::licenseUser, val.c_str(), sizeof(mods::licenseUser) - 1);
                }
                else if (line.substr(0, 5) == "pass=") {
                    std::string val = line.substr(5);
                    strncpy_s(mods::licensePass, val.c_str(), sizeof(mods::licensePass) - 1);
                }
            }
            file.close();
            mods::bLicenseSaved = hasSave;
            return hasSave;
        }
        catch (...) { return false; }
    }

    inline bool DeleteLicense() {
        try {
            std::string filepath = GetLicensePath();
            mods::licenseUser[0] = '\0';
            mods::licensePass[0] = '\0';
            mods::bLicenseSaved = false;
            return std::filesystem::remove(filepath);
        }
        catch (...) { return false; }
    }
}
