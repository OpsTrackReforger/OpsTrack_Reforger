// CombatEvent.c
// Data class for combat events (kills, wounds, self-harm)

class CombatEvent
{
	string actorUid;
	string actorName;
	string actorFactionName;
	string victimUid;
	string victimName;
	string victimFactionName;
	string weapon;
	int distance;
	bool isBlueOnBlue;
	string timeStamp;
	OpsTrack_EventType eventType;
	
	void CombatEvent(int actorId, string actorNameParam, string actorFactionNameParam, int victimId, string victimNameParam, string victimFactionNameParam, string weaponParam, int distanceParam, bool isBlueOnBlueParam, OpsTrack_EventType eventTypeParam)
	{
		// Use safe getter for identity IDs
		this.actorUid = OpsTrack_EntityUtils.GetPlayerIdentityIdSafe(actorId);
		this.victimUid = OpsTrack_EntityUtils.GetPlayerIdentityIdSafe(victimId);
		
		this.actorName = actorNameParam;
		this.actorFactionName = actorFactionNameParam;
		this.victimName = victimNameParam;
		this.victimFactionName = victimFactionNameParam;
		this.weapon = weaponParam;
		this.distance = distanceParam;
		this.isBlueOnBlue = isBlueOnBlueParam;
		this.timeStamp = OpsTrack_DateTime.ToISO8601UTC();
		this.eventType = eventTypeParam;
		
		// Empty string for non-player entities (API expects empty or valid GUID)
		if (!this.actorUid || this.actorUid == "0")
			this.actorUid = "";
		
		if (!this.victimUid || this.victimUid == "0")
			this.victimUid = "";
		
		OpsTrackLogger.Debug(string.Format(
			"CombatEvent created: actorId=%1, victimId=%2, eventType=%3", 
			actorId, victimId, eventType
		));
	}
	
	string AsPayload()
	{
		// Convert bool to lowercase string for JSON
		string isTeamKillStr = "false";
		if (isBlueOnBlue)
			isTeamKillStr = "true";
		
		// Build JSON in parts to stay within string.Format limits
		string part1 = string.Format(
			"{" +
				"\"actorId\":\"%1\"," +
				"\"actorName\":\"%2\"," +
				"\"actorFaction\":\"%3\"," +
				"\"victimId\":\"%4\"," +
				"\"victimName\":\"%5\"," +
				"\"victimFaction\":\"%6\",",
			actorUid,
			actorName,
			actorFactionName,
			victimUid,
			victimName,
			victimFactionName
		);
		
		string part2 = string.Format(
			"\"weapon\":\"%1\"," +
			"\"distance\":%2," +
			"\"isTeamKill\":%3," +
			"\"timeStamp\":\"%4\"," +
			"\"eventTypeId\":%5" +
			"}",
			weapon,
			distance,
			isTeamKillStr,
			timeStamp,
			eventType
		);
		
		return part1 + part2;
	}
}
