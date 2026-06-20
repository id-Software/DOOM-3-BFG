/*
===========================================================================

DOOM: The Dark Ages - Combat System
Advanced melee combat with combos, parries, dodges, and shield blocking

===========================================================================
*/

#ifndef __DARKAGES_COMBAT_H__
#define __DARKAGES_COMBAT_H__

#include "DarkAgesGame.h"

// ============================================================================
// Melee Attack Types
// ============================================================================

enum darkAgesMeleeType_t {
	DA_MELEE_LIGHT = 0,
	DA_MELEE_HEAVY,
	DA_MELEE_THRUST,
	DA_MELEE_OVERHEAD,
	DA_MELEE_SWEEP,
	DA_MELEE_UPPERCUT,
	DA_MELEE_COUNT
};

// ============================================================================
// Combat Combo Definitions
// Each weapon has unique combos
// ============================================================================

struct darkAgesComboMove_t {
	darkAgesMeleeType_t		attackType;
	float					damageMultiplier;
	float					speedMultiplier;
	float					staminaCost;
	float					knockback;
	bool					canChain;
	const char *			animName;
};

struct darkAgesComboChain_t {
	const char *			name;
	darkAgesWeaponType_t	weapon;
	int						numMoves;
	darkAgesComboMove_t		moves[DA_MAX_COMBO_HITS];
	float					finisherDamageMult;
	const char *			finisherFx;
};

// ============================================================================
// Predefined Combo Chains
// ============================================================================

// Sword combos
static const darkAgesComboChain_t DA_COMBO_SWORD_FURY = {
	"Fury of Steel",
	DA_WEAPON_SWORD,
	5,
	{
		{ DA_MELEE_LIGHT,		1.0f,	1.0f,	8.0f,	2000.0f,	true,	"sword_slash_r" },
		{ DA_MELEE_LIGHT,		1.1f,	1.1f,	8.0f,	2500.0f,	true,	"sword_slash_l" },
		{ DA_MELEE_HEAVY,		1.5f,	0.8f,	15.0f,	5000.0f,	true,	"sword_overhead" },
		{ DA_MELEE_THRUST,		1.3f,	1.2f,	12.0f,	3000.0f,	true,	"sword_thrust" },
		{ DA_MELEE_OVERHEAD,	2.5f,	0.6f,	25.0f,	10000.0f,	false,	"sword_finisher" },
	},
	3.0f,
	"fx/combat/sword_fury_finisher"
};

static const darkAgesComboChain_t DA_COMBO_SWORD_WHIRLWIND = {
	"Whirlwind",
	DA_WEAPON_SWORD,
	3,
	{
		{ DA_MELEE_SWEEP,		0.8f,	1.3f,	10.0f,	3000.0f,	true,	"sword_sweep_1" },
		{ DA_MELEE_SWEEP,		0.9f,	1.4f,	10.0f,	3500.0f,	true,	"sword_sweep_2" },
		{ DA_MELEE_SWEEP,		1.5f,	1.5f,	20.0f,	8000.0f,	false,	"sword_sweep_3" },
	},
	2.0f,
	"fx/combat/sword_whirlwind"
};

// Mace combos
static const darkAgesComboChain_t DA_COMBO_MACE_CRUSHER = {
	"Skull Crusher",
	DA_WEAPON_MACE,
	4,
	{
		{ DA_MELEE_HEAVY,		1.2f,	0.8f,	12.0f,	5000.0f,	true,	"mace_smash_1" },
		{ DA_MELEE_HEAVY,		1.4f,	0.7f,	15.0f,	7000.0f,	true,	"mace_smash_2" },
		{ DA_MELEE_OVERHEAD,	1.8f,	0.6f,	20.0f,	10000.0f,	true,	"mace_overhead" },
		{ DA_MELEE_OVERHEAD,	3.0f,	0.4f,	30.0f,	20000.0f,	false,	"mace_finisher" },
	},
	4.0f,
	"fx/combat/mace_crusher_finisher"
};

// Axe combos
static const darkAgesComboChain_t DA_COMBO_AXE_CLEAVE = {
	"Cleaving Storm",
	DA_WEAPON_AXE,
	4,
	{
		{ DA_MELEE_LIGHT,		1.0f,	1.1f,	9.0f,	3000.0f,	true,	"axe_chop_r" },
		{ DA_MELEE_LIGHT,		1.1f,	1.2f,	9.0f,	3500.0f,	true,	"axe_chop_l" },
		{ DA_MELEE_HEAVY,		1.6f,	0.7f,	18.0f,	8000.0f,	true,	"axe_heavy_chop" },
		{ DA_MELEE_SWEEP,		2.0f,	0.8f,	22.0f,	12000.0f,	false,	"axe_cleave" },
	},
	2.5f,
	"fx/combat/axe_cleave_finisher"
};

