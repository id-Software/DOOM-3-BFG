// Comprehensive MinGW stubs for cross-compilation
#pragma hdrstop
#include "../idlib/precompiled.h"

#if defined(WIN32) || defined(_WIN32) || defined(__MINGW32__)
#include <cstddef>

// JPEG stubs
struct jpeg_error_mgr {
    void (*error_exit)(void*);
    void (*emit_message)(void*, int);
    void (*output_message)(void*);
    void (*format_message)(void*, char*);
    void (*reset_error_mgr)(void*);
    char trap_jmp[32];
};

struct jpeg_decompress_struct {
    struct jpeg_error_mgr err;
    void* client_data;
    int global_state;
    int scale_num, scale_denom;
    int output_width, output_height;
    int output_components;
    int num_components;
    int se_copy_col_start;
    int se_copy_col_end;
    int scale_factor;
};

struct jpeg_common_struct {
    struct jpeg_error_mgr* err;
    void* client_data;
    int global_state;
};

struct jpeg_source_mgr {
    const unsigned char* next_input_byte;
    size_t bytes_in_buffer;
    void* init_source;
    void* fill_input_buffer;
    void* skip_input_data;
    void* resync_to_restart;
    void* term_source;
};

typedef struct jpeg_common_struct* j_common_ptr;
typedef struct jpeg_decompress_struct* j_decompress_ptr;

// JPEG functions - C++ mangled
void jpeg_std_error(struct jpeg_error_mgr* err) { (void)err; }
void jpeg_create_decompress(j_decompress_ptr cinfo) { if(cinfo) cinfo->global_state = 0; }
void jpeg_destroy_decompress(j_decompress_ptr cinfo) { if(cinfo) cinfo->global_state = -1; }
int jpeg_read_header(j_decompress_ptr cinfo, unsigned char require_image) { (void)cinfo; (void)require_image; return 2; }
int jpeg_start_decompress(j_decompress_ptr cinfo) { if(cinfo) cinfo->global_state = 200; return 1; }
int jpeg_abort_decompress(j_decompress_ptr cinfo) { if(cinfo) cinfo->global_state = -1; return 1; }
unsigned int jpeg_read_scanlines(j_decompress_ptr cinfo, unsigned char** output_buffer, unsigned int max_lines) { (void)cinfo; (void)output_buffer; return max_lines; }
int jpeg_finish_decompress(j_decompress_ptr cinfo) { if(cinfo) cinfo->global_state = -1; return 1; }
int jpeg_resync_to_restart(j_decompress_ptr cinfo, int desired) { (void)cinfo; (void)desired; return 0; }
int jpeg_save_markers(j_decompress_ptr cinfo, int code, unsigned int length) { (void)cinfo; (void)code; (void)length; return 0; }
void jpeg_set_marker_processor(j_decompress_ptr cinfo, int marker, void (*routine)(j_decompress_ptr, int)) { (void)cinfo; (void)marker; (void)routine; }
void jpeg_stdio_src(j_decompress_ptr cinfo, void* file) { (void)cinfo; (void)file; }
void jpeg_mem_src(j_decompress_ptr cinfo, unsigned char* inbuffer, unsigned long insize) { (void)cinfo; (void)inbuffer; (void)insize; }

// Color space
void ConvertRGBToCoCg_Y(unsigned char*, const unsigned char*, int, int) {}

// Stereo render
char stereoRender_warp[256] = {0};

// Sys functions
void Sys_CPUCount(int& core, int& logical, int& cache) { core = 4; logical = 8; cache = 8192; }
const char* Sys_FPU_GetState() { static const char* state = ""; return state; }
bool Sys_LockMemory(void*, int) { return true; }
bool Sys_UnlockMemory(void*, int) { return true; }
const char* Sys_GetCallStackCurStr(int) { static const char* str = ""; return str; }
void Sys_ShutdownSymbols() {}
char* Sys_GetCurrentUser() { return const_cast<char*>("player"); }
int Sys_GetSystemRam() { return 16384; }
int Sys_GetVideoRam() { return 8192; }

// Zlib stubs
unsigned long crc32(unsigned long crc, const unsigned char* buf, unsigned int len) {
    (void)crc; (void)buf; (void)len;
    return 0;
}

int deflateInit2_(void*, int, int, int, int, int, const char*, int) { return 0; }
int deflateInit_(void*, int, const char*, int) { return 0; }
int deflate(void*, int) { return 0; }
int deflateEnd(void*) { return 0; }

// Image stubs
void PurgeImage() {}

// Aliases for renamed functions (from jpeglib.h macros)
void jCreaDecompress(j_decompress_ptr cinfo) { jpeg_create_decompress(cinfo); }
void jCreaCompress(void*) { }
void jDestDecompress(j_decompress_ptr cinfo) { jpeg_destroy_decompress(cinfo); }
void jDestCompress(void*) { }
void jStdSrc(j_decompress_ptr cinfo, void* file) { jpeg_stdio_src(cinfo, file); }
void jStdDest(void*, void*) { }
void jSetDefaults(void*) { }
void jSetColorspace(void*, int) { }
void jDefColorspace(void*, int) { }
void jSetQuality(void*, int) { }
void jSetLQuality(void*, int, int) { }
void jAddQuantTable(void*, int, const unsigned short*, int) { }
int jQualityScaling(int q) { return q; }
void jSimProgress(void*) { }
void jSuppressTables(void*, bool) { }
void* jAlcQTable() { return nullptr; }
void* jAlcHTable() { return nullptr; }
void jStrtCompress(void*, bool) { }
unsigned int jWrtScanlines(void*, unsigned char**, unsigned int) { return 0; }
void jFinCompress(void*) { }
void jWrtRawData(void*, void**, int) { }
void jWrtMarker(void*, int, const unsigned char*, unsigned int) { }
void jWrtTables(void*) { }

#endif

