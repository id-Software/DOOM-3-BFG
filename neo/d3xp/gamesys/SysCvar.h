/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef __SYS_CVAR_H__
#define __SYS_CVAR_H__

extern idCVar	developer;

extern idCVar	g_cinematic;
extern idCVar	g_cinematicMaxSkipTime;

extern idCVar	g_monsters;
extern idCVar	g_decals;
extern idCVar	g_knockback;
extern idCVar	g_skill;
extern idCVar	g_gravity;
extern idCVar	g_skipFX;
extern idCVar	g_bloodEffects;
extern idCVar	g_projectileLights;
extern idCVar	g_muzzleFlash;

extern idCVar	g_disasm;
extern idCVar	g_debugBounds;
extern idCVar	g_debugAnim;
extern idCVar	g_debugMove;
extern idCVar	g_debugDamage;
extern idCVar	g_debugWeapon;
extern idCVar	g_debugScript;
extern idCVar	g_debugMover;
extern idCVar	g_debugTriggers;
extern idCVar	g_debugCinematic;
extern idCVar	g_stopTime;
extern idCVar	g_armorProtection;
extern idCVar	g_armorProtectionMP;
extern idCVar	g_damageScale;
extern idCVar	g_useDynamicProtection;
extern idCVar	g_healthTakeTime;
extern idCVar	g_healthTakeAmt;
extern idCVar	g_healthTakeLimit;

extern idCVar	g_showPVS;
extern idCVar	g_showTargets;
extern idCVar	g_showTriggers;
extern idCVar	g_showCollisionWorld;
extern idCVar	g_showCollisionModels;
extern idCVar	g_showCollisionTraces;
extern idCVar	g_maxShowDistance;
extern idCVar	g_showEntityInfo;
extern idCVar	g_showviewpos;
extern idCVar	g_showcamerainfo;
extern idCVar	g_showTestModelFrame;
extern idCVar	g_showActiveEntities;
extern idCVar	g_showEnemies;

extern idCVar	g_frametime;
extern idCVar	g_timeentities;

extern idCVar	ai_debugScript;
extern idCVar	ai_debugMove;
extern idCVar	ai_debugTrajectory;
extern idCVar	ai_testPredictPath;
extern idCVar	ai_showCombatNodes;
extern idCVar	ai_showPaths;
extern idCVar	ai_showObstacleAvoidance;
extern idCVar	ai_blockedFailSafe;
extern idCVar	ai_showHealth;

extern idCVar	g_dvTime;
extern idCVar	g_dvAmplitude;
extern idCVar	g_dvFrequency;

extern idCVar	g_kickTime;
extern idCVar	g_kickAmplitude;
extern idCVar	g_blobTime;
extern idCVar	g_blobSize;

extern idCVar	g_testHealthVision;
extern idCVar	g_editEntityMode;
extern idCVar	g_dragEntity;
extern idCVar	g_dragDamping;
extern idCVar	g_dragShowSelection;
extern idCVar	g_dropItemRotation;

extern idCVar	g_vehicleVelocity;
extern idCVar	g_vehicleForce;
extern idCVar	g_vehicleSuspensionUp;
extern idCVar	g_vehicleSuspensionDown;
extern idCVar	g_vehicleSuspensionKCompress;
extern idCVar	g_vehicleSuspensionDamping;
extern idCVar	g_vehicleTireFriction;
extern idCVar	g_vehicleDebug;
extern idCVar	g_debugShockwave;
extern idCVar	g_enablePortalSky;

extern idCVar	ik_enable;
extern idCVar	ik_debug;

extern idCVar	af_useLinearTime;
extern idCVar	af_useImpulseFriction;
extern idCVar	af_useJointImpulseFriction;
extern idCVar	af_useSymmetry;
extern idCVar	af_skipSelfCollision;
extern idCVar	af_skipLimits;
extern idCVar	af_skipFriction;
extern idCVar	af_forceFriction;
extern idCVar	af_maxLinearVelocity;
extern idCVar	af_maxAngularVelocity;
extern idCVar	af_timeScale;
extern idCVar	af_jointFrictionScale;
extern idCVar	af_contactFrictionScale;
extern idCVar	af_highlightBody;
extern idCVar	af_highlightConstraint;
extern idCVar	af_showTimings;
extern idCVar	af_showConstraints;
extern idCVar	af_showConstraintNames;
extern idCVar	af_showConstrainedBodies;
extern idCVar	af_showPrimaryOnly;
extern idCVar	af_showTrees;
extern idCVar	af_showLimits;
extern idCVar	af_showBodies;
extern idCVar	af_showBodyNames;
extern idCVar	af_showMass;
extern idCVar	af_showTotalMass;
extern idCVar	af_showInertia;
extern idCVar	af_showVelocity;
extern idCVar	af_showActive;
extern idCVar	af_testSolid;