// Flail combos
static const darkAgesComboChain_t DA_COMBO_FLAIL_CHAOS = {
	"Chains of Chaos",
	DA_WEAPON_FLAIL,
	4,
	{
		{ DA_MELEE_SWEEP,		0.9f,	1.2f,	10.0f,	4000.0f,	true,	"flail_spin_1" },
		{ DA_MELEE_SWEEP,		1.0f,	1.3f,	10.0f,	4500.0f,	true,	"flail_spin_2" },
		{ DA_MELEE_SWEEP,		1.2f,	1.4f,	12.0f,	5000.0f,	true,	"flail_spin_3" },
		{ DA_MELEE_OVERHEAD,	2.5f,	0.5f,	25.0f,	15000.0f,	false,	"flail_overhead" },
	},
	3.0f,
	"fx/combat/flail_chaos_finisher"
};

// Great Hammer combos
static const darkAgesComboChain_t DA_COMBO_HAMMER_EARTHQUAKE = {
	"Earthquake",
	DA_WEAPON_GREATHAMMER,
	3,
	{
		{ DA_MELEE_HEAVY,		1.5f,	0.6f,	15.0f,	8000.0f,	true,	"hammer_smash_1" },
		{ DA_MELEE_HEAVY,		2.0f,	0.5f,	20.0f,	12000.0f,	true,	"hammer_smash_2" },
		{ DA_MELEE_OVERHEAD,	4.0f,	0.3f,	40.0f,	25000.0f,	false,	"hammer_groundpound" },
	},
	5.0f,
	"fx/combat/hammer_earthquake"
};

// ============================================================================
// DarkAgesCombatSystem class
// Manages all combat interactions
// ============================================================================

class DarkAgesCombatSystem {
public:
	static DarkAgesCombatSystem &	Instance();
	
	void				Init();
	void				Update( float deltaTime );
	
	// Attack processing
	float				ProcessMeleeAttack( darkAgesWeaponType_t weapon, darkAgesMeleeType_t type, float baseDamage );
	bool				ProcessBlock( float incomingDamage, float attackAngle, float & reducedDamage );
	bool				ProcessParry( float attackTime, float & counterDamage );
	bool				ProcessDodge( float dodgeDirection );
	
	// Combo system
	void				RegisterHit( darkAgesWeaponType_t weapon, darkAgesMeleeType_t type );
	int					GetComboCount() const { return comboCount; }
	float				GetComboMultiplier() const;
	void				ResetCombo();
	bool				IsComboFinisher() const;
	const darkAgesComboChain_t * GetActiveCombo() const { return activeCombo; }
	
	// Stamina management
	float				GetStamina() const { return stamina; }
	bool				HasStamina( float cost ) const { return stamina >= cost; }
	void				ConsumeStamina( float amount );
	void				RegenerateStamina( float deltaTime );
	
	// Glory Kill system (execution moves on staggered enemies)
	bool				CanGloryKill() const;
	float				GetGloryKillDamage() const;
	void				PerformGloryKill();
	
	// Backstab detection
	bool				IsBackstab( float attackerYaw, float victimYaw ) const;
	
private:
	DarkAgesCombatSystem();
	
	float				stamina;
	int					comboCount;
	float				lastHitTime;
	float				comboWindowTimer;
	const darkAgesComboChain_t * activeCombo;
	int					comboStep;
	bool				gloryKillAvailable;
};

// ============================================================================
// DarkAgesEnemyCombatAI
// Enhanced AI combat behaviors for medieval combat
// ============================================================================

class DarkAgesEnemyCombatAI {
public:
	enum enemyState_t {
		ESTATE_IDLE,
		ESTATE_PATROL,
		ESTATE_ALERT,
		ESTATE_PURSUING,
		ESTATE_ATTACKING,
		ESTATE_BLOCKING,
		ESTATE_STAGGERED,
		ESTATE_RETREATING,
		ESTATE_FLANKING,
		ESTATE_CIRCLING,
		ESTATE_DEAD
	};
	
	enum attackPattern_t {
		EATTACK_SINGLE,
		EATTACK_COMBO_2,
		EATTACK_COMBO_3,
		EATTACK_LUNGE,
		EATTACK_SWEEP,
		EATTACK_RANGED,
		EATTACK_SPECIAL
	};
	
	void				Init( int enemyType, float healthMult, float damageMult );
	void				Update( float deltaTime );
	
	// State management
	enemyState_t		GetState() const { return state; }
	void				SetState( enemyState_t newState );
	
	// Combat decisions
	attackPattern_t		ChooseAttack() const;
	bool				ShouldBlock() const;
	bool				ShouldRetreat() const;
	bool				ShouldFlank() const;
	
	// Group combat coordination
	void				RegisterWithGroup();
	void				CoordinateAttack();
	
private:
	enemyState_t		state;
	float				health;
	float				maxHealth;
	float				damageMult;
	float				blockChance;
	float				aggressionLevel;
	float				lastAttackTime;
	float				lastBlockTime;
	float				staggerTimer;
	int					hitsTaken;
	int					groupId;
	bool				isLeader;
};

#endif /* !__DARKAGES_COMBAT_H__ */
