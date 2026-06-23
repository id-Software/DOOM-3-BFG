// DoomLib stubs - minimal implementations

// Forward declarations for shadow volume job parameters
struct preLightShadowVolumeParms_t;
struct staticShadowVolumeParms_t;
struct dynamicShadowVolumeParms_t;

// DoomInterface class - defined outside namespace with explicit method definitions
struct DoomInterface {
    void Shutdown();
    void Startup(int mode, bool dedicated);
    bool Frame(int doomTics, void* userCmdMgr);
};

void DoomInterface::Shutdown() {}
void DoomInterface::Startup(int mode, bool dedicated) { (void)mode; (void)dedicated; }
bool DoomInterface::Frame(int doomTics, void* userCmdMgr) { (void)doomTics; (void)userCmdMgr; return false; }

// DoomLib namespace with C++ linkage
namespace DoomLib {
    int expansionDirty = 0;
    int skipToLoad = 0;
    int skipToNew = 0;
    int idealExpansion = 0;
    int chosenSkill = 0;
    int chosenEpisode = 0;
    
    struct idMatchParameters {
        int maxPlayers;
        int skill;
        int invuln;
        int respawnItems;
        int respawnPowerups;
        int forceRespawn;
    };
    idMatchParameters matchParms = {0};
    
    DoomInterface Interface;
    
    void* GetGlobalData(int index) { (void)index; return nullptr; }
    void SetPlayer(int player) { (void)player; }
    void SetCurrentExpansion(int expansion) { (void)expansion; }
    int RemapControl(int key) { (void)key; return 0; }
}

// g variable
void* g = nullptr;

// Doom event types
struct event_t {
    int type;
    int data1;
    int data2;
};

// D_PostEvent
void D_PostEvent(event_t* ev) { (void)ev; }

// Shadow volume parameter types - must be defined before the functions
struct preLightShadowVolumeParms_t { int dummy; };
struct staticShadowVolumeParms_t { int dummy; };
struct dynamicShadowVolumeParms_t { int dummy; };

// Shadow volume jobs
void PreLightShadowVolumeJob(const preLightShadowVolumeParms_t* parms) { (void)parms; }
void StaticShadowVolumeJob(const staticShadowVolumeParms_t* parms) { (void)parms; }
void DynamicShadowVolumeJob(const dynamicShadowVolumeParms_t* parms) { (void)parms; }

// GL functions
void GL_SelectTexture(int unit) { (void)unit; }
void GL_State(unsigned long long state, bool force) { (void)state; (void)force; }
void GL_Color(float r, float g, float b) { (void)r; (void)g; (void)b; }
void GL_Color(float r, float g, float b, float a) { (void)r; (void)g; (void)b; (void)a; }
void GL_Color(float* color) { (void)color; }
void GL_SetDefaultState() {}
void GL_Cull(int cull) { (void)cull; }
void GL_Viewport(int x, int y, int w, int h) { (void)x; (void)y; (void)w; (void)h; }
void GL_Scissor(int x, int y, int w, int h) { (void)x; (void)y; (void)w; (void)h; }
void GL_PolygonOffset(float scale, float bias) { (void)scale; (void)bias; }
void GL_Clear(bool color, bool depth, bool stencil, unsigned char mask, float r, float g, float b, float a) { (void)color; (void)depth; (void)stencil; (void)mask; (void)r; (void)g; (void)b; (void)a; }
void GL_DepthBoundsTest(float zmin, float zmax) { (void)zmin; (void)zmax; }

// DXT encoder stubs
void CompressImageDXT5HQ(const unsigned char*, unsigned char*, int, int) {}
void CompressImageDXT1HQ(const unsigned char*, unsigned char*, int, int) {}
void CompressImageDXT1Fast_Generic(const unsigned char*, unsigned char*, int, int) {}
void CompressImageDXT5Fast_Generic(const unsigned char*, unsigned char*, int, int) {}
void CompressNormalMapDXT5HQ(const unsigned char*, unsigned char*, int, int) {}
void CompressNormalMapDXT5Fast_Generic(const unsigned char*, unsigned char*, int, int) {}
void CompressYCoCgDXT5HQ(const unsigned char*, unsigned char*, int, int) {}
void CompressYCoCgDXT5Fast_Generic(const unsigned char*, unsigned char*, int, int) {}
void ConvertRGBToCoCg_Y(unsigned char*, const unsigned char*, int, int) {}

// Stereo render
char stereoRender_warp[256] = {0};

// Sys stubs
void Sys_CPUCount(int& core, int& logical, int& cache) { core = 4; logical = 8; cache = 8192; }

// XAudio2 stubs
struct IXAudio2;
extern "C" {
int XAudio2Create(IXAudio2** ppXAudio2, unsigned int Flags, unsigned int processorOpt) { (void)ppXAudio2; (void)Flags; (void)processorOpt; return 0; }
}

void I_InitSoundHardware(int numChannels, int sampleRate) { (void)numChannels; (void)sampleRate; }
void I_ShutdownSoundHardware() {}

// C linkage symbols (with underscores for backward compat)
extern "C" {

int DoomLib_expansionDirty = 0;
int DoomLib_skipToLoad = 0;
int DoomLib_skipToNew = 0;
int DoomLib_idealExpansion = 0;
int DoomLib_chosenSkill = 0;
int DoomLib_chosenEpisode = 0;

}  // extern "C"
