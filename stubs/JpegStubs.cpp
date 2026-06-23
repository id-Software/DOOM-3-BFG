// JPEG stubs for cross-compilation
#include <cstdio>
#include <csetjmp>

struct jpeg_error_mgr {
    void (*error_exit)(void*);
    void (*emit_message)(void*, int);
    void (*output_message)(void*);
    void (*format_message)(void*, char*);
    void (*reset_error_mgr)(void*);
};

struct jpeg_decompress_struct {
    jpeg_error_mgr err;
    void* client_data;
    int global_state;
};

extern "C" {

typedef void* j_common_ptr;
typedef jpeg_decompress_struct* j_decompress_ptr;

void jpeg_std_error(void* err) { (void)err; }
void jpeg_create_decompress(j_decompress_ptr cinfo) { if(cinfo) cinfo->global_state = 0; }
void jpeg_destroy_decompress(j_decompress_ptr cinfo) { if(cinfo) cinfo->global_state = -1; }
int jpeg_read_header(j_decompress_ptr cinfo, int require_image) { (void)cinfo; (void)require_image; return 2; }
int jpeg_start_decompress(j_decompress_ptr cinfo) { if(cinfo) cinfo->global_state = 200; return 1; }
int jpeg_abort_decompress(j_decompress_ptr cinfo) { if(cinfo) cinfo->global_state = -1; return 1; }
unsigned int jpeg_read_scanlines(j_decompress_ptr, unsigned char**, unsigned int max_lines) { return max_lines; }
int jpeg_finish_decompress(j_decompress_ptr cinfo) { if(cinfo) cinfo->global_state = -1; return 1; }
int jpeg_resync_to_restart(j_decompress_ptr cinfo, int desired) { (void)cinfo; (void)desired; return 0; }

}
