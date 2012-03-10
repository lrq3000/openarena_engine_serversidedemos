/*
===========================================================================
Copyright (C) 2008 Amanieu d'Antras

This file is part of Tremulous.

Tremulous is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Tremulous is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Tremulous; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

// sv_demo.c -- Server side demo recording

#include "server.h"

/***********************************************
 * VARIABLES
 *
 ***********************************************/

// Headers/markers for demo messages (events)
typedef enum {
	demo_endFrame, // player and gentity state marker (recorded each end of frame, hence the name) - at the same time marks the end of the demo frame
	demo_configString, // config string setting event
	demo_clientConfigString, // client config string setting event
	demo_clientCommand, // client command event
	demo_serverCommand, // server command event
	demo_gameCommand, // game command event
	demo_clientUserinfo, // client userinfo event (client_t management)
	demo_entityState, // gentity game state event
	demo_entityShared, // more gentity game state event
	demo_playerState, // players game state event (player_t management)
	demo_endDemo, // end of demo event (close the demo)
	demo_EOF // end of file/flux event (end of event, separator, notify the demo parser to iterate to the next event of the _same_ frame)
} demo_ops_e;

// Big fat buffer to store all our stuff
static byte buf[0x400000];

// Save maxclients and democlients and restore them after the demo
static int savedMaxClients = -1;
static int savedDemoClients = -1;
static int savedBotMinPlayers = -1;
static int savedFPS = -1;
static int keepSaved = 0; // var that memorizes if we keep the new maxclients and democlients values (in the case that we restart the map/server for these cvars to be affected since they are latched) or if we can restore them



/***********************************************
 * CHECKING AND FILTERING FUNCTIONS
 *
 * Functions used to either trim unnecessary or privacy concerned data, or to just check if the data should be written in the demo, relayed to a more specialized function or just dropped.
 ***********************************************/

/*
====================
SV_CheckClientCommand

Check that the clientCommand is OK (if not we either continue with a more specialized function, or drop it altogether if there's a privacy concern)
====================
*/
qboolean SV_CheckClientCommand( client_t *client, const char *cmd )
{
	char *userinfo = ""; // init with an empty string so that the compiler doesn't shout errors

	if ( !strncmp(cmd, "userinfo", 8) ) { // If that's a userinfo command, we directly handle that with a specialized function
		strncpy(userinfo, cmd+9, strlen(cmd)-9); // trimming out the "userinfo " substring (because we only need the userinfo string)
		SV_DemoWriteClientUserinfo(client, (const char*)userinfo); // passing relay to the specialized function for this job

		return qfalse; // we return false if the check wasn't right (meaning that this function does not need to process anything)
	} else if( !strncmp(cmd, "tell", 4) || !strncmp(cmd, "say_team", 8) ) { // Privacy check: if the command is tell or say_team, we just drop it
		return qfalse;
	}
	return qtrue; // else, the check is OK and we continue to process with the original function
}

/*
====================
SV_CheckServerCommand

Check that the serverCommand is OK (or if we drop it because already handled somewhere else)
====================
*/
qboolean SV_CheckServerCommand( const char *cmd )
{
	if ( !strncmp(cmd, "print", 5) || !strncmp(cmd, "cp", 2) ) { // If that's a print/cp command, it's already handled by gameCommand (here it's a redundancy). FIXME? possibly some print/cp are not handled by gameCommand? From my tests it was ok, but if someday a few prints are missing, try to delete this check...
		return qfalse; // we return false if the check wasn't right
	}
	return qtrue; // else, the check is OK and we continue to process with the original function
}

/*
====================
SV_CheckGameCommand

Check that the gameCommand is OK (or if we drop it for privacy concerns and/or because it's handled somewhere else already)
====================
*/
qboolean SV_CheckGameCommand( const char *cmd )
{
	if ( !strncmp(cmd, "chat", 4) || !strncmp(cmd, "tchat", 5) ) { // we filter out the chat and tchat commands which are recorded and handled directly by clientCommand (which is easier to manage because it makes a difference between say, say_team and tell, which we don't have here in gamecommands: we either have chat(for say and tell) or tchat (for say_team) and the other difference is that chat/tchat messages directly contain the name of the player, while clientCommand only contains the clientid, so that it itself fetch the name from client->name
		return qfalse; // we return false if the check wasn't right
	}
	return qtrue; // else, the check is OK and we continue to process with the original function
}

/*
====================
SV_CheckConfigString

Check that the configstring is OK (or if we relay to ClientConfigString)
====================
*/
qboolean SV_CheckConfigString( int cs_index, const char *cs_string )
{
	if ( cs_index >= CS_PLAYERS && cs_index < CS_PLAYERS + sv_maxclients->integer ) { // if this is a player, we save the configstring as a clientconfigstring
		SV_DemoWriteClientConfigString( cs_index - CS_PLAYERS, cs_string );
		return qfalse; // we return false if the check wasn't right
	} else {
		SV_DemoWriteConfigString( cs_index, cs_string ); // else we save it as a normal configstring (for capture scores CS_SCORES1/2, for CS_FLAGSTATUS, etc..)
	}
	return qtrue; // else, the check is OK and we continue to process with the original function
}

