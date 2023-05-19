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

#include <algorithm>
#include "lua_api/l_env.h"
#include "lua_api/l_internal.h"
#include "lua_api/l_nodemeta.h"
#include "common/c_converter.h"
#include "common/c_content.h"
#include "scripting_server.h"
#include "environment.h"
#include "mapblock.h"
#include "server.h"
#include "nodedef.h"
#include "daynightratio.h"
#include "util/pointedthing.h"
#include "emerge.h"
#include "face_position_cache.h"
#include "remoteplayer.h"
#include "server/luaentity_sao.h"
#include "server/player_sao.h"
#include "util/string.h"
#include "translation.h"
#ifndef SERVER
#include "client/client.h"
#endif

const EnumString ModApiEnvMod::es_BlockStatusType[] =
{
	{ServerEnvironment::BS_UNKNOWN, "unknown"},
	{ServerEnvironment::BS_EMERGING, "emerging"},
	{ServerEnvironment::BS_LOADED,  "loaded"},
	{ServerEnvironment::BS_ACTIVE,  "active"},
	{0, NULL},
};

///////////////////////////////////////////////////////////////////////////////



int LuaRaycast::l_next(lua_State *L)
{
	GET_PLAIN_ENV_PTR;

	bool csm = false;
#ifndef SERVER
	csm = getClient(L) != nullptr;
#endif

	LuaRaycast *o = checkObject<LuaRaycast>(L, 1);
	PointedThing pointed;
	env->continueRaycast(&o->state, &pointed);
	if (pointed.type == POINTEDTHING_NOTHING)
		lua_pushnil(L);
	else
		push_pointed_thing(L, pointed, csm, true);

	return 1;
}

int LuaRaycast::create_object(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	bool objects = true;
	bool liquids = false;

	v3f pos1 = checkFloatPos(L, 1);
	v3f pos2 = checkFloatPos(L, 2);
	if (lua_isboolean(L, 3)) {
		objects = readParam<bool>(L, 3);
	}
	if (lua_isboolean(L, 4)) {
		liquids = readParam<bool>(L, 4);
	}

	LuaRaycast *o = new LuaRaycast(core::line3d<f32>(pos1, pos2),
		objects, liquids);

	*(void **) (lua_newuserdata(L, sizeof(void *))) = o;
	luaL_getmetatable(L, className);
	lua_setmetatable(L, -2);
	return 1;
}

int LuaRaycast::gc_object(lua_State *L)
{
	LuaRaycast *o = *(LuaRaycast **) (lua_touserdata(L, 1));
	delete o;
	return 0;
}

void LuaRaycast::Register(lua_State *L)
{
	static const luaL_Reg metamethods[] = {
		{"__call", l_next},
		{"__gc", gc_object},
		{0, 0}
	};
	registerClass(L, className, methods, metamethods);

	lua_register(L, className, create_object);
}

const char LuaRaycast::className[] = "Raycast";
const luaL_Reg LuaRaycast::methods[] =
{
	luamethod(LuaRaycast, next),
	{ 0, 0 }
};

// Exported functions

// set_node(pos, node)
// pos = {x=num, y=num, z=num}
int ModApiEnvMod::l_set_node(lua_State *L)
{
	GET_ENV_PTR;

	// parameters
	v3s16 pos = read_v3s16(L, 1);
	MapNode n = readnode(L, 2);
	// Do it
	bool succeeded = env->setNode(pos, n);
	lua_pushboolean(L, succeeded);
	return 1;
}

// bulk_set_node([pos1, pos2, ...], node)
// pos = {x=num, y=num, z=num}
int ModApiEnvMod::l_bulk_set_node(lua_State *L)
{
	GET_ENV_PTR;

	// parameters
	if (!lua_istable(L, 1)) {
		return 0;
	}

	s32 len = lua_objlen(L, 1);
	if (len == 0) {
		lua_pushboolean(L, true);
		return 1;
	}

	MapNode n = readnode(L, 2);

	// Do it
	bool succeeded = true;
	for (s32 i = 1; i <= len; i++) {
		lua_rawgeti(L, 1, i);
		if (!env->setNode(read_v3s16(L, -1), n))
			succeeded = false;
		lua_pop(L, 1);
	}

	lua_pushboolean(L, succeeded);
	return 1;
}

