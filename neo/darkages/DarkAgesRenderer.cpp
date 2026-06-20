/*
===========================================================================

DOOM: The Dark Ages - Renderer Implementation

===========================================================================
*/

#include "DarkAgesRenderer.h"
#include <cstring>
#include <cmath>

// ============================================================================
// DarkAgesRenderer Implementation
// ============================================================================

DarkAgesRenderer & DarkAgesRenderer::Instance() {
	static DarkAgesRenderer instance;
	return instance;
}

DarkAgesRenderer::DarkAgesRenderer() :
	currentSettings( &DA_RENDER_T460S ),
	currentAtmosphere( &DA_ATMOSPHERE_ACT1 ),
	resolutionScale( 1.0f ),
	avgFrameTime( 8.33f ),
	frameTimeIndex( 0 ),
	targetFrameTime( 8.33f )
{
	memset( frameTimeHistory, 0, sizeof( frameTimeHistory ) );
}

void DarkAgesRenderer::Init() {
	// Default to T460s optimized preset
	SetQualityPreset( DA_QUALITY_T460S_OPTIMIZED );
	SetActAtmosphere( 0 );
}

void DarkAgesRenderer::Shutdown() {
}

// ============================================================================
// Quality Settings
// ============================================================================

void DarkAgesRenderer::SetQualityPreset( darkAgesQualityPreset_t preset ) {
	switch ( preset ) {
		case DA_QUALITY_LOW:
			currentSettings = &DA_RENDER_LOW;
			break;
		case DA_QUALITY_MEDIUM:
			currentSettings = &DA_RENDER_MEDIUM;
			break;
		case DA_QUALITY_HIGH:
			currentSettings = &DA_RENDER_HIGH;
			break;
		case DA_QUALITY_ULTRA:
			currentSettings = &DA_RENDER_ULTRA;
			break;
		case DA_QUALITY_T460S_OPTIMIZED:
		default:
			currentSettings = &DA_RENDER_T460S;
			break;
	}
	
	targetFrameTime = 1000.0f / currentSettings->dynamicResTarget;
	ApplySettings( *currentSettings );
}

void DarkAgesRenderer::ApplySettings( const darkAgesRenderSettings_t & settings ) {
	// Apply shadow settings
	// r_shadowMapSize -> settings.shadowMapSize
	// r_maxShadowLights -> settings.maxShadowLights
	// r_shadowDistance -> settings.shadowDistance
	
	// Apply lighting settings
	// r_maxDynamicLights -> settings.maxDynamicLights
	
	// Apply particle settings
	// r_maxParticles -> settings.maxParticles
	
	// Apply LOD settings
	// r_lodDistance1 -> settings.lodDistance1
	
	// Apply post-processing
	// r_fxaa -> settings.fxaa
	// r_bloom -> settings.bloom
	// r_bloomIntensity -> settings.bloomIntensity
	// r_motionBlur -> settings.motionBlur
	
	// Apply atmosphere
	// r_fogEnable -> settings.fog
	// r_fogDensity -> settings.fogDensity
	
	// Set dynamic resolution boundaries
	if ( settings.dynamicResolution ) {
		resolutionScale = 1.0f;
	}
}

// ============================================================================
// Atmosphere
// ============================================================================

void DarkAgesRenderer::SetAtmosphere( const darkAgesAtmosphere_t & atmosphere ) {
	currentAtmosphere = &atmosphere;
	
	// Apply fog
	// r_fogColor -> atmosphere.fogColorR, G, B
	// r_fogDensity -> atmosphere.fogDensity
	// r_fogStart -> atmosphere.fogStart
	// r_fogEnd -> atmosphere.fogEnd
	
	// Apply ambient light
	// r_ambientColor -> atmosphere.ambientR, G, B
	// r_ambientIntensity -> atmosphere.ambientIntensity
	
	// Apply color grading (post-process)
	// r_saturation -> atmosphere.saturation
	// r_contrast -> atmosphere.contrast
	// r_brightness -> atmosphere.brightness
	// r_colorTint -> atmosphere.colorTintR, G, B
}

void DarkAgesRenderer::SetActAtmosphere( int actNumber ) {
	if ( actNumber >= 0 && actNumber < DA_ACT_COUNT ) {
		SetAtmosphere( *DA_ACT_ATMOSPHERES[actNumber] );
	}
}

// ============================================================================
// Dynamic Resolution
// ============================================================================

void DarkAgesRenderer::UpdateDynamicResolution( float frameTimeMs ) {
	if ( currentSettings == NULL || !currentSettings->dynamicResolution ) {
		return;
	}
	
	// Store frame time in history
	frameTimeHistory[frameTimeIndex] = frameTimeMs;
	frameTimeIndex = ( frameTimeIndex + 1 ) % 120;
	
	// Calculate average frame time over last 120 frames
	float total = 0.0f;
	for ( int i = 0; i < 120; i++ ) {
		total += frameTimeHistory[i];
	}
	avgFrameTime = total / 120.0f;
	
	// Adjust resolution scale based on performance
	float currentFPS = 1000.0f / avgFrameTime;
	float targetFPS = currentSettings->dynamicResTarget;
	
	if ( currentFPS < targetFPS * 0.9f ) {
		// Below target - decrease resolution
		resolutionScale -= 0.02f;
		if ( resolutionScale < currentSettings->dynamicResMin ) {
			resolutionScale = currentSettings->dynamicResMin;
		}
	} else if ( currentFPS > targetFPS * 1.1f ) {
		// Above target - increase resolution
		resolutionScale += 0.01f;
		if ( resolutionScale > currentSettings->dynamicResMax ) {
			resolutionScale = currentSettings->dynamicResMax;
		}
	}
}

// ============================================================================
// Performance Monitoring
// ============================================================================

float DarkAgesRenderer::GetCurrentFPS() const {
	if ( avgFrameTime <= 0.0f ) {
		return 0.0f;
	}
	return 1000.0f / avgFrameTime;
}

bool DarkAgesRenderer::IsAboveTargetFPS() const {
	return GetCurrentFPS() >= currentSettings->dynamicResTarget;
}