/*
====================
SV_DemoFilterClientUserinfo

Filters privacy keys from a client userinfo to later write in a demo file
====================
*/
void SV_DemoFilterClientUserinfo( const char *userinfo )
{
	char *buf;

	buf = (char *)userinfo;
	Info_RemoveKey(buf, "cl_guid");
	Info_RemoveKey(buf, "ip");
	Info_SetValueForKey(buf, "cl_voip", "0");

	userinfo = (const char*)buf;
}



/***********************************************
 * DEMO WRITING FUNCTIONS
 *
 * Functions used to construct and write demo events
 ***********************************************/

/*
====================
SV_DemoWriteMessage

Write a message/event to the demo file
====================
*/
static void SV_DemoWriteMessage(msg_t *msg)
{
	int len;

	// Write the entire message to the file, prefixed by the length
	MSG_WriteByte(msg, demo_EOF); // append EOF (end-of-file or rather end-of-flux) to the message so that it will tell the demo parser when the demo will be read that the message ends here, and that it can proceed to the next message
	len = LittleLong(msg->cursize);
	FS_Write(&len, 4, sv.demoFile);
	FS_Write(msg->data, msg->cursize, sv.demoFile);
	MSG_Clear(msg);
}

/*
====================
SV_DemoWriteClientCommand

Write a client command to the demo file
====================
*/
void SV_DemoWriteClientCommand( client_t *client, const char *cmd )
{
	msg_t msg;

	if ( !SV_CheckClientCommand(client, cmd) ) { // check that this function really needs to store the command (or if another more specialized function should do it instead, eg: userinfo)
		return;
	}

	MSG_Init(&msg, buf, sizeof(buf));
	MSG_WriteByte(&msg, demo_clientCommand);
	MSG_WriteBits(&msg, client - svs.clients, CLIENTNUM_BITS);
	MSG_WriteString(&msg, cmd);
	SV_DemoWriteMessage(&msg);
}

/*
====================
SV_DemoWriteServerCommand

Write a server command to the demo file
====================
*/
void SV_DemoWriteServerCommand( const char *cmd )
{
	msg_t msg;

	if ( !SV_CheckServerCommand( cmd ) ) { // check that this function really needs to store the command
		return;
	}

	MSG_Init(&msg, buf, sizeof(buf));
	MSG_WriteByte(&msg, demo_serverCommand);
	MSG_WriteString(&msg, cmd);
	SV_DemoWriteMessage(&msg);
}

/*
====================
SV_DemoWriteGameCommand

Write a game command to the demo file
====================
*/
void SV_DemoWriteGameCommand( int clientNum, const char *cmd )
{
	msg_t msg;

	if ( !SV_CheckGameCommand( cmd ) ) { // check that this function really needs to store the command
		return;
	}

	MSG_Init(&msg, buf, sizeof(buf));
	MSG_WriteByte(&msg, demo_gameCommand);
	MSG_WriteByte(&msg, clientNum);
	MSG_WriteString(&msg, cmd);
	SV_DemoWriteMessage(&msg);
}

/*
====================
SV_DemoWriteConfigString

Write a configstring to the demo file
cs_index = index of the configstring (see bg_public.h)
cs_string = content of the configstring to set
====================
*/
void SV_DemoWriteConfigString( int cs_index, const char *cs_string )
{
	msg_t msg;
	char *cindex[MAX_CONFIGSTRINGS];

	if ( !SV_CheckConfigString( cs_index, cs_string ) ) { // check that this function really needs to store the configstring (or if it's a client configstring we relay to the specialized function)
		return;
	} // else we save it below as a normal configstring (for capture scores CS_SCORES1/2, for CS_FLAGSTATUS, etc..)

	MSG_Init(&msg, buf, sizeof(buf));
	MSG_WriteByte(&msg, demo_configString);
	//MSG_WriteBits(&msg, index, MAX_CONFIGSTRINGS); // doesn't work - too long - try to replace by a WriteLong instead of WriteString? Or WriteData (with length! and it uses WriteByte)
	sprintf(cindex, "%i", cs_index); // convert index to a string since we don't have any other way to store values that are greater than a byte (and max_configstrings is 1024 currently)
	MSG_WriteString(&msg, (const char*)cindex);
	MSG_WriteString(&msg, cs_string);
	SV_DemoWriteMessage(&msg);
}

