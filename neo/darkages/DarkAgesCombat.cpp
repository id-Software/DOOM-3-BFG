/*
===========================================================================

DOOM: The Dark Ages - Combat System Implementation

===========================================================================
*/

#include "DarkAgesCombat.h"
#include <cmath>
#include <cstring>

// ============================================================================
// DarkAgesCombatSystem Implementation
// ============================================================================

DarkAgesCombatSystem & DarkAgesCombatSystem::Instance() {
	static DarkAgesCombatSystem instance;
	return instance;
}

DarkAgesCombatSystem::DarkAgesCombatSystem() :
	stamina( DA_STAMINA_MAX ),
	comboCount( 0 ),
	lastHitTime( 0.0f ),
	comboWindowTimer( 0.0f ),
	activeCombo( NULL ),
	comboStep( 0 ),
	gloryKillAvailable( false )
{
}

void DarkAgesCombatSystem::Init() {
	stamina = DA_STAMINA_MAX;
	comboCount = 0;
	lastHitTime = 0.0f;
	comboWindowTimer = 0.0f;
	activeCombo = NULL;
	comboStep = 0;
	gloryKillAvailable = false;
}

void DarkAgesCombatSystem::Update( float deltaTime ) {
	RegenerateStamina( deltaTime );
	
	// Update combo window
	if ( comboCount > 0 ) {
		comboWindowTimer -= deltaTime;
		if ( comboWindowTimer <= 0.0f ) {
			ResetCombo();
		}
	}
}

// ============================================================================
// Attack Processing
// ============================================================================

float DarkAgesCombatSystem::ProcessMeleeAttack( darkAgesWeaponType_t weapon, darkAgesMeleeType_t type, float baseDamage ) {
	float staminaCost = DA_STAMINA_ATTACK_COST;
	
	// Heavy attacks cost more stamina
	if ( type == DA_MELEE_HEAVY || type == DA_MELEE_OVERHEAD ) {
		staminaCost *= 1.5f;
	}
	
	// Check stamina
	if ( !HasStamina( staminaCost ) ) {
		return 0.0f;
	}
	
	ConsumeStamina( staminaCost );
	
	// Register the hit for combo tracking
	RegisterHit( weapon, type );
	
	// Calculate damage with combo multiplier
	float damage = baseDamage * GetComboMultiplier();
	
	// Apply difficulty settings
	const darkAgesDifficultySettings_t * diff = DarkAgesGame::Instance().GetDifficultySettings();
	damage *= diff->playerDamageMult;
	
	return damage;
}

bool DarkAgesCombatSystem::ProcessBlock( float incomingDamage, float attackAngle, float & reducedDamage ) {
	if ( !HasStamina( DA_STAMINA_BLOCK_COST ) ) {
		reducedDamage = incomingDamage;
		return false;
	}
	
	// Check if attack is within block arc
	if ( fabsf( attackAngle ) > DA_SHIELD_BLOCK_ARC * 0.5f ) {
		reducedDamage = incomingDamage;
		return false;
	}
	
	ConsumeStamina( DA_STAMINA_BLOCK_COST );
	
	// Reduce damage based on shield
	reducedDamage = incomingDamage * DA_SHIELD_BLOCK_MULT;
	
	return true;
}

bool DarkAgesCombatSystem::ProcessParry( float attackTime, float & counterDamage ) {
	float timeSinceBlock = attackTime - lastHitTime;
	
	// Parry window check
	if ( timeSinceBlock > DA_PARRY_WINDOW ) {
		return false;
	}
	
	if ( !HasStamina( DA_STAMINA_BLOCK_COST * 0.5f ) ) {
		return false;
	}
	
	ConsumeStamina( DA_STAMINA_BLOCK_COST * 0.5f );
	
	// Successful parry - counter damage
	counterDamage = 50.0f * DA_PARRY_DAMAGE_MULT;
	
	DarkAgesGame::Instance().RecordParry();
	
	return true;
}

