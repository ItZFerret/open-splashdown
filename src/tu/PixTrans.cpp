// PixTrans.cpp  @ 0x002668D0 (text 0x2668D0-0x26B1B0)
// MATCHED (mwccps2 2.4.1.01, -O4,p -Cpp_exceptions off -sdatathreshold 1 -str readonly, mode: cpp) — byte-identical to retail.
//
// A retail translation unit without a static initializer: the pixel format
// converters, mip scalers and DitherImage. It has no .rodata; its .bss is
// AllocatedRowSize (0x473F10, between PhysicsRigidBody.cpp's axis vectors and
// PSXAudio.cpp's). The three PixelChange*to96bpp widening converters are
// static: only DitherImage calls them.



#include "RGBTripple/RGBTripple.h"
#include "RGBQuad/RGBQuad.h"
#include "ColorMapper/ColorMapper.h"

// ---- src/global/PixelChange24to96bpp.cpp

struct RGBTrippleInt {
    int r, g, b;
};

// Widens 8-bit channels to 16.16 fixed point.
static void PixelChange24to96bpp(RGBTripple *src, RGBTrippleInt *dst, int count)
{
    do {
        dst->r = src->r << 16;
        dst->g = src->g << 16;
        dst->b = src->b << 16;
        count--;
        dst++;
        src++;
    } while (count);
}


// ---- src/global/PixelChange16to96bpp.cpp

// Widens 5:6:5 channels to 16.16 fixed point.
static void PixelChange16to96bpp(unsigned short *src, RGBTrippleInt *dst, int count)
{
    do {
        dst->r = (*src & 0xF800) << 8;
        dst->g = (*src & 0x07E0) << 13;
        dst->b = (*src & 0x001F) << 19;
        count--;
        dst++;
        src++;
    } while (count);
}


// ---- src/global/PixelChange15to96bpp.cpp

// Widens 5:5:5 channels to 16.16 fixed point.
static void PixelChange15to96bpp(unsigned short *src, RGBTrippleInt *dst, int count)
{
    do {
        dst->r = (*src & 0x7C00) << 9;
        dst->g = (*src & 0x03E0) << 14;
        dst->b = (*src & 0x001F) << 19;
        count--;
        dst++;
        src++;
    } while (count);
}


// ---- src/global/MipScale888.cpp

// Halves an 888 image: each output pixel is the average of a 2x2 block.
int MipScale888(RGBTripple *dst, RGBTripple *src, int width, int height, int dstPitch, int srcPitch)
{
    int y, x;
    for (y = 0; y < height; y++) {
        RGBTripple *s = src;
        RGBTripple *d = dst;
        for (x = 0; x < width; x++) {
            d->r = (s[0].r + s[1].r + s[srcPitch].r + s[srcPitch + 1].r) >> 2;
            d->g = (s[0].g + s[1].g + s[srcPitch].g + s[srcPitch + 1].g) >> 2;
            d->b = (s[0].b + s[1].b + s[srcPitch].b + s[srcPitch + 1].b) >> 2;
            d++;
            s += 2;
        }
        src += srcPitch + srcPitch;
        dst += dstPitch;
    }
    return 1;
}


// ---- src/global/MipScale8888.cpp

// Halves an 8888 image: each output pixel averages the pixels of its 2x2
// block that have a non-zero alpha.
int MipScale8888(RGBQuad *dst, RGBQuad *src, int width, int height, int dstPitch, int srcPitch)
{
    int y, x;
    for (y = 0; y < height; y++) {
        RGBQuad *s = src;
        RGBQuad *d = dst;
        for (x = 0; x < width; x++) {
            int n = 0, r = 0, g = 0, b = 0, a = 0;
            unsigned char alpha;
            alpha = s[0].a;
            if (alpha) {
                a += alpha;
                n++;
                r += s[0].r;
                g += s[0].g;
                b += s[0].b;
            }
            alpha = s[1].a;
            if (alpha) {
                a += alpha;
                n++;
                r += s[1].r;
                g += s[1].g;
                b += s[1].b;
            }
            alpha = s[srcPitch].a;
            if (alpha) {
                r += s[srcPitch].r;
                g += s[srcPitch].g;
                b += s[srcPitch].b;
                a += alpha;
                n++;
            }
            if (s[srcPitch + 1].a) {
                // Retail forgets to reload alpha here: this adds the alpha
                // of the pixel above.
                a += alpha;
                n++;
                r += s[srcPitch + 1].r;
                g += s[srcPitch + 1].g;
                b += s[srcPitch + 1].b;
            }
            switch (n) {
            case 0:
                d->r = 0;
                d->g = 0;
                d->b = 0;
                d->a = 0;
                break;
            case 1:
                d->r = r;
                d->g = g;
                d->b = b;
                d->a = a;
                break;
            case 2:
                d->r = r >> 1;
                d->g = g >> 1;
                d->b = b >> 1;
                d->a = a >> 1;
                break;
            case 3:
                d->r = r / 3;
                d->g = g / 3;
                d->b = b / 3;
                d->a = a / 3;
                break;
            case 4:
                d->r = r >> 2;
                d->g = g >> 2;
                d->b = b >> 2;
                d->a = a >> 2;
                break;
            }
            d++;
            s += 2;
        }
        src += srcPitch + srcPitch;
        dst += dstPitch;
    }
    return 1;
}


// ---- src/global/MipScale4444.cpp
// Halves a 4444 image: each output pixel averages a 2x2 block, channel by
// channel. With bit 0 of flags set, the channels are summed in parallel,
// two per word (that path never advances its source pointer and runs the
// row loop height times; reproduced from retail).
int MipScale4444(unsigned short *dst, unsigned short *src, int width, int height, int dstPitch, int srcPitch,
                 unsigned int flags)
{
    if (flags & 1) {
        // Declaration order chosen for retail register allocation.
        int y;
        int n;
        int p2;
        int p1;
        int p0;
        int c;
        unsigned short *line;
        int p3;
        line = dst;
        for (y = 0; y < height; y++) {
            dst = line;
            n = height;
            while (n--) {
                p0 = src[0];
                p1 = src[1];
                p2 = src[srcPitch];
                p3 = src[srcPitch + 1];
                c = (p0 & 0xF0F0) << 12 | (p0 & 0xF0F);
                c += (p1 & 0xF0F0) << 12 | (p1 & 0xF0F);
                c += (p2 & 0xF0F0) << 12 | (p2 & 0xF0F);
                c += (p3 & 0xF0F0) << 12 | (p3 & 0xF0F);
                c >>= 2;
                *dst++ = (c & 0xF0F0000) >> 12 | (c & 0xF0F);
            }
            src += srcPitch + srcPitch;
            line += dstPitch;
        }
    } else {
        // Declaration order chosen for retail register allocation.
        unsigned short p0;
        int r;
        unsigned short *s;
        unsigned short p2;
        int a;
        unsigned short p3;
        unsigned short p1;
        int g;
        unsigned short *d;
        int b;
        int y;
        int x;
        for (y = 0; y < height; y++) {
            s = src;
            d = dst;
            for (x = 0; x < width; x++) {
                p0 = s[0];
                p1 = s[1];
                p2 = s[srcPitch];
                p3 = s[srcPitch + 1];
                a = ((p0 & 0xF000) >> 10) + ((p1 & 0xF000) >> 10) + ((p2 & 0xF000) >> 10) + ((p3 & 0xF000) >> 10);
                r = ((p0 & 0xF00) >> 6) + ((p1 & 0xF00) >> 6) + ((p2 & 0xF00) >> 6) + ((p3 & 0xF00) >> 6);
                g = ((p0 & 0xF0) >> 2) + ((p1 & 0xF0) >> 2) + ((p2 & 0xF0) >> 2) + ((p3 & 0xF0) >> 2);
                b = (p0 & 0xF) + (p1 & 0xF) + (p2 & 0xF) + (p3 & 0xF);
                *d++ = (a + 2) >> 4 << 12 | (r + 2) >> 4 << 8 | (g + 2) >> 4 << 4 | ((b << 2) + 2) >> 4;
                s += 2;
            }
            src += srcPitch + srcPitch;
            dst += dstPitch;
        }
    }
    return 1;
}


// ---- src/global/MipScale1555.cpp
// Halves a 1555 image: each output pixel averages the pixels of its 2x2
// block that have the alpha bit set; fewer than two such pixels give 0.
int MipScale1555(unsigned short *dst, unsigned short *src, int width, int height, int dstPitch, int srcPitch)
{
    // Declaration and initialisation order chosen for retail register allocation.
    unsigned short *s;
    unsigned short *d;
    unsigned short p1;
    unsigned short p2;
    unsigned short p3;
    int r;
    int g;
    int b;
    int y;
    int n;
    int x;
    unsigned short p0;
    for (y = 0; y < height; y++) {
        s = src;
        d = dst;
        for (x = 0; x < width; x++) {
            p0 = s[0];
            p1 = s[1];
            p2 = s[srcPitch];
            p3 = s[srcPitch + 1];
            n = 0;
            b = 0;
            g = 0;
            r = 0;
            if (p0 & 0x8000) {
                r += (p0 & 0x7C00) >> 7;
                g += (p0 & 0x3E0) >> 2;
                b += (p0 & 0x1F) << 3;
                n++;
            }
            if (p1 & 0x8000) {
                r += (p1 & 0x7C00) >> 7;
                g += (p1 & 0x3E0) >> 2;
                b += (p1 & 0x1F) << 3;
                n++;
            }
            if (p2 & 0x8000) {
                r += (p2 & 0x7C00) >> 7;
                g += (p2 & 0x3E0) >> 2;
                b += (p2 & 0x1F) << 3;
                n++;
            }
            if (p3 & 0x8000) {
                r += (p3 & 0x7C00) >> 7;
                g += (p3 & 0x3E0) >> 2;
                b += (p3 & 0x1F) << 3;
                n++;
            }
            if (n >= 2) {
                r = (r + n * 4) / n;
                g = (g + n * 4) / n;
                b = (b + n * 4) / n;
                *d++ = (short)(unsigned short)((r >> 3) << 10 | (g >> 3) << 5 | b >> 3) | 0x8000;
            } else {
                *d++ = 0;
            }
            s += 2;
        }
        src += srcPitch + srcPitch;
        dst += dstPitch;
    }
    return 1;
}


