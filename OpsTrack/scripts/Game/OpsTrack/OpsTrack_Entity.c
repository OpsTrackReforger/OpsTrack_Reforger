//OpsTrack_Entity.c
//Data class for game entities (players, vehicles, AI)

class OpsTrack_Entity
{
	UUID entityId;
	string name;
	OpsTrack_EntityType type;
	string faction;
	string playerId;

	void OpsTrack_Entity(UUID id, string playerName, string factionName, string playerId, OpsTrack_EntityType type)
	{
		this.entityId = id;
		this.name = playerName;
		this.type = type;
		this.faction = factionName;
		this.playerId = playerId;
	}
	
    string AsPayload()
    {
        // PlayerId - null eller string
        string playerIdPart = "null";
        if (playerId != "")
            playerIdPart = "\"" + playerId + "\"";

        // Note: missionId is sent at batch level, not per entity
        return string.Format(
            "{" +
                "\"entityId\":\"%1\"," +
                "\"name\":\"%2\"," +
                "\"type\":%3," +
                "\"faction\":\"%4\"," +
                "\"playerId\":%5" +
            "}",
            entityId,
            name,
            type,
            faction,
            playerIdPart
        );
    }
}