/*
server.cpp
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

#include "server.h"
#include <iostream>
#include <queue>
#include <algorithm>
#include "network/connection.h"
#include "network/networkprotocol.h"
#include "network/serveropcodes.h"
#include "ban.h"
#include "environment.h"
#include "map.h"
#include "threading/mutex_auto_lock.h"
#include "constants.h"
#include "voxel.h"
#include "config.h"
#include "version.h"
#include "filesys.h"
#include "mapblock.h"
#include "server/serveractiveobject.h"
#include "settings.h"
#include "profiler.h"
<<<<<<< HEAD
#include "log_types.h"
#include "scripting_game.h"
=======
#include "log.h"
#include "scripting_server.h"
>>>>>>> 5.5.0
#include "nodedef.h"
#include "itemdef.h"
#include "craftdef.h"
#include "emerge.h"
#include "mapgen/mapgen.h"
#include "mapgen/mg_biome.h"
#include "content_mapnode.h"
#include "content_nodemeta.h"
<<<<<<< HEAD
#include "content_abm.h"
#include "content_sao.h"
#include "mods.h"
#include "util/sha1.h"
#include "util/base64.h"
#include "tool.h"
#include "sound.h" // dummySoundManager
#include "event_manager.h"
#include "util/hex.h"
#include "serverlist.h"
#include "util/string.h"
#include "util/pointedthing.h"
#include "util/mathconstants.h"
=======
#include "content/mods.h"
#include "modchannels.h"
#include "serverlist.h"
#include "util/string.h"
>>>>>>> 5.5.0
#include "rollback.h"
#include "util/serialize.h"
#include "util/thread.h"
#include "defaultsettings.h"
<<<<<<< HEAD
//#include "stat.h"

#include <iomanip>
#include "msgpack_fix.h"
#include <chrono>
#include "threading/thread_pool.h"
#include "key_value_storage.h"
#include "database.h"


#if !MINETEST_PROTO
#include "network/fm_serverpacketsender.cpp"
#endif

=======
#include "server/mods.h"
#include "util/base64.h"
#include "util/sha1.h"
#include "util/hex.h"
#include "database/database.h"
#include "chatmessage.h"
#include "chat_interface.h"
#include "remoteplayer.h"
#include "server/player_sao.h"
#include "server/serverinventorymgr.h"
#include "translation.h"
#include "database/database-sqlite3.h"
#include "database/database-files.h"
#include "database/database-dummy.h"
#include "gameparams.h"
>>>>>>> 5.5.0

class ClientNotFoundException : public BaseException
{
public:
	ClientNotFoundException(const char *s):
		BaseException(s)
	{}
};

#include "fm_server.cpp"

#if 0

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
	BEGIN_DEBUG_EXCEPTION_HANDLER

	/*
	 * The real business of the server happens on the ServerThread.
	 * How this works:
	 * AsyncRunStep() runs an actual server step as soon as enough time has
	 * passed (dedicated_server_loop keeps track of that).
	 * Receive() blocks at least(!) 30ms waiting for a packet (so this loop
	 * doesn't busy wait) and will process any remaining packets.
	 */

	try {
		m_server->AsyncRunStep(true);
	} catch (con::ConnectionBindFailed &e) {
		m_server->setAsyncFatalError(e.what());
	} catch (LuaError &e) {
		m_server->setAsyncFatalError(e);
	}

	while (!stopRequested()) {
		try {
			m_server->AsyncRunStep();

			m_server->Receive();

		} catch (con::PeerNotFoundException &e) {
			infostream<<"Server: PeerNotFoundException"<<std::endl;
		} catch (ClientNotFoundException &e) {
		} catch (con::ConnectionBindFailed &e) {
			m_server->setAsyncFatalError(e.what());
		} catch (LuaError &e) {
			m_server->setAsyncFatalError(e);
		}
	}

	END_DEBUG_EXCEPTION_HANDLER

	return nullptr;
}

#endif

v3f ServerSoundParams::getPos(ServerEnvironment *env, bool *pos_exists) const
{
	if(pos_exists) *pos_exists = false;
	switch(type){
	case SSP_LOCAL:
		return v3f(0,0,0);
	case SSP_POSITIONAL:
		if(pos_exists) *pos_exists = true;
		return pos;
	case SSP_OBJECT: {
		if(object == 0)
			return v3f(0,0,0);
		ServerActiveObject *sao = env->getActiveObject(object);
		if(!sao)
			return v3f(0,0,0);
		if(pos_exists) *pos_exists = true;
		return sao->getBasePosition(); }
	}
	return v3f(0,0,0);
}

<<<<<<< HEAD
=======
void Server::ShutdownState::reset()
{
	m_timer = 0.0f;
	message.clear();
	should_reconnect = false;
	is_requested = false;
}

void Server::ShutdownState::trigger(float delay, const std::string &msg, bool reconnect)
{
	m_timer = delay;
	message = msg;
	should_reconnect = reconnect;
}

void Server::ShutdownState::tick(float dtime, Server *server)
{
	if (m_timer <= 0.0f)
		return;

	// Timed shutdown
	static const float shutdown_msg_times[] =
	{
		1, 2, 3, 4, 5, 10, 20, 40, 60, 120, 180, 300, 600, 1200, 1800, 3600
	};

	// Automated messages
	if (m_timer < shutdown_msg_times[ARRLEN(shutdown_msg_times) - 1]) {
		for (float t : shutdown_msg_times) {
			// If shutdown timer matches an automessage, shot it
			if (m_timer > t && m_timer - dtime < t) {
				std::wstring periodicMsg = getShutdownTimerMessage();

				infostream << wide_to_utf8(periodicMsg).c_str() << std::endl;
				server->SendChatMessage(PEER_ID_INEXISTENT, periodicMsg);
				break;
			}
		}
	}

	m_timer -= dtime;
	if (m_timer < 0.0f) {
		m_timer = 0.0f;
		is_requested = true;
	}
}

std::wstring Server::ShutdownState::getShutdownTimerMessage() const
{
	std::wstringstream ws;
	ws << L"*** Server shutting down in "
		<< duration_to_string(myround(m_timer)).c_str() << ".";
	return ws.str();
}

>>>>>>> 5.5.0
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
<<<<<<< HEAD
	m_env(NULL),
	m_con(PROTOCOL_ID,
			simple_singleplayer_mode ? MAX_PACKET_SIZE_SINGLEPLAYER : MAX_PACKET_SIZE,
			CONNECTION_TIMEOUT,
			ipv6,
			this),
	m_banmanager(NULL),
	m_rollback(NULL),
	m_enable_rollback_recording(false),
	m_emerge(NULL),
	m_script(NULL),
	stat(path_world),
	m_itemdef(createItemDefManager()),
	m_nodedef(createNodeDefManager()),
	m_craftdef(createCraftDefManager()),
	m_event(new EventManager()),
	m_thread(NULL),
	m_map_thread(nullptr),
	m_sendblocks(nullptr),
	m_liquid(nullptr),
	m_envthread(nullptr),
	m_abmthread(nullptr),
	m_time_of_day_send_timer(0),
	m_uptime(0),
	m_clients(&m_con),
	m_shutdown_requested(false),
	m_shutdown_ask_reconnect(false),
=======
	m_con(std::make_shared<con::Connection>(PROTOCOL_ID,
			512,
			CONNECTION_TIMEOUT,
			m_bind_addr.isIPv6(),
			this)),
	m_itemdef(createItemDefManager()),
	m_nodedef(createNodeDefManager()),
	m_craftdef(createCraftDefManager()),
	m_thread(new ServerThread(this)),
	m_clients(m_con),
>>>>>>> 5.5.0
	m_admin_chat(iface),
	m_on_shutdown_errmsg(on_shutdown_errmsg),
	m_modchannel_mgr(new ModChannelMgr())
{
<<<<<<< HEAD
	m_liquid_transform_timer = 0.0;
	m_liquid_transform_every = 1.0;
	m_liquid_send_timer = 0.0;
	m_liquid_send_interval = 1.0;
	m_masterserver_timer = 0.0;
	//m_emergethread_trigger_timer = 5.0; // to start emerge threads instantly
	m_savemap_timer = 0.0;

	m_step_dtime = 0.0;
	m_lag = g_settings->getFloat("dedicated_server_step");
#if ENABLE_THREADS
	m_more_threads = g_settings->getBool("more_threads");
#endif

	if(path_world == "")
=======
	if (m_path_world.empty())
>>>>>>> 5.5.0
		throw ServerError("Supplied empty world path");

	if (!gamespec.isValid())
		throw ServerError("Supplied invalid gamespec");

#if USE_PROMETHEUS
	m_metrics_backend = std::unique_ptr<MetricsBackend>(createPrometheusMetricsBackend());
#else
	m_metrics_backend = std::unique_ptr<MetricsBackend>(new MetricsBackend());
#endif

	m_uptime_counter = m_metrics_backend->addCounter("minetest_core_server_uptime", "Server uptime (in seconds)");
	m_player_gauge = m_metrics_backend->addGauge("minetest_core_player_number", "Number of connected players");

	m_timeofday_gauge = m_metrics_backend->addGauge(
			"minetest_core_timeofday",
			"Time of day value");

	m_lag_gauge = m_metrics_backend->addGauge(
			"minetest_core_latency",
			"Latency value (in seconds)");

	m_aom_buffer_counter = m_metrics_backend->addCounter(
			"minetest_core_aom_generated_count",
			"Number of active object messages generated");

	m_packet_recv_counter = m_metrics_backend->addCounter(
			"minetest_core_server_packet_recv",
			"Processable packets received");

	m_packet_recv_processed_counter = m_metrics_backend->addCounter(
			"minetest_core_server_packet_recv_processed",
			"Valid received packets processed");

	m_lag_gauge->set(g_settings->getFloat("dedicated_server_step"));
}

Server::~Server()
{

	// Send shutdown message
	SendChatMessage(PEER_ID_INEXISTENT, ChatMessage(CHATMESSAGE_TYPE_ANNOUNCE,
			L"*** Server shutting down"));

	if (m_env) {
		MutexAutoLock envlock(m_env_mutex);

		infostream << "Server: Saving players" << std::endl;
		m_env->saveLoadedPlayers();

		infostream << "Server: Kicking players" << std::endl;
		std::string kick_msg;
		bool reconnect = false;
		if (isShutdownRequested()) {
			reconnect = m_shutdown_state.should_reconnect;
			kick_msg = m_shutdown_state.message;
		}
		if (kick_msg.empty()) {
			kick_msg = g_settings->get("kick_msg_shutdown");
		}
		m_env->saveLoadedPlayers(true);
		m_env->kickAllPlayers(SERVER_ACCESSDENIED_SHUTDOWN,
			kick_msg, reconnect);
	}

	actionstream << "Server: Shutting down" << std::endl;

	// Do this before stopping the server in case mapgen callbacks need to access
	// server-controlled resources (like ModStorages). Also do them before
	// shutdown callbacks since they may modify state that is finalized in a
	// callback.
	if (m_emerge)
		m_emerge->stopThreads();

	if (m_env) {
		MutexAutoLock envlock(m_env_mutex);

		// Execute script shutdown hooks
		infostream << "Executing shutdown hooks" << std::endl;
		try {
			m_script->on_shutdown();
		} catch (ModError &e) {
			errorstream << "ModError: " << e.what() << std::endl;
			if (m_on_shutdown_errmsg) {
				if (m_on_shutdown_errmsg->empty()) {
					*m_on_shutdown_errmsg = std::string("ModError: ") + e.what();
				} else {
					*m_on_shutdown_errmsg += std::string("\nModError: ") + e.what();
				}
			}
		}

		infostream << "Server: Saving environment metadata" << std::endl;
		m_env->saveMeta();
	}

	// Stop threads
	if (m_thread) {
		stop();
		delete m_thread;
	}

	// Write any changes before deletion.
	if (m_mod_storage_database)
		m_mod_storage_database->endSave();

	// Delete things in the reverse order of creation
	delete m_emerge;
	delete m_env;
	delete m_rollback;
	delete m_mod_storage_database;
	delete m_banmanager;
	delete m_itemdef;
	delete m_nodedef;
	delete m_craftdef;

	// Deinitialize scripting
	infostream << "Server: Deinitializing scripting" << std::endl;
	delete m_script;
	delete m_startup_server_map; // if available
	delete m_game_settings;

	while (!m_unsent_map_edit_queue.empty()) {
		delete m_unsent_map_edit_queue.front();
		m_unsent_map_edit_queue.pop();
	}
}

