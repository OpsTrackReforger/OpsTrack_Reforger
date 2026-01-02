// OpsTrackReloadCommand.c
// RCON and chat command to reload OpsTrack settings at runtime

class OpsTrackReloadCommand : ScrServerCommand
{
	override string GetKeyword()
	{
		return "opstrack_reload";
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
		OpsTrackManager manager = OpsTrackManager.GetIfExists();
		if (!manager)
		{
			OpsTrackLogger.Error("Cannot reload: OpsTrackManager not initialized.");
			return new ScrServerCmdResult("OpsTrack not initialized.", EServerCmdResultType.ERR);
		}
		
		manager.Reload();
		
		// Refresh singletons with new settings
		RefreshSingletons();
		
		OpsTrackLogger.Info("Settings reloaded via RCON command.");
		return new ScrServerCmdResult("OpsTrack settings reloaded via RCON.", EServerCmdResultType.OK);
	}

	override ref ScrServerCmdResult OnChatServerExecution(array<string> argv, int playerId)
	{
		// Verify admin permissions
		if (!GetGame() || !GetGame().GetPlayerManager())
		{
			return new ScrServerCmdResult("Game not ready.", EServerCmdResultType.ERR);
		}
		
		if (!GetGame().GetPlayerManager().HasPlayerRole(playerId, EPlayerRole.ADMINISTRATOR))
		{
			OpsTrackLogger.Warn(string.Format("Player %1 attempted to reload settings without admin permissions.", playerId));
			return new ScrServerCmdResult("You are not an administrator.", EServerCmdResultType.MISSING_PERMISSION);
		}

		OpsTrackManager manager = OpsTrackManager.GetIfExists();
		if (!manager)
		{
			OpsTrackLogger.Error("Cannot reload: OpsTrackManager not initialized.");
			return new ScrServerCmdResult("OpsTrack not initialized.", EServerCmdResultType.ERR);
		}
		
		manager.Reload();
		
		// Refresh singletons with new settings
		RefreshSingletons();
		
		OpsTrackLogger.Info(string.Format("Settings reloaded via in-game chat command by player %1.", playerId));
		return new ScrServerCmdResult("OpsTrack settings reloaded.", EServerCmdResultType.OK);
	}

	override ref ScrServerCmdResult OnChatClientExecution(array<string> argv, int playerId)
	{
		return new ScrServerCmdResult("", EServerCmdResultType.OK);
	}
	
	// Helper to refresh singleton settings after reload
	private void RefreshSingletons()
	{
		ConnectionEventSender connSender = ConnectionEventSender.Get();
		if (connSender)
			connSender.RefreshSettings();
		
		CombatEventSender combatSender = CombatEventSender.Get();
		if (combatSender)
			combatSender.RefreshSettings();
	}
}
