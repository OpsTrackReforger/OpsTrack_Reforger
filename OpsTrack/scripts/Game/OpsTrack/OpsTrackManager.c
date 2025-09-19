// OpsTrackManager.c
class OpsTrackManager
{
    private static ref OpsTrackManager s_Instance;
    private ref OpsTrackSettings m_Settings;
    private const string SETTINGS_PATH = "$profile:OpsTrackSettings.json";


    static OpsTrackManager Get()
    {
        if (!s_Instance) {
            s_Instance = new OpsTrackManager();
			Print("[OpsTrack] Manager initialized on server!");
            s_Instance.LoadOrCreate();
			
        }
        return s_Instance;
    }

    OpsTrackSettings GetSettings()
    {
        return m_Settings;
    }

    private void LoadOrCreate()
    {
        m_Settings = new OpsTrackSettings();

        SCR_JsonLoadContext loadCtx = new SCR_JsonLoadContext();
        if (loadCtx.LoadFromFile(SETTINGS_PATH)) {
            m_Settings.Load(loadCtx);
            Print("[OpsTrack] Settings loaded from " + SETTINGS_PATH);
            return;
        }

        SCR_JsonSaveContext saveCtx = new SCR_JsonSaveContext();
        m_Settings.Save(saveCtx);
        if (saveCtx.SaveToFile(SETTINGS_PATH)) {
            Print("[OpsTrack] No settings file found. Created default at " + SETTINGS_PATH);
        } else {
            Print("[OpsTrack] Failed to save default settings!");
        }
    }
}
