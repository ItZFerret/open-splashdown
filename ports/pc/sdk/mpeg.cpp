// libmpeg on the PC: the game's movie player (PSXMediaControl and
// PSXMpegVideo.cpp: Sony's ezmpeg sample in classes) runs as on the PS2,
// and only libmpeg's work is replaced:
//
// - sceMpegDemuxPss splits the PSS (an MPEG-2 program stream) and hands
//   each packet to the game's stream callbacks as libmpeg does: video
//   (0xE0 + ch) to sceMpegStrM2V's (PSXMpegInputBuffer::DemuxVideoData:
//   the 0x60000-byte video ring), private stream 1 sub-stream 0xA0 + ch
//   (PCM, with its 4-byte sub-header) to sceMpegStrPCM's
//   (DemuxAudioData: the audio ring, then SIF DMA to the sound driver's
//   movie buffer, ports/pc/audio/). A packet whose callback says "no room" is
//   left for the next call. The program end code (0x000001B9) ends the
//   demux: nothing more is consumed (the CD stream would read on past the
//   file).
// - sceMpegGetPicture asks for bitstream as libmpeg does, through the
//   game's Nodata callback (PSXMpegInputBuffer::NoDataAvailable: up to 4 KB
//   of the video ring by DMA ch4 to the IPU), takes that transfer
//   (port_ipu_set_movie_sink) and decodes the MPEG-2 video with
//   libavcodec. Before each picture it calls the game's Background
//   callback, as libmpeg does while the IPU decodes
//   (PSXMpegInputBuffer::BackgroundProcessing: demux into the video and
//   audio rings; on the PC until nothing more fits, an `#ifdef` for the
//   PS2's spin on timer 0, which never ends in PORT_DETERMINISTIC's virtual
//   time). That keeps the demux up to the video ring's 0x60000 bytes ahead
//   of the picture, and the audio with it: demuxing only what each picture
//   needed, the PSS's audio arrived just in time and the IOP buffer ran dry
//   at the first late frame. Taking the bitstream ahead into a buffer here
//   instead drained the game's ring, and NoDataAvailable's rounding at a
//   stalled demux then sent stale ring data.
// - The picture goes out as the IPU's CSC makes it (RGB32, alpha 0x80,
//   PCSX2's fixed-point BT.601, the same as ports/pc/ipu.cpp) in libmpeg's
//   order: 16x16 macroblocks, column by column (top to bottom,
//   then left to right), which PSXMpegVideoOutputBuffer's upload expects
//   (one 16-pixel-wide GS transfer per column).
// - The end: the video's sequence end code (0x000001B7; or, failing that,
//   the demux's end and an empty video ring): no more bitstream is asked
//   for, the decoder is drained, then sceMpegIsEnd answers 1 (the call that
//   finds the end repeats the last picture).
//
// The game shows one picture per game frame at 30 Hz (PSXMediaControl::
// Initialize: EnableVSyncTo30Hz), so a 29.97 fps movie plays at its speed.
//
// PORT_NO_MOVIES=1 (golden.py) or a build without ffmpeg: every movie is
// two black frames: the output buffer fills,
// the frames show and sceMpegIsEnd ends it; nothing is demuxed, so no
// audio reaches the driver. PORT_MOVIE_LOG=1 logs each movie's pictures
// and timing.
#include "port.h"
#include "settings.h"

#include <algorithm>
#include <string>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <vector>

#ifndef PORT_NO_FFMPEG
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
}
#endif

