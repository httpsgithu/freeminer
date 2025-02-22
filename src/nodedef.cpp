/*
nodedef.cpp
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>
*/

/*
This file is part of Freeminer.

Freeminer is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Freeminer  is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Freeminer.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "nodedef.h"

#include "itemdef.h"
#ifndef SERVER
#include "client/tile.h"
#include "mesh.h"
#include <IMeshManipulator.h>
#endif
#include "log.h"
#include "settings.h"
#include "nameidmapping.h"
#include "util/numeric.h"
#include "util/serialize.h"
//#include "profiler.h" // For TimeTaker
#include "network/connection.h"
#include "shader.h"
#include "exceptions.h"
#include "debug.h"
#include "gamedef.h"
#include "mapnode.h"
#include <fstream> // Used in applyTextureOverrides()

/*
	NodeBox
*/

void NodeBox::reset()
{
	type = NODEBOX_REGULAR;
	// default is empty
	fixed.clear();
	// default is sign/ladder-like
	wall_top = aabb3f(-BS/2, BS/2-BS/16., -BS/2, BS/2, BS/2, BS/2);
	wall_bottom = aabb3f(-BS/2, -BS/2, -BS/2, BS/2, -BS/2+BS/16., BS/2);
	wall_side = aabb3f(-BS/2, -BS/2, -BS/2, -BS/2+BS/16., BS/2, BS/2);
	// no default for other parts
	connect_top.clear();
	connect_bottom.clear();
	connect_front.clear();
	connect_left.clear();
	connect_back.clear();
	connect_right.clear();
}

void NodeBox::serialize(std::ostream &os, u16 protocol_version) const
{
	int version = 1;
	if (protocol_version >= 27)
		version = 3;
	else if (protocol_version >= 21)
		version = 2;
	writeU8(os, version);

	switch (type) {
	case NODEBOX_LEVELED:
	case NODEBOX_FIXED:
		if (version == 1)
			writeU8(os, NODEBOX_FIXED);
		else
			writeU8(os, type);

		writeU16(os, fixed.size());
		for (std::vector<aabb3f>::const_iterator
				i = fixed.begin();
				i != fixed.end(); ++i)
		{
			writeV3F1000(os, i->MinEdge);
			writeV3F1000(os, i->MaxEdge);
		}
		break;
	case NODEBOX_WALLMOUNTED:
		writeU8(os, type);

		writeV3F1000(os, wall_top.MinEdge);
		writeV3F1000(os, wall_top.MaxEdge);
		writeV3F1000(os, wall_bottom.MinEdge);
		writeV3F1000(os, wall_bottom.MaxEdge);
		writeV3F1000(os, wall_side.MinEdge);
		writeV3F1000(os, wall_side.MaxEdge);
		break;
	case NODEBOX_CONNECTED:
		if (version <= 2) {
			// send old clients nodes that can't be walked through
			// to prevent abuse
			writeU8(os, NODEBOX_FIXED);

			writeU16(os, 1);
			writeV3F1000(os, v3f(-BS/2, -BS/2, -BS/2));
			writeV3F1000(os, v3f(BS/2, BS/2, BS/2));
		} else {
			writeU8(os, type);

#define WRITEBOX(box) do { \
		writeU16(os, (box).size()); \
		for (std::vector<aabb3f>::const_iterator \
				i = (box).begin(); \
				i != (box).end(); ++i) { \
			writeV3F1000(os, i->MinEdge); \
			writeV3F1000(os, i->MaxEdge); \
		}; } while (0)

			WRITEBOX(fixed);
			WRITEBOX(connect_top);
			WRITEBOX(connect_bottom);
			WRITEBOX(connect_front);
			WRITEBOX(connect_left);
			WRITEBOX(connect_back);
			WRITEBOX(connect_right);
		}
		break;
	default:
		writeU8(os, type);
		break;
	}
}

void NodeBox::deSerialize(std::istream &is)
{
	int version = readU8(is);
	if (version < 1 || version > 3)
		throw SerializationError("unsupported NodeBox version");

	reset();

	type = (enum NodeBoxType)readU8(is);

	if(type == NODEBOX_FIXED || type == NODEBOX_LEVELED)
	{
		u16 fixed_count = readU16(is);
		while(fixed_count--)
		{
			aabb3f box;
			box.MinEdge = readV3F1000(is);
			box.MaxEdge = readV3F1000(is);
			fixed.push_back(box);
		}
	}
	else if(type == NODEBOX_WALLMOUNTED)
	{
		wall_top.MinEdge = readV3F1000(is);
		wall_top.MaxEdge = readV3F1000(is);
		wall_bottom.MinEdge = readV3F1000(is);
		wall_bottom.MaxEdge = readV3F1000(is);
		wall_side.MinEdge = readV3F1000(is);
		wall_side.MaxEdge = readV3F1000(is);
	}
	else if (type == NODEBOX_CONNECTED)
	{
#define READBOXES(box) do { \
		count = readU16(is); \
		(box).reserve(count); \
		while (count--) { \
			v3f min = readV3F1000(is); \
			v3f max = readV3F1000(is); \
			(box).push_back(aabb3f(min, max)); }; } while (0)

		u16 count;

		READBOXES(fixed);
		READBOXES(connect_top);
		READBOXES(connect_bottom);
		READBOXES(connect_front);
		READBOXES(connect_left);
		READBOXES(connect_back);
		READBOXES(connect_right);
	}
}

void NodeBox::msgpack_pack(msgpack::packer<msgpack::sbuffer> &pk) const
{
	int map_size = 1;
	if (type == NODEBOX_FIXED || type == NODEBOX_LEVELED)
		map_size += 1;
	else if (type == NODEBOX_WALLMOUNTED)
		map_size += 3;
	else if (type == NODEBOX_CONNECTED)
		map_size += 7;

	pk.pack_map(map_size);
	PACK(NODEBOX_S_TYPE, (int)type);

	if (type == NODEBOX_FIXED || type == NODEBOX_LEVELED || type == NODEBOX_CONNECTED)
		PACK(NODEBOX_S_FIXED, fixed);

	if (type == NODEBOX_WALLMOUNTED) {
		PACK(NODEBOX_S_WALL_TOP, wall_top);
		PACK(NODEBOX_S_WALL_BOTTOM, wall_bottom);
		PACK(NODEBOX_S_WALL_SIDE, wall_side);
	} else if (type == NODEBOX_CONNECTED) {
		PACK(NODEBOX_S_CONNECTED_TOP, connect_top);       // 2
		PACK(NODEBOX_S_CONNECTED_BOTTOM, connect_bottom); // 3
		PACK(NODEBOX_S_CONNECTED_FRONT, connect_front);   // 4
		PACK(NODEBOX_S_CONNECTED_LEFT, connect_left);     // 5
		PACK(NODEBOX_S_CONNECTED_BACK, connect_back);     // 6
		PACK(NODEBOX_S_CONNECTED_RIGHT, connect_right);   // 7
	} else if (type != NODEBOX_REGULAR && type != NODEBOX_FIXED && type != NODEBOX_LEVELED){
		warningstream<< "Unknown nodebox type = "<< (int)type << std::endl;
	}
}

void NodeBox::msgpack_unpack(msgpack::object o)
{
	reset();

	MsgpackPacket packet = o.as<MsgpackPacket>();

	int type_tmp = packet[NODEBOX_S_TYPE].as<int>();
	type = (NodeBoxType)type_tmp;

	//if(type == NODEBOX_FIXED || type == NODEBOX_LEVELED)
	if (packet.count(NODEBOX_S_FIXED))
		packet[NODEBOX_S_FIXED].convert(fixed);

	if (type == NODEBOX_WALLMOUNTED) {
		packet[NODEBOX_S_WALL_TOP].convert(wall_top);
		packet[NODEBOX_S_WALL_BOTTOM].convert(wall_bottom);
		packet[NODEBOX_S_WALL_SIDE].convert(wall_side);
	} else if(type == NODEBOX_CONNECTED) {
		if (packet.count(NODEBOX_S_CONNECTED_TOP) && packet.count(NODEBOX_S_CONNECTED_RIGHT)) { //lite check
			packet[NODEBOX_S_CONNECTED_TOP].convert(connect_top);       // 2
			packet[NODEBOX_S_CONNECTED_BOTTOM].convert(connect_bottom); // 3
			packet[NODEBOX_S_CONNECTED_FRONT].convert(connect_front);   // 4
			packet[NODEBOX_S_CONNECTED_LEFT].convert(connect_left);     // 5
			packet[NODEBOX_S_CONNECTED_BACK].convert(connect_back);     // 6
			packet[NODEBOX_S_CONNECTED_RIGHT].convert(connect_right);   // 7
		}
	}

}

/*
	TileDef
*/

void TileDef::serialize(std::ostream &os, u16 protocol_version) const
{
	if (protocol_version >= 26)
		writeU8(os, 2);
	else if (protocol_version >= 17)
		writeU8(os, 1);
	else
		writeU8(os, 0);
	os<<serializeString(name);
	writeU8(os, animation.type);
	writeU16(os, animation.aspect_w);
	writeU16(os, animation.aspect_h);
	writeF1000(os, animation.length);
	if (protocol_version >= 17)
		writeU8(os, backface_culling);
	if (protocol_version >= 26) {
		writeU8(os, tileable_horizontal);
		writeU8(os, tileable_vertical);
	}
}

