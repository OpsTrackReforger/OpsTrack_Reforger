class OpsTrackSettings : Managed
{
    // --- Fields ---
    string ApiBaseUrl;
    bool EnableConnectionEvents;
    bool EnableKillEvents;
    int MaxRetries;
    bool EnableDebug;

    // --- Constructor with defaults ---
    void OpsTrackSettings()
    {
        ApiBaseUrl = "http://127.0.0.1:5050";
        EnableConnectionEvents = true;
        EnableKillEvents = false;
        MaxRetries = 20;
        EnableDebug = false;
    }

    // --- Load fields ---
    bool Load(SCR_JsonLoadContext ctx)
    {
        if (!ctx) {
            OpsTrackLogger.Error("Failed to load settings: JsonLoadContext is null.");
            return false;
        }

        string s; bool b; int i;
        if (ctx.ReadValue("ApiBaseUrl", s)) ApiBaseUrl = s;
        if (ctx.ReadValue("EnableConnectionEvents", b)) EnableConnectionEvents = b;
        if (ctx.ReadValue("EnableKillEvents", b)) EnableKillEvents = b;
        if (ctx.ReadValue("MaxRetries", i)) MaxRetries = i;
        if (ctx.ReadValue("EnableDebug", b)) EnableDebug = b;

        OpsTrackLogger.Debug("Settings loaded: ApiBaseUrl=" + ApiBaseUrl
            + ", EnableConnectionEvents=" + EnableConnectionEvents
            + ", EnableKillEvents=" + EnableKillEvents
            + ", MaxRetries=" + MaxRetries
            + ", EnableDebug=" + EnableDebug);

        return true;
    }

    // --- Save fields ---
    void Save(SCR_JsonSaveContext ctx)
    {
        if (!ctx) {
            OpsTrackLogger.Error("Failed to save settings: JsonSaveContext is null.");
            return;
        }

        ctx.WriteValue("ApiBaseUrl", ApiBaseUrl);
        ctx.WriteValue("EnableConnectionEvents", EnableConnectionEvents);
        ctx.WriteValue("EnableKillEvents", EnableKillEvents);
        ctx.WriteValue("MaxRetries", MaxRetries);
        ctx.WriteValue("EnableDebug", EnableDebug);

        OpsTrackLogger.Debug("Settings saved: ApiBaseUrl=" + ApiBaseUrl
            + ", EnableConnectionEvents=" + EnableConnectionEvents
            + ", EnableKillEvents=" + EnableKillEvents
            + ", MaxRetries=" + MaxRetries
            + ", EnableDebug=" + EnableDebug);
    }
}