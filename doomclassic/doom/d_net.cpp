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

#include "Precompiled.h"
#include "globaldata.h"


#include "m_menu.h"
#include "i_system.h"
#include "i_video.h"
#include "i_net.h"
#include "g_game.h"
#include "doomdef.h"
#include "doomstat.h"

#include "doomlib.h"
#include "Main.h"
#include "d3xp/Game_local.h"


void I_GetEvents( controller_t * );
void D_ProcessEvents (void); 
void G_BuildTiccmd (ticcmd_t *cmd, idUserCmdMgr *, int newTics ); 
void D_DoAdvanceDemo (void);

extern bool globalNetworking;

//
// NETWORKING
//
// ::g->gametic is the tic about to (or currently being) run
// ::g->maketic is the tick that hasn't had control made for it yet
// ::g->nettics[] has the maketics for all ::g->players 
//
// a ::g->gametic cannot be run until ::g->nettics[] > ::g->gametic for all ::g->players
//




#define NET_TIMEOUT	1 * TICRATE





//
//
//
int NetbufferSize (void)
{
	int size = (intptr_t)&(((doomdata_t *)0)->cmds[::g->netbuffer->numtics]);

	return size;
}

//
// Checksum 
//
unsigned NetbufferChecksum (void)
{
	unsigned		c;
	int		i,l;

	c = 0x1234567;

	if ( globalNetworking ) {
		l = (NetbufferSize () - (intptr_t)&(((doomdata_t *)0)->retransmitfrom))/4;
		for (i=0 ; i<l ; i++)
			c += ((unsigned *)&::g->netbuffer->retransmitfrom)[i] * (i+1);
	}

	return c & NCMD_CHECKSUM;
}

//
//
//
int ExpandTics (int low)
{
	int	delta;

	delta = low - (::g->maketic&0xff);

	if (delta >= -64 && delta <= 64)
		return (::g->maketic&~0xff) + low;
	if (delta > 64)
		return (::g->maketic&~0xff) - 256 + low;
	if (delta < -64)
		return (::g->maketic&~0xff) + 256 + low;

	I_Error ("ExpandTics: strange value %i at ::g->maketic %i",low,::g->maketic);
	return 0;
}



//
// HSendPacket
//
void
HSendPacket
(int	node,
 int	flags )
{
	::g->netbuffer->checksum = NetbufferChecksum () | flags;

	if (!node)
	{
		::g->reboundstore = *::g->netbuffer;
		::g->reboundpacket = true;
		return;
	}

	if (::g->demoplayback)
		return;

	if (!::g->netgame)
		I_Error ("Tried to transmit to another node");

	::g->doomcom.command = CMD_SEND;
	::g->doomcom.remotenode = node;
	::g->doomcom.datalength = NetbufferSize ();

	if (::g->debugfile)
	{
		int		i;
		int		realretrans;
		if (::g->netbuffer->checksum & NCMD_RETRANSMIT)
			realretrans = ExpandTics (::g->netbuffer->retransmitfrom);
		else
			realretrans = -1;

		fprintf (::g->debugfile,"send (%i + %i, R %i) [%i] ",
			ExpandTics(::g->netbuffer->starttic),
			::g->netbuffer->numtics, realretrans, ::g->doomcom.datalength);

		for (i=0 ; i < ::g->doomcom.datalength ; i++)
			fprintf (::g->debugfile,"%i ",((byte *)::g->netbuffer)[i]);

		fprintf (::g->debugfile,"\n");
	}

	I_NetCmd ();
}

