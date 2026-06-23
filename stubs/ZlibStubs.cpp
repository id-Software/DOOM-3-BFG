// Zlib stubs for cross-compilation
#include <stdexcept>
#define ZLIB_WINAPI
#include <zlib.h>

int inflateInit_(const z_streamp strm, const char* version, int stream_size) {
    return inflateInit(strm);
}
int inflateInit2_(z_streamp strm, int windowBits, const char* version, int stream_size) {
    return inflateInit2(strm, windowBits);
}
int deflateInit2_(z_streamp strm, int level, int method, int windowBits, int memLevel, int strategy, const char* version, int stream_size) {
    return deflateInit2(strm, level, method, windowBits, memLevel, strategy);
}