/*
====================
SV_DemoWriteClientConfigString

Write a client configstring to the demo file
Note: this is a bit redundant with SV_DemoWriteUserinfo, because clientCommand userinfo sets the new userinfo for a democlient, and clients configstrings are always derived from userinfo string. But this function is left for security purpose (so we make sure the configstring is set right).
====================
*/
void SV_DemoWriteClientConfigString( int clientNum, const char *cs_string )
{
	msg_t msg;

	MSG_Init(&msg, buf, sizeof(buf));
	MSG_WriteByte(&msg, demo_clientConfigString);
	MSG_WriteBits(&msg, clientNum, CLIENTNUM_BITS);
	//MSG_WriteString(&msg, sv.configstrings[CS_PLAYERS + client]); // replaced by the line below, should allow for a more flexible use
	MSG_WriteString(&msg, cs_string);
	SV_DemoWriteMessage(&msg);
}

/*
====================
SV_DemoWriteClientUserinfo

Write a client userinfo to the demo file (client_t fields can almost all be filled from the userinfo string)
Note: in practice, clients userinfo should be loaded before their configstrings (because configstrings derive from userinfo in a real game)
====================
*/
void SV_DemoWriteClientUserinfo( client_t *client, const char *userinfo )
{
	msg_t msg;

	SV_DemoFilterClientUserinfo( userinfo ); // filters out privacy keys such as ip, cl_guid, cl_voip

	MSG_Init(&msg, buf, sizeof(buf));
	MSG_WriteByte(&msg, demo_clientUserinfo); // write the event marker
	MSG_WriteBits(&msg, client - svs.clients, CLIENTNUM_BITS); // write the client number (client_t - svs.clients = client num int)
	MSG_WriteString(&msg, userinfo); // write the (filtered) userinfo string
	SV_DemoWriteMessage(&msg); // commit this demo event in the demo file
}

/*
====================
SV_DemoWriteFrame

Record all the entities (gentities fields) and players (player_t fields) at the end of every frame (this is the only write function to be called in every frame for sure)
Will be called once per server's frame
Called in the main server's loop SV_Frame() in sv_main.c
====================
*/
void SV_DemoWriteFrame(void)
{
	msg_t msg;
	playerState_t *player;
	sharedEntity_t *entity;
	int i;

	MSG_Init(&msg, buf, sizeof(buf));

	// Write entities
	MSG_WriteByte(&msg, demo_entityState);
	for (i = 0; i < sv.num_entities; i++)
	{
		if (i >= sv_maxclients->integer && i < MAX_CLIENTS)
			continue;
		entity = SV_GentityNum(i);
		entity->s.number = i;
		MSG_WriteDeltaEntity(&msg, &sv.demoEntities[i].s, &entity->s, qfalse);
		sv.demoEntities[i].s = entity->s;
	}
	MSG_WriteBits(&msg, ENTITYNUM_NONE, GENTITYNUM_BITS);
	MSG_WriteByte(&msg, demo_entityShared);
	for (i = 0; i < sv.num_entities; i++)
	{
		if (i >= sv_maxclients->integer && i < MAX_CLIENTS)
			continue;
		entity = SV_GentityNum(i);
		MSG_WriteDeltaSharedEntity(&msg, &sv.demoEntities[i].r, &entity->r, qfalse, i);
		sv.demoEntities[i].r = entity->r;
	}
	MSG_WriteBits(&msg, ENTITYNUM_NONE, GENTITYNUM_BITS);
	SV_DemoWriteMessage(&msg);

	// Write clients
	for (i = 0; i < sv_maxclients->integer; i++)
	{
		if (svs.clients[i].state < CS_ACTIVE)
			continue;
		player = SV_GameClientNum(i);
		MSG_WriteByte(&msg, demo_playerState);
		MSG_WriteBits(&msg, i, CLIENTNUM_BITS);
		MSG_WriteDeltaPlayerstate(&msg, &sv.demoPlayerStates[i], player);
		sv.demoPlayerStates[i] = *player;
	}
	MSG_WriteByte(&msg, demo_endFrame);
	MSG_WriteLong(&msg, sv.time);
	SV_DemoWriteMessage(&msg);
}



/***********************************************
 * DEMO READING FUNCTIONS
 *
 * Functions to read demo events
 ***********************************************/