//
// HGetPacket
// Returns false if no packet is waiting
//
qboolean HGetPacket (void)
{	
	if (::g->reboundpacket)
	{
		*::g->netbuffer = ::g->reboundstore;
		::g->doomcom.remotenode = 0;
		::g->reboundpacket = false;
		return true;
	}

	if (!::g->netgame)
		return false;

	if (::g->demoplayback)
		return false;

	::g->doomcom.command = CMD_GET;
	I_NetCmd ();

	if (::g->doomcom.remotenode == -1)
		return false;

	if (::g->doomcom.datalength != NetbufferSize ())
	{
		if (::g->debugfile)
			fprintf (::g->debugfile,"bad packet length %i\n",::g->doomcom.datalength);
		return false;
	}

	// ALAN NETWORKING -- this fails a lot on 4 player split debug!!
	// TODO: Networking
#ifdef ID_ENABLE_DOOM_CLASSIC_NETWORKING
	if ( !gameLocal->IsSplitscreen() && NetbufferChecksum() != (::g->netbuffer->checksum&NCMD_CHECKSUM) )
	{
		if (::g->debugfile) {
			fprintf (::g->debugfile,"bad packet checksum\n");
		}

		return false;
	}
#endif

	if (::g->debugfile)
	{
		int		realretrans;
		int	i;

		if (::g->netbuffer->checksum & NCMD_SETUP)
			fprintf (::g->debugfile,"setup packet\n");
		else
		{
			if (::g->netbuffer->checksum & NCMD_RETRANSMIT)
				realretrans = ExpandTics (::g->netbuffer->retransmitfrom);
			else
				realretrans = -1;

			fprintf (::g->debugfile,"get %i = (%i + %i, R %i)[%i] ",
				::g->doomcom.remotenode,
				ExpandTics(::g->netbuffer->starttic),
				::g->netbuffer->numtics, realretrans, ::g->doomcom.datalength);

			for (i=0 ; i < ::g->doomcom.datalength ; i++)
				fprintf (::g->debugfile,"%i ",((byte *)::g->netbuffer)[i]);
			fprintf (::g->debugfile,"\n");
		}
	}
	return true;	
}


//
// GetPackets
//

void GetPackets (void)
{
	int		netconsole;
	int		netnode;
	ticcmd_t	*src, *dest;
	int		realend;
	int		realstart;

	while ( HGetPacket() )
	{
		if (::g->netbuffer->checksum & NCMD_SETUP)
			continue;		// extra setup packet

		netconsole = ::g->netbuffer->player & ~PL_DRONE;
		netnode = ::g->doomcom.remotenode;

		// to save bytes, only the low byte of tic numbers are sent
		// Figure out what the rest of the bytes are
		realstart = ExpandTics (::g->netbuffer->starttic);		
		realend = (realstart+::g->netbuffer->numtics);

		// check for exiting the game
		if (::g->netbuffer->checksum & NCMD_EXIT)
		{
			if (!::g->nodeingame[netnode])
				continue;
			::g->nodeingame[netnode] = false;
			::g->playeringame[netconsole] = false;
			strcpy (::g->exitmsg, "Player 1 left the game");
			::g->exitmsg[7] += netconsole;
			::g->players[::g->consoleplayer].message = ::g->exitmsg;

			if( ::g->demorecording ) {
				G_CheckDemoStatus();
			}
			continue;
		}

		// check for a remote game kill
/*
		if (::g->netbuffer->checksum & NCMD_KILL)
			I_Error ("Killed by network driver");
*/

		::g->nodeforplayer[netconsole] = netnode;

		// check for retransmit request
		if ( ::g->resendcount[netnode] <= 0 
			&& (::g->netbuffer->checksum & NCMD_RETRANSMIT) )
		{
			::g->resendto[netnode] = ExpandTics(::g->netbuffer->retransmitfrom);
			if (::g->debugfile)
				fprintf (::g->debugfile,"retransmit from %i\n", ::g->resendto[netnode]);
			::g->resendcount[netnode] = RESENDCOUNT;
		}
		else
			::g->resendcount[netnode]--;

		// check for out of order / duplicated packet		
		if (realend == ::g->nettics[netnode])
			continue;

		if (realend < ::g->nettics[netnode])
		{
			if (::g->debugfile)
				fprintf (::g->debugfile,
				"out of order packet (%i + %i)\n" ,
				realstart,::g->netbuffer->numtics);
			continue;
		}

		// check for a missed packet
		if (realstart > ::g->nettics[netnode])
		{
			// stop processing until the other system resends the missed tics
			if (::g->debugfile)
				fprintf (::g->debugfile,
				"missed tics from %i (%i - %i)\n",
				netnode, realstart, ::g->nettics[netnode]);
			::g->remoteresend[netnode] = true;
			continue;
		}

		// update command store from the packet
		{
			int		start;

			::g->remoteresend[netnode] = false;

			start = ::g->nettics[netnode] - realstart;		
			src = &::g->netbuffer->cmds[start];

			while (::g->nettics[netnode] < realend)
			{
				dest = &::g->netcmds[netconsole][::g->nettics[netnode]%BACKUPTICS];
				::g->nettics[netnode]++;
				*dest = *src;
				src++;
			}
		}
	}
}