void TileDef::deSerialize(std::istream &is, const u8 contenfeatures_version, const NodeDrawType drawtype)
{
	int version = readU8(is);
	name = deSerializeString(is);
	animation.type = (TileAnimationType)readU8(is);
	animation.aspect_w = readU16(is);
	animation.aspect_h = readU16(is);
	animation.length = readF1000(is);
	if (version >= 1)
		backface_culling = readU8(is);
	if (version >= 2) {
		tileable_horizontal = readU8(is);
		tileable_vertical = readU8(is);
	}

	if ((contenfeatures_version < 8) &&
		((drawtype == NDT_MESH) ||
		 (drawtype == NDT_FIRELIKE) ||
		 (drawtype == NDT_LIQUID) ||
		 (drawtype == NDT_PLANTLIKE)))
		backface_culling = false;
}

void TileDef::msgpack_pack(msgpack::packer<msgpack::sbuffer> &pk) const
{
	pk.pack_map(8);
	PACK(TILEDEF_NAME, name);
	PACK(TILEDEF_ANIMATION_TYPE, (int)animation.type);
	PACK(TILEDEF_ANIMATION_ASPECT_W, animation.aspect_w);
	PACK(TILEDEF_ANIMATION_ASPECT_H, animation.aspect_h);
	PACK(TILEDEF_ANIMATION_LENGTH, animation.length);
	PACK(TILEDEF_BACKFACE_CULLING, backface_culling);
	PACK(TILEDEF_TILEABLE_VERTICAL, tileable_vertical);
	PACK(TILEDEF_TILEABLE_HORIZONTAL, tileable_horizontal);
}

void TileDef::msgpack_unpack(msgpack::object o)
{
	MsgpackPacket packet = o.as<MsgpackPacket>();
	packet[TILEDEF_NAME].convert(name);

	int type_tmp;
	packet[TILEDEF_ANIMATION_TYPE].convert(type_tmp);
	animation.type = (TileAnimationType)type_tmp;

	packet[TILEDEF_ANIMATION_ASPECT_W].convert(animation.aspect_w);
	packet[TILEDEF_ANIMATION_ASPECT_H].convert(animation.aspect_h);
	packet[TILEDEF_ANIMATION_LENGTH].convert(animation.length);
	packet[TILEDEF_BACKFACE_CULLING].convert(backface_culling);
	packet_convert_safe(packet, TILEDEF_TILEABLE_VERTICAL, tileable_vertical);
	packet_convert_safe(packet, TILEDEF_TILEABLE_HORIZONTAL, tileable_horizontal);
}

/*
	SimpleSoundSpec serialization
*/

static void serializeSimpleSoundSpec(const SimpleSoundSpec &ss,
		std::ostream &os)
{
	os<<serializeString(ss.name);
	writeF1000(os, ss.gain);
}
static void deSerializeSimpleSoundSpec(SimpleSoundSpec &ss, std::istream &is)
{
	ss.name = deSerializeString(is);
	ss.gain = readF1000(is);
}

void TextureSettings::readSettings()
{
	connected_glass                = g_settings->getBool("connected_glass");
	opaque_water                   = g_settings->getBool("opaque_water");
	bool enable_shaders            = g_settings->getBool("enable_shaders");
	bool enable_bumpmapping        = g_settings->getBool("enable_bumpmapping");
	bool enable_parallax_occlusion = g_settings->getBool("enable_parallax_occlusion");
	enable_mesh_cache              = g_settings->getBool("enable_mesh_cache");
	enable_minimap                 = g_settings->getBool("enable_minimap");
	std::string leaves_style_str   = g_settings->get("leaves_style");

	use_normal_texture = enable_shaders &&
		(enable_bumpmapping || enable_parallax_occlusion);
	if (leaves_style_str == "fancy") {
		leaves_style = LEAVES_FANCY;
	} else if (leaves_style_str == "simple") {
		leaves_style = LEAVES_SIMPLE;
	} else {
		leaves_style = LEAVES_OPAQUE;
	}
}

/*
	ContentFeatures
*/

ContentFeatures::ContentFeatures()
{
	reset();
}

ContentFeatures::~ContentFeatures()
{
}

void ContentFeatures::reset()
{
	/*
		Cached stuff
	*/
//#ifndef SERVER
	solidness = 2;
	visual_solidness = 0;
	backface_culling = true;

//#endif
	has_on_construct = false;
	has_on_destruct = false;
	has_after_destruct = false;
	has_on_activate = false;
	has_on_deactivate = false;
	/*
		Actual data

		NOTE: Most of this is always overridden by the default values given
		      in builtin.lua
	*/
	name = "";
	groups.clear();
	// Unknown nodes can be dug
	groups["dig_immediate"] = 2;
	drawtype = NDT_NORMAL;
	mesh = "";
#ifndef SERVER
	for(u32 i = 0; i < 24; i++)
		mesh_ptr[i] = NULL;
	minimap_color = video::SColor(0, 0, 0, 0);
#endif
	visual_scale = 1.0;
	for(u32 i = 0; i < 6; i++)
		tiledef[i] = TileDef();
	for(u16 j = 0; j < CF_SPECIAL_COUNT; j++)
		tiledef_special[j] = TileDef();
	alpha = 255;
	post_effect_color = video::SColor(0, 0, 0, 0);
	param_type = CPT_NONE;
	param_type_2 = CPT2_NONE;
	is_ground_content = false;
	light_propagates = false;
	sunlight_propagates = false;
	walkable = true;
	pointable = true;
	diggable = true;
	climbable = false;
	buildable_to = false;
	floodable = false;
	rightclickable = true;
	leveled = 0;
	liquid_type = LIQUID_NONE;
	liquid_alternative_flowing = "";
	liquid_alternative_source = "";
	liquid_viscosity = 0;
	liquid_renewable = true;
	liquid_range = LIQUID_LEVEL_MAX+1;
	drowning = 0;
	light_source = 0;
	damage_per_second = 0;
	node_box = NodeBox();
	selection_box = NodeBox();
	collision_box = NodeBox();
	waving = 0;
	legacy_facedir_simple = false;
	legacy_wallmounted = false;
	sound_footstep = SimpleSoundSpec();
	sound_dig = SimpleSoundSpec("__group");
	sound_dug = SimpleSoundSpec();


//freeminer:
	solidness_far = 0;

	freeze = "";
	melt = "";
	is_circuit_element = false;
	is_wire = false;
	is_wire_connector = false;
	for(int i = 0; i < 6; ++i)
	{
		wire_connections[i] = 0;
	}
	for(int i = 0; i < 64; ++i)
	{
		circuit_element_func[i] = 0;
	}
	circuit_element_delay = 0;


	connects_to.clear();
	connects_to_ids.clear();
	connect_sides = 0;
}

void ContentFeatures::serialize(std::ostream &os, u16 protocol_version) const
{
	if(protocol_version < 24){
		//serializeOld(os, protocol_version);
		return;
	}

	writeU8(os, protocol_version < 27 ? 7 : 8);

	os<<serializeString(name);
	writeU16(os, groups.size());
	for(ItemGroupList::const_iterator
			i = groups.begin(); i != groups.end(); ++i){
		os<<serializeString(i->first);
		writeS16(os, i->second);
	}
	writeU8(os, drawtype);
	writeF1000(os, visual_scale);
	writeU8(os, 6);
	for(u32 i = 0; i < 6; i++)
		tiledef[i].serialize(os, protocol_version);
	writeU8(os, CF_SPECIAL_COUNT);
	for(u32 i = 0; i < CF_SPECIAL_COUNT; i++){
		tiledef_special[i].serialize(os, protocol_version);
	}
	writeU8(os, alpha);
	writeU8(os, post_effect_color.getAlpha());
	writeU8(os, post_effect_color.getRed());
	writeU8(os, post_effect_color.getGreen());
	writeU8(os, post_effect_color.getBlue());
	writeU8(os, param_type);
	if ((protocol_version < 28) && (param_type_2 == CPT2_MESHOPTIONS))
		writeU8(os, CPT2_NONE);
	else
		writeU8(os, param_type_2);
	writeU8(os, is_ground_content);
	writeU8(os, light_propagates);
	writeU8(os, sunlight_propagates);
	writeU8(os, walkable);
	writeU8(os, pointable);
	writeU8(os, diggable);
	writeU8(os, climbable);
	writeU8(os, buildable_to);
	os<<serializeString(""); // legacy: used to be metadata_name
	writeU8(os, liquid_type);
	os<<serializeString(liquid_alternative_flowing);
	os<<serializeString(liquid_alternative_source);
	writeU8(os, liquid_viscosity);
	writeU8(os, liquid_renewable);
	writeU8(os, light_source);
	writeU32(os, damage_per_second);
	node_box.serialize(os, protocol_version);
	selection_box.serialize(os, protocol_version);
	writeU8(os, legacy_facedir_simple);
	writeU8(os, legacy_wallmounted);
	serializeSimpleSoundSpec(sound_footstep, os);
	serializeSimpleSoundSpec(sound_dig, os);
	serializeSimpleSoundSpec(sound_dug, os);
	writeU8(os, rightclickable);
	writeU8(os, drowning);
	writeU8(os, leveled);
	writeU8(os, liquid_range);
	writeU8(os, waving);
	// Stuff below should be moved to correct place in a version that otherwise changes
	// the protocol version
	os<<serializeString(mesh);
	collision_box.serialize(os, protocol_version);
	writeU8(os, floodable);
	writeU16(os, connects_to_ids.size());
	for (auto i = connects_to_ids.begin();
			i != connects_to_ids.end(); ++i)
		writeU16(os, *i);
	writeU8(os, connect_sides);
}