// ---- src/global/MipScale555.cpp
// Halves a 555 image: each output pixel averages a 2x2 block, channel by
// channel. Bit 0 of flags sums the channels in parallel, two per word (that
// path never advances its source pointer and runs the row loop height times;
// reproduced from retail). Bit 1 skips pixels equal to the colour key, and a
// block with no other pixels gives the key.
int MipScale555(unsigned short *dst, unsigned short *src, int width, int height, int dstPitch, int srcPitch,
                unsigned int flags, unsigned short key)
{
    // Declaration order and scopes chosen for retail register allocation.
    unsigned short *s;
    unsigned short *d;
    unsigned short p0;
    unsigned short p1;
    unsigned short p2;
    unsigned short p3;
    int r;
    int g;
    int b;
    int y;
    int n;
    int x;
    if (flags & 1) {
        int y;
        int n;
        int p2;
        int p1;
        int p0;
        int c;
        unsigned short *line;
        int p3;
        line = dst;
        for (y = 0; y < height; y++) {
            dst = line;
            n = height;
            while (n--) {
                p0 = src[0];
                p1 = src[1];
                p2 = src[srcPitch];
                p3 = src[srcPitch + 1];
                c = (p0 & 0x3E0) << 15 | (p0 & 0x7C1F);
                c += (p1 & 0x3E0) << 15 | (p1 & 0x7C1F);
                c += (p2 & 0x3E0) << 15 | (p2 & 0x7C1F);
                c += (p3 & 0x3E0) << 15 | (p3 & 0x7C1F);
                c >>= 2;
                *dst++ = (c & 0x1F00000) >> 15 | (c & 0x7C1F);
            }
            src += srcPitch + srcPitch;
            line += dstPitch;
        }
    } else if (flags & 2) {
        for (y = 0; y < height; y++) {
            s = src;
            d = dst;
            for (x = 0; x < width; x++) {
                p0 = s[0];
                p1 = s[1];
                p2 = s[srcPitch];
                p3 = s[srcPitch + 1];
                n = 0;
                b = 0;
                g = 0;
                r = 0;
                if (p0 != key) {
                    r += (p0 & 0x7C00) >> 7;
                    g += (p0 & 0x3E0) >> 2;
                    b += (p0 & 0x1F) << 3;
                    n++;
                }
                if (p1 != key) {
                    r += (p1 & 0x7C00) >> 7;
                    g += (p1 & 0x3E0) >> 2;
                    b += (p1 & 0x1F) << 3;
                    n++;
                }
                if (p2 != key) {
                    r += (p2 & 0x7C00) >> 7;
                    g += (p2 & 0x3E0) >> 2;
                    b += (p2 & 0x1F) << 3;
                    n++;
                }
                if (p3 != key) {
                    r += (p3 & 0x7C00) >> 7;
                    g += (p3 & 0x3E0) >> 2;
                    b += (p3 & 0x1F) << 3;
                    n++;
                }
                if (n > 0) {
                    r = (r + n * 4) / n;
                    g = (g + n * 4) / n;
                    b = (b + n * 4) / n;
                    *d++ = (r >> 3) << 10 | (g >> 3) << 5 | b >> 3;
                } else {
                    *d++ = key;
                }
                s += 2;
            }
            src += srcPitch + srcPitch;
            dst += dstPitch;
        }
    } else {
        int y;
        int x;
        for (y = 0; y < height; y++) {
            s = src;
            d = dst;
            for (x = 0; x < width; x++) {
                p0 = s[0];
                p1 = s[1];
                p2 = s[srcPitch];
                p3 = s[srcPitch + 1];
                r = ((p0 & 0x7C00) >> 7) + ((p1 & 0x7C00) >> 7) + ((p2 & 0x7C00) >> 7) + ((p3 & 0x7C00) >> 7);
                g = ((p0 & 0x3E0) >> 2) + ((p1 & 0x3E0) >> 2) + ((p2 & 0x3E0) >> 2) + ((p3 & 0x3E0) >> 2);
                b = (p0 & 0x1F) + (p1 & 0x1F) + (p2 & 0x1F) + (p3 & 0x1F);
                *d++ = (r + 2) >> 5 << 10 | (g + 2) >> 5 << 5 | ((b << 3) + 2) >> 5;
                s += 2;
            }
            src += srcPitch + srcPitch;
            dst += dstPitch;
        }
    }
    return 1;
}


// ---- src/global/MipScale565.cpp
// Halves a 565 image: each output pixel averages a 2x2 block, channel by
// channel. Bit 0 of flags sums the channels in parallel, two per word (that
// path never advances its source pointer and runs the row loop height times;
// reproduced from retail). Bit 1 skips pixels equal to the colour key, and a
// block with no other pixels gives the key.
int MipScale565(unsigned short *dst, unsigned short *src, int width, int height, int dstPitch, int srcPitch,
                unsigned int flags, unsigned short key)
{
    // Declaration order and scopes chosen for retail register allocation.
    unsigned short *s;
    unsigned short *d;
    unsigned short p0;
    unsigned short p1;
    unsigned short p2;
    unsigned short p3;
    int r;
    int g;
    int b;
    int y;
    int n;
    int x;
    if (flags & 1) {
        int y;
        int n;
        int p2;
        int p1;
        int p0;
        int c;
        unsigned short *line;
        int p3;
        line = dst;
        for (y = 0; y < height; y++) {
            dst = line;
            n = height;
            while (n--) {
                p0 = src[0];
                p1 = src[1];
                p2 = src[srcPitch];
                p3 = src[srcPitch + 1];
                c = (p0 & 0x7E0) << 15 | (p0 & 0xF81F);
                c += (p1 & 0x7E0) << 15 | (p1 & 0xF81F);
                c += (p2 & 0x7E0) << 15 | (p2 & 0xF81F);
                c += (p3 & 0x7E0) << 15 | (p3 & 0xF81F);
                c >>= 2;
                *dst++ = (c & 0x3F00000) >> 15 | (c & 0xF81F);
            }
            src += srcPitch + srcPitch;
            line += dstPitch;
        }
    } else if (flags & 2) {
        for (y = 0; y < height; y++) {
            s = src;
            d = dst;
            for (x = 0; x < width; x++) {
                p0 = s[0];
                p1 = s[1];
                p2 = s[srcPitch];
                p3 = s[srcPitch + 1];
                n = 0;
                b = 0;
                g = 0;
                r = 0;
                if (p0 != key) {
                    r += (p0 & 0xF800) >> 8;
                    g += (p0 & 0x7E0) >> 3;
                    b += (p0 & 0x1F) << 3;
                    n++;
                }
                if (p1 != key) {
                    r += (p1 & 0xF800) >> 8;
                    g += (p1 & 0x7E0) >> 3;
                    b += (p1 & 0x1F) << 3;
                    n++;
                }
                if (p2 != key) {
                    r += (p2 & 0xF800) >> 8;
                    g += (p2 & 0x7E0) >> 3;
                    b += (p2 & 0x1F) << 3;
                    n++;
                }
                if (p3 != key) {
                    r += (p3 & 0xF800) >> 8;
                    g += (p3 & 0x7E0) >> 3;
                    b += (p3 & 0x1F) << 3;
                    n++;
                }
                if (n > 0) {
                    r = (r + n * 4) / n;
                    g = (g + n * 2) / n;
                    b = (b + n * 4) / n;
                    *d++ = (r >> 3) << 11 | (g >> 2) << 5 | b >> 3;
                } else {
                    *d++ = key;
                }
                s += 2;
            }
            src += srcPitch + srcPitch;
            dst += dstPitch;
        }
    } else {
        int y;
        int x;
        for (y = 0; y < height; y++) {
            s = src;
            d = dst;
            for (x = 0; x < width; x++) {
                p0 = s[0];
                p1 = s[1];
                p2 = s[srcPitch];
                p3 = s[srcPitch + 1];
                r = ((p0 & 0xF800) >> 8) + ((p1 & 0xF800) >> 8) + ((p2 & 0xF800) >> 8) + ((p3 & 0xF800) >> 8);
                g = ((p0 & 0x7E0) >> 3) + ((p1 & 0x7E0) >> 3) + ((p2 & 0x7E0) >> 3) + ((p3 & 0x7E0) >> 3);
                b = (p0 & 0x1F) + (p1 & 0x1F) + (p2 & 0x1F) + (p3 & 0x1F);
                *d++ = (r + 8) >> 5 << 11 | (g + 4) >> 4 << 5 | ((b << 3) + 8) >> 5;
                s += 2;
            }
            src += srcPitch + srcPitch;
            dst += dstPitch;
        }
    }
    return 1;
}


// ---- src/global/MipScale8.cpp
// MipScale8__FPUcPUciiiiP11ColorMapper  @ 0x00267A30
// Register allocation notes (rounds 19-27): c1..c3 are re-read in the key compares (s[1] etc.)
// so IroCSE makes them the lowest-numbered temps; c0/r0/g0/b0 become codegen temps through
// backend copy propagation. The keyed loop's local declaration order fixes the $t/$s
// colouring (r declared first, g1 next to r1), and zeroing the accumulators before the palette
// loads keeps the pixel-0/1 palette pointers live alongside them, which puts both pointers
// in $s1 as retail does.



