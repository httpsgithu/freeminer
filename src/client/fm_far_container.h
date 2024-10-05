#pragma once

#include "fm_nodecontainer.h"
#include "mapblock.h"
#include "threading/concurrent_unordered_map.h"

class Mapgen;
class Client;
class FarContainer : public NodeContainer
{
	Client *m_client;

public:
	Mapgen *m_mg{};

	std::array<concurrent_unordered_map<v3bpos_t, MapBlockP>, FARMESH_STEP_MAX>
			far_blocks;

	FarContainer(Client *client);
	const MapNode &getNodeRefUnsafe(const v3pos_t &p) override;
};
