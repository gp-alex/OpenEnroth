#include "Platform/Posix/Posix.h"

#include <SDL.h>
#include <dirent.h>
#include <glob.h>
#include <fnmatch.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <string>
#include <cstring>
#include <sstream>
#include <vector>

#include "Engine/Point.h"

void OS_MsgBox(const char *msg, const char *title) {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title, msg, nullptr);
}

unsigned int OS_GetTime() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

void OS_ShowCursor(bool show) {
    SDL_ShowCursor(show ? SDL_ENABLE : SDL_DISABLE);
}

void OS_Sleep(int ms) {
    SDL_Delay(ms);
}

bool OS_OpenConsole() {
    return true;
}

std::vector<std::string> OS_FindFiles(const std::string& folder, const std::string& mask) {
    std::vector<std::string> result;

    struct dirent *entry;
    DIR *dp;
    int flags = FNM_PATHNAME | FNM_PERIOD | FNM_CASEFOLD;

    dp = opendir(folder.c_str());
    if (dp != NULL) {
        while ((entry = readdir(dp)))
            if (fnmatch(mask.c_str(), entry->d_name, flags) == 0)
                result.push_back(entry->d_name);
        closedir(dp);
    }

    return result;
}


//////////////////// There is no Windows Registry ////////////////////

bool OS_GetAppString(const char* path, char* out_string, int out_string_size) {
    return false;
}

char OS_GetDirSeparator() {
    return '/';
}

std::string OS_casepath(std::string path) {
    std::string r;
    std::string sep;

    sep.push_back(OS_GetDirSeparator());
    size_t pos = 0;

    DIR *d = nullptr;
    if (!path.substr(0, 1).compare(sep)) {
        r = sep;
        d = opendir("/");
    } else {
        d = opendir(".");
    }

    std::stringstream ss (path);
    std::string s;

    while (std::getline(ss, s, OS_GetDirSeparator())) {
        if (s.empty())
            continue;

        if (!d) {
            if (!r.empty() && r.compare(sep))
                r += sep;
            r += s;

            continue;
        }

        bool found = false;
        struct dirent *e = readdir(d);
        while (e) {
            if (strcasecmp(s.c_str(), e->d_name) == 0) {
                found = true;

                if (!r.empty() && r.compare(sep))
                    r += sep;
                r += e->d_name;

                // some filesystems like reiserfs don't set entry type and we need additional step
                if (e->d_type == DT_UNKNOWN) {
                    struct stat st;
                    if (stat(r.c_str(), &st) != -1) {
                        if (S_ISDIR(st.st_mode))
                            e->d_type = DT_DIR;
                    }
                }

                if (e->d_type == DT_DIR) {
                    closedir(d);
                    d = opendir(r.c_str());
                } else {
                    closedir(d);
                    d = nullptr;
                }

                break;
            }

            e = readdir(d);
        }

        if (!found) {
            if (!r.empty() && r.compare(sep))
                r += sep;
            r += s;
        }
    }

    if (d)
        closedir(d);

    return r;
}

bool OS_FileExists(const std::string& path) {
    return _access(path.c_str(), 0) != -1;
}