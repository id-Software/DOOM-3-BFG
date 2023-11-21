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

#ifndef __GAME_TRIGGER_H__
#define __GAME_TRIGGER_H__

extern const idEventDef EV_Enable;
extern const idEventDef EV_Disable;

/*
===============================================================================

  Trigger base.

===============================================================================
*/

class idTrigger : public idEntity
{
public:
	CLASS_PROTOTYPE( idTrigger );

	static void			DrawDebugInfo();

	idTrigger();
	void				Spawn();

	const function_t* 	GetScriptFunction() const;

	void				Save( idSaveGame* savefile ) const;
	void				Restore( idRestoreGame* savefile );

	virtual void		Enable();
	virtual void		Disable();

protected:
	void				CallScript() const;

	void				Event_Enable();
	void				Event_Disable();

	const function_t* 	scriptFunction;
};


/*
===============================================================================

  Trigger which can be activated multiple times.

===============================================================================
*/

class idTrigger_Multi : public idTrigger
{
public:
	CLASS_PROTOTYPE( idTrigger_Multi );

	idTrigger_Multi();

	void				Spawn();

	void				Save( idSaveGame* savefile ) const;
	void				Restore( idRestoreGame* savefile );

protected:

	float				wait;
	float				random;
	float				delay;
	float				random_delay;
	int					nextTriggerTime;
	idStr				requires;
	int					removeItem;
	bool				touchClient;
	bool				touchOther;
	bool				triggerFirst;
	bool				triggerWithSelf;

	bool				CheckFacing( idEntity* activator );
	void				TriggerAction( idEntity* activator );
	void				Event_TriggerAction( idEntity* activator );
	void				Event_Trigger( idEntity* activator );
	void				Event_Touch( idEntity* other, trace_t* trace );
};


/*
===============================================================================

  Trigger which can only be activated by an entity with a specific name.

===============================================================================
*/

class idTrigger_EntityName : public idTrigger
{
public:
	CLASS_PROTOTYPE( idTrigger_EntityName );

	idTrigger_EntityName();

	void				Save( idSaveGame* savefile ) const;
	void				Restore( idRestoreGame* savefile );

	void				Spawn();

private:
	float				wait;
	float				random;
	float				delay;
	float				random_delay;
	int					nextTriggerTime;
	bool				triggerFirst;
	idStr				entityName;
	bool				testPartialName;

	void				TriggerAction( idEntity* activator );
	void				Event_TriggerAction( idEntity* activator );
	void				Event_Trigger( idEntity* activator );
	void				Event_Touch( idEntity* other, trace_t* trace );
};

/*
===============================================================================

  Trigger which repeatedly fires targets.

===============================================================================
*/

class idTrigger_Timer : public idTrigger
{
public:
	CLASS_PROTOTYPE( idTrigger_Timer );

	idTrigger_Timer();

	void				Save( idSaveGame* savefile ) const;
	void				Restore( idRestoreGame* savefile );

	void				Spawn();

	virtual void		Enable();
	virtual void		Disable();

private:
	float				random;
	float				wait;
	bool				on;
	float				delay;
	idStr				onName;
	idStr				offName;

	void				Event_Timer();
	void				Event_Use( idEntity* activator );
};


/*
===============================================================================

  Trigger which fires targets after being activated a specific number of times.

===============================================================================
*/

class idTrigger_Count : public idTrigger
{
public:
	CLASS_PROTOTYPE( idTrigger_Count );

	idTrigger_Count();

	void				Save( idSaveGame* savefile ) const;
	void				Restore( idRestoreGame* savefile );

	void				Spawn();

private:
	int					goal;
	int					count;
	float				delay;

	void				Event_Trigger( idEntity* activator );
	void				Event_TriggerAction( idEntity* activator );
};


/*
===============================================================================

  Trigger which hurts touching entities.

===============================================================================
*/

class idTrigger_Hurt : public idTrigger
{
public:
	CLASS_PROTOTYPE( idTrigger_Hurt );

	idTrigger_Hurt();

	void				Save( idSaveGame* savefile ) const;
	void				Restore( idRestoreGame* savefile );

	void				Spawn();

private:
	bool				on;
	float				delay;
	int					nextTime;

	void				Event_Touch( idEntity* other, trace_t* trace );
	void				Event_Toggle( idEntity* activator );
};


/*
===============================================================================

  Trigger which fades the player view.

===============================================================================
*/

class idTrigger_Fade : public idTrigger
{
public:

	CLASS_PROTOTYPE( idTrigger_Fade );

private:
	void				Event_Trigger( idEntity* activator );
};


/*
===============================================================================

  Trigger which continuously tests whether other entities are touching it.

===============================================================================
*/

class idTrigger_Touch : public idTrigger
{
public:
	~idTrigger_Touch();

	CLASS_PROTOTYPE( idTrigger_Touch );

	idTrigger_Touch();

	void				Spawn();
	virtual void		Think();

	void				Save( idSaveGame* savefile );
	void				Restore( idRestoreGame* savefile );

	virtual void		Enable();
	virtual void		Disable();

	void				TouchEntities();

private:
	idClipModel* 		clipModel;

	void				Event_Trigger( idEntity* activator );
};

/*
===============================================================================

  Trigger that responces to CTF flags

===============================================================================
*/
class idTrigger_Flag : public idTrigger_Multi
{
public:
	CLASS_PROTOTYPE( idTrigger_Flag );

	idTrigger_Flag();
	void				Spawn();

private:
	int					team;
	bool				player;			// flag must be attached/carried by player

	const idEventDef* 	eventFlag;

	void				Event_Touch( idEntity* other, trace_t* trace );
};

#endif /* !__GAME_TRIGGER_H__ */