void ContentFeatures::deSerialize(std::istream &is)
{
	int version = readU8(is);
	if (version < 7) {
		//deSerializeOld(is, version);
		return;
	} else if (version > 8) {
		throw SerializationError("unsupported ContentFeatures version");
	}
	name = deSerializeString(is);
	groups.clear();
	u32 groups_size = readU16(is);
	for(u32 i = 0; i < groups_size; i++){
		std::string name = deSerializeString(is);
		int value = readS16(is);
		groups[name] = value;
	}
	drawtype = (enum NodeDrawType)readU8(is);

	visual_scale = readF1000(is);
	if(readU8(is) != 6)
		throw SerializationError("unsupported tile count");
	for(u32 i = 0; i < 6; i++)
		tiledef[i].deSerialize(is, version, drawtype);
	if(readU8(is) != CF_SPECIAL_COUNT)
		throw SerializationError("unsupported CF_SPECIAL_COUNT");
	for(u32 i = 0; i < CF_SPECIAL_COUNT; i++)
		tiledef_special[i].deSerialize(is, version, drawtype);
	alpha = readU8(is);
	post_effect_color.setAlpha(readU8(is));
	post_effect_color.setRed(readU8(is));
	post_effect_color.setGreen(readU8(is));
	post_effect_color.setBlue(readU8(is));
	param_type = (enum ContentParamType)readU8(is);
	param_type_2 = (enum ContentParamType2)readU8(is);
	is_ground_content = readU8(is);
	light_propagates = readU8(is);
	sunlight_propagates = readU8(is);
	walkable = readU8(is);
	pointable = readU8(is);
	diggable = readU8(is);
	climbable = readU8(is);
	buildable_to = readU8(is);
	deSerializeString(is); // legacy: used to be metadata_name
	liquid_type = (enum LiquidType)readU8(is);
	liquid_alternative_flowing = deSerializeString(is);
	liquid_alternative_source = deSerializeString(is);
	liquid_viscosity = readU8(is);
	liquid_renewable = readU8(is);
	light_source = readU8(is);
	light_source = MYMIN(light_source, LIGHT_MAX);
	damage_per_second = readU32(is);
	node_box.deSerialize(is);
	selection_box.deSerialize(is);
	legacy_facedir_simple = readU8(is);
	legacy_wallmounted = readU8(is);
	deSerializeSimpleSoundSpec(sound_footstep, is);
	deSerializeSimpleSoundSpec(sound_dig, is);
	deSerializeSimpleSoundSpec(sound_dug, is);
	rightclickable = readU8(is);
	drowning = readU8(is);
	leveled = readU8(is);
	liquid_range = readU8(is);
	waving = readU8(is);
	// If you add anything here, insert it primarily inside the try-catch
	// block to not need to increase the version.
	try{
		// Stuff below should be moved to correct place in a version that
		// otherwise changes the protocol version
	mesh = deSerializeString(is);
	collision_box.deSerialize(is);
	floodable = readU8(is);
	u16 connects_to_size = readU16(is);
	connects_to_ids.clear();
	for (u16 i = 0; i < connects_to_size; i++)
		connects_to_ids.insert(readU16(is));
	connect_sides = readU8(is);
	}catch(SerializationError &e) {};
}

void ContentFeatures::msgpack_pack(msgpack::packer<msgpack::sbuffer> &pk) const
{
	pk.pack_map(40);
	PACK(CONTENTFEATURES_NAME, name);
	PACK(CONTENTFEATURES_GROUPS, groups);
	PACK(CONTENTFEATURES_DRAWTYPE, (int)drawtype);
	PACK(CONTENTFEATURES_VISUAL_SCALE, visual_scale);

	pk.pack((int)CONTENTFEATURES_TILEDEF);
	pk.pack_array(6);
	for (size_t i = 0; i < 6; ++i)
		pk.pack(tiledef[i]);

	pk.pack((int)CONTENTFEATURES_TILEDEF_SPECIAL);
	pk.pack_array(CF_SPECIAL_COUNT);
	for (size_t i = 0; i < CF_SPECIAL_COUNT; ++i)
		pk.pack(tiledef_special[i]);

	PACK(CONTENTFEATURES_ALPHA, alpha);
	PACK(CONTENTFEATURES_POST_EFFECT_COLOR, post_effect_color);
	PACK(CONTENTFEATURES_PARAM_TYPE, (int)param_type);
	PACK(CONTENTFEATURES_PARAM_TYPE_2, (int)param_type_2);
	PACK(CONTENTFEATURES_IS_GROUND_CONTENT, is_ground_content);
	PACK(CONTENTFEATURES_LIGHT_PROPAGATES, light_propagates);
	PACK(CONTENTFEATURES_SUNLIGHT_PROPAGATES, sunlight_propagates);
	PACK(CONTENTFEATURES_WALKABLE, walkable);
	PACK(CONTENTFEATURES_POINTABLE, pointable);
	PACK(CONTENTFEATURES_DIGGABLE, diggable);
	PACK(CONTENTFEATURES_CLIMBABLE, climbable);
	PACK(CONTENTFEATURES_BUILDABLE_TO, buildable_to);
	PACK(CONTENTFEATURES_LIQUID_TYPE, (int)liquid_type);
	PACK(CONTENTFEATURES_LIQUID_ALTERNATIVE_FLOWING, liquid_alternative_flowing);
	PACK(CONTENTFEATURES_LIQUID_ALTERNATIVE_SOURCE, liquid_alternative_source);
	PACK(CONTENTFEATURES_LIQUID_VISCOSITY, liquid_viscosity);
	PACK(CONTENTFEATURES_LIQUID_RENEWABLE, liquid_renewable);
	PACK(CONTENTFEATURES_LIGHT_SOURCE, light_source);
	PACK(CONTENTFEATURES_DAMAGE_PER_SECOND, damage_per_second);
	PACK(CONTENTFEATURES_NODE_BOX, node_box);
	PACK(CONTENTFEATURES_SELECTION_BOX, selection_box);
	PACK(CONTENTFEATURES_LEGACY_FACEDIR_SIMPLE, legacy_facedir_simple);
	PACK(CONTENTFEATURES_LEGACY_WALLMOUNTED, legacy_wallmounted);
	PACK(CONTENTFEATURES_SOUND_FOOTSTEP, sound_footstep);
	PACK(CONTENTFEATURES_SOUND_DIG, sound_dig);
	PACK(CONTENTFEATURES_SOUND_DUG, sound_dug);
	PACK(CONTENTFEATURES_RIGHTCLICKABLE, rightclickable);
	PACK(CONTENTFEATURES_DROWNING, drowning);
	PACK(CONTENTFEATURES_LEVELED, leveled);
	PACK(CONTENTFEATURES_WAVING, waving);
	PACK(CONTENTFEATURES_MESH, mesh);
	PACK(CONTENTFEATURES_COLLISION_BOX, collision_box);

	// PACK(CONTENTFEATURES_FLOODABLE, floodable); //not used on client

	PACK(CONTENTFEATURES_CONNECT_TO_IDS, connects_to_ids);
	PACK(CONTENTFEATURES_CONNECT_SIDES, connect_sides);
}

