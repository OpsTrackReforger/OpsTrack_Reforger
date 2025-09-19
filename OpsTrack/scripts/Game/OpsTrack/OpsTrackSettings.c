// OpsTrackSettings.c
class OpsTrackSettings : Managed
{
    string ApiBaseUrl;
    bool EnableConnectionEvents;
    bool EnableKillEvents;
    int MaxRetries;

    void OpsTrackSettings()
    {
        ApiBaseUrl = "http://127.0.0.1:5050";
        EnableConnectionEvents = true;
        EnableKillEvents = false;
        MaxRetries = 20;
    }

    void Load(SCR_JsonLoadContext ctx)
    {
        string s; bool b; int i;
        ctx.ReadValue("ApiBaseUrl", s); ApiBaseUrl = s;
        ctx.ReadValue("EnableConnectionEvents", b); EnableConnectionEvents = b;
        ctx.ReadValue("EnableKillEvents", b); EnableKillEvents = b;
        ctx.ReadValue("MaxRetries", i); MaxRetries = i;
    }

    void Save(SCR_JsonSaveContext ctx)
    {
        ctx.WriteValue("ApiBaseUrl", ApiBaseUrl);
        ctx.WriteValue("EnableConnectionEvents", EnableConnectionEvents);
        ctx.WriteValue("EnableKillEvents", EnableKillEvents);
        ctx.WriteValue("MaxRetries", MaxRetries);
    }
}
