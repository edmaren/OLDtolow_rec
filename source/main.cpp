#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string>
#include <vector>

extern "C" {
#include <ps5/kernel.h>
}

typedef struct notify_request {
    char useless1[45];
    char message[3075];
} notify_request_t;

extern "C" {
    int sceKernelSendNotificationRequest(int, notify_request_t *, size_t, int);
}

#define printf_notification(fmt, ...) \
{   notify_request_t req; \
    bzero(&req, sizeof req); \
    snprintf(req.message, sizeof req.message, fmt, ##__VA_ARGS__); \
    sceKernelSendNotificationRequest(0, &req, sizeof req, 0); \
} while(0);

static int g_renamed = 0;
static int g_skipped = 0;
static int g_failed  = 0;

static std::string to_lower(const std::string &s) {
    std::string r = s;
    for (char &c : r)
        c = (char)tolower((unsigned char)c);
    return r;
}

// Only files are checked; directories are always eligible for renaming.
// Returns true for .prx and .sprx extensions (case-insensitive).
static bool should_skip_file(const std::string &name) {
    auto pos = name.rfind('.');
    if (pos == std::string::npos) return false;
    std::string ext = to_lower(name.substr(pos));
    return ext == ".prx" || ext == ".sprx";
}

static void rename_lowercase_recursive(const std::string &path) {
    DIR *dir = opendir(path.c_str());
    if (!dir) return;

    std::vector<std::string> subdirs;
    std::vector<std::string> all_entries;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        std::string name = entry->d_name;
        if (name == "." || name == "..") continue;
        std::string full = path + "/" + name;
        struct stat st;
        if (lstat(full.c_str(), &st) != 0) continue;
        if (S_ISDIR(st.st_mode)) {
            subdirs.push_back(name);
            all_entries.push_back(name);
        } else {
            if (!should_skip_file(name))
                all_entries.push_back(name);
        }
    }
    closedir(dir);

    // Recurse with original names before renaming dirs
    for (const auto &sub : subdirs)
        rename_lowercase_recursive(path + "/" + sub);

    for (const auto &name : all_entries) {
        std::string lower = to_lower(name);
        if (name == lower) continue;
        std::string old_path = path + "/" + name;
        std::string new_path = path + "/" + lower;
        struct stat st_check;
        if (lstat(new_path.c_str(), &st_check) == 0) {
            g_skipped++;
            continue;
        }
        if (rename(old_path.c_str(), new_path.c_str()) == 0)
            g_renamed++;
        else
            g_failed++;
    }
}

int main() {
    pid_t pid = getpid();
    kernel_set_ucred_authid(pid, 0x4801000000000013L);
    kernel_set_proc_rootdir(pid, kernel_get_root_vnode());

    printf_notification("TOLOWER RECURSIVE STARTED!");

    rename_lowercase_recursive("/data/homebrew");
    rename_lowercase_recursive("/mnt/ext1/homebrew");

    printf_notification("OK: %d renamed, %d skipped (collision), %d failed",
                        g_renamed, g_skipped, g_failed);
    return 0;
}
