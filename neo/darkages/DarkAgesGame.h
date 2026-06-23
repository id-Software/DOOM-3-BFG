/*
===========================================================================

DOOM: The Dark Ages
Copyright (C) 2024 - Dark Ages Total Conversion

Based on Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.

This file is part of DOOM: The Dark Ages.

===========================================================================
*/

#ifndef __DARKAGES_GAME_H__
#define __DARKAGES_GAME_H__

// ============================================================================
// DOOM: The Dark Ages - Core Game System
// Medieval FPS inspired by DOOM: The Dark Ages
// ============================================================================

#define DARKAGES_VERSION		"1.0.0"
#define DARKAGES_GAME_NAME		"DOOM: The Dark Ages"
#define DARKAGES_WINDOW_TITLE	"DOOM: The Dark Ages"

// ============================================================================
// Game Constants
// ============================================================================

static const int DA_MAX_LEVELS				= 25;
static const int DA_MAX_WEAPONS				= 10;
static const int DA_MAX_ENEMIES_PER_LEVEL	= 64;
static const int DA_MAX_ITEMS_PER_LEVEL		= 128;
static const int DA_MAX_SECRETS_PER_LEVEL	= 10;
static const int DA_MAX_OBJECTIVES			= 8;
static const int DA_NUM_ACTS				= 5;

// Player constants
static const int DA_PLAYER_MAX_HEALTH		= 200;
static const int DA_PLAYER_MAX_ARMOR		= 200;
static const int DA_PLAYER_START_HEALTH		= 100;
static const int DA_PLAYER_START_ARMOR		= 0;
static const float DA_PLAYER_WALK_SPEED		= 140.0f;
static const float DA_PLAYER_RUN_SPEED		= 280.0f;
static const float DA_PLAYER_SPRINT_SPEED	= 400.0f;
static const float DA_PLAYER_JUMP_HEIGHT	= 48.0f;

// Combat constants
static const float DA_MELEE_COMBO_WINDOW	= 0.8f;
static const int DA_MAX_COMBO_HITS			= 5;
static const float DA_COMBO_DAMAGE_MULT		= 0.25f;
static const float DA_SHIELD_BLOCK_ARC		= 60.0f;
static const float DA_SHIELD_BLOCK_MULT		= 0.2f;
static const float DA_PARRY_WINDOW			= 0.3f;
static const float DA_PARRY_DAMAGE_MULT		= 2.0f;
static const float DA_BACKSTAB_MULT			= 3.0f;

// Stamina system
static const float DA_STAMINA_MAX			= 100.0f;
static const float DA_STAMINA_REGEN_RATE	= 15.0f;
static const float DA_STAMINA_SPRINT_COST	= 20.0f;
static const float DA_STAMINA_ATTACK_COST	= 10.0f;
static const float DA_STAMINA_BLOCK_COST	= 5.0f;
static const float DA_STAMINA_DODGE_COST	= 25.0f;

// ============================================================================
// Enumerations
// ============================================================================

enum darkAgesAct_t {
	DA_ACT_FALLEN_KINGDOM = 0,
	DA_ACT_DARK_LANDS,
	DA_ACT_UNDERWORLD,
	DA_ACT_CURSED_REALM,
	DA_ACT_HELLS_DOMAIN,
	DA_ACT_COUNT
};

enum darkAgesWeaponType_t {
	DA_WEAPON_FISTS = 0,
	DA_WEAPON_SWORD,
	DA_WEAPON_MACE,
	DA_WEAPON_AXE,
	DA_WEAPON_FLAIL,
	DA_WEAPON_CROSSBOW,
	DA_WEAPON_THROWAXE,
	DA_WEAPON_SHIELD,
	DA_WEAPON_HELLSTAFF,
	DA_WEAPON_GREATHAMMER,
	DA_WEAPON_COUNT
};

enum darkAgesDifficulty_t {
	DA_DIFFICULTY_SQUIRE = 0,
	DA_DIFFICULTY_KNIGHT,
	DA_DIFFICULTY_CHAMPION,
	DA_DIFFICULTY_NIGHTMARE,
	DA_DIFFICULTY_COUNT
};

enum darkAgesPlayerState_t {
	DA_PSTATE_IDLE = 0,
	DA_PSTATE_RUNNING,
	DA_PSTATE_SPRINTING,
	DA_PSTATE_ATTACKING,
	DA_PSTATE_BLOCKING,
	DA_PSTATE_DODGING,
	DA_PSTATE_PARRYING,
	DA_PSTATE_STAGGERED,
	DA_PSTATE_DEAD
};

enum darkAgesComboType_t {
	DA_COMBO_NONE = 0,
	DA_COMBO_LIGHT_LIGHT,
	DA_COMBO_LIGHT_HEAVY,
	DA_COMBO_HEAVY_LIGHT,
	DA_COMBO_HEAVY_HEAVY,
	DA_COMBO_SPECIAL
};

// ============================================================================
// Structures
// ============================================================================

