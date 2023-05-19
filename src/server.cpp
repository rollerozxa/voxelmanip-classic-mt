/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "server.h"
#include <iostream>
#include "network/connection.h"
#include "network/networkprotocol.h"
#include "constants.h"
#include "filesys.h"
#include "settings.h"
#include "scripting_server.h"
#include "nodedef.h"
#include "itemdef.h"
#include "modchannels.h"
#include "util/string.h"
#include "util/serialize.h"
#include "server/mods.h"
#include "chatmessage.h"
#include "chat_interface.h"
#include "remoteplayer.h"
#include "server/player_sao.h"
#include "server/serverinventorymgr.h"
#include "database/database-files.h"
#include "database/database-dummy.h"

class ClientNotFoundException : public BaseException
{
public:
	ClientNotFoundException(const char *s):
		BaseException(s)
	{}
};

class ServerThread : public Thread
{
public:

	ServerThread(Server *server):
		Thread("Server"),
		m_server(server)
	{}

	void *run();

private:
	Server *m_server;
};

void *ServerThread::run()
{

	return nullptr;
}

/*
	Server
*/

Server::Server(
		const std::string &path_world,
		const SubgameSpec &gamespec,
		bool simple_singleplayer_mode,
		Address bind_addr,
		bool dedicated,
		ChatInterface *iface,
		std::string *on_shutdown_errmsg
	):
	m_bind_addr(bind_addr),
	m_path_world(path_world),
	m_gamespec(gamespec),
	m_simple_singleplayer_mode(simple_singleplayer_mode),
	m_dedicated(dedicated),
	m_async_fatal_error(""),
	m_con(std::make_shared<con::Connection>(PROTOCOL_ID,
			512,
			CONNECTION_TIMEOUT,
			m_bind_addr.isIPv6(),
			this)),
	m_itemdef(createItemDefManager()),
	m_nodedef(createNodeDefManager()),
	m_thread(new ServerThread(this)),
	m_clients(m_con),
	m_admin_chat(iface),
	m_on_shutdown_errmsg(on_shutdown_errmsg),
	m_modchannel_mgr(new ModChannelMgr()) {}

Server::~Server() {}

void Server::init() {}

void Server::start() {}

void Server::stop() {}

void Server::step(float dtime) {}

void Server::AsyncRunStep(bool initial_step)
{


}

void Server::Receive() {}

void Server::ProcessData(NetworkPacket *pkt) {}

void Server::setTimeOfDay(u32 time) {}

void Server::onMapEditEvent(const MapEditEvent &event) {}

void Server::peerAdded(con::Peer *peer) {}

void Server::deletingPeer(con::Peer *peer, bool timeout) {}

void Server::Send(NetworkPacket *pkt) {}

void Server::Send(session_t peer_id, NetworkPacket *pkt) {}

void Server::SendMovement(session_t peer_id) {}

void Server::HandlePlayerHPChange(PlayerSAO *playersao, const PlayerHPChangeReason &reason) {}

void Server::SendPlayerHP(PlayerSAO *playersao, bool effect) {}

void Server::SendHP(session_t peer_id, u16 hp, bool effect) {}

void Server::SendBreath(session_t peer_id, u16 breath) {}

void Server::SendAccessDenied(session_t peer_id, AccessDeniedCode reason,
		const std::string &custom_reason, bool reconnect) {}

void Server::SendDeathscreen(session_t peer_id, bool set_camera_point_target,
		v3f camera_point_target) {}

void Server::SendNodeDef(session_t peer_id,
	const NodeDefManager *nodedef, u16 protocol_version) {}

void Server::SendInventory(PlayerSAO *sao, bool incremental) {}

void Server::SendPlayerBreath(PlayerSAO *sao) {}

void Server::SendMovePlayer(session_t peer_id) {}

void Server::stopSound(s32 handle) {}

void Server::fadeSound(s32 handle, float step, float gain) {}

bool Server::SendBlock(session_t peer_id, const v3s16 &blockpos)
{
	return true;
}


/*
	Something random
*/