void ContentFeatures::msgpack_unpack(msgpack::object o)
{
	MsgpackPacket packet = o.as<MsgpackPacket>();
	packet[CONTENTFEATURES_NAME].convert(name);
	groups.clear();
	packet[CONTENTFEATURES_GROUPS].convert(groups);

	int drawtype_tmp;
	packet[CONTENTFEATURES_DRAWTYPE].convert(drawtype_tmp);
	drawtype = (NodeDrawType)drawtype_tmp;

	packet[CONTENTFEATURES_VISUAL_SCALE].convert(visual_scale);

	std::vector<TileDef> tiledef_received;
	packet[CONTENTFEATURES_TILEDEF].convert(tiledef_received);
	if (tiledef_received.size() != 6)
		throw SerializationError("unsupported tile count");
	for(size_t i = 0; i < 6; ++i)
		tiledef[i] = tiledef_received[i];

	std::vector<TileDef> tiledef_special_received;
	packet[CONTENTFEATURES_TILEDEF_SPECIAL].convert(tiledef_special_received);
	if(tiledef_special_received.size() != CF_SPECIAL_COUNT)
		throw SerializationError("unsupported CF_SPECIAL_COUNT");
	for (size_t i = 0; i < CF_SPECIAL_COUNT; ++i)
		tiledef_special[i] = tiledef_special_received[i];

	packet[CONTENTFEATURES_ALPHA].convert(alpha);
	packet[CONTENTFEATURES_POST_EFFECT_COLOR].convert(post_effect_color);

	int param_type_tmp;
	packet[CONTENTFEATURES_PARAM_TYPE].convert(param_type_tmp);
	param_type = (ContentParamType)param_type_tmp;
	packet[CONTENTFEATURES_PARAM_TYPE_2].convert(param_type_tmp);
	param_type_2 = (ContentParamType2)param_type_tmp;

	packet[CONTENTFEATURES_IS_GROUND_CONTENT].convert(is_ground_content);
	packet[CONTENTFEATURES_LIGHT_PROPAGATES].convert(light_propagates);
	packet[CONTENTFEATURES_SUNLIGHT_PROPAGATES].convert(sunlight_propagates);
	packet[CONTENTFEATURES_WALKABLE].convert(walkable);
	packet[CONTENTFEATURES_POINTABLE].convert(pointable);
	packet[CONTENTFEATURES_DIGGABLE].convert(diggable);
	packet[CONTENTFEATURES_CLIMBABLE].convert(climbable);
	packet[CONTENTFEATURES_BUILDABLE_TO].convert(buildable_to);

	int liquid_type_tmp;
	packet[CONTENTFEATURES_LIQUID_TYPE].convert(liquid_type_tmp);
	liquid_type = (LiquidType)liquid_type_tmp;

	packet[CONTENTFEATURES_LIQUID_ALTERNATIVE_FLOWING].convert(liquid_alternative_flowing);
	packet[CONTENTFEATURES_LIQUID_ALTERNATIVE_SOURCE].convert(liquid_alternative_source);
	packet[CONTENTFEATURES_LIQUID_VISCOSITY].convert(liquid_viscosity);
	packet[CONTENTFEATURES_LIGHT_SOURCE].convert(light_source);
	packet[CONTENTFEATURES_DAMAGE_PER_SECOND].convert(damage_per_second);
	packet[CONTENTFEATURES_NODE_BOX].convert(node_box);
	packet[CONTENTFEATURES_SELECTION_BOX].convert(selection_box);
	packet[CONTENTFEATURES_LEGACY_FACEDIR_SIMPLE].convert(legacy_facedir_simple);
	packet[CONTENTFEATURES_LEGACY_WALLMOUNTED].convert(legacy_wallmounted);
	packet[CONTENTFEATURES_SOUND_FOOTSTEP].convert(sound_footstep);
	packet[CONTENTFEATURES_SOUND_DIG].convert(sound_dig);
	packet[CONTENTFEATURES_SOUND_DUG].convert(sound_dug);
	packet[CONTENTFEATURES_RIGHTCLICKABLE].convert(rightclickable);
	packet[CONTENTFEATURES_DROWNING].convert(drowning);
	packet[CONTENTFEATURES_LEVELED].convert(leveled);
	packet[CONTENTFEATURES_WAVING].convert(waving);
	packet[CONTENTFEATURES_MESH].convert(mesh);
	packet[CONTENTFEATURES_COLLISION_BOX].convert(collision_box);

	if(packet.count(CONTENTFEATURES_CONNECT_TO_IDS))
		packet[CONTENTFEATURES_CONNECT_TO_IDS].convert(connects_to_ids);
	if(packet.count(CONTENTFEATURES_CONNECT_SIDES))
		packet[CONTENTFEATURES_CONNECT_SIDES].convert(connect_sides);

}

#ifndef SERVER
void ContentFeatures::fillTileAttribs(ITextureSource *tsrc, TileSpec *tile,
		TileDef *tiledef, u32 shader_id, bool use_normal_texture,
		bool backface_culling, u8 alpha, u8 material_type)
{
	tile->shader_id     = shader_id;
	tile->texture       = tsrc->getTextureForMesh(tiledef->name, &tile->texture_id);
	tile->alpha         = alpha;
	tile->material_type = material_type;

	// Normal texture and shader flags texture
	if (use_normal_texture) {
		tile->normal_texture = tsrc->getNormalTexture(tiledef->name);
	}
	tile->flags_texture = tsrc->getShaderFlagsTexture(tile->normal_texture ? true : false);

	// Material flags
	tile->material_flags = 0;
	if (backface_culling)
		tile->material_flags |= MATERIAL_FLAG_BACKFACE_CULLING;
	if (tiledef->animation.type == TAT_VERTICAL_FRAMES)
		tile->material_flags |= MATERIAL_FLAG_ANIMATION_VERTICAL_FRAMES;
	if (tiledef->tileable_horizontal)
		tile->material_flags |= MATERIAL_FLAG_TILEABLE_HORIZONTAL;
	if (tiledef->tileable_vertical)
		tile->material_flags |= MATERIAL_FLAG_TILEABLE_VERTICAL;

	// Animation parameters
	int frame_count = 1;
	if (tile->material_flags & MATERIAL_FLAG_ANIMATION_VERTICAL_FRAMES) {
		// Get texture size to determine frame count by aspect ratio
		v2u32 size = tile->texture->getOriginalSize();
		int frame_height = (float)size.X /
				(tiledef->animation.aspect_w ? (float)tiledef->animation.aspect_w : 1) *
				(tiledef->animation.aspect_h ? (float)tiledef->animation.aspect_h : 1);
		frame_count = size.Y / (frame_height ? frame_height : size.Y ? size.Y : 1);
		int frame_length_ms = 1000.0 * tiledef->animation.length / frame_count;
		tile->animation_frame_count = frame_count;
		tile->animation_frame_length_ms = frame_length_ms;
	}

	if (frame_count == 1) {
		tile->material_flags &= ~MATERIAL_FLAG_ANIMATION_VERTICAL_FRAMES;
	} else {
		std::ostringstream os(std::ios::binary);
		tile->frames.resize(frame_count);

		for (int i = 0; i < frame_count; i++) {

			FrameSpec frame;

			os.str("");
			os << tiledef->name << "^[verticalframe:"
				<< frame_count << ":" << i;

			frame.texture = tsrc->getTextureForMesh(os.str(), &frame.texture_id);
			if (tile->normal_texture)
				frame.normal_texture = tsrc->getNormalTexture(os.str());
			frame.flags_texture = tile->flags_texture;
			tile->frames[i] = frame;
		}
	}
}
#endif

