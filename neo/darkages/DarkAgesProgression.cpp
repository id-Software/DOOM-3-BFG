/*
===========================================================================

DOOM: The Dark Ages - Progression System Implementation

===========================================================================
*/

#include "DarkAgesProgression.h"
#include <cstring>
#include <cstdio>

// ============================================================================
// DarkAgesProgression Implementation
// ============================================================================

DarkAgesProgression & DarkAgesProgression::Instance() {
	static DarkAgesProgression instance;
	return instance;
}

DarkAgesProgression::DarkAgesProgression() :
	soulEssences( 0 ),
	totalStars( 0 )
{
	memset( upgradeLevels, 0, sizeof( upgradeLevels ) );
	memset( levelStars, 0, sizeof( levelStars ) );
	memset( levelsCompleted, 0, sizeof( levelsCompleted ) );
}

void DarkAgesProgression::Init() {
	soulEssences = 0;
	totalStars = 0;
	memset( upgradeLevels, 0, sizeof( upgradeLevels ) );
	memset( levelStars, 0, sizeof( levelStars ) );
	memset( levelsCompleted, 0, sizeof( levelsCompleted ) );
}

// ============================================================================
// Soul Essences
// ============================================================================

void DarkAgesProgression::AddSoulEssences( int amount ) {
	float soulGainMult = 1.0f + GetUpgradeValue( DA_UPG_SOUL_GAIN );
	soulEssences += (int)( amount * soulGainMult );
}

bool DarkAgesProgression::SpendSoulEssences( int amount ) {
	if ( soulEssences < amount ) {
		return false;
	}
	soulEssences -= amount;
	return true;
}

// ============================================================================
// Upgrades
// ============================================================================

int DarkAgesProgression::GetUpgradeLevel( darkAgesUpgradeId_t id ) const {
	if ( id < 0 || id >= DA_UPG_COUNT ) return 0;
	return upgradeLevels[id];
}

bool DarkAgesProgression::CanPurchaseUpgrade( darkAgesUpgradeId_t id ) const {
	if ( id < 0 || id >= DA_UPG_COUNT ) return false;
	if ( upgradeLevels[id] >= DA_UPGRADES[id].maxLevel ) return false;
	return soulEssences >= GetUpgradeCost( id );
}

int DarkAgesProgression::GetUpgradeCost( darkAgesUpgradeId_t id ) const {
	if ( id < 0 || id >= DA_UPG_COUNT ) return 999999;
	return DA_UPGRADES[id].baseCost + DA_UPGRADES[id].costPerLevel * upgradeLevels[id];
}

bool DarkAgesProgression::PurchaseUpgrade( darkAgesUpgradeId_t id ) {
	if ( !CanPurchaseUpgrade( id ) ) {
		return false;
	}
	
	int cost = GetUpgradeCost( id );
	if ( !SpendSoulEssences( cost ) ) {
		return false;
	}
	
	upgradeLevels[id]++;
	return true;
}

float DarkAgesProgression::GetUpgradeValue( darkAgesUpgradeId_t id ) const {
	if ( id < 0 || id >= DA_UPG_COUNT ) return 0.0f;
	return DA_UPGRADES[id].baseValue * upgradeLevels[id];
}

// ============================================================================
// Calculated Stats
// ============================================================================

float DarkAgesProgression::GetMeleeDamageMult() const {
	return 1.0f + GetUpgradeValue( DA_UPG_MELEE_DAMAGE );
}

float DarkAgesProgression::GetRangedDamageMult() const {
	return 1.0f + GetUpgradeValue( DA_UPG_RANGED_DAMAGE );
}

float DarkAgesProgression::GetMaxHealth() const {
	return DA_PLAYER_MAX_HEALTH + GetUpgradeValue( DA_UPG_MAX_HEALTH );
}

float DarkAgesProgression::GetMaxArmor() const {
	return DA_PLAYER_MAX_ARMOR + GetUpgradeValue( DA_UPG_MAX_ARMOR );
}

float DarkAgesProgression::GetMaxStamina() const {
	return DA_STAMINA_MAX + GetUpgradeValue( DA_UPG_STAMINA_MAX );
}

