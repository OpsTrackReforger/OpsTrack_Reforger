// OpsTrackManager.c
// Central manager for OpsTrack system initialization and configuration

class OpsTrackManager
{
	private static ref OpsTrackManager s_Instance;
	private ref OpsTrackSettings m_Settings;
	private static ref ApiClient m_ApiClient;
	private static ref OpsTrack_EntityManager m_EntityManager;
	
	private const string SETTINGS_PATH = "$profile:OpsTrackSettings.json";
	
	// Recording state
	private bool m_IsRecording;
	private UUID m_CurrentMissionId;
	private string m_CurrentMissionName;

	private void OpsTrackManager()
	{
		// Constructor kept private - use Get() for singleton access
		m_IsRecording = false;
		m_CurrentMissionId = UUID.NULL_UUID;
		m_CurrentMissionName = "";
	}

	// Main accessor - creates instance if needed
	// NOTE: Static references can persist across Workbench reloads but become invalid.
	// We must check and recreate them each time.
	static OpsTrackManager Get()
	{
		// Always create fresh instance on first call after reload
		// The static ref may exist but point to garbage after Workbench reload
		if (!s_Instance || !s_Instance.m_Settings)
		{
			s_Instance = new OpsTrackManager();
			s_Instance.LoadOrCreate();

			// Also clear stale static refs
			m_ApiClient = null;
			m_EntityManager = null;

			OpsTrackLogger.Info("OpsTrack Manager initialized on server.");
		}

		// Ensure ApiClient exists
		if (!m_ApiClient)
		{
			OpsTrackLogger.Info("Creating ApiClient...");
			m_ApiClient = new ApiClient();

			if (!m_ApiClient)
				OpsTrackLogger.Error("Failed to create ApiClient!");
		}

		// Ensure EntityManager exists
		if (!m_EntityManager)
		{
			m_EntityManager = OpsTrack_EntityManager.Get();
		}

		return s_Instance;
	}
	
	// Safe accessor - returns null if not yet initialized
	// Used by logger to avoid circular dependency during startup
	static OpsTrackManager GetIfExists()
	{
		return s_Instance;
	}

	OpsTrackSettings GetSettings()
	{
		return m_Settings;
	}
	
	ApiClient GetApiClient()
	{
		return m_ApiClient;
	}
	
	OpsTrack_EntityManager GetEntityManager()
	{
		return m_EntityManager;
	}
	
	// --- Recording State ---
	
	bool IsRecording()
	{
		return m_IsRecording;
	}
	
	UUID GetCurrentMissionId()
	{
		return m_CurrentMissionId;
	}
	
	string GetCurrentMissionName()
	{
		return m_CurrentMissionName;
	}
	
	// Start recording - called from command
	void StartRecording(string missionName, string mapName)
	{
		if (m_IsRecording)
		{
			OpsTrackLogger.Warn("Already recording. Stop current recording first.");
			return;
		}

		// Generate mission ID
		m_CurrentMissionId = UUID.GenV4();
		m_CurrentMissionName = missionName;
		m_IsRecording = true;

		// Send mission to API
		string payload = string.Format(
			"{\"missionId\":\"%1\",\"name\":\"%2\",\"mapName\":\"%3\"}",
			m_CurrentMissionId,
			missionName,
			mapName
		);

		if (m_ApiClient)
			m_ApiClient.SendMissionStart(payload);

		// Delay entity assignment to allow mission creation to complete in API
		// This avoids race condition where assign-mission is called before mission exists
		if (GetGame() && GetGame().GetCallqueue())
			GetGame().GetCallqueue().CallLater(AssignExistingEntitiesToMission, 1000, false);

		// Start position tracking
		OpsTrack_StateTracker stateTracker = OpsTrack_StateTracker.Get();
		if (stateTracker)
			stateTracker.StartTracking();

		OpsTrackLogger.Info(string.Format("Recording started: %1 (ID: %2)", missionName, m_CurrentMissionId));
	}

	// Assign all cached entities to the current mission
	protected void AssignExistingEntitiesToMission()
	{
		// Safety check: ensure still recording (in case StopRecording was called during delay)
		if (!m_IsRecording || m_CurrentMissionId.IsNull())
			return;

		if (!m_EntityManager || !m_ApiClient)
			return;

		array<UUID> entityIds = m_EntityManager.GetAllEntityIds();
		if (entityIds.Count() == 0)
			return;

		// Queue each entity assignment (ApiClient handles batching)
		foreach (UUID entityId : entityIds)
		{
			m_ApiClient.EnqueueEntityAssignment(string.Format("%1", entityId));
		}

		OpsTrackLogger.Info(string.Format("Queued %1 existing entities for mission assignment", entityIds.Count()));
	}

	// Stop recording - called from command
	void StopRecording()
	{
		if (!m_IsRecording)
		{
			OpsTrackLogger.Warn("Not currently recording.");
			return;
		}

		// Stop position tracking first (flushes remaining states)
		OpsTrack_StateTracker stateTracker = OpsTrack_StateTracker.Get();
		if (stateTracker)
			stateTracker.StopTracking();

		// Send end mission to API
		if (m_ApiClient)
			m_ApiClient.SendMissionEnd(m_CurrentMissionId);

		OpsTrackLogger.Info(string.Format("Recording stopped: %1", m_CurrentMissionName));

		m_IsRecording = false;
		m_CurrentMissionId = UUID.NULL_UUID;
		m_CurrentMissionName = "";

		// Clear entity cache (new session on next recording)
		if (m_EntityManager)
			m_EntityManager.ClearCache();
	}

	// --- Settings ---

	void Reload()
	{
		LoadOrCreate();
		OpsTrackLogger.Info("Settings reloaded at runtime.");
	}

	private void LoadOrCreate()
	{
		SCR_JsonLoadContext loadCtx = new SCR_JsonLoadContext();
		
		if (loadCtx.LoadFromFile(SETTINGS_PATH))
		{
			m_Settings = new OpsTrackSettings();
			
			if (m_Settings.Load(loadCtx))
			{
				OpsTrackLogger.Info("Settings loaded from " + SETTINGS_PATH);
				
				// Validate loaded settings
				if (!m_Settings.IsValid())
				{
					OpsTrackLogger.Warn("Loaded settings have validation warnings, check configuration.");
				}
				
				SavePretty();
				return;
			}
			else
			{
				OpsTrackLogger.Warn("Failed to parse settings file. Falling back to defaults.");
			}
		}

		// --- Fallback to defaults ---
		m_Settings = new OpsTrackSettings();
		SavePretty();
		OpsTrackLogger.Warn("No settings file found. Created default settings at " + SETTINGS_PATH);
	}

	private void SavePretty()
	{
		if (!m_Settings)
		{
			OpsTrackLogger.Error("Tried to save settings but m_Settings is null!");
			return;
		}
		
		PrettyJsonSaveContainer pretty = new PrettyJsonSaveContainer();
		pretty.SetFormatOptions(EPrettyFormatOptions.FormatDefault);
		pretty.SetIndent(" ", 4);

		SCR_JsonSaveContext saveCtx = new SCR_JsonSaveContext(false);
		saveCtx.SetContainer(pretty);

		m_Settings.Save(saveCtx);

		if (pretty.SaveToFile(SETTINGS_PATH))
		{
			OpsTrackLogger.Info("Settings saved to " + SETTINGS_PATH);
		}
		else
		{
			OpsTrackLogger.Error("Failed to save settings to " + SETTINGS_PATH);
		}
	}
}