extern idCVar	rb_showTimings;
extern idCVar	rb_showBodies;
extern idCVar	rb_showMass;
extern idCVar	rb_showInertia;
extern idCVar	rb_showVelocity;
extern idCVar	rb_showActive;

extern idCVar	pm_jumpheight;
extern idCVar	pm_stepsize;
extern idCVar	pm_crouchspeed;
extern idCVar	pm_walkspeed;
extern idCVar	pm_runspeed;
extern idCVar	pm_noclipspeed;
extern idCVar	pm_spectatespeed;
extern idCVar	pm_spectatebbox;
extern idCVar	pm_usecylinder;
extern idCVar	pm_minviewpitch;
extern idCVar	pm_maxviewpitch;
extern idCVar	pm_stamina;
extern idCVar	pm_staminathreshold;
extern idCVar	pm_staminarate;
extern idCVar	pm_crouchheight;
extern idCVar	pm_crouchviewheight;
extern idCVar	pm_normalheight;
extern idCVar	pm_normalviewheight;
extern idCVar	pm_deadheight;
extern idCVar	pm_deadviewheight;
extern idCVar	pm_crouchrate;
extern idCVar	pm_bboxwidth;
extern idCVar	pm_crouchbob;
extern idCVar	pm_walkbob;
extern idCVar	pm_runbob;
extern idCVar	pm_runpitch;
extern idCVar	pm_runroll;
extern idCVar	pm_bobup;
extern idCVar	pm_bobpitch;
extern idCVar	pm_bobroll;
extern idCVar	pm_thirdPersonRange;
extern idCVar	pm_thirdPersonHeight;
extern idCVar	pm_thirdPersonAngle;
extern idCVar	pm_thirdPersonClip;
extern idCVar	pm_thirdPerson;
extern idCVar	pm_thirdPersonDeath;
extern idCVar	pm_modelView;
extern idCVar	pm_airMsec;

extern idCVar	g_showPlayerShadow;
extern idCVar	g_showHud;
extern idCVar	g_showProjectilePct;
extern idCVar	g_showBrass;
extern idCVar	g_gun_x;
extern idCVar	g_gun_y;
extern idCVar	g_gun_z;
extern idCVar	g_gunScale;
extern idCVar	g_viewNodalX;
extern idCVar	g_viewNodalZ;
extern idCVar	g_fov;
extern idCVar	g_testDeath;
extern idCVar	g_skipViewEffects;
extern idCVar   g_mpWeaponAngleScale;

extern idCVar	g_testParticle;
extern idCVar	g_testParticleName;

extern idCVar	g_testPostProcess;

extern idCVar	g_testModelRotate;
extern idCVar	g_testModelAnimate;
extern idCVar	g_testModelBlend;
extern idCVar	g_flushSave;

extern idCVar	g_enableSlowmo;
extern idCVar	g_slowmoStepRate;
extern idCVar	g_testFullscreenFX;
extern idCVar	g_testHelltimeFX;
extern idCVar	g_testMultiplayerFX;
extern idCVar	g_moveableDamageScale;
extern idCVar	g_testBloomIntensity;
extern idCVar	g_testBloomNumPasses;

extern idCVar	g_grabberHoldSeconds;
extern idCVar	g_grabberEnableShake;
extern idCVar	g_grabberRandomMotion;
extern idCVar	g_grabberHardStop;
extern idCVar	g_grabberDamping;

extern idCVar	g_xp_bind_run_once;

extern idCVar	aas_test;
extern idCVar	aas_showAreas;
extern idCVar	aas_showPath;
extern idCVar	aas_showFlyPath;
extern idCVar	aas_showWallEdges;
extern idCVar	aas_showHideArea;
extern idCVar	aas_pullPlayer;
extern idCVar	aas_randomPullPlayer;
extern idCVar	aas_goalArea;
extern idCVar	aas_showPushIntoArea;

extern idCVar	net_clientPredictGUI;

extern idCVar	si_timeLimit;
extern idCVar	si_fragLimit;
extern idCVar	si_spectators;

extern idCVar si_flagDropTimeLimit;
extern idCVar si_midnight;

extern idCVar g_flagAttachJoint;
extern idCVar g_flagAttachOffsetX;
extern idCVar g_flagAttachOffsetY;
extern idCVar g_flagAttachOffsetZ;
extern idCVar g_flagAttachAngleX;
extern idCVar g_flagAttachAngleY;
extern idCVar g_flagAttachAngleZ;

extern idCVar g_CTFArrows;

extern idCVar	net_clientSelfSmoothing;

#endif /* !__SYS_CVAR_H__ */
