class CombatEventSender : ApiClient
{
    private static ref CombatEventSender s_Instance;
	
	//Save last event-tid as (victim, actor)
	private ref map<string, float> m_LastWoundedEvent = new map<string, float>();

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
    void SendWounded(SCR_InstigatorContextData contextData)
    {	
		CombatEvent combatEvent = CreateCombatEvent(contextData, OpsTrack_EventType.WOUNDED);
        SendCombatEvent(combatEvent); // 2 = Wounded
    }

    void SendKill(SCR_InstigatorContextData contextData)
    {
		CombatEvent combatEvent = CreateCombatEvent(contextData, OpsTrack_EventType.KILL);
        SendCombatEvent(combatEvent); // 1 = Kill
    }
	
	
	protected CombatEvent CreateCombatEvent(SCR_InstigatorContextData contextData, OpsTrack_EventType eventType)
	{
		IEntity victim = contextData.GetVictimEntity();
		IEntity killerEntity = contextData.GetKillerEntity();
		Instigator instigator = contextData.GetInstigator();
		
		// Hent IDs og navne
		int victimId = contextData.GetVictimPlayerID();
		int actorId = contextData.GetKillerPlayerID();
		
		// Spam-protection
		string key = string.Format("%1:%2", victimId, actorId);
		float now = GetGame().GetWorld().GetWorldTime();
		if (m_LastWoundedEvent.Contains(key) && now - m_LastWoundedEvent.Get(key) < 200)
		{
			return null;
		}
		m_LastWoundedEvent.Set(key, now); //Set last wounded event

		string victimName = ResolveCharacterName(victim);
		
		if(victimId > 0)
		{
			victimName = GetGame().GetPlayerManager().GetPlayerName(victimId);
		}
		
		
		string actorName = ResolveCharacterName(killerEntity);

		if(actorId > 0)
		{
			actorName = GetGame().GetPlayerManager().GetPlayerName(actorId);
		}
		

		// Faktioner
		string victimFactionName = "Unknown";
		string actorFactionName = "Unknown";
		
		Faction victimFaction = GetFaction(victim, victimId);
		Faction killerFaction = GetFaction(killerEntity, actorId);

		if (victimFaction)
			victimFactionName = victimFaction.GetFactionName();

		if (killerFaction)
			actorFactionName = killerFaction.GetFactionName();

		// Distance
		int distance = 0;
		if (killerEntity && victim)
			distance = (int)vector.Distance(killerEntity.GetOrigin(), victim.GetOrigin());

		// Weapon
		string weaponName = ResolveWeaponName(instigator);

		// Relation
		bool isTeamKill = victimFaction.IsFactionFriendly(killerFaction);
		
		CombatEvent combatEvent = new CombatEvent(actorId, actorName, actorFactionName, victimId, victimName, victimFactionName, weaponName, distance, isTeamKill, eventType);
		
		return combatEvent;
		
	}

    // --- Core ---
    protected void SendCombatEvent(CombatEvent combatEvent, int attempt = 0)
    {
		
		if(!combatEvent) return; //Return if event is null
		
        if (!settings)
        {
            settings = OpsTrackManager.Get().GetSettings();
            if (!settings)
            {
                OpsTrackLogger.Error("Settings not available in CombatEventSender.");
                return;
            }
        }

        string json = BuildPayload(combatEvent);
        string endpoint = "/events/combat";

        OpsTrackLogger.Info(string.Format(
            "Sending CombatEvent type %1: Actor=%2, ActorName=%3, ActorFaction=%4, Victim=%5, VictimName=%6 VictimFaction=%7 Weapon=%8 Distance=%9 TeamKill=%10",
            combatEvent.eventType, combatEvent.actorUid, combatEvent.actorName, combatEvent.actorFactionName, combatEvent.victimUid, combatEvent.victimName, combatEvent.victimFactionName, combatEvent.weapon, combatEvent.distance
        ) +
		string.Format(" %1", combatEvent.isBlueOnBlue)
		
		);

        Post(endpoint, json);
    }

    // --- Payload builder ---
    protected string BuildPayload(CombatEvent combatEvent)
    {
        return string.Format(
        
		"{" +
			"\"eventTypeId\":%1," +
			"\"actorId\":\"%2\"," +
			"\"actorName\":\"%3\"," +
			"\"actorFaction\":\"%4\"," +
			"\"victimId\":\"%5\"," +
			"\"victimName\":\"%6\"," +
			"\"victimFaction\":\"%7\"," +
			"\"weapon\":\"%8\"," +
			"\"distance\":%9,",
		
            combatEvent.eventType,
            combatEvent.actorUid,
			combatEvent.actorName,
			combatEvent.actorFactionName,
            combatEvent.victimUid,
			combatEvent.victimName,
			combatEvent.victimFactionName,
            combatEvent.weapon,
            combatEvent.distance
        ) + string.Format(			
		"\"isTeamKill\":%1" +
		"}",
		combatEvent.isBlueOnBlue.ToString()
		);
    }
	
	
	
	//-----------------------------------------------------------------------------------------------------
	//Utilities
		// Utility
	protected string ResolveCharacterName(IEntity entity)
	{
		if (!entity)
			return "Environment";
	
		int playerId = SCR_PossessingManagerComponent.GetPlayerIdFromControlledEntity(entity);
		if (playerId > 0)
			return GetGame().GetPlayerManager().GetPlayerName(playerId);
	
		SCR_EditableEntityComponent entityComp = SCR_EditableEntityComponent.Cast(entity.FindComponent(SCR_EditableEntityComponent));
		if (entityComp)
			return entityComp.GetDisplayName();
	
		return entity.GetName(); // fallback
	}

	// Utility
	protected string ResolveWeaponName(Instigator inst)
	{
		if (!inst) return "Unknown";
		IEntity ent = inst.GetInstigatorEntity();
		if (!ent) return "Unknown";
	
		// Try infantry weapon
		CharacterWeaponManagerComponent weapMgr = CharacterWeaponManagerComponent.Cast(ent.FindComponent(CharacterWeaponManagerComponent));
		if (weapMgr)
		{
			BaseWeaponComponent weapon = weapMgr.GetCurrentWeapon();
			if (weapon)
			{
				UIInfo info = weapon.GetUIInfo();
				if (info)
					return info.GetName();
			}
		}
	
		return "Unknown";
	}
	
	//~ Prioritizes getting the faction of the entity but will try get player faction if the faction changes
	protected Faction GetFaction(IEntity entity, int playerID)
	{
		if (!entity)
			return GetFactionFromPlayerID(playerID);
		
		FactionAffiliationComponent factionAffiliation = FactionAffiliationComponent.Cast(entity.FindComponent(FactionAffiliationComponent));
		if (!factionAffiliation)
			return GetFactionFromPlayerID(playerID);
		
		return factionAffiliation.GetAffiliatedFaction();
	}
	
	//------------------------------------------------------------------------------------------------
	//~ Get faction off player ID
	protected Faction GetFactionFromPlayerID(int playerID)
	{
		if (playerID <= 0)
			return null;
		
		return SCR_FactionManager.SGetPlayerFaction(playerID);
	}
}