struct darkAgesLevelInfo_t {
	const char *	name;
	const char *	mapFile;
	const char *	description;
	int				actNumber;
	int				levelNumber;
	int				numSecrets;
	int				estimatedMinutes;
	bool			hasBoss;
	const char *	bossType;
	const char *	nextMap;
};

struct darkAgesPlayerStats_t {
	int				totalKills;
	int				totalDeaths;
	int				secretsFound;
	int				totalSecrets;
	float			completionTime;
	int				comboMaxHits;
	int				parriesSuccessful;
	int				backstabs;
	int				itemsCollected;
	float			damageDealt;
	float			damageTaken;
};

struct darkAgesCombatState_t {
	float			stamina;
	int				comboCount;
	float			lastAttackTime;
	float			lastBlockTime;
	float			lastDodgeTime;
	bool			isBlocking;
	bool			isParrying;
	bool			isDodging;
	darkAgesComboType_t	currentCombo;
};

struct darkAgesUpgrade_t {
	const char *	name;
	const char *	description;
	int				cost;
	int				maxLevel;
	int				currentLevel;
};

// ============================================================================
// Campaign Structure
// ============================================================================

static const darkAgesLevelInfo_t DA_CAMPAIGN[] = {
	// ACT I: The Fallen Kingdom
	{ "The Burning Village",	"maps/darkages/da01_burning_village",	"Your village burns...",			0, 1,  3, 45,  false, "",							"maps/darkages/da02_castle_siege" },
	{ "Castle Under Siege",		"maps/darkages/da02_castle_siege",		"Defend the castle...",			0, 2,  4, 60,  false, "",							"maps/darkages/da03_kings_crypt" },
	{ "The King's Crypt",		"maps/darkages/da03_kings_crypt",		"Descend into the crypts...",	0, 3,  5, 50,  false, "",							"maps/darkages/da04_forest_of_shadows" },
	{ "Forest of Shadows",		"maps/darkages/da04_forest_of_shadows",	"A cursed forest...",			0, 4,  6, 55,  false, "",							"maps/darkages/da05_mountain_pass" },
	{ "The Mountain Pass",		"maps/darkages/da05_mountain_pass",		"Cross the mountain...",		0, 5,  4, 70,  true,  "monster_darkages_firedrake",	"maps/darkages/da06_dark_cathedral" },
	
	// ACT II: The Dark Lands
	{ "The Dark Cathedral",		"maps/darkages/da06_dark_cathedral",	"A corrupted cathedral...",		1, 6,  5, 65,  false, "",							"maps/darkages/da07_plague_town" },
	{ "Plague Town",			"maps/darkages/da07_plague_town",		"A plagued town...",			1, 7,  4, 70,  false, "",							"maps/darkages/da08_iron_fortress" },
	{ "The Iron Fortress",		"maps/darkages/da08_iron_fortress",		"A fortress of black iron...",	1, 8,  6, 75,  false, "",							"maps/darkages/da09_bone_wastes" },
	{ "The Bone Wastes",		"maps/darkages/da09_bone_wastes",		"A desert of bones...",			1, 9,  5, 60,  false, "",							"maps/darkages/da10_siege_demon_lair" },
	{ "Siege Demon's Lair",		"maps/darkages/da10_siege_demon_lair",	"Siege Demon lair...",			1, 10, 4, 80,  true,  "monster_darkages_siege_demon", "maps/darkages/da11_frozen_depths" },
	
	// ACT III: The Underworld
	{ "The Frozen Depths",		"maps/darkages/da11_frozen_depths",		"Ice caverns...",				2, 11, 5, 55,  false, "",							"maps/darkages/da12_lava_forge" },
	{ "The Lava Forge",			"maps/darkages/da12_lava_forge",		"An ancient forge...",			2, 12, 4, 65,  false, "",							"maps/darkages/da13_tomb_of_heroes" },
	{ "Tomb of Fallen Heroes",	"maps/darkages/da13_tomb_of_heroes",	"Ancient tomb...",				2, 13, 7, 60,  false, "",							"maps/darkages/da14_crystal_caverns" },
	{ "Crystal Caverns",		"maps/darkages/da14_crystal_caverns",	"Crystal-filled caverns...",		2, 14, 5, 70,  false, "",							"maps/darkages/da15_dragon_den" },
	{ "The Dragon's Den",		"maps/darkages/da15_dragon_den",		"Dragon King's lair...",		2, 15, 4, 90,  true,  "monster_darkages_dragonking", "maps/darkages/da16_ruined_abbey" },
	
	// ACT IV: The Cursed Realm
	{ "The Ruined Abbey",		"maps/darkages/da16_ruined_abbey",		"Fractured reality...",			3, 16, 5, 55,  false, "",							"maps/darkages/da17_blood_swamp" },
	{ "The Blood Swamp",		"maps/darkages/da17_blood_swamp",		"Swamp of blood...",			3, 17, 4, 65,  false, "",							"maps/darkages/da18_shadow_maze" },
	{ "The Shadow Maze",		"maps/darkages/da18_shadow_maze",		"Shifting maze...",				3, 18, 8, 75,  false, "",							"maps/darkages/da19_demon_arena" },
	{ "The Demon Arena",		"maps/darkages/da19_demon_arena",		"Demonic colosseum...",			3, 19, 3, 80,  false, "",							"maps/darkages/da20_tower_of_despair" },
	{ "Tower of Despair",		"maps/darkages/da20_tower_of_despair",	"Towering hellspire...",		3, 20, 6, 85,  false, "",							"maps/darkages/da21_gates_of_hell" },
	
	// ACT V: Hell's Domain
	{ "Gates of Hell",			"maps/darkages/da21_gates_of_hell",		"The gates of Hell...",			4, 21, 5, 75,  false, "",							"maps/darkages/da22_river_of_souls" },
	{ "River of Lost Souls",	"maps/darkages/da22_river_of_souls",	"River of souls...",			4, 22, 4, 65,  false, "",							"maps/darkages/da23_fortress_of_doom" },
	{ "Fortress of Doom",		"maps/darkages/da23_fortress_of_doom",	"The Dark Lord's fortress...",	4, 23, 7, 90,  false, "",							"maps/darkages/da24_throne_of_darkness" },
	{ "Throne of Darkness",		"maps/darkages/da24_throne_of_darkness","The final throne...",			4, 24, 5, 100, true,  "monster_darkages_darklord",	"maps/darkages/da25_final_stand" },
	{ "The Final Stand",		"maps/darkages/da25_final_stand",		"Hell collapses...",			4, 25, 10, 90, false, "",							"" },
};

