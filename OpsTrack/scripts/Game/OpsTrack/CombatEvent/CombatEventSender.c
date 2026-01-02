// CombatEventSender.c
// Handles creation and sending of combat events

class CombatEventSender
{
	private static ref CombatEventSender s_Instance;
	private OpsTrackSettings m_Settings;
	
	// Spam protection: map of "eventType:victimId:actorId" -> timestamp
	// Only applies to WOUNDED events (prevents flood from explosions/fire)
	private ref map<string, float> m_LastWoundedTime;
	
	private static const int CLEANUP_THRESHOLD = 100;
	private static const int CLEANUP_AGE_MS = 60000; // 1 minute
	private static const int SPAM_WINDOW_MS = 200;   // Minimum time between same victim/actor pair for WOUNDED
	
	private float m_LastCleanupTime;

	private void CombatEventSender()
	{
		m_LastWoundedTime = new map<string, float>();
		m_LastCleanupTime = 0;
		RefreshSettings();
	}

	static CombatEventSender Get()
	{
		if (!s_Instance)
			s_Instance = new CombatEventSender();
		return s_Instance;
	}
	
	// Refresh settings reference (called on reload)
	void RefreshSettings()
	{
		OpsTrackManager manager = OpsTrackManager.GetIfExists();
		if (manager)
			m_Settings = manager.GetSettings();
	}

	// --- Public API ---
	void SendWounded(SCR_InstigatorContextData contextData)
	{
		OpsTrackLogger.Debug("SendWounded called");
		
		// Wounded events use spam protection (grenades, fire, etc. cause many hits)
		CombatEvent combatEvent = CreateCombatEvent(contextData, OpsTrack_EventType.WOUNDED, true);
		if (!combatEvent)
		{
			OpsTrackLogger.Debug("Wounded event suppressed (spam protection or creation failed)");
			return;
		}
		
		SendCombatEvent(combatEvent);
	}

	void SendKill(SCR_InstigatorContextData contextData)
	{
		OpsTrackLogger.Debug("SendKill called");
		
		// Kill events are NEVER spam protected - a kill is always important
		CombatEvent combatEvent = CreateCombatEvent(contextData, OpsTrack_EventType.KILL, false);
		if (!combatEvent)
		{
			OpsTrackLogger.Debug("Kill event creation failed");
			return;
		}
		
		SendCombatEvent(combatEvent);
	}
	
	void SendSelfHarm(SCR_InstigatorContextData contextData)
	{
		OpsTrackLogger.Debug("SendSelfHarm called");
		
		// Self-harm events are NEVER spam protected
		CombatEvent combatEvent = CreateCombatEvent(contextData, OpsTrack_EventType.SELF_HARM, false);
		if (!combatEvent)
		{
			OpsTrackLogger.Debug("SelfHarm event creation failed");
			return;
		}
		
		SendCombatEvent(combatEvent);
	}
	
