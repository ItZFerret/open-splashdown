// libcdvd on the PC: sector reads from the disc image.
//
// The game finds its files through its own table of LSNs (FileIO.cpp), so
// reads go to the image by sector, 2048 bytes each, as the drive served
// them. The image is $SPLASHDOWN_DISC (ports/pc/main.cpp sets it from the
// command line: an image dropped on the executable), else the first .iso or
// .bin beside the executable, else baserom/splashdown.bin. Reads complete at
// once: sceCdSync never waits.
#include "port.h"
#include "port_os.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <dirent.h>
#include <mutex>
#include <vector>
#include <string>
#ifdef _WIN32
#include <io.h>
#define dup _dup
#define fseeko fseeko64
#else
#include <unistd.h>
#endif

namespace {

const unsigned kSector = 2048;

FILE *g_disc;
int g_error;
unsigned g_streamLsn; // the next sector a sceCdSt* stream reads
unsigned g_streamStart; // where the stream was started: its file's first sector

std::string DiscPath()
{
    if (const char *env = getenv("SPLASHDOWN_DISC"); env && *env)
        return env;
    static std::string found;
    static std::once_flag once;
    std::call_once(once, [] {
        std::string dir = port_exe_dir();
        std::vector<std::string> names;
        if (DIR *d = opendir(dir.c_str())) {
            while (dirent *e = readdir(d)) {
                std::string n = e->d_name;
                std::string ext = n.size() > 4 ? n.substr(n.size() - 4) : "";
                std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return (char)tolower(c); });
                if (ext == ".iso" || ext == ".bin")
                    names.push_back(n);
            }
            closedir(d);
        }
        std::sort(names.begin(), names.end());
        if (!names.empty()) {
            found = dir + "/" + names[0];
            port_log("disc image: %s", found.c_str());
        }
    });
    return found.empty() ? "baserom/splashdown.bin" : found;
}

FILE *Disc()
{
    if (g_disc)
        return g_disc;
    std::string path = DiscPath();
    g_disc = fopen(path.c_str(), "rb");
    if (!g_disc)
        port_fatal("Can't open the disc image %s.\n\nPut your Splashdown (NTSC-U, SLUS-20223) .iso in the same folder as the game, "
                   "drop it on the executable, or set SPLASHDOWN_DISC.", path.c_str());
    return g_disc;
}

int ReadSectors(unsigned lsn, unsigned sectors, void *buf)
{
    FILE *f = Disc();
    if (fseeko(f, (long long)lsn * kSector, SEEK_SET) != 0) {
        g_error = 0x32; // SCECdErREAD
        return 0;
    }
    size_t got = fread(buf, kSector, sectors, f);
    if (got < sectors)
        memset((char *)buf + got * kSector, 0, (sectors - got) * kSector);
    g_error = 0;
    return (int)got;
}

} // namespace

// The IOP's own reads (ports/pc/audio: sound banks and streams), from any
// thread: pread on a descriptor of their own. Returns the bytes read.
size_t port_disc_read(uint64_t offset, void *buf, size_t bytes)
{
    static int fd = -1;
    static std::once_flag once;
    std::call_once(once, [] {
        std::string path = DiscPath();
        FILE *f = fopen(path.c_str(), "rb");
        if (!f)
            port_fatal("Can't open the disc image %s.\n\nPut your Splashdown (NTSC-U, SLUS-20223) .iso in the same folder as the game, "
                   "drop it on the executable, or set SPLASHDOWN_DISC.", path.c_str());
        fd = dup(fileno(f));
        fclose(f);
    });
    size_t done = 0;
    while (done < bytes) {
        long long got = port_pread(fd, (char *)buf + done, bytes - done, offset + done);
        if (got <= 0)
            break;
        done += got;
    }
    if (done < bytes)
        memset((char *)buf + done, 0, bytes - done);
    return done;
}

// The first sector of the file the current stream (a movie) was started on.
unsigned port_disc_stream_lsn(void)
{
    return g_streamStart;
}

extern "C" {

int sceCdInit(int mode)
{
    Disc();
    return 1;
}

int sceCdMmode(int media)
{
    return 1;
}

int sceCdDiskReady(int mode)
{
    return 2; // SCECdComplete
}

int sceCdStatus(void)
{
    return 0; // stopped, no error
}

int sceCdRead(unsigned int lsn, unsigned int sectors, void *buf, void *mode)
{
    ReadSectors(lsn, sectors, buf);
    return 1;
}

int sceCdSync(int mode)
{
    return 0; // done
}

int sceCdGetError(void)
{
    return g_error;
}

unsigned int sceCdGetReadPos(void)
{
    return 0;
}

void sceCdBreak(void)
{
}

// Streaming: only the PSS movies use it (PSXMpegStreamFile). Reads continue
// from the last seek.
int sceCdStInit(unsigned int bufmax, unsigned int bankmax, unsigned int iop_bufaddr)
{
    return 1;
}

int sceCdStStart(unsigned int lbn, void *mode)
{
    g_streamStart = lbn;
    g_streamLsn = lbn;
    return 1;
}

int sceCdStSeek(unsigned int lbn)
{
    g_streamLsn = lbn;
    return 1;
}

int sceCdStRead(unsigned int sectors, unsigned int *buf, unsigned int mode, unsigned int *err)
{
    int got = ReadSectors(g_streamLsn, sectors, buf);
    g_streamLsn += got;
    if (err)
        *err = g_error;
    return got;
}

int sceCdStStop(void)
{
    return 1;
}

} // extern "C"
