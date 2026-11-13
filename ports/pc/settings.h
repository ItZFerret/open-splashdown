// The port's own display settings (ports/pc/settings.cpp), changed in the
// overlay (ports/pc/overlay.cpp, F1) and kept in <data dir>/settings.ini.
#ifndef PORT_SETTINGS_H
#define PORT_SETTINGS_H

enum { kAspect43, kAspect169, kAspect1610, kAspect219, kAspectFill, kAspectCount };
enum { kScaleSmooth, kScaleSharp, kScaleInteger, kScaleCount };

struct PortSettings {
    int aspect = kAspect43;     // the picture's shape (and the game's field of view)
    int fullscreen = 0;         // borderless, at the desktop's resolution
    int windowScale = 2;        // windowed: the picture's height in 448-line steps, 1-4
    int scaling = kScaleSmooth; // how the 640x448 picture is enlarged
    int vsync = 0;
    int showFps = 0;
};

// The settings (main thread only), loaded from the file at first use.
PortSettings &port_settings(void);
void port_settings_save(void);

// The picture's width / height for an aspect setting in a window of ww x wh
// pixels (kAspectFill: the window's own shape).
float port_aspect_value(int aspect, int ww, int wh);

// Widescreen: the game renders 640x448 whatever the shape it is shown in, so
// a wider picture needs a wider horizontal field of view in the same pixels.
// ProjectionMatrix (ports/pc/game/widescreen_pc.cpp) multiplies its x scale
// by this: (4/3) / aspect, 1 for the original 4:3. Set from the main thread,
// read by the game's.
void port_widescreen_set(float aspect);
extern "C" float port_widescreen_scale(void);

#endif
