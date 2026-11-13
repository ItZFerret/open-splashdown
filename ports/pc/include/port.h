// Shared declarations of the PC platform layer (ports/pc/). C++20.
#ifndef PORT_H
#define PORT_H

#include <cstddef>
#include <cstdint>
#include <cstdio>

// Print a message and abort.
[[noreturn]] void port_fatal(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

// Log one line to stderr, prefixed with "[port] ".
void port_log(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

// Flush stdio and end the process at once, without static destructors or
// atexit handlers (the game's threads are still running).
[[noreturn]] void port_exit(int code);
// PORT_PROFILE=out.txt: write the sampling profiler's samples (ports/pc/profile.cpp).
void port_profile_dump(void);

// An SDK function the port doesn't implement yet: logs its name the first
// time it runs.
#define PORT_STUB()                                                            \
    do {                                                                       \
        static bool logged_;                                                   \
        if (!logged_) {                                                        \
            logged_ = true;                                                    \
            port_log("stub: %s", __func__);                                    \
        }                                                                      \
    } while (0)

// ports/pc/watchdog.cpp: the hang watchdog ("Watchdog and stress
// runs"). Name this thread (dumps, /proc/<pid>/task/<tid>/comm); name a
// futex word, so a thread blocked on it shows what it waits for; dump now.
void port_watchdog_start(void);
void port_thread_name(const char *name);
void port_watch_futex(const void *addr, const char *name);
void port_watchdog_dump(const char *why);
// ports/pc/heapguard.cpp: this thread allocates from the port threads' heap,
// not the game's (port_thread_name does it for every named thread).
void port_heap_this_thread(void);
uintptr_t port_heapguard_end(void);
// Per thread, read by the dump on the thread itself: the inline-asm
// statement being interpreted (ports/pc/ee/asm_interp.cpp), and bit 31 | D_PCR
// while in a bc0f wait (ports/pc/dma.cpp).
extern __thread const char *port_tls_asm;
extern __thread uint32_t port_tls_bc0f;
// Each subsystem's state for the dump.
void port_hw_debug(FILE *f);
void port_dma_debug(FILE *f);
void vif1_debug(FILE *f);
void gs_debug(FILE *f);

// ports/pc/hw.cpp: the PS2 memory map, timers and interrupts.
void port_hw_start(void);
uint64_t port_vblanks(void);
// Run the INTC handler for cause now, on this thread, if it is enabled.
void port_raise_intc(int cause);
// PORT_DETERMINISTIC: virtual time (ports/pc/hw.cpp).
bool port_deterministic(void);
// The game's waits (port_prelude.h): a vblank, a timer mark.
extern "C" void port_vsync_wait(void);
extern "C" void port_time_spin(void);

// ports/pc/dma.cpp: the DMA controller thread.
void port_dma_start(void);
uint64_t port_dma_kicks(int ch);
const char *port_dma_channel_name(int ch);
// Run channel ch's transfer now if the game has started one (from inside
// another transfer: a VIF1 interrupt handler that kicks the GIF).
void port_dma_service(int ch);
// COP0's condition (bc0f/bc0t): every channel D_PCR selects has finished.
bool port_dma_cpcond0(void);
// Sleep until a transfer finishes (bc0f's wait loop).
void port_dma_wait_cpcond0(void);
// Wait until no channel is running (deterministic mode's vblanks).
void port_dma_wait_idle(void);
// Wait until channel ch's transfer (if any) has finished.
void port_dma_wait_channel(int ch);
extern "C" void port_dma_poll(int ch);

// ports/pc/video.cpp: present the GS output (window, or headless screenshots)
// on the main thread until *done.
void port_video_run(volatile bool *done);

// ports/pc/input.cpp: pads from the keyboard, SDL gamepads and PORT_INPUT.
// One DualShock 2's report: buttons active high in libpad's bit order
// (data[2] << 8 | data[3], inverted), sticks 0..255 (0x80 centre),
// pressures in data[8..19]'s order (right left up down triangle circle
// cross square L1 R1 L2 R2).
struct PortPad {
    uint16_t buttons;
    uint8_t rx, ry, lx, ly;
    uint8_t pressure[12];
};
// Main thread, with SDL up: an SDL_Event (gamepads come and go), and the
// state of keyboard and gamepads once per vblank.
void port_input_event(const void *sdlEvent);
void port_input_poll(void);
// Main thread: the game sees neutral pads while blocked (the settings overlay).
void port_input_block(bool blocked);
// Game thread: pads connected (at least 1), pad n's state, its motors.
int port_input_pads(void);
void port_input_read(int pad, PortPad *out);
void port_input_rumble(int pad, uint8_t small, uint8_t large);

// ports/pc/sdk/cdvd.cpp: read the disc image at a byte offset, from any thread
// (the IOP's reads: ports/pc/audio). Returns the bytes read (the rest zeroed).
size_t port_disc_read(uint64_t offset, void *buf, size_t bytes);

// ports/pc/audio: the IOP sound driver and its output.
// main(): open the output (SDL audio, PORT_AUDIO_WAV, PORT_NO_AUDIO).
void port_audio_start(void);
// Deterministic mode: mix one vblank's samples, on the game thread.
void port_audio_vblank(void);
// port_exit: finish the WAV file.
void port_audio_shutdown(void);

// ports/pc/ipu.cpp: while sink is set, DMA ch4 (toIPU) hands its data to it
// instead of the IPU (ports/pc/sdk/mpeg.cpp: libmpeg's bitstream).
void port_ipu_set_movie_sink(void (*sink)(const void *data, size_t bytes));

// ports/pc/sdk/microcode.cpp: the name of the microprogram whose upload chain
// starts at p, or nullptr.
const char *port_microprogram_name(const void *p);
// The microprogram whose upload data contains p, or nullptr.
const char *port_microprogram_containing(const void *p);

#endif
