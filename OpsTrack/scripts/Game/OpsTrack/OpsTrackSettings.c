// OpsTrackSettings.c
class OpsTrackSettings : Managed
{
    // --- Felter ---
    string ApiBaseUrl;
    bool EnableConnectionEvents;
    bool EnableKillEvents;
    int MaxRetries;

    // --- Singleton instans ---
    private static ref OpsTrackSettings s_Instance;

    // --- Ctor med defaults ---
    private void OpsTrackSettings()
    {
        ApiBaseUrl = "http://127.0.0.1:5050";
        EnableConnectionEvents = true;
        EnableKillEvents = false;
        MaxRetries = 20;
    }

    // --- Singleton getter ---
    static OpsTrackSettings Get()
    {
        if (!s_Instance)
        {
            s_Instance = new OpsTrackSettings();
            s_Instance.LoadFromFile();
        }
        return s_Instance;
    }

    // --- Reload fra fil (kan kaldes runtime) ---
    void Reload()
    {
        LoadFromFile();
    }

    // --- Intern load/save helpers ---
    private void LoadFromFile()
    {
        SCR_JsonLoadContext loadCtx = new SCR_JsonLoadContext();
        if (loadCtx.LoadFromFile("OpsTrackSettings.json"))
        {
            Load(loadCtx);
            Print("[OpsTrack] Settings loaded");
        }
        else
        {
            SaveToFile(); // gem defaults
            Print("[OpsTrack] No settings file found, created default");
        }
    }

    private void SaveToFile()
    {
        SCR_JsonSaveContext saveCtx = new SCR_JsonSaveContext();
        Save(saveCtx);
        saveCtx.SaveToFile("OpsTrackSettings.json");
    }

    // --- Load/Save felter ---
    void Load(SCR_JsonLoadContext ctx)
    {
        string s; bool b; int i;
        if (ctx.ReadValue("ApiBaseUrl", s)) ApiBaseUrl = s;
        if (ctx.ReadValue("EnableConnectionEvents", b)) EnableConnectionEvents = b;
        if (ctx.ReadValue("EnableKillEvents", b)) EnableKillEvents = b;
        if (ctx.ReadValue("MaxRetries", i)) MaxRetries = i;
    }

    void Save(SCR_JsonSaveContext ctx)
    {
        ctx.WriteValue("ApiBaseUrl", ApiBaseUrl);
        ctx.WriteValue("EnableConnectionEvents", EnableConnectionEvents);
        ctx.WriteValue("EnableKillEvents", EnableKillEvents);
        ctx.WriteValue("MaxRetries", MaxRetries);
    }
}