// Halves an 8-bit palettised image: each output pixel is the palette index
// (through the 555 lookup) of the average colour of a 2x2 block. When the
// palette has a magenta (255, 0, 255) key, key pixels are left out of the
// average and a block with fewer than two opaque pixels stays the key.
int MipScale8(unsigned char *dst, unsigned char *src, int width, int height, int dstPitch,
              int srcPitch, ColorMapper *mapper)
{
    unsigned char *srcRow = src;
    unsigned char *dstRow = dst;
    int key = -1;
    unsigned char *lookup;
    RGBTripple *palette;
    int i;
    unsigned char c1;
    unsigned char c2;
    unsigned char c3;
    unsigned char g0;
    unsigned char r0;
    int c0;
    int x;
    unsigned char *s;
    unsigned char *d;

    if (!mapper || !(lookup = mapper->GetLookup15()) || !(palette = (RGBTripple *)mapper->GetPalette()))
        return 0;

    for (i = 0; i < 256; i++) {
        if (palette[i].r == 0xFF && palette[i].g == 0 && palette[i].b == 0xFF) {
            key = i;
            break;
        }
    }

    if (key != -1) {
        for (i = 0; i < height; i++) {
            s = srcRow;
            d = dstRow;
            for (x = 0; x < width; x++) {
                int r;
                int g;
                int b;
                int n;
                unsigned char b3;
                unsigned char b0;
                unsigned char r1;
                unsigned char g1;
                unsigned char b1;
                unsigned char r2;
                unsigned char g2;
                unsigned char b2;
                unsigned char r3;
                unsigned char g3;
                c0 = s[0];
                c1 = s[1];
                c2 = s[srcPitch];
                c3 = s[srcPitch + 1];
                r = g = b = n = 0;
                r0 = palette[c0].r;
                g0 = palette[c0].g;
                b0 = palette[c0].b;
                r1 = palette[c1].r;
                g1 = palette[c1].g;
                b1 = palette[c1].b;
                r2 = palette[c2].r;
                g2 = palette[c2].g;
                b2 = palette[c2].b;
                r3 = palette[c3].r;
                g3 = palette[c3].g;
                b3 = palette[c3].b;
                if (c0 != key) { r += r0; g += g0; b += b0; n++; }
                if (s[1] != key) { r += r1; g += g1; b += b1; n++; }
                if (key != s[srcPitch]) { r += r2; g += g2; b += b2; n++; }
                if (key != s[srcPitch + 1]) { r += r3; g += g3; b += b3; n++; }
                if (n >= 2) {
                    r = (r + n * 4) / n;
                    g = (g + n * 4) / n;
                    b = (b + n * 4) / n;
                    *d++ = lookup[(r >> 3) << 10 | (g >> 3) << 5 | b >> 3];
                } else {
                    *d++ = key;
                }
                s += 2;
            }
            srcRow += srcPitch + srcPitch;
            dstRow += dstPitch;
        }
    } else {
        for (i = 0; i < height; i++) {
            s = srcRow;
            d = dstRow;
            for (x = 0; x < width; x++) {
                RGBTripple *p0 = &palette[s[0]];
                RGBTripple *p1 = &palette[s[1]];
                RGBTripple *p2 = &palette[s[srcPitch]];
                RGBTripple *p3 = &palette[s[srcPitch + 1]];
                int r = p0->r + p1->r + p2->r + p3->r;
                int g = p0->g + p1->g + p2->g + p3->g;
                int b = p0->b + p1->b + p2->b + p3->b;
                *d++ = lookup[((r + 16) >> 5) << 10 | ((g + 16) >> 5) << 5 | (b + 16) >> 5];
                s += 2;
            }
            srcRow += srcPitch + srcPitch;
            dstRow += dstPitch;
        }
    }
    return 1;
}


// ---- src/global/DitherImage.cpp
// Shapes retail needs (rounds 22-26):
//  - `src = (void *)((int)src + step)` at the bottom: an integer add the CSE doesn't merge
//    with the top's nextSrc (retail recomputes it there and doesn't store src at the top);
//  - the x loop indexes cur[x] (strength-reduced pointer) instead of cur++: the load of
//    Row96[y % 2] then comes last before the `if` and fills the beq delay slot;
//  - the three post-error clamps are ?: (Clamp()), whose join labels don't get .align 8;
//  - Quantize as a macro, with all x-loop locals declared up front in the order below;
//  - `for (x = 0; x < width; x++, next++)` puts the cur update before next's, as retail;
//  - `int y, x;` before lookup/palette fixes the y/lookup stack slots;
// - the green and blue errors reuse dg/db: as second live ranges of those
//    variables they become web-split temps, numbered below every named local, which is
//    the colouring order retail's registers need (eg $v1 and eb $a0 before the diffusion
//    temporaries in $a1). Named `eg`/`eb` locals can't reach it.

extern "C" void *malloc(unsigned int size);
extern "C" void free(void *p);




int BitsPerPixel(int format);

int AllocatedRowSize;                   // .bss 0x473F10
RGBTrippleInt *Row96[2] = { 0, 0 };     // .data 0x3FB6D0, the two dither rows

extern "C" int abs(int x);

// Rounds an 8.16 fixed-point channel to the nearest 5-bit level.
static inline int Quantize(int c)
{
    int q;
    int step = 0x80000;
    if (c < 0)
        return 0;
    if (c >= 0x1000000)
        return 0xFFFFFF;
    q = c / step * step;
    if (c - q > step / 2)
        q += step;
    return q;
}

#define QUANTIZE(q, c)                    \
    if ((c) < 0)                          \
        q = 0;                            \
    else if ((c) >= 0x1000000)            \
        q = 0xFFFFFF;                     \
    else {                                \
        q = (c) / 0x80000 * 0x80000;      \
        if ((c) - q > 0x80000 / 2)        \
            q += 0x80000;                 \
    }
static inline int Clamp(int c) { return c >= 0x1000000 ? 0xFFFFFF : c; }

// Floyd-Steinberg dithers an image down to 15/16-bit (and optionally to the
// mapper's 8-bit palette), writing any of the requested destinations.
int DitherImage(void *src, int srcFormat, int width, int height, int srcPitch, int dstPitch, RGBTripple *dst24,
                unsigned short *dst565, unsigned short *dst555, unsigned char *dst8, ColorMapper *mapper)
{
    int bpp = BitsPerPixel(srcFormat);
    int y, x;
    unsigned char *lookup;
    RGBTripple *palette;

    if (mapper) {
        lookup = mapper->GetLookup15();
        palette = (RGBTripple *)mapper->GetPalette();
    }
    if (AllocatedRowSize < width) {
        if (Row96[0])
            free(Row96[0]);
        AllocatedRowSize = 0;
        if (!(Row96[0] = (RGBTrippleInt *)malloc(width * 2 * sizeof(RGBTrippleInt))))
            return 0;
        AllocatedRowSize = width;
        Row96[1] = Row96[0] + width;
    }
    if (srcFormat == 0x22B)
        PixelChange15to96bpp((unsigned short *)src, Row96[0], width);
    else if (srcFormat == 0x235)
        PixelChange16to96bpp((unsigned short *)src, Row96[0], width);
    else
        PixelChange24to96bpp((RGBTripple *)src, Row96[0], width);
    for (y = 0; y < height; y++) {
        RGBTrippleInt *cur = Row96[y % 2];
        RGBTrippleInt *next = Row96[1 - y % 2];
        void *nextSrc = (char *)src + (srcPitch * bpp >> 3);
        RGBTripple *d24 = dst24 + y * dstPitch;
        unsigned short *d565 = dst565 + y * dstPitch;
        unsigned short *d555 = dst555 + y * dstPitch;
        unsigned char *d8 = dst8 + y * dstPitch;
        if (y != height - 1) {
            if (srcFormat == 0x22B)
                PixelChange15to96bpp((unsigned short *)nextSrc, next, width);
            else if (srcFormat == 0x235)
                PixelChange16to96bpp((unsigned short *)nextSrc, next, width);
            else
                PixelChange24to96bpp((RGBTripple *)nextSrc, next, width);
        }
        for (x = 0; x < width; x++, next++) {
            int dg;
            int db;
            int R;
            int G;
            int B;
            int r;
            int g;
            int b;
            unsigned char index;
            int er;
            int qg;
            int qr;
            int qb;
            RGBTripple *c;
            int pr;
            int pg;
            int pb;
            int dr;
            r = cur[x].r;
            QUANTIZE(R, r)
            g = cur[x].g;
            QUANTIZE(G, g)
            b = cur[x].b;
            QUANTIZE(B, b)
            if (mapper) {
                qg = Clamp(G);
                qr = Clamp(R);
                qb = Clamp(B);
                index = lookup[(unsigned short)((qr >> 19) << 10 | (qg >> 19) << 5 | qb >> 19)];
                c = &palette[index];
                pr = c->r << 16;
                pg = c->g << 16;
                pb = c->b << 16;
                dg = g - pg;
                dr = r - pr;
                db = b - pb;
                if (abs(dr) <= 0 && abs(dg) <= 0 && abs(db) <= 0) {
                    R = pr;
                    G = pg;
                    B = pb;
                }
            }
            er = cur[x].r - R;
            // The green/blue errors live in dg/db (see the header).
            db = cur[x].b - B;
            dg = cur[x].g - G;
            cur[x].r = Clamp(R);
            cur[x].g = Clamp(G);
            cur[x].b = Clamp(B);
            cur[x].r = cur[x].r < 0 ? 0 : cur[x].r;
            cur[x].g = cur[x].g < 0 ? 0 : cur[x].g;
            cur[x].b = cur[x].b < 0 ? 0 : cur[x].b;
            if (dst24) {
                d24->r = cur[x].r >> 16;
                d24->g = cur[x].g >> 16;
                d24->b = cur[x].b >> 16;
                d24++;
            }
            if (dst565) {
                *d565 = (cur[x].r >> 19) << 11 | (cur[x].g >> 18) << 5 | cur[x].b >> 19;
                d565++;
            }
            if (dst555) {
                *d555 = (cur[x].r >> 19) << 10 | (cur[x].g >> 19) << 5 | cur[x].b >> 19;
                d555++;
            }
            if (dst8) {
                *d8 = index;
                d8++;
            }
            if (x < width - 1) {
                cur[x + 1].r += er * 7 / 16;
                next[1].r += er - (er * 5 / 16 + (er * 7 / 16 + er * 3 / 16));
                cur[x + 1].g += dg * 7 / 16;
                next[1].g += dg - (dg * 5 / 16 + (dg * 7 / 16 + dg * 3 / 16));
                cur[x + 1].b += db * 7 / 16;
                next[1].b += db - (db * 5 / 16 + (db * 7 / 16 + db * 3 / 16));
            }
            next->r += er * 5 / 16;
            next->g += dg * 5 / 16;
            next->b += db * 5 / 16;
            if (x > 0) {
                next[-1].r += er * 3 / 16;
                next[-1].g += dg * 3 / 16;
                next[-1].b += db * 3 / 16;
            }
        }
        src = (void *)PS2_ADDR(((int)src + (srcPitch * bpp >> 3)));
    }
    return 1;
}