void Server::init()
{
	infostream << "Server created for gameid \"" << m_gamespec.id << "\"";
	if (m_simple_singleplayer_mode)
		infostream << " in simple singleplayer mode" << std::endl;
	else
		infostream << std::endl;
	infostream << "- world:  " << m_path_world << std::endl;
	infostream << "- game:   " << m_gamespec.path << std::endl;

	m_game_settings = Settings::createLayer(SL_GAME);

<<<<<<< HEAD
	// Initialize default settings and override defaults with those provided
	// by the game
	set_default_settings(g_settings);
	Settings gamedefaults;
	getGameMinetestConfig(gamespec.path, gamedefaults);
	override_default_settings(g_settings, &gamedefaults);

	// Create server thread
	m_thread = new ServerThread(this);
=======
	// Create world if it doesn't exist
	try {
		loadGameConfAndInitWorld(m_path_world,
				fs::GetFilenameFromPath(m_path_world.c_str()),
				m_gamespec, false);
	} catch (const BaseException &e) {
		throw ServerError(std::string("Failed to initialize world: ") + e.what());
	}
>>>>>>> 5.5.0

	// Create emerge manager
	m_emerge = new EmergeManager(this);

	if (m_more_threads) {
		m_map_thread = new MapThread(this);
		m_sendblocks = new SendBlocksThread(this);
		m_liquid = new LiquidThread(this);
		m_envthread = new EnvThread(this);
		m_abmthread = new AbmThread(this);
	}

	// Create world if it doesn't exist
	if(!loadGameConfAndInitWorld(m_path_world, m_gamespec))
		throw ServerError("Failed to initialize world");

	// Create ban manager
	std::string ban_path = m_path_world + DIR_DELIM "ipban.txt";
	m_banmanager = new BanManager(ban_path);

	// Create mod storage database and begin a save for later
	m_mod_storage_database = openModStorageDatabase(m_path_world);
	m_mod_storage_database->beginSave();

	m_modmgr = std::unique_ptr<ServerModManager>(new ServerModManager(m_path_world));
	std::vector<ModSpec> unsatisfied_mods = m_modmgr->getUnsatisfiedMods();
	// complain about mods with unsatisfied dependencies
	if (!m_modmgr->isConsistent()) {
		m_modmgr->printUnsatisfiedModsError();
	}

	//lock environment
	//MutexAutoLock envlock(m_env_mutex);

	// Create the Map (loads map_meta.txt, overriding configured mapgen params)
	ServerMap *servermap = new ServerMap(m_path_world, this, m_emerge, m_metrics_backend.get());
	m_startup_server_map = servermap;

	// Initialize scripting
	infostream << "Server: Initializing Lua" << std::endl;

	m_script = new ServerScripting(this);

	// Must be created before mod loading because we have some inventory creation
	m_inventory_mgr = std::unique_ptr<ServerInventoryManager>(new ServerInventoryManager());

	m_script->loadMod(getBuiltinLuaPath() + DIR_DELIM "init.lua", BUILTIN_MOD_NAME);

<<<<<<< HEAD
	// Print mods
	infostream << "Server: Loading mods: ";
	for(std::vector<ModSpec>::iterator i = m_mods.begin();
			i != m_mods.end(); ++i) {
		const ModSpec &mod = *i;
		infostream << mod.name << " ";
	}
	infostream << std::endl;
	// Load and run "mod" scripts
	for (std::vector<ModSpec>::iterator it = m_mods.begin();
			it != m_mods.end(); ++it) {
		const ModSpec &mod = *it;
		if (!string_allowed(mod.name, MODNAME_ALLOWED_CHARS)) {
			throw ModError("Error loading mod \"" + mod.name +
				"\": Mod name does not follow naming conventions: "
				"Only chararacters [a-z0-9_] are allowed.");
		}
		std::string script_path = mod.path + DIR_DELIM + "init.lua";
		if (!fs::PathExists(script_path)) {
			errorstream << "Ignoring empty mod: "<< mod.name << std::endl;
			continue;
		}
		infostream << "  [" << padStringRight(mod.name, 12) << "] [\""
				<< script_path << "\"]" << std::endl;
		m_script->loadMod(script_path, mod.name);
	}
=======
	m_modmgr->loadMods(m_script);
>>>>>>> 5.5.0

	// Read Textures and calculate sha1 sums
	fillMediaCache();

	// Apply item aliases in the node definition manager
	m_nodedef->updateAliases(m_itemdef);

	// Apply texture overrides from texturepack/override.txt
	std::vector<std::string> paths;
	fs::GetRecursiveDirs(paths, g_settings->get("texture_path"));
	fs::GetRecursiveDirs(paths, m_gamespec.path + DIR_DELIM + "textures");
	for (const std::string &path : paths) {
		TextureOverrideSource override_source(path + DIR_DELIM + "override.txt");
		m_nodedef->applyTextureOverrides(override_source.getNodeTileOverrides());
		m_itemdef->applyTextureOverrides(override_source.getItemTextureOverrides());
	}

	m_nodedef->setNodeRegistrationStatus(true);

	// Perform pending node name resolutions
	m_nodedef->runNodeResolveCallbacks();

	// unmap node names in cross-references
	m_nodedef->resolveCrossrefs();

	// init the recipe hashes to speed up crafting
	m_craftdef->initHashes(this);

	// Initialize Environment
	m_startup_server_map = nullptr; // Ownership moved to ServerEnvironment
	m_env = new ServerEnvironment(servermap, m_script, this, m_path_world);
	m_env->m_more_threads = m_more_threads;
	m_emerge->env = m_env;

	m_inventory_mgr->setEnv(m_env);
	m_clients.setEnv(m_env);

	if (!servermap->settings_mgr.makeMapgenParams())
		FATAL_ERROR("Couldn't create any mapgen type");

	// Initialize mapgens
	m_emerge->initMapgens(servermap->getMapgenParams());

<<<<<<< HEAD
#if USE_SQLITE
	m_enable_rollback_recording = g_settings->getBool("enable_rollback_recording");
	if (m_enable_rollback_recording) {
=======
	if (g_settings->getBool("enable_rollback_recording")) {
>>>>>>> 5.5.0
		// Create rollback manager
		m_rollback = new RollbackManager(m_path_world, this);
	}
#endif

	// Give environment reference to scripting api
	m_script->initializeEnvironment(m_env);

	// Register us to receive map edit events
	servermap->addEventReceiver(this);

	m_env->loadMeta();

<<<<<<< HEAD
	m_env->m_abmhandler.init(m_env->m_abms); // uses result of add_legacy_abms and m_script->initializeEnvironment
	m_liquid_send_interval = g_settings->getFloat("liquid_send");

	m_liquid_transform_every = g_settings->getFloat("liquid_update");
	m_max_chatmessage_length = g_settings->getU16("chat_message_max_size");

	m_emerge->startThreads();
=======
	// Those settings can be overwritten in world.mt, they are
	// intended to be cached after environment loading.
	m_liquid_transform_every = g_settings->getFloat("liquid_update");
	m_max_chatmessage_length = g_settings->getU16("chat_message_max_size");
	m_csm_restriction_flags = g_settings->getU64("csm_restriction_flags");
	m_csm_restriction_noderange = g_settings->getU32("csm_restriction_noderange");
>>>>>>> 5.5.0
}

void Server::start()
{
	init();

<<<<<<< HEAD
	if (!m_simple_singleplayer_mode && g_settings->getBool("server_announce"))
		ServerList::sendAnnounce("delete", m_bind_addr.getPort());

	// Send shutdown message
	SendChatMessage(PEER_ID_INEXISTENT, "*** Server shutting down");

	{
		//MutexAutoLock envlock(m_env_mutex);

		// Execute script shutdown hooks
		m_script->on_shutdown();

		infostream << "Server: Saving players" << std::endl;
		m_env->saveLoadedPlayers();

		infostream << "Server: Kicking players" << std::endl;
		std::string kick_msg;
		bool reconnect = false;
		if (getShutdownRequested()) {
			reconnect = m_shutdown_ask_reconnect;
			kick_msg = m_shutdown_msg;
		}
		if (kick_msg == "") {
			kick_msg = g_settings->get("kick_msg_shutdown");
		}
		m_env->kickAllPlayers(SERVER_ACCESSDENIED_SHUTDOWN,
			kick_msg, reconnect);

		infostream << "Server: Saving environment metadata" << std::endl;
		m_env->saveMeta();
	}

	// Stop threads
	stop();
	delete m_thread;

	delete m_liquid;
	delete m_sendblocks;
	delete m_map_thread;
	delete m_abmthread;
	delete m_envthread;

	// stop all emerge threads before deleting players that may have
	// requested blocks to be emerged
	m_emerge->stopThreads();

	// Delete things in the reverse order of creation
	delete m_emerge;
	delete m_env;
	delete m_rollback;
	delete m_banmanager;
	delete m_event;
	delete m_itemdef;
	delete m_nodedef;
	delete m_craftdef;

	// Deinitialize scripting
	infostream<<"Server: Deinitializing scripting"<<std::endl;
	delete m_script;

	// Delete detached inventories
	for (std::map<std::string, Inventory*>::iterator
			i = m_detached_inventories.begin();
			i != m_detached_inventories.end(); ++i) {
		delete i->second;
	}
	while (!m_unsent_map_edit_queue.empty())
		delete m_unsent_map_edit_queue.pop_front();
}

void Server::start(Address bind_addr)
{
	DSTACK(FUNCTION_NAME);

	m_bind_addr = bind_addr;

	infostream<<"Starting server on "
			<< bind_addr.serializeString() <<"..."<<std::endl;
=======
	infostream << "Starting server on " << m_bind_addr.serializeString()
			<< "..." << std::endl;
>>>>>>> 5.5.0

	// Initialize connection
<<<<<<< HEAD
	m_con.Serve(bind_addr);
=======
	m_con->SetTimeoutMs(30);
	m_con->Serve(m_bind_addr);
>>>>>>> 5.5.0

	// Start thread
	m_thread->restart();
	if (m_map_thread)
		m_map_thread->restart();
	if (m_sendblocks)
		m_sendblocks->restart();
	if (m_liquid)
		m_liquid->restart();
	if(m_envthread)
		m_envthread->restart();
	if(m_abmthread)
		m_abmthread->restart();

<<<<<<< HEAD
	actionstream << "\033[1mfree\033[1;33mminer \033[1;36mv" << g_version_hash << "\033[0m \t"
#if ENABLE_THREADS
			<< " THREADS \t"
#endif
#ifndef NDEBUG
			<< " DEBUG \t"
#endif
#if MINETEST_PROTO
			<< " MINETEST_PROTO \t"
#endif
#if USE_SCTP
			<< " SCTP \t"
#endif
			<< " cpp=" <<__cplusplus << " \t"

			<< " cores=";
	auto cores_online = std::thread::hardware_concurrency(), cores_avail = Thread::getNumberOfProcessors();
	if (cores_online != cores_avail)
		actionstream << cores_online << "/";
	actionstream << cores_avail

#if __ANDROID__
			<< " android=" << porting::android_version_sdk_int
#endif
			<< std::endl;
	actionstream<<"World at ["<<m_path_world<<"]"<<std::endl;
	actionstream<<"Server for gameid=\""<<m_gamespec.id
			<< "\" mapgen=\"" << Mapgen::getMapgenName(m_emerge->mgparams->mgtype)
			<<"\" listening on "<<bind_addr.serializeString()<<":"
			<<bind_addr.getPort() << "."<<std::endl;

	if (!m_simple_singleplayer_mode && g_settings->getBool("serverlist_lan"))
		lan_adv_server.serve(m_bind_addr.getPort());
=======
	// ASCII art for the win!
	std::cerr
		<< "         __.               __.                 __.  " << std::endl
		<< "  _____ |__| ____   _____ /  |_  _____  _____ /  |_ " << std::endl
		<< " /     \\|  |/    \\ /  __ \\    _\\/  __ \\/   __>    _\\" << std::endl
		<< "|  Y Y  \\  |   |  \\   ___/|  | |   ___/\\___  \\|  |  " << std::endl
		<< "|__|_|  /  |___|  /\\______>  |  \\______>_____/|  |  " << std::endl
		<< "      \\/ \\/     \\/         \\/                  \\/   " << std::endl;
	actionstream << "World at [" << m_path_world << "]" << std::endl;
	actionstream << "Server for gameid=\"" << m_gamespec.id
			<< "\" listening on ";
	m_bind_addr.print(&actionstream);
	actionstream << "." << std::endl;
>>>>>>> 5.5.0
}

void Server::stop()
{
	infostream<<"Server: Stopping and waiting threads"<<std::endl;

	// Stop threads (set run=false first so both start stopping)
	m_thread->stop();
<<<<<<< HEAD
	if (m_liquid)
		m_liquid->stop();
	if (m_sendblocks)
		m_sendblocks->stop();
	if (m_map_thread)
		m_map_thread->stop();
	if(m_abmthread)
		m_abmthread->stop();
	if(m_envthread)
		m_envthread->stop();

	//m_emergethread.setRun(false);
	m_thread->join();
	//m_emergethread.stop();
	if (m_liquid)
		m_liquid->join();
	if (m_sendblocks)
		m_sendblocks->join();
	if (m_map_thread)
		m_map_thread->join();
	if(m_abmthread)
		m_abmthread->join();
	if(m_envthread)
		m_envthread->join();
=======
	m_thread->wait();
>>>>>>> 5.5.0

	infostream<<"Server: Threads stopped"<<std::endl;
}

void Server::step(float dtime)
{
	// Limit a bit
	if (dtime > 2.0)
		dtime = 2.0;
	{
		MutexAutoLock lock(m_step_dtime_mutex);
		m_step_dtime += dtime;
	}
	// Assert if fatal error occurred in thread
	std::string async_err = m_async_fatal_error.get();
	if (!async_err.empty()) {
/*
		if (!m_simple_singleplayer_mode) {
			m_env->kickAllPlayers(SERVER_ACCESSDENIED_CRASH,
				g_settings->get("kick_msg_crash"),
				g_settings->getBool("ask_reconnect_on_crash"));
		}
<<<<<<< HEAD
		throw ServerError(async_err);
*/
=======
		throw ServerError("AsyncErr: " + async_err);
>>>>>>> 5.5.0
	}
}

void Server::AsyncRunStep(float dtime, bool initial_step)
{
<<<<<<< HEAD
	DSTACK(FUNCTION_NAME);

	TimeTaker timer_step("Server step");
	g_profiler->add("Server::AsyncRunStep (num)", 1);
/*
=======

>>>>>>> 5.5.0
	float dtime;
	{
		MutexAutoLock lock1(m_step_dtime_mutex);
		dtime = m_step_dtime;
	}
*/

	if (!m_more_threads)
	{
		TimeTaker timer_step("Server step: SendBlocks");
		// Send blocks to clients
		SendBlocks(dtime);
	}

	if((dtime < 0.001) && !initial_step)
		return;

<<<<<<< HEAD
/*
	g_profiler->add("Server::AsyncRunStep with dtime (num)", 1);
*/
	ScopeProfiler sp(g_profiler, "Server::AsyncRunStep, avg", SPT_AVG);
	//infostream<<"Server steps "<<dtime<<std::endl;
	//infostream<<"Server::AsyncRunStep(): dtime="<<dtime<<std::endl;
=======
	ScopeProfiler sp(g_profiler, "Server::AsyncRunStep()", SPT_AVG);
>>>>>>> 5.5.0

/*
	{
		TimeTaker timer_step("Server step: SendBlocks");
		MutexAutoLock lock1(m_step_dtime_mutex);
		m_step_dtime -= dtime;
	}
*/

	/*
		Update uptime
	*/
	m_uptime_counter->increment(dtime);

	f32 dedicated_server_step = g_settings->getFloat("dedicated_server_step");
	//u32 max_cycle_ms = 1000 * (m_lag > dedicated_server_step ? dedicated_server_step/(m_lag/dedicated_server_step) : dedicated_server_step);
	u32 max_cycle_ms = 1000 * (dedicated_server_step/(m_lag/dedicated_server_step));
	if (max_cycle_ms < 40)
		max_cycle_ms = 40;

	{
		TimeTaker timer_step("Server step: handlePeerChanges");
		// This has to be called so that the client list gets synced
		// with the peer list of the connection
		handlePeerChanges();
	}

	/*
		Update time of day and overall game time
	*/
	{
		TimeTaker timer_step("Server step: pdate time of day and overall game time");
		//MutexAutoLock envlock(m_env_mutex);

		m_env->setTimeOfDaySpeed(g_settings->getFloat("time_speed"));

<<<<<<< HEAD
		/*
			Send to clients at constant intervals
		*/

		m_time_of_day_send_timer -= dtime;
		if(m_time_of_day_send_timer < 0.0)
		{
			m_time_of_day_send_timer = g_settings->getFloat("time_send_interval");
			u16 time = m_env->getTimeOfDay();
			float time_speed = g_settings->getFloat("time_speed");
			SendTimeOfDay(PEER_ID_INEXISTENT, time, time_speed);

			// bad place, but every 5s ok
			lan_adv_server.clients_num = m_clients.getPlayerNames().size();
		}
=======
	m_time_of_day_send_timer -= dtime;
	if (m_time_of_day_send_timer < 0.0) {
		m_time_of_day_send_timer = g_settings->getFloat("time_send_interval");
		u16 time = m_env->getTimeOfDay();
		float time_speed = g_settings->getFloat("time_speed");
		SendTimeOfDay(PEER_ID_INEXISTENT, time, time_speed);

		m_timeofday_gauge->set(time);
>>>>>>> 5.5.0
	}

	{
		//TimeTaker timer_step("Server step: m_env->step");
		//MutexAutoLock lock(m_env_mutex);
		// Figure out and report maximum lag to environment
		float max_lag = m_env->getMaxLagEstimate();
		max_lag *= 0.9998; // Decrease slowly (about half per 5 minutes)
		if(dtime > max_lag){
			if(dtime > dedicated_server_step && dtime > max_lag * 2.0)
				infostream<<"Server: Maximum lag peaked to "<<dtime
						<<" s"<<std::endl;
			max_lag = dtime;
		}
		m_env->reportMaxLagEstimate(max_lag);
		g_profiler->add("Server: dtime max_lag", max_lag);
		g_profiler->add("Server: dtime", dtime);
		// Step environment
<<<<<<< HEAD
		//ScopeProfiler sp(g_profiler, "SEnv step");
		if (!m_more_threads)
		m_env->step(dtime, m_uptime.get(), max_cycle_ms);
=======
		m_env->step(dtime);
>>>>>>> 5.5.0
	}

/*
	static const float map_timer_and_unload_dtime = 2.92;
	if(m_map_timer_and_unload_interval.step(dtime, map_timer_and_unload_dtime))
	{
		MutexAutoLock lock(m_env_mutex);
		// Run Map's timers and unload unused data
		ScopeProfiler sp(g_profiler, "Server: map timer and unload");
		m_env->getMap().timerUpdate(map_timer_and_unload_dtime,
			g_settings->getFloat("server_unload_unused_data_timeout"),
			U32_MAX);
	}
*/

	/*
		Listen to the admin chat, if available
	*/
	if (m_admin_chat) {
		if (!m_admin_chat->command_queue.empty()) {
			MutexAutoLock lock(m_env_mutex);
			while (!m_admin_chat->command_queue.empty()) {
				ChatEvent *evt = m_admin_chat->command_queue.pop_frontNoEx();
				handleChatInterfaceEvent(evt);
				delete evt;
			}
		}
		m_admin_chat->outgoing_queue.push_back(
			new ChatEventTimeInfo(m_env->getGameTime(), m_env->getTimeOfDay()));
	}

	/*
		Do background stuff
	*/

	if (!m_more_threads)
		AsyncRunMapStep(dtime, dedicated_server_step, false);

<<<<<<< HEAD
	m_clients.step(dtime);

	m_lag += (m_lag > dtime ? -1 : 1) * dtime/100;
=======
		MutexAutoLock lock(m_env_mutex);

		ScopeProfiler sp(g_profiler, "Server: liquid transform");

		std::map<v3s16, MapBlock*> modified_blocks;
		m_env->getMap().transformLiquids(modified_blocks, m_env);

		/*
			Set the modified blocks unsent for all the clients
		*/
		if (!modified_blocks.empty()) {
			SetBlocksNotSent(modified_blocks);
		}
	}
	m_clients.step(dtime);

	// increase/decrease lag gauge gradually
	if (m_lag_gauge->get() > dtime) {
		m_lag_gauge->decrement(dtime/100);
	} else {
		m_lag_gauge->increment(dtime/100);
	}

	{
		float &counter = m_step_pending_dyn_media_timer;
		counter += dtime;
		if (counter >= 5.0f) {
			stepPendingDynMediaCallbacks(counter);
			counter = 0;
		}
	}


#if USE_CURL
>>>>>>> 5.5.0
	// send masterserver announce
	{
		float &counter = m_masterserver_timer;
		if (!isSingleplayer() && (!counter || counter >= 300.0) &&
				g_settings->getBool("server_announce")) {
			ServerList::sendAnnounce(counter ? ServerList::AA_UPDATE :
						ServerList::AA_START,
					m_bind_addr.getPort(),
					m_clients.getPlayerNames(),
					m_uptime_counter->get(),
					m_env->getGameTime(),
					m_lag_gauge->get(),
					m_gamespec.id,
					Mapgen::getMapgenName(m_emerge->mgparams->mgtype),
					m_modmgr->getMods(),
					m_dedicated);
			counter = 0.01;
		}
		counter += dtime;
	}

	/*
		Check added and deleted active objects
	*/
	{
		TimeTaker timer_step("Server step: Check added and deleted active objects");
		//infostream<<"Server: Checking added and deleted active objects"<<std::endl;
		//MutexAutoLock envlock(m_env_mutex);

<<<<<<< HEAD
		auto clients = m_clients.getClientList();
		ScopeProfiler sp(g_profiler, "Server: checking added and deleted objs");
=======
		m_clients.lock();
		const RemoteClientMap &clients = m_clients.getClientList();
		ScopeProfiler sp(g_profiler, "Server: update objects within range");
>>>>>>> 5.5.0

		m_player_gauge->set(clients.size());
		for (const auto &client_it : clients) {
			RemoteClient *client = client_it.second;

<<<<<<< HEAD
		// Radius inside which players are active
		static const bool is_transfer_limited =
			g_settings->exists("unlimited_player_transfer_distance") &&
			!g_settings->getBool("unlimited_player_transfer_distance");
		static const s16 player_transfer_dist = g_settings->getS16("player_transfer_distance") * MAP_BLOCKSIZE;
		s16 player_radius = player_transfer_dist;
		if (player_radius == 0 && is_transfer_limited)
			player_radius = radius;

		for(auto & client : clients) {

			// If definitions and textures have not been sent, don't
			// send objects either
=======
>>>>>>> 5.5.0
			if (client->getState() < CS_DefinitionsSent)
				continue;

			// This can happen if the client times out somehow
			if (!m_env->getPlayer(client->peer_id))
				continue;

<<<<<<< HEAD
			s16 my_radius = MYMIN(radius, playersao->getWantedRange() * MAP_BLOCKSIZE);
			if (my_radius <= 0) my_radius = radius;
			my_radius *= 1.5;
			//infostream << "Server: Active Radius " << my_radius << std::endl;

			std::queue<u16> removed_objects;
			std::queue<u16> added_objects;
			m_env->getRemovedActiveObjects(playersao, my_radius, player_radius,
					client->m_known_objects, removed_objects);
			m_env->getAddedActiveObjects(playersao, my_radius, player_radius,
					client->m_known_objects, added_objects);

			// Ignore if nothing happened
			if (removed_objects.empty() && added_objects.empty()) {
=======
			PlayerSAO *playersao = getPlayerSAO(client->peer_id);
			if (!playersao)
>>>>>>> 5.5.0
				continue;

<<<<<<< HEAD
#if MINETEST_PROTO

			std::string data_buffer;

			char buf[4];

			// Handle removed objects
			writeU16((u8*)buf, removed_objects.size());
			data_buffer.append(buf, 2);
			while (!removed_objects.empty()) {
				// Get object
				u16 id = removed_objects.front();
				ServerActiveObject* obj = m_env->getActiveObject(id, true);

				// Add to data buffer for sending
				writeU16((u8*)buf, id);
				data_buffer.append(buf, 2);

				// Remove from known objects
				client->m_known_objects.erase(id);

				if(obj && obj->m_known_by_count > 0)
					obj->m_known_by_count--;
				removed_objects.pop();
			}

			// Handle added objects
			writeU16((u8*)buf, added_objects.size());
			data_buffer.append(buf, 2);
			while (!added_objects.empty()) {
				// Get object
				u16 id = added_objects.front();
				ServerActiveObject* obj = m_env->getActiveObject(id);

				// Get object type
				u8 type = ACTIVEOBJECT_TYPE_INVALID;
				if(obj == NULL)
					warningstream<<FUNCTION_NAME
							<<": NULL object"<<std::endl;
				else
					type = obj->getSendType();

				// Add to data buffer for sending
				writeU16((u8*)buf, id);
				data_buffer.append(buf, 2);
				writeU8((u8*)buf, type);
				data_buffer.append(buf, 1);

				if(obj)
					data_buffer.append(serializeLongString(
							obj->getClientInitializationData(client->net_proto_version)));
				else
					data_buffer.append(serializeLongString(""));

				// Add to known objects
				client->m_known_objects.set(id, true);

				if(obj)
					obj->m_known_by_count++;

				added_objects.pop();
			}

			u32 pktSize = SendActiveObjectRemoveAdd(client->peer_id, data_buffer);
			verbosestream << "Server: Sent object remove/add: "
					<< removed_objects.size() << " removed, "
					<< added_objects.size() << " added, "
					<< "packet size is " << pktSize << std::endl;



#else


			std::set<u16> removed_objects_data;

			// Handle removed objects
			while (!removed_objects.empty()) {
				// Get object
				u16 id = removed_objects.front();
				ServerActiveObject* obj = m_env->getActiveObject(id, true);

				// Remove from known objects
				client->m_known_objects.erase(id);

				if(obj && obj->m_known_by_count > 0)
					obj->m_known_by_count--;

				removed_objects_data.insert(id);
				removed_objects.pop();
			}

			std::vector<ActiveObjectAddData> added_objects_data;

			// Handle added objects
			while (!added_objects.empty()) {
				// Get object
				u16 id = added_objects.front();
				added_objects.pop();

				ServerActiveObject* obj = m_env->getActiveObject(id);
				if(!obj) {
					warningstream<<FUNCTION_NAME
							<<": NULL object"<<std::endl;
					continue;
				}
				// Get object type
				u8 type = obj->getSendType();

				std::string data = obj->getClientInitializationData(client->net_proto_version);
				if (!data.size())
					continue;

				added_objects_data.push_back(ActiveObjectAddData(id, type, data));

				// Add to known objects
				client->m_known_objects.set(id, true);

				obj->m_known_by_count++;

			}

			MSGPACK_PACKET_INIT((int)TOCLIENT_ACTIVE_OBJECT_REMOVE_ADD, 2);
			PACK(TOCLIENT_ACTIVE_OBJECT_REMOVE_ADD_REMOVE, removed_objects_data);
			PACK(TOCLIENT_ACTIVE_OBJECT_REMOVE_ADD_ADD, added_objects_data);

			// Send as reliable
			m_clients.send(client->peer_id, 0, buffer, true);
#endif

		}
=======
			SendActiveObjectRemoveAdd(client, playersao);
		}
		m_clients.unlock();

		// Write changes to the mod storage
		m_mod_storage_save_timer -= dtime;
		if (m_mod_storage_save_timer <= 0.0f) {
			m_mod_storage_save_timer = g_settings->getFloat("server_map_save_interval");
			m_mod_storage_database->endSave();
			m_mod_storage_database->beginSave();
		}
>>>>>>> 5.5.0
	}

	/*
		Send object messages
	*/
	{
<<<<<<< HEAD
		TimeTaker timer_step("Server step: Send object messages");
		//MutexAutoLock envlock(m_env_mutex);
		ScopeProfiler sp(g_profiler, "Server: sending object messages");
=======
		MutexAutoLock envlock(m_env_mutex);
		ScopeProfiler sp(g_profiler, "Server: send SAO messages");
>>>>>>> 5.5.0

		// Key = object id
		// Value = data sent by object
		std::unordered_map<u16, std::vector<ActiveObjectMessage>*> buffered_messages;

		// Get active object messages from environment
		ActiveObjectMessage aom(0);
		u32 aom_count = 0;
		for(;;) {
			if (!m_env->getActiveObjectMessage(&aom))
				break;

			std::vector<ActiveObjectMessage>* message_list = nullptr;
			auto n = buffered_messages.find(aom.id);
			if (n == buffered_messages.end()) {
				message_list = new std::vector<ActiveObjectMessage>;
				buffered_messages[aom.id] = message_list;
			} else {
				message_list = n->second;
			}
			message_list->push_back(std::move(aom));
			aom_count++;
		}

<<<<<<< HEAD
		auto clients = m_clients.getClientList();
		// Route data to every client
		for (auto & client : clients) {

#if MINETEST_PROTO
			std::string reliable_data;
			std::string unreliable_data;
=======
		m_aom_buffer_counter->increment(aom_count);

		m_clients.lock();
		const RemoteClientMap &clients = m_clients.getClientList();
		// Route data to every client
		std::string reliable_data, unreliable_data;
		for (const auto &client_it : clients) {
			reliable_data.clear();
			unreliable_data.clear();
			RemoteClient *client = client_it.second;
			PlayerSAO *player = getPlayerSAO(client->peer_id);
>>>>>>> 5.5.0
			// Go through all objects in message buffer
			for (const auto &buffered_message : buffered_messages) {
				// If object does not exist or is not known by client, skip it
				u16 id = buffered_message.first;
				ServerActiveObject *sao = m_env->getActiveObject(id);
				if (!sao || client->m_known_objects.find(id) == client->m_known_objects.end())
					continue;

				// Get message list of object
				std::vector<ActiveObjectMessage>* list = buffered_message.second;
				// Go through every message
				for (const ActiveObjectMessage &aom : *list) {
					// Send position updates to players who do not see the attachment
					if (aom.datastring[0] == AO_CMD_UPDATE_POSITION) {
						if (sao->getId() == player->getId())
							continue;

						// Do not send position updates for attached players
						// as long the parent is known to the client
						ServerActiveObject *parent = sao->getParent();
						if (parent && client->m_known_objects.find(parent->getId()) !=
								client->m_known_objects.end())
							continue;
					}

					// Add full new data to appropriate buffer
					std::string &buffer = aom.reliable ? reliable_data : unreliable_data;
					char idbuf[2];
					writeU16((u8*) idbuf, aom.id);
					// u16 id
					// std::string data
					buffer.append(idbuf, sizeof(idbuf));
					buffer.append(serializeString16(aom.datastring));
				}
			}
			/*
				reliable_data and unreliable_data are now ready.
				Send them.
			*/
			if (!reliable_data.empty()) {
				SendActiveObjectMessages(client->peer_id, reliable_data);
			}

			if (!unreliable_data.empty()) {
				SendActiveObjectMessages(client->peer_id, unreliable_data, false);
			}

#else
			ActiveObjectMessages reliable_data;
			ActiveObjectMessages unreliable_data;
			// Go through all objects in message buffer
			for(auto
					j = buffered_messages.begin();
					j != buffered_messages.end(); ++j) {
				// If object is not known by client, skip it
				u16 id = j->first;
				if (client->m_known_objects.find(id) == client->m_known_objects.end())
					continue;
				// Get message list of object
				std::vector<ActiveObjectMessage>* list = j->second;
				// Go through every message
				for(auto
						k = list->begin(); k != list->end(); ++k) {
					// Add data to buffer
					if(k->reliable)
						reliable_data.push_back(make_pair(k->id, k->datastring));
					else
						unreliable_data.push_back(make_pair(k->id, k->datastring));
				}
			}
			/*
				reliable_data and unreliable_data are now ready.
				Send them.
			*/
			if(reliable_data.size() > 0) {
				SendActiveObjectMessages(client->peer_id, reliable_data);
			}
			if(unreliable_data.size() > 0) {
				SendActiveObjectMessages(client->peer_id, unreliable_data, false);
			}
#endif
		}
		// Clear buffered_messages
<<<<<<< HEAD
		for (auto
				i = buffered_messages.begin();
				i != buffered_messages.end(); ++i) {
			delete i->second;
=======
		for (auto &buffered_message : buffered_messages) {
			delete buffered_message.second;
>>>>>>> 5.5.0
		}
	}

	/*
		Send queued-for-sending map edit events.
	*/
	{
		TimeTaker timer_step("Server step: Send queued-for-sending map edit events.");
		ScopeProfiler sp(g_profiler, "Server: Map events process");
		// We will be accessing the environment
		//MutexAutoLock lock(m_env_mutex);

		// Don't send too many at a time
		u32 count = 0;

		// Single change sending is disabled if queue size is not small
		bool disable_single_change_sending = false;
		if(m_unsent_map_edit_queue.size() > 1)
			disable_single_change_sending = true;

		//int event_count = m_unsent_map_edit_queue.size();

		// We'll log the amount of each
		Profiler prof;

<<<<<<< HEAD
		u32 end_ms = porting::getTimeMs() + max_cycle_ms;
#if !ENABLE_THREADS
		auto lock = m_env->getMap().m_nothread_locker.lock_shared_rec();
		if (lock->owns_lock())
#endif
		while(m_unsent_map_edit_queue.size() != 0)
		{
			auto event = std::unique_ptr<MapEditEvent>(m_unsent_map_edit_queue.pop_front());
=======
		std::list<v3s16> node_meta_updates;

		while (!m_unsent_map_edit_queue.empty()) {
			MapEditEvent* event = m_unsent_map_edit_queue.front();
			m_unsent_map_edit_queue.pop();
>>>>>>> 5.5.0

			// Players far away from the change are stored here.
			// Instead of sending the changes, MapBlocks are set not sent
			// for them.
			std::unordered_set<u16> far_players;

			if(event->type == MEET_ADDNODE || event->type == MEET_SWAPNODE) {
				//infostream<<"Server: MEET_ADDNODE"<<std::endl;
				prof.add("MEET_ADDNODE", 1);
<<<<<<< HEAD
				if(disable_single_change_sending)
					sendAddNode(event->p, event->n, event->already_known_by_peer,
							&far_players, 5, event->type == MEET_ADDNODE);
				else
					sendAddNode(event->p, event->n, event->already_known_by_peer,
							&far_players, 30, event->type == MEET_ADDNODE);
			}
			else if(event->type == MEET_REMOVENODE) {
				//infostream<<"Server: MEET_REMOVENODE"<<std::endl;
				prof.add("MEET_REMOVENODE", 1);
				if(disable_single_change_sending)
					sendRemoveNode(event->p, event->already_known_by_peer,
							&far_players, 5);
				else
					sendRemoveNode(event->p, event->already_known_by_peer,
							&far_players, 30);
			}
			else if(event->type == MEET_BLOCK_NODE_METADATA_CHANGED) {
/*
				infostream<<"Server: MEET_BLOCK_NODE_METADATA_CHANGED"<<std::endl;
*/
				prof.add("MEET_BLOCK_NODE_METADATA_CHANGED", 1);
				setBlockNotSent(event->p);
			}
			else if(event->type == MEET_OTHER) {
/*
				infostream<<"Server: MEET_OTHER"<<std::endl;
*/
				prof.add("MEET_OTHER", 1);
/*
				for(std::set<v3s16>::iterator
						i = event->modified_blocks.begin();
						i != event->modified_blocks.end(); ++i)
				{
					setBlockNotSent(*i);
=======
				sendAddNode(event->p, event->n, &far_players,
						disable_single_change_sending ? 5 : 30,
						event->type == MEET_ADDNODE);
				break;
			case MEET_REMOVENODE:
				prof.add("MEET_REMOVENODE", 1);
				sendRemoveNode(event->p, &far_players,
						disable_single_change_sending ? 5 : 30);
				break;
			case MEET_BLOCK_NODE_METADATA_CHANGED: {
				prof.add("MEET_BLOCK_NODE_METADATA_CHANGED", 1);
				if (!event->is_private_change) {
					// Don't send the change yet. Collect them to eliminate dupes.
					node_meta_updates.remove(event->p);
					node_meta_updates.push_back(event->p);
				}

				if (MapBlock *block = m_env->getMap().getBlockNoCreateNoEx(
						getNodeBlockPos(event->p))) {
					block->raiseModified(MOD_STATE_WRITE_NEEDED,
						MOD_REASON_REPORT_META_CHANGE);
				}
				break;
			}
			case MEET_OTHER:
				prof.add("MEET_OTHER", 1);
				for (const v3s16 &modified_block : event->modified_blocks) {
					m_clients.markBlockposAsNotSent(modified_block);
>>>>>>> 5.5.0
				}
*/
				SetBlocksNotSent();
			}
			else {
				prof.add("unknown", 1);
				warningstream << "Server: Unknown MapEditEvent "
						<< ((u32)event->type) << std::endl;
				//break;
			}

			/*
				Set blocks not sent to far players
			*/
			if (!far_players.empty()) {
				// Convert list format to that wanted by SetBlocksNotSent
/*
				std::map<v3s16, MapBlock*> modified_blocks2;
				for (const v3s16 &modified_block : event->modified_blocks) {
					modified_blocks2[modified_block] =
							m_env->getMap().getBlockNoCreateNoEx(modified_block);
				}
*/
				// Set blocks not sent
<<<<<<< HEAD
				for (auto
						i = far_players.begin();
						i != far_players.end(); ++i) {
					u16 peer_id = *i;
					RemoteClient *client = getClient(peer_id);
					if(client==NULL)
						continue;
					client->SetBlocksNotSent(/*modified_blocks2*/);
				}
			}

			//delete event;

			++count;
			/*// Don't send too many at a time
			if(count >= 1 && m_unsent_map_edit_queue.size() < 100)
				break;*/
			if (porting::getTimeMs() > end_ms)
				break;
		}

/*
		if(event_count >= 10){
			infostream<<"Server: MapEditEvents count="<<count<<"/"<<event_count<<" :"<<std::endl;
			prof.print(infostream);
		} else if(event_count != 0){
			verbosestream<<"Server: MapEditEvents count="<<count<<"/"<<event_count<<" :"<<std::endl;
=======
				for (const u16 far_player : far_players) {
					if (RemoteClient *client = getClient(far_player))
						client->SetBlocksNotSent(modified_blocks2);
				}
			}

			delete event;
		}

		if (event_count >= 5) {
			infostream << "Server: MapEditEvents:" << std::endl;
			prof.print(infostream);
		} else if (event_count != 0) {
			verbosestream << "Server: MapEditEvents:" << std::endl;
>>>>>>> 5.5.0
			prof.print(verbosestream);
		}
*/

		// Send all metadata updates
		if (node_meta_updates.size())
			sendMetadataChanged(node_meta_updates);
	}

	/*
		Trigger emerge thread
		Doing this every 2s is left over from old code, unclear if this is still needed.
	*/
/*
	if (!maintenance_status)
	{
		TimeTaker timer_step("Server step: Trigger emergethread");
		float &counter = m_emergethread_trigger_timer;
		counter -= dtime;
		if (counter <= 0.0f) {
			counter = 2.0f;

			m_emerge->startThreads();
		}
	}
*/

	{
		if (porting::g_sighup) {
			porting::g_sighup = false;
			if(!maintenance_status) {
				maintenance_status = 1;
				maintenance_start();
				maintenance_status = 2;
			} else if(maintenance_status == 2) {
				maintenance_status = 3;
				maintenance_end();
				maintenance_status = 0;
			}
		}
		if (porting::g_siginfo) {
			// todo: add here more info
			porting::g_siginfo = false;
			infostream<<"uptime="<< (int)m_uptime.get()<<std::endl;
			m_clients.UpdatePlayerList(); //print list
			g_profiler->print(infostream);
			g_profiler->clear();
		}
	}
}

int Server::save(float dtime, float dedicated_server_step, bool breakable) {
	// Save map, players and auth stuff
	int ret = 0;
		float &counter = m_savemap_timer;
		counter += dtime;
		static thread_local const float save_interval =
			g_settings->getFloat("server_map_save_interval");
		if (counter >= save_interval) {
			counter = 0.0;
			TimeTaker timer_step("Server step: Save map, players and auth stuff");
			//MutexAutoLock lock(m_env_mutex);

			ScopeProfiler sp(g_profiler, "Server: map saving (sum)");

			// Save changed parts of map
			if(m_env->getMap().save(MOD_STATE_WRITE_NEEDED, dedicated_server_step, breakable)) {
				// partial save, will continue on next step
				counter = g_settings->getFloat("server_map_save_interval");
				++ret;
				if (breakable)
					goto save_break;
			}

			// Save ban file
			if (m_banmanager->isModified()) {
				m_banmanager->save();
			}

			// Save players
			m_env->saveLoadedPlayers();

			// Save environment metadata
			m_env->saveMeta();

			stat.save();
		}
<<<<<<< HEAD
		save_break:;

	return ret;
=======
	}

	m_shutdown_state.tick(dtime, this);
>>>>>>> 5.5.0
}

u16 Server::Receive(int ms)
{
<<<<<<< HEAD
	DSTACK(FUNCTION_NAME);
	SharedBuffer<u8> data;
	u16 peer_id = 0;
	u16 received = 0;
	try {
		TimeTaker timer_step("Server recieve one packet");

		NetworkPacket pkt;
		auto size = m_con.Receive(&pkt, ms);
		peer_id = pkt.getPeerId();
		if (size) {
			ProcessData(&pkt);
			++received;
		}
	}
	catch(con::InvalidIncomingDataException &e) {
		infostream<<"Server::Receive(): "
				"InvalidIncomingDataException: what()="
				<<e.what()<<std::endl;
	}
	catch(SerializationError &e) {
		infostream<<"Server::Receive(): "
				"SerializationError: what()="
				<<e.what()<<std::endl;
	}
	catch(ClientStateError &e) {
		errorstream << "ProcessData: peer=" << peer_id  << e.what() << std::endl;
		DenyAccess_Legacy(peer_id, L"Your client sent something server didn't expect."
				L"Try reconnecting or updating your client");
	}
	catch(con::PeerNotFoundException &e) {
		// Do nothing
	} catch (ClientNotFoundException &e) {
		//verbosestream<<"Server: recieve: clientnotfound:"<< e.what() <<std::endl;
	} catch (msgpack::v1::type_error &e) {
		verbosestream<<"Server: recieve: msgpack:"<< e.what() <<std::endl;
	} catch (std::exception &e) {
#if !MINETEST_PROTO
		infostream<<"Server: recieve: exception:"<< e.what() <<std::endl;
#endif
=======
	NetworkPacket pkt;
	session_t peer_id;
	bool first = true;
	for (;;) {
		pkt.clear();
		peer_id = 0;
		try {
			/*
				In the first iteration *wait* for a packet, afterwards process
				all packets that are immediately available (no waiting).
			*/
			if (first) {
				m_con->Receive(&pkt);
				first = false;
			} else {
				if (!m_con->TryReceive(&pkt))
					return;
			}

			peer_id = pkt.getPeerId();
			m_packet_recv_counter->increment();
			ProcessData(&pkt);
			m_packet_recv_processed_counter->increment();
		} catch (const con::InvalidIncomingDataException &e) {
			infostream << "Server::Receive(): InvalidIncomingDataException: what()="
					<< e.what() << std::endl;
		} catch (const SerializationError &e) {
			infostream << "Server::Receive(): SerializationError: what()="
					<< e.what() << std::endl;
		} catch (const ClientStateError &e) {
			errorstream << "ProcessData: peer=" << peer_id << " what()="
					 << e.what() << std::endl;
			DenyAccess_Legacy(peer_id, L"Your client sent something server didn't expect."
					L"Try reconnecting or updating your client");
		} catch (const con::PeerNotFoundException &e) {
			// Do nothing
		} catch (const con::NoIncomingDataException &e) {
			return;
		}
>>>>>>> 5.5.0
	}
	return received;
}

PlayerSAO* Server::StageTwoClientInit(session_t peer_id)
{
	std::string playername;
	PlayerSAO *playersao = NULL;
		RemoteClient* client = m_clients.lockedGetClientNoEx(peer_id, CS_InitDone);
		if (client) {
			playername = client->getName();
			playersao = emergePlayer(playername.c_str(), peer_id, client->net_proto_version);
		}

<<<<<<< HEAD
	RemotePlayer *player =
		static_cast<RemotePlayer*>(m_env->getPlayer(playername));
=======
	RemotePlayer *player = m_env->getPlayer(playername.c_str());
>>>>>>> 5.5.0

	// If failed, cancel
	if (!playersao || !player) {
		if (player && player->getPeerId() != PEER_ID_INEXISTENT) {
			actionstream << "Server: Failed to emerge player \"" << playername
					<< "\" (player allocated to an another client)" << std::endl;
			DenyAccess_Legacy(peer_id, L"Another client is connected with this "
					L"name. If your client closed unexpectedly, try again in "
					L"a minute.");
		} else {
			errorstream << "Server: " << playername << ": Failed to emerge player"
					<< std::endl;
			DenyAccess_Legacy(peer_id, L"Could not allocate player.");
		}
		return NULL;
	}

	/*
		Send complete position information
	*/
	SendMovePlayer(peer_id);

	// Send privileges
	SendPlayerPrivileges(peer_id);

	// Send inventory formspec
	SendPlayerInventoryFormspec(peer_id);

	// Send inventory
	SendInventory(playersao, false);

	// Send HP
	SendPlayerHP(playersao);

	// Send death screen
	if (playersao->isDead())
		SendDeathscreen(peer_id, false, v3f(0,0,0));

<<<<<<< HEAD
	// Note things in chat if not in simple singleplayer mode
	if(!m_simple_singleplayer_mode) {
		// Send information about server to player in chat
		SendChatMessage(peer_id, getStatusString());
	}

/*
	Address addr = getPeerAddress(player->peer_id);
	std::string ip_str = addr.serializeString();
	actionstream<<player->getName() <<" [" << ip_str << "] joins game. " << std::endl;
*/
=======
	// Send Breath
	SendPlayerBreath(playersao);

>>>>>>> 5.5.0
	/*
		Print out action
	*/
	{
		Address addr = getPeerAddress(player->getPeerId());
		std::string ip_str = addr.serializeString();
		const std::vector<std::string> &names = m_clients.getPlayerNames();

<<<<<<< HEAD
		actionstream << player->getName() << " [" << getPeerAddress(peer_id).serializeString() << "]" 
			<< " joins game. List of players: ";
=======
		actionstream << player->getName() << " [" << ip_str << "] joins game. List of players: ";
>>>>>>> 5.5.0

		for (const std::string &name : names) {
			actionstream << name << " ";
		}

		actionstream << player->getName() <<std::endl;
	}
	return playersao;
}

inline void Server::handleCommand(NetworkPacket *pkt)
{
	const ToServerCommandHandler &opHandle = toServerCommandTable[pkt->getCommand()];
	(this->*opHandle.handler)(pkt);
}

void Server::ProcessData(NetworkPacket *pkt)
{
	// Environment is locked first.
	//MutexAutoLock envlock(m_env_mutex);

	ScopeProfiler sp(g_profiler, "Server: Process network packet (sum)");
	u32 peer_id = pkt->getPeerId();

	try {
		Address address = getPeerAddress(peer_id);
		std::string addr_s = address.serializeString();

		if(m_banmanager->isIpBanned(addr_s)) {
			std::string ban_name = m_banmanager->getBanName(addr_s);
			infostream << "Server: A banned client tried to connect from "
					<< addr_s << "; banned name was "
					<< ban_name << std::endl;
			// This actually doesn't seem to transfer to the client
			DenyAccess_Legacy(peer_id, L"Your ip is banned. Banned name was "
					+ utf8_to_wide(ban_name));
			return;
		}
	}
	catch(con::PeerNotFoundException &e) {
		/*
		 * no peer for this packet found
		 * most common reason is peer timeout, e.g. peer didn't
		 * respond for some time, your server was overloaded or
		 * things like that.
		 */
		infostream << "Server::ProcessData(): Canceling: peer "
				<< peer_id << " not found" << std::endl;
		return;
	}

	try {
#if !MINETEST_PROTO
		if (!pkt->packet_unpack())
			return;
#endif

		ToServerCommand command = (ToServerCommand) pkt->getCommand();

		// Command must be handled into ToServerCommandHandler
		if (command >= TOSERVER_NUM_MSG_TYPES) {
			infostream << "Server: Ignoring unknown command "
					 << command << std::endl;
			return;
		}

		if (overload) {
			if (command == TOSERVER_PLAYERPOS || command == TOSERVER_DRAWCONTROL)
				return;
			if (overload > 2000 && command == TOSERVER_BREATH)
				return;
			if (overload > 30000 && command == TOSERVER_INTERACT) // FMTODO queue here for post-process
				return;
			//errorstream << "overload cmd=" << command << " n="<< toServerCommandTable[command].name << "\n";
		}

		if (toServerCommandTable[command].state == TOSERVER_STATE_NOT_CONNECTED) {
			handleCommand(pkt);
			return;
		}

		u8 peer_ser_ver = getClient(peer_id, CS_InitDone)->serialization_version;

		if(peer_ser_ver == SER_FMT_VER_INVALID) {
			errorstream << "Server::ProcessData(): Cancelling: Peer"
					" serialization format invalid or not initialized."
					" Skipping incoming command=" << command << std::endl;
			return;
		}

		/* Handle commands related to client startup */
		if (toServerCommandTable[command].state == TOSERVER_STATE_STARTUP) {
			handleCommand(pkt);
			return;
		}

		if (m_clients.getClientState(peer_id) < CS_Active) {
			if (command == TOSERVER_PLAYERPOS) return;

			errorstream << "Got packet command: " << command << " for peer id "
					<< peer_id << " but client isn't active yet. Dropping packet "
					<< std::endl;
			return;
		}

		handleCommand(pkt);
	} catch (SendFailedException &e) {
		errorstream << "Server::ProcessData(): SendFailedException: "
				<< "what=" << e.what()
				<< std::endl;
	} catch (PacketError &e) {
		actionstream << "Server::ProcessData(): PacketError: "
				<< "what=" << e.what()
				<< std::endl;
	}
}

void Server::setTimeOfDay(u32 time)
{
	m_env->setTimeOfDay(time);
	m_time_of_day_send_timer = 0;
}

void Server::onMapEditEvent(const MapEditEvent &event)
{
<<<<<<< HEAD
	//infostream<<"Server::onMapEditEvent()"<<std::endl;
	if(m_ignore_map_edit_events)
		return;
/* thread unsafe
	if(m_ignore_map_edit_events_area.contains(event->getArea()))
		return;
*/
	MapEditEvent *e = event->clone();
	m_unsent_map_edit_queue.push(e);
}

Inventory* Server::getInventory(const InventoryLocation &loc)
{
	switch (loc.type) {
	case InventoryLocation::UNDEFINED:
	case InventoryLocation::CURRENT_PLAYER:
		break;
	case InventoryLocation::PLAYER:
	{
		RemotePlayer *player = dynamic_cast<RemotePlayer *>(m_env->getPlayer(loc.name.c_str()));
		if(!player)
			return NULL;
		PlayerSAO *playersao = player->getPlayerSAO();
		if(!playersao)
			return NULL;
		return playersao->getInventory();
	}
		break;
	case InventoryLocation::NODEMETA:
	{
		NodeMetadata *meta = m_env->getMap().getNodeMetadata(loc.p);
		if(!meta)
			return NULL;
		return meta->getInventory();
	}
		break;
	case InventoryLocation::DETACHED:
	{
		if(m_detached_inventories.count(loc.name) == 0)
			return NULL;
		return m_detached_inventories[loc.name];
	}
		break;
	default:
		break;
	}
	return NULL;
}
void Server::setInventoryModified(const InventoryLocation &loc, bool playerSend)
{
	switch(loc.type){
	case InventoryLocation::UNDEFINED:
		break;
	case InventoryLocation::PLAYER:
	{
		if (!playerSend)
			return;

		RemotePlayer *player =
			dynamic_cast<RemotePlayer *>(m_env->getPlayer(loc.name.c_str()));

		if (!player)
			return;

		PlayerSAO *playersao = player->getPlayerSAO();
		if(!playersao)
			return;

		SendInventory(playersao);
	}
	break;
	case InventoryLocation::NODEMETA:
	{
		v3s16 blockpos = getNodeBlockPos(loc.p);

		MapBlock *block = m_env->getMap().getBlockNoCreateNoEx(blockpos, false, true);
		if(block)
			block->raiseModified(MOD_STATE_WRITE_NEEDED, MOD_REASON_REPORT_META_CHANGE);

		setBlockNotSent(blockpos);
	}
	break;
	case InventoryLocation::DETACHED:
	{
		sendDetachedInventory(loc.name,PEER_ID_INEXISTENT);
	}
	break;
	default:
		break;
	}
=======
	if (m_ignore_map_edit_events_area.contains(event.getArea()))
		return;

	m_unsent_map_edit_queue.push(new MapEditEvent(event));
>>>>>>> 5.5.0
}

void Server::SetBlocksNotSent(std::map<v3s16, MapBlock *>& block)
{
<<<<<<< HEAD
	SetBlocksNotSent();
=======
	std::vector<session_t> clients = m_clients.getClientIDs();
	m_clients.lock();
	// Set the modified blocks unsent for all the clients
	for (const session_t client_id : clients) {
			if (RemoteClient *client = m_clients.lockedGetClientNoEx(client_id))
				client->SetBlocksNotSent(block);
	}
	m_clients.unlock();
>>>>>>> 5.5.0
}

void Server::SetBlocksNotSent()
{
	std::vector<u16> clients = m_clients.getClientIDs();
	// Set the modified blocks unsent for all the clients
	for (auto
		 i = clients.begin();
		 i != clients.end(); ++i) {
			if (RemoteClient *client = m_clients.lockedGetClientNoEx(*i))
				client->SetBlocksNotSent();
	}
}

void Server::peerAdded(u16 peer_id)
{
	verbosestream<<"Server::peerAdded(): peer->id="
			<<peer_id<<std::endl;

<<<<<<< HEAD
	con::PeerChange c;
	c.type = con::PEER_ADDED;
	c.peer_id = peer_id;
	c.timeout = false;
	m_peer_change_queue.push(c);
=======
	m_peer_change_queue.push(con::PeerChange(con::PEER_ADDED, peer->id, false));
>>>>>>> 5.5.0
}

void Server::deletingPeer(u16 peer_id, bool timeout)
{
	verbosestream<<"Server::deletingPeer(): peer->id="
			<<peer_id<<", timeout="<<timeout<<std::endl;

<<<<<<< HEAD
	m_clients.event(peer_id, CSE_Disconnect);
	con::PeerChange c;
	c.type = con::PEER_REMOVED;
	c.peer_id = peer_id;
	c.timeout = timeout;
	m_peer_change_queue.push(c);
=======
	m_clients.event(peer->id, CSE_Disconnect);
	m_peer_change_queue.push(con::PeerChange(con::PEER_REMOVED, peer->id, timeout));
>>>>>>> 5.5.0
}

bool Server::getClientConInfo(session_t peer_id, con::rtt_stat_type type, float* retval)
{
	*retval = m_con->getPeerStat(peer_id,type);
	return *retval != -1;
}

bool Server::getClientInfo(session_t peer_id, ClientInfo &ret)
{
<<<<<<< HEAD
	*state = m_clients.getClientState(peer_id);
	RemoteClient* client = m_clients.lockedGetClientNoEx(peer_id, CS_Invalid);

	if (client == NULL) {
=======
	m_clients.lock();
	RemoteClient* client = m_clients.lockedGetClientNoEx(peer_id, CS_Invalid);

	if (!client) {
		m_clients.unlock();
>>>>>>> 5.5.0
		return false;
	}

	ret.state = client->getState();
	ret.addr = client->getAddress();
	ret.uptime = client->uptime();
	ret.ser_vers = client->serialization_version;
	ret.prot_vers = client->net_proto_version;

	ret.major = client->getMajor();
	ret.minor = client->getMinor();
	ret.patch = client->getPatch();
	ret.vers_string = client->getFullVer();

	ret.lang_code = client->getLangCode();

	return true;
}

void Server::handlePeerChanges()
{
	while(!m_peer_change_queue.empty())
	{
		con::PeerChange c = m_peer_change_queue.pop_front();

		verbosestream<<"Server: Handling peer change: "
				<<"id="<<c.peer_id<<", timeout="<<c.timeout
				<<std::endl;

		switch(c.type)
		{
			case con::PEER_ADDED:
				m_clients.CreateClient(c.peer_id);
				break;

			case con::PEER_REMOVED:
				DeleteClient(c.peer_id, c.timeout?CDR_TIMEOUT:CDR_LEAVE);
				break;

			default:
				assert("Invalid peer change event received!" == 0);
				break;
		}
	}
}

void Server::printToConsoleOnly(const std::string &text)
{
	if (m_admin_chat) {
		m_admin_chat->outgoing_queue.push_back(
			new ChatEventChat("", utf8_to_wide(text)));
	} else {
		std::cout << text << std::endl;
	}
}

<<<<<<< HEAD
#if MINETEST_PROTO
void Server::Send(NetworkPacket* pkt)
{
	g_profiler->add("Server: Packets sended", 1);
	m_clients.send(pkt->getPeerId(),
=======
void Server::Send(NetworkPacket *pkt)
{
	Send(pkt->getPeerId(), pkt);
}

void Server::Send(session_t peer_id, NetworkPacket *pkt)
{
	m_clients.send(peer_id,
>>>>>>> 5.5.0
		clientCommandFactoryTable[pkt->getCommand()].channel,
		pkt,
		clientCommandFactoryTable[pkt->getCommand()].reliable);
}

void Server::SendMovement(session_t peer_id)
{
	std::ostringstream os(std::ios_base::binary);

	NetworkPacket pkt(TOCLIENT_MOVEMENT, 12 * sizeof(float), peer_id);

	pkt << g_settings->getFloat("movement_acceleration_default");
	pkt << g_settings->getFloat("movement_acceleration_air");
	pkt << g_settings->getFloat("movement_acceleration_fast");
	pkt << g_settings->getFloat("movement_speed_walk");
	pkt << g_settings->getFloat("movement_speed_crouch");
	pkt << g_settings->getFloat("movement_speed_fast");
	pkt << g_settings->getFloat("movement_speed_climb");
	pkt << g_settings->getFloat("movement_speed_jump");
	pkt << g_settings->getFloat("movement_liquid_fluidity");
	pkt << g_settings->getFloat("movement_liquid_fluidity_smooth");
	pkt << g_settings->getFloat("movement_liquid_sink");
	pkt << g_settings->getFloat("movement_gravity");

	Send(&pkt);
}
#endif

void Server::HandlePlayerHPChange(PlayerSAO *playersao, const PlayerHPChangeReason &reason)
{
	m_script->player_event(playersao, "health_changed");
	SendPlayerHP(playersao);

	// Send to other clients
	playersao->sendPunchCommand();

	if (playersao->isDead())
		HandlePlayerDeath(playersao, reason);
}

<<<<<<< HEAD

#if MINETEST_PROTO
void Server::SendHP(u16 peer_id, u8 hp)
=======
void Server::SendPlayerHP(PlayerSAO *playersao)
>>>>>>> 5.5.0
{
	SendHP(playersao->getPeerID(), playersao->getHP());
}

void Server::SendHP(session_t peer_id, u16 hp)
{
	NetworkPacket pkt(TOCLIENT_HP, 1, peer_id);
	pkt << hp;
	Send(&pkt);
}

void Server::SendBreath(session_t peer_id, u16 breath)
{
	NetworkPacket pkt(TOCLIENT_BREATH, 2, peer_id);
	pkt << (u16) breath;
	Send(&pkt);
}

void Server::SendAccessDenied(session_t peer_id, AccessDeniedCode reason,
		const std::string &custom_reason, bool reconnect)
{
	if (reason >= SERVER_ACCESSDENIED_MAX)
		return;

	NetworkPacket pkt(TOCLIENT_ACCESS_DENIED, 1, peer_id);
	pkt << (u8)reason;
	if (reason == SERVER_ACCESSDENIED_CUSTOM_STRING)
		pkt << narrow_to_wide(custom_reason);
	else if (reason == SERVER_ACCESSDENIED_SHUTDOWN ||
			reason == SERVER_ACCESSDENIED_CRASH)
		pkt << narrow_to_wide(custom_reason) << (u8)reconnect;
	Send(&pkt);
}

<<<<<<< HEAD
void Server::SendAccessDenied_Legacy(u16 peer_id,const std::string &reason)
=======
void Server::SendAccessDenied_Legacy(session_t peer_id,const std::wstring &reason)
>>>>>>> 5.5.0
{
	NetworkPacket pkt(TOCLIENT_ACCESS_DENIED_LEGACY, 0, peer_id);
	pkt << narrow_to_wide(reason);
	Send(&pkt);
}

void Server::SendDeathscreen(session_t peer_id, bool set_camera_point_target,
		v3f camera_point_target)
{
	NetworkPacket pkt(TOCLIENT_DEATHSCREEN, 1 + sizeof(v3f), peer_id);
	pkt << set_camera_point_target << camera_point_target;
	Send(&pkt);
}

void Server::SendItemDef(session_t peer_id,
		IItemDefManager *itemdef, u16 protocol_version)
{
	NetworkPacket pkt(TOCLIENT_ITEMDEF, 0, peer_id);

	/*
		u16 command
		u32 length of the next item
		zlib-compressed serialized ItemDefManager
	*/
	std::ostringstream tmp_os(std::ios::binary);
	itemdef->serialize(tmp_os, protocol_version);
	std::ostringstream tmp_os2(std::ios::binary);
	compressZlib(tmp_os.str(), tmp_os2);
	pkt.putLongString(tmp_os2.str());

	// Make data buffer
	verbosestream << "Server: Sending item definitions to id(" << peer_id
			<< "): size=" << pkt.getSize() << std::endl;

	Send(&pkt);
}

void Server::SendNodeDef(session_t peer_id,
	const NodeDefManager *nodedef, u16 protocol_version)
{
	NetworkPacket pkt(TOCLIENT_NODEDEF, 0, peer_id);

	/*
		u16 command
		u32 length of the next item
		zlib-compressed serialized NodeDefManager
	*/
	std::ostringstream tmp_os(std::ios::binary);
	nodedef->serialize(tmp_os, protocol_version);
	std::ostringstream tmp_os2(std::ios::binary);
	compressZlib(tmp_os.str(), tmp_os2);

	pkt.putLongString(tmp_os2.str());

	// Make data buffer
	verbosestream << "Server: Sending node definitions to id(" << peer_id
			<< "): size=" << pkt.getSize() << std::endl;

	Send(&pkt);
}

/*
	Non-static send methods
*/

void Server::SendInventory(PlayerSAO *sao, bool incremental)
{
	RemotePlayer *player = sao->getPlayer();

	// Do not send new format to old clients
	incremental &= player->protocol_version >= 38;

	UpdateCrafting(player);

	/*
		Serialize it
	*/

	NetworkPacket pkt(TOCLIENT_INVENTORY, 0, sao->getPeerID());

	std::ostringstream os(std::ios::binary);
	sao->getInventory()->serialize(os, incremental);
	sao->getInventory()->setModified(false);
	player->setModified(true);

	const std::string &s = os.str();
	pkt.putRawString(s.c_str(), s.size());
	Send(&pkt);
}

<<<<<<< HEAD
void Server::SendChatMessage(u16 peer_id, const std::string &message)
=======
void Server::SendChatMessage(session_t peer_id, const ChatMessage &message)
>>>>>>> 5.5.0
{
	NetworkPacket pkt(TOCLIENT_CHAT_MESSAGE, 0, peer_id);
<<<<<<< HEAD
	pkt << narrow_to_wide(message);
=======
	u8 version = 1;
	u8 type = message.type;
	pkt << version << type << message.sender << message.message
		<< static_cast<u64>(message.timestamp);
>>>>>>> 5.5.0

	if (peer_id != PEER_ID_INEXISTENT) {
		RemotePlayer *player = m_env->getPlayer(peer_id);
		if (!player)
			return;

		Send(&pkt);
	} else {
		m_clients.sendToAll(&pkt);
	}
}

void Server::SendShowFormspecMessage(session_t peer_id, const std::string &formspec,
	const std::string &formname)
{
	NetworkPacket pkt(TOCLIENT_SHOW_FORMSPEC, 0, peer_id);
	if (formspec.empty()){
		//the client should close the formspec
		//but make sure there wasn't another one open in meantime
		const auto it = m_formspec_state_data.find(peer_id);
		if (it != m_formspec_state_data.end() && it->second == formname) {
			m_formspec_state_data.erase(peer_id);
		}
		pkt.putLongString("");
	} else {
		m_formspec_state_data[peer_id] = formname;
		pkt.putLongString(formspec);
	}
	pkt << formname;

	Send(&pkt);
}

// Spawns a particle on peer with peer_id
void Server::SendSpawnParticle(session_t peer_id, u16 protocol_version,
	const ParticleParameters &p)
{
	static thread_local const float radius =
			g_settings->getS16("max_block_send_distance") * MAP_BLOCKSIZE * BS;

	if (peer_id == PEER_ID_INEXISTENT) {
		std::vector<session_t> clients = m_clients.getClientIDs();
		const v3f pos = p.pos * BS;
		const float radius_sq = radius * radius;

		for (const session_t client_id : clients) {
			RemotePlayer *player = m_env->getPlayer(client_id);
			if (!player)
				continue;

			PlayerSAO *sao = player->getPlayerSAO();
			if (!sao)
				continue;

			// Do not send to distant clients
			if (sao->getBasePosition().getDistanceFromSQ(pos) > radius_sq)
				continue;

			SendSpawnParticle(client_id, player->protocol_version, p);
		}
		return;
	}
	assert(protocol_version != 0);

	NetworkPacket pkt(TOCLIENT_SPAWN_PARTICLE, 0, peer_id);

	{
		// NetworkPacket and iostreams are incompatible...
		std::ostringstream oss(std::ios_base::binary);
		p.serialize(oss, protocol_version);
		pkt.putRawString(oss.str());
	}

	Send(&pkt);
}

// Adds a ParticleSpawner on peer with peer_id
void Server::SendAddParticleSpawner(session_t peer_id, u16 protocol_version,
	const ParticleSpawnerParameters &p, u16 attached_id, u32 id)
{
	static thread_local const float radius =
			g_settings->getS16("max_block_send_distance") * MAP_BLOCKSIZE * BS;

	if (peer_id == PEER_ID_INEXISTENT) {
		std::vector<session_t> clients = m_clients.getClientIDs();
		const v3f pos = (p.minpos + p.maxpos) / 2.0f * BS;
		const float radius_sq = radius * radius;
		/* Don't send short-lived spawners to distant players.
		 * This could be replaced with proper tracking at some point. */
		const bool distance_check = !attached_id && p.time <= 1.0f;

		for (const session_t client_id : clients) {
			RemotePlayer *player = m_env->getPlayer(client_id);
			if (!player)
				continue;

			if (distance_check) {
				PlayerSAO *sao = player->getPlayerSAO();
				if (!sao)
					continue;
				if (sao->getBasePosition().getDistanceFromSQ(pos) > radius_sq)
					continue;
			}

			SendAddParticleSpawner(client_id, player->protocol_version,
				p, attached_id, id);
		}
		return;
	}
	assert(protocol_version != 0);

	NetworkPacket pkt(TOCLIENT_ADD_PARTICLESPAWNER, 100, peer_id);

	pkt << p.amount << p.time << p.minpos << p.maxpos << p.minvel
		<< p.maxvel << p.minacc << p.maxacc << p.minexptime << p.maxexptime
		<< p.minsize << p.maxsize << p.collisiondetection;

	pkt.putLongString(p.texture);

	pkt << id << p.vertical << p.collision_removal << attached_id;
	{
		std::ostringstream os(std::ios_base::binary);
		p.animation.serialize(os, protocol_version);
		pkt.putRawString(os.str());
	}
	pkt << p.glow << p.object_collision;
	pkt << p.node.param0 << p.node.param2 << p.node_tile;

	Send(&pkt);
}

void Server::SendDeleteParticleSpawner(session_t peer_id, u32 id)
{
	NetworkPacket pkt(TOCLIENT_DELETE_PARTICLESPAWNER, 4, peer_id);

	pkt << id;

	if (peer_id != PEER_ID_INEXISTENT)
		Send(&pkt);
	else
		m_clients.sendToAll(&pkt);

}

void Server::SendHUDAdd(session_t peer_id, u32 id, HudElement *form)
{
	NetworkPacket pkt(TOCLIENT_HUDADD, 0 , peer_id);

	pkt << id << (u8) form->type << form->pos << form->name << form->scale
			<< form->text << form->number << form->item << form->dir
			<< form->align << form->offset << form->world_pos << form->size
			<< form->z_index << form->text2 << form->style;

	Send(&pkt);
}

void Server::SendHUDRemove(session_t peer_id, u32 id)
{
	NetworkPacket pkt(TOCLIENT_HUDRM, 4, peer_id);
	pkt << id;
	Send(&pkt);
}

void Server::SendHUDChange(session_t peer_id, u32 id, HudElementStat stat, void *value)
{
	NetworkPacket pkt(TOCLIENT_HUDCHANGE, 0, peer_id);
	pkt << id << (u8) stat;

	switch (stat) {
		case HUD_STAT_POS:
		case HUD_STAT_SCALE:
		case HUD_STAT_ALIGN:
		case HUD_STAT_OFFSET:
			pkt << *(v2f *) value;
			break;
		case HUD_STAT_NAME:
		case HUD_STAT_TEXT:
		case HUD_STAT_TEXT2:
			pkt << *(std::string *) value;
			break;
		case HUD_STAT_WORLD_POS:
			pkt << *(v3f *) value;
			break;
		case HUD_STAT_SIZE:
			pkt << *(v2s32 *) value;
			break;
		default: // all other types
			pkt << *(u32 *) value;
			break;
	}

	Send(&pkt);
}

void Server::SendHUDSetFlags(session_t peer_id, u32 flags, u32 mask)
{
	NetworkPacket pkt(TOCLIENT_HUD_SET_FLAGS, 4 + 4, peer_id);

	flags &= ~(HUD_FLAG_HEALTHBAR_VISIBLE | HUD_FLAG_BREATHBAR_VISIBLE);

	pkt << flags << mask;

	Send(&pkt);
}

void Server::SendHUDSetParam(session_t peer_id, u16 param, const std::string &value)
{
	NetworkPacket pkt(TOCLIENT_HUD_SET_PARAM, 0, peer_id);
	pkt << param << value;
	Send(&pkt);
}

void Server::SendSetSky(session_t peer_id, const SkyboxParams &params)
{
	NetworkPacket pkt(TOCLIENT_SET_SKY, 0, peer_id);

	// Handle prior clients here
	if (m_clients.getProtocolVersion(peer_id) < 39) {
		pkt << params.bgcolor << params.type << (u16) params.textures.size();

		for (const std::string& texture : params.textures)
			pkt << texture;

		pkt << params.clouds;
	} else { // Handle current clients and future clients
		pkt << params.bgcolor << params.type
		<< params.clouds << params.fog_sun_tint
		<< params.fog_moon_tint << params.fog_tint_type;

		if (params.type == "skybox") {
			pkt << (u16) params.textures.size();
			for (const std::string &texture : params.textures)
				pkt << texture;
		} else if (params.type == "regular") {
			pkt << params.sky_color.day_sky << params.sky_color.day_horizon
				<< params.sky_color.dawn_sky << params.sky_color.dawn_horizon
				<< params.sky_color.night_sky << params.sky_color.night_horizon
				<< params.sky_color.indoors;
		}
	}

	Send(&pkt);
}

void Server::SendSetSun(session_t peer_id, const SunParams &params)
{
	NetworkPacket pkt(TOCLIENT_SET_SUN, 0, peer_id);
	pkt << params.visible << params.texture
		<< params.tonemap << params.sunrise
		<< params.sunrise_visible << params.scale;

	Send(&pkt);
}
void Server::SendSetMoon(session_t peer_id, const MoonParams &params)
{
	NetworkPacket pkt(TOCLIENT_SET_MOON, 0, peer_id);

	pkt << params.visible << params.texture
		<< params.tonemap << params.scale;

	Send(&pkt);
}
void Server::SendSetStars(session_t peer_id, const StarParams &params)
{
	NetworkPacket pkt(TOCLIENT_SET_STARS, 0, peer_id);

	pkt << params.visible << params.count
		<< params.starcolor << params.scale;

	Send(&pkt);
}

void Server::SendCloudParams(session_t peer_id, const CloudParams &params)
{
	NetworkPacket pkt(TOCLIENT_CLOUD_PARAMS, 0, peer_id);
	pkt << params.density << params.color_bright << params.color_ambient
			<< params.height << params.thickness << params.speed;
	Send(&pkt);
}

void Server::SendOverrideDayNightRatio(session_t peer_id, bool do_override,
		float ratio)
{
	NetworkPacket pkt(TOCLIENT_OVERRIDE_DAY_NIGHT_RATIO,
			1 + 2, peer_id);

	pkt << do_override << (u16) (ratio * 65535);

	Send(&pkt);
}

void Server::SendTimeOfDay(session_t peer_id, u16 time, f32 time_speed)
{
	NetworkPacket pkt(TOCLIENT_TIME_OF_DAY, 0, peer_id);
	pkt << time << time_speed;

	if (peer_id == PEER_ID_INEXISTENT) {
		m_clients.sendToAll(&pkt);
	}
	else {
		Send(&pkt);
	}
}

void Server::SendPlayerBreath(PlayerSAO *sao)
{
	assert(sao);

	m_script->player_event(sao, "breath_changed");
	SendBreath(sao->getPeerID(), sao->getBreath());
}

void Server::SendMovePlayer(session_t peer_id)
{
	RemotePlayer *player = m_env->getPlayer(peer_id);
	assert(player);
	PlayerSAO *sao = player->getPlayerSAO();
	assert(sao);

	// Send attachment updates instantly to the client prior updating position
	sao->sendOutdatedData();

	NetworkPacket pkt(TOCLIENT_MOVE_PLAYER, sizeof(v3f) + sizeof(f32) * 2, peer_id);
	pkt << sao->getBasePosition() << sao->getLookPitch() << sao->getRotation().Y;

	{
		v3f pos = sao->getBasePosition();
		verbosestream << "Server: Sending TOCLIENT_MOVE_PLAYER"
				<< " pos=(" << pos.X << "," << pos.Y << "," << pos.Z << ")"
				<< " pitch=" << sao->getLookPitch()
				<< " yaw=" << sao->getRotation().Y
				<< std::endl;
	}

	Send(&pkt);
}

void Server::SendPlayerFov(session_t peer_id)
{
	NetworkPacket pkt(TOCLIENT_FOV, 4 + 1 + 4, peer_id);

	PlayerFovSpec fov_spec = m_env->getPlayer(peer_id)->getFov();
	pkt << fov_spec.fov << fov_spec.is_multiplier << fov_spec.transition_time;

	Send(&pkt);
}

void Server::SendLocalPlayerAnimations(session_t peer_id, v2s32 animation_frames[4],
		f32 animation_speed)
{
	NetworkPacket pkt(TOCLIENT_LOCAL_PLAYER_ANIMATIONS, 0,
		peer_id);

	pkt << animation_frames[0] << animation_frames[1] << animation_frames[2]
			<< animation_frames[3] << animation_speed;

	Send(&pkt);
}

void Server::SendEyeOffset(session_t peer_id, v3f first, v3f third)
{
	NetworkPacket pkt(TOCLIENT_EYE_OFFSET, 0, peer_id);
	pkt << first << third;
	Send(&pkt);
}

void Server::SendPlayerPrivileges(session_t peer_id)
{
	RemotePlayer *player = m_env->getPlayer(peer_id);
	assert(player);
	if(player->getPeerId() == PEER_ID_INEXISTENT)
		return;

	std::set<std::string> privs;
	m_script->getAuth(player->getName(), NULL, &privs);

	NetworkPacket pkt(TOCLIENT_PRIVILEGES, 0, peer_id);
	pkt << (u16) privs.size();

	for (const std::string &priv : privs) {
		pkt << priv;
	}

	Send(&pkt);
}

void Server::SendPlayerInventoryFormspec(session_t peer_id)
{
	RemotePlayer *player = m_env->getPlayer(peer_id);
	assert(player);
	if (player->getPeerId() == PEER_ID_INEXISTENT)
		return;

	NetworkPacket pkt(TOCLIENT_INVENTORY_FORMSPEC, 0, peer_id);
	pkt.putLongString(player->inventory_formspec);

	Send(&pkt);
}

void Server::SendPlayerFormspecPrepend(session_t peer_id)
{
	RemotePlayer *player = m_env->getPlayer(peer_id);
	assert(player);
	if (player->getPeerId() == PEER_ID_INEXISTENT)
		return;

	NetworkPacket pkt(TOCLIENT_FORMSPEC_PREPEND, 0, peer_id);
	pkt << player->formspec_prepend;
	Send(&pkt);
}

void Server::SendActiveObjectRemoveAdd(RemoteClient *client, PlayerSAO *playersao)
{
	// Radius inside which objects are active
	static thread_local const s16 radius =
		g_settings->getS16("active_object_send_range_blocks") * MAP_BLOCKSIZE;

	// Radius inside which players are active
	static thread_local const bool is_transfer_limited =
		g_settings->exists("unlimited_player_transfer_distance") &&
		!g_settings->getBool("unlimited_player_transfer_distance");

	static thread_local const s16 player_transfer_dist =
		g_settings->getS16("player_transfer_distance") * MAP_BLOCKSIZE;

	s16 player_radius = player_transfer_dist == 0 && is_transfer_limited ?
		radius : player_transfer_dist;

	s16 my_radius = MYMIN(radius, playersao->getWantedRange() * MAP_BLOCKSIZE);
	if (my_radius <= 0)
		my_radius = radius;

	std::queue<u16> removed_objects, added_objects;
	m_env->getRemovedActiveObjects(playersao, my_radius, player_radius,
		client->m_known_objects, removed_objects);
	m_env->getAddedActiveObjects(playersao, my_radius, player_radius,
		client->m_known_objects, added_objects);

	int removed_count = removed_objects.size();
	int added_count   = added_objects.size();

	if (removed_objects.empty() && added_objects.empty())
		return;

	char buf[4];
	std::string data;

	// Handle removed objects
	writeU16((u8*)buf, removed_objects.size());
	data.append(buf, 2);
	while (!removed_objects.empty()) {
		// Get object
		u16 id = removed_objects.front();
		ServerActiveObject* obj = m_env->getActiveObject(id);

		// Add to data buffer for sending
		writeU16((u8*)buf, id);
		data.append(buf, 2);

		// Remove from known objects
		client->m_known_objects.erase(id);

		if (obj && obj->m_known_by_count > 0)
			obj->m_known_by_count--;

		removed_objects.pop();
	}

	// Handle added objects
	writeU16((u8*)buf, added_objects.size());
	data.append(buf, 2);
	while (!added_objects.empty()) {
		// Get object
		u16 id = added_objects.front();
		ServerActiveObject *obj = m_env->getActiveObject(id);
		added_objects.pop();

		if (!obj) {
			warningstream << FUNCTION_NAME << ": NULL object id="
				<< (int)id << std::endl;
			continue;
		}

		// Get object type
		u8 type = obj->getSendType();

		// Add to data buffer for sending
		writeU16((u8*)buf, id);
		data.append(buf, 2);
		writeU8((u8*)buf, type);
		data.append(buf, 1);

		data.append(serializeString32(
			obj->getClientInitializationData(client->net_proto_version)));

		// Add to known objects
		client->m_known_objects.insert(id);

		obj->m_known_by_count++;
	}

	NetworkPacket pkt(TOCLIENT_ACTIVE_OBJECT_REMOVE_ADD, data.size(), client->peer_id);
	pkt.putRawString(data.c_str(), data.size());
	Send(&pkt);

	verbosestream << "Server::SendActiveObjectRemoveAdd: "
		<< removed_count << " removed, " << added_count << " added, "
		<< "packet size is " << pkt.getSize() << std::endl;
}

void Server::SendActiveObjectMessages(session_t peer_id, const std::string &datas,
		bool reliable)
{
	NetworkPacket pkt(TOCLIENT_ACTIVE_OBJECT_MESSAGES,
			datas.size(), peer_id);

	pkt.putRawString(datas.c_str(), datas.size());

	m_clients.send(pkt.getPeerId(),
			reliable ? clientCommandFactoryTable[pkt.getCommand()].channel : 1,
			&pkt, reliable);
}

void Server::SendCSMRestrictionFlags(session_t peer_id)
{
	NetworkPacket pkt(TOCLIENT_CSM_RESTRICTION_FLAGS,
		sizeof(m_csm_restriction_flags) + sizeof(m_csm_restriction_noderange), peer_id);
	pkt << m_csm_restriction_flags << m_csm_restriction_noderange;
	Send(&pkt);
}

void Server::SendPlayerSpeed(session_t peer_id, const v3f &added_vel)
{
	NetworkPacket pkt(TOCLIENT_PLAYER_SPEED, 0, peer_id);
	pkt << added_vel;
	Send(&pkt);
}

inline s32 Server::nextSoundId()
{
	s32 ret = m_next_sound_id;
	if (m_next_sound_id == INT32_MAX)
		m_next_sound_id = 0; // signed overflow is undefined
	else
		m_next_sound_id++;
	return ret;
}

s32 Server::playSound(const SimpleSoundSpec &spec,
		const ServerSoundParams &params, bool ephemeral)
{
	// Find out initial position of sound
	bool pos_exists = false;
	v3f pos = params.getPos(m_env, &pos_exists);
	// If position is not found while it should be, cancel sound
	if(pos_exists != (params.type != ServerSoundParams::SSP_LOCAL))
		return -1;

	// Filter destination clients
	std::vector<session_t> dst_clients;
	if (!params.to_player.empty()) {
		RemotePlayer *player = m_env->getPlayer(params.to_player.c_str());
		if(!player){
			infostream<<"Server::playSound: Player \""<<params.to_player
					<<"\" not found"<<std::endl;
			return -1;
		}
		if (player->getPeerId() == PEER_ID_INEXISTENT) {
			infostream<<"Server::playSound: Player \""<<params.to_player
					<<"\" not connected"<<std::endl;
			return -1;
		}
		dst_clients.push_back(player->getPeerId());
	} else {
		std::vector<session_t> clients = m_clients.getClientIDs();

		for (const session_t client_id : clients) {
			RemotePlayer *player = m_env->getPlayer(client_id);
			if (!player)
				continue;
			if (!params.exclude_player.empty() &&
					params.exclude_player == player->getName())
				continue;

			PlayerSAO *sao = player->getPlayerSAO();
			if (!sao)
				continue;

			if (pos_exists) {
				if(sao->getBasePosition().getDistanceFrom(pos) >
						params.max_hear_distance)
					continue;
			}
			dst_clients.push_back(client_id);
		}
	}

	if(dst_clients.empty())
		return -1;

	// Create the sound
	s32 id;
	ServerPlayingSound *psound = nullptr;
	if (ephemeral) {
		id = -1; // old clients will still use this, so pick a reserved ID
	} else {
		id = nextSoundId();
		// The sound will exist as a reference in m_playing_sounds
		m_playing_sounds[id] = ServerPlayingSound();
		psound = &m_playing_sounds[id];
		psound->params = params;
		psound->spec = spec;
	}

	float gain = params.gain * spec.gain;
	NetworkPacket pkt(TOCLIENT_PLAY_SOUND, 0);
	pkt << id << spec.name << gain
			<< (u8) params.type << pos << params.object
			<< params.loop << params.fade << params.pitch
			<< ephemeral;

	bool as_reliable = !ephemeral;

	for (const u16 dst_client : dst_clients) {
		if (psound)
			psound->clients.insert(dst_client);
		m_clients.send(dst_client, 0, &pkt, as_reliable);
	}
	return id;
}
void Server::stopSound(s32 handle)
{
	// Get sound reference
	std::unordered_map<s32, ServerPlayingSound>::iterator i =
		m_playing_sounds.find(handle);
	if (i == m_playing_sounds.end())
		return;
	ServerPlayingSound &psound = i->second;

	NetworkPacket pkt(TOCLIENT_STOP_SOUND, 4);
	pkt << handle;

	for (std::unordered_set<session_t>::const_iterator si = psound.clients.begin();
			si != psound.clients.end(); ++si) {
		// Send as reliable
		m_clients.send(*si, 0, &pkt, true);
	}
	// Remove sound reference
	m_playing_sounds.erase(i);
}

void Server::fadeSound(s32 handle, float step, float gain)
{
	// Get sound reference
	std::unordered_map<s32, ServerPlayingSound>::iterator i =
		m_playing_sounds.find(handle);
	if (i == m_playing_sounds.end())
		return;

	ServerPlayingSound &psound = i->second;
	psound.params.gain = gain;

	NetworkPacket pkt(TOCLIENT_FADE_SOUND, 4);
	pkt << handle << step << gain;

	// Backwards compability
	bool play_sound = gain > 0;
	ServerPlayingSound compat_psound = psound;
	compat_psound.clients.clear();

	NetworkPacket compat_pkt(TOCLIENT_STOP_SOUND, 4);
	compat_pkt << handle;

	for (std::unordered_set<u16>::iterator it = psound.clients.begin();
			it != psound.clients.end();) {
		if (m_clients.getProtocolVersion(*it) >= 32) {
			// Send as reliable
			m_clients.send(*it, 0, &pkt, true);
			++it;
		} else {
			compat_psound.clients.insert(*it);
			// Stop old sound
			m_clients.send(*it, 0, &compat_pkt, true);
			psound.clients.erase(it++);
		}
	}

	// Remove sound reference
	if (!play_sound || psound.clients.empty())
		m_playing_sounds.erase(i);

	if (play_sound && !compat_psound.clients.empty()) {
		// Play new sound volume on older clients
		playSound(compat_psound.spec, compat_psound.params);
	}
}

void Server::sendRemoveNode(v3s16 p, std::unordered_set<u16> *far_players,
		float far_d_nodes)
{
	float maxd = far_d_nodes * BS;
	v3f p_f = intToFloat(p, BS);
	v3s16 block_pos = getNodeBlockPos(p);

	NetworkPacket pkt(TOCLIENT_REMOVENODE, 6);
	pkt << p;

<<<<<<< HEAD
	std::vector<u16> clients = m_clients.getClientIDs();
	for (std::vector<u16>::iterator i = clients.begin(); i != clients.end(); ++i) {
		if (far_players) {
			// Get player
			if (RemotePlayer *player = m_env->getPlayer(*i)) {
				PlayerSAO *sao = player->getPlayerSAO();
				if (!sao)
					continue;

				// If player is far away, only set modified blocks not sent
				v3f player_pos = sao->getBasePosition();
				if (player_pos.getDistanceFrom(p_f) > maxd) {
					far_players->push_back(*i);
					continue;
				}
			}
		}

		// Send as reliable
		m_clients.send(*i, 0, &pkt, true);
	}
}

void Server::sendAddNode(v3s16 p, MapNode n, u16 ignore_id,
		std::vector<u16> *far_players, float far_d_nodes,
		bool remove_metadata)
{
	float maxd = far_d_nodes*BS;
	v3f p_f = intToFloat(p, BS);

	std::vector<u16> clients = m_clients.getClientIDs();
	for(std::vector<u16>::iterator i = clients.begin();	i != clients.end(); ++i) {
		if (far_players) {
			// Get player
			if (RemotePlayer *player = m_env->getPlayer(*i)) {
				PlayerSAO *sao = player->getPlayerSAO();
				if (!sao)
					continue;

				// If player is far away, only set modified blocks not sent
				v3f player_pos = sao->getBasePosition();
				if(player_pos.getDistanceFrom(p_f) > maxd) {
					far_players->push_back(*i);
					continue;
				}
			}
		}

		NetworkPacket pkt(TOCLIENT_ADDNODE, 6 + 2 + 1 + 1 + 1);
		m_clients.lock();
		RemoteClient* client = m_clients.lockedGetClientNoEx(*i);
		if (client != 0) {
			pkt << p << n.param0 << n.param1 << n.param2
					<< (u8) (remove_metadata ? 0 : 1);

			if (!remove_metadata) {
				if (client->net_proto_version <= 21) {
					// Old clients always clear metadata; fix it
					// by sending the full block again.
					client->SetBlockNotSent(getNodeBlockPos(p));
				}
			}
		}
		m_clients.unlock();

		// Send as reliable
		if (pkt.getSize() > 0)
			m_clients.send(*i, 0, &pkt, true);
	}
}

#endif

void Server::SendChatMessage(u16 peer_id, const std::wstring &message) {
	SendChatMessage(peer_id, wide_to_narrow(message));
}

void Server::setBlockNotSent(v3s16 p)
{
	auto clients = m_clients.getClientIDs();
	for(auto
		i = clients.begin();
		i != clients.end(); ++i)
	{
		RemoteClient *client = m_clients.lockedGetClientNoEx(*i);
		client->SetBlocksNotSent();
	}
}

#if MINETEST_PROTO

void Server::SendBlockNoLock(u16 peer_id, MapBlock *block, u8 ver, u16 net_proto_version)
=======
	std::vector<session_t> clients = m_clients.getClientIDs();
	m_clients.lock();

	for (session_t client_id : clients) {
		RemoteClient *client = m_clients.lockedGetClientNoEx(client_id);
		if (!client)
			continue;

		RemotePlayer *player = m_env->getPlayer(client_id);
		PlayerSAO *sao = player ? player->getPlayerSAO() : nullptr;

		// If player is far away, only set modified blocks not sent
		if (!client->isBlockSent(block_pos) || (sao &&
				sao->getBasePosition().getDistanceFrom(p_f) > maxd)) {
			if (far_players)
				far_players->emplace(client_id);
			else
				client->SetBlockNotSent(block_pos);
			continue;
		}

		// Send as reliable
		m_clients.send(client_id, 0, &pkt, true);
	}

	m_clients.unlock();
}

void Server::sendAddNode(v3s16 p, MapNode n, std::unordered_set<u16> *far_players,
		float far_d_nodes, bool remove_metadata)
>>>>>>> 5.5.0
{
	float maxd = far_d_nodes * BS;
	v3f p_f = intToFloat(p, BS);
	v3s16 block_pos = getNodeBlockPos(p);

	NetworkPacket pkt(TOCLIENT_ADDNODE, 6 + 2 + 1 + 1 + 1);
	pkt << p << n.param0 << n.param1 << n.param2
			<< (u8) (remove_metadata ? 0 : 1);

	std::vector<session_t> clients = m_clients.getClientIDs();
	m_clients.lock();

	for (session_t client_id : clients) {
		RemoteClient *client = m_clients.lockedGetClientNoEx(client_id);
		if (!client)
			continue;

		RemotePlayer *player = m_env->getPlayer(client_id);
		PlayerSAO *sao = player ? player->getPlayerSAO() : nullptr;

		// If player is far away, only set modified blocks not sent
		if (!client->isBlockSent(block_pos) || (sao &&
				sao->getBasePosition().getDistanceFrom(p_f) > maxd)) {
			if (far_players)
				far_players->emplace(client_id);
			else
				client->SetBlockNotSent(block_pos);
			continue;
		}

		// Send as reliable
		m_clients.send(client_id, 0, &pkt, true);
	}

	m_clients.unlock();
}

void Server::sendMetadataChanged(const std::list<v3s16> &meta_updates, float far_d_nodes)
{
	float maxd = far_d_nodes * BS;
	NodeMetadataList meta_updates_list(false);
	std::vector<session_t> clients = m_clients.getClientIDs();

	m_clients.lock();

	for (session_t i : clients) {
		RemoteClient *client = m_clients.lockedGetClientNoEx(i);
		if (!client)
			continue;

		ServerActiveObject *player = m_env->getActiveObject(i);
		v3f player_pos = player ? player->getBasePosition() : v3f();

		for (const v3s16 &pos : meta_updates) {
			NodeMetadata *meta = m_env->getMap().getNodeMetadata(pos);

			if (!meta)
				continue;

			v3s16 block_pos = getNodeBlockPos(pos);
			if (!client->isBlockSent(block_pos) || (player &&
					player_pos.getDistanceFrom(intToFloat(pos, BS)) > maxd)) {
				client->SetBlockNotSent(block_pos);
				continue;
			}

			// Add the change to send list
			meta_updates_list.set(pos, meta);
		}
		if (meta_updates_list.size() == 0)
			continue;

		// Send the meta changes
		std::ostringstream os(std::ios::binary);
		meta_updates_list.serialize(os, client->serialization_version, false, true, true);
		std::ostringstream oss(std::ios::binary);
		compressZlib(os.str(), oss);

		NetworkPacket pkt(TOCLIENT_NODEMETA_CHANGED, 0);
		pkt.putLongString(oss.str());
		m_clients.send(i, 0, &pkt, true);

		meta_updates_list.clear();
	}

	m_clients.unlock();
}

void Server::SendBlockNoLock(session_t peer_id, MapBlock *block, u8 ver,
		u16 net_proto_version)
{
	/*
		Create a packet with the block in the right format
	*/
	thread_local const int net_compression_level = rangelim(g_settings->getS16("map_compression_level_net"), -1, 9);
	std::ostringstream os(std::ios_base::binary);
	block->serialize(os, ver, false, net_compression_level);
	block->serializeNetworkSpecific(os);
	std::string s = os.str();

	NetworkPacket pkt(TOCLIENT_BLOCKDATA, 2 + 2 + 2 + s.size(), peer_id);

	pkt << block->getPos();
	pkt.putRawString(s.c_str(), s.size());
	Send(&pkt);
}

#endif

int Server::SendBlocks(float dtime)
{
<<<<<<< HEAD
	//TimeTaker timer("SendBlocks inside");
	DSTACK(FUNCTION_NAME);

	//MutexAutoLock envlock(m_env_mutex);
	//TODO check if one big lock could be faster then multiple small ones

	//ScopeProfiler sp(g_profiler, "Server: sel and send blocks to clients");

	int total = 0;

	std::vector<PrioritySortedBlockTransfer> queue;

	{
		//ScopeProfiler sp(g_profiler, "Server: selecting blocks for sending");
=======
	MutexAutoLock envlock(m_env_mutex);
	//TODO check if one big lock could be faster then multiple small ones

	std::vector<PrioritySortedBlockTransfer> queue;

	u32 total_sending = 0;

	{
		ScopeProfiler sp2(g_profiler, "Server::SendBlocks(): Collect list");
>>>>>>> 5.5.0

		std::vector<session_t> clients = m_clients.getClientIDs();

<<<<<<< HEAD
		for(auto
			i = clients.begin();
			i != clients.end(); ++i)
		{
			auto client = m_clients.getClient(*i, CS_Active);
=======
		m_clients.lock();
		for (const session_t client_id : clients) {
			RemoteClient *client = m_clients.lockedGetClientNoEx(client_id, CS_Active);
>>>>>>> 5.5.0

			if (!client)
				continue;

<<<<<<< HEAD
			total += client->GetNextBlocks(m_env, m_emerge, dtime, m_uptime.get() + m_env->m_game_time_start, queue);
=======
			total_sending += client->getSendingCount();
			client->GetNextBlocks(m_env,m_emerge, dtime, queue);
>>>>>>> 5.5.0
		}
	}

	// Sort.
	// Lowest priority number comes first.
	// Lowest is most important.
	std::sort(queue.begin(), queue.end());

<<<<<<< HEAD
	for(u32 i=0; i<queue.size(); i++)
	{
		//TODO: Calculate limit dynamically

		PrioritySortedBlockTransfer q = queue[i];

		MapBlock *block = NULL;
		try
		{
#if !ENABLE_THREADS
			auto lock = m_env->getServerMap().m_nothread_locker.lock_shared_rec();
#endif
			block = m_env->getMap().getBlockNoCreate(q.pos);
		}
		catch(InvalidPositionException &e)
		{
			continue;
		}

		RemoteClient *client = m_clients.lockedGetClientNoEx(q.peer_id, CS_Active);

		if(!client)
			continue;

		{
		auto lock = block->try_lock_shared_rec();
		if (!lock->owns_lock())
			continue;

		// maybe sometimes blocks will not load (must wait 1+ minute), but reduce network load: q.priority<=4
		SendBlockNoLock(q.peer_id, block, client->serialization_version, client->net_proto_version);
		}

		client->SentBlock(q.pos, m_uptime.get() + m_env->m_game_time_start);
		++total;
=======
	m_clients.lock();

	// Maximal total count calculation
	// The per-client block sends is halved with the maximal online users
	u32 max_blocks_to_send = (m_env->getPlayerCount() + g_settings->getU32("max_users")) *
		g_settings->getU32("max_simultaneous_block_sends_per_client") / 4 + 1;

	ScopeProfiler sp(g_profiler, "Server::SendBlocks(): Send to clients");
	Map &map = m_env->getMap();

	for (const PrioritySortedBlockTransfer &block_to_send : queue) {
		if (total_sending >= max_blocks_to_send)
			break;

		MapBlock *block = map.getBlockNoCreateNoEx(block_to_send.pos);
		if (!block)
			continue;

		RemoteClient *client = m_clients.lockedGetClientNoEx(block_to_send.peer_id,
				CS_Active);
		if (!client)
			continue;

		SendBlockNoLock(block_to_send.peer_id, block, client->serialization_version,
				client->net_proto_version);

		client->SentBlock(block_to_send.pos);
		total_sending++;
>>>>>>> 5.5.0
	}
	return total;
}

bool Server::SendBlock(session_t peer_id, const v3s16 &blockpos)
{
	MapBlock *block = m_env->getMap().getBlockNoCreateNoEx(blockpos);
	if (!block)
		return false;

	m_clients.lock();
	RemoteClient *client = m_clients.lockedGetClientNoEx(peer_id, CS_Active);
	if (!client || client->isBlockSent(blockpos)) {
		m_clients.unlock();
		return false;
	}
	SendBlockNoLock(peer_id, block, client->serialization_version,
			client->net_proto_version);
	m_clients.unlock();

	return true;
}

bool Server::addMediaFile(const std::string &filename,
	const std::string &filepath, std::string *filedata_to,
	std::string *digest_to)
{
	// If name contains illegal characters, ignore the file
	if (!string_allowed(filename, TEXTURENAME_ALLOWED_CHARS)) {
		infostream << "Server: ignoring illegal file name: \""
				<< filename << "\"" << std::endl;
		return false;
	}
	// If name is not in a supported format, ignore it
	const char *supported_ext[] = {
		".png", ".jpg", ".bmp", ".tga",
		".ogg",
		".x", ".b3d", ".obj",
		// Custom translation file format
		".tr",
		NULL
	};
	if (removeStringEnd(filename, supported_ext).empty()) {
		infostream << "Server: ignoring unsupported file extension: \""
				<< filename << "\"" << std::endl;
		return false;
	}
	// Ok, attempt to load the file and add to cache

	// Read data
	std::string filedata;
	if (!fs::ReadFile(filepath, filedata)) {
		errorstream << "Server::addMediaFile(): Failed to open \""
					<< filename << "\" for reading" << std::endl;
		return false;
	}

	if (filedata.empty()) {
		errorstream << "Server::addMediaFile(): Empty file \""
				<< filepath << "\"" << std::endl;
		return false;
	}

	SHA1 sha1;
	sha1.addBytes(filedata.c_str(), filedata.length());

	unsigned char *digest = sha1.getDigest();
	std::string sha1_base64 = base64_encode(digest, 20);
	std::string sha1_hex = hex_encode((char*) digest, 20);
	if (digest_to)
		*digest_to = std::string((char*) digest, 20);
	free(digest);

	// Put in list
	m_media[filename] = MediaInfo(filepath, sha1_base64);
	verbosestream << "Server: " << sha1_hex << " is " << filename
			<< std::endl;

	if (filedata_to)
		*filedata_to = std::move(filedata);
	return true;
}

void Server::fillMediaCache()
{
	infostream << "Server: Calculating media file checksums" << std::endl;

	// Collect all media file paths
<<<<<<< HEAD
	std::list<std::string> paths;
	for(std::vector<ModSpec>::iterator i = m_mods.begin();
			i != m_mods.end(); ++i) {
		const ModSpec &mod = *i;
		paths.push_back(mod.path + DIR_DELIM + "textures");
		paths.push_back(mod.path + DIR_DELIM + "sounds");
		paths.push_back(mod.path + DIR_DELIM + "media");
		paths.push_back(mod.path + DIR_DELIM + "models");
	}
	paths.push_back(porting::path_user + DIR_DELIM + "textures" + DIR_DELIM + "server");
=======
	std::vector<std::string> paths;

	// ordered in descending priority
	paths.push_back(getBuiltinLuaPath() + DIR_DELIM + "locale");
	fs::GetRecursiveDirs(paths, porting::path_user + DIR_DELIM + "textures" + DIR_DELIM + "server");
	fs::GetRecursiveDirs(paths, m_gamespec.path + DIR_DELIM + "textures");
	m_modmgr->getModsMediaPaths(paths);
>>>>>>> 5.5.0

	unsigned int size_total = 0, files_total = 0;
	// Collect media file information from paths into cache
<<<<<<< HEAD
	for(auto i = paths.begin();
			i != paths.end(); ++i)
	{
		std::string mediapath = *i;
=======
	for (const std::string &mediapath : paths) {
>>>>>>> 5.5.0
		std::vector<fs::DirListNode> dirlist = fs::GetDirListing(mediapath);
		for (const fs::DirListNode &dln : dirlist) {
			if (dln.dir) // Ignore dirs (already in paths)
				continue;
<<<<<<< HEAD
			std::string filename = dirlist[j].name;
			// If name contains illegal characters, ignore the file
			if (!string_allowed(filename, TEXTURENAME_ALLOWED_CHARS)) {
				infostream<<"Server: ignoring illegal file name: \""
						<<filename<<"\""<<std::endl;
				continue;
			}
			// If name is not in a supported format, ignore it
			const char *supported_ext[] = {
				".png", ".jpg", ".bmp", ".tga",
				".pcx", ".ppm", ".psd", ".wal", ".rgb",
				".ogg",
				".x", ".b3d", ".md2", ".obj",
				NULL
			};
			if (removeStringEnd(filename, supported_ext) == "") {
				infostream<<"Server: ignoring unsupported file extension: \""
						<<filename<<"\""<<std::endl;
				continue;
			}
			// Ok, attempt to load the file and add to cache
			std::string filepath = mediapath + DIR_DELIM + filename;
			// Read data
			std::ifstream fis(filepath.c_str(), std::ios_base::binary);
			if (!fis.good()) {
				errorstream<<"Server::fillMediaCache(): Could not open \""
						<<filename<<"\" for reading"<<std::endl;
				continue;
			}
			std::ostringstream tmp_os(std::ios_base::binary);
			bool bad = false;
			for (;;) {
				char buf[1024];
				fis.read(buf, 1024);
				std::streamsize len = fis.gcount();
				tmp_os.write(buf, len);
				if (fis.eof())
					break;
				if (!fis.good()) {
					bad = true;
					break;
				}
			}
			if (bad) {
				errorstream<<"Server::fillMediaCache(): Failed to read \""
						<<filename<<"\""<<std::endl;
				continue;
			}
			if (tmp_os.str().length() == 0) {
				errorstream<<"Server::fillMediaCache(): Empty file \""
						<<filepath<<"\""<<std::endl;
				continue;
			}
			size_total += tmp_os.str().length();
			++files_total;
=======
>>>>>>> 5.5.0

			const std::string &filename = dln.name;
			if (m_media.find(filename) != m_media.end()) // Do not override
				continue;

<<<<<<< HEAD
			unsigned char *digest = sha1.getDigest();
			std::string sha1_base64 = base64_encode(digest, 20);
			std::string sha1_hex = hex_encode((char*)digest, 20);
			free(digest);

			// Put in list
			this->m_media[filename] = MediaInfo(filepath, sha1_base64);
			verbosestream<<"Server: "<<sha1_hex<<" is "<<filename<<std::endl;
		}
	}
	actionstream << "Serving " << files_total <<" files, " << size_total << " bytes" << std::endl;
}


#if MINETEST_PROTO

void Server::sendMediaAnnouncement(u16 peer_id)
=======
			std::string filepath = mediapath;
			filepath.append(DIR_DELIM).append(filename);
			addMediaFile(filename, filepath);
		}
	}

	infostream << "Server: " << m_media.size() << " media files collected" << std::endl;
}

void Server::sendMediaAnnouncement(session_t peer_id, const std::string &lang_code)
>>>>>>> 5.5.0
{
	// Make packet
	NetworkPacket pkt(TOCLIENT_ANNOUNCE_MEDIA, 0, peer_id);

	u16 media_sent = 0;
	std::string lang_suffix;
	lang_suffix.append(".").append(lang_code).append(".tr");
	for (const auto &i : m_media) {
		if (i.second.no_announce)
			continue;
		if (str_ends_with(i.first, ".tr") && !str_ends_with(i.first, lang_suffix))
			continue;
		media_sent++;
	}

	pkt << media_sent;

	for (const auto &i : m_media) {
		if (i.second.no_announce)
			continue;
		if (str_ends_with(i.first, ".tr") && !str_ends_with(i.first, lang_suffix))
			continue;
		pkt << i.first << i.second.sha1_digest;
	}

	pkt << g_settings->get("remote_media");
	Send(&pkt);

	verbosestream << "Server: Announcing files to id(" << peer_id
		<< "): count=" << media_sent << " size=" << pkt.getSize() << std::endl;
}

#endif

struct SendableMedia
{
	std::string name;
	std::string path;
	std::string data;

	SendableMedia(const std::string &name, const std::string &path,
			std::string &&data):
		name(name), path(path), data(std::move(data))
	{}
};

<<<<<<< HEAD
#if MINETEST_PROTO

void Server::sendRequestedMedia(u16 peer_id,
=======
void Server::sendRequestedMedia(session_t peer_id,
>>>>>>> 5.5.0
		const std::vector<std::string> &tosend)
{
	verbosestream<<"Server::sendRequestedMedia(): "
			<<"Sending files to client"<<std::endl;

	/* Read files */

	// Put 5kB in one bunch (this is not accurate)
	u32 bytes_per_bunch = 5000;

	std::vector< std::vector<SendableMedia> > file_bunches;
	file_bunches.emplace_back();

	u32 file_size_bunch_total = 0;

<<<<<<< HEAD
	for (std::vector<std::string>::const_iterator i = tosend.begin();
			i != tosend.end(); ++i) {
		const std::string &name = *i;

=======
	for (const std::string &name : tosend) {
>>>>>>> 5.5.0
		if (m_media.find(name) == m_media.end()) {
			errorstream<<"Server::sendRequestedMedia(): Client asked for "
					<<"unknown file \""<<(name)<<"\""<<std::endl;
			continue;
		}

		const auto &m = m_media[name];

		// Read data
<<<<<<< HEAD
		std::ifstream fis(tpath.c_str(), std::ios_base::binary);
		if (!fis.good()) {
			errorstream<<"Server::sendRequestedMedia(): Could not open \""
					<<tpath<<"\" for reading"<<std::endl;
			continue;
		}
		std::ostringstream tmp_os(std::ios_base::binary);
		bool bad = false;
		for (;;) {
			char buf[1024];
			fis.read(buf, 1024);
			std::streamsize len = fis.gcount();
			tmp_os.write(buf, len);
			file_size_bunch_total += len;
			if (fis.eof())
				break;
			if (!fis.good()) {
				bad = true;
				break;
			}
		}
		if (bad) {
			errorstream<<"Server::sendRequestedMedia(): Failed to read \""
					<<name<<"\""<<std::endl;
			continue;
		}
		/*infostream<<"Server::sendRequestedMedia(): Loaded \""
				<<tname<<"\""<<std::endl;*/
=======
		std::string data;
		if (!fs::ReadFile(m.path, data)) {
			errorstream << "Server::sendRequestedMedia(): Failed to read \""
					<< name << "\"" << std::endl;
			continue;
		}
		file_size_bunch_total += data.size();

>>>>>>> 5.5.0
		// Put in list
		file_bunches.back().emplace_back(name, m.path, std::move(data));

		// Start next bunch if got enough data
<<<<<<< HEAD
		if (file_size_bunch_total >= bytes_per_bunch) {
			file_bunches.push_back(std::vector<SendableMedia>());
=======
		if(file_size_bunch_total >= bytes_per_bunch) {
			file_bunches.emplace_back();
>>>>>>> 5.5.0
			file_size_bunch_total = 0;
		}

	}

	/* Create and send packets */

	u16 num_bunches = file_bunches.size();
	for (u16 i = 0; i < num_bunches; i++) {
		/*
			u16 command
			u16 total number of texture bunches
			u16 index of this bunch
			u32 number of files in this bunch
			for each file {
				u16 length of name
				string name
				u32 length of data
				data
			}
		*/

		NetworkPacket pkt(TOCLIENT_MEDIA, 4 + 0, peer_id);
		pkt << num_bunches << i << (u32) file_bunches[i].size();

<<<<<<< HEAD
		for (std::vector<SendableMedia>::iterator
				j = file_bunches[i].begin();
				j != file_bunches[i].end(); ++j) {
			pkt << j->name;
			pkt.putLongString(j->data);
=======
		for (const SendableMedia &j : file_bunches[i]) {
			pkt << j.name;
			pkt.putLongString(j.data);
>>>>>>> 5.5.0
		}

		verbosestream << "Server::sendRequestedMedia(): bunch "
				<< i << "/" << num_bunches
				<< " files=" << file_bunches[i].size()
				<< " size="  << pkt.getSize() << std::endl;
		Send(&pkt);
	}
}

void Server::stepPendingDynMediaCallbacks(float dtime)
{
	MutexAutoLock lock(m_env_mutex);

	for (auto it = m_pending_dyn_media.begin(); it != m_pending_dyn_media.end();) {
		it->second.expiry_timer -= dtime;
		bool del = it->second.waiting_players.empty() || it->second.expiry_timer < 0;

		if (!del) {
			it++;
			continue;
		}

		const auto &name = it->second.filename;
		if (!name.empty()) {
			assert(m_media.count(name));
			// if no_announce isn't set we're definitely deleting the wrong file!
			sanity_check(m_media[name].no_announce);

			fs::DeleteSingleFileOrEmptyDirectory(m_media[name].path);
			m_media.erase(name);
		}
		getScriptIface()->freeDynamicMediaCallback(it->first);
		it = m_pending_dyn_media.erase(it);
	}
}

<<<<<<< HEAD
#endif

void Server::sendDetachedInventories(u16 peer_id)
=======
void Server::SendMinimapModes(session_t peer_id,
		std::vector<MinimapMode> &modes, size_t wanted_mode)
>>>>>>> 5.5.0
{
	RemotePlayer *player = m_env->getPlayer(peer_id);
	assert(player);
	if (player->getPeerId() == PEER_ID_INEXISTENT)
		return;

	NetworkPacket pkt(TOCLIENT_MINIMAP_MODES, 0, peer_id);
	pkt << (u16)modes.size() << (u16)wanted_mode;

	for (auto &mode : modes)
		pkt << (u16)mode.type << mode.label << mode.size << mode.texture << mode.scale;

	Send(&pkt);
}

void Server::sendDetachedInventory(Inventory *inventory, const std::string &name, session_t peer_id)
{
	NetworkPacket pkt(TOCLIENT_DETACHED_INVENTORY, 0, peer_id);
	pkt << name;

	if (!inventory) {
		pkt << false; // Remove inventory
	} else {
		pkt << true; // Update inventory

		// Serialization & NetworkPacket isn't a love story
		std::ostringstream os(std::ios_base::binary);
		inventory->serialize(os);
		inventory->setModified(false);

		const std::string &os_str = os.str();
		pkt << static_cast<u16>(os_str.size()); // HACK: to keep compatibility with 5.0.0 clients
		pkt.putRawString(os_str);
	}

	if (peer_id == PEER_ID_INEXISTENT)
		m_clients.sendToAll(&pkt);
	else
		Send(&pkt);
}

void Server::sendDetachedInventories(session_t peer_id, bool incremental)
{
	// Lookup player name, to filter detached inventories just after
	std::string peer_name;
	if (peer_id != PEER_ID_INEXISTENT) {
		peer_name = getClient(peer_id, CS_Created)->getName();
	}

	auto send_cb = [this, peer_id](const std::string &name, Inventory *inv) {
		sendDetachedInventory(inv, name, peer_id);
	};

	m_inventory_mgr->sendDetachedInventories(peer_name, incremental, send_cb);
}

/*
	Something random
*/

void Server::HandlePlayerDeath(PlayerSAO *playersao, const PlayerHPChangeReason &reason)
{
<<<<<<< HEAD
	DSTACK(FUNCTION_NAME);
	PlayerSAO *playersao = getPlayerSAO(peer_id);

	// In some rare cases this can be NULL -- if the player is disconnected
	// when a Lua function modifies l_punch, for example
	if (!playersao)
		return;

	playersao->m_ms_from_last_respawn = 0;

	auto player = playersao->getPlayer();
	if (!player)
		return;

=======
>>>>>>> 5.5.0
	infostream << "Server::DiePlayer(): Player "
			<< player->getName()
			<< " dies" << std::endl;

	playersao->clearParentAttachment();

	// Trigger scripted stuff
	m_script->on_dieplayer(playersao, reason);

<<<<<<< HEAD
	SendPlayerHP(peer_id);
	SendDeathscreen(peer_id, false, v3f(0,0,0));

	stat.add("die", player->getName());
=======
	SendDeathscreen(playersao->getPeerID(), false, v3f(0,0,0));
>>>>>>> 5.5.0
}

void Server::RespawnPlayer(session_t peer_id)
{
	PlayerSAO *playersao = getPlayerSAO(peer_id);
	if (!playersao)
		return;

	infostream << "Server::RespawnPlayer(): Player "
			<< playersao->getPlayer()->getName()
			<< " respawns" << std::endl;

	playersao->setHP(playersao->accessObjectProperties()->hp_max,
			PlayerHPChangeReason(PlayerHPChangeReason::RESPAWN));
	playersao->setBreath(playersao->accessObjectProperties()->breath_max);

	bool repositioned = m_script->on_respawnplayer(playersao);
	if (!repositioned) {
		// setPos will send the new position to client
<<<<<<< HEAD
		playersao->getPlayer()->setSpeed(v3f(0,0,0));
		playersao->setPos(pos);
	}

	playersao->m_ms_from_last_respawn = 0;

	stat.add("respawn", playersao->getPlayer()->getName());

	SendPlayerHP(peer_id);
	SendPlayerBreath(peer_id);
}


#if MINETEST_PROTO
void Server::DenySudoAccess(u16 peer_id)
=======
		playersao->setPos(findSpawnPos());
	}
}


void Server::DenySudoAccess(session_t peer_id)
>>>>>>> 5.5.0
{
	NetworkPacket pkt(TOCLIENT_DENY_SUDO_MODE, 0, peer_id);
	Send(&pkt);
}
#endif


void Server::DenyAccessVerCompliant(session_t peer_id, u16 proto_ver, AccessDeniedCode reason,
		const std::string &str_reason, bool reconnect)
{
<<<<<<< HEAD
	if (proto_ver >= 25) {
		SendAccessDenied(peer_id, reason, str_reason, reconnect);
	} else {
		std::string wreason = (
			reason == SERVER_ACCESSDENIED_CUSTOM_STRING ? str_reason :
			accessDeniedStrings[(u8)reason]);
#if MINETEST_PROTO
		SendAccessDenied_Legacy(peer_id, wreason);
#endif
	}
=======
	SendAccessDenied(peer_id, reason, str_reason, reconnect);
>>>>>>> 5.5.0

	m_clients.event(peer_id, CSE_SetDenied);
	DisconnectPeer(peer_id);
}


void Server::DenyAccess(session_t peer_id, AccessDeniedCode reason,
		const std::string &custom_reason)
{
	SendAccessDenied(peer_id, reason, custom_reason);
	m_clients.event(peer_id, CSE_SetDenied);
	DisconnectPeer(peer_id);
}

<<<<<<< HEAD
//fmtodo: remove:
void Server::DenyAccess(u16 peer_id, const std::string &custom_reason)
{
    DenyAccess(peer_id, SERVER_ACCESSDENIED_CUSTOM_STRING, custom_reason);
}

//fmtodo: remove:
void Server::DenyAccess_Legacy(u16 peer_id, const std::wstring &custom_reason)
{
    DenyAccess(peer_id, SERVER_ACCESSDENIED_CUSTOM_STRING, wide_to_narrow(custom_reason));
}

#if MINETEST_PROTO
void Server::acceptAuth(u16 peer_id, bool forSudoMode)
=======
// 13/03/15: remove this function when protocol version 25 will become
// the minimum version for MT users, maybe in 1 year
void Server::DenyAccess_Legacy(session_t peer_id, const std::wstring &reason)
{
	SendAccessDenied_Legacy(peer_id, reason);
	m_clients.event(peer_id, CSE_SetDenied);
	DisconnectPeer(peer_id);
}

void Server::DisconnectPeer(session_t peer_id)
>>>>>>> 5.5.0
{
	m_modchannel_mgr->leaveAllChannels(peer_id);
	m_con->DisconnectPeer(peer_id);
}

void Server::acceptAuth(session_t peer_id, bool forSudoMode)
{
	if (!forSudoMode) {
		RemoteClient* client = getClient(peer_id, CS_Invalid);

		NetworkPacket resp_pkt(TOCLIENT_AUTH_ACCEPT, 1 + 6 + 8 + 4, peer_id);

		// Right now, the auth mechs don't change between login and sudo mode.
		u32 sudo_auth_mechs = client->allowed_auth_mechs;
		client->allowed_sudo_mechs = sudo_auth_mechs;

		resp_pkt << v3f(0,0,0) << (u64) m_env->getServerMap().getSeed()
				<< g_settings->getFloat("dedicated_server_step")
				<< sudo_auth_mechs;

		Send(&resp_pkt);
		m_clients.event(peer_id, CSE_AuthAccept);
	} else {
		NetworkPacket resp_pkt(TOCLIENT_ACCEPT_SUDO_MODE, 1 + 6 + 8 + 4, peer_id);

		// We only support SRP right now
		u32 sudo_auth_mechs = AUTH_MECHANISM_FIRST_SRP;

		resp_pkt << sudo_auth_mechs;
		Send(&resp_pkt);
		m_clients.event(peer_id, CSE_SudoSuccess);
	}
}
#endif

void Server::DeleteClient(session_t peer_id, ClientDeletionReason reason)
{
<<<<<<< HEAD
	DSTACK(FUNCTION_NAME);
	std::string message;
=======
	std::wstring message;
>>>>>>> 5.5.0
	{
		/*
			Clear references to playing sounds
		*/
		for (std::unordered_map<s32, ServerPlayingSound>::iterator
				 i = m_playing_sounds.begin(); i != m_playing_sounds.end();) {
			ServerPlayingSound &psound = i->second;
			psound.clients.erase(peer_id);
			if (psound.clients.empty())
				m_playing_sounds.erase(i++);
			else
				++i;
		}

		// clear formspec info so the next client can't abuse the current state
		m_formspec_state_data.erase(peer_id);

		RemotePlayer *player = m_env->getPlayer(peer_id);

		/* Run scripts and remove from environment */
		if (player) {
			PlayerSAO *playersao = player->getPlayerSAO();
			assert(playersao);

			playersao->clearChildAttachments();
			playersao->clearParentAttachment();

			// inform connected clients
			const std::string &player_name = player->getName();
			NetworkPacket notice(TOCLIENT_UPDATE_PLAYER_LIST, 0, PEER_ID_INEXISTENT);
			// (u16) 1 + std::string represents a vector serialization representation
			notice << (u8) PLAYER_LIST_REMOVE  << (u16) 1 << player_name;
			m_clients.sendToAll(&notice);
			// run scripts
			m_script->on_leaveplayer(playersao, reason == CDR_TIMEOUT);

			playersao->disconnected();
		}

		/*
			Print out action
		*/
		{
			if (player && reason != CDR_DENY) {
				std::ostringstream os(std::ios_base::binary);
				std::vector<session_t> clients = m_clients.getClientIDs();

<<<<<<< HEAD
				for(auto
					i = clients.begin();
					i != clients.end(); ++i)
				{
=======
				for (const session_t client_id : clients) {
>>>>>>> 5.5.0
					// Get player
					RemotePlayer *player = m_env->getPlayer(client_id);
					if (!player)
						continue;
					// Get name of player
					os << player->getName() << " ";
				}

				std::string name = player->getName();
				actionstream << name << " "
						<< (reason == CDR_TIMEOUT ? "times out." : "leaves game.")
						<< " List of players: " << os.str() << std::endl;
				if (m_admin_chat)
					m_admin_chat->outgoing_queue.push_back(
						new ChatEventNick(CET_NICK_REMOVE, name));
			}
		}
		{
			//MutexAutoLock env_lock(m_env_mutex);
			m_clients.DeleteClient(peer_id);
		}
	}

	// Send leave chat message to all remaining clients
	if (!message.empty()) {
		SendChatMessage(PEER_ID_INEXISTENT,
				ChatMessage(CHATMESSAGE_TYPE_ANNOUNCE, message));
	}
}

void Server::UpdateCrafting(RemotePlayer *player)
{
	InventoryList *clist = player->inventory.getList("craft");
	if (!clist || clist->getSize() == 0)
		return;

	if (!clist->checkModified())
		return;

	// Get a preview for crafting
	ItemStack preview;
	InventoryLocation loc;
	loc.setPlayer(player->getName());
	std::vector<ItemStack> output_replacements;
	getCraftingResult(&player->inventory, preview, output_replacements, false, this);
	m_env->getScriptIface()->item_CraftPredict(preview, player->getPlayerSAO(),
			clist, loc);

	InventoryList *plist = player->inventory.getList("craftpreview");
<<<<<<< HEAD
	assert(plist);
	assert(plist->getSize() >= 1);
	plist->changeItem(0, preview);
=======
	if (plist && plist->getSize() >= 1) {
		// Put the new preview in
		plist->changeItem(0, preview);
	}
>>>>>>> 5.5.0
}

void Server::handleChatInterfaceEvent(ChatEvent *evt)
{
	if (evt->type == CET_NICK_ADD) {
		// The terminal informed us of its nick choice
		m_admin_nick = ((ChatEventNick *)evt)->nick;
		if (!m_script->getAuth(m_admin_nick, NULL, NULL)) {
			errorstream << "You haven't set up an account." << std::endl
				<< "Please log in using the client as '"
				<< m_admin_nick << "' with a secure password." << std::endl
				<< "Until then, you can't execute admin tasks via the console," << std::endl
				<< "and everybody can claim the user account instead of you," << std::endl
				<< "giving them full control over this server." << std::endl;
		}
	} else {
		assert(evt->type == CET_CHAT);
		handleAdminChat((ChatEventChat *)evt);
	}
}

std::wstring Server::handleChat(const std::string &name,
	std::wstring wmessage, bool check_shout_priv, RemotePlayer *player)
{
	// If something goes wrong, this player is to blame
	RollbackScopeActor rollback_scope(m_rollback,
			std::string("player:") + name);

	if (g_settings->getBool("strip_color_codes"))
		wmessage = unescape_enriched(wmessage);

	if (player) {
		switch (player->canSendChatMessage()) {
		case RPLAYER_CHATRESULT_FLOODING: {
			std::wstringstream ws;
			ws << L"You cannot send more messages. You are limited to "
					<< g_settings->getFloat("chat_message_limit_per_10sec")
					<< L" messages per 10 seconds.";
			return ws.str();
		}
		case RPLAYER_CHATRESULT_KICK:
			DenyAccess_Legacy(player->getPeerId(),
					L"You have been kicked due to message flooding.");
			return L"";
		case RPLAYER_CHATRESULT_OK:
			break;
		default:
			FATAL_ERROR("Unhandled chat filtering result found.");
		}
	}

	if (m_max_chatmessage_length > 0
			&& wmessage.length() > m_max_chatmessage_length) {
		return L"Your message exceed the maximum chat message limit set on the server. "
				L"It was refused. Send a shorter message";
	}

	auto message = trim(wide_to_utf8(wmessage));
	if (message.empty())
		return L"";

	if (message.find_first_of("\n\r") != std::wstring::npos) {
		return L"Newlines are not permitted in chat messages";
	}

	// Run script hook, exit if script ate the chat message
	if (m_script->on_chat_message(name, message))
		return L"";

	// Line to send
	std::wstring line;
	// Whether to send line to the player that sent the message, or to all players
	bool broadcast_line = true;

	if (check_shout_priv && !checkPriv(name, "shout")) {
		line += L"-!- You don't have permission to shout.";
		broadcast_line = false;
	} else {
		/*
			Workaround for fixing chat on Android. Lua doesn't handle
			the Cyrillic alphabet and some characters on older Android devices
		*/
#ifdef __ANDROID__
		line += L"<" + utf8_to_wide(name) + L"> " + wmessage;
#else
		line += utf8_to_wide(m_script->formatChatMessage(name,
				wide_to_utf8(wmessage)));
#endif
	}

	/*
		Tell calling method to send the message to sender
	*/
	if (!broadcast_line)
		return line;

	/*
		Send the message to others
	*/
	actionstream << "CHAT: " << wide_to_utf8(unescape_enriched(line)) << std::endl;

	ChatMessage chatmsg(line);

	std::vector<session_t> clients = m_clients.getClientIDs();
	for (u16 cid : clients)
		SendChatMessage(cid, chatmsg);

<<<<<<< HEAD
		u16 peer_id_to_avoid_sending = (player ? (u16)player->peer_id : PEER_ID_INEXISTENT);
		for (u16 i = 0; i < clients.size(); i++) {
			u16 cid = clients[i];
			if (cid != peer_id_to_avoid_sending)
				SendChatMessage(cid, line);
		}
	}
=======
>>>>>>> 5.5.0
	return L"";
}

void Server::handleAdminChat(const ChatEventChat *evt)
{
	std::string name = evt->nick;
	std::wstring wmessage = evt->evt_msg;

	std::wstring answer = handleChat(name, wmessage);

	// If asked to send answer to sender
	if (!answer.empty()) {
		m_admin_chat->outgoing_queue.push_back(new ChatEventChat("", answer));
	}
}

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

std::string Server::getPlayerName(session_t peer_id)
{
	RemotePlayer *player = m_env->getPlayer(peer_id);
	if (!player)
		return "[id="+itos(peer_id)+"]";
	return player->getName();
}

PlayerSAO *Server::getPlayerSAO(session_t peer_id)
{
	RemotePlayer *player = m_env->getPlayer(peer_id);
	if (!player)
		return NULL;
	return player->getPlayerSAO();
}

std::string Server::getStatusString()
{
	std::ostringstream os(std::ios_base::binary);
<<<<<<< HEAD
	os<<"# Server: ";
	// Version
	os<<"version="<<(g_version_string);
	// Uptime
	os<<", uptime="<<m_uptime.get();
	// Max lag estimate
	os<<", max_lag="<<m_env->getMaxLagEstimate();
	// Information about clients
	bool first = true;
	os<<", clients={";
	std::vector<u16> clients = m_clients.getClientIDs();
	for(auto i = clients.begin(); i != clients.end(); ++i) {
		// Get player
		RemotePlayer *player = m_env->getPlayer(*i);
		// Get name of player
		std::string name = "unknown";
		if(player != NULL)
			name = player->getName();
		// Add name to information string
		if(!first)
			os<<", ";
		else
			first = false;
		os<<name;
	}
	os<<"}";
	if(((ServerMap*)(&m_env->getMap()))->isSavingEnabled() == false)
		os<<std::endl<<"# Server: "<<" WARNING: Map saving is disabled.";
	if(g_settings->get("motd") != "")
		os<<std::endl<<"# Server: "<<g_settings->get("motd");
=======
	os << "# Server: ";
	// Version
	os << "version: " << g_version_string;
	// Game
	os << " | game: " << (m_gamespec.name.empty() ? m_gamespec.id : m_gamespec.name);
	// Uptime
	os << " | uptime: " << duration_to_string((int) m_uptime_counter->get());
	// Max lag estimate
	os << " | max lag: " << std::setprecision(3);
	os << (m_env ? m_env->getMaxLagEstimate() : 0) << "s";

	// Information about clients
	bool first = true;
	os << " | clients: ";
	if (m_env) {
		std::vector<session_t> clients = m_clients.getClientIDs();
		for (session_t client_id : clients) {
			RemotePlayer *player = m_env->getPlayer(client_id);

			// Get name of player
			const char *name = player ? player->getName() : "<unknown>";

			// Add name to information string
			if (!first)
				os << ", ";
			else
				first = false;
			os << name;
		}
	}

	if (m_env && !((ServerMap*)(&m_env->getMap()))->isSavingEnabled())
		os << std::endl << "# Server: " << " WARNING: Map saving is disabled.";

	if (!g_settings->get("motd").empty())
		os << std::endl << "# Server: " << g_settings->get("motd");

>>>>>>> 5.5.0
	return os.str();
}

std::set<std::string> Server::getPlayerEffectivePrivs(const std::string &name)
{
	std::set<std::string> privs;
	m_script->getAuth(name, NULL, &privs);
	return privs;
}

bool Server::checkPriv(const std::string &name, const std::string &priv)
{
	std::set<std::string> privs = getPlayerEffectivePrivs(name);
	return (privs.count(priv) != 0);
}

void Server::reportPrivsModified(const std::string &name)
{
<<<<<<< HEAD
	if(name == ""){
		std::vector<u16> clients = m_clients.getClientIDs();
		for(auto
				i = clients.begin();
				i != clients.end(); ++i){
			RemotePlayer *player = m_env->getPlayer(*i);
=======
	if (name.empty()) {
		std::vector<session_t> clients = m_clients.getClientIDs();
		for (const session_t client_id : clients) {
			RemotePlayer *player = m_env->getPlayer(client_id);
>>>>>>> 5.5.0
			reportPrivsModified(player->getName());
		}
	} else {
		RemotePlayer *player = m_env->getPlayer(name.c_str());
		if (!player)
			return;
		SendPlayerPrivileges(player->getPeerId());
		PlayerSAO *sao = player->getPlayerSAO();
		if(!sao)
			return;
		sao->updatePrivileges(
				getPlayerEffectivePrivs(name),
				isSingleplayer());
	}
}

void Server::reportInventoryFormspecModified(const std::string &name)
{
	RemotePlayer *player = m_env->getPlayer(name.c_str());
	if (!player)
		return;
	SendPlayerInventoryFormspec(player->getPeerId());
}

void Server::reportFormspecPrependModified(const std::string &name)
{
	RemotePlayer *player = m_env->getPlayer(name.c_str());
	if (!player)
		return;
	SendPlayerFormspecPrepend(player->getPeerId());
}

void Server::setIpBanned(const std::string &ip, const std::string &name)
{
	m_banmanager->add(ip, name);
}

void Server::unsetIpBanned(const std::string &ip_or_name)
{
	m_banmanager->remove(ip_or_name);
}

std::string Server::getBanDescription(const std::string &ip_or_name)
{
	return m_banmanager->getBanDescription(ip_or_name);
}

void Server::notifyPlayer(const char *name, const std::string &msg)
{
	// m_env will be NULL if the server is initializing
	if (!m_env)
		return;

	if (m_admin_nick == name && !m_admin_nick.empty()) {
		m_admin_chat->outgoing_queue.push_back(new ChatEventChat("", utf8_to_wide(msg)));
	}

	RemotePlayer *player = m_env->getPlayer(name);
	if (!player) {
		return;
	}

	if (player->getPeerId() == PEER_ID_INEXISTENT)
		return;

<<<<<<< HEAD
	//fmold: SendChatMessage(player->peer_id, std::string("\v#ffffff") + msg);
	SendChatMessage(player->peer_id, msg);
=======
	SendChatMessage(player->getPeerId(), ChatMessage(msg));
>>>>>>> 5.5.0
}

bool Server::showFormspec(const char *playername, const std::string &formspec,
	const std::string &formname)
{
	// m_env will be NULL if the server is initializing
	if (!m_env)
		return false;

	RemotePlayer *player = m_env->getPlayer(playername);
	if (!player)
		return false;

	SendShowFormspecMessage(player->getPeerId(), formspec, formname);
	return true;
}

u32 Server::hudAdd(RemotePlayer *player, HudElement *form)
{
	if (!player)
		return -1;

	u32 id = player->addHud(form);

	SendHUDAdd(player->getPeerId(), id, form);

	return id;
}

bool Server::hudRemove(RemotePlayer *player, u32 id) {
	if (!player)
		return false;

	HudElement* todel = player->removeHud(id);

	if (!todel)
		return false;

	delete todel;

	SendHUDRemove(player->getPeerId(), id);
	return true;
}

bool Server::hudChange(RemotePlayer *player, u32 id, HudElementStat stat, void *data)
{
	if (!player)
		return false;

	SendHUDChange(player->getPeerId(), id, stat, data);
	return true;
}

bool Server::hudSetFlags(RemotePlayer *player, u32 flags, u32 mask)
{
	if (!player)
		return false;

	SendHUDSetFlags(player->getPeerId(), flags, mask);
	player->hud_flags &= ~mask;
	player->hud_flags |= flags;

	PlayerSAO* playersao = player->getPlayerSAO();

	if (!playersao)
		return false;

	m_script->player_event(playersao, "hud_changed");
	return true;
}

bool Server::hudSetHotbarItemcount(RemotePlayer *player, s32 hotbar_itemcount)
{
	if (!player)
		return false;

	if (hotbar_itemcount <= 0 || hotbar_itemcount > HUD_HOTBAR_ITEMCOUNT_MAX)
		return false;

	player->setHotbarItemcount(hotbar_itemcount);
	std::ostringstream os(std::ios::binary);
	writeS32(os, hotbar_itemcount);
	SendHUDSetParam(player->getPeerId(), HUD_PARAM_HOTBAR_ITEMCOUNT, os.str());
	return true;
}

<<<<<<< HEAD
void Server::hudSetHotbarImage(RemotePlayer *player, std::string name, int items)
=======
void Server::hudSetHotbarImage(RemotePlayer *player, const std::string &name)
>>>>>>> 5.5.0
{
	if (!player)
		return;

	player->setHotbarImage(name);
<<<<<<< HEAD
	SendHUDSetParam(player->peer_id, HUD_PARAM_HOTBAR_IMAGE, name);
	SendHUDSetParam(player->peer_id, HUD_PARAM_HOTBAR_IMAGE_ITEMS, std::string() + std::to_string(items));
=======
	SendHUDSetParam(player->getPeerId(), HUD_PARAM_HOTBAR_IMAGE, name);
>>>>>>> 5.5.0
}

void Server::hudSetHotbarSelectedImage(RemotePlayer *player, const std::string &name)
{
	if (!player)
		return;

	player->setHotbarSelectedImage(name);
	SendHUDSetParam(player->getPeerId(), HUD_PARAM_HOTBAR_SELECTED_IMAGE, name);
}

Address Server::getPeerAddress(session_t peer_id)
{
	// Note that this is only set after Init was received in Server::handleCommand_Init
	return getClient(peer_id, CS_Invalid)->getAddress();
}

void Server::setLocalPlayerAnimations(RemotePlayer *player,
		v2s32 animation_frames[4], f32 frame_speed)
{
	sanity_check(player);
	player->setLocalAnimations(animation_frames, frame_speed);
	SendLocalPlayerAnimations(player->getPeerId(), animation_frames, frame_speed);
}

void Server::setPlayerEyeOffset(RemotePlayer *player, const v3f &first, const v3f &third)
{
	sanity_check(player);
	player->eye_offset_first = first;
	player->eye_offset_third = third;
	SendEyeOffset(player->getPeerId(), first, third);
}

void Server::setSky(RemotePlayer *player, const SkyboxParams &params)
{
	sanity_check(player);
	player->setSky(params);
	SendSetSky(player->getPeerId(), params);
}

void Server::setSun(RemotePlayer *player, const SunParams &params)
{
	sanity_check(player);
	player->setSun(params);
	SendSetSun(player->getPeerId(), params);
}

void Server::setMoon(RemotePlayer *player, const MoonParams &params)
{
	sanity_check(player);
	player->setMoon(params);
	SendSetMoon(player->getPeerId(), params);
}

void Server::setStars(RemotePlayer *player, const StarParams &params)
{
	sanity_check(player);
	player->setStars(params);
	SendSetStars(player->getPeerId(), params);
}

void Server::setClouds(RemotePlayer *player, const CloudParams &params)
{
	sanity_check(player);
	player->setCloudParams(params);
	SendCloudParams(player->getPeerId(), params);
}

void Server::overrideDayNightRatio(RemotePlayer *player, bool do_override,
	float ratio)
{
	sanity_check(player);
	player->overrideDayNightRatio(do_override, ratio);
	SendOverrideDayNightRatio(player->getPeerId(), do_override, ratio);
}

void Server::notifyPlayers(const std::string &msg)
{
	SendChatMessage(PEER_ID_INEXISTENT, ChatMessage(msg));
}

void Server::spawnParticle(const std::string &playername,
	const ParticleParameters &p)
{
	// m_env will be NULL if the server is initializing
	if (!m_env)
		return;

	session_t peer_id = PEER_ID_INEXISTENT;
	u16 proto_ver = 0;
	if (!playername.empty()) {
		RemotePlayer *player = m_env->getPlayer(playername.c_str());
		if (!player)
			return;
		peer_id = player->getPeerId();
		proto_ver = player->protocol_version;
	}

	SendSpawnParticle(peer_id, proto_ver, p);
}

u32 Server::addParticleSpawner(const ParticleSpawnerParameters &p,
	ServerActiveObject *attached, const std::string &playername)
{
	// m_env will be NULL if the server is initializing
	if (!m_env)
		return -1;

	session_t peer_id = PEER_ID_INEXISTENT;
	u16 proto_ver = 0;
	if (!playername.empty()) {
		RemotePlayer *player = m_env->getPlayer(playername.c_str());
		if (!player)
			return -1;
		peer_id = player->getPeerId();
		proto_ver = player->protocol_version;
	}

	u16 attached_id = attached ? attached->getId() : 0;

	u32 id;
	if (attached_id == 0)
		id = m_env->addParticleSpawner(p.time);
	else
		id = m_env->addParticleSpawner(p.time, attached_id);

	SendAddParticleSpawner(peer_id, proto_ver, p, attached_id, id);
	return id;
}

void Server::deleteParticleSpawner(const std::string &playername, u32 id)
{
	// m_env will be NULL if the server is initializing
	if (!m_env)
		throw ServerError("Can't delete particle spawners during initialisation!");

	session_t peer_id = PEER_ID_INEXISTENT;
	if (!playername.empty()) {
		RemotePlayer *player = m_env->getPlayer(playername.c_str());
		if (!player)
			return;
		peer_id = player->getPeerId();
	}

	m_env->deleteParticleSpawner(id);
	SendDeleteParticleSpawner(peer_id, id);
}

bool Server::dynamicAddMedia(std::string filepath,
	const u32 token, const std::string &to_player, bool ephemeral)
{
	std::string filename = fs::GetFilenameFromPath(filepath.c_str());
	auto it = m_media.find(filename);
	if (it != m_media.end()) {
		// Allow the same path to be "added" again in certain conditions
		if (ephemeral || it->second.path != filepath) {
			errorstream << "Server::dynamicAddMedia(): file \"" << filename
				<< "\" already exists in media cache" << std::endl;
			return false;
		}
	}
<<<<<<< HEAD
	Inventory *inv = new Inventory(m_itemdef);
	assert(inv);
	m_detached_inventories[name] = inv;
	m_detached_inventories_player[name] = player;
	//TODO find a better way to do this
	sendDetachedInventory(name,PEER_ID_INEXISTENT);
	return inv;
=======

	// Load the file and add it to our media cache
	std::string filedata, raw_hash;
	bool ok = addMediaFile(filename, filepath, &filedata, &raw_hash);
	if (!ok)
		return false;

	if (ephemeral) {
		// Create a copy of the file and swap out the path, this removes the
		// requirement that mods keep the file accessible at the original path.
		filepath = fs::CreateTempFile();
		bool ok = ([&] () -> bool {
			if (filepath.empty())
				return false;
			std::ofstream os(filepath.c_str(), std::ios::binary);
			if (!os.good())
				return false;
			os << filedata;
			os.close();
			return !os.fail();
		})();
		if (!ok) {
			errorstream << "Server: failed to create a copy of media file "
				<< "\"" << filename << "\"" << std::endl;
			m_media.erase(filename);
			return false;
		}
		verbosestream << "Server: \"" << filename << "\" temporarily copied to "
			<< filepath << std::endl;

		m_media[filename].path = filepath;
		m_media[filename].no_announce = true;
		// stepPendingDynMediaCallbacks will clean this up later.
	} else if (!to_player.empty()) {
		m_media[filename].no_announce = true;
	}

	// Push file to existing clients
	NetworkPacket pkt(TOCLIENT_MEDIA_PUSH, 0);
	pkt << raw_hash << filename << (bool)ephemeral;

	NetworkPacket legacy_pkt = pkt;

	// Newer clients get asked to fetch the file (asynchronous)
	pkt << token;
	// Older clients have an awful hack that just throws the data at them
	legacy_pkt.putLongString(filedata);

	std::unordered_set<session_t> delivered, waiting;
	m_clients.lock();
	for (auto &pair : m_clients.getClientList()) {
		if (pair.second->getState() == CS_DefinitionsSent && !ephemeral) {
			/*
				If a client is in the DefinitionsSent state it is too late to
				transfer the file via sendMediaAnnouncement() but at the same
				time the client cannot accept a media push yet.
				Short of artificially delaying the joining process there is no
				way for the server to resolve this so we (currently) opt not to.
			*/
			warningstream << "The media \"" << filename << "\" (dynamic) could "
				"not be delivered to " << pair.second->getName()
				<< " due to a race condition." << std::endl;
			continue;
		}
		if (pair.second->getState() < CS_Active)
			continue;

		const auto proto_ver = pair.second->net_proto_version;
		if (proto_ver < 39)
			continue;

		const session_t peer_id = pair.second->peer_id;
		if (!to_player.empty() && getPlayerName(peer_id) != to_player)
			continue;

		if (proto_ver < 40) {
			delivered.emplace(peer_id);
			/*
				The network layer only guarantees ordered delivery inside a channel.
				Since the very next packet could be one that uses the media, we have
				to push the media over ALL channels to ensure it is processed before
				it is used. In practice this means channels 1 and 0.
			*/
			m_clients.send(peer_id, 1, &legacy_pkt, true);
			m_clients.send(peer_id, 0, &legacy_pkt, true);
		} else {
			waiting.emplace(peer_id);
			Send(peer_id, &pkt);
		}
	}
	m_clients.unlock();

	// Run callback for players that already had the file delivered (legacy-only)
	for (session_t peer_id : delivered) {
		if (auto player = m_env->getPlayer(peer_id))
			getScriptIface()->on_dynamic_media_added(token, player->getName());
	}

	// Save all others in our pending state
	auto &state = m_pending_dyn_media[token];
	state.waiting_players = std::move(waiting);
	// regardless of success throw away the callback after a while
	state.expiry_timer = 60.0f;
	if (ephemeral)
		state.filename = filename;

	return true;
>>>>>>> 5.5.0
}

// actions: time-reversed list
// Return value: success/failure
bool Server::rollbackRevertActions(const std::list<RollbackAction> &actions,
		std::list<std::string> *log)
{
	infostream<<"Server::rollbackRevertActions(len="<<actions.size()<<")"<<std::endl;
	ServerMap *map = (ServerMap*)(&m_env->getMap());

	// Fail if no actions to handle
	if (actions.empty()) {
		assert(log);
		log->push_back("Nothing to do.");
		return false;
	}

	int num_tried = 0;
	int num_failed = 0;

	for (const RollbackAction &action : actions) {
		num_tried++;
		bool success = action.applyRevert(map, m_inventory_mgr.get(), this);
		if(!success){
			num_failed++;
			std::ostringstream os;
			os<<"Revert of step ("<<num_tried<<") "<<action.toString()<<" failed";
			infostream<<"Map::rollbackRevertActions(): "<<os.str()<<std::endl;
			if (log)
				log->push_back(os.str());
		}else{
			std::ostringstream os;
			os<<"Successfully reverted step ("<<num_tried<<") "<<action.toString();
			infostream<<"Map::rollbackRevertActions(): "<<os.str()<<std::endl;
			if (log)
				log->push_back(os.str());
		}
	}

	infostream<<"Map::rollbackRevertActions(): "<<num_failed<<"/"<<num_tried
			<<" failed"<<std::endl;

	// Call it done if less than half failed
	return num_failed <= num_tried/2;
}

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

ICraftDefManager *Server::getCraftDefManager()
{
	return m_craftdef;
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

IWritableCraftDefManager *Server::getWritableCraftDefManager()
{
	return m_craftdef;
}

const std::vector<ModSpec> & Server::getMods() const
{
	return m_modmgr->getMods();
}

const ModSpec *Server::getModSpec(const std::string &modname) const
{
	return m_modmgr->getModSpec(modname);
}

void Server::getModNames(std::vector<std::string> &modlist)
{
	m_modmgr->getModNames(modlist);
}

std::string Server::getBuiltinLuaPath()
{
	return porting::path_share + DIR_DELIM + "builtin";
}

v3f Server::findSpawnPos()
{
	ServerMap &map = m_env->getServerMap();
	v3f nodeposf;
<<<<<<< HEAD
	POS find = 0;
	g_settings->getS16NoEx("static_spawnpoint_find", find);
	if (g_settings->getV3FNoEx("static_spawnpoint", nodeposf) && !find) {
=======
	if (g_settings->getV3FNoEx("static_spawnpoint", nodeposf))
>>>>>>> 5.5.0
		return nodeposf * BS;

//todo: remove
	s16 water_level = map.getWaterLevel();
	s16 vertical_spawn_range = g_settings->getS16("vertical_spawn_range");
//============
	auto cache_block_before_spawn = g_settings->getBool("cache_block_before_spawn");

	bool is_good = false;
<<<<<<< HEAD
	POS min_air_height = 3;
	g_settings->getS16NoEx("static_spawnpoint_find_height", min_air_height);
=======
	// Limit spawn range to mapgen edges (determined by 'mapgen_limit')
	s32 range_max = map.getMapgenParams()->getSpawnRangeMax();
>>>>>>> 5.5.0

	// Try to find a good place a few times
	for (s32 i = 0; i < 4000 && !is_good; i++) {
		s32 range = MYMIN(1 + i, range_max);
		// We're going to try to throw the player to this position
		v2s16 nodepos2d = v2s16(
<<<<<<< HEAD
			nodeposf.X -range + (myrand() % (range * 2)),
			nodeposf.Z -range + (myrand() % (range * 2)));

// FM version:
		// Get ground height at point
		s16 spawn_level = map.findGroundLevel(nodepos2d, cache_block_before_spawn);
		// Don't go underwater or to high places
		if (spawn_level <= water_level ||
				spawn_level > water_level + vertical_spawn_range)

/*MT :
		// Get spawn level at point
		s16 spawn_level = m_emerge->getSpawnLevelAtPoint(nodepos2d);
		// Continue if MAX_MAP_GENERATION_LIMIT was returned by
		// the mapgen to signify an unsuitable spawn position
		if (spawn_level == MAX_MAP_GENERATION_LIMIT)
*/
			continue;

		v3s16 nodepos(nodepos2d.X, nodeposf.Y + spawn_level, nodepos2d.Y);

		s32 air_count = 0;
		for (s32 i = vertical_spawn_range > 0 ? 0 : vertical_spawn_range - 50; i < vertical_spawn_range; i++) {
			v3s16 blockpos = getNodeBlockPos(nodepos);
			map.emergeBlock(blockpos, false);
			content_t c = map.getNodeNoEx(nodepos).getContent();
			if (c == CONTENT_AIR /*|| c == CONTENT_IGNORE*/) {
				air_count++;
				if (air_count >= min_air_height) {
=======
			-range + (myrand() % (range * 2)),
			-range + (myrand() % (range * 2)));
		// Get spawn level at point
		s16 spawn_level = m_emerge->getSpawnLevelAtPoint(nodepos2d);
		// Continue if MAX_MAP_GENERATION_LIMIT was returned by the mapgen to
		// signify an unsuitable spawn position, or if outside limits.
		if (spawn_level >= MAX_MAP_GENERATION_LIMIT ||
				spawn_level <= -MAX_MAP_GENERATION_LIMIT)
			continue;

		v3s16 nodepos(nodepos2d.X, spawn_level, nodepos2d.Y);
		// Consecutive empty nodes
		s32 air_count = 0;

		// Search upwards from 'spawn level' for 2 consecutive empty nodes, to
		// avoid obstructions in already-generated mapblocks.
		// In ungenerated mapblocks consisting of 'ignore' nodes, there will be
		// no obstructions, but mapgen decorations are generated after spawn so
		// the player may end up inside one.
		for (s32 i = 0; i < 8; i++) {
			v3s16 blockpos = getNodeBlockPos(nodepos);
			map.emergeBlock(blockpos, true);
			content_t c = map.getNode(nodepos).getContent();

			// In generated mapblocks allow spawn in all 'airlike' drawtype nodes.
			// In ungenerated mapblocks allow spawn in 'ignore' nodes.
			if (m_nodedef->get(c).drawtype == NDT_AIRLIKE || c == CONTENT_IGNORE) {
				air_count++;
				if (air_count >= 2) {
					// Spawn in lower empty node
					nodepos.Y--;
>>>>>>> 5.5.0
					nodeposf = intToFloat(nodepos, BS);
					// Don't spawn the player outside map boundaries
					if (objectpos_over_limit(nodeposf))
						// Exit this loop, positions above are probably over limit
						break;

					// Good position found, cause an exit from main loop
					is_good = true;
					break;
				}
			} else {
				air_count = 0;
			}
			nodepos.Y++;
		}
	}

	if (is_good)
		return nodeposf;

	// No suitable spawn point found, return fallback 0,0,0
	return v3f(0.0f, 0.0f, 0.0f);
}

void Server::requestShutdown(const std::string &msg, bool reconnect, float delay)
{
	if (delay == 0.0f) {
	// No delay, shutdown immediately
		m_shutdown_state.is_requested = true;
		// only print to the infostream, a chat message saying
		// "Server Shutting Down" is sent when the server destructs.
		infostream << "*** Immediate Server shutdown requested." << std::endl;
	} else if (delay < 0.0f && m_shutdown_state.isTimerRunning()) {
		// Negative delay, cancel shutdown if requested
		m_shutdown_state.reset();
		std::wstringstream ws;

		ws << L"*** Server shutdown canceled.";

		infostream << wide_to_utf8(ws.str()).c_str() << std::endl;
		SendChatMessage(PEER_ID_INEXISTENT, ws.str());
		// m_shutdown_* are already handled, skip.
		return;
	} else if (delay > 0.0f) {
	// Positive delay, tell the clients when the server will shut down
		std::wstringstream ws;

		ws << L"*** Server shutting down in "
				<< duration_to_string(myround(delay)).c_str()
				<< ".";

		infostream << wide_to_utf8(ws.str()).c_str() << std::endl;
		SendChatMessage(PEER_ID_INEXISTENT, ws.str());
	}

	m_shutdown_state.trigger(delay, msg, reconnect);
}

PlayerSAO* Server::emergePlayer(const char *name, session_t peer_id, u16 proto_version)
{
	/*
		Try to get an existing player
	*/
	RemotePlayer *player = m_env->getPlayer(name);

	// If player is already connected, cancel
	if (player && player->getPeerId() != PEER_ID_INEXISTENT) {
		infostream<<"emergePlayer(): Player already connected"<<std::endl;
		return NULL;
	}

	/*
		If player with the wanted peer_id already exists, cancel.
	*/
	if (m_env->getPlayer(peer_id)) {
		infostream<<"emergePlayer(): Player with wrong name but same"
				" peer_id already exists"<<std::endl;
		return NULL;
	}

<<<<<<< HEAD
	if (!player && maintenance_status) {
		infostream<<"emergePlayer(): Maintenance in progress, disallowing loading player"<<std::endl;
		return nullptr;
	}

	// Create a new player active object
	PlayerSAO *playersao = new PlayerSAO(m_env, peer_id, isSingleplayer());
	player = m_env->loadPlayer(name, playersao);

	// Create player if it doesn't exist
	if (!player) {
		newplayer = true;
		player = new RemotePlayer(name, this->idef());
		// Set player position
		infostream<<"Server: Finding spawn place for player \""
				<<name<<"\""<<std::endl;
		playersao->setBasePosition(findSpawnPos());

		// Add player to environment
		m_env->addPlayer(player);
	} else {
		// If the player exists, ensure that they respawn inside legal bounds
		// This fixes an assert crash when the player can't be added
		// to the environment
		if (objectpos_over_limit(playersao->getBasePosition())) {
			actionstream << "Respawn position for player \""
				<< name << "\" outside limits, resetting" << std::endl;
			playersao->setBasePosition(findSpawnPos());
		}
=======
	if (!player) {
		player = new RemotePlayer(name, idef());
>>>>>>> 5.5.0
	}

	bool newplayer = false;

	// Load player
	PlayerSAO *playersao = m_env->loadPlayer(player, &newplayer, peer_id, isSingleplayer());

	// Complete init with server parts
	playersao->finalize(player, getPlayerEffectivePrivs(player->getName()));
	player->protocol_version = proto_version;

	/* Run scripts */
	if (newplayer) {
		m_script->on_newplayer(playersao);
	}

	return playersao;
}

bool Server::registerModStorage(ModMetadata *storage)
{
	if (m_mod_storages.find(storage->getModName()) != m_mod_storages.end()) {
		errorstream << "Unable to register same mod storage twice. Storage name: "
				<< storage->getModName() << std::endl;
		return false;
	}

	m_mod_storages[storage->getModName()] = storage;
	return true;
}

void Server::unregisterModStorage(const std::string &name)
{
	std::unordered_map<std::string, ModMetadata *>::const_iterator it = m_mod_storages.find(name);
	if (it != m_mod_storages.end())
		m_mod_storages.erase(name);
}

void dedicated_server_loop(Server &server, bool &kill)
{
<<<<<<< HEAD
	DSTACK(FUNCTION_NAME);

	IntervalLimiter m_profiler_interval;

	int errors = 0;
	double run_time = 0;
	static const float steplen = g_settings->getFloat("dedicated_server_step");
	static const float profiler_print_interval =
=======
	verbosestream<<"dedicated_server_loop()"<<std::endl;

	IntervalLimiter m_profiler_interval;

	static thread_local const float steplen =
			g_settings->getFloat("dedicated_server_step");
	static thread_local const float profiler_print_interval =
>>>>>>> 5.5.0
			g_settings->getFloat("profiler_print_interval");

	/*
	 * The dedicated server loop only does time-keeping (in Server::step) and
	 * provides a way to main.cpp to kill the server externally (bool &kill).
	 */

	for(;;) {
		// This is kind of a hack but can be done like this
		// because server.step() is very light
<<<<<<< HEAD
		{
/*
			ScopeProfiler sp(g_profiler, "dedicated server sleep");
*/
			sleep_ms((int)(steplen*1000.0));
		}
		try {
		server.step(steplen);
		}
		//TODO: more errors here
		catch(std::exception &e) {
			if (!errors++ || !(errors % (int)(60/steplen)))
				errorstream<<"Fatal error n="<<errors<< " : "<<e.what()<<std::endl;
		}
		catch (...){
			if (!errors++ || !(errors % (int)(60/steplen)))
				errorstream<<"Fatal error unknown "<<errors<<std::endl;
		}
		if(server.getShutdownRequested() || kill)
		{
			infostream<<"Dedicated server quitting"<<std::endl;
=======
		sleep_ms((int)(steplen*1000.0));
		server.step(steplen);

		if (server.isShutdownRequested() || kill)
>>>>>>> 5.5.0
			break;

		run_time += steplen; // wrong not real time
		if (server.m_autoexit && run_time > server.m_autoexit && !server.lan_adv_server.clients_num) {
			server.requestShutdown("Automated server restart", true);
		}

		/*
			Profiler
		*/
		if (server.m_clients.getClientList().size() && profiler_print_interval) {
			if(m_profiler_interval.step(steplen, profiler_print_interval))
			{
				infostream<<"Profiler:"<<std::endl;
				g_profiler->print(infostream);
				g_profiler->clear();
			}
		}
	}

<<<<<<< HEAD
	if (server.m_autoexit || g_profiler_enabled) {
		actionstream << "Profiler:" << std::fixed << std::setprecision(9) << std::endl;
		g_profiler->print(actionstream);
	}

=======
	infostream << "Dedicated server quitting" << std::endl;
#if USE_CURL
	if (g_settings->getBool("server_announce"))
		ServerList::sendAnnounce(ServerList::AA_DELETE,
			server.m_bind_addr.getPort());
#endif
}

/*
 * Mod channels
 */


bool Server::joinModChannel(const std::string &channel)
{
	return m_modchannel_mgr->joinChannel(channel, PEER_ID_SERVER) &&
			m_modchannel_mgr->setChannelState(channel, MODCHANNEL_STATE_READ_WRITE);
}

bool Server::leaveModChannel(const std::string &channel)
{
	return m_modchannel_mgr->leaveChannel(channel, PEER_ID_SERVER);
}

bool Server::sendModChannelMessage(const std::string &channel, const std::string &message)
{
	if (!m_modchannel_mgr->canWriteOnChannel(channel))
		return false;

	broadcastModChannelMessage(channel, message, PEER_ID_SERVER);
	return true;
}

ModChannel* Server::getModChannel(const std::string &channel)
{
	return m_modchannel_mgr->getModChannel(channel);
}

void Server::broadcastModChannelMessage(const std::string &channel,
		const std::string &message, session_t from_peer)
{
	const std::vector<u16> &peers = m_modchannel_mgr->getChannelPeers(channel);
	if (peers.empty())
		return;

	if (message.size() > STRING_MAX_LEN) {
		warningstream << "ModChannel message too long, dropping before sending "
				<< " (" << message.size() << " > " << STRING_MAX_LEN << ", channel: "
				<< channel << ")" << std::endl;
		return;
	}

	std::string sender;
	if (from_peer != PEER_ID_SERVER) {
		sender = getPlayerName(from_peer);
	}

	NetworkPacket resp_pkt(TOCLIENT_MODCHANNEL_MSG,
			2 + channel.size() + 2 + sender.size() + 2 + message.size());
	resp_pkt << channel << sender << message;
	for (session_t peer_id : peers) {
		// Ignore sender
		if (peer_id == from_peer)
			continue;

		Send(peer_id, &resp_pkt);
	}

	if (from_peer != PEER_ID_SERVER) {
		m_script->on_modchannel_message(channel, sender, message);
	}
}

Translations *Server::getTranslationLanguage(const std::string &lang_code)
{
	if (lang_code.empty())
		return nullptr;

	auto it = server_translations.find(lang_code);
	if (it != server_translations.end())
		return &it->second; // Already loaded

	// [] will create an entry
	auto *translations = &server_translations[lang_code];

	std::string suffix = "." + lang_code + ".tr";
	for (const auto &i : m_media) {
		if (str_ends_with(i.first, suffix)) {
			std::string data;
			if (fs::ReadFile(i.second.path, data)) {
				translations->loadTranslation(data);
			}
		}
	}

	return translations;
}

ModMetadataDatabase *Server::openModStorageDatabase(const std::string &world_path)
{
	std::string world_mt_path = world_path + DIR_DELIM + "world.mt";
	Settings world_mt;
	if (!world_mt.readConfigFile(world_mt_path.c_str()))
		throw BaseException("Cannot read world.mt!");

	std::string backend = world_mt.exists("mod_storage_backend") ?
		world_mt.get("mod_storage_backend") : "files";
	if (backend == "files")
		warningstream << "/!\\ You are using the old mod storage files backend. "
			<< "This backend is deprecated and may be removed in a future release /!\\"
			<< std::endl << "Switching to SQLite3 is advised, "
			<< "please read http://wiki.minetest.net/Database_backends." << std::endl;

	return openModStorageDatabase(backend, world_path, world_mt);
}

ModMetadataDatabase *Server::openModStorageDatabase(const std::string &backend,
		const std::string &world_path, const Settings &world_mt)
{
	if (backend == "sqlite3")
		return new ModMetadataDatabaseSQLite3(world_path);

	if (backend == "files")
		return new ModMetadataDatabaseFiles(world_path);

	if (backend == "dummy")
		return new Database_Dummy();

	throw BaseException("Mod storage database backend " + backend + " not supported");
}

bool Server::migrateModStorageDatabase(const GameParams &game_params, const Settings &cmd_args)
{
	std::string migrate_to = cmd_args.get("migrate-mod-storage");
	Settings world_mt;
	std::string world_mt_path = game_params.world_path + DIR_DELIM + "world.mt";
	if (!world_mt.readConfigFile(world_mt_path.c_str())) {
		errorstream << "Cannot read world.mt!" << std::endl;
		return false;
	}

	std::string backend = world_mt.exists("mod_storage_backend") ?
		world_mt.get("mod_storage_backend") : "files";
	if (backend == migrate_to) {
		errorstream << "Cannot migrate: new backend is same"
			<< " as the old one" << std::endl;
		return false;
	}

	ModMetadataDatabase *srcdb = nullptr;
	ModMetadataDatabase *dstdb = nullptr;

	bool succeeded = false;

	try {
		srcdb = Server::openModStorageDatabase(backend, game_params.world_path, world_mt);
		dstdb = Server::openModStorageDatabase(migrate_to, game_params.world_path, world_mt);

		dstdb->beginSave();

		std::vector<std::string> mod_list;
		srcdb->listMods(&mod_list);
		for (const std::string &modname : mod_list) {
			StringMap meta;
			srcdb->getModEntries(modname, &meta);
			for (const auto &pair : meta) {
				dstdb->setModEntry(modname, pair.first, pair.second);
			}
		}

		dstdb->endSave();

		succeeded = true;

		actionstream << "Successfully migrated the metadata of "
			<< mod_list.size() << " mods" << std::endl;
		world_mt.set("mod_storage_backend", migrate_to);
		if (!world_mt.updateConfigFile(world_mt_path.c_str()))
			errorstream << "Failed to update world.mt!" << std::endl;
		else
			actionstream << "world.mt updated" << std::endl;

	} catch (BaseException &e) {
		errorstream << "An error occurred during migration: " << e.what() << std::endl;
	}

	delete srcdb;
	delete dstdb;

	if (succeeded && backend == "files") {
		// Back up files
		const std::string storage_path = game_params.world_path + DIR_DELIM + "mod_storage";
		const std::string backup_path = game_params.world_path + DIR_DELIM + "mod_storage.bak";
		if (!fs::Rename(storage_path, backup_path))
			warningstream << "After migration, " << storage_path
				<< " could not be renamed to " << backup_path << std::endl;
	}

	return succeeded;
>>>>>>> 5.5.0
}
