// OpsTrack_MapNameConfig.c
// Configuration for mapping world file identifiers to readable map names

//------------------------------------------------------------------------------------------------
// Individual map name entry
[BaseContainerProps(), SCR_BaseContainerCustomTitleField("m_sWorldIdentifier")]
class OpsTrack_MapNameEntry
{
	[Attribute("", UIWidgets.EditBox, "World file identifier (partial match against GetGame().GetWorldFile(), e.g. 'Everon', 'Arland')")]
	string m_sWorldIdentifier;

	[Attribute("", UIWidgets.EditBox, "Display name for the map")]
	string m_sMapName;
}

//------------------------------------------------------------------------------------------------
// Master configuration for map names
[BaseContainerProps(configRoot: true)]
class OpsTrack_MapNameConfig
{
	[Attribute("", UIWidgets.Object, "Map name entries")]
	ref array<ref OpsTrack_MapNameEntry> m_aMapEntries;

	//------------------------------------------------------------------------------------------------
	// Find map name for current world
	// @param worldFile - The world file path from GetGame().GetWorldFile()
	// @return Display name of the map, or "Unknown" if not found
	string GetMapName(string worldFile)
	{
		if (!m_aMapEntries || worldFile.IsEmpty())
			return "Unknown";

		foreach (OpsTrack_MapNameEntry entry : m_aMapEntries)
		{
			if (!entry || entry.m_sWorldIdentifier.IsEmpty())
				continue;

			// Partial match - allows "Everon" to match "{GUID}Worlds/MP/CTI_Everon.ent"
			if (worldFile.Contains(entry.m_sWorldIdentifier))
				return entry.m_sMapName;
		}

		return "Unknown";
	}
}
