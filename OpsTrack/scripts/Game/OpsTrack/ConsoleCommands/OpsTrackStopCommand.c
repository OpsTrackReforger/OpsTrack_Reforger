// OpsTrackStopCommand.c
// RCON and chat command to stop mission recording
// Usage: #opstrack_stop

class OpsTrackStopCommand : ScrServerCommand
{
	override string GetKeyword()
	{
		return "opstrack_stop";
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
		return ExecuteStop(-1);
	}

	override ref ScrServerCmdResult OnChatServerExecution(array<string> argv, int playerId)
	{
		if (!GetGame() || !GetGame().GetPlayerManager())
			return new ScrServerCmdResult("Game not ready.", EServerCmdResultType.ERR);

		if (!GetGame().GetPlayerManager().HasPlayerRole(playerId, EPlayerRole.ADMINISTRATOR))
		{
			OpsTrackLogger.Warn(string.Format("Player %1 attempted to stop recording without admin permissions.", playerId));
			return new ScrServerCmdResult("You are not an administrator.", EServerCmdResultType.MISSING_PERMISSION);
		}

		return ExecuteStop(playerId);
	}

	override ref ScrServerCmdResult OnChatClientExecution(array<string> argv, int playerId)
	{
		return new ScrServerCmdResult("", EServerCmdResultType.OK);
	}

	private ref ScrServerCmdResult ExecuteStop(int playerId)
	{
		OpsTrackManager manager = OpsTrackManager.GetIfExists();
		if (!manager)
		{
			OpsTrackLogger.Error("Cannot stop recording: OpsTrackManager not initialized.");
			return new ScrServerCmdResult("OpsTrack not initialized.", EServerCmdResultType.ERR);
		}

		if (!manager.IsRecording())
		{
			return new ScrServerCmdResult("Not currently recording.", EServerCmdResultType.ERR);
		}

		string missionName = manager.GetCurrentMissionName();
		manager.StopRecording();

		string msg = string.Format("Recording stopped: %1", missionName);
		if (playerId > 0)
			OpsTrackLogger.Info(string.Format("%1 (stopped by player %2)", msg, playerId));
		else
			OpsTrackLogger.Info(string.Format("%1 (stopped via RCON)", msg));

		return new ScrServerCmdResult(msg, EServerCmdResultType.OK);
	}
}