//
// NetUpdate
// Builds ticcmds for console player,
// sends out a packet
//

void NetUpdate ( idUserCmdMgr * userCmdMgr )
{
	int             nowtime;
	int             newtics;
	int				i,j;
	int				realstart;
	int				gameticdiv;

	// check time
	nowtime = I_GetTime ()/::g->ticdup;
	newtics = nowtime - ::g->gametime;
	::g->gametime = nowtime;

	if (newtics <= 0) 	// nothing new to update
		goto listen; 

	if (::g->skiptics <= newtics)
	{
		newtics -= ::g->skiptics;
		::g->skiptics = 0;
	}
	else
	{
		::g->skiptics -= newtics;
		newtics = 0;
	}


	::g->netbuffer->player = ::g->consoleplayer;

	// build new ticcmds for console player
	gameticdiv = ::g->gametic/::g->ticdup;
	for (i=0 ; i<newtics ; i++)
	{
		//I_GetEvents( ::g->I_StartTicCallback () );
		D_ProcessEvents ();
		if (::g->maketic - gameticdiv >= BACKUPTICS/2-1) {
			printf( "Out of room for ticcmds: maketic = %d, gameticdiv = %d\n", ::g->maketic, gameticdiv );
			break;          // can't hold any more
		}

		//I_Printf ("mk:%i ",::g->maketic);

		// Grab the latest tech5 command

		G_BuildTiccmd (&::g->localcmds[::g->maketic%BACKUPTICS], userCmdMgr, newtics );
		::g->maketic++;
	}


	if (::g->singletics)
		return;         // singletic update is syncronous

	// send the packet to the other ::g->nodes
	for (i=0 ; i < ::g->doomcom.numnodes ; i++) {

		if (::g->nodeingame[i])	{
			::g->netbuffer->starttic = realstart = ::g->resendto[i];
			::g->netbuffer->numtics = ::g->maketic - realstart;
			if (::g->netbuffer->numtics > BACKUPTICS)
				I_Error ("NetUpdate: ::g->netbuffer->numtics > BACKUPTICS");

			::g->resendto[i] = ::g->maketic - ::g->doomcom.extratics;

			for (j=0 ; j< ::g->netbuffer->numtics ; j++)
				::g->netbuffer->cmds[j] = 
				::g->localcmds[(realstart+j)%BACKUPTICS];

			if (::g->remoteresend[i])
			{
				::g->netbuffer->retransmitfrom = ::g->nettics[i];
				HSendPacket (i, NCMD_RETRANSMIT);
			}
			else
			{
				::g->netbuffer->retransmitfrom = 0;
				HSendPacket (i, 0);
			}
		}
	}

	// listen for other packets
listen:
	GetPackets ();
}



//
// CheckAbort
//
void CheckAbort (void)
{
	// DHM - Time starts at 0 tics when starting a multiplayer game, so we can
	// check for timeouts easily.  If we're still waiting after N seconds, abort.
	if ( I_GetTime() > NET_TIMEOUT ) {
		// TOOD: Show error & leave net game.
		printf( "NET GAME TIMED OUT!\n" );
		//gameLocal->showFatalErrorMessage( XuiLookupStringTable(globalStrings,L"Timed out waiting for match start.") );


		D_QuitNetGame();

		session->QuitMatch();
		common->Dialog().AddDialog( GDM_OPPONENT_CONNECTION_LOST, DIALOG_ACCEPT, NULL, NULL, false );
	}
}


