// libmc (sceMc*): an 8 MB PS2 memory card in slot 1 (port 0) backed by a
// host directory; slot 2 (port 1) is empty.
//
// - PORT_MEMCARD_DIR=path: the card's root (default
//   $XDG_DATA_HOME/splashdown/memcard0, else ~/.local/share/splashdown/memcard0;
//   created formatted and empty the first time).
// - PORT_NO_MEMCARD=1: no card in either slot (the old stub; golden.py).
// - PORT_MEMCARD_CLUSTERS=N: allocatable 1 KB clusters (default 8135, an
//   8 MB card after format) for "card full" tests.
// - PORT_MEMCARD_LOG=1: log every command and its result.
//
// A card directory is a host directory, a card file a host file; host names
// starting with '.' are not on the card (`.unformatted` there makes the card
// read as unformatted until sceMcFormat). The async model: each call
// registers one command and returns 0 (or -1 while an earlier command's
// result hasn't been collected); the command runs at once, its outputs
// (GetInfo's type/free/format, Read's data, GetDir's table) are written then,
// and sceMcSync(0 or 1, &cmd, &result) returns 1 (finished) with the
// command's number and result, then -1 (idle) until the next call.
#include "port.h"
#include "port_os.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <dirent.h>
#include <fcntl.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

#ifndef O_BINARY
#define O_BINARY 0   // Windows: no newline translation
#endif

