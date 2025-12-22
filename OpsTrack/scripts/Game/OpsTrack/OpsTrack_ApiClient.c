class ApiClient
{
    protected RestContext m_Context;

    // Separate buffers
    protected ref array<string> m_ConnectionEvents;
    protected ref array<string> m_CombatEvents;

    protected int m_LastFlushTick = 0;

    // circuit breaker
    protected bool m_ApiEnabled = true;
    protected int m_NextRetryTick = 0;

    // tuning
    const int FLUSH_INTERVAL_MS = 1000;   // 1s
    const int MAX_BATCH_SIZE   = 25;
    const int COOLDOWN_MS      = 120000;  // 2 min

    void ApiClient()
    {
        OpsTrackSettings settings = OpsTrackManager.Get().GetSettings();
        
        RestApi api = GetGame().GetRestApi();
        m_Context = api.GetContext(settings.ApiBaseUrl);
        m_Context.SetHeaders("Content-Type,application/json,X-Api-Key," + settings.ApiKey);

        m_ConnectionEvents = new array<string>();
        m_CombatEvents = new array<string>();

        // Start active flush timer (runs every 1 second)
        GetGame().GetCallqueue().CallLater(CheckFlushTimer, 1000, true);
    }

    // called by game events
    void Enqueue(string eventJson, OpsTrack_EventType type)
    {
        if (!CanSend())
            return;

        // Determine event category
        if (type == OpsTrack_EventType.SELF_HARM ||
            type == OpsTrack_EventType.KILL ||
            type == OpsTrack_EventType.WOUNDED)
        {
            m_CombatEvents.Insert(eventJson);
        }
        else if (type == OpsTrack_EventType.JOIN ||
                 type == OpsTrack_EventType.LEAVE)
        {
            m_ConnectionEvents.Insert(eventJson);
        }

        int now = System.GetTickCount();
        int totalCount = m_ConnectionEvents.Count() + m_CombatEvents.Count();

        // Lazy flush (still valid)
        if (totalCount >= MAX_BATCH_SIZE || now - m_LastFlushTick >= FLUSH_INTERVAL_MS)
        {
            Flush();
        }
    }

    // Active flush timer (runs every second)
    void CheckFlushTimer()
    {
        // No events → nothing to flush
        if (m_ConnectionEvents.IsEmpty() && m_CombatEvents.IsEmpty())
            return;

        int now = System.GetTickCount();

        // If flush interval exceeded → flush
        if (now - m_LastFlushTick >= FLUSH_INTERVAL_MS)
        {
            Flush();
        }
    }

    protected bool CanSend()
    {
        if (!m_ApiEnabled)
        {
            if (System.GetTickCount() < m_NextRetryTick)
                return false;

            m_ApiEnabled = true;
        }
        return true;
    }

    protected void Flush()
    {
        if (m_ConnectionEvents.IsEmpty() && m_CombatEvents.IsEmpty())
            return;

        string payload = BuildBatchPayload();
        
        m_ConnectionEvents.Clear();
        m_CombatEvents.Clear();

        m_LastFlushTick = System.GetTickCount();

        OpsTrackCallback cb = new OpsTrackCallback(this);
        m_Context.POST(cb, "/events", payload);
		
		
		OpsTrackLogger.Debug("Sending payload: " + payload);
    }

    protected string BuildBatchPayload()
    {
        string payload = "{";

        // connectionEvents
        payload += "\"connectionEvents\":[";
        for (int i = 0; i < m_ConnectionEvents.Count(); i++)
        {
            payload += m_ConnectionEvents[i];
            if (i < m_ConnectionEvents.Count() - 1)
                payload += ",";
        }
        payload += "],";

        // combatEvents
        payload += "\"combatEvents\":[";
        for (int i = 0; i < m_CombatEvents.Count(); i++)
        {
            payload += m_CombatEvents[i];
            if (i < m_CombatEvents.Count() - 1)
                payload += ",";
        }
        payload += "]";

        payload += "}";

        return payload;
    }

    void Backoff()
    {
        m_ApiEnabled = false;
        m_NextRetryTick = System.GetTickCount() + COOLDOWN_MS;

        // drop old telemetry
        m_ConnectionEvents.Clear();
        m_CombatEvents.Clear();
    }
}