static const int DA_NUM_LEVELS = sizeof(DA_CAMPAIGN) / sizeof(DA_CAMPAIGN[0]);

// ============================================================================
// Act Names
// ============================================================================

static const char * DA_ACT_NAMES[] = {
	"Act I: The Fallen Kingdom",
	"Act II: The Dark Lands",
	"Act III: The Underworld",
	"Act IV: The Cursed Realm",
	"Act V: Hell's Domain"
};

// ============================================================================
// Difficulty Settings
// ============================================================================

struct darkAgesDifficultySettings_t {
	const char *	name;
	float			enemyHealthMult;
	float			enemyDamageMult;
	float			playerDamageMult;
	float			enemySpeedMult;
	float			itemDropMult;
	int				extraEnemies;
};

static const darkAgesDifficultySettings_t DA_DIFFICULTIES[] = {
	{ "Squire",		0.6f,	0.5f,	1.5f,	0.8f,	1.5f,	0  },
	{ "Knight",		1.0f,	1.0f,	1.0f,	1.0f,	1.0f,	0  },
	{ "Champion",	1.5f,	1.5f,	0.8f,	1.2f,	0.7f,	4  },
	{ "Nightmare",	2.5f,	2.0f,	0.5f,	1.5f,	0.5f,	8  },
};

// ============================================================================
// DarkAgesGame class
// Main game management class for the Dark Ages total conversion
// ============================================================================

class DarkAgesGame {
public:
	static DarkAgesGame &	Instance();
	
	void					Init();
	void					Shutdown();
	
	// Campaign management
	void					StartNewGame( darkAgesDifficulty_t difficulty );
	void					LoadLevel( int levelIndex );
	void					NextLevel();
	bool					IsLevelComplete() const;
	int						GetCurrentLevel() const { return currentLevel; }
	int						GetCurrentAct() const;
	const darkAgesLevelInfo_t * GetLevelInfo( int index ) const;
	
	// Difficulty
	void					SetDifficulty( darkAgesDifficulty_t diff ) { difficulty = diff; }
	darkAgesDifficulty_t	GetDifficulty() const { return difficulty; }
	const darkAgesDifficultySettings_t * GetDifficultySettings() const;
	
	// Combat system
	void					UpdateCombat( float deltaTime );
	float					CalculateMeleeDamage( float baseDamage, int comboCount ) const;
	bool					CheckParry( float attackTime ) const;
	bool					CheckBlock( float attackAngle ) const;
	float					GetStaminaCost( darkAgesPlayerState_t action ) const;
	
	// Stats tracking
	void					RecordKill();
	void					RecordDeath();
	void					RecordSecret();
	void					RecordParry();
	void					RecordBackstab();
	const darkAgesPlayerStats_t & GetStats() const { return stats; }
	
	// Save/Load
	void					SaveGame( const char * saveName );
	void					LoadGame( const char * saveName );
	
	// Performance
	void					OptimizeForHardware();
	void					ApplyT460sOptimizations();
	
private:
	DarkAgesGame();
	~DarkAgesGame();
	
	int						currentLevel;
	darkAgesDifficulty_t	difficulty;
	darkAgesPlayerStats_t	stats;
	darkAgesCombatState_t	combatState;
	bool					isInitialized;
};

#endif /* !__DARKAGES_GAME_H__ */
