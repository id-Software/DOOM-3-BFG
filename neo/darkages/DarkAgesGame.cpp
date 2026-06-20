/*
===========================================================================

DOOM: The Dark Ages
Copyright (C) 2024 - Dark Ages Total Conversion

Based on Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.

===========================================================================
*/

#include "DarkAgesGame.h"
#include <cstring>
#include <cstdio>
#include <cmath>

// ============================================================================
// DarkAgesGame Implementation
// ============================================================================

DarkAgesGame & DarkAgesGame::Instance() {
	static DarkAgesGame instance;
	return instance;
}

DarkAgesGame::DarkAgesGame() :
	currentLevel( 0 ),
	difficulty( DA_DIFFICULTY_KNIGHT ),
	isInitialized( false )
{
	memset( &stats, 0, sizeof( stats ) );
	memset( &combatState, 0, sizeof( combatState ) );
	combatState.stamina = DA_STAMINA_MAX;
}

DarkAgesGame::~DarkAgesGame() {
	Shutdown();
}

void DarkAgesGame::Init() {
	if ( isInitialized ) {
		return;
	}
	
	currentLevel = 0;
	difficulty = DA_DIFFICULTY_KNIGHT;
	memset( &stats, 0, sizeof( stats ) );
	combatState.stamina = DA_STAMINA_MAX;
	combatState.comboCount = 0;
	combatState.lastAttackTime = 0.0f;
	combatState.lastBlockTime = 0.0f;
	combatState.lastDodgeTime = 0.0f;
	combatState.isBlocking = false;
	combatState.isParrying = false;
	combatState.isDodging = false;
	combatState.currentCombo = DA_COMBO_NONE;
	
	OptimizeForHardware();
	
	isInitialized = true;
}

void DarkAgesGame::Shutdown() {
	isInitialized = false;
}

// ============================================================================
// Campaign Management
// ============================================================================

void DarkAgesGame::StartNewGame( darkAgesDifficulty_t diff ) {
	difficulty = diff;
	currentLevel = 0;
	memset( &stats, 0, sizeof( stats ) );
	combatState.stamina = DA_STAMINA_MAX;
	
	LoadLevel( 0 );
}

void DarkAgesGame::LoadLevel( int levelIndex ) {
	if ( levelIndex < 0 || levelIndex >= DA_NUM_LEVELS ) {
		return;
	}
	
	currentLevel = levelIndex;
	
	const darkAgesLevelInfo_t * info = &DA_CAMPAIGN[levelIndex];
	
	stats.totalSecrets = info->numSecrets;
	stats.secretsFound = 0;
}

void DarkAgesGame::NextLevel() {
	if ( currentLevel < DA_NUM_LEVELS - 1 ) {
		LoadLevel( currentLevel + 1 );
	}
}

bool DarkAgesGame::IsLevelComplete() const {
	return false;
}

int DarkAgesGame::GetCurrentAct() const {
	if ( currentLevel < DA_NUM_LEVELS ) {
		return DA_CAMPAIGN[currentLevel].actNumber;
	}
	return 0;
}

const darkAgesLevelInfo_t * DarkAgesGame::GetLevelInfo( int index ) const {
	if ( index >= 0 && index < DA_NUM_LEVELS ) {
		return &DA_CAMPAIGN[index];
	}
	return NULL;
}

// ============================================================================
// Difficulty
// ============================================================================

const darkAgesDifficultySettings_t * DarkAgesGame::GetDifficultySettings() const {
	return &DA_DIFFICULTIES[difficulty];
}

// ============================================================================
// Combat System
// ============================================================================

void DarkAgesGame::UpdateCombat( float deltaTime ) {
	// Regenerate stamina
	if ( !combatState.isBlocking && !combatState.isDodging ) {
		combatState.stamina += DA_STAMINA_REGEN_RATE * deltaTime;
		if ( combatState.stamina > DA_STAMINA_MAX ) {
			combatState.stamina = DA_STAMINA_MAX;
		}
	}
	
	// Reset combo if too much time has passed
	if ( combatState.comboCount > 0 ) {
		float timeSinceAttack = deltaTime;
		if ( timeSinceAttack > DA_MELEE_COMBO_WINDOW ) {
			combatState.comboCount = 0;
			combatState.currentCombo = DA_COMBO_NONE;
		}
	}
	
	// Update parry window
	if ( combatState.isParrying ) {
		float parryElapsed = deltaTime;
		if ( parryElapsed > DA_PARRY_WINDOW ) {
			combatState.isParrying = false;
		}
	}
}

