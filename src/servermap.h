// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2024 celeron55, Perttu Ahola <celeron55@gmail.com>

#pragma once

#include "irr_v3d.h"
#include "mapblock.h"
#include "threading/concurrent_set.h"

#include <vector>
#include <memory>

#include "map.h"
#include "util/container.h"
#include "util/metricsbackend.h"
#include "map_settings_manager.h"
#include "util/unordered_map_hash.h"

class Server;

class Settings;
class MapDatabase;
class IRollbackManager;
class EmergeManager;
class ServerEnvironment;
struct BlockMakeData;
class MetricsBackend;

// TODO: this could wrap all calls to MapDatabase, including locking
struct MapDatabaseAccessor {
	/// Lock, to be taken for any operation
	std::mutex mutex;
	/// Main database
	MapDatabase *dbase = nullptr;
	/// Fallback database for read operations
	MapDatabase *dbase_ro = nullptr;

	/// Load a block, taking dbase_ro into account.
	/// @note call locked
	void loadBlock(v3s16 blockpos, std::string &ret);
};

/*
	ServerMap

	This is the only map class that is able to generate map.
*/

class ServerMap : public Map
{
public:


    // freeminer:
	virtual s16 updateBlockHeat(ServerEnvironment *env, const v3pos_t &p,
			MapBlock *block = nullptr, unordered_map_v3pos<s16> *cache = nullptr,
			bool block_add = true);
	virtual s16 updateBlockHumidity(ServerEnvironment *env, const v3pos_t &p,
			MapBlock *block = nullptr, unordered_map_v3pos<s16> *cache = nullptr,
			bool block_add = true);

	size_t transforming_liquid_size();
	v3pos_t transforming_liquid_pop();
	void transforming_liquid_add(const v3pos_t &p);
	size_t transformLiquidsReal(Server *m_server, const unsigned int max_cycle_ms);
	std::vector<v3pos_t> m_transforming_liquid_local;

	//getSurface level starting on basepos.y up to basepos.y + searchup
	//returns basepos.y -1 if no surface has been found
	// (due to limited data range of basepos.y this will always give a unique
	// return value as long as minetest is compiled at least on 32bit architecture)
	//int getSurface(v3s16 basepos, int searchup, bool walkable_only);
	virtual int getSurface(const v3pos_t &basepos, int searchup, bool walkable_only);
	/*
	{
		return basepos.Y - 1;
	}
*/

	//concurrent_unordered_map<v3POS, bool, v3posHash, v3posEqual> m_transforming_liquid;
	std::mutex m_transforming_liquid_mutex;
	typedef unordered_map_v3pos<int> lighting_map_t;
	std::mutex m_lighting_modified_mutex;
	std::map<v3bpos_t, int> m_lighting_modified_blocks;
	std::map<unsigned int, lighting_map_t> m_lighting_modified_blocks_range;
	void lighting_modified_add(const v3pos_t &pos, int range = 5);

	void unspreadLight(enum LightBank bank, std::map<v3pos_t, u8> &from_nodes,
			std::set<v3pos_t> &light_sources,
			std::map<v3bpos_t, MapBlock *> &modified_blocks);
	void spreadLight(enum LightBank bank, std::set<v3pos_t> &from_nodes,
			std::map<v3bpos_t, MapBlock *> &modified_blocks, uint64_t end_ms);

	u32 updateLighting(concurrent_map<v3bpos_t, MapBlock *> &a_blocks,
			std::map<v3bpos_t, MapBlock *> &modified_blocks, unsigned int max_cycle_ms);
	u32 updateLighting(lighting_map_t &a_blocks, unordered_map_v3pos<int> &processed,
			unsigned int max_cycle_ms = 0);
	unsigned int updateLightingQueue(unsigned int max_cycle_ms, int &loopcount);

	bool propagateSunlight(const v3bpos_t &pos, std::set<v3pos_t> &light_sources,
			bool remove_light = false);

	MapBlockPtr loadBlockNoStore(const v3bpos_t &p3d);

	bool m_map_loading_enabled;
	concurrent_shared_unordered_map<v3pos_t, unsigned int, v3posHash, v3posEqual> m_mapgen_process;

	// Carries out any initialization necessary before block is sent
	void prepareBlock(MapBlock *block);

	// Helper for placing objects on ground level
	s16 findGroundLevel(v2pos_t p2d, bool cacheBlocks);
	MapBlockPtr emergeBlockP(v3bpos_t p, bool create_blank=false) override;

	static std::atomic_uint time_life;

	// == end of freeminer





	/*
		savedir: directory to which map data should be saved
	*/
	ServerMap(const std::string &savedir, IGameDef *gamedef, EmergeManager *emerge, MetricsBackend *mb);
	~ServerMap();

	/*
		Get a sector from somewhere.
		- Check memory
		- Check disk (doesn't load blocks)
		- Create blank one
	*/
/*
	MapSector *createSector(v2s16 p);
*/

