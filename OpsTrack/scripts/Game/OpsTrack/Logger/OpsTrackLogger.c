class OpsTrackLogger
{
    private static const string LOG_DIR = "$profile:OpsTrackLogs";

    // Dynamisk log path baseret på dato
    private static string GetLogPath()
    {
        // Sørg for at mappen findes
        FileIO.MakeDirectory(LOG_DIR);

        string datetime = SCR_DateTimeHelper.GetDateTimeUTC(); // "yyyy-mm-dd hh:mm:ss"
        string dateOnly = datetime.Substring(0, 10);           // "yyyy-mm-dd"
        return LOG_DIR + "/OpsTrack_" + dateOnly + ".log";
    }

    static void Log(OpsTrackLogLevel level, string msg)
    {
        OpsTrackSettings settings = OpsTrackManager.Get().GetSettings();
        bool debugEnabled = settings && settings.EnableDebug;

        string timestamp = SCR_DateTimeHelper.GetDateTimeUTC();
        string levelStr = LevelToString(level);
        string line = string.Format("[OpsTrack][%1][%2] %3", levelStr, timestamp, msg);

        // --- Console output ---
        if (level == OpsTrackLogLevel.WARN || level == OpsTrackLogLevel.ERROR)
        {
            Print(line); // altid
        }
        else if (debugEnabled)
        {
            Print(line); // kun i debug mode
        }

        // --- Skip INFO/DEBUG i normal mode ---
        if (!debugEnabled && (level == OpsTrackLogLevel.INFO || level == OpsTrackLogLevel.DEBUG))
            return;

        // --- File output ---
        string logPath = GetLogPath();
        FileHandle fh = FileIO.OpenFile(logPath, FileMode.APPEND);
        if (!fh)
        {
            fh = FileIO.OpenFile(logPath, FileMode.WRITE);
        }
        if (fh)
        {
            fh.WriteLine(line);
        }
        else
        {
            Print("[OpsTrack][ERROR] Could not open log file: " + logPath);
        }
    }

    // --- Convenience wrappers ---
    static void Debug(string msg) { Log(OpsTrackLogLevel.DEBUG, msg); }
    static void Info(string msg)  { Log(OpsTrackLogLevel.INFO, msg); }
    static void Warn(string msg)  { Log(OpsTrackLogLevel.WARN, msg); }
    static void Error(string msg) { Log(OpsTrackLogLevel.ERROR, msg); }

    private static string LevelToString(OpsTrackLogLevel level)
    {
        switch (level)
        {
            case OpsTrackLogLevel.DEBUG: return "DEBUG";
            case OpsTrackLogLevel.INFO:  return "INFO";
            case OpsTrackLogLevel.WARN:  return "WARN";
            case OpsTrackLogLevel.ERROR: return "ERROR";
        }
        return "UNKNOWN";
    }
}