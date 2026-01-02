// OpsTrack_DateTime.c
// Utility for ISO8601 timestamp generation

class OpsTrack_DateTime
{
	static string ToISO8601UTC()
	{
		// "2025-12-22 21:08:09" -> "2025-12-22T21:08:09Z"
		string dt = SCR_DateTimeHelper.GetDateTimeUTC();
		
		if (!dt || dt == "")
			return "1970-01-01T00:00:00Z";

		// Replace space with T and append Z
		dt.Replace(" ", "T");
		dt = dt + "Z";

		return dt;
	}
}
