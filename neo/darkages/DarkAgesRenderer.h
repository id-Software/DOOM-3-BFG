/*
===========================================================================

DOOM: The Dark Ages - Renderer Modifications
Optimized rendering pipeline for Intel HD 520 (120+ FPS target)
Dark medieval atmosphere rendering

===========================================================================
*/

#ifndef __DARKAGES_RENDERER_H__
#define __DARKAGES_RENDERER_H__

// ============================================================================
// Performance Targets
// ThinkPad T460s: Intel HD Graphics 520
// Resolution: 1920x1080 (or dynamic resolution scaling)
// Target: 120+ FPS (8.33ms per frame budget)
// ============================================================================

// ============================================================================
// Rendering Quality Presets
// ============================================================================

enum darkAgesQualityPreset_t {
	DA_QUALITY_LOW = 0,
	DA_QUALITY_MEDIUM,
	DA_QUALITY_HIGH,
	DA_QUALITY_ULTRA,
	DA_QUALITY_T460S_OPTIMIZED,
	DA_QUALITY_COUNT
};

struct darkAgesRenderSettings_t {
	const char *		presetName;
	
	// Resolution
	int					renderWidth;
	int					renderHeight;
	bool				dynamicResolution;
	float				dynamicResMin;
	float				dynamicResMax;
	float				dynamicResTarget;
	
	// Shadows
	int					shadowMapSize;
	int					maxShadowLights;
	float				shadowDistance;
	bool				softShadows;
	bool				stencilShadows;
	
	// Lighting
	int					maxDynamicLights;
	bool				ambientOcclusion;
	int					aoQuality;
	bool				volumetricLighting;
	
	// Effects
	int					maxParticles;
	float				particleLODDistance;
	bool				motionBlur;
	bool				bloom;
	float				bloomIntensity;
	bool				filmGrain;
	float				filmGrainIntensity;
	
	// Geometry
	float				lodDistance1;
	float				lodDistance2;
	float				lodDistance3;
	int					maxDecals;
	
	// Post-Processing
	bool				fxaa;
	int					fxaaQuality;
	bool				chromaticAberration;
	bool				vignette;
	float				vignetteIntensity;
	
	// Atmosphere
	bool				fog;
	bool				rain;
	bool				snow;
	float				fogDensity;
	int					weatherParticleCount;
};

// ============================================================================
// T460s Optimized Preset (120+ FPS target)
// ============================================================================

static const darkAgesRenderSettings_t DA_RENDER_T460S = {
	"T460s Optimized (120+ FPS)",
	
	// Resolution - use dynamic resolution around 70-100%
	1920, 1080,
	true, 0.65f, 1.0f, 120.0f,
	
	// Shadows - minimal for performance
	512, 2, 512.0f,
	false, false,
	
	// Lighting - reduced
	4, false, 0, false,
	
	// Effects - minimal particles, no expensive effects
	256, 256.0f,
	false, true, 0.3f,
	false, 0.0f,
	
	// Geometry - aggressive LOD
	128.0f, 256.0f, 512.0f, 32,
	
	// Post-Processing - lightweight FXAA only
	true, 1,
	false, true, 0.15f,
	
	// Atmosphere - lightweight
	true, false, false, 0.003f, 0
};

static const darkAgesRenderSettings_t DA_RENDER_LOW = {
	"Low",
	
	1280, 720,
	false, 0.5f, 1.0f, 60.0f,
	
	256, 1, 256.0f,
	false, false,
	
	2, false, 0, false,
	
	128, 128.0f,
	false, false, 0.0f,
	false, 0.0f,
	
	64.0f, 128.0f, 256.0f, 16,
	
	false, 0,
	false, false, 0.0f,
	
	true, false, false, 0.005f, 0
};

static const darkAgesRenderSettings_t DA_RENDER_MEDIUM = {
	"Medium",
	
	1920, 1080,
	false, 0.7f, 1.0f, 60.0f,
	
	1024, 4, 1024.0f,
	true, false,
	
	6, true, 1, false,
	
	512, 512.0f,
	false, true, 0.4f,
	false, 0.0f,
	
	256.0f, 512.0f, 1024.0f, 64,
	
	true, 2,
	false, true, 0.2f,
	
	true, true, false, 0.002f, 256
};

static const darkAgesRenderSettings_t DA_RENDER_HIGH = {
	"High",
	
	1920, 1080,
	false, 0.8f, 1.0f, 60.0f,
	
	2048, 8, 2048.0f,
	true, true,
	
	12, true, 2, true,
	
	1024, 1024.0f,
	true, true, 0.5f,
	true, 0.02f,
	
	512.0f, 1024.0f, 2048.0f, 128,
	
	true, 3,
	true, true, 0.25f,
	
	true, true, true, 0.001f, 512
};

static const darkAgesRenderSettings_t DA_RENDER_ULTRA = {
	"Ultra",
	
	1920, 1080,
	false, 1.0f, 1.0f, 60.0f,
	
	4096, 16, 4096.0f,
	true, true,
	
	24, true, 3, true,
	
	2048, 2048.0f,
	true, true, 0.6f,
	true, 0.03f,
	
	1024.0f, 2048.0f, 4096.0f, 256,
	
	true, 4,
	true, true, 0.3f,
	
	true, true, true, 0.0005f, 1024
};

