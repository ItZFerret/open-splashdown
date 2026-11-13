// The PC entry point. PSXMain.cpp's main is compiled as splashdown_main
// (tools/port/build.py) and runs on a thread whose stack lies below 4 GB,
// so addresses of locals survive the game's 32-bit pointer casts (ports/pc/hw.cpp).
#include "port.h"
#include "port_os.h"

#include <cfenv>
#include <cstdio>
#include <cstdlib>
#include <xmmintrin.h>

extern "C" int splashdown_main(int argc, char **argv);
// The game's render switches (debug keys on the PS2)
extern int gRenderWater, RenderEcoSystem, gRenderWaterObjects, RenderSearchedObjects;

namespace {

const size_t kStackSize = 16 << 20;

struct Args {
    int argc;
    char **argv;
    int result;
    volatile bool done;
};

void *GameThread(void *p)
{
    Args *a = (Args *)p;
    port_thread_name("game");
    // PORT_FPE=1: trap invalid operations and division by zero on the game
    // thread (the EE's FPU has no NaN or infinity: a NaN here is a port bug
    // or a value the PS2 would have clamped) PORT_NO_WATER=1,
    // PORT_NO_ECO=1: skip the water / the vegetation
    if (getenv("PORT_NO_WATER"))
        gRenderWater = gRenderWaterObjects = 0;
    if (getenv("PORT_NO_ECO"))
        RenderEcoSystem = 0;
    // PORT_NO_OBJECTS=1: nothing from the visibility search (jetskis,
    // riders, buoys, scenery, vegetation, water): terrain, sky and HUD only
    if (getenv("PORT_NO_OBJECTS"))
        RenderSearchedObjects = 0;
    // PORT_FPE=1 invalid + division by zero, 2 division, 3 invalid, 4 invalid
    // + overflow (where an inf or NaN starts; the EE's FPU has neither)
    if (getenv("PORT_FPE")) {
        static const int kFpe[] = {0, FE_INVALID | FE_DIVBYZERO, FE_DIVBYZERO, FE_INVALID, FE_INVALID | FE_OVERFLOW};
        int mode = atoi(getenv("PORT_FPE"));
        int fe = kFpe[mode >= 1 && mode <= 4 ? mode : 1];
#ifdef __linux__
        feenableexcept(fe);
#else
        // x86-64 float math is SSE: unmask the exceptions in MXCSR
        unsigned mask = _MM_GET_EXCEPTION_MASK();
        if (fe & FE_INVALID) mask &= ~_MM_MASK_INVALID;
        if (fe & FE_DIVBYZERO) mask &= ~_MM_MASK_DIV_ZERO;
        if (fe & FE_OVERFLOW) mask &= ~_MM_MASK_OVERFLOW;
        _MM_SET_EXCEPTION_MASK(mask);
#endif
    }
    a->result = splashdown_main(a->argc, a->argv);
    a->done = true;
    return nullptr;
}

} // namespace

int main(int argc, char **argv)
{
    // An argument is the disc image (dropped on the executable, or
    // `splashdown game.iso`); the game itself takes none.
    if (argc > 1 && argv[1][0]) {
        port_setenv("SPLASHDOWN_DISC", argv[1]);
        argc = 1;
    }
    port_watchdog_start();
    port_hw_start();
    port_dma_start();
    port_audio_start();

    Args args = {argc, argv, 0, false};
    void *thread = port_thread_start_low(GameThread, &args, kStackSize);
    if (!thread)
        port_fatal("can't start the game thread on a stack below 4 GB");
    port_video_run(&args.done);
    port_thread_join(thread);
    port_log("splashdown_main returned %d", args.result);
    port_exit(args.result);
}