// ---- src/global/PixTrans888from565.cpp


int PixTrans888from565(RGBTripple *dst, unsigned short *src, int width, int height, int dstPad, int srcPad)
{
    // Declaration order fixes the register assignment.
    unsigned short *s;
    RGBTripple *d;
    int y;
    int x;

    for (y = 0; y < height; y++) {
        s = src;
        d = dst;
        for (x = 0; x < width; x++) {
            d->r = (*s & 0xF800) >> 8;
            d->g = (*s & 0x7E0) >> 3;
            d->b = (*s & 0x1F) << 3;
            d++;
            s++;
        }
        src += srcPad;
        dst += dstPad;
    }
    return 1;
}


// ---- src/global/PixTrans888from888.cpp
extern "C" void *memcpy(void *dst, const void *src, unsigned int n);
extern "C" void *malloc(unsigned int size);
extern "C" void free(void *p);


int MipScale888(RGBTripple *dst, RGBTripple *src, int width, int height, int dstPitch, int srcPitch);

// Copy a 888 image, box-filtering it down mipLevel times first.
int PixTrans888from888(RGBTripple *dst, RGBTripple *src, int width, int height, int dstPitch, int srcPitch, int mipLevel)
{
    if (mipLevel == 0) {
        for (int y = 0; y < height; y++)
            memcpy(dst + y * dstPitch, src + y * srcPitch, width * 3);
    } else if (mipLevel == 1) {
        MipScale888(dst, src, width, height, dstPitch, srcPitch);
    } else {
        int mipWidth = width << (mipLevel - 1);
        int mipHeight = height << (mipLevel - 1);
        RGBTripple *buffer = (RGBTripple *)malloc(mipWidth * mipHeight * 3);
        MipScale888(buffer, src, mipWidth, mipHeight, mipWidth, srcPitch);
        for (int i = 2; i < mipLevel; i++) {
            mipWidth /= 2;
            mipHeight /= 2;
            MipScale888(buffer, buffer, mipWidth, mipHeight, mipWidth, mipWidth * 2);
        }
        MipScale888(dst, buffer, width, height, dstPitch, mipWidth);
        free(buffer);
    }
    return 1;
}


// ---- src/global/PixTrans8888from8888.cpp
extern "C" void *memcpy(void *dst, const void *src, unsigned int n);
extern "C" void *malloc(unsigned int size);
extern "C" void free(void *p);


int MipScale8888(RGBQuad *dst, RGBQuad *src, int width, int height, int dstPitch, int srcPitch);

// Copy an 8888 image, box-filtering it down mipLevel times first.
int PixTrans8888from8888(RGBQuad *dst, RGBQuad *src, int width, int height, int dstPitch, int srcPitch, int mipLevel)
{
    RGBQuad *buffer;

    if (mipLevel == 0) {
        for (int y = 0; y < height; y++)
            memcpy(dst + y * dstPitch, src + y * srcPitch, width * 4);
    } else if (mipLevel == 1) {
        MipScale8888(dst, src, width, height, dstPitch, srcPitch);
    } else {
        int mipWidth = width << (mipLevel - 1);
        int mipHeight = height << (mipLevel - 1);
        buffer = (RGBQuad *)malloc(mipWidth * mipHeight * 4);
        if (!buffer)
            goto fail;
        MipScale8888(buffer, src, mipWidth, mipHeight, mipWidth, srcPitch);
        for (int i = 2; i < mipLevel; i++) {
            mipWidth /= 2;
            mipHeight /= 2;
            MipScale8888(buffer, buffer, mipWidth, mipHeight, mipWidth, mipWidth * 2);
        }
        MipScale8888(dst, buffer, width, height, dstPitch, mipWidth);
        free(buffer);
    }
    return 1;

fail:
    if (buffer)
        free(buffer);
    return 0;
}


// ---- src/global/Construct8888From888.cpp


// Expands 888 pixels to 8888, making pixels of the color key transparent.
void Construct8888From888(RGBQuad *dst, RGBTripple *src, int width, int height,
                          int dstPitch, int srcPitch, unsigned int colorKey)
{
    int keyR = (colorKey & 0xFF0000) >> 16;
    int keyG = (colorKey & 0xFF00) >> 8;
    int keyB = colorKey & 0xFF;
    for (int y = 0; y < height; y++) {
        RGBTripple *s = &src[y * srcPitch];
        RGBQuad *d = &dst[y * dstPitch];
        for (int x = 0; x < width; x++) {
            d->r = s->r;
            d->g = s->g;
            d->b = s->b;
            if (s->r == keyR && s->g == keyG && s->b == keyB)
                d->a = 0;
            else
                d->a = 0xFF;
            s++;
            d++;
        }
    }
}


// ---- src/global/PixTrans8888from888.cpp
extern "C" void *malloc(unsigned int size);
extern "C" void free(void *p);



void Construct8888From888(RGBQuad *dst, RGBTripple *src, int width, int height, int dstPitch, int srcPitch,
                         unsigned int colorKey);
int MipScale8888(RGBQuad *dst, RGBQuad *src, int width, int height, int dstPitch, int srcPitch);

// Convert a 888 image to 8888, box-filtering it down mipLevel times.
int PixTrans8888from888(RGBQuad *dst, RGBTripple *src, int width, int height, int dstPitch, int srcPitch,
                        int mipLevel, unsigned int colorKey)
{
    int fullWidth;
    int fullHeight;
    int i;
    RGBQuad *full;
    RGBQuad *buffer;
    int mipWidth;
    int mipHeight;
    if (mipLevel == 0) {
        Construct8888From888(dst, src, width, height, dstPitch, srcPitch, colorKey);
    } else {
        fullWidth = width << mipLevel;
        fullHeight = height << mipLevel;
        full = (RGBQuad *)malloc(fullWidth * fullHeight * sizeof(RGBQuad));
        if (!full)
            return 0;
        // Retail passes srcPitch as the destination pitch too.
        Construct8888From888(full, src, fullWidth, fullHeight, srcPitch, srcPitch, colorKey);
        if (mipLevel == 1) {
            MipScale8888(dst, full, width, height, dstPitch, srcPitch);
        } else {
            mipWidth = width << (mipLevel - 1);
            mipHeight = height << (mipLevel - 1);
            buffer = (RGBQuad *)malloc(mipWidth * mipHeight * sizeof(RGBQuad));
            if (!buffer)
                goto fail;
            MipScale8888(buffer, full, mipWidth, mipHeight, mipWidth, srcPitch);
            for (i = 2; i < mipLevel; i++) {
                mipWidth /= 2;
                mipHeight /= 2;
                MipScale8888(buffer, buffer, mipWidth, mipHeight, mipWidth, mipWidth * 2);
            }
            MipScale8888(dst, buffer, width, height, dstPitch, mipWidth);
            free(buffer);
        }
        free(full);
    }
    return 1;

fail:
    if (buffer)
        free(buffer);
    if (full)
        free(full);
    return 0;
}


// ---- src/global/PixTrans4444from4444.cpp
extern "C" void *memcpy(void *dst, const void *src, unsigned int n);
extern "C" void *malloc(unsigned int size);
extern "C" void free(void *p);

int MipScale4444(unsigned short *dst, unsigned short *src, int width, int height, int dstPitch, int srcPitch, unsigned int flags);

// Copy a 4444 image, box-filtering it down mipLevel times first.
int PixTrans4444from4444(unsigned short *dst, unsigned short *src, int width, int height, int dstPitch, int srcPitch, int mipLevel)
{
    if (mipLevel == 0) {
        for (int y = 0; y < height; y++)
            memcpy(dst + y * dstPitch, src + y * srcPitch, width * 2);
    } else if (mipLevel == 1) {
        MipScale4444(dst, src, width, height, dstPitch, srcPitch, 0);
    } else {
        int mipWidth = width << (mipLevel - 1);
        int mipHeight = height << (mipLevel - 1);
        unsigned short *buffer = (unsigned short *)malloc(mipWidth * mipHeight * 2);
        MipScale4444(buffer, src, mipWidth, mipHeight, mipWidth, srcPitch, 0);
        for (int i = 2; i < mipLevel; i++) {
            mipWidth /= 2;
            mipHeight /= 2;
            MipScale4444(buffer, buffer, mipWidth, mipHeight, mipWidth, mipWidth * 2, 0);
        }
        MipScale4444(dst, buffer, width, height, dstPitch, mipWidth, 0);
        free(buffer);
    }
    return 1;
}


// ---- src/global/PixTrans1555from1555.cpp
extern "C" void *memcpy(void *dst, const void *src, unsigned int n);
extern "C" void *malloc(unsigned int size);
extern "C" void free(void *p);

int MipScale1555(unsigned short *dst, unsigned short *src, int width, int height, int dstPitch, int srcPitch);

// Copy a 1555 image, box-filtering it down mipLevel times first.
int PixTrans1555from1555(unsigned short *dst, unsigned short *src, int width, int height, int dstPitch, int srcPitch, int mipLevel)
{
    if (mipLevel == 0) {
        for (int y = 0; y < height; y++)
            memcpy(dst + y * dstPitch, src + y * srcPitch, width * 2);
    } else if (mipLevel == 1) {
        MipScale1555(dst, src, width, height, dstPitch, srcPitch);
    } else {
        int mipWidth = width << (mipLevel - 1);
        int mipHeight = height << (mipLevel - 1);
        unsigned short *buffer = (unsigned short *)malloc(mipWidth * mipHeight * 2);
        MipScale1555(buffer, src, mipWidth, mipHeight, mipWidth, srcPitch);
        for (int i = 2; i < mipLevel; i++) {
            mipWidth /= 2;
            mipHeight /= 2;
            MipScale1555(buffer, buffer, mipWidth, mipHeight, mipWidth, mipWidth * 2);
        }
        MipScale1555(dst, buffer, width, height, dstPitch, mipWidth);
        free(buffer);
    }
    return 1;
}


// ---- src/global/PixTrans565from565.cpp
extern "C" void *memcpy(void *dst, const void *src, unsigned int n);
extern "C" void *malloc(unsigned int size);
extern "C" void free(void *p);