	/*
		Blocks are generated by using these and makeBlock().
	*/
	bool blockpos_over_mapgen_limit(v3s16 p);
	bool initBlockMake(v3s16 blockpos, BlockMakeData *data);
	void finishBlockMake(BlockMakeData *data,
		std::map<v3s16, MapBlock*> *changed_blocks);

	/*
		Get a block from somewhere.
		- Memory
		- Create blank
	*/
	MapBlockPtr createBlock(v3bpos_t p);

	/*
		Forcefully get a block from somewhere (blocking!).
		- Memory
		- Load from disk
		- Create blank filled with CONTENT_IGNORE

	*/
	MapBlock *emergeBlock(v3bpos_t p, bool create_blank=false) override;

	/*
		Try to get a block.
		If it does not exist in memory, add it to the emerge queue.
		- Memory
		- Emerge Queue (deferred disk or generate)
	*/
	MapBlock *getBlockOrEmerge(v3s16 p3d, bool generate);

	bool isBlockInQueue(v3s16 pos);

	void addNodeAndUpdate(v3s16 p, MapNode n,
			std::map<v3s16, MapBlock*> &modified_blocks,
			bool remove_metadata
			, int fast = 0, bool important = false
			) override;

	/*
		Database functions
	*/
	static MapDatabase *createDatabase(const std::string &name, const std::string &savedir, Settings &conf);

	// Call these before and after saving of blocks
	void beginSave() override;
	void endSave() override;

	s32 save(ModifiedState save_level
	, float dedicated_server_step = 0.1, bool breakable = 0
	) override;
	void listAllLoadableBlocks(std::vector<v3s16> &dst);
	void listAllLoadedBlocks(std::vector<v3s16> &dst);

	MapgenParams *getMapgenParams();

	bool saveBlock(MapBlock *block) override;
	static bool saveBlock(MapBlock *block, MapDatabase *db, int compression_level = -1);

	// Load block in a synchronous fashion
	MapBlock *loadBlock(v3s16 p) {return loadBlockP(p).get(); };
	MapBlockPtr loadBlockP(v3bpos_t p);
	/// Load a block that was already read from disk. Used by EmergeManager.
	/// @return non-null block (but can be blank)
	MapBlockPtr loadBlock(const std::string &blob, v3bpos_t p, bool save_after_load=false);

	// Helper for deserializing blocks from disk
	// @throws SerializationError
	static void deSerializeBlock(MapBlock *block, std::istream &is);

	// Blocks are removed from the map but not deleted from memory until
	// deleteDetachedBlocks() is called, since pointers to them may still exist
	// when deleteBlock() is called.
	bool deleteBlock(v3s16 blockpos) override;

	void deleteDetachedBlocks();

	void step();

	void updateVManip(v3s16 pos);

	// For debug printing
	void PrintInfo(std::ostream &out) override;

	bool isSavingEnabled(){ return m_map_saving_enabled; }

	u64 getSeed();

	/*!
	 * Fixes lighting in one map block.
	 * May modify other blocks as well, as light can spread
	 * out of the specified block.
	 * Returns false if the block is not generated (so nothing
	 * changed), true otherwise.
	 */
	bool repairBlockLight(v3s16 blockpos,
		std::map<v3s16, MapBlock *> *modified_blocks);

	size_t transformLiquids(std::map<v3s16, MapBlock*> & modified_blocks,
			ServerEnvironment *env
			, Server *m_server, unsigned int max_cycle_ms
			);

/*
	void transforming_liquid_add(v3s16 p);
*/

	MapSettingsManager settings_mgr;

protected:

	void reportMetrics(u64 save_time_us, u32 saved_blocks, u32 all_blocks) override;

private:
	friend class ModApiMapgen; // for m_transforming_liquid

	// Emerge manager
	EmergeManager *m_emerge;

public:
	std::string m_savedir;
	bool m_map_saving_enabled;
private:

	int m_map_compression_level;

	concurrent_set<v3s16> m_chunks_in_progress;

	// used by deleteBlock() and deleteDetachedBlocks()
	std::vector<std::unique_ptr<MapBlock>> m_detached_blocks;

	// Queued transforming water nodes
	UniqueQueue<v3s16> m_transforming_liquid;
	f32 m_transforming_liquid_loop_count_multiplier = 1.0f;
	u32 m_unprocessed_count = 0;
	u64 m_inc_trending_up_start_time = 0; // milliseconds
	bool m_queue_size_timer_started = false;

	/*
		Metadata is re-written on disk only if this is true.
		This is reset to false when written on disk.
	*/
	bool m_map_metadata_changed = true;

public:
	MapDatabaseAccessor m_db;
private:

	// Map metrics
	MetricGaugePtr m_loaded_blocks_gauge;
	MetricCounterPtr m_save_time_counter;
	MetricCounterPtr m_save_count_counter;
};