//
// D_ArbitrateNetStart
//
bool D_ArbitrateNetStart (void)
{
	int		i;

	::g->autostart = true;
	if (::g->doomcom.consoleplayer)
	{
		// listen for setup info from key player
		CheckAbort ();
		if (!HGetPacket ())
			return false;
		if (::g->netbuffer->checksum & NCMD_SETUP)
		{
			printf( "Received setup info\n" );

			if (::g->netbuffer->player != VERSION)
				I_Error ("Different DOOM versions cannot play a net game!");
			::g->startskill = (skill_t)(::g->netbuffer->retransmitfrom & 15);
			::g->deathmatch = (::g->netbuffer->retransmitfrom & 0xc0) >> 6;
			::g->nomonsters = (::g->netbuffer->retransmitfrom & 0x20) > 0;
			::g->respawnparm = (::g->netbuffer->retransmitfrom & 0x10) > 0;
			// VV original xbox doom :: don't do this.. it will be setup from the launcher
			//::g->startmap = ::g->netbuffer->starttic & 0x3f;
			//::g->startepisode = ::g->netbuffer->starttic >> 6;
			return true;
		}
		return false;
	}
	else
	{
		// key player, send the setup info
		CheckAbort ();
		for (i=0 ; i < ::g->doomcom.numnodes ; i++)
		{
			printf( "Sending setup info to node %d\n", i );

			::g->netbuffer->retransmitfrom = ::g->startskill;
			if (::g->deathmatch)
				::g->netbuffer->retransmitfrom |= (::g->deathmatch<<6);
			if (::g->nomonsters)
				::g->netbuffer->retransmitfrom |= 0x20;
			if (::g->respawnparm)
				::g->netbuffer->retransmitfrom |= 0x10;
			::g->netbuffer->starttic = ::g->startepisode * 64 + ::g->startmap;
			::g->netbuffer->player = VERSION;
			::g->netbuffer->numtics = 0;
			HSendPacket (i, NCMD_SETUP);
		}

		while (HGetPacket ())
		{
			::g->gotinfo[::g->netbuffer->player&0x7f] = true;
		}

		for (i=1 ; i < ::g->doomcom.numnodes ; i++) {
			if (!::g->gotinfo[i])
				break;
		}

		if (i >= ::g->doomcom.numnodes)
			return true;

		return false;
	}
}

//
// D_CheckNetGame
// Works out player numbers among the net participants
//

void D_CheckNetGame (void)
{
	int             i;

	for (i=0 ; i<MAXNETNODES ; i++)
	{
		::g->nodeingame[i] = false;
		::g->nettics[i] = 0;
		::g->remoteresend[i] = false;	// set when local needs tics
		::g->resendto[i] = 0;		// which tic to start sending
	}

	// I_InitNetwork sets ::g->doomcom and ::g->netgame
	I_InitNetwork ();
#ifdef ID_ENABLE_DOOM_CLASSIC_NETWORKING
	if (::g->doomcom.id != DOOMCOM_ID)
		I_Error ("Doomcom buffer invalid!");
#endif

	::g->netbuffer = &::g->doomcom.data;
	::g->consoleplayer = ::g->displayplayer = ::g->doomcom.consoleplayer;
}

bool D_PollNetworkStart()
{
	int             i;
	if (::g->netgame)
	{
		if (D_ArbitrateNetStart () == false)
			return false;
	}

	I_Printf ("startskill %i  deathmatch: %i  startmap: %i  startepisode: %i\n",
		::g->startskill, ::g->deathmatch, ::g->startmap, ::g->startepisode);

	// read values out of ::g->doomcom
	::g->ticdup = ::g->doomcom.ticdup;
	::g->maxsend = BACKUPTICS/(2*::g->ticdup)-1;
	if (::g->maxsend<1)
		::g->maxsend = 1;

	for (i=0 ; i < ::g->doomcom.numplayers ; i++)
		::g->playeringame[i] = true;
	for (i=0 ; i < ::g->doomcom.numnodes ; i++)
		::g->nodeingame[i] = true;

	I_Printf ("player %i of %i (%i ::g->nodes)\n",
		::g->consoleplayer+1, ::g->doomcom.numplayers, ::g->doomcom.numnodes);

	return true;
}


