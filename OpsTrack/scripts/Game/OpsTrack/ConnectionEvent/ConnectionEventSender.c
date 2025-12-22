class ConnectionEventSender
{
	private static ref ConnectionEventSender s_Instance;
	private OpsTrackSettings settings;
	
    private void ConnectionEventSender()
    {
        settings = OpsTrackManager.Get().GetSettings();
    }
	
	static ConnectionEventSender Get()
	{
		if(!s_Instance)
		{
			s_Instance = new ConnectionEventSender();
		}
		return s_Instance;
	}

	protected string BuildPayload(string uid, string name, OpsTrack_EventType eventTypeId)
	{
	    string timestamp = GetTimestampISO8601(); // eller din egen timestamp-funktion
	    return string.Format(
	        "{\"gameIdentity\":\"%1\",\"name\":\"%2\",\"timeStamp\":\"%3\",\"eventTypeId\":%4}",
	        uid,
	        name,
	        timestamp,
	        eventTypeId
	    );
	}
		
	string GetTimestampISO8601()
	{
	    int year, month, day;
	    int hour, minute, second;
	
	    System.GetYearMonthDayUTC(year, month, day);
	    System.GetHourMinuteSecondUTC(hour, minute, second);
	

	    return string.Format(
	        "%1-%2-%3T%4:%5:%6Z",
	        PadLeft(year, 4),
	        PadLeft(month, 2),
	        PadLeft(day, 2),
	        PadLeft(hour, 2),
	        PadLeft(minute, 2),
	        PadLeft(second, 2)
	    );

	}



    protected void SendWithRetry(int playerId, OpsTrack_EventType eventType, int attempt = 0)
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

        OpsTrackLogger.Info(string.Format(
            "Sending '%1' event for player '%2' (ID: %3, UID: %4).",
            eventType, name, playerId, uid
        ));

		OpsTrackManager.Get().GetApiClient().Enqueue(json, eventType);
    }
	
	string PadLeft(int value, int length)
	{
	    string s = value.ToString();
	    while (s.Length() < length)
	    {
	        s = "0" + s;
	    }
	    return s;
	}


    void SendJoin(int playerId)  { SendWithRetry(playerId, OpsTrack_EventType.JOIN);  }
    void SendLeave(int playerId) { SendWithRetry(playerId, OpsTrack_EventType.LEAVE); }
}