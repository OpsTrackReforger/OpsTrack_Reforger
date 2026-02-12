// OpsTrack_StateTracker.c
// Tracks entity positions and sends batched updates to API during recording

class OpsTrack_StateTracker
{
	private static ref OpsTrack_StateTracker s_Instance;

	private bool m_IsTracking;
	private int m_UpdateIntervalMs;

	private static const int DEFAULT_UPDATE_INTERVAL_MS = 1000; // 1 second - capture positions every second
	// Note: We no longer batch in StateTracker - ApiClient handles all batching via unified flush

	private void OpsTrack_StateTracker()
	{
		m_IsTracking = false;
		m_UpdateIntervalMs = DEFAULT_UPDATE_INTERVAL_MS;
	}

	static OpsTrack_StateTracker Get()
	{
		// Always create fresh instance - static refs become stale after Workbench reload
		if (!s_Instance)
		{
			s_Instance = new OpsTrack_StateTracker();
			OpsTrackLogger.Info("StateTracker instance created");
		}
		return s_Instance;
	}

	// Reset static instance (call on game reload)
	static void Reset()
	{
		s_Instance = null;
	}

	// Start tracking (called when recording starts)
	void StartTracking()
	{
		if (m_IsTracking)
		{
			OpsTrackLogger.Warn("StartTracking called but already tracking");
			return;
		}

		m_IsTracking = true;
		OpsTrackLogger.Info("EntityState tracking started");

		// Schedule first position capture (one-shot, will reschedule itself)
		ScheduleNextCapture();
	}

	// Schedule the next position capture (one-shot callback that reschedules itself)
	protected void ScheduleNextCapture()
	{
		if (!m_IsTracking)
			return;

		if (GetGame() && GetGame().GetCallqueue())
		{
			// Use one-shot callback (false = non-repeating) - this works reliably in Enfusion
			GetGame().GetCallqueue().CallLater(CaptureAllPositions, m_UpdateIntervalMs, false);
		}
		else
		{
			OpsTrackLogger.Error("Cannot schedule capture: GetCallqueue() returned null!");
		}
	}

	// Stop tracking (called when recording stops)
	void StopTracking()
	{
		if (!m_IsTracking)
			return;

		m_IsTracking = false;

		// Force flush any remaining states before stopping
		OpsTrackManager manager = OpsTrackManager.GetIfExists();
		if (manager)
		{
			ApiClient api = manager.GetApiClient();
			if (api)
				api.ForceFlush();
		}

		OpsTrackLogger.Info("EntityState tracking stopped");
	}

	bool IsTracking()
	{
		return m_IsTracking;
	}

	// Capture positions of all tracked entities
	protected void CaptureAllPositions()
	{
		// Reschedule next capture FIRST (ensures continuous operation even if we return early)
		if (m_IsTracking)
			ScheduleNextCapture();

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

				// Queue entity assignment to current mission (ApiClient handles batching)
				if (!entityId.IsNull() && manager.IsRecording())
				{
					ApiClient api = manager.GetApiClient();
					if (api)
						api.EnqueueEntityAssignment(string.Format("%1", entityId));
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

			// Queue state directly to ApiClient (it handles batching)
			ApiClient api = manager.GetApiClient();
			if (api)
				api.EnqueueEntityState(state.AsPayload());
		}

		// After capturing all positions, check if it's time to flush
		ApiClient api = manager.GetApiClient();
		if (api)
			api.CheckAndFlush();
	}

}