bool DarkAgesCombatSystem::ProcessDodge( float dodgeDirection ) {
	if ( !HasStamina( DA_STAMINA_DODGE_COST ) ) {
		return false;
	}
	
	ConsumeStamina( DA_STAMINA_DODGE_COST );
	
	return true;
}

// ============================================================================
// Combo System
// ============================================================================

void DarkAgesCombatSystem::RegisterHit( darkAgesWeaponType_t weapon, darkAgesMeleeType_t type ) {
	// Check if within combo window
	if ( comboWindowTimer > 0.0f && comboCount > 0 ) {
		comboCount++;
		if ( comboCount > DA_MAX_COMBO_HITS ) {
			comboCount = DA_MAX_COMBO_HITS;
		}
	} else {
		comboCount = 1;
	}
	
	// Reset combo window timer
	comboWindowTimer = DA_MELEE_COMBO_WINDOW;
	
	// Try to match a combo chain
	if ( comboCount == 1 ) {
		// Start matching combo chains
		switch ( weapon ) {
			case DA_WEAPON_SWORD:
				activeCombo = &DA_COMBO_SWORD_FURY;
				break;
			case DA_WEAPON_MACE:
				activeCombo = &DA_COMBO_MACE_CRUSHER;
				break;
			case DA_WEAPON_AXE:
				activeCombo = &DA_COMBO_AXE_CLEAVE;
				break;
			case DA_WEAPON_FLAIL:
				activeCombo = &DA_COMBO_FLAIL_CHAOS;
				break;
			case DA_WEAPON_GREATHAMMER:
				activeCombo = &DA_COMBO_HAMMER_EARTHQUAKE;
				break;
			default:
				activeCombo = NULL;
				break;
		}
		comboStep = 0;
	} else if ( activeCombo != NULL ) {
		comboStep++;
		if ( comboStep >= activeCombo->numMoves ) {
			comboStep = activeCombo->numMoves - 1;
		}
	}
}

float DarkAgesCombatSystem::GetComboMultiplier() const {
	if ( comboCount <= 1 ) {
		return 1.0f;
	}
	
	float mult = 1.0f + ( comboCount - 1 ) * DA_COMBO_DAMAGE_MULT;
	
	// If we're in a named combo chain, use the chain's multiplier
	if ( activeCombo != NULL && comboStep < activeCombo->numMoves ) {
		mult *= activeCombo->moves[comboStep].damageMultiplier;
	}
	
	return mult;
}

void DarkAgesCombatSystem::ResetCombo() {
	comboCount = 0;
	comboWindowTimer = 0.0f;
	activeCombo = NULL;
	comboStep = 0;
}

bool DarkAgesCombatSystem::IsComboFinisher() const {
	if ( activeCombo == NULL ) {
		return false;
	}
	return comboStep == activeCombo->numMoves - 1;
}

// ============================================================================
// Stamina Management
// ============================================================================

void DarkAgesCombatSystem::ConsumeStamina( float amount ) {
	stamina -= amount;
	if ( stamina < 0.0f ) {
		stamina = 0.0f;
	}
}

void DarkAgesCombatSystem::RegenerateStamina( float deltaTime ) {
	stamina += DA_STAMINA_REGEN_RATE * deltaTime;
	if ( stamina > DA_STAMINA_MAX ) {
		stamina = DA_STAMINA_MAX;
	}
}

// ============================================================================
// Glory Kill System
// ============================================================================

bool DarkAgesCombatSystem::CanGloryKill() const {
	return gloryKillAvailable;
}

float DarkAgesCombatSystem::GetGloryKillDamage() const {
	return 9999.0f;
}

void DarkAgesCombatSystem::PerformGloryKill() {
	gloryKillAvailable = false;
	// Restore some health on glory kill
}

// ============================================================================
// Backstab Detection
// ============================================================================

bool DarkAgesCombatSystem::IsBackstab( float attackerYaw, float victimYaw ) const {
	float angleDiff = fabsf( attackerYaw - victimYaw );
	if ( angleDiff > 180.0f ) {
		angleDiff = 360.0f - angleDiff;
	}
	
	// If attacker is behind the victim (within 60 degree cone)
	return angleDiff < 30.0f;
}