//
// D_QuitNetGame
// Called before quitting to leave a net game
// without hanging the other ::g->players
//
void D_QuitNetGame (void)
{
	int i;

	if ( (!::g->netgame && !::g->usergame) || ::g->consoleplayer == -1 || ::g->demoplayback || ::g->netbuffer == NULL )
		return;

	// send a quit packet to the other nodes
	::g->netbuffer->player = ::g->consoleplayer;
	::g->netbuffer->numtics = 0;

	for ( i=1; i < ::g->doomcom.numnodes; i++ ) {
		if ( ::g->nodeingame[i] ) {
			HSendPacket( i, NCMD_EXIT );
		}
	}
	DoomLib::SendNetwork();

	for (i=1 ; i<MAXNETNODES ; i++)
	{
		::g->nodeingame[i] = false;
		::g->nettics[i] = 0;
		::g->remoteresend[i] = false;	// set when local needs tics
		::g->resendto[i] = 0;		// which tic to start sending
	}

	//memset (&::g->doomcom, 0, sizeof(::g->doomcom) );

	// Reset singleplayer state
	::g->doomcom.id = DOOMCOM_ID;
	::g->doomcom.ticdup = 1;
	::g->doomcom.extratics = 0;
	::g->doomcom.numplayers = ::g->doomcom.numnodes = 1;
	::g->doomcom.deathmatch = false;
	::g->doomcom.consoleplayer = 0;
	::g->netgame = false;

	::g->netbuffer = &::g->doomcom.data;
	::g->consoleplayer = ::g->displayplayer = ::g->doomcom.consoleplayer;

	::g->ticdup = ::g->doomcom.ticdup;
	::g->maxsend = BACKUPTICS/(2*::g->ticdup)-1;
	if (::g->maxsend<1)
		::g->maxsend = 1;

	for (i=0 ; i < ::g->doomcom.numplayers ; i++)
		::g->playeringame[i] = true;
	for (i=0 ; i < ::g->doomcom.numnodes ; i++)
		::g->nodeingame[i] = true;
}