float DarkAgesProgression::GetSprintSpeedMult() const {
	return 1.0f + GetUpgradeValue( DA_UPG_SPRINT_SPEED );
}

float DarkAgesProgression::GetParryWindow() const {
	return DA_PARRY_WINDOW + GetUpgradeValue( DA_UPG_PARRY_WINDOW );
}

float DarkAgesProgression::GetShieldBlockMult() const {
	return DA_SHIELD_BLOCK_MULT - GetUpgradeValue( DA_UPG_SHIELD_STRENGTH );
}

float DarkAgesProgression::GetBackstabMult() const {
	return DA_BACKSTAB_MULT + GetUpgradeValue( DA_UPG_BACKSTAB_DAMAGE );
}

float DarkAgesProgression::GetDamageResist() const {
	return GetUpgradeValue( DA_UPG_DAMAGE_RESIST );
}

float DarkAgesProgression::GetStaminaRegenMult() const {
	return 1.0f + GetUpgradeValue( DA_UPG_STAMINA_REGEN );
}

int DarkAgesProgression::GetGloryKillHeal() const {
	return 25 + (int)GetUpgradeValue( DA_UPG_GLORY_KILL_HEAL );
}

// ============================================================================
// Level Completion
// ============================================================================

void DarkAgesProgression::OnLevelComplete( int levelNumber, int kills, int secrets, int totalSecrets, float time ) {
	if ( levelNumber < 0 || levelNumber >= DA_MAX_LEVELS ) return;
	
	levelsCompleted[levelNumber] = true;
	
	// Calculate stars (1-3)
	int stars = 1;
	
	// Star for finding all secrets
	if ( secrets >= totalSecrets ) {
		stars++;
	}
	
	// Star for fast completion (under estimated time)
	const darkAgesLevelInfo_t * info = &DA_CAMPAIGN[levelNumber];
	if ( time < info->estimatedMinutes * 60.0f ) {
		stars++;
	}
	
	if ( stars > levelStars[levelNumber] ) {
		totalStars += stars - levelStars[levelNumber];
		levelStars[levelNumber] = stars;
	}
	
	// Soul essence rewards
	int baseReward = 50 + levelNumber * 20;
	int secretBonus = secrets * 25;
	int starBonus = stars * 50;
	AddSoulEssences( baseReward + secretBonus + starBonus );
}

// ============================================================================
// Save/Load
// ============================================================================

void DarkAgesProgression::Save( const char * filename ) {
	FILE * f = fopen( filename, "wb" );
	if ( f == NULL ) return;
	
	const char header[] = "DKPG";
	fwrite( header, 1, 4, f );
	
	int version = 1;
	fwrite( &version, sizeof(int), 1, f );
	
	fwrite( &soulEssences, sizeof(int), 1, f );
	fwrite( &totalStars, sizeof(int), 1, f );
	fwrite( upgradeLevels, sizeof(int), DA_UPG_COUNT, f );
	fwrite( levelStars, sizeof(int), DA_MAX_LEVELS, f );
	fwrite( levelsCompleted, sizeof(bool), DA_MAX_LEVELS, f );
	
	fclose( f );
}

void DarkAgesProgression::Load( const char * filename ) {
	FILE * f = fopen( filename, "rb" );
	if ( f == NULL ) return;
	
	char header[4];
	fread( header, 1, 4, f );
	if ( header[0] != 'D' || header[1] != 'K' || header[2] != 'P' || header[3] != 'G' ) {
		fclose( f );
		return;
	}
	
	int version;
	fread( &version, sizeof(int), 1, f );
	if ( version != 1 ) {
		fclose( f );
		return;
	}
	
	fread( &soulEssences, sizeof(int), 1, f );
	fread( &totalStars, sizeof(int), 1, f );
	fread( upgradeLevels, sizeof(int), DA_UPG_COUNT, f );
	fread( levelStars, sizeof(int), DA_MAX_LEVELS, f );
	fread( levelsCompleted, sizeof(bool), DA_MAX_LEVELS, f );
	
	fclose( f );
}
