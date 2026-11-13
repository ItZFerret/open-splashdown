/* Force-included (-include) into every game TU on the PC port. */
#ifndef PORT_PRELUDE_H
#define PORT_PRELUDE_H

#define SPLASHDOWN_PC 1

#include "port_asm.h"
#include "port_ptr32.h"

/* include/ps2_port.h's names on the PC. */
#define EE_UNCACHED 0
#define EE_UNCACHED_ACCEL 0
#define PS2_PTR(T) Ptr32<T>
#define PS2_PTR_SIZE sizeof(void *)
#define PS2_SIZEOF_PTR(T) 4
#define PS2_PTR_INT intptr_t
#define PS2_OVERRUN(n) (n)

#ifdef __cplusplus
extern "C" {
#endif
/* mwcc's break intrinsic (ASSERT_BREAK): stop in the debugger or abort. */
void port_break(int code);
/* ports/pc/hw.cpp: the game's busy-waits on the PC (WaitForVSync's loop; loops
   on the timer), so the host CPU idles and PORT_DETERMINISTIC's virtual time
   can move. */
void port_vsync_wait(void);
void port_time_spin(void);
/* ports/pc/dma.cpp: SyncDmaPath's poll of channel ch (no wait). With
   PORT_DETERMINISTIC it waits for the channel first, so what the game sees
   doesn't depend on how fast the PC draws (getFrameBuffer's water readback
   state machine, IsRenderingComplete). */
void port_dma_poll(int ch);
/* ports/pc/gs/gs.cpp: a write of v to GS CSR's event bits (FINISH 2, VSINT 8,
   ...), which clears those set in v, atomically (write-1-to-clear: a plain
   store to the PC's register memory cleared the others too). */
void port_gs_csr_ack(unsigned int v);
/* ports/pc/settings.cpp: the projection's x scale for the picture's shape,
   (4/3) / aspect: 1 at the original 4:3 (ports/pc/game/widescreen_pc.cpp). */
float port_widescreen_scale(void);
#ifdef __cplusplus
}
#endif
/* The matching build's section placements (__attribute__((section(".sdata")))
   ...) are ELF's: Mach-O and COFF name sections differently, and the PC has
   no use for them. An empty attribute list is left. */
#ifndef __linux__
#define section(name)
#endif
#define __I_break(code) port_break(code)
#define __I_nop() ((void)0)

#endif
