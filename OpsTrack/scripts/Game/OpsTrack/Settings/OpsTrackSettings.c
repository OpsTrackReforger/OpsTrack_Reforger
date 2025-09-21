class OpsTrackSettings : Managed
{
    // --- Felter ---
    string ApiBaseUrl;
    bool EnableConnectionEvents;
    bool EnableKillEvents;
    int MaxRetries;
	bool EnableDebug;

    // --- Ctor med defaults ---
    void OpsTrackSettings()
    {
        ApiBaseUrl = "http://127.0.0.1:5050";
        EnableConnectionEvents = true;
        EnableKillEvents = false;
        MaxRetries = 20;
		EnableDebug = false;
    }

    // --- Load/Save felter ---
    bool Load(SCR_JsonLoadContext ctx)
    {
        if (!ctx) return false;

        string s; bool b; int i;
        if (ctx.ReadValue("ApiBaseUrl", s)) ApiBaseUrl = s;
        if (ctx.ReadValue("EnableConnectionEvents", b)) EnableConnectionEvents = b;
        if (ctx.ReadValue("EnableKillEvents", b)) EnableKillEvents = b;
        if (ctx.ReadValue("MaxRetries", i)) MaxRetries = i;
		if (ctx.ReadValue("EnableDebug", b)) EnableDebug = b;

        return true;
    }

    void Save(SCR_JsonSaveContext ctx)
    {
        if (!ctx) return;
        ctx.WriteValue("ApiBaseUrl", ApiBaseUrl);
        ctx.WriteValue("EnableConnectionEvents", EnableConnectionEvents);
        ctx.WriteValue("EnableKillEvents", EnableKillEvents);
        ctx.WriteValue("MaxRetries", MaxRetries);
		ctx.WriteValue("EnableDebug", EnableDebug);
    }
}