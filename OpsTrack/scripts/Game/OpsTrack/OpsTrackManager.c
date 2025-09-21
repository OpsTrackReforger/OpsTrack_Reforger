class OpsTrackManager
{
    private static ref OpsTrackManager s_Instance;
    private ref OpsTrackSettings m_Settings;
    private const string SETTINGS_PATH = "$profile:OpsTrackSettings.json";

    private void OpsTrackManager()
    {
        LoadOrCreate();
    }

    static OpsTrackManager Get()
    {
        if (!s_Instance) {
            s_Instance = new OpsTrackManager();
            Print("[OpsTrack] Manager initialized on server!");
        }
        return s_Instance;
    }

    OpsTrackSettings GetSettings()
    {
        return m_Settings;
    }

    void Reload()
    {
        LoadOrCreate();
        Print("[OpsTrack] Settings reloaded at runtime");
    }

    private void LoadOrCreate()
    {
        SCR_JsonLoadContext loadCtx = new SCR_JsonLoadContext();
        if (loadCtx.LoadFromFile(SETTINGS_PATH)) {
            m_Settings = new OpsTrackSettings();
            if (m_Settings.Load(loadCtx)) {
                Print("[OpsTrack] Settings loaded from " + SETTINGS_PATH);
                return;
            } else {
                Print("[OpsTrack] Failed to parse settings file, falling back to defaults");
            }
        }

        // --- Fallback til defaults ---
        m_Settings = new OpsTrackSettings();

        PrettyJsonSaveContainer pretty = new PrettyJsonSaveContainer();
        pretty.SetFormatOptions(EPrettyFormatOptions.FormatDefault);
        pretty.SetIndent(" ", 4);

        SCR_JsonSaveContext saveCtx = new SCR_JsonSaveContext(false);
        saveCtx.SetContainer(pretty);

        m_Settings.Save(saveCtx);

        if (pretty.SaveToFile(SETTINGS_PATH)) {
            Print("[OpsTrack] No settings file found. Created default (pretty) at " + SETTINGS_PATH);
        } else {
            Print("[OpsTrack] Failed to save default settings!");
        }
    }
}