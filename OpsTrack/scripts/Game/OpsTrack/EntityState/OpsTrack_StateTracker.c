// OpsTrack_StateTracker.c
// Tracks entity positions and sends batched updates to API during recording

class OpsTrack_StateTracker
{
	private static ref OpsTrack_StateTracker s_Instance;

	private ref array<string> m_PendingStates;
	private bool m_IsTracking;
	private int m_UpdateIntervalMs;

	private static const int DEFAULT_UPDATE_INTERVAL_MS = 1000; // 1 second
	private static const int MAX_BATCH_SIZE = 50;

	private void OpsTrack_StateTracker()
	{
		m_PendingStates = new array<string>();
		m_IsTracking = false;
		m_UpdateIntervalMs = DEFAULT_UPDATE_INTERVAL_MS;
	}

	static OpsTrack_StateTracker Get()
	{
		if (!s_Instance)
			s_Instance = new OpsTrack_StateTracker();
		return s_Instance;
	}

	// Start tracking (called when recording starts)
	void StartTracking()
	{
		if (m_IsTracking)
			return;

		m_IsTracking = true;

		// Start periodic position capture
		if (GetGame() && GetGame().GetCallqueue())
			GetGame().GetCallqueue().CallLater(CaptureAllPositions, m_UpdateIntervalMs, true);

		OpsTrackLogger.Info("EntityState tracking started");
	}

	// Stop tracking (called when recording stops)
	void StopTracking()
	{
		if (!m_IsTracking)
			return;

		m_IsTracking = false;

		// Flush any remaining states
		FlushStates();

		// Clear pending states
		m_PendingStates.Clear();

		OpsTrackLogger.Info("EntityState tracking stopped");
	}

	bool IsTracking()
	{
		return m_IsTracking;
	}

	// Capture positions of all tracked entities
	protected void CaptureAllPositions()
	{
		if (!m_IsTracking)
			return;

		OpsTrackManager manager = OpsTrackManager.GetIfExists();
		if (!manager || !manager.IsRecording())
		{
			StopTracking();
			return;
		}

		OpsTrack_EntityManager entityMgr = manager.GetEntityManager();
		if (!entityMgr)
			return;

		// Get current timestamp (seconds since epoch)
		int timestamp = System.GetUnixTime();

		// Get all player entities and their controlled characters
		PlayerManager playerMgr = GetGame().GetPlayerManager();
		if (!playerMgr)
			return;

		array<int> playerIds = {};
		playerMgr.GetPlayers(playerIds);

		foreach (int playerId : playerIds)
		{
			IEntity controlledEntity = playerMgr.GetPlayerControlledEntity(playerId);
			if (!controlledEntity)
				continue;

			UUID entityId = entityMgr.GetEntityId(playerId);

			// If player doesn't have an entity yet, create one (handles Game Master spawns, etc.)
			if (entityId.IsNull())
			{
				string playerName = playerMgr.GetPlayerName(playerId);
				string factionName = "Unknown";
				Faction faction = OpsTrack_EntityUtils.GetFaction(controlledEntity, playerId);
				if (faction)
					factionName = faction.GetFactionName();

				entityId = entityMgr.GetOrCreatePlayerEntity(playerId, playerName, factionName);

				// Entity needs to be assigned to current mission
				if (!entityId.IsNull() && manager.IsRecording())
				{
					ApiClient api = manager.GetApiClient();
					if (api)
					{
						string assignPayload = string.Format(
							"{\"missionId\":\"%1\",\"entityIds\":[\"%2\"]}",
							manager.GetCurrentMissionId(),
							entityId
						);
						api.AssignEntitiesToMission(assignPayload);
					}
				}
			}

			// Get position
			vector pos = controlledEntity.GetOrigin();

			// Get rotation (yaw)
			vector angles = controlledEntity.GetYawPitchRoll();
			float rotation = angles[0]; // Yaw

			// Check if alive
			bool isAlive = true;
			SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(
				controlledEntity.FindComponent(SCR_CharacterControllerComponent)
			);
			if (controller)
				isAlive = !controller.IsDead();

			// Create state
			OpsTrack_EntityState state = new OpsTrack_EntityState(
				entityId,
				timestamp,
				pos[0], pos[1], pos[2],
				rotation,
				isAlive
			);

			m_PendingStates.Insert(state.AsPayload());
		}

		// Flush if we have enough states
		if (m_PendingStates.Count() >= MAX_BATCH_SIZE)
			FlushStates();
	}

	// Flush states to API
	protected void FlushStates()
	{
		if (m_PendingStates.Count() == 0)
			return;

		OpsTrackManager manager = OpsTrackManager.GetIfExists();
		if (!manager)
			return;

		ApiClient api = manager.GetApiClient();
		if (!api)
			return;

		string payload = BuildBatchPayload();
		int count = m_PendingStates.Count();

		m_PendingStates.Clear();

		api.SendEntityStates(payload);
	}

	protected string BuildBatchPayload()
	{
		string payload = "{\"states\":[";

		for (int i = 0; i < m_PendingStates.Count(); i++)
		{
			payload += m_PendingStates[i];
			if (i < m_PendingStates.Count() - 1)
				payload += ",";
		}

		payload += "]}";
		return payload;
	}

}