namespace {

// The game's sceMpeg (src/PSXMpegVideoDecoder/libmpeg.h): 0x48 bytes on
// the PC too (sys is the last field)
struct Mpeg {
    int width, height, frameCount;
    int64_t pts, dts;
    uint64_t flags;
    int64_t pts2nd, dts2nd;
    uint64_t flags2nd;
    void *sys;
};
static_assert(sizeof(Mpeg) == 0x48 && offsetof(Mpeg, sys) == 0x40);

struct CbData {
    int type;
};
struct CbDataStr {
    int type;
    unsigned char *header;
    unsigned char *data;
    unsigned int len;
};
struct CbDataError {
    int type;
    char *errMessage;
};
typedef int (*Callback)(void *mp, void *cbdata, void *anyData);

// libmpeg's sceMpegCb* and sceMpegStr*
enum { kCbError = 0, kCbNodata = 1, kCbBackground = 4, kCbCount = 7 };
enum { kStrM2V = 0, kStrIPU = 1, kStrPCM = 2, kStrADPCM = 3 };

bool NoMovies()
{
#ifdef PORT_NO_FFMPEG
    return true;
#else
    static bool off = getenv("PORT_NO_MOVIES") && strcmp(getenv("PORT_NO_MOVIES"), "0");
    return off;
#endif
}

} // namespace

// The game's table of the disc's files (src/tu/MapFile.cpp): the name of
// the one the current stream was started on, or "".
struct MapFileEntry {
    char *name;
    int unk04;
    unsigned int lsn;
    int unk0C;
    unsigned int lastLsn;
    int unk14;
    unsigned int size;
    int unk1C;
    unsigned int length;
    int unk24;
};

extern MapFileEntry mapFile[];

namespace {

const char *StreamName()
{
    unsigned lsn = port_disc_stream_lsn();
    for (int i = 0; i < 0x551; i++)
        if (mapFile[i].name && lsn >= mapFile[i].lsn && lsn <= mapFile[i].lastLsn)
            return mapFile[i].name;
    return "";
}

// "Skip intro" (the F1 settings): the publisher logos and the intro movie,
// which play before the main menu, become the two black frames too.
bool SkipThisMovie()
{
    if (!port_skip_intro())
        return false;
    std::string name = StreamName();
    for (char &c : name)
        c = (char)toupper((unsigned char)c);
    for (const char *intro : {"\\ATARI.PSS", "\\RAINBOW.PSS", "\\INTRO.PSS"})
        if (name.find(intro) != std::string::npos)
            return true;
    return false;
}

[[maybe_unused]] bool Log()
{
    static bool on = getenv("PORT_MOVIE_LOG") && strcmp(getenv("PORT_MOVIE_LOG"), "0");
    return on;
}

const int kStubFrames = 2; // PORT_NO_MOVIES: the output buffer's size

#ifndef PORT_NO_FFMPEG

struct Picture {
    int w = 0, h = 0;
    std::vector<uint32_t> mb; // macroblock order (see above)
};

struct Ctx {
    Callback cb[kCbCount] = {};
    void *cbData[kCbCount] = {};
    struct StrCb {
        int type, ch;
        Callback fn;
        void *data;
    };
    std::vector<StrCb> str;

    AVCodecContext *dec = nullptr;
    AVCodecParserContext *parser = nullptr;
    AVPacket *pkt = nullptr;
    AVFrame *frame = nullptr;

    std::vector<uint8_t> carry; // a packet cut by the end of a demux call
    bool demuxEnded = false;    // the program end code was seen
    bool seqEnded = false;      // the video's sequence end code was taken
    uint32_t tail = ~0u;        // the last 4 bitstream bytes
    bool flushed = false;       // the decoder was drained
    bool ended = false;         // sceMpegIsEnd
    std::deque<Picture> ready;
    Picture last;
    std::vector<uint8_t> in;    // the ch4 transfer being taken