	// --- Event Creation ---
	protected CombatEvent CreateCombatEvent(SCR_InstigatorContextData contextData, OpsTrack_EventType eventType, bool useSpamProtection)
	{
		if (!contextData)
		{
			OpsTrackLogger.Warn("CreateCombatEvent called with null contextData");
			return null;
		}
		
		IEntity victim = contextData.GetVictimEntity();
		IEntity killerEntity = contextData.GetKillerEntity();
		Instigator instigator = contextData.GetInstigator();
		
		// Get current time
		float now = 0;
		if (GetGame() && GetGame().GetWorld())
			now = GetGame().GetWorld().GetWorldTime();
		
		// Periodic cleanup of old entries
		if (m_LastWoundedTime.Count() > CLEANUP_THRESHOLD && (now - m_LastCleanupTime) > CLEANUP_AGE_MS)
		{
			CleanupOldEvents(now);
			m_LastCleanupTime = now;
		}
		
		// Get IDs
		int victimId = contextData.GetVictimPlayerID();
		int actorId = contextData.GetKillerPlayerID();
		
		// Spam protection - ONLY for wounded events
		if (useSpamProtection)
		{
			string spamKey = string.Format("%1:%2", victimId, actorId);
			if (m_LastWoundedTime.Contains(spamKey))
			{
				float lastTime = m_LastWoundedTime.Get(spamKey);
				if (now - lastTime < SPAM_WINDOW_MS)
				{
					OpsTrackLogger.Debug(string.Format("Wounded event suppressed (spam): %1", spamKey));
					return null;
				}
			}
			m_LastWoundedTime.Set(spamKey, now);
		}

		// Resolve names using shared utility
		string victimName = OpsTrack_EntityUtils.ResolveCharacterName(victim);
		if (victimId > 0)
		{
			string playerName = GetGame().GetPlayerManager().GetPlayerName(victimId);
			if (playerName && playerName != "")
				victimName = playerName;
		}
		
		string actorName = OpsTrack_EntityUtils.ResolveCharacterName(killerEntity);
		if (actorId > 0)
		{
			string playerName = GetGame().GetPlayerManager().GetPlayerName(actorId);
			if (playerName && playerName != "")
				actorName = playerName;
		}

		// Resolve factions using shared utility
		string victimFactionName = "Unknown";
		string actorFactionName = "Unknown";
		
		Faction victimFaction = OpsTrack_EntityUtils.GetFaction(victim, victimId);
		Faction killerFaction = OpsTrack_EntityUtils.GetFaction(killerEntity, actorId);

		if (victimFaction)
			victimFactionName = victimFaction.GetFactionName();

		if (killerFaction)
			actorFactionName = killerFaction.GetFactionName();

		// Calculate distance
		float distance = 0;
		if (killerEntity && victim)
			distance = vector.Distance(killerEntity.GetOrigin(), victim.GetOrigin());

		// Resolve weapon using shared utility
		string weaponName = OpsTrack_EntityUtils.ResolveWeaponName(instigator);

		// Determine team kill status
		bool isTeamKill = false;
		if (victimFaction && killerFaction)
		{
			isTeamKill = victimFaction.IsFactionFriendly(killerFaction);
		}
		else
		{
			OpsTrackLogger.Debug("Cannot determine team kill status: faction data missing");
		}
		
		// Create and return event
		return new CombatEvent(
			actorId, actorName, actorFactionName,
			victimId, victimName, victimFactionName,
			weaponName, distance, isTeamKill, eventType
		);
	}

	// --- Core Send Logic ---
	protected void SendCombatEvent(CombatEvent combatEvent)
	{
		if (!combatEvent)
		{
			OpsTrackLogger.Debug("SendCombatEvent called with null event");
			return;
		}
		
		// Ensure settings are available
		if (!m_Settings)
		{
			RefreshSettings();
			if (!m_Settings)
			{
				OpsTrackLogger.Error("Settings not available in CombatEventSender.");
				return;
			}
			OpsTrackLogger.Warn("Settings were null, reloaded from OpsTrackManager");
		}

		string json = combatEvent.AsPayload();
		OpsTrackLogger.Debug(string.Format("Combat event JSON: %1", json));

		OpsTrackLogger.Info(string.Format(
			"Sending CombatEvent: type=%1, actor=%2 (%3), victim=%4 (%5), weapon=%6, distance=%7m, teamkill=%8",
			combatEvent.eventType,
			combatEvent.actorName,
			combatEvent.actorFactionName,
			combatEvent.victimName,
			combatEvent.victimFactionName,
			combatEvent.weapon,
			combatEvent.distance,
			combatEvent.isBlueOnBlue
		));

		OpsTrackManager manager = OpsTrackManager.GetIfExists();
		if (manager)
		{
			ApiClient api = manager.GetApiClient();
			if (api)
				api.Enqueue(json, combatEvent.eventType);
			else
				OpsTrackLogger.Error("ApiClient not available");
		}
		else
		{
			OpsTrackLogger.Error("OpsTrackManager not available");
		}
	}
	
	// --- Cleanup ---
	protected void CleanupOldEvents(float currentTime)
	{
		float threshold = currentTime - CLEANUP_AGE_MS;
		
		ref array<string> keysToRemove = new array<string>();
		
		foreach (string key, float timestamp : m_LastWoundedTime)
		{
			if (timestamp < threshold)
				keysToRemove.Insert(key);
		}
		
		foreach (string key : keysToRemove)
		{
			m_LastWoundedTime.Remove(key);
		}
		
		int removedCount = keysToRemove.Count();
		if (removedCount > 0)
			OpsTrackLogger.Debug(string.Format("Cleaned up %1 old wounded event entries", removedCount));
	}
}