int MipScale565(unsigned short *dst, unsigned short *src, int width, int height, int dstPitch, int srcPitch,
                unsigned int colorKey, unsigned short keyMask);

// Copy a 565 image, box-filtering it down mipLevel times first.
int PixTrans565from565(unsigned short *dst, unsigned short *src, int width, int height, int dstPitch, int srcPitch, int mipLevel,
                       int colorKey)
{
    if (mipLevel == 0) {
        for (int y = 0; y < height; y++)
            memcpy(dst + y * dstPitch, src + y * srcPitch, width * 2);
    } else if (mipLevel == 1) {
        MipScale565(dst, src, width, height, dstPitch, srcPitch, colorKey, 0xF81F);
    } else {
        int mipWidth = width << (mipLevel - 1);
        int mipHeight = height << (mipLevel - 1);
        unsigned short *buffer = (unsigned short *)malloc(mipWidth * mipHeight * 2);
        MipScale565(buffer, src, mipWidth, mipHeight, mipWidth, srcPitch, colorKey, 0xF81F);
        for (int i = 2; i < mipLevel; i++) {
            mipWidth /= 2;
            mipHeight /= 2;
            MipScale565(buffer, buffer, mipWidth, mipHeight, mipWidth, mipWidth * 2, colorKey, 0xF81F);
        }
        MipScale565(dst, buffer, width, height, dstPitch, mipWidth, colorKey, 0xF81F);
        free(buffer);
    }
    return 1;
}


// ---- src/global/PixTrans555from555.cpp
extern "C" void *memcpy(void *dst, const void *src, unsigned int n);
extern "C" void *malloc(unsigned int size);
extern "C" void free(void *p);

int MipScale555(unsigned short *dst, unsigned short *src, int width, int height, int dstPitch, int srcPitch,
                unsigned int colorKey, unsigned short keyMask);

// Copy a 555 image, box-filtering it down mipLevel times first.
int PixTrans555from555(unsigned short *dst, unsigned short *src, int width, int height, int dstPitch, int srcPitch, int mipLevel,
                       int colorKey)
{
    if (mipLevel == 0) {
        for (int y = 0; y < height; y++)
            memcpy(dst + y * dstPitch, src + y * srcPitch, width * 2);
    } else if (mipLevel == 1) {
        MipScale555(dst, src, width, height, dstPitch, srcPitch, colorKey, 0x7C1F);
    } else {
        int mipWidth = width << (mipLevel - 1);
        int mipHeight = height << (mipLevel - 1);
        unsigned short *buffer = (unsigned short *)malloc(mipWidth * mipHeight * 2);
        MipScale555(buffer, src, mipWidth, mipHeight, mipWidth, srcPitch, colorKey, 0x7C1F);
        for (int i = 2; i < mipLevel; i++) {
            mipWidth /= 2;
            mipHeight /= 2;
            MipScale555(buffer, buffer, mipWidth, mipHeight, mipWidth, mipWidth * 2, colorKey, 0x7C1F);
        }
        MipScale555(dst, buffer, width, height, dstPitch, mipWidth, colorKey, 0x7C1F);
        free(buffer);
    }
    return 1;
}


// ---- src/global/PixTrans8from8.cpp
extern "C" void *memcpy(void *dst, const void *src, unsigned int n);
extern "C" void *malloc(unsigned int size);
extern "C" void free(void *p);

class ColorMapper;

int MipScale8(unsigned char *dst, unsigned char *src, int width, int height, int dstPitch, int srcPitch,
              ColorMapper *mapper);

// Copy an 8-bit paletted image, box-filtering it down mipLevel times first.
int PixTrans8from8(unsigned char *dst, unsigned char *src, int width, int height, int dstPitch, int srcPitch, int mipLevel,
                   ColorMapper *mapper)
{
    if (mipLevel == 0) {
        for (int y = 0; y < height; y++)
            memcpy(dst + y * dstPitch, src + y * srcPitch, width);
    } else if (mipLevel == 1) {
        MipScale8(dst, src, width, height, dstPitch, srcPitch, mapper);
    } else {
        int mipWidth = width << (mipLevel - 1);
        int mipHeight = height << (mipLevel - 1);
        unsigned char *buffer = (unsigned char *)malloc(mipWidth * mipHeight);
        MipScale8(buffer, src, mipWidth, mipHeight, mipWidth, srcPitch, mapper);
        for (int i = 2; i < mipLevel; i++) {
            mipWidth /= 2;
            mipHeight /= 2;
            MipScale8(buffer, buffer, mipWidth, mipHeight, mipWidth, mipWidth * 2, mapper);
        }
        MipScale8(dst, buffer, width, height, dstPitch, mipWidth, mapper);
        free(buffer);
    }
    return 1;
}


// ---- src/global/PixTrans8888from565.cpp


int PixTrans8888from565(RGBQuad *dst, unsigned short *src, int width, int height, int dstPad, int srcPad, unsigned int colorKey)
{
    // Declaration order fixes the register assignment.
    unsigned short *s;
    RGBQuad *d;
    int y;
    int x;
    unsigned short key;
    key = ((colorKey & 0xF80000) >> 8) | ((colorKey & 0xFC00) >> 5) | ((colorKey & 0xF8) >> 3);

    for (y = 0; y < height; y++) {
        s = src;
        d = dst;
        for (x = 0; x < width; x++) {
            if (*s == key) {
                // Color-keyed pixels become magenta.
                d->r = 0xFF;
                d->g = 0;
                d->b = 0xFF;
                d->a = 0xFF;
            } else {
                d->r = (*s & 0xF800) >> 8;
                d->g = (*s & 0x7E0) >> 3;
                d->b = (*s & 0x1F) << 3;
                d->a = 0xFF;
            }
            d++;
            s++;
        }
        src += srcPad;
        dst += dstPad;
    }
    return 1;
}


// ---- src/global/PixTrans8888from1555.cpp


int PixTrans8888from1555(RGBQuad *dst, unsigned short *src, int width, int height, int dstPad, int srcPad)
{
    // Declaration order fixes the register assignment.
    unsigned short *s;
    RGBQuad *d;
    int y;
    int x;

    for (y = 0; y < height; y++) {
        s = src;
        d = dst;
        for (x = 0; x < width; x++) {
            d->r = (*s & 0x7C00) >> 7;
            d->g = (*s & 0x3E0) >> 2;
            d->b = (*s & 0x1F) << 3;
            if (*s & 0x8000)
                d->a = 0xFF;
            else
                d->a = 0;
            d++;
            s++;
        }
        src += srcPad;
        dst += dstPad;
    }
    return 1;
}


// ---- src/global/PixTrans8888from4444.cpp


int PixTrans8888from4444(RGBQuad *dst, unsigned short *src, int width, int height, int dstPad, int srcPad)
{
    // Declaration order fixes the register assignment.
    unsigned short *s;
    RGBQuad *d;
    int y;
    int x;

    for (y = 0; y < height; y++) {
        s = src;
        d = dst;
        for (x = 0; x < width; x++) {
            d->a = (*s & 0xF000) >> 8;
            d->r = (*s & 0xF00) >> 4;
            d->g = *s & 0xF0;
            d->b = (*s & 0xF) << 4;
            d++;
            s++;
        }
        src += srcPad;
        dst += dstPad;
    }
    return 1;
}


// ---- src/global/PixTrans565from888.cpp


class ColorMapper;
int DitherImage(void *src, int format, int width, int height, int srcPad, int dstPad,
                 RGBTripple *dst24, unsigned short *dst565, unsigned short *dst555,
                 unsigned char *dst8, ColorMapper *mapper);

int PixTrans565from888(unsigned short *dst, RGBTripple *src, int width, int height, int dstPad, int srcPad, int dither)
{
    // Declaration order fixes the register assignment.
    int y;
    int x;
    RGBTripple *s;
    unsigned short *d;

    if (dither) {
        DitherImage(src, 0x378, width, height, srcPad, dstPad, 0, dst, 0, 0, 0);
    } else {
        for (y = 0; y < height; y++) {
            s = src;
            d = dst;
            for (x = 0; x < width; x++) {
                    *d = ((s->r >> 3) << 11) | ((s->g >> 2) << 5) | (s->b >> 3);
                d++;
                s++;
            }
            src += srcPad;
            dst += dstPad;
        }
    }
    return 1;
}


// ---- src/global/PixTrans555from888.cpp


class ColorMapper;
int DitherImage(void *src, int format, int width, int height, int srcPad, int dstPad,
                 RGBTripple *dst24, unsigned short *dst565, unsigned short *dst555,
                 unsigned char *dst8, ColorMapper *mapper);

int PixTrans555from888(unsigned short *dst, RGBTripple *src, int width, int height, int dstPad, int srcPad, int dither)
{
    // Declaration order fixes the register assignment.
    RGBTripple *s;
    unsigned short *d;
    int y;
    int x;

    if (dither) {
        DitherImage(src, 0x378, width, height, srcPad, dstPad, 0, 0, dst, 0, 0);
    } else {
        for (y = 0; y < height; y++) {
            s = src;
            d = dst;
            for (x = 0; x < width; x++) {
                    *d = ((s->r >> 3) << 10) | ((s->g >> 3) << 5) | (s->b >> 3);
                d++;
                s++;
            }
            src += srcPad;
            dst += dstPad;
        }
    }
    return 1;
}


// ---- src/global/PixTrans8from888.cpp


int DitherImage(void *src, int format, int width, int height, int srcPad, int dstPad,
                 RGBTripple *dst24, unsigned short *dst565, unsigned short *dst555,
                 unsigned char *dst8, ColorMapper *mapper);

int PixTrans8from888(unsigned char *dst, RGBTripple *src, int width, int height, int dstPad,
                     int srcPad, int dither, ColorMapper *mapper)
{
    // Declaration order fixes the register assignment.
    RGBTripple *s;
    int y;
    unsigned char *d;
    int x;
    unsigned char *lookup;
    RGBTripple *srcRow;
    unsigned char *dstRow;

    if (dither && mapper) {
        DitherImage(src, 0x378, width, height, srcPad, dstPad, 0, 0, 0, dst, mapper);
    } else {
        dstRow = dst;
        srcRow = src;
        lookup = mapper->GetLookup15();
        for (y = 0; y < height; y++) {
            s = srcRow;
            d = dstRow;
            for (x = 0; x < width; x++) {
                *d = lookup[(unsigned short)(((s->r >> 3) << 10) | ((s->g >> 3) << 5) | (s->b >> 3))];
                s++;
                d++;
            }
            srcRow += srcPad;
            dstRow += dstPad;
        }
    }
    return 1;
}