/*
====================
SV_DemoReadFrame

Play a frame from the demo file
This function will read one frame per call, and will process every events contained (switch to the next event when meeting the demo_EOF marker) until it meets the end of the frame (demo_endDemo marker)
Called in the main server's loop SV_Frame() in sv_main.c
====================
*/
void SV_DemoReadFrame(void)
{
	msg_t msg;
	int cmd, r, num, i;
	playerState_t *player;
	client_t *client;
	sharedEntity_t *entity;
	char	*tmpmsg;

	MSG_Init(&msg, buf, sizeof(buf));

	while (1)
	{
exit_loop:
		// Get a message
		r = FS_Read(&msg.cursize, 4, sv.demoFile);
		if (r != 4)
		{
			SV_DemoStopPlayback();
			return;
		}
		msg.cursize = LittleLong(msg.cursize);
		if (msg.cursize > msg.maxsize)
			Com_Error(ERR_DROP, "SV_DemoReadFrame: demo message too long");
		r = FS_Read(msg.data, msg.cursize, sv.demoFile);
		if (r != msg.cursize)
		{
			Com_Printf("Demo file was truncated.\n");
			SV_DemoStopPlayback();
			return;
		}

		// Parse the message
		while (1)
		{
			cmd = MSG_ReadByte(&msg);
			switch (cmd)
			{
			default:
				Com_Error(ERR_DROP, "SV_DemoReadFrame: Illegible demo message\n");
				return;
			case demo_EOF: // end of a demo event (the loop will continue to real the next event)
				MSG_Clear(&msg);
				goto exit_loop;
			case demo_endDemo: // end of the demo file
				SV_DemoStopPlayback();
				return;
			case demo_endFrame: // end of the frame - players and entities game status management: we move every players, update their game status (ammos, armor, placement, etc...) and update entities (items respawn, flags items, etc.), then release the demo frame reading here to the next server (and demo) frame
				// Overwrite anything the game may have changed
				for (i = 0; i < sv.num_entities; i++)
				{
					if (i >= sv_democlients->integer && i < MAX_CLIENTS) // FIXME? shouldn't MAX_CLIENTS be sv_maxclients->integer?
						continue;
					*SV_GentityNum(i) = sv.demoEntities[i];
				}
				for (i = 0; i < sv_democlients->integer; i++)
					*SV_GameClientNum(i) = sv.demoPlayerStates[i];
				// Set the server time
				sv.time = MSG_ReadLong(&msg);
				return;
			case demo_configString: // general configstrings setting (such as capture scores CS_SCORES1/2, etc.) - except clients configstrings
				//num = MSG_ReadBits(&msg, MAX_CONFIGSTRINGS);
				num = atoi(MSG_ReadString(&msg));
				tmpmsg = MSG_ReadString(&msg);
				Com_DPrintf("DebugGBOconfigString: %i %s\n", num, tmpmsg);
				if ( num < CS_PLAYERS + sv_democlients->integer || num >= CS_PLAYERS + sv_maxclients->integer ) { // we make sure to not overwrite real client configstrings (else when the demo starts, normal players will have no name, no model and no status!)
					SV_SetConfigstring(num, tmpmsg); //, qtrue
				}
				break;
			case demo_clientConfigString: // client configstrings setting and clients status management
				num = MSG_ReadBits(&msg, CLIENTNUM_BITS);
				tmpmsg = MSG_ReadString(&msg);
				Com_DPrintf("DebugGBOclientConfigString: %i %i %s\n", num, CS_PLAYERS + num, tmpmsg);
				//SV_SetConfigstring(CS_PLAYERS + num, tmpmsg); //, qtrue
				//SV_SetUserinfo( num, tmpmsg );
				//SV_UpdateUserinfo_f(client);
				//SV_ClientEnterWorld(&svs.clients[num], NULL);
				//VM_Call( gvm, GAME_CLIENT_BEGIN, num );
				//VM_Call( gvm, GAME_CLIENT_USERINFO_CHANGED, num );
				//SV_UpdateConfigstrings( num );
				//SV_SetUserinfo( drop - svs.clients, "" );
				if ( strcmp(sv.configstrings[CS_PLAYERS + num], tmpmsg) && tmpmsg ) { // client begin or just changed team: previous configstring and new one are different, and the new one is not null
					SV_SetConfigstring(CS_PLAYERS + num, tmpmsg);

					// Set some infos about this user:
					svs.clients[num].demoClient = qtrue; // to check if a client is a democlient, you can either rely on this variable, either you can check if num (index of client) is >= CS_PLAYERS + sv_democlients->integer && < CS_PLAYERS + sv_maxclients->integer (if it's not a configstring, remove CS_PLAYERS from your if)
					strcpy( svs.clients[num].name, Info_ValueForKey( tmpmsg, "n" ) ); // set the name (useful for internal functions such as status_f). We use strcpy to copy a const char* to a char[32] (an array, so we need this function) // FIXME: extracted normally from userinfo?
					//svs.clients[num].state or client->state = CS_ACTIVE; // SHOULD NOT SET CS_ACTIVE! Else the engine will try to communicate with these clients, and will produce the following error: Server crashed: netchan queue is not properly initialized in SV_Netchan_TransmitNextFragment

					/*
					// Update userinfo
					client = &svs.clients[num];
					Q_strncpyz( client->userinfo, tmpmsg, sizeof(client->userinfo) );
					SV_UserinfoChanged( client );
					// call prog code to allow overrides
					//VM_Call( gvm, GAME_CLIENT_USERINFO_CHANGED, client - svs.clients );
					*/


					//client = &svs.clients[num];
					//SV_ClientEnterWorld(client, &client->lastUsercmd);
					//SV_SendClientGameState( client );
					VM_Call( gvm, GAME_CLIENT_BEGIN, num ); // does not use argv (directly fetch client infos from userinfo) so no need to tokenize with Cmd_TokenizeString()
				} else if ( strcmp(sv.configstrings[CS_PLAYERS + num], tmpmsg) && !tmpmsg ) { // client disconnect: different configstrings and the new one is empty, so the client is not there anymore
					SV_SetConfigstring(CS_PLAYERS + num, tmpmsg);

					client = &svs.clients[num];
					SV_DropClient( client, "disconnected" ); // or SV_Disconnect_f(client);
					//client->state = CS_FREE;
					//VM_Call( gvm, GAME_CLIENT_DISCONNECT, num ); // Works too! But using SV_DropClient should be cleaner (same as using SV_Disconnect_f)
					//SV_SendServerCommand( client, "disconnect \"%s\"", NULL);
				} else {
					SV_SetConfigstring(CS_PLAYERS + num, tmpmsg);
				}
				//SV_SendClientMessages();
				break;
			case demo_clientUserinfo: // client userinfo setting and client_t fields management
				Cmd_SaveCmdContext();
				num = MSG_ReadBits(&msg, CLIENTNUM_BITS);
				client = &svs.clients[num];
				tmpmsg = MSG_ReadString(&msg);
				sprintf(tmpmsg, "userinfo %s", tmpmsg); // we need to prepend the userinfo command (or in fact any word) to tokenize the userinfo string to the second index because SV_UpdateUserinfo_f expects to fetch it with Argv(1)
				Cmd_TokenizeString(tmpmsg);
				SV_UpdateUserinfo_f(client);
				Com_DPrintf("DebugGBOclientUserinfo: %i %s - %s\n", num, tmpmsg, svs.clients[num].userinfo);
				Cmd_RestoreCmdContext();
				break;
			case demo_clientCommand: // client command management (generally automatic, such as tinfo for HUD team overlay status, team selection, etc.) - except userinfo command that is managed by another event
				Cmd_SaveCmdContext();
				num = MSG_ReadBits(&msg, CLIENTNUM_BITS);
				//client = SV_GameClientNum(num);
				tmpmsg = MSG_ReadString(&msg);
				//Cmd_TokenizeString(tmpmsg);
				Com_DPrintf("DebugGBOclientCommand: %i %s\n", num, tmpmsg);
				SV_ExecuteClientCommand(&svs.clients[num], tmpmsg, qtrue); // 3rd arg = clientOK, and it's necessarily true since we saved the command in the demo (else it wouldn't be saved)
				player = SV_GameClientNum( num );
				Com_DPrintf("DebugGBOclientCommand2 captures: %i %i\n", num, player->persistant[PERS_CAPTURES] );
				Cmd_RestoreCmdContext();
				break;
			case demo_serverCommand: // server command management - except print/cp (already handled by serverCommand),
				Cmd_SaveCmdContext();
				tmpmsg = MSG_ReadString(&msg);
				Cmd_TokenizeString(tmpmsg);
				Com_DPrintf("DebugGBOserverCommand: %s \n", tmpmsg);
				SV_SendServerCommand(NULL, "%s", tmpmsg);
				//SV_SendServerCommand(NULL, "%s \"%s\"", Cmd_Argv(0), Cmd_ArgsFrom(1));
				Cmd_RestoreCmdContext();
				break;
			case demo_gameCommand: // game command management - such as prints/centerprint (cp) scores command - except chat/tchat (handled by clientCommand)
				num = MSG_ReadByte(&msg);
				Cmd_SaveCmdContext();
				tmpmsg = MSG_ReadString(&msg);
				Cmd_TokenizeString(tmpmsg);
				if (strcmp(Cmd_Argv(0), "tinfo")) // too much spamming of tinfo (hud team overlay infos) - don't need those to debug
					Com_DPrintf("DebugGBOgameCommand: %s \n", tmpmsg);
				//VM_Call(gvm, GAME_DEMO_COMMAND, num);
				SV_GameSendServerCommand( -1, tmpmsg );
				//SV_SendServerCommand(NULL, "%s \"%s\"", Cmd_Argv(0), Cmd_ArgsFrom(1)); // same as SV_GameSendServerCommand(-1, text);
				//Com_DPrintf("DebugGBOgameCommand2: %i %s \"%s\"\n", num, Cmd_Argv(0), Cmd_ArgsFrom(1));
				Cmd_RestoreCmdContext();
				break;
			case demo_playerState: // manage playerState_t (some more players game status management, see demo_endFrame)
				num = MSG_ReadBits(&msg, CLIENTNUM_BITS);
				player = SV_GameClientNum(num);
				MSG_ReadDeltaPlayerstate(&msg, &sv.demoPlayerStates[num], player);
				sv.demoPlayerStates[num] = *player;
				break;
			case demo_entityState: // manage gentity (some more gentities game status management, see demo_endFrame)
				while (1)
				{
					num = MSG_ReadBits(&msg, GENTITYNUM_BITS);
					if (num == ENTITYNUM_NONE)
						break;
					entity = SV_GentityNum(num);
					MSG_ReadDeltaEntity(&msg, &sv.demoEntities[num].s, &entity->s, num);
					sv.demoEntities[num].s = entity->s;
				}
				break;
			case demo_entityShared: // more gentity management (GBO: I guess this is what manages items availability for example)
				while (1)
				{
					num = MSG_ReadBits(&msg, GENTITYNUM_BITS);
					if (num == ENTITYNUM_NONE)
						break;
					entity = SV_GentityNum(num);
					MSG_ReadDeltaSharedEntity(&msg, &sv.demoEntities[num].r, &entity->r, num);

					// Link/unlink the entity
					if (entity->r.linked && (!sv.demoEntities[num].r.linked ||
					    entity->r.linkcount != sv.demoEntities[num].r.linkcount))
						SV_LinkEntity(entity);
					else if (!entity->r.linked && sv.demoEntities[num].r.linked)
						SV_UnlinkEntity(entity);

					sv.demoEntities[num].r = entity->r;
					if (num > sv.num_entities)
						sv.num_entities = num;
				}
				break;
			}
		}
	}
}



