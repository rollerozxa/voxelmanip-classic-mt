/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "scripting_server.h"
#include "server.h"
#include "log.h"
#include "settings.h"
#include "cpp_api/s_internal.h"
#include "lua_api/l_base.h"
#include "lua_api/l_env.h"
#include "lua_api/l_item.h"
#include "lua_api/l_itemstackmeta.h"
#include "lua_api/l_modchannels.h"
#include "lua_api/l_nodemeta.h"
#include "lua_api/l_object.h"
#include "lua_api/l_particles.h"
#include "lua_api/l_util.h"
#include "lua_api/l_settings.h"
#include "lua_api/l_storage.h"

extern "C" {
#include <lualib.h>
}

ServerScripting::ServerScripting(Server* server):
		ScriptApiBase(ScriptingType::Server)
{
	setGameDef(server);

	// setEnv(env) is called by ScriptApiEnv::initializeEnvironment()
	// once the environment has been created

	SCRIPTAPI_PRECHECKHEADER

	lua_getglobal(L, "core");
	int top = lua_gettop(L);

	lua_newtable(L);
	lua_setfield(L, -2, "object_refs");

	lua_newtable(L);
	lua_setfield(L, -2, "luaentities");

	// Initialize our lua_api modules
	InitializeModApi(L, top);
	lua_pop(L, 1);

	// Push builtin initialization type
	lua_pushstring(L, "game");
	lua_setglobal(L, "INIT");

	infostream << "SCRIPTAPI: Initialized game modules" << std::endl;
}

void ServerScripting::InitializeModApi(lua_State *L, int top)
{
	// Register reference classes (userdata)
	ItemStackMetaRef::Register(L);
	LuaItemStack::Register(L);
	NodeMetaRef::Register(L);
	LuaSettings::Register(L);
	StorageRef::Register(L);
	ModChannelRef::Register(L);

	// Initialize mod api modules
	ModApiEnv::Initialize(L, top);
	ModApiUtil::Initialize(L, top);
	ModApiStorage::Initialize(L, top);
	ModApiChannels::Initialize(L, top);
}
