/* PS2_PTR(T) and PS2_ADDR(x) on the PC (include/ps2_port.h): pointers the
 * game keeps in 32 bits, as DMA tags and its files hold them and as its
 * `(int)p` casts leave them.
 *
 * x86-64 (Linux, Windows, macOS under Rosetta): the game's data lies below
 * 4 GB (ports/pc/hw.cpp), so a pointer's low 32 bits are the pointer and
 * PORT_GUEST_BASE is 0.
 *
 * arm64 macOS: nothing can be mapped below 4 GB there, so the game's memory
 * is the 4 GB window that starts at PORT_GUEST_BASE = 0x100000000, where the
 * executable is loaded (without address randomization: ports/pc/os.cpp).
 * A 32-bit value v != 0 stands for PORT_GUEST_BASE + v, and 0 for the null
 * pointer; truncating a pointer in the window gives its 32-bit value back.
 * The PS2's register addresses (0x10000000 ...) are mapped at the same
 * offsets in the window.
 *
 * No constructors, so structs holding a Ptr32 stay aggregates (C++98). */
#ifndef PORT_PTR32_H
#define PORT_PTR32_H

#include <stdint.h>

#if defined(__aarch64__) || defined(__arm64__)
#define PORT_GUEST_BASE 0x100000000ull
#else
#define PORT_GUEST_BASE 0ull
#endif

/* A hardware address (a register, the scratchpad) as the game writes it. */
#define PORT_HW(addr) (PORT_GUEST_BASE + (addr))

#ifdef __cplusplus
extern "C" void port_ptr32_overflow(const void *p);
extern "C" void port_ptr32_unaligned(const void *p, const void *caller);

inline uint32_t port_ptr32(const void *p)
{
    uintptr_t a = (uintptr_t)p;
    if (!a)
        return 0;
    a -= PORT_GUEST_BASE;
    if (a >> 32)
        port_ptr32_overflow(p);
    return (uint32_t)a;
}

// The address a 32-bit value stands for (0: null).
inline uintptr_t port_guest_addr(uint32_t v)
{
    return v ? (uintptr_t)(PORT_GUEST_BASE + v) : 0;
}

// The same for a value the port reads that may be either a host pointer or
// a 32-bit one (an interpreted register): below 4 GB it is 32-bit.
inline uintptr_t port_host_addr(uint64_t v)
{
    return PORT_GUEST_BASE && v && v < 0x100000000ull ? (uintptr_t)(PORT_GUEST_BASE + v) : (uintptr_t)v;
}

#if defined(__aarch64__) || defined(__arm64__)
#define PS2_ADDR(x) port_guest_addr((uint32_t)(x))
#else
#define PS2_ADDR(x) (x)
#endif

// Every PS2_PTR but a string's is a DMA address the DMAC reads qword-aligned.
template <class T> struct PortPtr32Qword { enum { value = 1 }; };
template <> struct PortPtr32Qword<char> { enum { value = 0 }; };

template <class T> struct Ptr32 {
    uint32_t m_addr;

    operator T *() const { return (T *)port_guest_addr(m_addr); }
    T *operator->() const { return (T *)port_guest_addr(m_addr); }
    Ptr32 &operator=(T *p)
    {
        if (PortPtr32Qword<T>::value && ((uintptr_t)p & 0xF))
            port_ptr32_unaligned(p, __builtin_return_address(0));
        m_addr = port_ptr32(p);
        return *this;
    }
};
#endif

#endif
