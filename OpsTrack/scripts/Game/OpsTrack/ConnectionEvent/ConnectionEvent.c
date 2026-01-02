// ConnectionEvent.c
// Data class for player connection events (join/leave)

class ConnectionEvent
{
	string GameIdentity;
	string Name;
	OpsTrack_EventType EventTypeId;
	string TimeStamp;
	
	void ConnectionEvent(string gameIdentity, string name, OpsTrack_EventType eventTypeId)
	{
		this.GameIdentity = gameIdentity;
		this.Name = name;
		this.EventTypeId = eventTypeId;
		this.TimeStamp = OpsTrack_DateTime.ToISO8601UTC();
	}
	
	string AsPayload()
	{
		return string.Format(
			"{" + 
				"\"gameIdentity\":\"%1\"," + 
				"\"name\":\"%2\"," +
				"\"timeStamp\":\"%3\"," +
				"\"eventTypeId\":%4" +
			"}",
			GameIdentity,
			Name,
			TimeStamp,
			EventTypeId
		);
	}
}