/*
#ifndef SERVER
*/
void ContentFeatures::updateTextures(ITextureSource *tsrc, IShaderSource *shdsrc,
	scene::ISceneManager *smgr, scene::IMeshManipulator *meshmanip,
	IGameDef *gamedef, const TextureSettings &tsettings,
	bool server
	)
{
#ifndef SERVER
	// minimap pixel color - the average color of a texture
	if (tsrc)
	if (tsettings.enable_minimap && tiledef[0].name != "")
		minimap_color = tsrc->getTextureAverageColor(tiledef[0].name);
#endif

	// Figure out the actual tiles to use
	TileDef tdef[6];
	for (u32 j = 0; j < 6; j++) {
		tdef[j] = tiledef[j];
		if (tdef[j].name == "")
			tdef[j].name = "unknown_node.png";
	}

	bool is_liquid = false;
	bool is_water_surface = false;

	u8 material_type = (alpha == 255) ?
		TILE_MATERIAL_BASIC : TILE_MATERIAL_ALPHA;

	switch (drawtype) {
	default:
	case NDT_NORMAL:
		solidness = 2;
		break;
	case NDT_AIRLIKE:
		solidness = 0;
		break;
	case NDT_LIQUID:
		//assert(liquid_type == LIQUID_SOURCE);
		if (tsettings.opaque_water)
			alpha = 255;
		solidness = 1;
		is_liquid = true;
		break;
	case NDT_FLOWINGLIQUID:
		//assert(liquid_type == LIQUID_FLOWING);
		solidness = 0;
		if (tsettings.opaque_water)
			alpha = 255;
		is_liquid = true;
		break;
	case NDT_GLASSLIKE:
		solidness_far = 1;
		solidness = 0;
		visual_solidness = 1;
		break;
	case NDT_GLASSLIKE_FRAMED:
		solidness_far = 1;
		solidness = 0;
		visual_solidness = 1;
		break;
	case NDT_GLASSLIKE_FRAMED_OPTIONAL:
		solidness_far = 1;
		solidness = 0;
		visual_solidness = 1;
		if (!server)
		drawtype = tsettings.connected_glass ? NDT_GLASSLIKE_FRAMED : NDT_GLASSLIKE;
		break;
	case NDT_ALLFACES:
		solidness_far = 1;
		solidness = 0;
		visual_solidness = 1;
		break;
	case NDT_ALLFACES_OPTIONAL:
		if (tsettings.leaves_style == LEAVES_FANCY) {
			if (!server)
			drawtype = NDT_ALLFACES;
			solidness = 0;
			visual_solidness = 1;
		} else if (tsettings.leaves_style == LEAVES_SIMPLE) {
			for (u32 j = 0; j < 6; j++) {
				if (tiledef_special[j].name != "")
					tdef[j].name = tiledef_special[j].name;
			}
			if (!server)
			drawtype = NDT_GLASSLIKE;
			solidness = 0;
			visual_solidness = 1;
		} else {
			if (!server)
			drawtype = NDT_NORMAL;
			solidness = 2;
			for (u32 i = 0; i < 6; i++)
				tdef[i].name += std::string("^[noalpha");
		}
		if (waving == 1)
			material_type = TILE_MATERIAL_WAVING_LEAVES;
		solidness_far = 1;
		break;
	case NDT_PLANTLIKE:
		solidness = 0;
		if (waving == 1)
			material_type = TILE_MATERIAL_WAVING_PLANTS;
		break;
	case NDT_FIRELIKE:
		solidness = 0;
		break;
	case NDT_MESH:
		solidness = 0;
		break;
	case NDT_TORCHLIKE:
	case NDT_SIGNLIKE:
	case NDT_FENCELIKE:
	case NDT_RAILLIKE:
	case NDT_NODEBOX:
		solidness = 0;
		break;
	}

	if (drawtype == NDT_NODEBOX)
		solidness_far = 1;

#ifndef SERVER
	if (is_liquid) {
		material_type = (alpha == 255) ?
			TILE_MATERIAL_LIQUID_OPAQUE : TILE_MATERIAL_LIQUID_TRANSPARENT;
		if (name == "default:water_source")
			is_water_surface = true;
	}

	u32 tile_shader[6];
	if (shdsrc) {
	for (u16 j = 0; j < 6; j++) {
		tile_shader[j] = shdsrc->getShader("nodes_shader",
			material_type, drawtype);
	}

	if (is_water_surface) {
		tile_shader[0] = shdsrc->getShader("water_surface_shader",
			material_type, drawtype);
	}
	}

	if (tsrc) {
	// Tiles (fill in f->tiles[])
	for (u16 j = 0; j < 6; j++) {
		fillTileAttribs(tsrc, &tiles[j], &tdef[j], tile_shader[j],
			tsettings.use_normal_texture,
			tiledef[j].backface_culling, alpha, material_type);
	}

	// Special tiles (fill in f->special_tiles[])
	for (u16 j = 0; j < CF_SPECIAL_COUNT; j++) {
		fillTileAttribs(tsrc, &special_tiles[j], &tiledef_special[j],
			tile_shader[j], tsettings.use_normal_texture,
			tiledef_special[j].backface_culling, alpha, material_type);
	}
	}

	if ((drawtype == NDT_MESH) && (mesh != "")) {
	  if (gamedef) {
		// Meshnode drawtype
		// Read the mesh and apply scale
		mesh_ptr[0] = gamedef->getMesh(mesh);
		if (mesh_ptr[0]){
			v3f scale = v3f(1.0, 1.0, 1.0) * BS * visual_scale;
			scaleMesh(mesh_ptr[0], scale);
			recalculateBoundingBox(mesh_ptr[0]);
			meshmanip->recalculateNormals(mesh_ptr[0], true, false);
		}
	  }
	} else if ((drawtype == NDT_NODEBOX) &&
			((node_box.type == NODEBOX_REGULAR) ||
			(node_box.type == NODEBOX_FIXED)) &&
			(!node_box.fixed.empty())) {
		//Convert regular nodebox nodes to meshnodes
		//Change the drawtype and apply scale
		if (!server)
		drawtype = NDT_MESH;
		mesh_ptr[0] = convertNodeboxesToMesh(node_box.fixed);
		v3f scale = v3f(1.0, 1.0, 1.0) * visual_scale;
		scaleMesh(mesh_ptr[0], scale);
		recalculateBoundingBox(mesh_ptr[0]);
		meshmanip->recalculateNormals(mesh_ptr[0], true, false);
	}

	//Cache 6dfacedir and wallmounted rotated clones of meshes
	if (tsettings.enable_mesh_cache && mesh_ptr[0] && (param_type_2 == CPT2_FACEDIR)) {
		for (u16 j = 1; j < 24; j++) {
			mesh_ptr[j] = cloneMesh(mesh_ptr[0]);
			rotateMeshBy6dFacedir(mesh_ptr[j], j);
			recalculateBoundingBox(mesh_ptr[j]);
			meshmanip->recalculateNormals(mesh_ptr[j], true, false);
		}
	} else if (tsettings.enable_mesh_cache && mesh_ptr[0] && (param_type_2 == CPT2_WALLMOUNTED)) {
		static const u8 wm_to_6d[6] = {20, 0, 16+1, 12+3, 8, 4+2};
		for (u16 j = 1; j < 6; j++) {
			mesh_ptr[j] = cloneMesh(mesh_ptr[0]);
			rotateMeshBy6dFacedir(mesh_ptr[j], wm_to_6d[j]);
			recalculateBoundingBox(mesh_ptr[j]);
			meshmanip->recalculateNormals(mesh_ptr[j], true, false);
		}
		rotateMeshBy6dFacedir(mesh_ptr[0], wm_to_6d[0]);
		recalculateBoundingBox(mesh_ptr[0]);
		meshmanip->recalculateNormals(mesh_ptr[0], true, false);
	}
#endif
}
/*
#endif
*/

/*
	CNodeDefManager
*/

class CNodeDefManager: public IWritableNodeDefManager {
public:
	CNodeDefManager();
	virtual ~CNodeDefManager();
	void clear();
	virtual IWritableNodeDefManager *clone();
	inline virtual const ContentFeatures& get(content_t c) const;
	inline virtual const ContentFeatures& get(const MapNode &n) const;
	virtual bool getId(const std::string &name, content_t &result) const;
	virtual content_t getId(const std::string &name) const;
	virtual bool getIds(const std::string &name, FMBitset &result) const;
	virtual bool getIds(const std::string &name, std::unordered_set<content_t> &result) const;
	virtual const ContentFeatures& get(const std::string &name) const;
	content_t allocateId();
	virtual content_t set(const std::string &name, const ContentFeatures &def);
	virtual content_t allocateDummy(const std::string &name);
	virtual void removeNode(const std::string &name);
	virtual void updateAliases(IItemDefManager *idef);
	virtual void applyTextureOverrides(const std::string &override_filepath);
	virtual void updateTextures(IGameDef *gamedef,
		void (*progress_cbk)(void *progress_args, u32 progress, u32 max_progress),
		void *progress_cbk_args);
	void serialize(std::ostream &os, u16 protocol_version) const;
	void deSerialize(std::istream &is);
	void msgpack_pack(msgpack::packer<msgpack::sbuffer> &pk) const;
	void msgpack_unpack(msgpack::object o);

	inline virtual bool getNodeRegistrationStatus() const;
	inline virtual void setNodeRegistrationStatus(bool completed);

	virtual void pendNodeResolve(NodeResolver *nr);
	virtual bool cancelNodeResolveCallback(NodeResolver *nr);
	virtual void runNodeResolveCallbacks();
	virtual void resetNodeResolveState();
	virtual void mapNodeboxConnections();
	virtual bool nodeboxConnects(MapNode from, MapNode to, u8 connect_face);

private:
	void addNameIdMapping(content_t i, std::string name);

	// Features indexed by id
	std::vector<ContentFeatures> m_content_features;

	// A mapping for fast converting back and forth between names and ids
	NameIdMapping m_name_id_mapping;

	// Like m_name_id_mapping, but only from names to ids, and includes
	// item aliases too. Updated by updateAliases()
	// Note: Not serialized.

	UNORDERED_MAP<std::string, content_t> m_name_id_mapping_with_aliases;

	// A mapping from groups to a list of content_ts (and their levels)
	// that belong to it.  Necessary for a direct lookup in getIds().
	// Note: Not serialized.
	UNORDERED_MAP<std::string, GroupItems> m_group_to_items;

	// Next possibly free id
	content_t m_next_id;

	// NodeResolvers to callback once node registration has ended
	std::vector<NodeResolver *> m_pending_resolve_callbacks;

	// True when all nodes have been registered
	bool m_node_registration_complete;
};


CNodeDefManager::CNodeDefManager()
{
	clear();
}


CNodeDefManager::~CNodeDefManager()
{
#ifndef SERVER
	for (u32 i = 0; i < m_content_features.size(); i++) {
		ContentFeatures *f = &m_content_features[i];
		for (u32 j = 0; j < 24; j++) {
			if (f->mesh_ptr[j])
				f->mesh_ptr[j]->drop();
		}
	}
#endif
}