// ---- src/global/PixTrans8from565.cpp


int DitherImage(void *src, int format, int width, int height, int srcPad, int dstPad,
                 RGBTripple *dst24, unsigned short *dst565, unsigned short *dst555,
                 unsigned char *dst8, ColorMapper *mapper);

int PixTrans8from565(unsigned char *dst, unsigned short *src, int width, int height, int dstPad,
                     int srcPad, int dither, ColorMapper *mapper)
{
    // Declaration order fixes the register assignment.
    unsigned short *s;
    unsigned char *d;
    int y;
    int x;
    unsigned char *lookup;
    unsigned short *srcRow;
    unsigned char *dstRow;


    if (dither && mapper) {
        DitherImage(src, 0x235, width, height, srcPad, dstPad, 0, 0, 0, dst, mapper);
    } else {
        dstRow = dst;
        srcRow = src;
        lookup = mapper->GetLookup16();
        for (y = 0; y < height; y++) {
            s = srcRow;
            d = dstRow;
            for (x = 0; x < width; x++) {
                *d = lookup[*s];
                s++;
                d++;
            }
            srcRow += srcPad;
            dstRow += dstPad;
        }
    }
    return 1;
}


// ---- src/global/ColorMapApplyPalette15.cpp
// Converts 16-bit pixels to 8-bit through a 64K-entry lookup table.
void ColorMapApplyPalette15(unsigned short *src, unsigned char *dst, int pitch, int width, int height, unsigned char *table)
{
    unsigned char *dstRow = dst;
    for (int y = 0; y < height; y++) {
        dst = dstRow;
        for (int x = 0; x < width; x++) {
            *dst++ = table[*src++];
        }
        dstRow += pitch;
    }
}


// ---- src/global/PixTrans8from555.cpp


int DitherImage(void *src, int format, int width, int height, int srcPad, int dstPad,
                 RGBTripple *dst24, unsigned short *dst565, unsigned short *dst555,
                 unsigned char *dst8, ColorMapper *mapper);
void ColorMapApplyPalette15(unsigned short *src, unsigned char *dst, int dstPad, int width, int height,
                            unsigned char *lookup);

int PixTrans8from555(unsigned char *dst, unsigned short *src, int width, int height, int dstPad,
                     int srcPad, int dither, ColorMapper *mapper)
{
    // Declaration order fixes the register assignment.
    unsigned short *s;
    unsigned char *d;
    int y;
    int x;
    unsigned char *lookup;

    if (dither && mapper) {
        DitherImage(src, 0x22B, width, height, srcPad, dstPad, 0, 0, 0, dst, mapper);
    } else if (srcPad == width && srcPad % 8 == 0) {
        lookup = mapper->GetLookup15();
        ColorMapApplyPalette15(src, dst, dstPad, width, height, lookup);
    } else {
        lookup = mapper->GetLookup15();
        for (y = 0; y < height; y++) {
            s = src;
            d = dst;
            for (x = 0; x < width; x++) {
                *d = lookup[*s];
                s++;
                d++;
            }
            src += srcPad;
            dst += dstPad;
        }
    }
    return 1;
}


// ---- src/global/PixTrans8888from555.cpp


int PixTrans8888from555(RGBQuad *dst, unsigned short *src, int width, int height, int dstPad, int srcPad, unsigned int colorKey)
{
    // Declaration order fixes the register assignment.
    unsigned short *s;
    RGBQuad *d;
    int y;
    int x;
    unsigned short key;
    key = ((colorKey & 0xF80000) >> 9) | ((colorKey & 0xF800) >> 6) | ((colorKey & 0xF8) >> 3);

    for (y = 0; y < height; y++) {
        s = src;
        d = dst;
        for (x = 0; x < width; x++) {
            if (*s == key) {
                // Color-keyed pixels become magenta.
                d->r = 0xFF;
                d->g = 0;
                d->b = 0xFF;
                d->a = 0xFF;
            } else {
                d->r = (*s & 0x7C00) >> 7;
                d->g = (*s & 0x3E0) >> 2;
                d->b = (*s & 0x1F) << 3;
                d->a = 0xFF;
            }
            d++;
            s++;
        }
        src += srcPad;
        dst += dstPad;
    }
    return 1;
}


// ---- src/global/PixTrans888from555.cpp


int PixTrans888from555(RGBTripple *dst, unsigned short *src, int width, int height, int dstPad, int srcPad)
{
    // Declaration order fixes the register assignment.
    unsigned short *s;
    RGBTripple *d;
    int y;
    int x;

    for (y = 0; y < height; y++) {
        s = src;
        d = dst;
        for (x = 0; x < width; x++) {
            if (*s == 0x7C1F) {
                // Transparent magenta stays magenta.
                d->r = 0xFF;
                d->g = 0;
                d->b = 0xFF;
            } else {
                d->r = (*s & 0x7C00) >> 7;
                d->g = (*s & 0x3E0) >> 2;
                d->b = (*s & 0x1F) << 3;
            }
            d++;
            s++;
        }
        src += srcPad;
        dst += dstPad;
    }
    return 1;
}


// ---- src/global/PixTrans565from555.cpp
int PixTrans565from555(unsigned short *dst, unsigned short *src, int width, int height, int dstPad, int srcPad)
{
    // Declaration order fixes the register assignment.
    unsigned short *s;
    unsigned short *d;
    int y;
    int x;

    for (y = 0; y < height; y++) {
        s = src;
        d = dst;
        for (x = 0; x < width; x++) {
            *d = (*s & 0x1F) | ((*s >> 5) << 6);
            d++;
            s++;
        }
        src += srcPad;
        dst += dstPad;
    }
    return 1;
}



// ---- src/global/PixTrans555from565.cpp
int PixTrans555from565(unsigned short *dst, unsigned short *src, int width, int height, int dstPad, int srcPad)
{
    // Declaration order fixes the register assignment.
    unsigned short *s;
    unsigned short *d;
    int y;
    int x;

    for (y = 0; y < height; y++) {
        s = src;
        d = dst;
        for (x = 0; x < width; x++) {
            *d = (*s & 0x1F) | ((*s >> 6) << 5);
            d++;
            s++;
        }
        src += srcPad;
        dst += dstPad;
    }
    return 1;
}



// ---- src/global/PixTrans888from8.cpp


int PixTrans888from8(RGBTripple *dst, unsigned char *src, int width, int height, int dstPad, int srcPad, ColorMapper *mapper)
{
    // Declaration order fixes the register assignment.
    unsigned char *s;
    RGBTripple *d;
    int y;
    int x;

    for (y = 0; y < height; y++) {
        s = src;
        d = dst;
        for (x = 0; x < width; x++) {
            d->r = mapper->m_palette[*s].r;
            d->g = mapper->m_palette[*s].g;
            d->b = mapper->m_palette[*s].b;
            d++;
            s++;
        }
        src += srcPad;
        dst += dstPad;
    }
    return 1;
}


// ---- src/global/PixTrans565from8.cpp

int PixTrans565from8(unsigned short *dst, unsigned char *src, int width, int height, int dstPitch, int srcPitch, ColorMapper *mapper)
{
    // Declaration order fixes the register assignment.
    unsigned char *s;
    unsigned short *d;
    int y;
    int x;

    for (y = 0; y < height; y++) {
        s = src;
        d = dst;
        for (x = 0; x < width; x++) {
            *d++ = mapper->m_palette16[*s++];
        }
        src += srcPitch;
        dst += dstPitch;
    }
    return 1;
}


// ---- src/global/PixTrans555from8.cpp

int PixTrans555from8(unsigned short *dst, unsigned char *src, int width, int height, int dstPitch, int srcPitch, ColorMapper *mapper)
{
    // Declaration order fixes the register assignment.
    unsigned char *s;
    unsigned short *d;
    int y;
    int x;

    for (y = 0; y < height; y++) {
        s = src;
        d = dst;
        for (x = 0; x < width; x++) {
            *d++ = mapper->m_palette15[*s++];
        }
        src += srcPitch;
        dst += dstPitch;
    }
    return 1;
}


// ---- src/global/PixTrans4444from8888.cpp


int PixTrans4444from8888(unsigned short *dst, RGBQuad *src, int width, int height, int dstPad, int srcPad)
{
    // Declaration order fixes the register assignment.
    int y;
    int x;
    RGBQuad *s;
    unsigned short *d;

    for (y = 0; y < height; y++) {
        s = src;
        d = dst;
        for (x = 0; x < width; x++) {
            *d = ((s->r >> 4) << 8) | (s->g & ~0xF) | (s->b >> 4) | ((s->a >> 4) << 12);
            d++;
            s++;
        }
        src += srcPad;
        dst += dstPad;
    }
    return 1;
}


// ---- src/global/PixTrans1555from8888.cpp

int PixTrans1555from8888(unsigned short *dst, RGBQuad *src, int width, int height, int dstPad, int srcPad, int alphaThreshold)
{
    // Declaration order fixes the register assignment.
    int y;
    int x;
    RGBQuad *s;
    unsigned short *d;

    for (y = 0; y < height; y++) {
        s = src;
        d = dst;
        for (x = 0; x < width; x++) {
            *d = ((s->r >> 3) << 10) | ((s->g >> 3) << 5) | (s->b >> 3);
            if (s->a >= alphaThreshold)
                *d |= 0x8000;
            d++;
            s++;
        }
        src += srcPad;
        dst += dstPad;
    }
    return 1;
}


// ---- src/global/PixTrans888from8888.cpp


