class CombatEvent
{
	string actorUid;
	string actorName;
	string actorFactionName;
	string victimUid;
	string victimName;
	string victimFactionName;
	string weapon;
	float distance;
	bool isBlueOnBlue;
	OpsTrack_EventType eventType;
	
	void CombatEvent(int actorId, string actorName, string actorFactionName, int victimId, string victimName, string victimFactionName, string weapon, float distance, bool isBlueOnBlue, OpsTrack_EventType eventType)
	{
		this.actorUid = GetGame().GetBackendApi().GetPlayerIdentityId(actorId);
		this.actorName = actorName;
		this.actorFactionName = actorFactionName;
		this.victimUid = GetGame().GetBackendApi().GetPlayerIdentityId(victimId);
		this.victimName = victimName;
		this.victimFactionName = victimFactionName;
		this.weapon = weapon;
		this.distance = distance;
		this.isBlueOnBlue = isBlueOnBlue;
		this.eventType = eventType;
		
		
		//Validate Uids
		if(this.actorUid == "" || this.actorUid == "0") this.actorUid = "Environment";
		if(this.victimUid == "" || this.victimUid == "0") this.actorUid = "Environment";
		
	}
	

}
