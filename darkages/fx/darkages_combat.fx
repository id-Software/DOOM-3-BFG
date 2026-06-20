// ============================================================================
// DOOM: THE DARK AGES - Combat Effects
// ============================================================================

fx fx/combat/sword_fury_finisher {
	{
		delay				0
		duration			1.0
		restart				0
		particle			"glory_kill_burst"
		offset				0, 0, 32
		useLight			1
		light				"lights/swordFlash"
		lightColor			1.0, 0.7, 0.2
		lightRadius			200
	}
	{
		delay				0
		duration			0.5
		restart				0
		shakeTime			0.5
		shakeAmplitude		4
		shakeDistance		128
		shakeImpulse		0
	}
}

fx fx/combat/mace_crusher_finisher {
	{
		delay				0
		duration			1.2
		restart				0
		particle			"glory_kill_burst"
		offset				0, 0, 16
		useLight			1
		light				"lights/maceImpact"
		lightColor			1.0, 0.8, 0.3
		lightRadius			256
	}
	{
		delay				0
		duration			0.8
		restart				0
		shakeTime			0.8
		shakeAmplitude		8
		shakeDistance		200
		shakeImpulse		0
	}
}

fx fx/combat/axe_cleave_finisher {
	{
		delay				0
		duration			0.8
		restart				0
		particle			"blood_spray"
		offset				0, 0, 48
		useLight			1
		light				"lights/axeFlash"
		lightColor			1.0, 0.5, 0.1
		lightRadius			180
	}
	{
		delay				0
		duration			0.4
		restart				0
		shakeTime			0.4
		shakeAmplitude		3
		shakeDistance		96
	}
}

fx fx/combat/flail_chaos_finisher {
	{
		delay				0
		duration			1.0
		restart				0
		particle			"sword_sparks"
		offset				0, 0, 48
		particle			"blood_spray"
		offset				0, 0, 32
	}
	{
		delay				0
		duration			0.6
		restart				0
		shakeTime			0.6
		shakeAmplitude		5
		shakeDistance		128
	}
}

fx fx/combat/hammer_earthquake {
	{
		delay				0
		duration			2.0
		restart				0
		particle			"glory_kill_burst"
		offset				0, 0, 0
		useLight			1
		light				"lights/hammerShock"
		lightColor			1.0, 0.6, 0.1
		lightRadius			400
	}
	{
		delay				0
		duration			1.5
		restart				0
		shakeTime			1.5
		shakeAmplitude		12
		shakeDistance		400
		shakeImpulse		0
	}
}

fx fx/combat/parry_success {
	{
		delay				0
		duration			0.5
		restart				0
		particle			"parry_flash"
		offset				0, 0, 48
		useLight			1
		light				"lights/parryFlash"
		lightColor			1.0, 1.0, 0.5
		lightRadius			150
	}
}

fx fx/combat/glory_kill {
	{
		delay				0
		duration			1.5
		restart				0
		particle			"glory_kill_burst"
		offset				0, 0, 48
		useLight			1
		light				"lights/gloryKill"
		lightColor			1.0, 0.2, 0.0
		lightRadius			300
	}
	{
		delay				0.1
		duration			0.3
		restart				0
		shakeTime			0.3
		shakeAmplitude		6
		shakeDistance		200
	}
}

fx fx/combat/shield_block {
	{
		delay				0
		duration			0.3
		restart				0
		particle			"shield_block_spark"
		offset				24, 0, 32
		useLight			1
		light				"lights/shieldBlock"
		lightColor			0.5, 0.6, 1.0
		lightRadius			100
	}
}

fx fx/environment/fire_torch {
	{
		delay				0
		duration			0
		restart				0
		particle			"hellfire_projectile"
		offset				0, 0, 0
		useLight			1
		light				"lights/torch"
		lightColor			1.0, 0.6, 0.2
		lightRadius			200
	}
}

fx fx/boss/darklord_entrance {
	{
		delay				0
		duration			3.0
		restart				0
		useLight			1
		light				"lights/bossEntrance"
		lightColor			1.0, 0.0, 0.0
		lightRadius			800
	}
	{
		delay				0
		duration			2.0
		restart				0
		shakeTime			2.0
		shakeAmplitude		10
		shakeDistance		1000
	}
}

fx fx/boss/dragonking_roar {
	{
		delay				0
		duration			2.5
		restart				0
		useLight			1
		light				"lights/dragonRoar"
		lightColor			1.0, 0.5, 0.0
		lightRadius			600
	}
	{
		delay				0
		duration			1.5
		restart				0
		shakeTime			1.5
		shakeAmplitude		8
		shakeDistance		800
	}
}
