// OpsTrack_MapNameHelper.c
// Helper class to load and access map name configuration

class OpsTrack_MapNameHelper
{
	// Path to the config file - update this after creating the .conf file
	protected static const ResourceName CONFIG_PATH = "{3B879C69FDB29B47}scripts/Game/OpsTrack/Config/OpsTrack_MapNames.conf";

	protected static ref OpsTrack_MapNameConfig s_Config;
	protected static bool s_bLoadAttempted = false;

	//------------------------------------------------------------------------------------------------
	// Get the current map's display name (config only - no fallback)
	// @return Map name string (e.g. "Everon", "Arland") or "Unknown" if not in config
	static string GetCurrentMapName()
	{
		if (!GetGame())
			return "Unknown";

		string worldFile = GetGame().GetWorldFile();
		if (worldFile.IsEmpty())
			return "Unknown";

		// Only use config lookup - no fallback
		OpsTrack_MapNameConfig config = GetConfig();
		if (config)
		{
			return config.GetMapName(worldFile);
		}

		return "Unknown";
	}

	//------------------------------------------------------------------------------------------------
	// Get the current map's display name with fallback to filename extraction
	// @return Map name string - never returns "Unknown" unless game not ready
	static string GetCurrentMapNameWithFallback()
	{
		if (!GetGame())
			return "Unknown";

		string worldFile = GetGame().GetWorldFile();
		if (worldFile.IsEmpty())
			return "Unknown";

		// Try config lookup first
		OpsTrack_MapNameConfig config = GetConfig();
		if (config)
		{
			string mapName = config.GetMapName(worldFile);
			if (mapName != "Unknown")
				return mapName;
		}

		// Fallback: extract from world file path
		return ExtractMapNameFromPath(worldFile);
	}

	//------------------------------------------------------------------------------------------------
	// Extract map name from world file path as fallback
	// @param worldFile - Full world file path
	// @return Extracted name or "Unknown"
	protected static string ExtractMapNameFromPath(string worldFile)
	{
		if (worldFile.IsEmpty())
			return "Unknown";

		// worldFile is something like "{59AD59FE94E4E411}Worlds/MP/CTI_Everon.ent"
		int lastSlash = worldFile.LastIndexOf("/");
		if (lastSlash < 0)
			lastSlash = worldFile.LastIndexOf("\\");

		string fileName;
		if (lastSlash >= 0)
			fileName = worldFile.Substring(lastSlash + 1, worldFile.Length() - lastSlash - 1);
		else
			fileName = worldFile;

		// Remove .ent extension
		int dotPos = fileName.LastIndexOf(".");
		if (dotPos >= 0)
			fileName = fileName.Substring(0, dotPos);

		return fileName;
	}

	//------------------------------------------------------------------------------------------------
	// Load and cache the config
	protected static OpsTrack_MapNameConfig GetConfig()
	{
		if (s_Config)
			return s_Config;

		if (s_bLoadAttempted)
			return null;

		s_bLoadAttempted = true;

		Resource resource = Resource.Load(CONFIG_PATH);
		if (!resource || !resource.IsValid())
		{
			OpsTrackLogger.Warn("OpsTrack_MapNameHelper: Could not load map name config from: " + CONFIG_PATH);
			return null;
		}

		BaseContainer container = resource.GetResource().ToBaseContainer();
		if (!container)
		{
			OpsTrackLogger.Warn("OpsTrack_MapNameHelper: Invalid config container");
			return null;
		}

		s_Config = OpsTrack_MapNameConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(container));
		if (!s_Config)
		{
			OpsTrackLogger.Warn("OpsTrack_MapNameHelper: Failed to create config instance");
			return null;
		}

		OpsTrackLogger.Info("OpsTrack_MapNameHelper: Loaded map name config successfully");
		return s_Config;
	}
}
