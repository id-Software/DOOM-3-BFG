// Pure stub implementations - no zlib header needed
typedef void* z_streamp;
typedef struct { void* next_in; unsigned int avail_in; void* next_out; unsigned int avail_out; void* state; void* zalloc; void* zfree; void* opaque; } z_stream;

int inflateInit_(z_streamp strm, const char* version, int stream_size) { (void)strm; (void)version; (void)stream_size; return 0; }
int inflateInit2_(z_streamp strm, int windowBits, const char* version, int stream_size) { (void)strm; (void)windowBits; (void)version; (void)stream_size; return 0; }
int inflate(z_streamp strm, int flush) { (void)strm; (void)flush; return 0; }
int inflateEnd(z_streamp strm) { (void)strm; return 0; }
int deflateInit2_(z_streamp strm, int level, int method, int windowBits, int memLevel, int strategy, const char* version, int stream_size) { (void)strm; (void)level; (void)method; (void)windowBits; (void)memLevel; (void)strategy; (void)version; (void)stream_size; return 0; }
