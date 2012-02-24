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

// Headers for demo messages
typedef enum {
	demo_endFrame,
	demo_configString,
	demo_clientConfigString,
	demo_clientCommand,
	demo_serverCommand,
	demo_gameCommand,
	demo_entityState,
	demo_entityShared,
	demo_playerState,
	demo_separator, // used to separate different kind of suite of demo messages (eg: clientconfigstrings, then configstrings, etc..) - can be used in a do loop
	demo_endDemo,
	demo_EOF
} demo_ops_e;

// Big fat buffer to store all our stuff
static byte buf[0x400000];

// Save maxclients and democlients and restore them after the demo
static int savedMaxClients = -1;
static int savedDemoClients = -1;
static int savedBotMinPlayers = -1;
static int savedFPS = -1;
static int keepSaved = 0; // var that memorizes if we keep the new maxclients and democlients values (in the case that we restart the map/server for these cvars to be affected since they are latched) or if we can restore them

/*
====================
SV_DemoWriteMessage

Write a message to the demo file
====================
*/
static void SV_DemoWriteMessage(msg_t *msg)
{
	int len;

	// Write the entire message to the file, prefixed by the length
	MSG_WriteByte(msg, demo_EOF);
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
void SV_DemoWriteClientCommand( client_t *client, const char *str )
{
	msg_t msg;

	MSG_Init(&msg, buf, sizeof(buf));
	MSG_WriteByte(&msg, demo_clientCommand);
	MSG_WriteBits(&msg, client - svs.clients, CLIENTNUM_BITS);
	MSG_WriteString(&msg, str);
	SV_DemoWriteMessage(&msg);
}

/*
====================
SV_DemoWriteServerCommand

Write a server command to the demo file
====================
*/
void SV_DemoWriteServerCommand(const char *str)
{
	msg_t msg;

	MSG_Init(&msg, buf, sizeof(buf));
	MSG_WriteByte(&msg, demo_serverCommand);
	MSG_WriteString(&msg, str);
	SV_DemoWriteMessage(&msg);
}

/*
====================
SV_DemoWriteGameCommand

Write a game command to the demo file
====================
*/
void SV_DemoWriteGameCommand(int cmd, const char *str)
{
	msg_t msg;

	MSG_Init(&msg, buf, sizeof(buf));
	MSG_WriteByte(&msg, demo_gameCommand);
	MSG_WriteByte(&msg, cmd);
	MSG_WriteString(&msg, str);
	SV_DemoWriteMessage(&msg);
}

/*
====================
SV_DemoWriteConfigString

Write a configstring to the demo file
====================
*/
void SV_DemoWriteConfigString( int index, const char *str )
{
	msg_t msg;
	char *cindex[MAX_CONFIGSTRINGS];

	MSG_Init(&msg, buf, sizeof(buf));
	MSG_WriteByte(&msg, demo_configString);
	//MSG_WriteBits(&msg, index, MAX_CONFIGSTRINGS); // doesn't work - too long - try to replace by a WriteLong instead of WriteString? Or WriteData (with length! and it uses WriteByte)
	sprintf(cindex, "%i", index); // convert index to a string since we don't have any other way to store values that are greater than a byte (and max_configstrings is 1024 currently)
	MSG_WriteString(&msg, cindex);
	MSG_WriteString(&msg, str);
	SV_DemoWriteMessage(&msg);
}

/*
====================
SV_DemoWriteClientConfigString

Write a client configstring to the demo file
====================
*/
void SV_DemoWriteClientConfigString(int client)
{
	msg_t msg;

	MSG_Init(&msg, buf, sizeof(buf));
	MSG_WriteByte(&msg, demo_clientConfigString);
	MSG_WriteBits(&msg, client, CLIENTNUM_BITS);
	MSG_WriteString(&msg, sv.configstrings[CS_PLAYERS + client]);
	SV_DemoWriteMessage(&msg);
}

/*
====================
SV_DemoWriteFrame

Record all the entities and players at the end the frame
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

/*
====================
SV_DemoReadFrame

Play a frame from the demo file
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
			case demo_EOF:
				MSG_Clear(&msg);
				goto exit_loop;
			case demo_endDemo:
				SV_DemoStopPlayback();
				return;
			case demo_endFrame:
				// Overwrite anything the game may have changed
				for (i = 0; i < sv.num_entities; i++)
				{
					if (i >= sv_democlients->integer && i < MAX_CLIENTS)
						continue;
					*SV_GentityNum(i) = sv.demoEntities[i];
				}
				for (i = 0; i < sv_democlients->integer; i++)
					*SV_GameClientNum(i) = sv.demoPlayerStates[i];
				// Set the server time
				sv.time = MSG_ReadLong(&msg);
				return;
			case demo_configString:
				//num = MSG_ReadBits(&msg, MAX_CONFIGSTRINGS);
				num = atoi(MSG_ReadString(&msg));
				tmpmsg = MSG_ReadString(&msg);
				Com_Printf("DebugGBOconfigString: %i %s\n", num, tmpmsg);
				SV_SetConfigstring(num, tmpmsg); //, qtrue
				break;
			case demo_clientConfigString:
				num = MSG_ReadBits(&msg, CLIENTNUM_BITS);
				tmpmsg = MSG_ReadString(&msg);
				Com_Printf("DebugGBOclientConfigString: %i %i %s\n", num, CS_PLAYERS + num, tmpmsg);
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

					//client = &svs.clients[num];
					//SV_ClientEnterWorld(client, &client->lastUsercmd);
					//SV_SendClientGameState( client );
					VM_Call( gvm, GAME_CLIENT_BEGIN, num );
				} else if ( strcmp(sv.configstrings[CS_PLAYERS + num], tmpmsg) && !tmpmsg ) { // client disconnect: different configstrings and the new one is empty, so the client is not there anymore
					SV_SetConfigstring(CS_PLAYERS + num, tmpmsg);

					//client = &svs.clients[num];
					//SV_DropClient( client, "disconnected" ); or SV_Disconnect_f(client);
					VM_Call( gvm, GAME_CLIENT_DISCONNECT, num );
					//SV_SendServerCommand( client, "disconnect \"%s\"", NULL);
				} else {
					SV_SetConfigstring(CS_PLAYERS + num, tmpmsg);
				}
				//SV_SendClientMessages();
				break;
			case demo_clientCommand:
				Cmd_SaveCmdContext();
				num = MSG_ReadBits(&msg, CLIENTNUM_BITS);
				//client = SV_GameClientNum(num);
				tmpmsg = MSG_ReadString(&msg);
				//Cmd_TokenizeString(tmpmsg);
				Com_Printf("DebugGBOclientCommand: %i %s\n", num, tmpmsg);
				SV_ExecuteClientCommand(&svs.clients[num], tmpmsg, qtrue); // 3rd arg = clientOK, and it's necessarily true since we saved the command in the demo (else it wouldn't be saved)
				player = SV_GameClientNum( i );
				Com_Printf("DebugGBOclientCommand2 captures: %i %i\n", num, player->persistant[PERS_CAPTURES] );
				Cmd_RestoreCmdContext();
				break;
			case demo_serverCommand:
				Cmd_SaveCmdContext();
				tmpmsg = MSG_ReadString(&msg);
				Cmd_TokenizeString(tmpmsg);
				Com_Printf("DebugGBOserverCommand: %s\n", tmpmsg);
				SV_SendServerCommand(NULL, "%s", tmpmsg);
				//SV_SendServerCommand(NULL, "%s \"%s\"", Cmd_Argv(0), Cmd_ArgsFrom(1));
				Cmd_RestoreCmdContext();
				break;
			case demo_gameCommand:
				num = MSG_ReadByte(&msg);
				Cmd_SaveCmdContext();
				tmpmsg = MSG_ReadString(&msg);
				Cmd_TokenizeString(tmpmsg);
				if (strcmp(Cmd_Argv(0), "tinfo")) // too much spamming of tinfo (hud team overlay infos) - don't need those to debug
					Com_Printf("DebugGBOgameCommand: %s\n", tmpmsg);
				//VM_Call(gvm, GAME_DEMO_COMMAND, num);
				SV_GameSendServerCommand( -1, tmpmsg );
				//SV_SendServerCommand(NULL, "%s \"%s\"", Cmd_Argv(0), Cmd_ArgsFrom(1)); // same as SV_GameSendServerCommand(-1, text);
				//Com_Printf("DebugGBOgameCommand2: %i %s \"%s\"\n", num, Cmd_Argv(0), Cmd_ArgsFrom(1));
				Cmd_RestoreCmdContext();
				break;
			case demo_playerState:
				num = MSG_ReadBits(&msg, CLIENTNUM_BITS);
				player = SV_GameClientNum(num);
				MSG_ReadDeltaPlayerstate(&msg, &sv.demoPlayerStates[num], player);
				sv.demoPlayerStates[num] = *player;
				break;
			case demo_entityState:
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
			case demo_entityShared:
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

/*
====================
SV_DemoStartRecord

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

	// Write client configstrings
	for (i = 0; i < sv_maxclients->integer; i++)
	{
		//if (svs.clients[i].state == CS_ACTIVE && sv.configstrings[CS_PLAYERS + i])
		if (&sv.configstrings[CS_PLAYERS + i])
			SV_DemoWriteClientConfigString(i);
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

Write end of file and close the demo file
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

sv.demo* have already been set and the demo file opened, start reading gamestate info
====================
*/
void SV_DemoStartPlayback(void)
{
	msg_t msg;
	int r, i, clients, fps, gametype, num;
	char *map;
	char *str;

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
	// TOFIX? why sv_cheats is needed?
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
		Com_Printf("DebugGBOINITclientConfigString: %i %s\n", num, str);
		svs.clients[num].demoClient = qtrue;
		SV_SetConfigstring(CS_PLAYERS + num, str);
		VM_Call( gvm, GAME_CLIENT_BEGIN, num );
	}

	// Fill the general game configstrings (such as capture score CS_SCORES1/2, etc...)
	for (i = 0; i < MAX_CONFIGSTRINGS; i++) {
		num = atoi(MSG_ReadString(&msg));
		str = MSG_ReadString(&msg);
		if (&sv.configstrings[num]) {
			Com_Printf("DebugGBOINITconfigString: %i %s\n", num, str);
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

Close the demo file and restart the map
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
