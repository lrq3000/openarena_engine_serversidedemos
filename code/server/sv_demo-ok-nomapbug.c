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
	demo_serverCommand,
	demo_gameCommand,
	demo_entityState,
	demo_entityShared,
	demo_playerState,
	demo_endDemo,
	demo_EOF
} demo_ops_e;

// Big fat buffer to store all our stuff
static byte buf[0x400000];

// Save maxclients and democlients and restore them after the demo
static int savedMaxClients, savedDemoClients;

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

Write a client configstring to the demo file
====================
*/
void SV_DemoWriteConfigString(int client)
{
	msg_t msg;

	MSG_Init(&msg, buf, sizeof(buf));
	MSG_WriteByte(&msg, demo_configString);
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
	sharedEntity_t *entity;

	MSG_Init(&msg, buf, sizeof(buf));

	while (1)
	{
exit_loop:
		// Get a message
		r = FS_Read(&msg.cursize, 4, sv.demoFile);
		Com_Printf("DEBUGGBOA: %d\n", r);
		if (r != 4)
		{
			SV_DemoStopPlayback();
			return;
		}
		msg.cursize = LittleLong(msg.cursize);
		if (msg.cursize > msg.maxsize)
			Com_Error(ERR_DROP, "SV_DemoReadFrame: demo message too long");
		r = FS_Read(msg.data, msg.cursize, sv.demoFile);
		Com_Printf("DEBUGGBOB: %d\n", r);
		Com_Printf("DEBUGGBOB2: %d\n", msg.cursize);
		if (r != msg.cursize)
		{
			Com_Printf("Demo file was truncated.\n");
			SV_DemoStopPlayback();
			return;
		}

		// Parse the message
		while (1)
		{
			Com_Printf("DEBUGGBOPREMSG\n");
			cmd = MSG_ReadByte(&msg);
			Com_Printf("DEBUGGBOPOSTMSG\n");
			switch (cmd)
			{
			default:
				Com_Printf("DEBUGGBOS1\n");
				Com_Error(ERR_DROP, "SV_DemoReadFrame: Illegible demo message\n");
				return;
			case demo_EOF:
				Com_Printf("DEBUGGBOS2\n");
				MSG_Clear(&msg);
				goto exit_loop;
			case demo_endDemo:
				Com_Printf("DEBUGGBOS3\n");
				SV_DemoStopPlayback();
				return;
			case demo_endFrame:
				Com_Printf("DEBUGGBOS4\n");
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
				num = MSG_ReadBits(&msg, CLIENTNUM_BITS);
				SV_SetConfigstring(CS_PLAYERS + num, MSG_ReadString(&msg));
				break;
			case demo_serverCommand:
				Com_Printf("DEBUGGBOS5\n");
				Cmd_SaveCmdContext();
				Cmd_TokenizeString(MSG_ReadString(&msg));
				SV_SendServerCommand(NULL, "%s \"^3[DEMO] ^7%s\"", Cmd_Argv(0), Cmd_ArgsFrom(1));
				Com_Printf("DEBUGGBOS5-2: %s --- %s\n",Cmd_Argv(0), Cmd_ArgsFrom(1));
				Cmd_RestoreCmdContext();
				break;
			case demo_gameCommand:
				Com_Printf("DEBUGGBOS6\n");
				num = MSG_ReadByte(&msg);
				Cmd_SaveCmdContext();
				Cmd_TokenizeString(MSG_ReadString(&msg));
				VM_Call(gvm, GAME_DEMO_COMMAND, num);
				Cmd_RestoreCmdContext();
				break;
			case demo_playerState:
				Com_Printf("DEBUGGBOS7\n");
				num = MSG_ReadBits(&msg, CLIENTNUM_BITS);
				player = SV_GameClientNum(num);
				MSG_ReadDeltaPlayerstate(&msg, &sv.demoPlayerStates[num], player);
				sv.demoPlayerStates[num] = *player;
				break;
			case demo_entityState:
				Com_Printf("DEBUGGBOS8\n");
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
				Com_Printf("DEBUGGBOS9\n");
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
			Com_Printf("DEBUGGBOENDSWITCH\n");
		}
		Com_Printf("DEBUGGBOENDWHILE\n");
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

	MSG_Init(&msg, buf, sizeof(buf));

	// Write number of clients (sv_maxclients < MAX_CLIENTS or else we can't playback)
	MSG_WriteBits(&msg, sv_maxclients->integer, CLIENTNUM_BITS);
	// Write current time
	MSG_WriteLong(&msg, sv.time);
	// Write map name
	MSG_WriteString(&msg, sv_mapname->string);
	SV_DemoWriteMessage(&msg);

	// Write client configstrings
	for (i = 0; i < sv_maxclients->integer; i++)
	{
		if (svs.clients[i].state == CS_ACTIVE && sv.configstrings[CS_PLAYERS + i])
			SV_DemoWriteConfigString(i);
	}
	SV_DemoWriteMessage(&msg);

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
	int r, i, clients;
	char *s;

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
	if (sv_democlients->integer < clients)
	{
		Com_Printf("Not enough demo slots, increase sv_democlients to %d.\n", clients);
		SV_DemoStopPlayback();
		return;
	}
	r = MSG_ReadLong(&msg);
	if (r < 400)
	{
		Com_Printf("Demo time too small: %d.\n", r);
		SV_DemoStopPlayback();
		return;
	}
	s = MSG_ReadString(&msg);
	if (!FS_FOpenFileRead(va("maps/%s.bsp", s), NULL, qfalse))
	{
		Com_Printf("Map does not exist: %s.\n", s);
		SV_DemoStopPlayback();
		return;
	}
	if (!com_sv_running->integer || strcmp(sv_mapname->string, s) ||
	    !Cvar_VariableIntegerValue("sv_cheats") || r < sv.time ||
	    sv_maxclients->modified || sv_democlients->modified)
	{
		// Change to the right map and start the demo with a g_warmup second delay
		Cbuf_AddText(va("devmap %s\ndelay %d %s\n", s, Cvar_VariableIntegerValue("g_warmup") * 1000, Cmd_Cmd()));
		//Cbuf_AddText(va("devmap %s\ndelay %d\n%s\n", s, Cvar_VariableIntegerValue("g_warmup") * 1000, Cmd_Cmd())); // DEBUGTEST
		//Cbuf_AddText(va("devmap %s\nwait 200\n%s\n", s, Cmd_Cmd()));
		SV_DemoStopPlayback();
		return;
	}

	Com_Printf("DEBUGGBO1\n");

	// Initialize our stuff
	Com_Memset(sv.demoEntities, 0, sizeof(sv.demoEntities));
	Com_Printf("DEBUGGBO2\n");
	Com_Memset(sv.demoPlayerStates, 0, sizeof(sv.demoPlayerStates));
	Com_Printf("DEBUGGBO3\n");
	Cvar_SetValue("sv_democlients", clients);
	Com_Printf("DEBUGGBO4\n");
	for (i = 0; i < sv_democlients->integer; i++)
		SV_SetConfigstring(CS_PLAYERS + i, NULL); //qtrue
	SV_DemoReadFrame();
	Com_Printf("DEBUGGBO5\n");
	Com_Printf("Playing demo %s.\n", sv.demoName);
	Com_Printf("DEBUGGBO6\n");
	sv.demoState = DS_PLAYBACK;
	Com_Printf("DEBUGGBO7\n");
	Cvar_SetValue("sv_demoState", DS_PLAYBACK);
	Com_Printf("DEBUGGBO8\n");
}

/*
====================
SV_DemoStopPlayback

Close the demo file and restart the map
====================
*/
void SV_DemoStopPlayback(void)
{
	// Clear client configstrings
	int i;
	for (i = 0; i < sv_democlients->integer; i++)
		SV_SetConfigstring(CS_PLAYERS + i, NULL); //qtrue

	FS_FCloseFile(sv.demoFile);
	sv.demoState = DS_NONE;
	Cvar_SetValue("sv_demoState", DS_NONE);
	Com_Printf("Stopped playing demo %s.\n", sv.demoName);

	// restore maxclients and democlients
	// TOFIX: set to 0 (why?)
	//Cvar_SetValue("sv_maxclients", savedMaxClients);
	//Cvar_SetValue("sv_democlients", savedDemoClients);

	// demo hasn't actually started yet

	if (sv.demoState != DS_PLAYBACK)
#ifdef DEDICATED
		Cbuf_AddText("map_restart 0\n");
#else
		Cbuf_AddText("killserver\n");
#endif

}
