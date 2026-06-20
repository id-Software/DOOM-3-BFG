/*
===========================================================================

DOOM: The Dark Ages - Player Progression & Upgrade System
Soul Essences earned from kills power permanent upgrades

===========================================================================
*/

#ifndef __DARKAGES_PROGRESSION_H__
#define __DARKAGES_PROGRESSION_H__

#include "DarkAgesGame.h"

// ============================================================================
// Upgrade Categories
// ============================================================================

enum darkAgesUpgradeCategory_t {
	DA_UPGRADE_COMBAT = 0,
	DA_UPGRADE_DEFENSE,
	DA_UPGRADE_MOBILITY,
	DA_UPGRADE_UTILITY,
	DA_UPGRADE_CATEGORY_COUNT
};

enum darkAgesUpgradeId_t {
	// Combat upgrades
	DA_UPG_MELEE_DAMAGE = 0,
	DA_UPG_COMBO_SPEED,
	DA_UPG_COMBO_WINDOW,
	DA_UPG_HEAVY_DAMAGE,
	DA_UPG_BACKSTAB_DAMAGE,
	DA_UPG_GLORY_KILL_HEAL,
	DA_UPG_RANGED_DAMAGE,
	DA_UPG_AMMO_CAPACITY,
	
	// Defense upgrades
	DA_UPG_MAX_HEALTH,
	DA_UPG_MAX_ARMOR,
	DA_UPG_SHIELD_STRENGTH,
	DA_UPG_PARRY_WINDOW,
	DA_UPG_DAMAGE_RESIST,
	DA_UPG_HEALTH_REGEN,
	
	// Mobility upgrades
	DA_UPG_SPRINT_SPEED,
	DA_UPG_DODGE_DISTANCE,
	DA_UPG_STAMINA_MAX,
	DA_UPG_STAMINA_REGEN,
	DA_UPG_JUMP_HEIGHT,
	
	// Utility upgrades
	DA_UPG_ITEM_MAGNET,
	DA_UPG_SECRET_SENSE,
	DA_UPG_SOUL_GAIN,
	DA_UPG_ARMOR_EFFICIENCY,
	
	DA_UPG_COUNT
};

// ============================================================================
// Upgrade Definitions
// ============================================================================

struct darkAgesUpgradeDef_t {
	const char *			name;
	const char *			description;
	darkAgesUpgradeCategory_t category;
	int						maxLevel;
	int						baseCost;
	int						costPerLevel;
	float					baseValue;
	float					valuePerLevel;
};