int PixTrans888from8888(RGBTripple *dst, RGBQuad *src, int width, int height, int dstPad, int srcPad)
{
    // Declaration order fixes the register assignment.
    int y;
    int x;
    RGBQuad *s;
    RGBTripple *d;

    for (y = 0; y < height; y++) {
        s = src;
        d = dst;
        for (x = 0; x < width; x++) {
            d->r = s->r;
            d->g = s->g;
            d->b = s->b;
            d++;
            s++;
        }
        src += srcPad;
        dst += dstPad;
    }
    return 1;
}


// ---- src/global/PixTrans1555from888.cpp


int PixTrans1555from888(unsigned short *dst, RGBTripple *src, int width, int height, int dstPad, int srcPad, int colorKey)
{
    // Declaration order fixes the register assignment.
    int y;
    int x;
    RGBTripple *s;
    unsigned short key;
    unsigned short *d;
    key = ((colorKey & 0xF80000) >> 9) | ((colorKey & 0xF800) >> 6) | ((colorKey & 0xF8) >> 3);

    for (y = 0; y < height; y++) {
        s = src;
        d = dst;
        for (x = 0; x < width; x++) {
            if (s->r == 0xFF && s->g == 0 && s->b == 0xFF) {
                // Magenta is transparent.
                *d = key;
            } else {
                *d = ((s->r >> 3) << 10) | ((s->g >> 3) << 5) | (s->b >> 3);
                *d |= 0x8000;
            }
            d++;
            s++;
        }
        src += srcPad;
        dst += dstPad;
    }
    return 1;
}


// ---- src/global/PixTrans555from4444.cpp


int PixTrans555from4444(unsigned short *dst, unsigned short *src, int width, int height, int dstPad, int srcPad, int alphaThreshold, unsigned int colorKey)
{
    // Declaration order fixes the register assignment.
    int y;
    int x;
    unsigned short key;
    unsigned short *s;
    unsigned short *d;
    alphaThreshold = (alphaThreshold >> 4) << 12;
    key = ((colorKey & 0xF80000) >> 9) | ((colorKey & 0xF800) >> 6) | ((colorKey & 0xF8) >> 3);

    for (y = 0; y < height; y++) {
        s = src;
        d = dst;
        for (x = 0; x < width; x++) {
            unsigned short p = *s;
            if ((p & 0xF000) < alphaThreshold)
                *d++ = key;
            else
                *d++ = ((p & 0xF00) << 3) | ((p & 0xF0) << 2) | ((p & 0xF) << 1);
            s++;
        }
        src += srcPad;
        dst += dstPad;
    }
    return 1;
}


// ---- src/global/PixTrans1555from4444.cpp


int PixTrans1555from4444(unsigned short *dst, unsigned short *src, int width, int height, int dstPad, int srcPad, int alphaThreshold)
{
    // Declaration order fixes the register assignment.
    int y;
    int x;
    unsigned short *s;
    unsigned short *d;
    alphaThreshold = (alphaThreshold >> 4) << 12;

    for (y = 0; y < height; y++) {
        s = src;
        d = dst;
        for (x = 0; x < width; x++) {
            unsigned short p = *s;
            if ((p & 0xF000) < alphaThreshold)
                *d++ = 0;
            else
                *d++ = ((p & 0xF00) << 3) | ((p & 0xF0) << 2) | ((p & 0xF) << 1) | 0x8000;
            s++;
        }
        src += srcPad;
        dst += dstPad;
    }
    return 1;
}


// ---- src/global/PixTrans565from4444.cpp


int PixTrans565from4444(unsigned short *dst, unsigned short *src, int width, int height, int dstPad, int srcPad, int alphaThreshold, unsigned int colorKey)
{
    // Declaration order fixes the register assignment.
    int y;
    int x;
    unsigned short key;
    unsigned short *s;
    unsigned short *d;
    alphaThreshold = (alphaThreshold >> 4) << 12;
    key = ((colorKey & 0xF80000) >> 8) | ((colorKey & 0xFC00) >> 5) | ((colorKey & 0xF8) >> 3);

    for (y = 0; y < height; y++) {
        s = src;
        d = dst;
        for (x = 0; x < width; x++) {
            unsigned short p = *s;
            if ((p & 0xF000) < alphaThreshold)
                *d++ = key;
            else
                *d++ = ((p & 0xF00) << 4) | ((p & 0xF0) << 3) | ((p & 0xF) << 1);
            s++;
        }
        src += srcPad;
        dst += dstPad;
    }
    return 1;
}


// ---- src/global/PixTrans1555from555.cpp


int PixTrans1555from555(unsigned short *dst, unsigned short *src, int width, int height, int dstPad, int srcPad, unsigned int colorKey)
{
    // Declaration order fixes the register assignment.
    int y;
    int x;
    unsigned short *s;
    unsigned short key;
    unsigned short *d;
    key = ((colorKey & 0xF80000) >> 9) | ((colorKey & 0xF800) >> 6) | ((colorKey & 0xF8) >> 3);

    for (y = 0; y < height; y++) {
        s = src;
        d = dst;
        for (x = 0; x < width; x++) {
            // Magenta (0x7C1F) is the transparent color.
            *d = *s;
            if (*s == 0x7C1F)
                *d = key;
            else
                *d |= 0x8000;
            d++;
            s++;
        }
        src += srcPad;
        dst += dstPad;
    }
    return 1;
}


// ---- src/global/PixTrans555from1555.cpp


int PixTrans555from1555(unsigned short *dst, unsigned short *src, int width, int height, int dstPad, int srcPad, unsigned int colorKey)
{
    // Declaration order fixes the register assignment.
    int y;
    int x;
    unsigned short *s;
    unsigned short key;
    unsigned short *d;
    key = ((colorKey & 0xF80000) >> 9) | ((colorKey & 0xF800) >> 6) | ((colorKey & 0xF8) >> 3);

    for (y = 0; y < height; y++) {
        s = src;
        d = dst;
        for (x = 0; x < width; x++) {
            // Retail never copies the source pixel here.
            if (*s & 0x8000)
                *d++ &= 0x7FFF;
            else
                *d++ = key;
            s++;
        }
        src += srcPad;
        dst += dstPad;
    }
    return 1;
}


// ---- src/global/PixTrans1555from565.cpp


int PixTrans1555from565(unsigned short *dst, unsigned short *src, int width, int height, int dstPad, int srcPad, unsigned int colorKey)
{
    // Declaration order fixes the register assignment.
    int y;
    int x;
    unsigned short *s;
    unsigned short *d;
    unsigned short key;
    key = ((colorKey & 0xF80000) >> 8) | ((colorKey & 0xFC00) >> 5) | ((colorKey & 0xF8) >> 3);

    for (y = 0; y < height; y++) {
        s = src;
        d = dst;
        for (x = 0; x < width; x++) {
            *d = (*s & 0x1F) | ((*s >> 6) << 5);
            if (*s != key)
                *d |= 0x8000;
            d++;
            s++;
        }
        src += srcPad;
        dst += dstPad;
    }
    return 1;
}


// ---- src/global/PixTrans565from1555.cpp


int PixTrans565from1555(unsigned short *dst, unsigned short *src, int width, int height, int dstPad, int srcPad, unsigned int colorKey)
{
    // Declaration order fixes the register assignment.
    int y;
    int x;
    unsigned short key;
    unsigned short *s;
    unsigned short *d;
    key = ((colorKey & 0xF80000) >> 8) | ((colorKey & 0xFC00) >> 5) | ((colorKey & 0xF8) >> 3);

    for (y = 0; y < height; y++) {
        s = src;
        d = dst;
        for (x = 0; x < width; x++) {
            unsigned short p = *s;
            if (p & 0x8000)
                *d++ = (p & 0x1F) | ((p >> 5) << 6);
            else
                *d++ = key;
            s++;
        }
        src += srcPad;
        dst += dstPad;
    }
    return 1;
}


// ---- src/global/PixTransScale.cpp
struct RGBTripple;
struct RGBQuad;
class ColorMapper;

int PixTrans8from8(unsigned char *dst, unsigned char *src, int, int, int, int, int, ColorMapper *cm);
int PixTrans555from555(unsigned short *dst, unsigned short *src, int, int, int, int, int, int);
int PixTrans565from565(unsigned short *dst, unsigned short *src, int, int, int, int, int, int);
int PixTrans888from888(RGBTripple *dst, RGBTripple *src, int, int, int, int, int);
int PixTrans1555from1555(unsigned short *dst, unsigned short *src, int, int, int, int, int);
int PixTrans4444from4444(unsigned short *dst, unsigned short *src, int, int, int, int, int);
int PixTrans8888from8888(RGBQuad *dst, RGBQuad *src, int, int, int, int, int);

// Scaled pixel copy between two images of the same format (the format code
// is the bit layout, e.g. 0x22B8 = 8888).
int PixTransScale(void *dst, void *src, int a, int b, int c, int d, int e, int format,
                  ColorMapper *cm, int f)
{
    switch (format) {
    case 0x22B8:
        return PixTrans8888from8888((RGBQuad *)dst, (RGBQuad *)src, a, b, c, d, e);
    case 0x115C:
        return PixTrans4444from4444((unsigned short *)dst, (unsigned short *)src, a, b, c, d, e);
    case 0x613:
        return PixTrans1555from1555((unsigned short *)dst, (unsigned short *)src, a, b, c, d, e);
    case 0x378:
        return PixTrans888from888((RGBTripple *)dst, (RGBTripple *)src, a, b, c, d, e);
    case 0x235:
        return PixTrans565from565((unsigned short *)dst, (unsigned short *)src, a, b, c, d, e, f);
    case 0x22B:
        return PixTrans555from555((unsigned short *)dst, (unsigned short *)src, a, b, c, d, e, f);
    case 8:
        return PixTrans8from8((unsigned char *)dst, (unsigned char *)src, a, b, c, d, e, cm);
    }
    return 0;
}


// ---- src/global/PixTransColorConvert.cpp
struct RGBQuad;
struct RGBTripple;
class ColorMapper;

typedef unsigned char u8;
typedef unsigned short u16;

