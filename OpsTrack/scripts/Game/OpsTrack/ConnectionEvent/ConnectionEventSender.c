class ConnectionEventSender : ApiClient
{
	private static ref ConnectionEventSender s_Instance;
	
	
    private void ConnectionEventSender()
    {
        // Parent constructor initializes settings, m_Context and default m_Callback
        // If you want a specific callback, you can override here:
        // m_Callback = new OpsTrackCallback();
    }
	
	static ConnectionEventSender Get()
	{
		if(!s_Instance)
		{
			s_Instance = new ConnectionEventSender();
		}
		return s_Instance;
	}

    protected string BuildPayload(string uid, string name, string eventType)
    {
        return string.Format("{\"gameIdentity\":\"%1\",\"name\":\"%2\"}", uid, name);
    }

    protected void SendWithRetry(int playerId, string eventType, int attempt = 0)
    {
        // Ensure settings are available
        if (!settings) {
            settings = OpsTrackManager.Get().GetSettings();
            if (!settings) {
                OpsTrackLogger.Error("Settings not available in ConnectionEventSender.");
                return;
            }
        }

        string uid = GetGame().GetBackendApi().GetPlayerIdentityId(playerId);
        string name = GetGame().GetPlayerManager().GetPlayerName(playerId);

        // Retry until identity is available or max retries reached
        if (uid == "" && attempt < settings.MaxRetries) {
            GetGame().GetCallqueue().CallLater(SendWithRetry, 100, false, playerId, eventType, attempt + 1);
            return;
        }

        if (uid == "") {
            OpsTrackLogger.Warn(string.Format(
                "Gave up waiting for identity for player %1 after %2 attempts.",
                playerId, attempt
            ));
            return;
        }

        string json = BuildPayload(uid, name, eventType);
        string endpoint = "/events/connections/" + eventType;

        OpsTrackLogger.Info(string.Format(
            "Sending '%1' event for player '%2' (ID: %3, UID: %4).",
            eventType, name, playerId, uid
        ));

        Post(endpoint, json);
    }

    void SendJoin(int playerId)  { SendWithRetry(playerId, "join");  }
    void SendLeave(int playerId) { SendWithRetry(playerId, "leave"); }
}