namespace {

// libmc.h
enum {
    kFuncGetInfo = 1, kFuncOpen = 2, kFuncClose = 3, kFuncSeek = 4, kFuncRead = 5,
    kFuncWrite = 6, kFuncFlush = 0xA, kFuncMkdir = 0xB, kFuncChDir = 0xC,
    kFuncGetDir = 0xD, kFuncFileInfo = 0xE, kFuncDelete = 0xF, kFuncFormat = 0x10,
    kFuncUnformat = 0x11, kFuncGetEntSpace = 0x12,
};
enum {
    kResSucceed = 0, kResChangedCard = -1, kResNoFormat = -2, kResFullDevice = -3,
    kResNoEntry = -4, kResDeniedPermit = -5, kResNotEmpty = -6, kResUpLimitHandle = -7,
    kResFailDetect = -10,
};
enum { kRdOnly = 1, kWrOnly = 2, kRdWr = 3, kCreat = 0x200 };
enum { kTypePS2 = 2 };
constexpr int kMaxHandles = 3;
constexpr int kClusterSize = 1024;

struct Handle {
    int fd = -1;
    int mode = 0;
    std::string path;   // card path, "/DIR/FILE"
};

struct Card {
    bool present = false;
    bool infoSeen = false;  // GetInfo has reported the insertion (-1/-2)
    std::string root;       // host directory
    std::string cwd = "/";
    int capacity = 8135;
    std::string getDirPattern;
    std::vector<std::string> getDirNames;
    size_t getDirNext = 0;
};

Card g_card[2];
Handle g_handle[kMaxHandles];
bool g_init;
bool g_log;
bool g_pending;
int g_cmd;
int g_result;

#pragma pack(push, 1)
struct McDateTime {
    uint8_t resv2, sec, min, hour, day, month;
    uint16_t year;
};
struct McTblGetDir {
    McDateTime create;
    McDateTime modify;
    uint32_t fileSizeByte;
    uint16_t attrFile;
    uint16_t reserve1;
    uint32_t reserve2;
    uint32_t pdaAplNo;
    char entryName[32];
};
#pragma pack(pop)
static_assert(sizeof(McTblGetDir) == 0x40);

void MkdirP(const std::string &path)
{
    std::string cur;
    for (size_t i = 0; i <= path.size(); i++) {
        if (i == path.size() || path[i] == '/') {
            if (!cur.empty())
                port_mkdir(cur.c_str());
        }
        if (i < path.size())
            cur += path[i];
    }
}

void Init()
{
    if (g_init)
        return;
    g_init = true;
    g_log = getenv("PORT_MEMCARD_LOG") && atoi(getenv("PORT_MEMCARD_LOG"));
    const char *no = getenv("PORT_NO_MEMCARD");
    if (no && atoi(no)) {
        port_log("memory card: none (PORT_NO_MEMCARD)");
        return;
    }
    Card &c = g_card[0];
    if (const char *d = getenv("PORT_MEMCARD_DIR"); d && *d) {
        c.root = d;
    } else if (const char *d = port_data_dir(); *d) {
        c.root = std::string(d) + "/memcard0";
    } else {
        port_log("memory card: no PORT_MEMCARD_DIR and no home directory: none");
        return;
    }
    while (c.root.size() > 1 && c.root.back() == '/')
        c.root.pop_back();
    struct stat st;
    if (stat(c.root.c_str(), &st) != 0) {
        MkdirP(c.root);   // a new card: formatted, empty
        if (stat(c.root.c_str(), &st) != 0) {
            port_log("memory card: can't create %s (%s): none", c.root.c_str(), strerror(errno));
            return;
        }
        port_log("memory card: new formatted card in %s", c.root.c_str());
    }
    if (!S_ISDIR(st.st_mode)) {
        port_log("memory card: %s is not a directory: none", c.root.c_str());
        return;
    }
    if (const char *n = getenv("PORT_MEMCARD_CLUSTERS"); n && *n)
        c.capacity = atoi(n);
    c.present = true;
    port_log("memory card: slot 1 = %s", c.root.c_str());
}

bool Formatted(const Card &c)
{
    return access((c.root + "/.unformatted").c_str(), F_OK) != 0;
}

// Normalises a card path against cwd: "/A/B" (root "/"); false if it climbs
// above the root or has an empty/overlong component.
bool Resolve(const Card &c, const char *name, std::string &out)
{
    std::vector<std::string> parts;
    std::string s = name ? name : "";
    if (s.empty() || s[0] != '/')
        s = c.cwd + "/" + s;
    size_t i = 0;
    while (i < s.size()) {
        size_t j = s.find('/', i);
        if (j == std::string::npos)
            j = s.size();
        std::string p = s.substr(i, j - i);
        i = j + 1;
        if (p.empty() || p == ".")
            continue;
        if (p == "..") {
            if (!parts.empty())
                parts.pop_back();
            continue;
        }
        if (p.size() > 31 || p[0] == '.')
            return false;
        parts.push_back(p);
    }
    out.clear();
    for (auto &p : parts)
        out += "/" + p;
    if (out.empty())
        out = "/";
    return true;
}

std::string Host(const Card &c, const std::string &path)
{
    return path == "/" ? c.root : c.root + path;
}

std::string Parent(const std::string &path)
{
    size_t k = path.rfind('/');
    return k == 0 ? "/" : path.substr(0, k);
}

int ClustersFor(long long bytes)
{
    return (int)((bytes + kClusterSize - 1) / kClusterSize);
}

// Card entries in a directory (host names not starting with '.').
std::vector<std::string> List(const std::string &hostDir)
{
    std::vector<std::string> names;
    if (DIR *d = opendir(hostDir.c_str())) {
        while (dirent *e = readdir(d)) {
            if (e->d_name[0] != '.')
                names.push_back(e->d_name);
        }
        closedir(d);
    }
    std::sort(names.begin(), names.end());
    return names;
}

// Clusters a directory tree takes: two 512-byte entries a cluster ("." and
// ".." included), each file its size rounded up to clusters.
int Used(const std::string &hostDir)
{
    std::vector<std::string> names = List(hostDir);
    int used = ClustersFor((long long)(names.size() + 2) * 512);
    for (auto &n : names) {
        std::string p = hostDir + "/" + n;
        struct stat st;
        if (stat(p.c_str(), &st) != 0)
            continue;
        if (S_ISDIR(st.st_mode))
            used += Used(p);
        else
            used += ClustersFor(st.st_size);
    }
    return used;
}

int Free(const Card &c)
{
    int f = c.capacity - Used(c.root);
    return f < 0 ? 0 : f;
}

// A new entry in dir: one more cluster for the directory when its entry
// count is even.
int EntryCost(const Card &c, const std::string &dir)
{
    size_t n = List(Host(c, dir)).size() + 2;
    return (n % 2 == 0) ? 1 : 0;
}

bool IsDir(const std::string &host)
{
    struct stat st;
    return stat(host.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

bool Exists(const std::string &host)
{
    struct stat st;
    return stat(host.c_str(), &st) == 0;
}

void FillTime(McDateTime &t, time_t when)
{
    when += 9 * 3600;   // the PS2 keeps JST
    struct tm tm;
    gmtime_r(&when, &tm);
    t.resv2 = 0;
    t.sec = tm.tm_sec;
    t.min = tm.tm_min;
    t.hour = tm.tm_hour;
    t.day = tm.tm_mday;
    t.month = tm.tm_mon + 1;
    t.year = tm.tm_year + 1900;
}

void FillEntry(McTblGetDir &e, const std::string &host, const char *name)
{
    memset(&e, 0, sizeof(e));
    struct stat st;
    if (stat(host.c_str(), &st) != 0)
        memset(&st, 0, sizeof(st));
    FillTime(e.create, st.st_mtime);
    FillTime(e.modify, st.st_mtime);
    if (S_ISDIR(st.st_mode)) {
        e.attrFile = 0x8427;   // exists, closed, directory, rwx
        e.fileSizeByte = strcmp(name, ".") && strcmp(name, "..") ? List(host).size() + 2 : 0;
    } else {
        e.attrFile = 0x8497;   // exists, closed, file, rwx, protected
        e.fileSizeByte = (uint32_t)st.st_size;
    }
    strncpy(e.entryName, name, sizeof(e.entryName) - 1);
}

// '*' and '?' as libmc matches them.
bool Match(const char *pat, const char *s)
{
    if (!*pat)
        return !*s;
    if (*pat == '*')
        return Match(pat + 1, s) || (*s && Match(pat, s + 1));
    if (!*s)
        return false;
    return (*pat == '?' || *pat == *s) && Match(pat + 1, s + 1);
}

// Registers a command. False (the call returns -1) while the previous
// command's result has not been collected by sceMcSync.
bool Begin(int cmd)
{
    Init();
    if (g_pending) {
        port_log("memory card: command %d while %d is unsynced", cmd, g_cmd);
        return false;
    }
    g_pending = true;
    g_cmd = cmd;
    g_result = 0;
    return true;
}

int Finish(int result, const char *what, const char *arg = "")
{
    g_result = result;
    if (g_log && what)
        port_log("mc %s %s -> %d", what, arg, result);
    return 0;
}

Card *CardAt(int port, int slot)
{
    if (port < 0 || port > 1 || slot != 0 || !g_card[port].present)
        return nullptr;
    return &g_card[port];
}

// Common checks for commands on a card: present and formatted.
int Usable(Card *c)
{
    if (!c)
        return kResFailDetect;
    if (!Formatted(*c))
        return kResNoFormat;
    return kResSucceed;
}

} // namespace

extern "C" {

int sceMcInit(void)
{
    Init();
    return 0;
}

int sceMcSync(int mode, int *cmd, int *result)
{
    (void)mode;   // commands finish when called: mode 0 (wait) = mode 1 (poll)
    if (!g_pending)
        return -1;   // sceMcExecIdle
    g_pending = false;
    if (cmd)
        *cmd = g_cmd;
    if (result)
        *result = g_result;
    return 1;        // sceMcExecFinish
}

int sceMcGetInfo(int port, int slot, int *type, int *free, int *format)
{
    if (!Begin(kFuncGetInfo))
        return -1;
    Card *c = CardAt(port, slot);
    if (!c) {
        if (type) *type = 0;
        if (free) *free = 0;
        if (format) *format = 0;
        return Finish(kResFailDetect, "GetInfo", port ? "port 1" : "port 0");
    }
    bool fmt = Formatted(*c);
    if (type) *type = kTypePS2;
    if (format) *format = fmt ? 1 : 0;
    if (free) *free = fmt ? Free(*c) : 0;
    int r = kResSucceed;
    if (!c->infoSeen) {
        c->infoSeen = true;
        r = fmt ? kResChangedCard : kResNoFormat;   // a card inserted since the last call
    }
    // Logged when the answer changes (pages poll it four times a second).
    static std::string last;
    char buf[64];
    snprintf(buf, sizeof buf, "port %d fmt %d free %d (%d)", port, fmt, fmt ? Free(*c) : 0, r);
    if (last == buf)
        return Finish(r, nullptr);
    last = buf;
    return Finish(r, "GetInfo", buf);
}

int sceMcFormat(int port, int slot)
{
    if (!Begin(kFuncFormat))
        return -1;
    Card *c = CardAt(port, slot);
    if (!c)
        return Finish(kResFailDetect, "Format");
    for (int i = 0; i < kMaxHandles; i++) {
        if (g_handle[i].fd >= 0) {
            close(g_handle[i].fd);
            g_handle[i] = Handle();
        }
    }
    // Remove everything under the root, the .unformatted marker included.
    std::vector<std::string> todo{c->root};
    std::vector<std::string> dirs;
    while (!todo.empty()) {
        std::string d = todo.back();
        todo.pop_back();
        if (DIR *dd = opendir(d.c_str())) {
            while (dirent *e = readdir(dd)) {
                if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, ".."))
                    continue;
                std::string p = d + "/" + e->d_name;
                if (IsDir(p)) {
                    todo.push_back(p);
                    dirs.push_back(p);
                } else {
                    unlink(p.c_str());
                }
            }
            closedir(dd);
        }
    }
    for (auto it = dirs.rbegin(); it != dirs.rend(); ++it)
        rmdir(it->c_str());
    c->cwd = "/";
    return Finish(kResSucceed, "Format");
}

int sceMcChdir(int port, int slot, const char *newDir, char *curDir)
{
    if (!Begin(kFuncChDir))
        return -1;
    Card *c = CardAt(port, slot);
    if (int r = Usable(c))
        return Finish(r, "ChDir", newDir);
    if (curDir)
        strcpy(curDir, c->cwd.c_str());
    std::string p;
    if (!Resolve(*c, newDir, p) || !IsDir(Host(*c, p)))
        return Finish(kResNoEntry, "ChDir", newDir);
    c->cwd = p;
    return Finish(kResSucceed, "ChDir", newDir);
}

int sceMcMkdir(int port, int slot, const char *name)
{
    if (!Begin(kFuncMkdir))
        return -1;
    Card *c = CardAt(port, slot);
    if (int r = Usable(c))
        return Finish(r, "Mkdir", name);
    std::string p;
    if (!Resolve(*c, name, p) || p == "/" || !IsDir(Host(*c, Parent(p))))
        return Finish(kResNoEntry, "Mkdir", name);
    if (Exists(Host(*c, p)))
        return Finish(kResNoEntry, "Mkdir", name);   // libmc: -4 for an existing name
    if (Free(*c) < 1 + EntryCost(*c, Parent(p)))
        return Finish(kResFullDevice, "Mkdir", name);
    if (port_mkdir(Host(*c, p).c_str()) != 0)
        return Finish(kResDeniedPermit, "Mkdir", name);
    return Finish(kResSucceed, "Mkdir", name);
}

int sceMcGetDir(int port, int slot, const char *name, unsigned int mode, int maxEnt, void *table)
{
    if (!Begin(kFuncGetDir))
        return -1;
    Card *c = CardAt(port, slot);
    if (int r = Usable(c))
        return Finish(r, "GetDir", name);
    if (mode == 0) {
        // A new search: the directory part of name, then its entries that
        // match the last component ("." and ".." outside the root).
        std::string s = name ? name : "";
        std::string dirPart, pat = s;
        size_t k = s.rfind('/');
        if (k != std::string::npos) {
            dirPart = s.substr(0, k + 1);
            pat = s.substr(k + 1);
        }
        std::string dir;
        if (!Resolve(*c, dirPart.empty() ? "." : dirPart.c_str(), dir) || !IsDir(Host(*c, dir))) {
            c->getDirNames.clear();
            c->getDirNext = 0;
            return Finish(kResNoEntry, "GetDir", name);
        }
        c->getDirPattern = Host(*c, dir);
        c->getDirNames.clear();
        if (dir != "/") {
            if (Match(pat.c_str(), "."))
                c->getDirNames.push_back(".");
            if (Match(pat.c_str(), ".."))
                c->getDirNames.push_back("..");
        }
        for (auto &n : List(c->getDirPattern))
            if (Match(pat.c_str(), n.c_str()))
                c->getDirNames.push_back(n);
        c->getDirNext = 0;
    }
    McTblGetDir *out = (McTblGetDir *)table;
    int n = 0;
    while (n < maxEnt && c->getDirNext < c->getDirNames.size()) {
        const std::string &e = c->getDirNames[c->getDirNext++];
        std::string host = e == "." ? c->getDirPattern
                         : e == ".." ? c->getDirPattern + "/.."
                                     : c->getDirPattern + "/" + e;
        FillEntry(out[n++], host, e.c_str());
    }
    return Finish(n, "GetDir", name);
}

int sceMcOpen(int port, int slot, char *name, int mode)
{
    if (!Begin(kFuncOpen))
        return -1;
    Card *c = CardAt(port, slot);
    if (int r = Usable(c))
        return Finish(r, "Open", name);
    std::string p;
    if (!Resolve(*c, name, p) || p == "/" || !IsDir(Host(*c, Parent(p))))
        return Finish(kResNoEntry, "Open", name);
    int h = 0;
    while (h < kMaxHandles && g_handle[h].fd >= 0)
        h++;
    if (h == kMaxHandles)
        return Finish(kResUpLimitHandle, "Open", name);
    std::string host = Host(*c, p);
    if (IsDir(host))
        return Finish(kResDeniedPermit, "Open", name);
    bool exists = Exists(host);
    if (!exists && !(mode & kCreat))
        return Finish(kResNoEntry, "Open", name);
    if (!exists && Free(*c) < EntryCost(*c, Parent(p)))
        return Finish(kResFullDevice, "Open", name);
    int acc = (mode & 3) == kRdOnly ? O_RDONLY : (mode & 3) == kWrOnly ? O_WRONLY : O_RDWR;
    int fd = open(host.c_str(), acc | O_BINARY | ((mode & kCreat) ? O_CREAT : 0), 0644);
    if (fd < 0)
        return Finish(kResDeniedPermit, "Open", name);
    g_handle[h].fd = fd;
    g_handle[h].mode = mode;
    g_handle[h].path = p;
    return Finish(h, "Open", name);
}

int sceMcClose(int fd)
{
    if (!Begin(kFuncClose))
        return -1;
    if (fd < 0 || fd >= kMaxHandles || g_handle[fd].fd < 0)
        return Finish(kResDeniedPermit, "Close");
    close(g_handle[fd].fd);
    g_handle[fd] = Handle();
    return Finish(kResSucceed, "Close");
}

int sceMcRead(int fd, void *buf, int size)
{
    if (!Begin(kFuncRead))
        return -1;
    if (fd < 0 || fd >= kMaxHandles || g_handle[fd].fd < 0 || !(g_handle[fd].mode & kRdOnly))
        return Finish(kResDeniedPermit, "Read");
    if (!Formatted(g_card[0]))
        return Finish(kResNoFormat, "Read");
    long n = 0;
    while (n < size) {
        ssize_t k = read(g_handle[fd].fd, (char *)buf + n, size - n);
        if (k <= 0)
            break;
        n += k;
    }
    char arg[300];
    snprintf(arg, sizeof arg, "%s %d", g_handle[fd].path.c_str(), size);
    return Finish((int)n, "Read", arg);
}

int sceMcWrite(int fd, const void *buf, int size)
{
    if (!Begin(kFuncWrite))
        return -1;
    if (fd < 0 || fd >= kMaxHandles || g_handle[fd].fd < 0 || !(g_handle[fd].mode & kWrOnly))
        return Finish(kResDeniedPermit, "Write");
    Card &c = g_card[0];
    if (!Formatted(c))
        return Finish(kResNoFormat, "Write");
    struct stat st;
    fstat(g_handle[fd].fd, &st);
    off_t pos = lseek(g_handle[fd].fd, 0, SEEK_CUR);
    long long newSize = pos + size > st.st_size ? pos + size : st.st_size;
    char arg[300];
    snprintf(arg, sizeof arg, "%s %d", g_handle[fd].path.c_str(), size);
    if (ClustersFor(newSize) - ClustersFor(st.st_size) > Free(c))
        return Finish(kResFullDevice, "Write", arg);
    long n = 0;
    while (n < size) {
        ssize_t k = write(g_handle[fd].fd, (const char *)buf + n, size - n);
        if (k <= 0)
            break;
        n += k;
    }
    return Finish((int)n, "Write", arg);
}

int sceMcDelete(int port, int slot, const char *name)
{
    if (!Begin(kFuncDelete))
        return -1;
    Card *c = CardAt(port, slot);
    if (int r = Usable(c))
        return Finish(r, "Delete", name);
    std::string p;
    if (!Resolve(*c, name, p) || p == "/")
        return Finish(kResNoEntry, "Delete", name);
    std::string host = Host(*c, p);
    if (!Exists(host))
        return Finish(kResNoEntry, "Delete", name);
    for (int i = 0; i < kMaxHandles; i++)
        if (g_handle[i].fd >= 0 && g_handle[i].path == p)
            return Finish(kResDeniedPermit, "Delete", name);   // open
    if (IsDir(host)) {
        if (!List(host).empty())
            return Finish(kResNotEmpty, "Delete", name);
        if (rmdir(host.c_str()) != 0)
            return Finish(kResDeniedPermit, "Delete", name);
    } else if (unlink(host.c_str()) != 0) {
        return Finish(kResDeniedPermit, "Delete", name);
    }
    return Finish(kResSucceed, "Delete", name);
}

} // extern "C"
