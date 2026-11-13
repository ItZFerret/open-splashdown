// The port's display settings: defaults, the settings file, and the
// widescreen factor the game's projection reads.
#include "settings.h"

#include "port.h"
#include "port_os.h"

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace {

PortSettings g_settings;
bool g_loaded;
std::atomic<float> g_widescreen{1.0f};

struct Field {
    const char *name;
    int PortSettings::*member;
    int min, max;
};
const Field kFields[] = {
    {"aspect", &PortSettings::aspect, 0, kAspectCount - 1},
    {"fullscreen", &PortSettings::fullscreen, 0, 1},
    {"window_scale", &PortSettings::windowScale, 1, 4},
    {"scaling", &PortSettings::scaling, 0, kScaleCount - 1},
    {"vsync", &PortSettings::vsync, 0, 1},
    {"show_fps", &PortSettings::showFps, 0, 1},
};

std::string Path()
{
    const char *dir = port_data_dir();
    return *dir ? std::string(dir) + "/settings.ini" : "";
}

void Load()
{
    g_loaded = true;
    std::string path = Path();
    FILE *f = path.empty() ? nullptr : fopen(path.c_str(), "r");
    if (!f)
        return;
    char line[256];
    while (fgets(line, sizeof line, f)) {
        char name[64];
        int value;
        if (sscanf(line, " %63[a-z_] = %d", name, &value) != 2)
            continue;
        for (const Field &fd : kFields)
            if (!strcmp(name, fd.name))
                g_settings.*fd.member = std::clamp(value, fd.min, fd.max);
    }
    fclose(f);
}

} // namespace

PortSettings &port_settings(void)
{
    if (!g_loaded)
        Load();
    return g_settings;
}

void port_settings_save(void)
{
    std::string path = Path();
    if (path.empty())
        return;
    port_mkdir(port_data_dir());
    FILE *f = fopen(path.c_str(), "w");
    if (!f) {
        port_log("settings: can't write %s", path.c_str());
        return;
    }
    fputs("# Splashdown PC port: display settings (the F1 menu writes this file)\n", f);
    for (const Field &fd : kFields)
        fprintf(f, "%s = %d\n", fd.name, g_settings.*fd.member);
    fclose(f);
}

float port_aspect_value(int aspect, int ww, int wh)
{
    switch (aspect) {
    case kAspect169: return 16.0f / 9;
    case kAspect1610: return 16.0f / 10;
    case kAspect219: return 21.0f / 9;
    case kAspectFill:
        // any shape from square to three screens side by side
        return wh > 0 ? std::clamp((float)ww / wh, 1.0f, 5.4f) : 4.0f / 3;
    default: return 4.0f / 3;
    }
}

void port_widescreen_set(float aspect)
{
    g_widescreen.store((4.0f / 3) / aspect, std::memory_order_relaxed);
}

extern "C" float port_widescreen_scale(void)
{
    return g_widescreen.load(std::memory_order_relaxed);
}
