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

#ifndef __COMMON_DIALOG_H__
#define __COMMON_DIALOG_H__

static const int	MAX_DIALOGS			= 4;		// maximum dialogs that can be open at one time
static const int	PC_KEYBOARD_WAIT	= 20000;

/*
================================================
Dialog box message types
================================================
*/
enum gameDialogMessages_t
{
	GDM_INVALID,
	GDM_SWAP_DISKS_TO1,
	GDM_SWAP_DISKS_TO2,
	GDM_SWAP_DISKS_TO3,
	GDM_NO_GAMER_PROFILE,
	GDM_PLAY_ONLINE_NO_PROFILE,
	GDM_LEADERBOARD_ONLINE_NO_PROFILE,
	GDM_NO_STORAGE_SELECTED,
	GDM_ONLINE_INCORRECT_PERMISSIONS,
	GDM_SP_QUIT_SAVE,
	GDM_SP_RESTART_SAVE,
	GDM_SP_SIGNIN_CHANGE,
	GDM_SERVER_NOT_AVAILABLE,
	GDM_CONNECTION_LOST_HOST,
	GDM_CONNECTION_LOST,
	GDM_OPPONENT_CONNECTION_LOST,
	GDM_HOST_CONNECTION_LOST,
	GDM_HOST_CONNECTION_LOST_STATS,
	GDM_FAILED_TO_LOAD_RANKINGS,
	GDM_HOST_QUIT,
	GDM_BECAME_HOST_PARTY,			// Became host of party
	GDM_NEW_HOST_PARTY,				// Someone else became host of party
	GDM_LOBBY_BECAME_HOST_GAME,		// In lobby, you became game host
	GDM_LOBBY_NEW_HOST_GAME,		// In lobby, new game host was chosen (not you)
	GDM_NEW_HOST_GAME,				// Host left/DC'd, someone else is new host, unranked game
	GDM_NEW_HOST_GAME_STATS_DROPPED,// Host left/DC'd, someone else is new host, ranked game so stats were dropped
	GDM_BECAME_HOST_GAME,				// Host left/DC'd, you became host, unranked game
	GDM_BECAME_HOST_GAME_STATS_DROPPED, // Host left/DC'd, you became host, ranked game so stats were dropped
	GDM_LOBBY_DISBANDED,
	GDM_LEAVE_WITH_PARTY,
	GDM_LEAVE_LOBBY_RET_MAIN,
	GDM_LEAVE_LOBBY_RET_NEW_PARTY,
	GDM_MIGRATING,
	GDM_OPPONENT_LEFT,
	GDM_NO_MATCHES_FOUND,
	GDM_INVALID_INVITE,
	GDM_KICKED,
	GDM_BANNED,
	GDM_SAVING,
	GDM_OVERWRITE_SAVE,
	GDM_LOAD_REQUEST,
	GDM_AUTOSAVE_DISABLED_STORAGE_REMOVED,
	GDM_STORAGE_INVALID,
	GDM_STORAGE_REMOVED,
	GDM_CONNECTING,
	GDM_REFRESHING,
	GDM_DELETE_SAVE,
	GDM_DELETING,
	GDM_BINDING_ALREDY_SET,
	GDM_CANNOT_BIND,
	GDM_OVERLAY_DISABLED,
	GDM_DIRECT_MAP_CHANGE,
	GDM_DELETE_AUTOSAVE,
	GDM_QUICK_SAVE,
	GDM_MULTI_RETRY,
	GDM_MULTI_SELF_DESTRUCT,
	GDM_MULTI_VDM_QUIT,
	GDM_MULTI_COOP_QUIT,
	GDM_LOADING_PROFILE,
	GDM_STORAGE_REQUIRED,
	GDM_INSUFFICENT_STORAGE_SPACE,
	GDM_PARTNER_LEFT,
	GDM_RESTORE_CORRUPT_SAVEGAME,
	GDM_UNRECOVERABLE_SAVEGAME,
	GDM_PROFILE_SAVE_ERROR,
	GDM_LOBBY_FULL,
	GDM_QUIT_GAME,
	GDM_CONNECTION_PROBLEMS,
	GDM_VOICE_RESTRICTED,
	GDM_LOAD_DAMAGED_FILE,
	GDM_MUST_SIGNIN,
	GDM_CONNECTION_LOST_NO_LEADERBOARD,
	GDM_SP_SIGNIN_CHANGE_POST,
	GDM_MIGRATING_WAITING,
	GDM_MIGRATING_RELAUNCHING,
	GDM_MIGRATING_FAILED_CONNECTION,
	GDM_MIGRATING_FAILED_CONNECTION_STATS,
	GDM_MIGRATING_FAILED_DISBANDED,
	GDM_MIGRATING_FAILED_DISBANDED_STATS,
	GDM_MIGRATING_FAILED_PARTNER_LEFT,
	GDM_HOST_RETURNED_TO_LOBBY,
	GDM_HOST_RETURNED_TO_LOBBY_STATS_DROPPED,
	GDM_FAILED_JOIN_LOCAL_SESSION,
	GDM_DELETE_CORRUPT_SAVEGAME,
	GDM_LEAVE_INCOMPLETE_INSTANCE,
	GDM_UNBIND_CONFIRM,
	GDM_BINDINGS_RESTORE,
	GDM_NEW_HOST,
	GDM_CONFIRM_VIDEO_CHANGES,
	GDM_UNABLE_TO_USE_SELECTED_STORAGE_DEVICE,
	GDM_ERROR_LOADING_SAVEGAME,
	GDM_ERROR_SAVING_SAVEGAME,
	GDM_DISCARD_CHANGES,
	GDM_LEAVE_LOBBY,
	GDM_LEAVE_LOBBY_AND_TEAM,
	GDM_CONTROLLER_DISCONNECTED_0,
	GDM_CONTROLLER_DISCONNECTED_1,
	GDM_CONTROLLER_DISCONNECTED_2,
	GDM_CONTROLLER_DISCONNECTED_3,
	GDM_CONTROLLER_DISCONNECTED_4,
	GDM_CONTROLLER_DISCONNECTED_5,
	GDM_CONTROLLER_DISCONNECTED_6,
	GDM_DLC_ERROR_REMOVED,
	GDM_DLC_ERROR_CORRUPT,
	GDM_DLC_ERROR_MISSING,
	GDM_DLC_ERROR_MISSING_GENERIC,
	GDM_DISC_SWAP,
	GDM_NEEDS_INSTALL,
	GDM_NO_SAVEGAMES_AVAILABLE,
	GDM_ERROR_JOIN_TWO_PROFILES_ONE_BOX,
	GDM_WARNING_PLAYING_COOP_SOLO,
	GDM_MULTI_COOP_QUIT_LOSE_LEADERBOARDS,
	GDM_CORRUPT_CONTINUE,
	GDM_MULTI_VDM_QUIT_LOSE_LEADERBOARDS,
	GDM_WARNING_PLAYING_VDM_SOLO,
	GDM_NO_GUEST_SUPPORT,
	GDM_DISC_SWAP_CONFIRMATION,
	GDM_ERROR_LOADING_PROFILE,
	GDM_CANNOT_INVITE_LOBBY_FULL,
	GDM_WARNING_FOR_NEW_DEVICE_ABOUT_TO_LOSE_PROGRESS,
	GDM_DISCONNECTED,
	GDM_INCOMPATIBLE_NEWER_SAVE,
	GDM_ACHIEVEMENTS_DISABLED_DUE_TO_CHEATING,
	GDM_INCOMPATIBLE_POINTER_SIZE,
	GDM_TEXTUREDETAIL_RESTARTREQUIRED,
	GDM_TEXTUREDETAIL_INSUFFICIENT_CPU,
	GDM_CHECKPOINT_SAVE,
	GDM_CALCULATING_BENCHMARK,
	GDM_DISPLAY_BENCHMARK,
	GDM_DISPLAY_CHANGE_FAILED,
	GDM_GPU_TRANSCODE_FAILED,
	GDM_OUT_OF_MEMORY,
	GDM_CORRUPT_PROFILE,
	GDM_PROFILE_TOO_OUT_OF_DATE_DEVELOPMENT_ONLY,
	GDM_SP_LOAD_SAVE,
	GDM_INSTALLING_TROPHIES,
	GDM_XBOX_DEPLOYMENT_TYPE_FAIL,
	GDM_SAVEGAME_WRONG_LANGUAGE,
	GDM_GAME_RESTART_REQUIRED,
	GDM_MAX
};

