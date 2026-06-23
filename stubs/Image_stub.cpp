// idImage stub implementation - no precompiled headers needed
#ifdef _WIN32
#include <cstddef>
#include <cstdint>

// Forward declarations
class idImage;

// idImage stub class with method
class idImage {
public:
    void PurgeImage() {}
};

// DXT encoder stubs
namespace idDxtEncoder {
void CompressImageDXT5HQ( const unsigned char*, unsigned char*, int, int ) {}
void CompressImageDXT1HQ( const unsigned char*, unsigned char*, int, int ) {}
void CompressImageDXT1Fast_Generic( const unsigned char*, unsigned char*, int, int ) {}
void CompressImageDXT5Fast_Generic( const unsigned char*, unsigned char*, int, int ) {}
void CompressNormalMapDXT5HQ( const unsigned char*, unsigned char*, int, int ) {}
void CompressNormalMapDXT5Fast_Generic( const unsigned char*, unsigned char*, int, int ) {}
void CompressYCoCgDXT5HQ( const unsigned char*, unsigned char*, int, int ) {}
void CompressYCoCgDXT5Fast_Generic( const unsigned char*, unsigned char*, int, int ) {}
}

// Color space stubs
namespace idColorSpace {
void ConvertRGBToCoCg_Y( unsigned char*, const unsigned char*, int, int ) {}
}

// Sys stubs
void Sys_CPUCount( int& core, int& logical, int& cache ) { core = 4; logical = 8; cache = 8192; }

// _chkstk stub - provides the required symbol
// This is needed because GCC 14 changed how it handles stack probing
#ifdef __MINGW32__
extern "C" {
    // __chkstk_ms is called by _chkstk in some GCC configurations
    void __chkstk_ms( void ) { }

    // _chkstk is referenced by win_main.cpp - provide weak alias
    // This does nothing but satisfies the linker
    void _chkstk( void ) __attribute__((weak));
    void _chkstk( void ) { }
}
#endif

// JPEG stubs are in MinGWStubs.cpp

#endif // _WIN32