// ============================================================================
// Dark Ages Atmosphere System
// ============================================================================

struct darkAgesAtmosphere_t {
	// Fog settings
	float				fogColorR;
	float				fogColorG;
	float				fogColorB;
	float				fogDensity;
	float				fogStart;
	float				fogEnd;
	
	// Ambient lighting
	float				ambientR;
	float				ambientG;
	float				ambientB;
	float				ambientIntensity;
	
	// Color grading
	float				saturation;
	float				contrast;
	float				brightness;
	float				colorTintR;
	float				colorTintG;
	float				colorTintB;
	
	// Weather
	bool				hasRain;
	bool				hasSnow;
	bool				hasAsh;
	bool				hasEmbers;
	float				weatherIntensity;
};

// Per-act atmospheres
static const darkAgesAtmosphere_t DA_ATMOSPHERE_ACT1 = {
	// Fog: warm orange/brown (burning village)
	0.5f, 0.3f, 0.15f, 0.003f, 100.0f, 2000.0f,
	// Ambient: dim warm
	0.15f, 0.1f, 0.08f, 0.3f,
	// Color grading: desaturated, warm
	0.7f, 1.1f, 0.95f, 1.0f, 0.9f, 0.8f,
	// Weather
	false, false, true, true, 0.3f
};

static const darkAgesAtmosphere_t DA_ATMOSPHERE_ACT2 = {
	// Fog: sickly green (plague lands)
	0.2f, 0.25f, 0.1f, 0.004f, 50.0f, 1500.0f,
	// Ambient: very dim greenish
	0.08f, 0.12f, 0.06f, 0.25f,
	// Color grading: green tint, low saturation
	0.6f, 1.2f, 0.9f, 0.8f, 1.0f, 0.7f,
	// Weather
	true, false, false, false, 0.2f
};

static const darkAgesAtmosphere_t DA_ATMOSPHERE_ACT3 = {
	// Fog: cool blue (underground)
	0.1f, 0.15f, 0.3f, 0.005f, 20.0f, 1000.0f,
	// Ambient: dark blue
	0.05f, 0.06f, 0.12f, 0.2f,
	// Color grading: blue cold
	0.5f, 1.3f, 0.85f, 0.7f, 0.8f, 1.2f,
	// Weather
	false, true, false, false, 0.4f
};

static const darkAgesAtmosphere_t DA_ATMOSPHERE_ACT4 = {
	// Fog: deep red/purple (cursed realm)
	0.3f, 0.05f, 0.15f, 0.004f, 30.0f, 1200.0f,
	// Ambient: dark purple
	0.1f, 0.04f, 0.1f, 0.25f,
	// Color grading: purple tint
	0.55f, 1.25f, 0.88f, 1.1f, 0.6f, 1.1f,
	// Weather
	false, false, true, false, 0.5f
};

static const darkAgesAtmosphere_t DA_ATMOSPHERE_ACT5 = {
	// Fog: hellish red/orange
	0.5f, 0.1f, 0.0f, 0.003f, 50.0f, 3000.0f,
	// Ambient: hellish glow
	0.2f, 0.05f, 0.02f, 0.35f,
	// Color grading: extreme red
	0.4f, 1.4f, 0.92f, 1.3f, 0.5f, 0.3f,
	// Weather
	false, false, true, true, 0.8f
};

static const darkAgesAtmosphere_t * DA_ACT_ATMOSPHERES[] = {
	&DA_ATMOSPHERE_ACT1,
	&DA_ATMOSPHERE_ACT2,
	&DA_ATMOSPHERE_ACT3,
	&DA_ATMOSPHERE_ACT4,
	&DA_ATMOSPHERE_ACT5
};

// ============================================================================
// DarkAgesRenderer class
// ============================================================================

class DarkAgesRenderer {
public:
	static DarkAgesRenderer &	Instance();
	
	void						Init();
	void						Shutdown();
	
	// Quality settings
	void						SetQualityPreset( darkAgesQualityPreset_t preset );
	void						ApplySettings( const darkAgesRenderSettings_t & settings );
	const darkAgesRenderSettings_t * GetCurrentSettings() const { return currentSettings; }
	
	// Atmosphere
	void						SetAtmosphere( const darkAgesAtmosphere_t & atmosphere );
	void						SetActAtmosphere( int actNumber );
	const darkAgesAtmosphere_t * GetCurrentAtmosphere() const { return currentAtmosphere; }
	
	// Dynamic resolution
	void						UpdateDynamicResolution( float frameTimeMs );
	float						GetCurrentResolutionScale() const { return resolutionScale; }
	
	// Performance monitoring
	float						GetAverageFrameTime() const { return avgFrameTime; }
	float						GetCurrentFPS() const;
	bool						IsAboveTargetFPS() const;
	
private:
	DarkAgesRenderer();
	
	const darkAgesRenderSettings_t * currentSettings;
	const darkAgesAtmosphere_t * currentAtmosphere;
	float						resolutionScale;
	float						avgFrameTime;
	float						frameTimeHistory[120];
	int							frameTimeIndex;
	float						targetFrameTime;
};

#endif /* !__DARKAGES_RENDERER_H__ */