int ModApiEnvMod::l_add_node(lua_State *L)
{
	return l_set_node(L);
}

// remove_node(pos)
// pos = {x=num, y=num, z=num}
int ModApiEnvMod::l_remove_node(lua_State *L)
{
	GET_ENV_PTR;

	// parameters
	v3s16 pos = read_v3s16(L, 1);
	// Do it
	bool succeeded = env->removeNode(pos);
	lua_pushboolean(L, succeeded);
	return 1;
}

// swap_node(pos, node)
// pos = {x=num, y=num, z=num}
int ModApiEnvMod::l_swap_node(lua_State *L)
{
	GET_ENV_PTR;

	// parameters
	v3s16 pos = read_v3s16(L, 1);
	MapNode n = readnode(L, 2);
	// Do it
	bool succeeded = env->swapNode(pos, n);
	lua_pushboolean(L, succeeded);
	return 1;
}

// get_node(pos)
// pos = {x=num, y=num, z=num}
int ModApiEnvMod::l_get_node(lua_State *L)
{
	GET_ENV_PTR;

	// pos
	v3s16 pos = read_v3s16(L, 1);
	// Do it
	MapNode n = env->getMap().getNode(pos);
	// Return node
	pushnode(L, n);
	return 1;
}

// get_node_or_nil(pos)
// pos = {x=num, y=num, z=num}
int ModApiEnvMod::l_get_node_or_nil(lua_State *L)
{
	GET_ENV_PTR;

	// pos
	v3s16 pos = read_v3s16(L, 1);
	// Do it
	bool pos_ok;
	MapNode n = env->getMap().getNode(pos, &pos_ok);
	if (pos_ok) {
		// Return node
		pushnode(L, n);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

// get_node_light(pos, timeofday)
// pos = {x=num, y=num, z=num}
// timeofday: nil = current time, 0 = night, 0.5 = day
int ModApiEnvMod::l_get_node_light(lua_State *L)
{
	GET_PLAIN_ENV_PTR;

	// Do it
	v3s16 pos = read_v3s16(L, 1);
	u32 time_of_day = env->getTimeOfDay();
	if(lua_isnumber(L, 2))
		time_of_day = 24000.0 * lua_tonumber(L, 2);
	time_of_day %= 24000;
	u32 dnr = time_to_daynight_ratio(time_of_day, true);

	bool is_position_ok;
	MapNode n = env->getMap().getNode(pos, &is_position_ok);
	if (is_position_ok) {
		const NodeDefManager *ndef = env->getGameDef()->ndef();
		lua_pushinteger(L, n.getLightBlend(dnr, ndef->getLightingFlags(n)));
	} else {
		lua_pushnil(L);
	}
	return 1;
}


// get_natural_light(pos, timeofday)
// pos = {x=num, y=num, z=num}
// timeofday: nil = current time, 0 = night, 0.5 = day
int ModApiEnvMod::l_get_natural_light(lua_State *L)
{
	GET_ENV_PTR;

	v3s16 pos = read_v3s16(L, 1);

	bool is_position_ok;
	MapNode n = env->getMap().getNode(pos, &is_position_ok);
	if (!is_position_ok)
		return 0;

	// If the daylight is 0, nothing needs to be calculated
	u8 daylight = n.param1 & 0x0f;
	if (daylight == 0) {
		lua_pushinteger(L, 0);
		return 1;
	}

	u32 time_of_day;
	if (lua_isnumber(L, 2)) {
		time_of_day = 24000.0 * lua_tonumber(L, 2);
		time_of_day %= 24000;
	} else {
		time_of_day = env->getTimeOfDay();
	}
	u32 dnr = time_to_daynight_ratio(time_of_day, true);

	// If it's the same as the artificial light, the sunlight needs to be
	// searched for because the value may not emanate from the sun
	if (daylight == n.param1 >> 4)
		daylight = env->findSunlight(pos);

	lua_pushinteger(L, dnr * daylight / 1000);
	return 1;
}

// place_node(pos, node)
// pos = {x=num, y=num, z=num}
int ModApiEnvMod::l_place_node(lua_State *L)
{
	GET_ENV_PTR;

	ScriptApiItem *scriptIfaceItem = getScriptApi<ScriptApiItem>(L);
	Server *server = getServer(L);
	const NodeDefManager *ndef = server->ndef();
	IItemDefManager *idef = server->idef();

	v3s16 pos = read_v3s16(L, 1);
	MapNode n = readnode(L, 2);

	// Don't attempt to load non-loaded area as of now
	MapNode n_old = env->getMap().getNode(pos);
	if(n_old.getContent() == CONTENT_IGNORE){
		lua_pushboolean(L, false);
		return 1;
	}
	// Create item to place
	Optional<ItemStack> item = ItemStack(ndef->get(n).name, 1, 0, idef);
	// Make pointed position
	PointedThing pointed;
	pointed.type = POINTEDTHING_NODE;
	pointed.node_abovesurface = pos;
	pointed.node_undersurface = pos + v3s16(0,-1,0);
	// Place it with a NULL placer (appears in Lua as nil)
	bool success = scriptIfaceItem->item_OnPlace(item, nullptr, pointed);
	lua_pushboolean(L, success);
	return 1;
}

// dig_node(pos)
// pos = {x=num, y=num, z=num}
int ModApiEnvMod::l_dig_node(lua_State *L)
{
	GET_ENV_PTR;

	ScriptApiNode *scriptIfaceNode = getScriptApi<ScriptApiNode>(L);

	v3s16 pos = read_v3s16(L, 1);

	// Don't attempt to load non-loaded area as of now
	MapNode n = env->getMap().getNode(pos);
	if(n.getContent() == CONTENT_IGNORE){
		lua_pushboolean(L, false);
		return 1;
	}
	// Dig it out with a NULL digger (appears in Lua as a
	// non-functional ObjectRef)
	bool success = scriptIfaceNode->node_on_dig(pos, n, NULL);
	lua_pushboolean(L, success);
	return 1;
}

// punch_node(pos)
// pos = {x=num, y=num, z=num}
int ModApiEnvMod::l_punch_node(lua_State *L)
{
	GET_ENV_PTR;

	ScriptApiNode *scriptIfaceNode = getScriptApi<ScriptApiNode>(L);

	v3s16 pos = read_v3s16(L, 1);

	// Don't attempt to load non-loaded area as of now
	MapNode n = env->getMap().getNode(pos);
	if(n.getContent() == CONTENT_IGNORE){
		lua_pushboolean(L, false);
		return 1;
	}
	// Punch it with a NULL puncher (appears in Lua as a non-functional
	// ObjectRef)
	bool success = scriptIfaceNode->node_on_punch(pos, n, NULL, PointedThing());
	lua_pushboolean(L, success);
	return 1;
}

// get_node_max_level(pos)
// pos = {x=num, y=num, z=num}
int ModApiEnvMod::l_get_node_max_level(lua_State *L)
{
	GET_PLAIN_ENV_PTR;

	v3s16 pos = read_v3s16(L, 1);
	MapNode n = env->getMap().getNode(pos);
	lua_pushnumber(L, n.getMaxLevel(env->getGameDef()->ndef()));
	return 1;
}

// get_node_level(pos)
// pos = {x=num, y=num, z=num}
int ModApiEnvMod::l_get_node_level(lua_State *L)
{
	GET_PLAIN_ENV_PTR;

	v3s16 pos = read_v3s16(L, 1);
	MapNode n = env->getMap().getNode(pos);
	lua_pushnumber(L, n.getLevel(env->getGameDef()->ndef()));
	return 1;
}

// set_node_level(pos, level)
// pos = {x=num, y=num, z=num}
// level: 0..63
int ModApiEnvMod::l_set_node_level(lua_State *L)
{
	GET_ENV_PTR;

	v3s16 pos = read_v3s16(L, 1);
	u8 level = 1;
	if(lua_isnumber(L, 2))
		level = lua_tonumber(L, 2);
	MapNode n = env->getMap().getNode(pos);
	lua_pushnumber(L, n.setLevel(env->getGameDef()->ndef(), level));
	env->setNode(pos, n);
	return 1;
}

// add_node_level(pos, level)
// pos = {x=num, y=num, z=num}
// level: -127..127
int ModApiEnvMod::l_add_node_level(lua_State *L)
{
	GET_ENV_PTR;

	v3s16 pos = read_v3s16(L, 1);
	s16 level = 1;
	if(lua_isnumber(L, 2))
		level = lua_tonumber(L, 2);
	MapNode n = env->getMap().getNode(pos);
	lua_pushnumber(L, n.addLevel(env->getGameDef()->ndef(), level));
	env->setNode(pos, n);
	return 1;
}

// find_nodes_with_meta(pos1, pos2)
int ModApiEnvMod::l_find_nodes_with_meta(lua_State *L)
{
	GET_PLAIN_ENV_PTR;

	std::vector<v3s16> positions = env->getMap().findNodesWithMetadata(
		check_v3s16(L, 1), check_v3s16(L, 2));

	lua_createtable(L, positions.size(), 0);
	for (size_t i = 0; i != positions.size(); i++) {
		push_v3s16(L, positions[i]);
		lua_rawseti(L, -2, i + 1);
	}

	return 1;
}

// get_meta(pos)
int ModApiEnvMod::l_get_meta(lua_State *L)
{
	GET_ENV_PTR;

	// Do it
	v3s16 p = read_v3s16(L, 1);
	NodeMetaRef::create(L, p, env);
	return 1;
}

// add_entity(pos, entityname, [staticdata]) -> ObjectRef or nil
// pos = {x=num, y=num, z=num}
int ModApiEnvMod::l_add_entity(lua_State *L)
{
	GET_ENV_PTR;

	v3f pos = checkFloatPos(L, 1);
	const char *name = luaL_checkstring(L, 2);
	std::string staticdata = readParam<std::string>(L, 3, "");

	ServerActiveObject *obj = new LuaEntitySAO(env, pos, name, staticdata);
	int objectid = env->addActiveObject(obj);
	// If failed to add, return nothing (reads as nil)
	if(objectid == 0)
		return 0;

	// If already deleted (can happen in on_activate), return nil
	if (obj->isGone())
		return 0;
	getScriptApiBase(L)->objectrefGetOrCreate(L, obj);
	return 1;
}

// add_item(pos, itemstack or itemstring or table) -> ObjectRef or nil
// pos = {x=num, y=num, z=num}
int ModApiEnvMod::l_add_item(lua_State *L)
{
	GET_ENV_PTR;

	// pos
	//v3f pos = checkFloatPos(L, 1);
	// item
	ItemStack item = read_item(L, 2,getServer(L)->idef());
	if(item.empty() || !item.isKnown(getServer(L)->idef()))
		return 0;

	int error_handler = PUSH_ERROR_HANDLER(L);

	// Use spawn_item to spawn a __builtin:item
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "spawn_item");
	lua_remove(L, -2); // Remove core
	if(lua_isnil(L, -1))
		return 0;
	lua_pushvalue(L, 1);
	lua_pushstring(L, item.getItemString().c_str());

	PCALL_RESL(L, lua_pcall(L, 2, 1, error_handler));

	lua_remove(L, error_handler);
	return 1;
}

// get_connected_players()
int ModApiEnvMod::l_get_connected_players(lua_State *L)
{
	ServerEnvironment *env = (ServerEnvironment *) getEnv(L);
	if (!env) {
		log_deprecated(L, "Calling get_connected_players() at mod load time"
				" is deprecated");
		lua_createtable(L, 0, 0);
		return 1;
	}

	lua_createtable(L, env->getPlayerCount(), 0);
	u32 i = 0;
	for (RemotePlayer *player : env->getPlayers()) {
		if (player->getPeerId() == PEER_ID_INEXISTENT)
			continue;
		PlayerSAO *sao = player->getPlayerSAO();
		if (sao && !sao->isGone()) {
			getScriptApiBase(L)->objectrefGetOrCreate(L, sao);
			lua_rawseti(L, -2, ++i);
		}
	}
	return 1;
}

// get_player_by_name(name)
int ModApiEnvMod::l_get_player_by_name(lua_State *L)
{
	GET_ENV_PTR;

	// Do it
	const char *name = luaL_checkstring(L, 1);
	RemotePlayer *player = env->getPlayer(name);
	if (!player || player->getPeerId() == PEER_ID_INEXISTENT)
		return 0;
	PlayerSAO *sao = player->getPlayerSAO();
	if (!sao || sao->isGone())
		return 0;
	// Put player on stack
	getScriptApiBase(L)->objectrefGetOrCreate(L, sao);
	return 1;
}

// get_objects_inside_radius(pos, radius)
int ModApiEnvMod::l_get_objects_inside_radius(lua_State *L)
{
	GET_ENV_PTR;
	ScriptApiBase *script = getScriptApiBase(L);

	// Do it
	v3f pos = checkFloatPos(L, 1);
	float radius = readParam<float>(L, 2) * BS;
	std::vector<ServerActiveObject *> objs;

	auto include_obj_cb = [](ServerActiveObject *obj){ return !obj->isGone(); };
	env->getObjectsInsideRadius(objs, pos, radius, include_obj_cb);

	int i = 0;
	lua_createtable(L, objs.size(), 0);
	for (const auto obj : objs) {
		// Insert object reference into table
		script->objectrefGetOrCreate(L, obj);
		lua_rawseti(L, -2, ++i);
	}
	return 1;
}

// get_objects_in_area(pos, minp, maxp)
int ModApiEnvMod::l_get_objects_in_area(lua_State *L)
{
	GET_ENV_PTR;
	ScriptApiBase *script = getScriptApiBase(L);

	v3f minp = read_v3f(L, 1) * BS;
	v3f maxp = read_v3f(L, 2) * BS;
	aabb3f box(minp, maxp);
	box.repair();
	std::vector<ServerActiveObject *> objs;

	auto include_obj_cb = [](ServerActiveObject *obj){ return !obj->isGone(); };
	env->getObjectsInArea(objs, box, include_obj_cb);

	int i = 0;
	lua_createtable(L, objs.size(), 0);
	for (const auto obj : objs) {
		// Insert object reference into table
		script->objectrefGetOrCreate(L, obj);
		lua_rawseti(L, -2, ++i);
	}
	return 1;
}

// set_timeofday(val)
// val = 0...1
int ModApiEnvMod::l_set_timeofday(lua_State *L)
{
	GET_ENV_PTR;

	// Do it
	float timeofday_f = readParam<float>(L, 1);
	luaL_argcheck(L, timeofday_f >= 0.0f && timeofday_f <= 1.0f, 1,
		"value must be between 0 and 1");
	int timeofday_mh = (int)(timeofday_f * 24000.0f);
	// This should be set directly in the environment but currently
	// such changes aren't immediately sent to the clients, so call
	// the server instead.
	//env->setTimeOfDay(timeofday_mh);
	getServer(L)->setTimeOfDay(timeofday_mh);
	return 0;
}

// get_timeofday() -> 0...1
int ModApiEnvMod::l_get_timeofday(lua_State *L)
{
	GET_PLAIN_ENV_PTR;

	// Do it
	int timeofday_mh = env->getTimeOfDay();
	float timeofday_f = (float)timeofday_mh / 24000.0f;
	lua_pushnumber(L, timeofday_f);
	return 1;
}

// get_day_count() -> int
int ModApiEnvMod::l_get_day_count(lua_State *L)
{
	GET_PLAIN_ENV_PTR;

	lua_pushnumber(L, env->getDayCount());
	return 1;
}

// get_gametime()
int ModApiEnvMod::l_get_gametime(lua_State *L)
{
	GET_ENV_PTR;

	int game_time = env->getGameTime();
	lua_pushnumber(L, game_time);
	return 1;
}

void ModApiEnvMod::collectNodeIds(lua_State *L, int idx, const NodeDefManager *ndef,
	std::vector<content_t> &filter)
{
	if (lua_istable(L, idx)) {
		lua_pushnil(L);
		while (lua_next(L, idx) != 0) {
			// key at index -2 and value at index -1
			luaL_checktype(L, -1, LUA_TSTRING);
			ndef->getIds(readParam<std::string>(L, -1), filter);
			// removes value, keeps key for next iteration
			lua_pop(L, 1);
		}
	} else if (lua_isstring(L, idx)) {
		ndef->getIds(readParam<std::string>(L, 3), filter);
	}
}

// find_node_near(pos, radius, nodenames, [search_center]) -> pos or nil
// nodenames: eg. {"ignore", "group:tree"} or "default:dirt"
int ModApiEnvMod::l_find_node_near(lua_State *L)
{
	GET_PLAIN_ENV_PTR;

	const NodeDefManager *ndef = env->getGameDef()->ndef();
	Map &map = env->getMap();

	v3s16 pos = read_v3s16(L, 1);
	int radius = luaL_checkinteger(L, 2);
	std::vector<content_t> filter;
	collectNodeIds(L, 3, ndef, filter);

	int start_radius = (lua_isboolean(L, 4) && readParam<bool>(L, 4)) ? 0 : 1;

#ifndef SERVER
	// Client API limitations
	if (Client *client = getClient(L))
		radius = client->CSMClampRadius(pos, radius);
#endif

	for (int d = start_radius; d <= radius; d++) {
		const std::vector<v3s16> &list = FacePositionCache::getFacePositions(d);
		for (const v3s16 &i : list) {
			v3s16 p = pos + i;
			content_t c = map.getNode(p).getContent();
			if (CONTAINS(filter, c)) {
				push_v3s16(L, p);
				return 1;
			}
		}
	}
	return 0;
}

static void checkArea(v3s16 &minp, v3s16 &maxp)
{
	auto volume = VoxelArea(minp, maxp).getVolume();
	// Volume limit equal to 8 default mapchunks, (80 * 2) ^ 3 = 4,096,000
	if (volume > 4096000) {
		throw LuaError("Area volume exceeds allowed value of 4096000");
	}

	// Clamp to map range to avoid problems
#define CLAMP(arg) core::clamp(arg, (s16)-MAX_MAP_GENERATION_LIMIT, (s16)MAX_MAP_GENERATION_LIMIT)
	minp = v3s16(CLAMP(minp.X), CLAMP(minp.Y), CLAMP(minp.Z));
	maxp = v3s16(CLAMP(maxp.X), CLAMP(maxp.Y), CLAMP(maxp.Z));
#undef CLAMP
}

// find_nodes_in_area(minp, maxp, nodenames, [grouped])
int ModApiEnvMod::l_find_nodes_in_area(lua_State *L)
{
	GET_PLAIN_ENV_PTR;

	v3s16 minp = read_v3s16(L, 1);
	v3s16 maxp = read_v3s16(L, 2);
	sortBoxVerticies(minp, maxp);

	const NodeDefManager *ndef = env->getGameDef()->ndef();
	Map &map = env->getMap();

#ifndef SERVER
	if (Client *client = getClient(L)) {
		minp = client->CSMClampPos(minp);
		maxp = client->CSMClampPos(maxp);
	}
#endif

	checkArea(minp, maxp);

	std::vector<content_t> filter;
	collectNodeIds(L, 3, ndef, filter);

	bool grouped = lua_isboolean(L, 4) && readParam<bool>(L, 4);

	if (grouped) {
		// create the table we will be returning
		lua_createtable(L, 0, filter.size());
		int base = lua_gettop(L);

		// create one table for each filter
		std::vector<u32> idx;
		idx.resize(filter.size());
		for (u32 i = 0; i < filter.size(); i++)
			lua_newtable(L);

		map.forEachNodeInArea(minp, maxp, [&](v3s16 p, MapNode n) -> bool {
			content_t c = n.getContent();

			auto it = std::find(filter.begin(), filter.end(), c);
			if (it != filter.end()) {
				// Calculate index of the table and append the position
				u32 filt_index = it - filter.begin();
				push_v3s16(L, p);
				lua_rawseti(L, base + 1 + filt_index, ++idx[filt_index]);
			}

			return true;
		});

		// last filter table is at top of stack
		u32 i = filter.size() - 1;
		do {
			if (idx[i] == 0) {
				// No such node found -> drop the empty table
				lua_pop(L, 1);
			} else {
				// This node was found -> put table into the return table
				lua_setfield(L, base, ndef->get(filter[i]).name.c_str());
			}
		} while (i-- != 0);

		assert(lua_gettop(L) == base);
		return 1;
	} else {
		std::vector<u32> individual_count;
		individual_count.resize(filter.size());

		lua_newtable(L);
		u32 i = 0;
		map.forEachNodeInArea(minp, maxp, [&](v3s16 p, MapNode n) -> bool {
			content_t c = n.getContent();

			auto it = std::find(filter.begin(), filter.end(), c);
			if (it != filter.end()) {
				push_v3s16(L, p);
				lua_rawseti(L, -2, ++i);

				u32 filt_index = it - filter.begin();
				individual_count[filt_index]++;
			}

			return true;
		});

		lua_createtable(L, 0, filter.size());
		for (u32 i = 0; i < filter.size(); i++) {
			lua_pushinteger(L, individual_count[i]);
			lua_setfield(L, -2, ndef->get(filter[i]).name.c_str());
		}
		return 2;
	}
}

// find_nodes_in_area_under_air(minp, maxp, nodenames) -> list of positions
// nodenames: e.g. {"ignore", "group:tree"} or "default:dirt"
int ModApiEnvMod::l_find_nodes_in_area_under_air(lua_State *L)
{
	/* Note: A similar but generalized (and therefore slower) version of this
	 * function could be created -- e.g. find_nodes_in_area_under -- which
	 * would accept a node name (or ID?) or list of names that the "above node"
	 * should be.
	 * TODO
	 */

	GET_PLAIN_ENV_PTR;

	v3s16 minp = read_v3s16(L, 1);
	v3s16 maxp = read_v3s16(L, 2);
	sortBoxVerticies(minp, maxp);

	const NodeDefManager *ndef = env->getGameDef()->ndef();
	Map &map = env->getMap();

#ifndef SERVER
	if (Client *client = getClient(L)) {
		minp = client->CSMClampPos(minp);
		maxp = client->CSMClampPos(maxp);
	}
#endif

	checkArea(minp, maxp);

	std::vector<content_t> filter;
	collectNodeIds(L, 3, ndef, filter);

	lua_newtable(L);
	u32 i = 0;
	v3s16 p;
	for (p.X = minp.X; p.X <= maxp.X; p.X++)
	for (p.Z = minp.Z; p.Z <= maxp.Z; p.Z++) {
		p.Y = minp.Y;
		content_t c = map.getNode(p).getContent();
		for (; p.Y <= maxp.Y; p.Y++) {
			v3s16 psurf(p.X, p.Y + 1, p.Z);
			content_t csurf = map.getNode(psurf).getContent();
			if (c != CONTENT_AIR && csurf == CONTENT_AIR &&
					CONTAINS(filter, c)) {
				push_v3s16(L, p);
				lua_rawseti(L, -2, ++i);
			}
			c = csurf;
		}
	}
	return 1;
}

// line_of_sight(pos1, pos2) -> true/false, pos
int ModApiEnvMod::l_line_of_sight(lua_State *L)
{
	GET_PLAIN_ENV_PTR;

	// read position 1 from lua
	v3f pos1 = checkFloatPos(L, 1);
	// read position 2 from lua
	v3f pos2 = checkFloatPos(L, 2);

	v3s16 p;

	bool success = env->line_of_sight(pos1, pos2, &p);
	lua_pushboolean(L, success);
	if (!success) {
		push_v3s16(L, p);
		return 2;
	}
	return 1;
}

int ModApiEnvMod::l_raycast(lua_State *L)
{
	return LuaRaycast::create_object(L);
}

void ModApiEnvMod::Initialize(lua_State *L, int top)
{
	API_FCT(set_node);
	API_FCT(bulk_set_node);
	API_FCT(add_node);
	API_FCT(swap_node);
	API_FCT(add_item);
	API_FCT(remove_node);
	API_FCT(get_node);
	API_FCT(get_node_or_nil);
	API_FCT(get_node_light);
	API_FCT(get_natural_light);
	API_FCT(place_node);
	API_FCT(dig_node);
	API_FCT(punch_node);
	API_FCT(get_node_max_level);
	API_FCT(get_node_level);
	API_FCT(set_node_level);
	API_FCT(add_node_level);
	API_FCT(add_entity);
	API_FCT(find_nodes_with_meta);
	API_FCT(get_meta);
	API_FCT(get_connected_players);
	API_FCT(get_player_by_name);
	API_FCT(get_objects_in_area);
	API_FCT(get_objects_inside_radius);
	API_FCT(set_timeofday);
	API_FCT(get_timeofday);
	API_FCT(get_gametime);
	API_FCT(get_day_count);
	API_FCT(find_node_near);
	API_FCT(find_nodes_in_area);
	API_FCT(find_nodes_in_area_under_air);
	API_FCT(line_of_sight);
	API_FCT(raycast);
}

void ModApiEnvMod::InitializeClient(lua_State *L, int top)
{
	API_FCT(get_node_light);
	API_FCT(get_timeofday);
	API_FCT(get_node_max_level);
	API_FCT(get_node_level);
	API_FCT(find_nodes_with_meta);
	API_FCT(find_node_near);
	API_FCT(find_nodes_in_area);
	API_FCT(find_nodes_in_area_under_air);
	API_FCT(line_of_sight);
	API_FCT(raycast);
}