void Server::DenyAccess(session_t peer_id, AccessDeniedCode reason,
		const std::string &custom_reason, bool reconnect) {}

RemoteClient *Server::getClient(session_t peer_id, ClientState state_min)
{
	RemoteClient *client = getClientNoEx(peer_id,state_min);
	if(!client)
		throw ClientNotFoundException("Client not found");

	return client;
}
RemoteClient *Server::getClientNoEx(session_t peer_id, ClientState state_min)
{
	return m_clients.getClientNoEx(peer_id, state_min);
}

PlayerSAO *Server::getPlayerSAO(session_t peer_id)
{
	RemotePlayer *player = m_env->getPlayer(peer_id);
	if (!player)
		return NULL;
	return player->getPlayerSAO();
}

void Server::setLocalPlayerAnimations(RemotePlayer *player,
		v2s32 animation_frames[4], f32 frame_speed) {}

void Server::setPlayerEyeOffset(RemotePlayer *player, const v3f &first, const v3f &third) {}

void Server::setSky(RemotePlayer *player, const SkyboxParams &params) {}

void Server::setSun(RemotePlayer *player, const SunParams &params) {}

void Server::setMoon(RemotePlayer *player, const MoonParams &params) {}

void Server::setStars(RemotePlayer *player, const StarParams &params) {}

void Server::setClouds(RemotePlayer *player, const CloudParams &params) {}

void Server::overrideDayNightRatio(RemotePlayer *player, bool do_override,
	float ratio) {}

void Server::setLighting(RemotePlayer *player, const Lighting &lighting) {}

void Server::spawnParticle(const std::string &playername,
	const ParticleParameters &p) {}

u32 Server::addParticleSpawner(const ParticleSpawnerParameters &p,
	ServerActiveObject *attached, const std::string &playername)
{
	return -1;

}

void Server::deleteParticleSpawner(const std::string &playername, u32 id) {}

// IGameDef interface
// Under envlock
IItemDefManager *Server::getItemDefManager()
{
	return m_itemdef;
}

const NodeDefManager *Server::getNodeDefManager()
{
	return m_nodedef;
}

u16 Server::allocateUnknownNodeId(const std::string &name)
{
	return m_nodedef->allocateDummy(name);
}

IWritableItemDefManager *Server::getWritableItemDefManager()
{
	return m_itemdef;
}

NodeDefManager *Server::getWritableNodeDefManager()
{
	return m_nodedef;
}

const std::vector<ModSpec> & Server::getMods() const
{
	return m_modmgr->getMods();
}

const ModSpec *Server::getModSpec(const std::string &modname) const
{
	return m_modmgr->getModSpec(modname);
}

std::string Server::getBuiltinLuaPath()
{
	return porting::path_share + DIR_DELIM + "builtin";
}

v3f Server::findSpawnPos()
{
	return v3f(0.0f, 0.0f, 0.0f);
}

/*
 * Mod channels
 */


bool Server::joinModChannel(const std::string &channel)
{
	return true;
}

bool Server::leaveModChannel(const std::string &channel)
{
	return true;
}

bool Server::sendModChannelMessage(const std::string &channel, const std::string &message)
{

	return true;
}

ModChannel* Server::getModChannel(const std::string &channel)
{
	return m_modchannel_mgr->getModChannel(channel);
}

ModStorageDatabase *Server::openModStorageDatabase(const std::string &world_path)
{
	std::string world_mt_path = world_path + DIR_DELIM + "world.mt";
	Settings world_mt;
	if (!world_mt.readConfigFile(world_mt_path.c_str()))
		throw BaseException("Cannot read world.mt!");

	std::string backend = world_mt.exists("mod_storage_backend") ?
		world_mt.get("mod_storage_backend") : "files";

	return openModStorageDatabase(backend, world_path, world_mt);
}

ModStorageDatabase *Server::openModStorageDatabase(const std::string &backend,
		const std::string &world_path, const Settings &world_mt)
{
	if (backend == "files")
		return new ModStorageDatabaseFiles(world_path);

	if (backend == "dummy")
		return new Database_Dummy();

	throw BaseException("Mod storage database backend " + backend + " not supported");
}
