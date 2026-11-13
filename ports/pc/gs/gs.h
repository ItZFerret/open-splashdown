// The Graphics Synthesizer on the PC (ports/pc/gs/): GIF packet parsing, the GS
// registers, 4 MB of local memory with the real pixel-storage layouts, and a
// software rasterizer that draws into that memory, so render targets,
// textures, CLUTs and framebuffer copies read back as on the PS2.
// The gs_* calls come from the DMA thread (ports/pc/dma.cpp) and serialize on
// one lock; primitives are drawn by worker threads behind them.
#ifndef PORT_GS_H
#define PORT_GS_H

#include <cstddef>
#include <cstdint>
#include "port_os.h"
#include <vector>

// Picture buffers live outside the game's brk heap (a frame a vblank would
// churn it): mmap'd.
template <class T> struct PortMmapAllocator {
    using value_type = T;
    PortMmapAllocator() = default;
    template <class U> PortMmapAllocator(const PortMmapAllocator<U> &) {}
    T *allocate(size_t n)
    {
        return (T *)port_map(n * sizeof(T));
    }
    void deallocate(T *p, size_t n) { port_unmap(p, n * sizeof(T)); }
    template <class U> bool operator==(const PortMmapAllocator<U> &) const { return true; }
};
using PortFrame = std::vector<uint32_t, PortMmapAllocator<uint32_t>>;

// The GIF's three inputs: PATH1 (VU1 XGKICK), PATH2 (VIF1 DIRECT), PATH3
// (the GIF DMA channel). Each keeps its own tag state between calls.
enum GsPath { GS_PATH1 = 0, GS_PATH2 = 1, GS_PATH3 = 2 };

// The internal resolution: 1 (the PS2's 640x448), 2 or 3 times it in each
// direction, from the next frame on. The picture gs_read_display gives is
// that much larger; local memory, and what the game reads from it, is not.
void gs_set_hires(int scale);

// Feed n quadwords of a GIF stream.
void gs_gif_write(GsPath path, const void *qwords, size_t n);
// PATH1: one GIF packet (tags up to EOP) from VU1 data memory at qword addr.
// xscale != 1 (widescreen, a 2D microprogram's packet: ports/pc/vu/vu1.cpp):
// its elements' x are scaled by it, so what the presenter stretches comes
// out in its own proportions; about the picture's centre, or with
// anchorEdges about the nearer edge for elements in its left and right
// thirds (the HUD's corner panels).
void gs_gif_kick(const uint8_t *vumem, uint32_t qaddr, float xscale = 1.0f, bool anchorEdges = false);
// Wait until the GS thread has taken everything queued (gs_gif_write/kick
// return before it does).
void gs_sync();
// A local->host transfer (TRXDIR 1): read n quadwords into dst.
void gs_read_image(void *dst, size_t n);

// Serializes gs_* calls between the DMA thread and the presenter.
void gs_lock();
void gs_unlock();

// The displayed picture (PMODE, DISPFB1/2, DISPLAY1/2 as the game wrote them
// to 0x12000000), as RGBA8 rows. Returns false if no circuit is enabled.
bool gs_read_display(PortFrame &rgba, int &w, int &h);

// Finished frames so far: FINISHes with a display enabled (screenshots and
// PORT_INPUT count these; frame seq is the displayed picture as it stood at
// that FINISH, before the game moved DISPFB). Any thread.
uint64_t gs_frame_seq();
// Game frames (Flips: FINISHes after DISPFB moved).
uint64_t gs_flip_count();

// ports/pc/video.cpp, called at each finished frame on the GS's thread: does the
// presenter want frame seq's picture (a window, a screenshot)? If so it gets
// it (gs_read_display) before seq is published.
bool port_video_want(uint64_t seq);
void port_video_frame(uint64_t seq, PortFrame &&rgba, int w, int h);
// A picture buffer the presenter is done with, or an empty one: a frame is
// megabytes at a high internal resolution, so they go round instead of
// being mapped and unmapped for each. Give back one that wasn't handed over.
PortFrame port_video_buffer(void);
void port_video_recycle(PortFrame &&rgba);
// FINISH register writes (the game sends one per Flip).
uint64_t gs_finish_count();
// Primitives drawn so far.
uint64_t gs_prim_count();
// Times the parser waited for the drawing threads.
uint64_t gs_barrier_count();

// Debugging: decode the texture TEX0 describes (CLUT loaded from its CBP).
void gs_read_texture(uint64_t tex0, std::vector<uint32_t> &rgba, int &w, int &h);

#endif