/***********************************************
 * DEMO MANAGEMENT FUNCTIONS
 *
 * Functions to start/stop the recording/playback of a demo file
 ***********************************************/

/*
====================
SV_DemoStartRecord

Start the recording of a demo by saving some headers data (such as the number of clients, mapname, gametype, etc..)
sv.demo* have already been set and the demo file opened, start writing gamestate info
====================
*/
void SV_DemoStartRecord(void)
{
	msg_t msg;
	int i;

	// Set democlients to 0 since it's only used for replaying demo
	Cvar_SetValueLatched("sv_democlients", 0);

	MSG_Init(&msg, buf, sizeof(buf));

	// Write number of clients (sv_maxclients < MAX_CLIENTS or else we can't playback)
	MSG_WriteBits(&msg, sv_maxclients->integer, CLIENTNUM_BITS);
	// Write current time
	MSG_WriteLong(&msg, sv.time);
	// Write sv_fps
	MSG_WriteLong(&msg, sv_fps->integer);
	// Write g_gametype
	MSG_WriteLong(&msg, sv_gametype->integer);
	// Write map name
	MSG_WriteString(&msg, sv_mapname->string);

	// Write all the above into the demo file
	SV_DemoWriteMessage(&msg);

	// Write initial client configstrings
	for (i = 0; i < sv_maxclients->integer; i++)
	{
		//if (svs.clients[i].state == CS_ACTIVE && sv.configstrings[CS_PLAYERS + i])
		if (&sv.configstrings[CS_PLAYERS + i])
			SV_DemoWriteClientConfigString(i, (const char *)sv.configstrings[CS_PLAYERS + i]);
	}
	//SV_DemoWriteMessage(&msg);

	// Write all configstrings (such as current capture score CS_SCORE1/2, etc...)
	for (i = 0; i < MAX_CONFIGSTRINGS; i++)
	{
		if (&sv.configstrings[i] && i >= 4 && (i < CS_PLAYERS || i > CS_PLAYERS + sv_maxclients->integer)) // client configstrings are already recorded above, we don't want to record them again here - and we don't want to save the first 3 configstrings which are system set
			SV_DemoWriteConfigString(i, sv.configstrings[i]);
	}
	// MSG_WriteByte(&msg2, demo_separator);
	//SV_DemoWriteMessage(demo_separator);
	//SV_DemoWriteMessage(&msg2);

	// Write entities and players
	Com_Memset(sv.demoEntities, 0, sizeof(sv.demoEntities));
	Com_Memset(sv.demoPlayerStates, 0, sizeof(sv.demoPlayerStates));
	SV_DemoWriteFrame();
	Com_Printf("Recording demo %s.\n", sv.demoName);
	sv.demoState = DS_RECORDING;
	Cvar_SetValue("sv_demoState", DS_RECORDING);
}

