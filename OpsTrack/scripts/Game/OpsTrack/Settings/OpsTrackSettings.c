// OpsTrackSettings.c
// Configuration management with JSON serialization

class OpsTrackSettings : Managed
{
	// --- Fields ---
	string ApiBaseUrl;
	string ApiKey;
	bool EnableConnectionEvents;
	bool EnableKillEvents;
	int MaxRetries;
	bool EnableDebug;

	// --- Constructor with defaults ---
	void OpsTrackSettings()
	{
		ApiBaseUrl = "http://127.0.0.1:5050";
		ApiKey = "defaultSecretKey";
		EnableConnectionEvents = true;
		EnableKillEvents = false;
		MaxRetries = 20;
		EnableDebug = false;
	}

	// --- Load fields ---
	bool Load(SCR_JsonLoadContext ctx)
	{
		if (!ctx)
		{
			OpsTrackLogger.Error("Failed to load settings: JsonLoadContext is null.");
			return false;
		}

		string s;
		bool b;
		int i;
		
		if (ctx.ReadValue("ApiBaseUrl", s))
			ApiBaseUrl = s;
		
		if (ctx.ReadValue("ApiKey", s))
			ApiKey = s;
		
		if (ctx.ReadValue("EnableConnectionEvents", b))
			EnableConnectionEvents = b;
		
		if (ctx.ReadValue("EnableKillEvents", b))
			EnableKillEvents = b;
		
		if (ctx.ReadValue("MaxRetries", i))
			MaxRetries = i;
		
		if (ctx.ReadValue("EnableDebug", b))
			EnableDebug = b;

		// FIX: Don't log API key for security
		OpsTrackLogger.Debug(string.Format(
			"Settings loaded: ApiBaseUrl=%1, EnableConnectionEvents=%2, EnableKillEvents=%3, MaxRetries=%4, EnableDebug=%5",
			ApiBaseUrl,
			EnableConnectionEvents,
			EnableKillEvents,
			MaxRetries,
			EnableDebug
		));

		return true;
	}

	// --- Save fields ---
	void Save(SCR_JsonSaveContext ctx)
	{
		if (!ctx)
		{
			OpsTrackLogger.Error("Failed to save settings: JsonSaveContext is null.");
			return;
		}

		ctx.WriteValue("ApiBaseUrl", ApiBaseUrl);
		ctx.WriteValue("ApiKey", ApiKey);
		ctx.WriteValue("EnableConnectionEvents", EnableConnectionEvents);
		ctx.WriteValue("EnableKillEvents", EnableKillEvents);
		ctx.WriteValue("MaxRetries", MaxRetries);
		ctx.WriteValue("EnableDebug", EnableDebug);

		// FIX: Don't log API key for security
		OpsTrackLogger.Debug(string.Format(
			"Settings saved: ApiBaseUrl=%1, EnableConnectionEvents=%2, EnableKillEvents=%3, MaxRetries=%4, EnableDebug=%5",
			ApiBaseUrl,
			EnableConnectionEvents,
			EnableKillEvents,
			MaxRetries,
			EnableDebug
		));
	}
	
	// Validates settings and returns true if valid
	bool IsValid()
	{
		if (!ApiBaseUrl || ApiBaseUrl == "")
		{
			OpsTrackLogger.Error("Settings invalid: ApiBaseUrl is empty");
			return false;
		}
		
		if (!ApiKey || ApiKey == "")
		{
			OpsTrackLogger.Warn("Settings warning: ApiKey is empty");
			// Not a hard failure, but worth warning about
		}
		
		if (MaxRetries < 0)
		{
			OpsTrackLogger.Warn("Settings warning: MaxRetries is negative, using 0");
			MaxRetries = 0;
		}
		
		if (MaxRetries > 100)
		{
			OpsTrackLogger.Warn("Settings warning: MaxRetries is very high, capping at 100");
			MaxRetries = 100;
		}
		
		return true;
	}
}