void CNodeDefManager::clear()
{
	m_content_features.clear();
	m_name_id_mapping.clear();
	m_name_id_mapping_with_aliases.clear();
	m_group_to_items.clear();
	m_next_id = 0;

	resetNodeResolveState();

	u32 initial_length = 0;
	initial_length = MYMAX(initial_length, CONTENT_UNKNOWN + 1);
	initial_length = MYMAX(initial_length, CONTENT_AIR + 1);
	initial_length = MYMAX(initial_length, CONTENT_IGNORE + 1);
	m_content_features.resize(initial_length);

	// Set CONTENT_UNKNOWN
	{
		ContentFeatures f;
		f.name = "unknown";
		// Insert directly into containers
		content_t c = CONTENT_UNKNOWN;
		m_content_features[c] = f;
		addNameIdMapping(c, f.name);
	}

	// Set CONTENT_AIR
	{
		ContentFeatures f;
		f.name                = "air";
		f.drawtype            = NDT_AIRLIKE;
		f.param_type          = CPT_LIGHT;
		f.light_propagates    = true;
		f.sunlight_propagates = true;
		f.walkable            = false;
		f.pointable           = false;
		f.diggable            = false;
		f.buildable_to        = true;
		f.floodable           = true;
		f.is_ground_content   = true;
#ifndef SERVER
		f.minimap_color = video::SColor(0,0,0,0);
#endif
		// Insert directly into containers
		content_t c = CONTENT_AIR;
		m_content_features[c] = f;
		addNameIdMapping(c, f.name);
	}

	// Set CONTENT_IGNORE
	{
		ContentFeatures f;
		f.name                = "ignore";
		f.drawtype            = NDT_AIRLIKE;
		f.param_type          = CPT_NONE;
		f.light_propagates    = false;
		f.sunlight_propagates = false;
		f.walkable            = false;
		f.pointable           = false;
		f.diggable            = false;
		f.buildable_to        = true; // A way to remove accidental CONTENT_IGNOREs
		f.is_ground_content   = true;
#ifndef SERVER
		f.minimap_color = video::SColor(0,0,0,0);
#endif
		// Insert directly into containers
		content_t c = CONTENT_IGNORE;
		m_content_features[c] = f;
		addNameIdMapping(c, f.name);
		// mtproto: 0 must be ignore always
		if (c)
			m_content_features[0] = f;
	}
}


IWritableNodeDefManager *CNodeDefManager::clone()
{
	CNodeDefManager *mgr = new CNodeDefManager();
	*mgr = *this;
	return mgr;
}


inline const ContentFeatures& CNodeDefManager::get(content_t c) const
{
	return c < m_content_features.size()
			? m_content_features[c] : m_content_features[CONTENT_UNKNOWN];
}


inline const ContentFeatures& CNodeDefManager::get(const MapNode &n) const
{
	return get(n.getContent());
}


bool CNodeDefManager::getId(const std::string &name, content_t &result) const
{
	UNORDERED_MAP<std::string, content_t>::const_iterator
		i = m_name_id_mapping_with_aliases.find(name);
	if(i == m_name_id_mapping_with_aliases.end())
		return false;
	result = i->second;
	return true;
}


content_t CNodeDefManager::getId(const std::string &name) const
{
	content_t id = CONTENT_IGNORE;
	getId(name, id);
	return id;
}


bool CNodeDefManager::getIds(const std::string &name,
		std::unordered_set<content_t> &result) const
{
	//TimeTaker t("getIds", NULL, PRECISION_MICRO);
	if (name.substr(0,6) != "group:") {
		content_t id = CONTENT_IGNORE;
		bool exists = getId(name, id);
		if (exists)
			result.insert(id);
		return exists;
	}
	std::string group = name.substr(6);

	UNORDERED_MAP<std::string, GroupItems>::const_iterator
		i = m_group_to_items.find(group);
	if (i == m_group_to_items.end())
		return true;

	const GroupItems &items = i->second;
	for (GroupItems::const_iterator j = items.begin();
		j != items.end(); ++j) {
		if ((*j).second != 0)
			result.insert((*j).first);
	}
	//printf("getIds: %dus\n", t.stop());
	return true;
}

	bool CNodeDefManager::getIds(const std::string &name, FMBitset &result) const {
		if(name.substr(0,6) != "group:"){
			content_t id = CONTENT_IGNORE;
			bool exists = getId(name, id);
			if (exists)
				result.set(id, true);
			return exists;
		}
		std::string group = name.substr(6);

		auto
			i = m_group_to_items.find(group);
		if (i == m_group_to_items.end())
			return true;

		const GroupItems &items = i->second;
		for (GroupItems::const_iterator j = items.begin();
			j != items.end(); ++j) {
			if ((*j).second != 0)
				result.set((*j).first, true);
		}
		return true;
	}


const ContentFeatures& CNodeDefManager::get(const std::string &name) const
{
	content_t id = CONTENT_UNKNOWN;
	getId(name, id);
	return get(id);
}


// returns CONTENT_IGNORE if no free ID found
content_t CNodeDefManager::allocateId()
{
	for (content_t id = m_next_id;
			id >= m_next_id; // overflow?
			++id) {
		while (id >= m_content_features.size()) {
			m_content_features.push_back(ContentFeatures());
		}
		const ContentFeatures &f = m_content_features[id];
		if (f.name == "") {
			m_next_id = id + 1;
			return id;
		}
	}
	// If we arrive here, an overflow occurred in id.
	// That means no ID was found
	return CONTENT_IGNORE;
}


// IWritableNodeDefManager
content_t CNodeDefManager::set(const std::string &name, const ContentFeatures &def)
{
	// Pre-conditions
	if (name == "")
		return CONTENT_IGNORE;
	if (name != def.name)
		return CONTENT_IGNORE;

	// Don't allow redefining ignore (but allow air and unknown)
	if (name == "ignore") {
		warningstream << "NodeDefManager: Ignoring "
			"CONTENT_IGNORE redefinition"<<std::endl;
		return CONTENT_IGNORE;
	}

	content_t id = CONTENT_IGNORE;
	if (!m_name_id_mapping.getId(name, id)) { // ignore aliases
		// Get new id
		id = allocateId();
		if (id == CONTENT_IGNORE) {
			warningstream << "NodeDefManager: Absolute "
				"limit reached" << std::endl;
			return CONTENT_IGNORE;
		}
		if (id == CONTENT_IGNORE)
			return CONTENT_IGNORE;
		addNameIdMapping(id, name);
	}
	m_content_features[id] = def;
	verbosestream << "NodeDefManager: registering content id \"" << id
		<< "\": name=\"" << def.name << "\""<<std::endl;

	// Add this content to the list of all groups it belongs to
	// FIXME: This should remove a node from groups it no longer
	// belongs to when a node is re-registered
	for (ItemGroupList::const_iterator i = def.groups.begin();
		i != def.groups.end(); ++i) {
		std::string group_name = i->first;

		UNORDERED_MAP<std::string, GroupItems>::iterator
			j = m_group_to_items.find(group_name);
		if (j == m_group_to_items.end()) {
			m_group_to_items[group_name].push_back(
				std::make_pair(id, i->second));
		} else {
			GroupItems &items = j->second;
			items.push_back(std::make_pair(id, i->second));
		}
	}
	return id;
}


content_t CNodeDefManager::allocateDummy(const std::string &name)
{
	if (name == "")
		return CONTENT_IGNORE;
	ContentFeatures f;
	f.name = name;
	return set(name, f);
}


void CNodeDefManager::removeNode(const std::string &name)
{
	// Pre-condition
	assert(name != "");

	// Erase name from name ID mapping
	content_t id = CONTENT_IGNORE;
	if (m_name_id_mapping.getId(name, id)) {
		m_name_id_mapping.eraseName(name);
		m_name_id_mapping_with_aliases.erase(name);
	}

	// Erase node content from all groups it belongs to
	for (UNORDERED_MAP<std::string, GroupItems>::iterator iter_groups =
			m_group_to_items.begin();
			iter_groups != m_group_to_items.end();) {
		GroupItems &items = iter_groups->second;
		for (GroupItems::iterator iter_groupitems = items.begin();
				iter_groupitems != items.end();) {
			if (iter_groupitems->first == id)
				items.erase(iter_groupitems++);
			else
				iter_groupitems++;
		}

		// Check if group is empty
		if (items.size() == 0)
			m_group_to_items.erase(iter_groups++);
		else
			iter_groups++;
	}
}


void CNodeDefManager::updateAliases(IItemDefManager *idef)
{
	std::set<std::string> all = idef->getAll();
	m_name_id_mapping_with_aliases.clear();
	for (std::set<std::string>::iterator
			i = all.begin(); i != all.end(); ++i) {
		std::string name = *i;
		std::string convert_to = idef->getAlias(name);
		content_t id;
		if (m_name_id_mapping.getId(convert_to, id)) {
			m_name_id_mapping_with_aliases.insert(
				std::make_pair(name, id));
		}
	}
}