// ============================================================================
// DarkAgesEnemyCombatAI Implementation
// ============================================================================

void DarkAgesEnemyCombatAI::Init( int enemyType, float healthMultiplier, float dmgMult ) {
	state = ESTATE_IDLE;
	maxHealth = 100.0f * healthMultiplier;
	health = maxHealth;
	damageMult = dmgMult;
	blockChance = 0.0f;
	aggressionLevel = 0.5f;
	lastAttackTime = 0.0f;
	lastBlockTime = 0.0f;
	staggerTimer = 0.0f;
	hitsTaken = 0;
	groupId = -1;
	isLeader = false;
	
	// Set per-enemy-type parameters
	switch ( enemyType ) {
		case 0: // Undead Soldier
			blockChance = 0.1f;
			aggressionLevel = 0.7f;
			break;
		case 1: // Dark Knight
			blockChance = 0.4f;
			aggressionLevel = 0.5f;
			break;
		case 2: // Hell Hound
			blockChance = 0.0f;
			aggressionLevel = 0.9f;
			break;
		case 3: // Plague Sorcerer
			blockChance = 0.1f;
			aggressionLevel = 0.3f;
			break;
		case 4: // Siege Demon
			blockChance = 0.2f;
			aggressionLevel = 0.6f;
			break;
		case 5: // Wraith
			blockChance = 0.0f;
			aggressionLevel = 0.4f;
			break;
		case 6: // Fire Drake
			blockChance = 0.0f;
			aggressionLevel = 0.5f;
			break;
		case 7: // Iron Golem
			blockChance = 0.6f;
			aggressionLevel = 0.4f;
			break;
		default:
			break;
	}
}

void DarkAgesEnemyCombatAI::Update( float deltaTime ) {
	// Update stagger timer
	if ( staggerTimer > 0.0f ) {
		staggerTimer -= deltaTime;
		if ( staggerTimer <= 0.0f ) {
			staggerTimer = 0.0f;
			if ( state == ESTATE_STAGGERED ) {
				state = ESTATE_PURSUING;
			}
		}
	}
}

void DarkAgesEnemyCombatAI::SetState( enemyState_t newState ) {
	state = newState;
}

DarkAgesEnemyCombatAI::attackPattern_t DarkAgesEnemyCombatAI::ChooseAttack() const {
	// Higher aggression = more combos
	float roll = (float)( rand() % 100 ) / 100.0f;
	
	if ( aggressionLevel > 0.7f ) {
		if ( roll < 0.3f ) return EATTACK_COMBO_3;
		if ( roll < 0.6f ) return EATTACK_COMBO_2;
		if ( roll < 0.8f ) return EATTACK_LUNGE;
		return EATTACK_SINGLE;
	} else if ( aggressionLevel > 0.4f ) {
		if ( roll < 0.2f ) return EATTACK_COMBO_2;
		if ( roll < 0.5f ) return EATTACK_SINGLE;
		if ( roll < 0.7f ) return EATTACK_SWEEP;
		return EATTACK_SINGLE;
	} else {
		if ( roll < 0.5f ) return EATTACK_RANGED;
		if ( roll < 0.7f ) return EATTACK_SINGLE;
		return EATTACK_SPECIAL;
	}
}

bool DarkAgesEnemyCombatAI::ShouldBlock() const {
	if ( blockChance <= 0.0f ) return false;
	float roll = (float)( rand() % 100 ) / 100.0f;
	return roll < blockChance;
}

bool DarkAgesEnemyCombatAI::ShouldRetreat() const {
	float healthRatio = health / maxHealth;
	return healthRatio < 0.2f && aggressionLevel < 0.5f;
}

bool DarkAgesEnemyCombatAI::ShouldFlank() const {
	return hitsTaken > 3 && aggressionLevel > 0.3f;
}

void DarkAgesEnemyCombatAI::RegisterWithGroup() {
	// Would register with nearby enemies for coordinated attacks
}

void DarkAgesEnemyCombatAI::CoordinateAttack() {
	// Only one enemy in the group attacks at a time (Dark Souls style)
	// Others circle and look for openings
}