static const darkAgesUpgradeDef_t DA_UPGRADES[] = {
	// Combat
	{ "Melee Damage",		"Increase melee damage by 10% per level",		DA_UPGRADE_COMBAT,		5, 100, 50,		0.1f, 0.1f },
	{ "Combo Speed",		"Attack 8% faster in combos per level",			DA_UPGRADE_COMBAT,		4, 150, 75,		0.08f, 0.08f },
	{ "Combo Window",		"Extend combo window by 0.1s per level",		DA_UPGRADE_COMBAT,		3, 200, 100,	0.1f, 0.1f },
	{ "Heavy Damage",		"Heavy attacks deal 15% more damage",			DA_UPGRADE_COMBAT,		5, 120, 60,		0.15f, 0.15f },
	{ "Backstab Damage",	"Backstab multiplier +0.5x per level",			DA_UPGRADE_COMBAT,		4, 180, 90,		0.5f, 0.5f },
	{ "Glory Kill Heal",	"Heal 10 more HP per glory kill per level",		DA_UPGRADE_COMBAT,		5, 100, 50,		10.0f, 10.0f },
	{ "Ranged Damage",		"Ranged weapons deal 10% more damage",			DA_UPGRADE_COMBAT,		5, 130, 65,		0.1f, 0.1f },
	{ "Ammo Capacity",		"Increase max ammo by 20% per level",			DA_UPGRADE_COMBAT,		4, 80, 40,		0.2f, 0.2f },
	
	// Defense
	{ "Max Health",			"Increase max health by 25 per level",			DA_UPGRADE_DEFENSE,		5, 100, 50,		25.0f, 25.0f },
	{ "Max Armor",			"Increase max armor by 25 per level",			DA_UPGRADE_DEFENSE,		5, 100, 50,		25.0f, 25.0f },
	{ "Shield Strength",	"Shield blocks 10% more damage per level",		DA_UPGRADE_DEFENSE,		5, 120, 60,		0.1f, 0.1f },
	{ "Parry Window",		"Parry window +0.05s per level",				DA_UPGRADE_DEFENSE,		4, 200, 100,	0.05f, 0.05f },
	{ "Damage Resist",		"Take 5% less damage per level",				DA_UPGRADE_DEFENSE,		5, 150, 75,		0.05f, 0.05f },
	{ "Health Regen",		"Slowly regenerate HP (1/s per level)",			DA_UPGRADE_DEFENSE,		3, 250, 125,	1.0f, 1.0f },
	
	// Mobility
	{ "Sprint Speed",		"Sprint 8% faster per level",					DA_UPGRADE_MOBILITY,	4, 80, 40,		0.08f, 0.08f },
	{ "Dodge Distance",		"Dodge 15% further per level",					DA_UPGRADE_MOBILITY,	3, 120, 60,		0.15f, 0.15f },
	{ "Stamina Max",		"Increase max stamina by 15 per level",			DA_UPGRADE_MOBILITY,	5, 80, 40,		15.0f, 15.0f },
	{ "Stamina Regen",		"Stamina regenerates 15% faster per level",		DA_UPGRADE_MOBILITY,	5, 100, 50,		0.15f, 0.15f },
	{ "Jump Height",		"Jump 10% higher per level",					DA_UPGRADE_MOBILITY,	3, 100, 50,		0.1f, 0.1f },
	
	// Utility
	{ "Item Magnet",		"Pick up items from 50% further away",			DA_UPGRADE_UTILITY,		3, 80, 40,		0.5f, 0.5f },
	{ "Secret Sense",		"Nearby secrets glow (range +50 per level)",	DA_UPGRADE_UTILITY,		3, 150, 75,		50.0f, 50.0f },
	{ "Soul Gain",			"Earn 15% more soul essences per level",		DA_UPGRADE_UTILITY,		5, 100, 50,		0.15f, 0.15f },
	{ "Armor Efficiency",	"Armor absorbs 10% more damage per level",		DA_UPGRADE_UTILITY,		4, 120, 60,		0.1f, 0.1f },
};

// ============================================================================
// DarkAgesProgression class
// ============================================================================

class DarkAgesProgression {
public:
	static DarkAgesProgression &	Instance();
	
	void				Init();
	
	// Soul Essences (currency)
	int					GetSoulEssences() const { return soulEssences; }
	void				AddSoulEssences( int amount );
	bool				SpendSoulEssences( int amount );
	
	// Upgrades
	int					GetUpgradeLevel( darkAgesUpgradeId_t id ) const;
	bool				CanPurchaseUpgrade( darkAgesUpgradeId_t id ) const;
	int					GetUpgradeCost( darkAgesUpgradeId_t id ) const;
	bool				PurchaseUpgrade( darkAgesUpgradeId_t id );
	float				GetUpgradeValue( darkAgesUpgradeId_t id ) const;
	
	// Calculated stats
	float				GetMeleeDamageMult() const;
	float				GetRangedDamageMult() const;
	float				GetMaxHealth() const;
	float				GetMaxArmor() const;
	float				GetMaxStamina() const;
	float				GetSprintSpeedMult() const;
	float				GetParryWindow() const;
	float				GetShieldBlockMult() const;
	float				GetBackstabMult() const;
	float				GetDamageResist() const;
	float				GetStaminaRegenMult() const;
	int					GetGloryKillHeal() const;
	
	// Level completion
	void				OnLevelComplete( int levelNumber, int kills, int secrets, int totalSecrets, float time );
	int					GetTotalStars() const { return totalStars; }
	
	// Save/Load
	void				Save( const char * filename );
	void				Load( const char * filename );
	
private:
	DarkAgesProgression();
	
	int					soulEssences;
	int					upgradeLevels[DA_UPG_COUNT];
	int					totalStars;
	int					levelStars[DA_MAX_LEVELS];
	bool				levelsCompleted[DA_MAX_LEVELS];
};

#endif /* !__DARKAGES_PROGRESSION_H__ */
