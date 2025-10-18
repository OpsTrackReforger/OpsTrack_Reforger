class CombatEventSender : ApiClient
{
    private static ref CombatEventSender s_Instance;

    private void CombatEventSender()
    {
        // Parent constructor initializes settings, m_Context and default m_Callback
    }

    static CombatEventSender Get()
    {
        if (!s_Instance)
            s_Instance = new CombatEventSender();
        return s_Instance;
    }

    // --- Public API ---
    void SendWounded(int actorId, string actorFactionName, int victimId, string victimFactionName, string weapon, bool isBlueOnBlue, float distance)
    {
        SendCombatEvent(actorId, actorFactionName, victimId, victimFactionName, weapon, distance, isBlueOnBlue, 2); // 2 = Wounded
    }

    //void SendKill(int actorId, int victimId, string weapon, bool isBlueOnBlue, float distance)
    //{
    //    SendCombatEvent(actorId, victimId, weapon, distance, isBlueOnBlue, 1); // 1 = Kill
    //}

    // --- Core ---
    protected void SendCombatEvent(int actorId, string actorFactionName, int victimId, string victimFactionName, string weapon, float distance, bool isBlueOnBlue, int eventTypeId, int attempt = 0)
    {
		
        if (!settings)
        {
            settings = OpsTrackManager.Get().GetSettings();
            if (!settings)
            {
                OpsTrackLogger.Error("Settings not available in CombatEventSender.");
                return;
            }
        }

        string actorUid = GetGame().GetBackendApi().GetPlayerIdentityId(actorId);
        string victimUid = GetGame().GetBackendApi().GetPlayerIdentityId(victimId);

        // Retry hvis actor eller victim Uid ikke er klar
        if ((actorUid == "" || victimUid == "") && attempt < settings.MaxRetries)
        {
            GetGame().GetCallqueue().CallLater(SendCombatEvent, 100, false, actorId, actorFactionName, victimId, victimFactionName, weapon, distance, isBlueOnBlue, eventTypeId, attempt + 1);
            return;
        }
		
		if(actorUid == "") actorUid = "Environment";
		if(victimUid == "") victimUid = "Environment";

        string json = BuildPayload(actorUid, actorFactionName, victimUid, victimFactionName, weapon, distance, isBlueOnBlue, eventTypeId);
        string endpoint = "/events/combat";

        OpsTrackLogger.Info(string.Format(
            "Sending CombatEvent type %1: Actor=%2, ActorFaction=%3, Victim=%4, VictimFaction=%5 Weapon=%6 Distance=%7 TeamKill=%8",
            eventTypeId, actorUid, actorFactionName, victimUid, victimFactionName, weapon, distance, isBlueOnBlue
        ));

        Post(endpoint, json);
    }

    // --- Payload builder ---
    protected string BuildPayload(string actorUid, string actorFactionName, string victimUid, string victimFactionName, string weapon, float distance, bool isBlueOnBlue, int eventTypeId)
    {
        return string.Format(
        
		"{" +
			"\"eventTypeId\":%1," +
			"\"actorId\":\"%2\"," +
			"\"actorFaction\":\"%3\"," +
			"\"victimId\":\"%4\"," +
			"\"victimFaction\":\"%5\"," +
			"\"weapon\":\"%6\"," +
			"\"distance\":%7," +
			"\"isTeamKill\":%8" +
		"}"
		,
            eventTypeId,
            actorUid,
			actorFactionName,
            victimUid,
			victimFactionName,
            weapon,
            distance,
            isBlueOnBlue.ToString()
        );
    }
}