/*
================================================
Dialog box types
================================================
*/
enum dialogType_t
{
	DIALOG_INVALID = -1,
	DIALOG_ACCEPT,
	DIALOG_CONTINUE,
	DIALOG_ACCEPT_CANCEL,
	DIALOG_YES_NO,
	DIALOG_CANCEL,
	DIALOG_WAIT,
	DIALOG_WAIT_BLACKOUT,
	DIALOG_WAIT_CANCEL,
	DIALOG_DYNAMIC,
	DIALOG_QUICK_SAVE,
	DIALOG_TIMER_ACCEPT_REVERT,
	DIALOG_CRAWL_SAVE,
	DIALOG_CONTINUE_LARGE,
	DIALOG_BENCHMARK,
};

/*
================================================
idDialogInfo
================================================
*/
class idDialogInfo
{
public:
	idDialogInfo()
	{
		msg					= GDM_INVALID;
		type				= DIALOG_ACCEPT;
		acceptCB			= NULL;
		cancelCB			= NULL;
		altCBOne			= NULL;
		altCBTwo			= NULL;
		showing				= false;
		clear				= false;
		waitClear			= false;
		pause				= false;
		startTime			= 0;
		killTime			= 0;
		leaveOnClear		= false;
		renderDuringLoad	= false;
	}
	gameDialogMessages_t	msg;
	dialogType_t			type;
	idSWFScriptFunction* 	acceptCB;
	idSWFScriptFunction* 	cancelCB;
	idSWFScriptFunction* 	altCBOne;
	idSWFScriptFunction* 	altCBTwo;
	bool					showing;
	bool					clear;
	bool					waitClear;
	bool					pause;
	bool					forcePause;
	bool					leaveOnClear;
	bool					renderDuringLoad;
	int						startTime;
	int						killTime;
	idStrStatic< 256 >		overrideMsg;