/*
====================
SV_DemoStopRecord

Stop the recording of a demo
Write end of demo (demo_endDemo marker) and close the demo file
====================
*/
void SV_DemoStopRecord(void)
{
	msg_t msg;

	// End the demo
	MSG_Init(&msg, buf, sizeof(buf));
	MSG_WriteByte(&msg, demo_endDemo);
	SV_DemoWriteMessage(&msg);

	FS_FCloseFile(sv.demoFile);
	sv.demoState = DS_NONE;
	Cvar_SetValue("sv_demoState", DS_NONE);
	Com_Printf("Stopped recording demo %s.\n", sv.demoName);
}

/*
====================
SV_DemoStartPlayback

Start the playback of a demo
sv.demo* have already been set and the demo file opened, start reading gamestate info
This function will also check that everything is alright (such as gametype, map, sv_fps, sv_democlients, sv_maxclients, etc.), if not it will try to fix it automatically
Note for developers: this is basically a mirror of SV_DemoStartRecord() but the other way around (there it writes, here it reads) - but only for the part about the headers, for the rest (eg: userinfo, configstrings, playerstates) it's directly managed by ReadFrame.
====================
*/
void SV_DemoStartPlayback(void)
{
	msg_t msg;
	int r, clients, fps, gametype;
	//int i, num; // FIXME: useless variables
	char *map;
	//char *str;

	if (keepSaved > 0) { // restore keepSaved to 0 (because this is the second time we launch this function, so now there's no need to keep the cvars further)
		keepSaved--;
	}

	MSG_Init(&msg, buf, sizeof(buf));

	// Get the demo header
	r = FS_Read(&msg.cursize, 4, sv.demoFile);
	if (r != 4)
	{
		SV_DemoStopPlayback();
		return;
	}
	msg.cursize = LittleLong(msg.cursize);
	if (msg.cursize == -1)
	{
		SV_DemoStopPlayback();
		return;
	}
	if (msg.cursize > msg.maxsize)
		Com_Error(ERR_DROP, "SV_DemoReadFrame: demo message too long");
	r = FS_Read(msg.data, msg.cursize, sv.demoFile);
	if (r != msg.cursize)
	{
		Com_Printf("Demo file was truncated.\n");
		SV_DemoStopPlayback();
		return;
	}

	// Check slots, time and map
	clients = MSG_ReadBits(&msg, CLIENTNUM_BITS);
	if (sv_democlients->integer < clients) {
		Com_Printf("Not enough demo slots, automatically increasing sv_democlients to %d and sv_maxclients to %d.\n", clients, sv_maxclients->integer + clients);

		// save the old values of sv_maxclients, sv_democlients and bot_minplayers to later restore them
		savedMaxClients = sv_maxclients->integer;
		savedDemoClients = sv_democlients->integer;
		savedBotMinPlayers = Cvar_VariableIntegerValue("bot_minplayers");
		keepSaved = 1;

		// automatically adjusting sv_democlients, sv_maxclients and bot_minplayers
		Cvar_SetValueLatched("sv_democlients", clients);
		Cvar_SetValueLatched("sv_maxclients", sv_maxclients->integer + clients);
		Cvar_SetValue("bot_minplayers", 0); // if we have bots that autoconnect, this will make up for a very weird demo!
		//SV_DemoStopPlayback();
		//return;
	}

	// reading server time (from the demo)
	r = MSG_ReadLong(&msg);
	if (r < 400)
	{
		Com_Printf("Demo time too small: %d.\n", r);
		SV_DemoStopPlayback();
		return;
	}

	// reading sv_fps (from the demo)
	fps = MSG_ReadLong(&msg);
	if ( sv_fps->integer != fps ) {
		savedFPS = sv_fps->integer;
		keepSaved = 1;
		Cvar_SetValue("sv_fps", fps);
	}

	// reading g_gametype (from the demo)
	gametype = MSG_ReadLong(&msg);

	// reading map (from the demo)
	map = MSG_ReadString(&msg);
	if (!FS_FOpenFileRead(va("maps/%s.bsp", map), NULL, qfalse))
	{
		Com_Printf("Map does not exist: %s.\n", map);
		SV_DemoStopPlayback();
		return;
	}


	// Checking if all initial conditions from the demo are met (map, sv_fps, gametype, servertime, etc...)
	// FIXME? why sv_cheats is needed?
	if ( !com_sv_running->integer || strcmp(sv_mapname->string, map) ||
	    !Cvar_VariableIntegerValue("sv_cheats") || r < sv.time ||
	    sv_maxclients->modified || sv_democlients->modified ||
	    sv_gametype->integer != gametype )
	{
		/// Change to the right map and start the demo with a hardcoded 10 seconds delay
		// FIXME: this should not be a hardcoded value, there should be a way to ensure that the map fully restarted before continuing. And this can NOT be based on warmup, because if warmup is set to 0 (disabled), then you'll have no delay, and a delay is necessary! If the demo starts replaying before the map is restarted, it will simply do nothing.
		// delay command is here used as a workaround for waiting until the map is fully restarted

		SV_DemoStopPlayback();
		Cbuf_AddText(va("g_gametype %i\ndevmap %s\ndelay 10000 %s\n", gametype, map, Cmd_Cmd()));
		//Cmd_ExecuteString(va("devmap %s\ndelay 10000 %s\n", s, Cmd_Cmd())); // another way to do it, I think it would be preferable to use cmd_executestring, but it doesn't work (dunno why)
		//Cbuf_AddText(va("devmap %s\ndelay %d %s\n", s, Cvar_VariableIntegerValue("g_warmup") * 1000, Cmd_Cmd())); // Old tremfusion way to do it, which is bad way when g_warmup is 0, you get no delay

		return;
	}


	// Initialize our stuff
	Com_Memset(sv.demoEntities, 0, sizeof(sv.demoEntities));
	Com_Memset(sv.demoPlayerStates, 0, sizeof(sv.demoPlayerStates));
	Cvar_SetValue("sv_democlients", clients);

	// Init democlients configstrings to NULL (and set them as democlients)
	/*
	for (i = 0; i < sv_democlients->integer; i++) {
		svs.clients[i].demoClient = qtrue;
		//svs.clients[i].state = CS_ACTIVE;
		//VM_Call( gvm, GAME_CLIENT_CONNECT, i );
		SV_SetConfigstring(CS_PLAYERS + i, NULL); //qtrue
		//SV_ClientEnterWorld(&svs.clients[i], NULL);
		//VM_Call( gvm, GAME_CLIENT_BEGIN, i );
	}

	// Fill the real democlient configstrings
	for (i = 0; i < sv_democlients->integer; i++) {
		num = MSG_ReadBits(&msg, CLIENTNUM_BITS);
		str = MSG_ReadString(&msg);
		Com_DPrintf("DebugGBOINITclientConfigString: %i %s\n", num, str);
		svs.clients[num].demoClient = qtrue;
		SV_SetConfigstring(CS_PLAYERS + num, str);
		VM_Call( gvm, GAME_CLIENT_BEGIN, num );
	}

	// Fill the general game configstrings (such as capture score CS_SCORES1/2, etc...)
	for (i = 0; i < MAX_CONFIGSTRINGS; i++) {
		num = atoi(MSG_ReadString(&msg));
		str = MSG_ReadString(&msg);
		if (&sv.configstrings[num]) {
			Com_DPrintf("DebugGBOINITconfigString: %i %s\n", num, str);
			SV_SetConfigstring(num, str);
		}
	}
	*/

	// Start reading the first frame
	SV_DemoReadFrame();
	Com_Printf("Playing demo %s.\n", sv.demoName);
	sv.demoState = DS_PLAYBACK;
	Cvar_SetValue("sv_demoState", DS_PLAYBACK);
}