    // statistics (PORT_MOVIE_LOG)
    int pictures = 0, starved = 0, repeats = 0;
    uint64_t firstVblank = 0;
    uint64_t bytes = 0;
    std::chrono::steady_clock::time_point start;
};

Ctx *g_feeding; // the context whose ch4 transfer is in flight

// The game calls libmpeg on a deleted (null) sceMpeg too: a stopped
// movie's PSXMediaControl::Terminate after Stop's (sceMpegDelete), Reset
// (sceMpegReset). The PS2 reads low memory there; the PC ignores it.
Ctx *Get(void *mp)
{
    return mp ? (Ctx *)((Mpeg *)mp)->sys : nullptr;
}

void OpenDecoder(Ctx &c)
{
    const AVCodec *codec = avcodec_find_decoder(AV_CODEC_ID_MPEG2VIDEO);
    c.dec = avcodec_alloc_context3(codec);
    // slice threads only: frame threads would hold pictures back (more
    // input before the first output)
    c.dec->thread_count = 2;
    c.dec->thread_type = FF_THREAD_SLICE;
    if (avcodec_open2(c.dec, codec, nullptr) < 0)
        port_fatal("mpeg: avcodec_open2 failed");
    c.parser = av_parser_init(AV_CODEC_ID_MPEG2VIDEO);
}

void CloseDecoder(Ctx &c)
{
    av_parser_close(c.parser);
    c.parser = nullptr;
    avcodec_free_context(&c.dec);
}

// The IPU's CSC (ports/pc/ipu.cpp's, PCSX2's fixed-point BT.601), RGB32 with
// alpha 0x80 (no thresholds), into macroblock order.
void Convert(const AVFrame *f, Picture &p)
{
    p.w = f->width & ~15;
    p.h = f->height & ~15;
    p.mb.resize((size_t)p.w * p.h);
    const AVPixFmtDescriptor *d = av_pix_fmt_desc_get((AVPixelFormat)f->format);
    int sx = d ? d->log2_chroma_w : 1, sy = d ? d->log2_chroma_h : 1;
    auto clamp = [](int v) { return v < 0 ? 0 : v > 255 ? 255 : v; };
    uint32_t *out = p.mb.data();
    for (int mx = 0; mx < p.w; mx += 16)
        for (int my = 0; my < p.h; my += 16)
            for (int y = my; y < my + 16; y++) {
                const uint8_t *py = f->data[0] + (size_t)y * f->linesize[0];
                const uint8_t *pu = f->data[1] + (size_t)(y >> sy) * f->linesize[1];
                const uint8_t *pv = f->data[2] + (size_t)(y >> sy) * f->linesize[2];
                for (int x = mx; x < mx + 16; x++) {
                    int lum = (0x95 * std::max(0, (int)py[x] - 16)) >> 6;
                    int vcr = pv[x >> sx] - 128, vcb = pu[x >> sx] - 128;
                    int r = clamp((lum + ((0xCC * vcr) >> 6) + 1) >> 1);
                    int g = clamp((lum + ((-0x68 * vcr) >> 6) + ((-0x32 * vcb) >> 6) + 1) >> 1);
                    int b = clamp((lum + ((0x102 * vcb) >> 6) + 1) >> 1);
                    *out++ = (uint32_t)r | g << 8 | b << 16 | 0x80u << 24;
                }
            }
}

void Receive(Ctx &c)
{
    while (avcodec_receive_frame(c.dec, c.frame) == 0) {
        c.ready.emplace_back();
        Convert(c.frame, c.ready.back());
        av_frame_unref(c.frame);
    }
}

void Send(Ctx &c, const uint8_t *data, int size)
{
    c.pkt->data = (uint8_t *)data;
    c.pkt->size = size;
    if (avcodec_send_packet(c.dec, size ? c.pkt : nullptr) < 0 && Log())
        port_log("mpeg: avcodec_send_packet failed (%d bytes)", size);
    Receive(c);
}

// Bitstream into the parser, whole pictures into the decoder.
void Parse(Ctx &c, const uint8_t *data, int size)
{
    bool flush = size == 0;
    while (size > 0 || flush) {
        uint8_t *out;
        int outSize;
        int used = av_parser_parse2(c.parser, c.dec, &out, &outSize, data, size, AV_NOPTS_VALUE,
                                    AV_NOPTS_VALUE, 0);
        data += used;
        size -= used;
        if (outSize)
            Send(c, out, outSize);
        if (flush && !outSize)
            break;
    }
}

void Sink(const void *data, size_t bytes)
{
    if (g_feeding) {
        const uint8_t *p = (const uint8_t *)data;
        g_feeding->in.insert(g_feeding->in.end(), p, p + bytes);
    }
}

// libmpeg asks for bitstream: the game's Nodata callback starts a ch4
// transfer, which is taken here. Bytes received, or -1 (the callback's
// error: the CD failed).
int Request(Ctx &c, void *mp)
{
    if (!c.cb[kCbNodata])
        return 0;
    c.in.clear();
    g_feeding = &c;
    port_ipu_set_movie_sink(Sink);
    CbData cd = {kCbNodata};
    int r = c.cb[kCbNodata](mp, &cd, c.cbData[kCbNodata]);
    if (r >= 0)
        port_dma_wait_channel(4);
    port_ipu_set_movie_sink(nullptr);
    g_feeding = nullptr;
    if (r < 0)
        return -1;
    // the sequence end code ends the bitstream (the ring's bytes after it
    // are stale: NoDataAvailable rounds its last transfer up, and past the
    // write pointer it would send the old data around again)
    size_t n = c.in.size();
    for (size_t i = 0; i < n; i++) {
        c.tail = c.tail << 8 | c.in[i];
        if (c.tail == 0x000001B7) {
            n = i + 1;
            c.seqEnded = true;
            break;
        }
    }
    c.bytes += n;
    if (n)
        Parse(c, c.in.data(), (int)n);
    return (int)n;
}

// One packet (start code at p, n bytes): 1 taken, 0 refused by its callback.
int Packet(Ctx &c, void *mp, uint8_t *p, int n)
{
    int sid = p[3];
    if ((sid & 0xF0) != 0xE0 && sid != 0xBD)
        return 1;
    // PES header (MPEG-2: flags, header length; MPEG-1: stuffing, STD, PTS)
    int at = 6;
    if ((p[6] & 0xC0) == 0x80) {
        at = 9 + p[8];
    } else {
        while (at < n && p[at] == 0xFF)
            at++;
        if (at < n && (p[at] & 0xC0) == 0x40)
            at += 2;
        if (at < n)
            at += (p[at] & 0xF0) == 0x20 ? 5 : (p[at] & 0xF0) == 0x30 ? 10 : 1;
    }
    if (at >= n)
        return 1;
    int type, ch;
    if ((sid & 0xF0) == 0xE0) {
        type = kStrM2V;
        ch = sid & 0xF;
    } else {
        int sub = p[at + 1]; // PSS: ff a0 00 00, then the data
        if ((sub & 0xF0) == 0xA0)
            type = kStrPCM;
        else if ((sub & 0xF0) == 0x90)
            type = kStrADPCM;
        else
            return 1;
        ch = sub & 0xF;
    }
    for (const Ctx::StrCb &s : c.str) {
        if (s.type != type || s.ch != ch)
            continue;
        CbDataStr cd = {type, p, p + at, (unsigned)(n - at)};
        if (!s.fn(mp, &cd, s.data))
            return 0;
    }
    return 1;
}

// The length of the unit at p (a start code), or 0 if it isn't all in [p, p + n).
int UnitLength(const uint8_t *p, int n)
{
    if (n < 4)
        return 0;
    int sid = p[3], len;
    if (sid == 0xBA) {
        if (n < 14)
            return 0;
        len = (p[4] & 0xC0) == 0x40 ? 14 + (p[13] & 7) : 12; // MPEG-2 : MPEG-1
    } else if (sid == 0xB9) {
        len = 4;
    } else {
        if (n < 6)
            return 0;
        len = 6 + (p[4] << 8 | p[5]);
    }
    return len <= n ? len : 0;
}

int UnitNeed(const uint8_t *p, int n)
{
    if (n < 6)
        return 14;
    if (p[3] == 0xBA)
        return n < 14 ? 14 : ((p[4] & 0xC0) == 0x40 ? 14 + (p[13] & 7) : 12);
    return 6 + (p[4] << 8 | p[5]);
}

int Demux(Ctx &c, void *mp, uint8_t *data, int size)
{
    if (c.demuxEnded)
        return 0;
    int used = 0;
    // a unit cut by the end of the last call
    if (!c.carry.empty()) {
        for (;;) {
            int need = UnitNeed(c.carry.data(), (int)c.carry.size());
            if ((int)c.carry.size() >= need)
                break;
            int take = std::min(size - used, need - (int)c.carry.size());
            if (take <= 0)
                return used;
            c.carry.insert(c.carry.end(), data + used, data + used + take);
            used += take;
        }
        uint8_t *p = c.carry.data();
        if (p[3] == 0xB9) {
            c.demuxEnded = true;
            c.carry.clear();
            return used;
        }
        if (p[3] != 0xBA && !Packet(c, mp, p, (int)c.carry.size()))
            return used;
        c.carry.clear();
    }
    while (used < size) {
        uint8_t *p = data + used;
        int n = size - used;
        if (n >= 3 && !(p[0] == 0 && p[1] == 0 && p[2] == 1)) {
            used++; // not at a start code: resync
            continue;
        }
        int len = UnitLength(p, n);
        if (!len) {
            c.carry.assign(p, p + n);
            return size;
        }
        if (p[3] == 0xB9) {
            c.demuxEnded = true;
            if (Log())
                port_log("mpeg: program end after %d pictures", c.pictures);
            return used + len;
        }
        if (p[3] != 0xBA && !Packet(c, mp, p, len))
            return used;
        used += len;
    }
    return used;
}

void Reset(Ctx &c)
{
    if (c.dec)
        CloseDecoder(c);
    OpenDecoder(c);
    c.carry.clear();
    c.demuxEnded = c.seqEnded = c.flushed = c.ended = false;
    c.tail = ~0u;
    c.ready.clear();
    c.pictures = c.starved = c.repeats = 0;
    c.bytes = 0;
    c.start = std::chrono::steady_clock::now();
}

void Put(const Picture &src, Mpeg *m, void *rgb32, int mbcount)
{
    size_t n = std::min(src.mb.size(), (size_t)mbcount * 256);
    memcpy(rgb32, src.mb.data(), n * 4);
    m->width = src.w;
    m->height = src.h;
}

#endif // !PORT_NO_FFMPEG

} // namespace

