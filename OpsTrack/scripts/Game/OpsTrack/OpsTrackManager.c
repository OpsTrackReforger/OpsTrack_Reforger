class OpsTrackManager
{
    private static ref OpsTrackManager s_Instance;
    private ref OpsTrackSettings m_Settings;
    private const string SETTINGS_PATH = "$profile:OpsTrackSettings.json";
    private static bool s_Initialized = false;
	private static ref ApiClient m_ApiClient;
	
	//Event senders
	//ref ConnectionEventSender ConnectionEventsSender = new ConnectionEventSender();

	private void OpsTrackManager()
	{
		
	}


    static OpsTrackManager Get()
    {
		//Instantiation runs here instead of constructor to avoid multiple constructors running at the same time.
        if (!s_Instance) {
            s_Instance = new OpsTrackManager();

            if (!s_Initialized) {
                s_Instance.LoadOrCreate();
				m_ApiClient = new ApiClient();
                OpsTrackLogger.Info("OpsTrack Manager initialized on server.");
                s_Initialized = true;
            }
        }
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

    void Reload()
    {
        LoadOrCreate();
        OpsTrackLogger.Info("Settings reloaded at runtime.");
    }

    private void LoadOrCreate()
    {
        SCR_JsonLoadContext loadCtx = new SCR_JsonLoadContext();
        if (loadCtx.LoadFromFile(SETTINGS_PATH)) {
            m_Settings = new OpsTrackSettings();
            if (m_Settings.Load(loadCtx)) {
                OpsTrackLogger.Info("Settings loaded from " + SETTINGS_PATH);

                SavePretty();
                return;
            } else {
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
        if (!m_Settings) {
            OpsTrackLogger.Error("Tried to save settings but m_Settings is null!");
            return;
        }

        PrettyJsonSaveContainer pretty = new PrettyJsonSaveContainer();
        pretty.SetFormatOptions(EPrettyFormatOptions.FormatDefault);
        pretty.SetIndent(" ", 4);

        SCR_JsonSaveContext saveCtx = new SCR_JsonSaveContext(false);
        saveCtx.SetContainer(pretty);

        m_Settings.Save(saveCtx);

        if (pretty.SaveToFile(SETTINGS_PATH)) {
            OpsTrackLogger.Info("Settings saved to " + SETTINGS_PATH);
        } else {
            OpsTrackLogger.Error("Failed to save settings to " + SETTINGS_PATH);
        }
    }
}