	idStrId					txt1;
	idStrId					txt2;
	idStrId					txt3;
	idStrId					txt4;
};

/*
================================================
idLoadScreenInfo
================================================
*/
class idLoadScreenInfo
{
public:
	idStr	varName;
	idStr	value;
};

/*
==============================================================

  Common Dialog

==============================================================
*/

class idCommonDialog
{
public:
	void	Init();
	void	Render( bool loading );
	void	Shutdown();
	void	Restart();

	bool	IsDialogPausing()
	{
		return dialogPause;
	}
	void	ClearDialogs( bool forceClear = false );
	bool	HasDialogMsg( gameDialogMessages_t msg, bool* isNowActive );
	void	AddDialog( gameDialogMessages_t msg, dialogType_t type, idSWFScriptFunction* acceptCallback, idSWFScriptFunction* cancelCallback, bool pause, const char* location = NULL, int lineNumber = 0, bool leaveOnMapHeapReset = false, bool waitOnAtlas = false, bool renderDuringLoad = false );
	void	AddDynamicDialog( gameDialogMessages_t msg, const idStaticList< idSWFScriptFunction*, 4 >& callbacks, const idStaticList< idStrId, 4 >& optionText, bool pause, idStrStatic< 256 > overrideMsg, bool leaveOnMapHeapReset = false, bool waitOnAtlas = false, bool renderDuringLoad = false );
	void	AddDialogIntVal( const char* name, int val );
	bool	IsDialogActive();
	void	ClearDialog( gameDialogMessages_t msg, const char* location = NULL, int lineNumber = 0 );
	void	ShowSaveIndicator( bool show );
	bool	HasAnyActiveDialog() const
	{
		return ( messageList.Num() > 0 ) && ( !messageList[0].clear );
	}

	void	ClearAllDialogHack();
	idStr	GetDialogMsg( gameDialogMessages_t msg, idStr& message, idStr& title );
	bool	HandleDialogEvent( const sysEvent_t* sev );

protected:
	void	RemoveWaitDialogs();
	void	ShowDialog( const idDialogInfo& info );
	void	ShowNextDialog();
	void	ActivateDialog( bool activate );
	void	AddDialogInternal( idDialogInfo& info );
	void	ReleaseCallBacks( int index );

private:
	bool	dialogPause;
	idSWF* 	dialog;
	idSWF* 	saveIndicator;
	bool	dialogShowingSaveIndicatorRequested;
	int		dialogShowingSaveIndicatorTimeRemaining;

	idStaticList< idDialogInfo, MAX_DIALOGS > messageList;
	idStaticList< idLoadScreenInfo, 16 > loadScreenInfo;

	int		startSaveTime;		// with stopSaveTime, useful to pass 360 TCR# 047.  Need to keep the dialog on the screen for a minimum amount of time
	int		stopSaveTime;
	bool	dialogInUse;		// this is to prevent an active msg getting lost during a map heap reset
};

#endif
