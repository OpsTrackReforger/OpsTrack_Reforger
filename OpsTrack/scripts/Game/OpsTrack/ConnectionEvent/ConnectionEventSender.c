// ConnectionEventSender.c
// Handles creation and sending of player connection events

class ConnectionEventSender
{
	private static ref ConnectionEventSender s_Instance;
	private OpsTrackSettings m_Settings;
	
	private void ConnectionEventSender()
	{
		RefreshSettings();
	}
	
	static ConnectionEventSender Get()
	{
		if (!s_Instance)
			s_Instance = new ConnectionEventSender();
		return s_Instance;
	}
	
	// Refresh settings reference (called on reload)
	void RefreshSettings()
	{
		OpsTrackManager manager = OpsTrackManager.GetIfExists();
		if (manager)
			m_Settings = manager.GetSettings();
	}

	// --- Public API ---
	void SendJoin(int playerId)
	{
		SendWithRetry(playerId, OpsTrack_EventType.JOIN, 0);
	}
	
	void SendLeave(int playerId)
	{
		SendWithRetry(playerId, OpsTrack_EventType.LEAVE, 0);
	}

	// --- Core Logic ---
	protected void SendWithRetry(int playerId, OpsTrack_EventType eventType, int attempt)
	{
		// Ensure settings are available
		if (!m_Settings)
		{
			RefreshSettings();
			if (!m_Settings)
			{
				OpsTrackLogger.Error("Settings not available in ConnectionEventSender.");
				return;
			}
		}

		// Get player identity
		string gameIdentity = OpsTrack_EntityUtils.GetPlayerIdentityIdSafe(playerId);
		string name = "";
		
		if (GetGame() && GetGame().GetPlayerManager())
			name = GetGame().GetPlayerManager().GetPlayerName(playerId);

		// Retry until identity is available or max retries reached
		if ((!gameIdentity || gameIdentity == "") && attempt < m_Settings.MaxRetries)
		{
			GetGame().GetCallqueue().CallLater(SendWithRetry, 100, false, playerId, eventType, attempt + 1);
			return;
		}

		// Give up if identity still not available
		if (!gameIdentity || gameIdentity == "")
		{
			OpsTrackLogger.Warn(string.Format(
				"Gave up waiting for identity for player %1 after %2 attempts.",
				playerId, attempt
			));
			return;
		}

		// Create and send event
		ConnectionEvent cEvent = new ConnectionEvent(gameIdentity, name, eventType);
		string json = cEvent.AsPayload();

		OpsTrackLogger.Info(string.Format(
			"Sending '%1' event for player '%2' (UID: %3).",
			eventType, name, gameIdentity
		));

		OpsTrackManager manager = OpsTrackManager.GetIfExists();
		if (manager)
		{
			ApiClient api = manager.GetApiClient();
			if (api)
				api.Enqueue(json, eventType);
			else
				OpsTrackLogger.Error("ApiClient not available");
		}
		else
		{
			OpsTrackLogger.Error("OpsTrackManager not available");
		}
	}
}
