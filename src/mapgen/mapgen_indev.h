/*
mapgen_indev.h
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
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

#pragma once

#include "json/json.h"
#include "mapgen/mapgen.h"
#include "mapgen/mapgen_v6.h"
#include "mapgen/cavegen.h"
#include "mapnode.h"

//#define getNoiseIndevParams(x, y) getStruct((x), "f,f,v3,s32,s32,f,f,f,f", &(y), sizeof(y))
//#define setNoiseIndevParams(x, y) setStruct((x), "f,f,v3,s32,s32,f,f,f,f", &(y))


typedef struct {
	content_t content = CONTENT_IGNORE;
	MapNode node;
	int height_min = -MAX_MAP_GENERATION_LIMIT;
	int height_max = +MAX_MAP_GENERATION_LIMIT;
	int thickness = 1;
	//std::string name; //dev
} layer_data;

class Mapgen_features {
public:

	Mapgen_features(MapgenParams *params, EmergeParams *emerge);
	~Mapgen_features();

	int y_offset = 0;
	MapNode n_stone;
	Noise *noise_layers = nullptr;
	float noise_layers_width = 0;
	std::vector<layer_data> layers;
	std::vector<MapNode> layers_node;
	unsigned int layers_node_size = 0;
	void layers_init(EmergeParams *emerge, const Json::Value & layersj);
	void layers_prepare(const v3POS & node_min, const v3POS & node_max);
	MapNode layers_get(unsigned int index);

	Noise *noise_float_islands1 = nullptr;
	Noise *noise_float_islands2 = nullptr;
	Noise *noise_float_islands3 = nullptr;
	void float_islands_prepare(const v3POS & node_min, const v3POS & node_max, int min_y);
	int float_islands_generate(const v3POS & node_min, const v3POS & node_max, int min_y, MMVManip *vm);

	Noise *noise_cave_indev = nullptr;
	int cave_noise_threshold = 0;
	bool cave_noise_enabled = 0;
	void cave_prepare(const v3POS & node_min, const v3POS & node_max, int max_y);

};


struct MapgenIndevParams : public MapgenV6Params {
	s16 float_islands;

	NoiseParams np_float_islands1;
	NoiseParams np_float_islands2;
	NoiseParams np_float_islands3;
	NoiseParams np_layers;
	NoiseParams np_cave_indev;

	Json::Value paramsj;

	MapgenIndevParams();
	~MapgenIndevParams() {}

	void readParams(const Settings *settings);
	void writeParams(Settings *settings) const;
};

class MapgenIndev : public MapgenV6, public Mapgen_features {
public:
	MapgenIndevParams *sp;

	int xstride, ystride, zstride;

	virtual MapgenType getType() const { return MAPGEN_INDEV; }
	MapgenIndev(MapgenIndevParams *params, EmergeParams *emerge);
	~MapgenIndev();

	virtual void calculateNoise();
	int generateGround();
	void generateCaves(int max_stone_y);
	void generateExperimental();
};

/*
class CaveIndev : public CavesV6 {
public:
	CaveIndev(MapgenIndev *mg, PseudoRandom *ps, PseudoRandom *ps2,
			v3POS node_min, bool is_large_cave);
};
*/
