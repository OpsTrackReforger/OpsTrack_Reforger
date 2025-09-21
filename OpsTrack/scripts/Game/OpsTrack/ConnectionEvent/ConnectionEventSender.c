// ConnectionEventSender.c
class ConnectionEventSender : ApiClient
{
    void ConnectionEventSender()
    {
        // Parent ctor initialiserer settings, m_Context og default m_Callback
        // Hvis du vil have en specifik callback, kan du overskrive her:
        // m_Callback = new OpsTrackCallback();
    }

    protected string BuildPayload(string uid, string name, string eventType)
    {
        return string.Format("{\"gameIdentity\":\"%1\",\"name\":\"%2\"}", uid, name);
    }

    protected void SendWithRetry(int playerId, string eventType, int attempt = 0)
    {
        // Lazy guard: sikr at settings er sat
        if (!settings) {
            settings = OpsTrackManager.Get().GetSettings();
            if (!settings) {
                Print("[OpsTrack] ERROR: Settings not available in ConnectionEventSender");
                return;
            }
        }

        string uid = GetGame().GetBackendApi().GetPlayerIdentityId(playerId);
        string name = GetGame().GetPlayerManager().GetPlayerName(playerId);

        if (uid == "" && attempt < settings.MaxRetries)
        {
            GetGame().GetCallqueue().CallLater(SendWithRetry, 100, false, playerId, eventType, attempt + 1);
            return;
        }

        if (uid == "")
        {
            Print(string.Format("[OpsTrack] Gave up waiting for identity for player %1 after %2 attempts", playerId, attempt));
            return;
        }

        string json = BuildPayload(uid, name, eventType);
        string endpoint = "/player/" + eventType;
        Post(endpoint, json);
    }

    void SendJoin(int playerId)  { SendWithRetry(playerId, "join");  }
    void SendLeave(int playerId) { SendWithRetry(playerId, "leave"); }
}