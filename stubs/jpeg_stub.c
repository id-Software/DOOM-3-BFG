// JPEG stub - matches libjpeg turbo ABI
#include <stdio.h>
#include <setjmp.h>

#ifdef __cplusplus
extern "C" {
#endif

struct jpeg_error_mgr {
    void (*error_exit)(void*);
    void (*emit_message)(void*, int);
    void (*output_message)(void*);
    void (*format_message)(void*, char*);
    void (*reset_error_mgr)(void*);
    jmp_buf setjmp_buffer;
};

struct jpeg_decompress_struct {
    struct jpeg_error_mgr err;
    void* client_data;
    int global_state;
    int scale_num, scale_denom;
    int output_width, output_height;
    int output_components;
};

struct jpeg_common_struct {
    struct jpeg_error_mgr* err;
    void* client_data;
    int global_state;
};

typedef struct jpeg_common_struct* j_common_ptr;
typedef struct jpeg_decompress_struct* j_decompress_ptr;

void jpeg_std_error(struct jpeg_error_mgr* err) { (void)err; }
void jpeg_create_decompress(j_decompress_ptr cinfo) { if(cinfo) cinfo->global_state = 0; }
void jpeg_destroy_decompress(j_decompress_ptr cinfo) { if(cinfo) cinfo->global_state = -1; }
int jpeg_read_header(j_decompress_ptr cinfo, unsigned char require_image) { (void)cinfo; (void)require_image; return 2; }
int jpeg_start_decompress(j_decompress_ptr cinfo) { if(cinfo) cinfo->global_state = 200; return 1; }
int jpeg_abort_decompress(j_decompress_ptr cinfo) { if(cinfo) cinfo->global_state = -1; return 1; }
unsigned int jpeg_read_scanlines(j_decompress_ptr cinfo, unsigned char** output_buffer, unsigned int max_lines) { (void)cinfo; (void)output_buffer; return max_lines; }
int jpeg_finish_decompress(j_decompress_ptr cinfo) { if(cinfo) cinfo->global_state = -1; return 1; }
int jpeg_resync_to_restart(j_decompress_ptr cinfo, int desired) { (void)cinfo; (void)desired; return 0; }

#ifdef __cplusplus
}
#endif
