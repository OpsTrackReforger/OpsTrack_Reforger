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
	string timeStamp;
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
		this.timeStamp = GetTimestampISO8601();
		this.eventType = eventType;
		
		
		//Validate Uids
		if(this.actorUid == "" || this.actorUid == "0") this.actorUid = "Environment";
		if(this.victimUid == "" || this.victimUid == "0") this.actorUid = "Environment";
		
	}
	
	private string GetTimestampISO8601()
	{
	    int year, month, day;
	    int hour, minute, second;
	
	    System.GetYearMonthDayUTC(year, month, day);
	    System.GetHourMinuteSecondUTC(hour, minute, second);
	
	    return string.Format(
	        "%1-%2-%3T%4:%5:%6Z",
	        PadLeft(year, 4),
	        PadLeft(month, 2),
	        PadLeft(day, 2),
	        PadLeft(hour, 2),
	        PadLeft(minute, 2),
	        PadLeft(second, 2)
	    );

	}

	string PadLeft(int value, int length)
	{
	    string s = value.ToString();
	    while (s.Length() < length)
	    {
	        s = "0" + s;
	    }
	    return s;
	}


}