extern "C" {

int sceMpegInit(void)
{
    return 0;
}

int sceMpegCreate(void *mp, unsigned char *work_area, int work_area_size)
{
    memset(mp, 0, sizeof(Mpeg));
    if (NoMovies()) {
        port_log("mpeg: movie replaced by %d black frames (PORT_NO_MOVIES)", kStubFrames);
        return 0;
    }
    if (SkipThisMovie()) {
        port_log("mpeg: %s skipped (Skip intro)", StreamName());
        return 0;
    }
#ifndef PORT_NO_FFMPEG
    Ctx *c = new Ctx;
    c->pkt = av_packet_alloc();
    c->frame = av_frame_alloc();
    Reset(*c);
    ((Mpeg *)mp)->sys = c;
#endif
    return 0;
}

int sceMpegDelete(void *mp)
{
#ifndef PORT_NO_FFMPEG
    if (Ctx *c = Get(mp)) {
        CloseDecoder(*c);
        av_packet_free(&c->pkt);
        av_frame_free(&c->frame);
        delete c;
        ((Mpeg *)mp)->sys = nullptr;
    }
#endif
    return 0;
}

int sceMpegReset(void *mp)
{
    if (!mp)
        return 0;
    ((Mpeg *)mp)->frameCount = 0;
#ifndef PORT_NO_FFMPEG
    if (Ctx *c = Get(mp)) {
        if (Log() && c->pictures)
            port_log("mpeg: reset after %d pictures", c->pictures);
        Reset(*c);
    }
#endif
    return 0;
}

int sceMpegAddCallback(void *mp, int type, void *callback, void *anyData)
{
#ifndef PORT_NO_FFMPEG
    if (Ctx *c = Get(mp); c && type >= 0 && type < kCbCount) {
        c->cb[type] = (Callback)callback;
        c->cbData[type] = anyData;
    }
#endif
    return 0;
}

int sceMpegAddStrCallback(void *mp, int strType, int ch, void *callback, void *anyData)
{
#ifndef PORT_NO_FFMPEG
    if (Ctx *c = Get(mp))
        c->str.push_back({strType, ch, (Callback)callback, anyData});
#endif
    return 0;
}

int sceMpegDemuxPss(void *mp, unsigned char *data, int size)
{
#ifndef PORT_NO_FFMPEG
    if (Ctx *c = Get(mp))
        return Demux(*c, mp, data, size);
#endif
    return size;
}

// mbcount 16x16 macroblocks of RGBA32
int sceMpegGetPicture(void *mp, void *rgb32, int mbcount)
{
    Mpeg *m = (Mpeg *)mp;
#ifndef PORT_NO_FFMPEG
    if (Ctx *c = Get(mp)) {
        int empty = 0;
        if (c->cb[kCbBackground] && !c->seqEnded) {
            CbData cd = {kCbBackground};
            c->cb[kCbBackground](mp, &cd, c->cbData[kCbBackground]);
        }
        while (c->ready.empty() && !c->flushed) {
            int got = c->seqEnded ? 0 : Request(*c, mp);
            if (got < 0)
                return -1;
            if (got > 0) {
                empty = 0;
                continue;
            }
            if (c->seqEnded || c->demuxEnded) {
                // all in: the parser's last picture, then the decoder's
                Parse(*c, nullptr, 0);
                Send(*c, nullptr, 0);
                c->flushed = true;
            } else if (++empty >= 8) {
                // the demux stalls (a stream's ring is full) and the video
                // ring is empty: show the last picture again, so the game
                // runs a frame and drains its audio
                c->starved++;
                break;
            }
        }
        if (!c->ready.empty()) {
            c->last = std::move(c->ready.front());
            c->ready.pop_front();
            c->pictures++;
            if (c->pictures == 1)
                c->firstVblank = port_vblanks();
        } else if (c->flushed) {
            c->ended = true;
            c->repeats++;
            if (Log()) {
                double s = std::chrono::duration<double>(std::chrono::steady_clock::now() - c->start).count();
                uint64_t vb = port_vblanks() - c->firstVblank;
                port_log("mpeg: end: %d pictures (%.3f s at 29.97 fps) shown over %llu vblanks (%.3f s; %.3f s "
                         "since sceMpegReset), %llu bytes of video, %d stalls",
                         c->pictures, c->pictures * 1001 / 30000.0, (unsigned long long)vb, vb * 1001 / 60000.0, s,
                         (unsigned long long)c->bytes, c->starved);
            }
        } else {
            c->repeats++;
            if (Log())
                port_log("mpeg: no bitstream for picture %d: repeating the last", c->pictures + 1);
        }
        if (c->last.w)
            Put(c->last, m, rgb32, mbcount);
        else
            memset(rgb32, 0, (size_t)mbcount * 256 * 4);
        m->frameCount++;
        return 0;
    }
#endif
    // PORT_NO_MOVIES: black
    memset(rgb32, 0, (size_t)mbcount * 16 * 16 * 4);
    m->width = 640;
    m->height = mbcount * 16 * 16 / 640;
    m->frameCount++;
    return 0;
}

int sceMpegIsEnd(void *mp)
{
    if (!mp)
        return 1;
#ifndef PORT_NO_FFMPEG
    if (Ctx *c = Get(mp))
        return c->ended;
#endif
    return ((Mpeg *)mp)->frameCount >= kStubFrames;
}

} // extern "C"