float DarkAgesGame::CalculateMeleeDamage( float baseDamage, int comboCount ) const {
	float damage = baseDamage;
	
	// Combo multiplier: each successive hit does more damage
	if ( comboCount > 1 ) {
		damage *= 1.0f + ( comboCount - 1 ) * DA_COMBO_DAMAGE_MULT;
	}
	
	// Cap at max combo
	if ( comboCount > DA_MAX_COMBO_HITS ) {
		damage *= 1.0f + ( DA_MAX_COMBO_HITS - 1 ) * DA_COMBO_DAMAGE_MULT;
	}
	
	// Apply difficulty modifier
	const darkAgesDifficultySettings_t * diffSettings = GetDifficultySettings();
	damage *= diffSettings->playerDamageMult;
	
	return damage;
}

bool DarkAgesGame::CheckParry( float attackTime ) const {
	if ( !combatState.isParrying ) {
		return false;
	}
	
	float parryElapsed = attackTime - combatState.lastBlockTime;
	return parryElapsed <= DA_PARRY_WINDOW;
}

bool DarkAgesGame::CheckBlock( float attackAngle ) const {
	if ( !combatState.isBlocking ) {
		return false;
	}
	
	return fabsf( attackAngle ) <= DA_SHIELD_BLOCK_ARC * 0.5f;
}

float DarkAgesGame::GetStaminaCost( darkAgesPlayerState_t action ) const {
	switch ( action ) {
		case DA_PSTATE_SPRINTING:
			return DA_STAMINA_SPRINT_COST;
		case DA_PSTATE_ATTACKING:
			return DA_STAMINA_ATTACK_COST;
		case DA_PSTATE_BLOCKING:
			return DA_STAMINA_BLOCK_COST;
		case DA_PSTATE_DODGING:
			return DA_STAMINA_DODGE_COST;
		default:
			return 0.0f;
	}
}

// ============================================================================
// Stats Tracking
// ============================================================================

void DarkAgesGame::RecordKill() {
	stats.totalKills++;
}

void DarkAgesGame::RecordDeath() {
	stats.totalDeaths++;
}

void DarkAgesGame::RecordSecret() {
	stats.secretsFound++;
}

void DarkAgesGame::RecordParry() {
	stats.parriesSuccessful++;
}

void DarkAgesGame::RecordBackstab() {
	stats.backstabs++;
}

// ============================================================================
// Save/Load
// ============================================================================

void DarkAgesGame::SaveGame( const char * saveName ) {
	// Save game state to file
	char filename[256];
	snprintf( filename, sizeof(filename), "savegames/%s.dksave", saveName );
	
	FILE * f = fopen( filename, "wb" );
	if ( f == NULL ) {
		return;
	}
	
	// Write header
	const char header[] = "DKAG";
	fwrite( header, 1, 4, f );
	
	// Write version
	int version = 1;
	fwrite( &version, sizeof(int), 1, f );
	
	// Write game state
	fwrite( &currentLevel, sizeof(int), 1, f );
	fwrite( &difficulty, sizeof(int), 1, f );
	fwrite( &stats, sizeof(darkAgesPlayerStats_t), 1, f );
	fwrite( &combatState, sizeof(darkAgesCombatState_t), 1, f );
	
	fclose( f );
}

void DarkAgesGame::LoadGame( const char * saveName ) {
	char filename[256];
	snprintf( filename, sizeof(filename), "savegames/%s.dksave", saveName );
	
	FILE * f = fopen( filename, "rb" );
	if ( f == NULL ) {
		return;
	}
	
	// Read and verify header
	char header[4];
	fread( header, 1, 4, f );
	if ( header[0] != 'D' || header[1] != 'K' || header[2] != 'A' || header[3] != 'G' ) {
		fclose( f );
		return;
	}
	
	// Read version
	int version;
	fread( &version, sizeof(int), 1, f );
	
	if ( version != 1 ) {
		fclose( f );
		return;
	}
	
	// Read game state
	fread( &currentLevel, sizeof(int), 1, f );
	fread( &difficulty, sizeof(int), 1, f );
	fread( &stats, sizeof(darkAgesPlayerStats_t), 1, f );
	fread( &combatState, sizeof(darkAgesCombatState_t), 1, f );
	
	fclose( f );
}

// ============================================================================
// Performance Optimization
// ============================================================================

void DarkAgesGame::OptimizeForHardware() {
	// Auto-detect and apply optimizations
	ApplyT460sOptimizations();
}

void DarkAgesGame::ApplyT460sOptimizations() {
	// ThinkPad T460s optimization settings:
	// - Intel Core i5-6300U (2C/4T, 2.4-3.0GHz)
	// - Intel HD Graphics 520 (24 EUs, 300-1050MHz)
	// - 8-20GB DDR4-2133
	// - 1920x1080 display
	//
	// Target: 120+ FPS
	// Strategy:
	// 1. Reduce shadow quality (biggest GPU impact on Intel HD)
	// 2. Use lower-res shadow maps
	// 3. Reduce particle count
	// 4. Use simpler shaders where possible
	// 5. Aggressive LOD distance
	// 6. Limit dynamic lights
	// 7. Use forward rendering path (lighter on Intel integrated)
	// 8. Reduce post-processing
}
