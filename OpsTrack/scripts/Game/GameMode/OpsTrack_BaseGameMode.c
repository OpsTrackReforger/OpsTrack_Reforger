// OpsTrack_BaseGameMode.c
// Hooks into SCR_BaseGameMode for player connection and kill tracking

modded class SCR_BaseGameMode
{
	private ref ConnectionEventSender m_ConnectionEvents;
	private ref CombatEventSender m_CombatEvents;
	private bool m_OpsTrackEnabled;
	private bool m_KillEventsEnabled;

	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);

		// Only initialize on server
		if (!Replication.IsServer())
			return;
		
		// Initialize OpsTrack
		OpsTrackManager manager = OpsTrackManager.Get();
		if (!manager)
		{
			OpsTrackLogger.Error("Failed to initialize OpsTrackManager.");
			m_OpsTrackEnabled = false;
			return;
		}
		
		OpsTrackSettings settings = manager.GetSettings();
		if (!settings)
		{
			OpsTrackLogger.Error("Failed to get OpsTrack settings.");
			m_OpsTrackEnabled = false;
			return;
		}
		
		// Respect feature toggles
		m_OpsTrackEnabled = settings.EnableConnectionEvents;
		m_KillEventsEnabled = settings.EnableKillEvents;

		if (m_OpsTrackEnabled)
		{
			m_ConnectionEvents = ConnectionEventSender.Get();
			OpsTrackLogger.Info("OpsTrack initialized. Connection events enabled.");
		}
		else
		{
			OpsTrackLogger.Info("OpsTrack initialized. Connection events disabled by settings.");
		}
		
		if (m_KillEventsEnabled)
		{
			m_CombatEvents = CombatEventSender.Get();
			OpsTrackLogger.Info("OpsTrack: Kill events enabled.");
		}
		else
		{
			OpsTrackLogger.Info("OpsTrack: Kill events disabled by settings.");
		}
	}

	override void OnPlayerRegistered(int playerId)
	{
		super.OnPlayerRegistered(playerId);

		if (!Replication.IsServer())
			return;
		
		if (!m_OpsTrackEnabled)
			return;
		
		if (!m_ConnectionEvents)
		{
			OpsTrackLogger.Warn("OnPlayerRegistered: ConnectionEventSender not available");
			return;
		}
		
		m_ConnectionEvents.SendJoin(playerId);
	}

	override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		// Send leave event BEFORE calling super (which may clean up player data)
		if (Replication.IsServer() && m_OpsTrackEnabled && m_ConnectionEvents)
		{
			m_ConnectionEvents.SendLeave(playerId);
		}

		super.OnPlayerDisconnected(playerId, cause, timeout);
	}
	
	
	
	// Called when player spawn is finalized (player is now controlling the entity)
	override void OnPlayerSpawnFinalize_S(SCR_SpawnRequestComponent requestComponent, SCR_SpawnHandlerComponent handlerComponent, SCR_SpawnData data, IEntity entity)
	{
		super.OnPlayerSpawnFinalize_S(requestComponent, handlerComponent, data, entity);

		if (!entity)
			return;

		OpsTrackManager manager = OpsTrackManager.Get();
		if (!manager)
			return;

		OpsTrack_EntityManager entityMgr = manager.GetEntityManager();
		if (!entityMgr)
			return;

		int playerId = requestComponent.GetPlayerId();
		if (playerId <= 0)
			return;

		string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
		string factionName = "Unknown";

		Faction faction = OpsTrack_EntityUtils.GetFaction(entity, playerId);
		if (faction)
			factionName = faction.GetFactionName();

		entityMgr.GetOrCreatePlayerEntity(playerId, playerName, factionName);
	}

	
	// Called when any controllable entity is destroyed (players AND AI)
	override void OnControllableDestroyed(IEntity entity, IEntity killerEntity, notnull Instigator instigator)
	{
		super.OnControllableDestroyed(entity, killerEntity, instigator);
		
		if (!Replication.IsServer())
			return;
		
		if (!m_KillEventsEnabled || !m_CombatEvents)
			return;
		
		// Get player ID (0 for AI)
		int victimPlayerId = 0;
		if (GetGame() && GetGame().GetPlayerManager())
			victimPlayerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(entity);
		
		// Determine if self-harm
		bool isSelfHarm = (killerEntity == entity);
		
		// Create context data
		SCR_InstigatorContextData contextData = new SCR_InstigatorContextData(
			victimPlayerId,
			entity,
			killerEntity,
			instigator,
			false
		);
		
		if (isSelfHarm)
		{
			m_CombatEvents.SendSelfHarm(contextData);
		}
		else
		{
			m_CombatEvents.SendKill(contextData);
		}
	}
}