void CNodeDefManager::applyTextureOverrides(const std::string &override_filepath)
{
	infostream << "CNodeDefManager::applyTextureOverrides(): Applying "
		"overrides to textures from " << override_filepath << std::endl;

	std::ifstream infile(override_filepath.c_str());
	std::string line;
	int line_c = 0;
	while (std::getline(infile, line)) {
		line_c++;
		if (trim(line) == "")
			continue;
		std::vector<std::string> splitted = str_split(line, ' ');
		if (splitted.size() != 3) {
			errorstream << override_filepath
				<< ":" << line_c << " Could not apply texture override \""
				<< line << "\": Syntax error" << std::endl;
			continue;
		}

		content_t id;
		if (!getId(splitted[0], id))
			continue; // Ignore unknown node

		ContentFeatures &nodedef = m_content_features[id];

		if (splitted[1] == "top")
			nodedef.tiledef[0].name = splitted[2];
		else if (splitted[1] == "bottom")
			nodedef.tiledef[1].name = splitted[2];
		else if (splitted[1] == "right")
			nodedef.tiledef[2].name = splitted[2];
		else if (splitted[1] == "left")
			nodedef.tiledef[3].name = splitted[2];
		else if (splitted[1] == "back")
			nodedef.tiledef[4].name = splitted[2];
		else if (splitted[1] == "front")
			nodedef.tiledef[5].name = splitted[2];
		else if (splitted[1] == "all" || splitted[1] == "*")
			for (int i = 0; i < 6; i++)
				nodedef.tiledef[i].name = splitted[2];
		else if (splitted[1] == "sides")
			for (int i = 2; i < 6; i++)
				nodedef.tiledef[i].name = splitted[2];
		else {
			errorstream << override_filepath
				<< ":" << line_c << " Could not apply texture override \""
				<< line << "\": Unknown node side \""
				<< splitted[1] << "\"" << std::endl;
			continue;
		}
	}
}

void CNodeDefManager::updateTextures(IGameDef *gamedef,
	void (*progress_callback)(void *progress_args, u32 progress, u32 max_progress),
	void *progress_callback_args)
{
	infostream << "CNodeDefManager::updateTextures(): Updating "
		"textures in node definitions" << std::endl;
	bool server = !progress_callback;

	ITextureSource *tsrc = !gamedef ? nullptr : gamedef->tsrc();
	IShaderSource *shdsrc = !gamedef ? nullptr : gamedef->getShaderSource();
	scene::ISceneManager* smgr = !gamedef ? nullptr : gamedef->getSceneManager();
	scene::IMeshManipulator* meshmanip = !smgr ? nullptr : smgr->getMeshManipulator();
	TextureSettings tsettings;
	tsettings.readSettings();

	u32 size = m_content_features.size();

	for (u32 i = 0; i < size; i++) {
		m_content_features[i].updateTextures(tsrc, shdsrc, smgr, meshmanip, gamedef, tsettings, server);
		if (progress_callback)
		progress_callback(progress_callback_args, i, size);
	}
}



void CNodeDefManager::serialize(std::ostream &os, u16 protocol_version) const
{
	writeU8(os, 1); // version
	u16 count = 0;
	std::ostringstream os2(std::ios::binary);
	for (u32 i = 0; i < m_content_features.size(); i++) {
		if (i == CONTENT_IGNORE || i == CONTENT_AIR
				|| i == CONTENT_UNKNOWN)
			continue;
		const ContentFeatures *f = &m_content_features[i];
		if (f->name == "")
			continue;
		writeU16(os2, i);
		// Wrap it in a string to allow different lengths without
		// strict version incompatibilities
		std::ostringstream wrapper_os(std::ios::binary);
		f->serialize(wrapper_os, protocol_version);
		os2<<serializeString(wrapper_os.str());

		// must not overflow
		u16 next = count + 1;
		FATAL_ERROR_IF(next < count, "Overflow");
		count++;
	}
	writeU16(os, count);
	os << serializeLongString(os2.str());
}


void CNodeDefManager::deSerialize(std::istream &is)
{
	clear();
	int version = readU8(is);
	if (version != 1)
		throw SerializationError("unsupported NodeDefinitionManager version");
	u16 count = readU16(is);
	std::istringstream is2(deSerializeLongString(is), std::ios::binary);
	ContentFeatures f;
	for (u16 n = 0; n < count; n++) {
		u16 i = readU16(is2);

		// Read it from the string wrapper
		std::string wrapper = deSerializeString(is2);
		std::istringstream wrapper_is(wrapper, std::ios::binary);
		f.deSerialize(wrapper_is);

		// Check error conditions
		if (i == CONTENT_IGNORE || i == CONTENT_AIR || i == CONTENT_UNKNOWN) {
			warningstream << "NodeDefManager::deSerialize(): "
				"not changing builtin node " << i << std::endl;
			continue;
		}
		if (f.name == "") {
			warningstream << "NodeDefManager::deSerialize(): "
				"received empty name" << std::endl;
			continue;
		}

		// Ignore aliases
		u16 existing_id;
		if (m_name_id_mapping.getId(f.name, existing_id) && i != existing_id) {
			warningstream << "NodeDefManager::deSerialize(): "
				"already defined with different ID: " << f.name << std::endl;
			continue;
		}

		// All is ok, add node definition with the requested ID
		if (i >= m_content_features.size())
			m_content_features.resize((u32)(i) + 1);
		m_content_features[i] = f;
		addNameIdMapping(i, f.name);
		verbosestream << "deserialized " << f.name << std::endl;
	}
}

// map of content features, key = id, value = ContentFeatures
void CNodeDefManager::msgpack_pack(msgpack::packer<msgpack::sbuffer> &pk) const
{
	std::vector<std::pair<int, const ContentFeatures*> > features_to_pack;
	for (size_t i = 0; i < m_content_features.size(); ++i) {
		if (i == CONTENT_IGNORE || i == CONTENT_AIR || i == CONTENT_UNKNOWN || m_content_features[i].name == "")
			continue;
		features_to_pack.push_back(std::make_pair(i, &m_content_features[i]));
	}
	pk.pack_map(features_to_pack.size());
	for (size_t i = 0; i < features_to_pack.size(); ++i)
		PACK(features_to_pack[i].first, *features_to_pack[i].second);
}
void CNodeDefManager::msgpack_unpack(msgpack::object o)
{
	clear();

	std::map<int, ContentFeatures> unpacked_features;
	o.convert(unpacked_features);

	for (std::map<int, ContentFeatures>::iterator it = unpacked_features.begin();
			it != unpacked_features.end(); ++it) {
		unsigned int i = it->first;
		ContentFeatures f = it->second;

		if(i == CONTENT_IGNORE || i == CONTENT_AIR
				|| i == CONTENT_UNKNOWN){
			infostream<<"NodeDefManager::deSerialize(): WARNING: "
				<<"not changing builtin node "<<i
				<<std::endl;
			continue;
		}
		if(f.name == ""){
			infostream<<"NodeDefManager::deSerialize(): WARNING: "
				<<"received empty name"<<std::endl;
			continue;
		}
		u16 existing_id;
		bool found = m_name_id_mapping.getId(f.name, existing_id);  // ignore aliases
		if(found && i != existing_id){
			infostream<<"NodeDefManager::deSerialize(): WARNING: "
				<<"already defined with different ID: "
				<<f.name<<std::endl;
			continue;
		}

		// All is ok, add node definition with the requested ID
		if(i >= m_content_features.size())
			m_content_features.resize((u32)(i) + 1);
		m_content_features[i] = f;
		addNameIdMapping(i, f.name);
		verbosestream<<"deserialized "<<f.name<<std::endl;
	}
}


void CNodeDefManager::addNameIdMapping(content_t i, std::string name)
{
	m_name_id_mapping.set(i, name);
	m_name_id_mapping_with_aliases.insert(std::make_pair(name, i));
}


IWritableNodeDefManager *createNodeDefManager()
{
	return new CNodeDefManager();
}

