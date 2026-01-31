// OpsTrack_EntityState.c
// Data class for entity position state (used for replay)

class OpsTrack_EntityState
{
	UUID entityId;
	int timestamp;  // Unix timestamp in seconds
	float posX;
	float posY;
	float posZ;
	float rotation;
	bool isAlive;

	void OpsTrack_EntityState(UUID id, int ts, float x, float y, float z, float rot, bool alive)
	{
		entityId = id;
		timestamp = ts;
		posX = x;
		posY = y;
		posZ = z;
		rotation = rot;
		isAlive = alive;
	}

	string AsPayload()
	{
		string aliveStr = "false";
		if (isAlive)
			aliveStr = "true";

		return string.Format(
			"{" +
				"\"entityId\":\"%1\"," +
				"\"timestamp\":%2," +
				"\"posX\":%3," +
				"\"posY\":%4," +
				"\"posZ\":%5," +
				"\"rotation\":%6," +
				"\"isAlive\":%7" +
			"}",
			entityId,
			timestamp,
			posX,
			posY,
			posZ,
			rotation,
			aliveStr
		);
	}
}