/*
====================
SV_DemoStopPlayback

Close the demo file and restart the map (can be used both when recording or when playing or at shutdown of the game)
====================
*/
void SV_DemoStopPlayback(void)
{
	int olddemostate;
	olddemostate = sv.demoState;
	// Clear client configstrings
	int i;
	for (i = 0; i < sv_democlients->integer; i++)
		SV_SetConfigstring(CS_PLAYERS + i, NULL); //qtrue

	FS_FCloseFile(sv.demoFile);
	sv.demoState = DS_NONE;
	Cvar_SetValue("sv_demoState", DS_NONE);
	Com_Printf("Stopped playing demo %s.\n", sv.demoName);

	// restore maxclients and democlients
	// Note: must do it before the map_restart! so that it takes effect (because it's latched)
	if (keepSaved == 0) {
		if (savedMaxClients >= 0 && savedDemoClients >= 0) {
			Cvar_SetValueLatched("sv_maxclients", savedMaxClients);
			Cvar_SetValueLatched("sv_democlients", savedDemoClients);
		}

		if (savedBotMinPlayers >= 0)
			Cvar_SetValue("bot_minplayers", savedBotMinPlayers);

		if (savedFPS > 0)
			Cvar_SetValue("sv_fps", savedFPS);
	}

	// demo hasn't actually started yet
	if (olddemostate == DS_NONE && keepSaved == 0) {
#ifdef DEDICATED
		//Cbuf_AddText("map_restart 0\n");
#else
		Com_Error (ERR_DROP,"An error happened while replaying the demo, please check the log for more info\n");
		Cbuf_AddText("killserver\n");
#endif
	} else if (olddemostate == DS_PLAYBACK) {
#ifdef DEDICATED
		Cbuf_AddText("map_restart 0\n");
#else
		Cbuf_AddText("map_restart 0\ndelay 2000 killserver\n"); // we have to do a map_restart before killing the client-side local server that was used to replay the demo, for the old restored values for sv_democlients and sv_maxclients to be updated (else, if you directly try to launch another demo just after, it will crash - it seems that 2 consecutive latching without an update makes the engine crash) + we need to have a delay between restarting map and killing the server else it will produce a bug
#endif
	}

	return;

}
