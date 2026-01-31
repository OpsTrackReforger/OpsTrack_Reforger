// OpsTrackStartCommand.c
// RCON and chat command to start mission recording
// Usage: #opstrack_start "Mission Name"

class OpsTrackStartCommand : ScrServerCommand
{
	override string GetKeyword()
	{
		return "opstrack_start";
	}

	override bool IsServerSide()
	{
		return true;
	}

	override int RequiredRCONPermission()
	{
		return ERCONPermissions.PERMISSIONS_ADMIN;
	}

	override int RequiredChatPermission()
	{
		return EPlayerRole.ADMINISTRATOR;
	}

	override ref ScrServerCmdResult OnUpdate()
	{
		return new ScrServerCmdResult("No update required", EServerCmdResultType.OK);
	}

	override ref ScrServerCmdResult OnRCONExecution(array<string> argv)
	{
		return ExecuteStart(argv, -1);
	}

	override ref ScrServerCmdResult OnChatServerExecution(array<string> argv, int playerId)
	{
		if (!GetGame() || !GetGame().GetPlayerManager())
			return new ScrServerCmdResult("Game not ready.", EServerCmdResultType.ERR);

		if (!GetGame().GetPlayerManager().HasPlayerRole(playerId, EPlayerRole.ADMINISTRATOR))
		{
			OpsTrackLogger.Warn(string.Format("Player %1 attempted to start recording without admin permissions.", playerId));
			return new ScrServerCmdResult("You are not an administrator.", EServerCmdResultType.MISSING_PERMISSION);
		}

		return ExecuteStart(argv, playerId);
	}

	override ref ScrServerCmdResult OnChatClientExecution(array<string> argv, int playerId)
	{
		return new ScrServerCmdResult("", EServerCmdResultType.OK);
	}

	private ref ScrServerCmdResult ExecuteStart(array<string> argv, int playerId)
	{
		OpsTrackManager manager = OpsTrackManager.GetIfExists();
		if (!manager)
		{
			OpsTrackLogger.Error("Cannot start recording: OpsTrackManager not initialized.");
			return new ScrServerCmdResult("OpsTrack not initialized.", EServerCmdResultType.ERR);
		}

		if (manager.IsRecording())
		{
			return new ScrServerCmdResult("Already recording. Use #opstrack_stop first.", EServerCmdResultType.ERR);
		}

		// Get mission name from arguments (default to timestamp if not provided)
		string missionName = "";
		if (argv && argv.Count() > 1)
		{
			// Join all arguments after command as mission name
			for (int i = 1; i < argv.Count(); i++)
			{
				if (i > 1)
					missionName += " ";
				missionName += argv[i];
			}
			// Remove surrounding quotes if present
			missionName = missionName.Trim();
			if (missionName.Length() >= 2)
			{
				if (missionName[0] == "\"" && missionName[missionName.Length() - 1] == "\"")
					missionName = missionName.Substring(1, missionName.Length() - 2);
			}
		}

		if (missionName == "")
			missionName = "Mission_" + OpsTrack_DateTime.ToISO8601UTC();

		// Get current map name - reject if map is not supported
		string mapName = OpsTrack_MapNameHelper.GetCurrentMapName();
		if (mapName == "Unknown" || mapName.IsEmpty())
		{
			string worldFile = "";
			if (GetGame())
				worldFile = GetGame().GetWorldFile();

			OpsTrackLogger.Error(string.Format("Cannot start recording: Map not supported. WorldFile: %1", worldFile));
			return new ScrServerCmdResult("Map not supported. Add this map to OpsTrack_MapNames.conf", EServerCmdResultType.ERR);
		}

		manager.StartRecording(missionName, mapName);

		string msg = string.Format("Recording started: %1", missionName);
		if (playerId > 0)
			OpsTrackLogger.Info(string.Format("%1 (started by player %2)", msg, playerId));
		else
			OpsTrackLogger.Info(string.Format("%1 (started via RCON)", msg));

		return new ScrServerCmdResult(msg, EServerCmdResultType.OK);
	}
}