int PixTrans8888from8888(RGBQuad *, RGBQuad *, int, int, int, int, int);
int PixTrans888from8888(RGBTripple *, RGBQuad *, int, int, int, int);
int PixTrans4444from8888(u16 *, RGBQuad *, int, int, int, int);
int PixTrans1555from8888(u16 *, RGBQuad *, int, int, int, int, int);
int PixTrans8888from888(RGBQuad *, RGBTripple *, int, int, int, int, int, unsigned int);
int DitherImage(void *, int, int, int, int, int, RGBTripple *, u16 *, u16 *, u8 *, ColorMapper *);
int PixTrans888from888(RGBTripple *, RGBTripple *, int, int, int, int, int);
int PixTrans565from888(u16 *, RGBTripple *, int, int, int, int, int);
int PixTrans555from888(u16 *, RGBTripple *, int, int, int, int, int);
int PixTrans1555from888(u16 *, RGBTripple *, int, int, int, int, int);
int PixTrans8from888(u8 *, RGBTripple *, int, int, int, int, int, ColorMapper *);
int PixTrans8888from4444(RGBQuad *, u16 *, int, int, int, int);
int PixTrans4444from4444(u16 *, u16 *, int, int, int, int, int);
int PixTrans1555from4444(u16 *, u16 *, int, int, int, int, int);
int PixTrans565from4444(u16 *, u16 *, int, int, int, int, int, unsigned int);
int PixTrans555from4444(u16 *, u16 *, int, int, int, int, int, unsigned int);
int PixTrans8888from1555(RGBQuad *, u16 *, int, int, int, int);
int PixTrans1555from1555(u16 *, u16 *, int, int, int, int, int);
int PixTrans565from1555(u16 *, u16 *, int, int, int, int, unsigned int);
int PixTrans555from1555(u16 *, u16 *, int, int, int, int, unsigned int);
int PixTrans8888from565(RGBQuad *, u16 *, int, int, int, int, unsigned int);
int PixTrans888from565(RGBTripple *, u16 *, int, int, int, int);
int PixTrans565from565(u16 *, u16 *, int, int, int, int, int, int);
int PixTrans555from565(u16 *, u16 *, int, int, int, int);
int PixTrans1555from565(u16 *, u16 *, int, int, int, int, unsigned int);
int PixTrans8from565(u8 *, u16 *, int, int, int, int, int, ColorMapper *);
int PixTrans8888from555(RGBQuad *, u16 *, int, int, int, int, unsigned int);
int PixTrans888from555(RGBTripple *, u16 *, int, int, int, int);
int PixTrans1555from555(u16 *, u16 *, int, int, int, int, unsigned int);
int PixTrans565from555(u16 *, u16 *, int, int, int, int);
int PixTrans555from555(u16 *, u16 *, int, int, int, int, int, int);
int PixTrans8from555(u8 *, u16 *, int, int, int, int, int, ColorMapper *);
int PixTrans888from8(RGBTripple *, u8 *, int, int, int, int, ColorMapper *);
int PixTrans565from8(u16 *, u8 *, int, int, int, int, ColorMapper *);
int PixTrans555from8(u16 *, u8 *, int, int, int, int, ColorMapper *);
int PixTrans8from8(u8 *, u8 *, int, int, int, int, int, ColorMapper *);

// Pixel formats, as stored in the texture files.
enum {
    FORMAT_8 = 8,
    FORMAT_555 = 0x22B,
    FORMAT_565 = 0x235,
    FORMAT_1555 = 0x613,
    FORMAT_4444 = 0x115C,
    FORMAT_888 = 0x378,
    FORMAT_8888 = 0x22B8
};

// Converts a block of pixels from srcFormat to dstFormat with the matching
// PixTrans routine; unsupported pairs return 0.
int PixTransColorConvert(void *dst, void *src, int width, int height, int dstPitch, int srcPitch, int dstFormat,
                         int srcFormat, int param, ColorMapper *mapper, int alphaParam, unsigned int colorKey)
{
    int result = 0;
    switch (srcFormat) {
    case FORMAT_8888:
        switch (dstFormat) {
        case FORMAT_8888:
            result = PixTrans8888from8888((RGBQuad *)dst, (RGBQuad *)src, width, height, dstPitch, srcPitch, 0);
            break;
        case FORMAT_888:
            result = PixTrans888from8888((RGBTripple *)dst, (RGBQuad *)src, width, height, dstPitch, srcPitch);
            break;
        case FORMAT_4444:
            result = PixTrans4444from8888((u16 *)dst, (RGBQuad *)src, width, height, dstPitch, srcPitch);
            break;
        case FORMAT_1555:
            result = PixTrans1555from8888((u16 *)dst, (RGBQuad *)src, width, height, dstPitch, srcPitch, alphaParam);
            break;
        }
        break;
    case FORMAT_888:
        switch (dstFormat) {
        case FORMAT_8888:
            result = PixTrans8888from888((RGBQuad *)dst, (RGBTripple *)src, width, height, dstPitch, srcPitch, 0, colorKey);
            break;
        case FORMAT_888:
            if (param)
                result = DitherImage(src, srcFormat, width, height, srcPitch, dstPitch, (RGBTripple *)dst, 0, 0, 0, 0);
            else
                result = PixTrans888from888((RGBTripple *)dst, (RGBTripple *)src, width, height, dstPitch, srcPitch, 0);
            break;
        case FORMAT_565:
            result = PixTrans565from888((u16 *)dst, (RGBTripple *)src, width, height, dstPitch, srcPitch, param);
            break;
        case FORMAT_555:
            result = PixTrans555from888((u16 *)dst, (RGBTripple *)src, width, height, dstPitch, srcPitch, param);
            break;
        case FORMAT_1555:
            result = PixTrans1555from888((u16 *)dst, (RGBTripple *)src, width, height, dstPitch, srcPitch, colorKey);
            break;
        case FORMAT_8:
            result = PixTrans8from888((u8 *)dst, (RGBTripple *)src, width, height, dstPitch, srcPitch, param, mapper);
            break;
        }
        break;
    case FORMAT_4444:
        switch (dstFormat) {
        case FORMAT_8888:
            result = PixTrans8888from4444((RGBQuad *)dst, (u16 *)src, width, height, dstPitch, srcPitch);
            break;
        case FORMAT_4444:
            result = PixTrans4444from4444((u16 *)dst, (u16 *)src, width, height, dstPitch, srcPitch, 0);
            break;
        case FORMAT_1555:
            result = PixTrans1555from4444((u16 *)dst, (u16 *)src, width, height, dstPitch, srcPitch, alphaParam);
            break;
        case FORMAT_565:
            result = PixTrans565from4444((u16 *)dst, (u16 *)src, width, height, dstPitch, srcPitch, alphaParam, colorKey);
            break;
        case FORMAT_555:
            result = PixTrans555from4444((u16 *)dst, (u16 *)src, width, height, dstPitch, srcPitch, alphaParam, colorKey);
            break;
        }
        break;
    case FORMAT_1555:
        switch (dstFormat) {
        case FORMAT_8888:
            result = PixTrans8888from1555((RGBQuad *)dst, (u16 *)src, width, height, dstPitch, srcPitch);
            break;
        case FORMAT_1555:
            result = PixTrans1555from1555((u16 *)dst, (u16 *)src, width, height, dstPitch, srcPitch, 0);
            break;
        case FORMAT_565:
            result = PixTrans565from1555((u16 *)dst, (u16 *)src, width, height, dstPitch, srcPitch, colorKey);
            break;
        case FORMAT_555:
            result = PixTrans555from1555((u16 *)dst, (u16 *)src, width, height, dstPitch, srcPitch, colorKey);
            break;
        }
        break;
    case FORMAT_565:
        switch (dstFormat) {
        case FORMAT_8888:
            result = PixTrans8888from565((RGBQuad *)dst, (u16 *)src, width, height, dstPitch, srcPitch, colorKey);
            break;
        case FORMAT_888:
            result = PixTrans888from565((RGBTripple *)dst, (u16 *)src, width, height, dstPitch, srcPitch);
            break;
        case FORMAT_565:
            result = PixTrans565from565((u16 *)dst, (u16 *)src, width, height, dstPitch, srcPitch, 0, 2);
            break;
        case FORMAT_555:
            result = PixTrans555from565((u16 *)dst, (u16 *)src, width, height, dstPitch, srcPitch);
            break;
        case FORMAT_1555:
            result = PixTrans1555from565((u16 *)dst, (u16 *)src, width, height, dstPitch, srcPitch, colorKey);
            break;
        case FORMAT_8:
            result = PixTrans8from565((u8 *)dst, (u16 *)src, width, height, dstPitch, srcPitch, param, mapper);
            break;
        }
        break;
    case FORMAT_555:
        switch (dstFormat) {
        case FORMAT_8888:
            result = PixTrans8888from555((RGBQuad *)dst, (u16 *)src, width, height, dstPitch, srcPitch, colorKey);
            break;
        case FORMAT_888:
            result = PixTrans888from555((RGBTripple *)dst, (u16 *)src, width, height, dstPitch, srcPitch);
            break;
        case FORMAT_1555:
            result = PixTrans1555from555((u16 *)dst, (u16 *)src, width, height, dstPitch, srcPitch, colorKey);
            break;
        case FORMAT_565:
            result = PixTrans565from555((u16 *)dst, (u16 *)src, width, height, dstPitch, srcPitch);
            break;
        case FORMAT_555:
            result = PixTrans555from555((u16 *)dst, (u16 *)src, width, height, dstPitch, srcPitch, 0, 2);
            break;
        case FORMAT_8:
            result = PixTrans8from555((u8 *)dst, (u16 *)src, width, height, dstPitch, srcPitch, param, mapper);
            break;
        }
        break;
    case FORMAT_8:
        switch (dstFormat) {
        case FORMAT_888:
            result = PixTrans888from8((RGBTripple *)dst, (u8 *)src, width, height, dstPitch, srcPitch, mapper);
            break;
        case FORMAT_565:
            result = PixTrans565from8((u16 *)dst, (u8 *)src, width, height, dstPitch, srcPitch, mapper);
            break;
        case FORMAT_555:
            result = PixTrans555from8((u16 *)dst, (u8 *)src, width, height, dstPitch, srcPitch, mapper);
            break;
        case FORMAT_8:
            result = PixTrans8from8((u8 *)dst, (u8 *)src, width, height, dstPitch, srcPitch, 0, mapper);
            break;
        }
        break;
    }
    return result;
}