#if 0
void ContentFeatures::deSerializeOld(std::istream &is, int version)
	if (version == 5) // In PROTOCOL_VERSION 13
	{
		name = deSerializeString(is);
		groups.clear();
		u32 groups_size = readU16(is);
		for(u32 i=0; i<groups_size; i++){
			std::string name = deSerializeString(is);
			int value = readS16(is);
			groups[name] = value;
		}
		drawtype = (enum NodeDrawType)readU8(is);

		visual_scale = readF1000(is);
		if (readU8(is) != 6)
			throw SerializationError("unsupported tile count");
		for (u32 i = 0; i < 6; i++)
			tiledef[i].deSerialize(is, version, drawtype);
		if (readU8(is) != CF_SPECIAL_COUNT)
			throw SerializationError("unsupported CF_SPECIAL_COUNT");
		for (u32 i = 0; i < CF_SPECIAL_COUNT; i++)
			tiledef_special[i].deSerialize(is, version, drawtype);
		alpha = readU8(is);
		post_effect_color.setAlpha(readU8(is));
		post_effect_color.setRed(readU8(is));
		post_effect_color.setGreen(readU8(is));
		post_effect_color.setBlue(readU8(is));
		param_type = (enum ContentParamType)readU8(is);
		param_type_2 = (enum ContentParamType2)readU8(is);
		is_ground_content = readU8(is);
		light_propagates = readU8(is);
		sunlight_propagates = readU8(is);
		walkable = readU8(is);
		pointable = readU8(is);
		diggable = readU8(is);
		climbable = readU8(is);
		buildable_to = readU8(is);
		deSerializeString(is); // legacy: used to be metadata_name
		liquid_type = (enum LiquidType)readU8(is);
		liquid_alternative_flowing = deSerializeString(is);
		liquid_alternative_source = deSerializeString(is);
		liquid_viscosity = readU8(is);
		light_source = readU8(is);
		light_source = MYMIN(light_source, LIGHT_MAX);
		damage_per_second = readU32(is);
		node_box.deSerialize(is);
		selection_box.deSerialize(is);
		legacy_facedir_simple = readU8(is);
		legacy_wallmounted = readU8(is);
		deSerializeSimpleSoundSpec(sound_footstep, is);
		deSerializeSimpleSoundSpec(sound_dig, is);
		deSerializeSimpleSoundSpec(sound_dug, is);
	} else if (version == 6) {
		name = deSerializeString(is);
		groups.clear();
		u32 groups_size = readU16(is);
		for (u32 i = 0; i < groups_size; i++) {
			std::string name = deSerializeString(is);
			int	value = readS16(is);
			groups[name] = value;
		}
		drawtype = (enum NodeDrawType)readU8(is);
		visual_scale = readF1000(is);
		if (readU8(is) != 6)
			throw SerializationError("unsupported tile count");
		for (u32 i = 0; i < 6; i++)
			tiledef[i].deSerialize(is, version, drawtype);
		// CF_SPECIAL_COUNT in version 6 = 2
		if (readU8(is) != 2)
			throw SerializationError("unsupported CF_SPECIAL_COUNT");
		for (u32 i = 0; i < 2; i++)
			tiledef_special[i].deSerialize(is, version, drawtype);
		alpha = readU8(is);
		post_effect_color.setAlpha(readU8(is));
		post_effect_color.setRed(readU8(is));
		post_effect_color.setGreen(readU8(is));
		post_effect_color.setBlue(readU8(is));
		param_type = (enum ContentParamType)readU8(is);
		param_type_2 = (enum ContentParamType2)readU8(is);
		is_ground_content = readU8(is);
		light_propagates = readU8(is);
		sunlight_propagates = readU8(is);
		walkable = readU8(is);
		pointable = readU8(is);
		diggable = readU8(is);
		climbable = readU8(is);
		buildable_to = readU8(is);
		deSerializeString(is); // legacy: used to be metadata_name
		liquid_type = (enum LiquidType)readU8(is);
		liquid_alternative_flowing = deSerializeString(is);
		liquid_alternative_source = deSerializeString(is);
		liquid_viscosity = readU8(is);
		liquid_renewable = readU8(is);
		light_source = readU8(is);
		damage_per_second = readU32(is);
		node_box.deSerialize(is);
		selection_box.deSerialize(is);
		legacy_facedir_simple = readU8(is);
		legacy_wallmounted = readU8(is);
		deSerializeSimpleSoundSpec(sound_footstep, is);
		deSerializeSimpleSoundSpec(sound_dig, is);
		deSerializeSimpleSoundSpec(sound_dug, is);
		rightclickable = readU8(is);
		drowning = readU8(is);
		leveled = readU8(is);
		liquid_range = readU8(is);
	} else {
		throw SerializationError("unsupported ContentFeatures version");
	}
}
#endif

inline bool CNodeDefManager::getNodeRegistrationStatus() const
{
	return m_node_registration_complete;
}


inline void CNodeDefManager::setNodeRegistrationStatus(bool completed)
{
	m_node_registration_complete = completed;
}


void CNodeDefManager::pendNodeResolve(NodeResolver *nr)
{
	nr->m_ndef = this;
	if (m_node_registration_complete)
		nr->nodeResolveInternal();
	else
		m_pending_resolve_callbacks.push_back(nr);
}


bool CNodeDefManager::cancelNodeResolveCallback(NodeResolver *nr)
{
	size_t len = m_pending_resolve_callbacks.size();
	for (size_t i = 0; i != len; i++) {
		if (nr != m_pending_resolve_callbacks[i])
			continue;

		len--;
		m_pending_resolve_callbacks[i] = m_pending_resolve_callbacks[len];
		m_pending_resolve_callbacks.resize(len);
		return true;
	}

	return false;
}


void CNodeDefManager::runNodeResolveCallbacks()
{
	for (size_t i = 0; i != m_pending_resolve_callbacks.size(); i++) {
		NodeResolver *nr = m_pending_resolve_callbacks[i];
		nr->nodeResolveInternal();
	}

	m_pending_resolve_callbacks.clear();
}


void CNodeDefManager::resetNodeResolveState()
{
	m_node_registration_complete = false;
	m_pending_resolve_callbacks.clear();
}

void CNodeDefManager::mapNodeboxConnections()
{
	for (u32 i = 0; i < m_content_features.size(); i++) {
		ContentFeatures *f = &m_content_features[i];
		if ((f->drawtype != NDT_NODEBOX) || (f->node_box.type != NODEBOX_CONNECTED))
			continue;
		for (std::vector<std::string>::iterator it = f->connects_to.begin();
				it != f->connects_to.end(); ++it) {
			getIds(*it, f->connects_to_ids);
		}
	}
}

bool CNodeDefManager::nodeboxConnects(MapNode from, MapNode to, u8 connect_face)
{
	const ContentFeatures &f1 = get(from);

	if ((f1.drawtype != NDT_NODEBOX) || (f1.node_box.type != NODEBOX_CONNECTED))
		return false;

	// lookup target in connected set
	if (f1.connects_to_ids.find(to.param0) == f1.connects_to_ids.end())
		return false;

	const ContentFeatures &f2 = get(to);

	if ((f2.drawtype == NDT_NODEBOX) && (f2.node_box.type == NODEBOX_CONNECTED))
		// ignores actually looking if back connection exists
		return (f2.connects_to_ids.find(from.param0) != f2.connects_to_ids.end());

	// does to node declare usable faces?
	if (f2.connect_sides > 0) {
		if ((f2.param_type_2 == CPT2_FACEDIR) && (connect_face >= 4)) {
			static const u8 rot[33 * 4] = {
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				4, 32, 16, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 4 - back
				8, 4, 32, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 8 - right
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				16, 8, 4, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 16 - front
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				32, 16, 8, 4 // 32 - left
			};
			return (f2.connect_sides & rot[(connect_face * 4) + to.param2]);
		}
		return (f2.connect_sides & connect_face);
	}
	// the target is just a regular node, so connect no matter back connection
	return true;
}

////
//// NodeResolver
////

NodeResolver::NodeResolver()
{
	m_ndef            = NULL;
	m_nodenames_idx   = 0;
	m_nnlistsizes_idx = 0;
	m_resolve_done    = false;

	m_nodenames.reserve(16);
	m_nnlistsizes.reserve(4);
}


NodeResolver::~NodeResolver()
{
	if (!m_resolve_done && m_ndef)
		m_ndef->cancelNodeResolveCallback(this);
}


void NodeResolver::nodeResolveInternal()
{
	m_nodenames_idx   = 0;
	m_nnlistsizes_idx = 0;

	resolveNodeNames();
	m_resolve_done = true;

	m_nodenames.clear();
	m_nnlistsizes.clear();
}


bool NodeResolver::getIdFromNrBacklog(content_t *result_out,
	const std::string &node_alt, content_t c_fallback)
{
	if (m_nodenames_idx == m_nodenames.size()) {
		*result_out = c_fallback;
		errorstream << "NodeResolver: no more nodes in list" << std::endl;
		return false;
	}

	content_t c;
	std::string name = m_nodenames[m_nodenames_idx++];

	bool success = m_ndef->getId(name, c);
	if (!success && node_alt != "") {
		name = node_alt;
		success = m_ndef->getId(name, c);
	}

	if (!success) {
		infostream << "NodeResolver: failed to resolve node name '" << name
			<< "'." << std::endl;
		c = c_fallback;
	}

	*result_out = c;
	return success;
}


bool NodeResolver::getIdsFromNrBacklog(std::vector<content_t> *result_out,
	bool all_required, content_t c_fallback)
{
	bool success = true;

	if (m_nnlistsizes_idx == m_nnlistsizes.size()) {
		infostream << "NodeResolver: no more node lists" << std::endl;
		return false;
	}

	size_t length = m_nnlistsizes[m_nnlistsizes_idx++];

	while (length--) {
		if (m_nodenames_idx == m_nodenames.size()) {
			infostream << "NodeResolver: no more nodes in list" << std::endl;
			return false;
		}

		content_t c;
		std::string &name = m_nodenames[m_nodenames_idx++];

		if (name.substr(0,6) != "group:") {
			if (m_ndef->getId(name, c)) {
				result_out->push_back(c);
			} else if (all_required) {
				infostream << "NodeResolver: failed to resolve node name '"
					<< name << "'." << std::endl;
				result_out->push_back(c_fallback);
				success = false;
			}
		} else {
			std::unordered_set<content_t> cids;
			m_ndef->getIds(name, cids);
			for (auto it = cids.begin(); it != cids.end(); ++it)
				result_out->push_back(*it);
		}
	}

	return success;
}