//
// TryRunTics
//
bool TryRunTics ( idUserCmdMgr * userCmdMgr )
{
	int		i;
	int		lowtic_node = -1;

	// get real tics		
	::g->trt_entertic = I_GetTime ()/::g->ticdup;
	::g->trt_realtics = ::g->trt_entertic - ::g->oldtrt_entertics;
	::g->oldtrt_entertics = ::g->trt_entertic;

	// get available tics
	NetUpdate ( userCmdMgr );

	::g->trt_lowtic = MAXINT;
	::g->trt_numplaying = 0;

	for (i=0 ; i < ::g->doomcom.numnodes ; i++) {

		if (::g->nodeingame[i]) {
			::g->trt_numplaying++;

			if (::g->nettics[i] < ::g->trt_lowtic) {
				::g->trt_lowtic = ::g->nettics[i];
				lowtic_node = i;
			}
		}
	}

	::g->trt_availabletics = ::g->trt_lowtic - ::g->gametic/::g->ticdup;

	// decide how many tics to run
	if (::g->trt_realtics < ::g->trt_availabletics-1) {
		::g->trt_counts = ::g->trt_realtics+1;
	} else if (::g->trt_realtics < ::g->trt_availabletics) {
		::g->trt_counts = ::g->trt_realtics;
	} else {
		::g->trt_counts = ::g->trt_availabletics;
	}

	if (::g->trt_counts < 1) {
		::g->trt_counts = 1;
	}

	::g->frameon++;

	if (::g->debugfile) {
		fprintf (::g->debugfile, "=======real: %i  avail: %i  game: %i\n", ::g->trt_realtics, ::g->trt_availabletics,::g->trt_counts);
	}

	if ( !::g->demoplayback )
	{	
		// ideally ::g->nettics[0] should be 1 - 3 tics above ::g->trt_lowtic
		// if we are consistantly slower, speed up time
		for (i=0 ; i<MAXPLAYERS ; i++) {
			if (::g->playeringame[i]) {
				break;
			}
		}

		if (::g->consoleplayer == i) {
			// the key player does not adapt
		}
		else {
			if (::g->nettics[0] <= ::g->nettics[::g->nodeforplayer[i]])	{
				::g->gametime--;
				//OutputDebugString("-");
			}

			::g->frameskip[::g->frameon&3] = (::g->oldnettics > ::g->nettics[::g->nodeforplayer[i]]);
			::g->oldnettics = ::g->nettics[0];

			if (::g->frameskip[0] && ::g->frameskip[1] && ::g->frameskip[2] && ::g->frameskip[3]) {
				::g->skiptics = 1;
				//OutputDebugString("+");
			}
		}
	}

	// wait for new tics if needed
	if (::g->trt_lowtic < ::g->gametic/::g->ticdup + ::g->trt_counts)	
	{
		int lagtime = 0;

		if (::g->trt_lowtic < ::g->gametic/::g->ticdup) {
			I_Error ("TryRunTics: ::g->trt_lowtic < gametic");
		}

		if ( ::g->lastnettic == 0 ) {
			::g->lastnettic = ::g->trt_entertic;
		}
		lagtime = ::g->trt_entertic - ::g->lastnettic;

		// Detect if a client has stopped sending updates, remove them from the game after 5 secs.
		if ( common->IsMultiplayer() && (!::g->demoplayback && ::g->netgame) && lagtime >= TICRATE ) {

			if ( lagtime > NET_TIMEOUT ) {

				if ( lowtic_node == ::g->nodeforplayer[::g->consoleplayer] ) {

#ifdef ID_ENABLE_DOOM_CLASSIC_NETWORKING
#ifndef __PS3__
					gameLocal->showFatalErrorMessage( XuiLookupStringTable(globalStrings,L"You have been disconnected from the match.") );
					gameLocal->Interface.QuitCurrentGame();
#endif
#endif
				} else {
					if (::g->nodeingame[lowtic_node]) {
						int i, consoleNum = lowtic_node;

						for ( i=0; i < ::g->doomcom.numnodes; i++ ) {
							if ( ::g->nodeforplayer[i] == lowtic_node ) {
								consoleNum = i;
								break;
							}
						}

						::g->nodeingame[lowtic_node] = false;
						::g->playeringame[consoleNum] = false;
						strcpy (::g->exitmsg, "Player 1 left the game");
						::g->exitmsg[7] += consoleNum;
						::g->players[::g->consoleplayer].message = ::g->exitmsg;

						// Stop a demo record now, as playback doesn't support losing players
						G_CheckDemoStatus();
					}
				}
			}
		} 

		return false;
	}

	::g->lastnettic = 0;

	// run the count * ::g->ticdup dics
	while (::g->trt_counts--)
	{
		for (i=0 ; i < ::g->ticdup ; i++)
		{
			if (::g->gametic/::g->ticdup > ::g->trt_lowtic) {
				I_Error ("gametic(%d) greater than trt_lowtic(%d), trt_counts(%d)", ::g->gametic, ::g->trt_lowtic, ::g->trt_counts );
				return false;
			}

			if (::g->advancedemo) {
				D_DoAdvanceDemo ();
			}

			M_Ticker ();
			G_Ticker ();
			::g->gametic++;

			// modify command for duplicated tics
			if (i != ::g->ticdup-1)
			{
				ticcmd_t	*cmd;
				int			buf;
				int			j;

				buf = (::g->gametic/::g->ticdup)%BACKUPTICS; 
				for (j=0 ; j<MAXPLAYERS ; j++)
				{
					cmd = &::g->netcmds[j][buf];
					if (cmd->buttons & BT_SPECIAL)
						cmd->buttons = 0;
				}
			}
		}

		NetUpdate ( userCmdMgr );	// check for new console commands
	}

	return true;
}

