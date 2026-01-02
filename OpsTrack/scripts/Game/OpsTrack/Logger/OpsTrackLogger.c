// OpsTrackLogger.c
// Logging utility with file and console output
// FIXED: FileHandle is now properly closed after writing

class OpsTrackLogger
{
	private static const string LOG_DIR = "$profile:OpsTrackLogs";

	// Dynamic log path based on date
	private static string GetLogPath()
	{
		// Ensure directory exists
		FileIO.MakeDirectory(LOG_DIR);

		string datetime = SCR_DateTimeHelper.GetDateTimeUTC(); // "yyyy-mm-dd hh:mm:ss"
		if (!datetime || datetime.Length() < 10)
			return LOG_DIR + "/OpsTrack_unknown.log";
		
		string dateOnly = datetime.Substring(0, 10);           // "yyyy-mm-dd"
		return LOG_DIR + "/OpsTrack_" + dateOnly + ".log";
	}

	static void Log(OpsTrackLogLevel level, string msg)
	{
		// Safe settings access - don't crash if manager not ready
		OpsTrackSettings settings;
		OpsTrackManager manager = OpsTrackManager.GetIfExists();
		if (manager)
			settings = manager.GetSettings();
		
		bool debugEnabled = settings && settings.EnableDebug;

		string timestamp = SCR_DateTimeHelper.GetDateTimeUTC();
		string levelStr = LevelToString(level);
		string line = string.Format("[OpsTrack][%1][%2] %3", levelStr, timestamp, msg);

		// --- Console output ---
		if (level == OpsTrackLogLevel.WARN || level == OpsTrackLogLevel.ERROR)
		{
			Print(line); // Always print warnings and errors
		}
		else if (debugEnabled)
		{
			Print(line); // Only in debug mode
		}

		// --- Skip INFO/DEBUG file output in normal mode ---
		if (!debugEnabled && (level == OpsTrackLogLevel.INFO || level == OpsTrackLogLevel.DEBUG))
			return;

		// --- File output with proper cleanup ---
		WriteToFile(line);
	}
	
	// Separate method for file writing with guaranteed cleanup
	private static void WriteToFile(string line)
	{
		string logPath = GetLogPath();
		
		FileHandle fh = FileIO.OpenFile(logPath, FileMode.APPEND);
		if (!fh)
		{
			// Try creating new file if append fails
			fh = FileIO.OpenFile(logPath, FileMode.WRITE);
		}
		
		if (fh)
		{
			fh.WriteLine(line);
			fh.Close(); // FIX: Always close the file handle!
		}
		else
		{
			// Fallback to console only - avoid recursive logging
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
