/* What shared game source needs to build for the PC port as well as for the
 * PS2. Only macros on the PS2 side: included from d3dtypes.h and
 * soultree_assert_block.h, it adds no declarations to the matching build.
 * On the PC (SPLASHDOWN_PC), ports/pc/include/port_prelude.h defines the same
 * names first.
 *
 * EE_UNCACHED, EE_UNCACHED_ACCEL: the EE address segments the game ORs into
 * pointers, uncached (0x20000000) and uncached accelerated (0x30000000). On
 * the PC the game's data is ordinary memory below 256 MB (ports/pc/hw.cpp), so
 * both are 0 and `p & 0x0FFFFFFF` leaves p as it is.
 *
 * PS2_PTR(T): a pointer kept in a 32-bit field the hardware or a file reads,
 * such as a DMA tag's address. `T *` on the PS2; on the PC a 32-bit
 * Ptr32<T> that converts to and from `T *` (ports/pc/include/port_ptr32.h).
 *
 * PS2_PTR_SIZE: the size of a pointer in memory, where the source wrote the
 * number (`malloc(n * 4)` for an array of pointers): 4 on the PS2,
 * sizeof(void *) on the PC.
 *
 * PS2_SIZEOF_PTR(T): the size of a pointer in the game's files, written
 * `sizeof(T *)` in the source: that on the PS2, 4 on the PC.
 *
 * PS2_PTR_INT: an integer the game keeps pointers in (`int m_offsets[63]`
 * holding 63 pointers): int on the PS2, intptr_t on the PC.
 *
 * PS2_ADDR(x): a pointer kept in a 32-bit integer, where the source casts
 * it back to a pointer (`(T *)PS2_ADDR(x)`): x on the PS2 and on a PC whose
 * game memory lies below 4 GB; where it doesn't (arm64 macOS,
 * ports/pc/include/port_ptr32.h), the address that 32-bit value stands for.
 *
 * PS2_OVERRUN(n): room for n elements retail writes past the end of a local
 * array. 0 on the PS2, so the frame stays retail's (the write lands on the
 * neighbouring stack slot, harmlessly, as it did there); n on the PC, where
 * the stack protector would abort. */
#ifndef PS2_PORT_H
#define PS2_PORT_H

#ifndef SPLASHDOWN_PC
#define EE_UNCACHED 0x20000000
#define EE_UNCACHED_ACCEL 0x30000000
#define PS2_PTR(T) T *
#define PS2_PTR_SIZE 4
#define PS2_SIZEOF_PTR(T) sizeof(T *)
#define PS2_PTR_INT int
#define PS2_ADDR(x) (x)
#define PS2_OVERRUN(n) 0
#endif

#endif
