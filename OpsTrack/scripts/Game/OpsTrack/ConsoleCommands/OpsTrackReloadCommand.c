// OpsTrackReloadCommand.c
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
        OpsTrackManager.Get().Reload();
        OpsTrackLogger.Info("Settings reloaded via RCON command.");
        return new ScrServerCmdResult("OpsTrack settings reloaded via RCON.", EServerCmdResultType.OK);
    }

    override ref ScrServerCmdResult OnChatServerExecution(array<string> argv, int playerId)
    {
        if (!GetGame().GetPlayerManager().HasPlayerRole(playerId, EPlayerRole.ADMINISTRATOR)) {
            OpsTrackLogger.Warn("Player " + playerId + " attempted to reload settings without admin permissions.");
            return new ScrServerCmdResult("You are not an administrator.", EServerCmdResultType.MISSING_PERMISSION);
        }

        OpsTrackManager.Get().Reload();
        OpsTrackLogger.Info("Settings reloaded via in-game chat command by player " + playerId + ".");
        return new ScrServerCmdResult("OpsTrack settings reloaded.", EServerCmdResultType.OK);
    }

    override ref ScrServerCmdResult OnChatClientExecution(array<string> argv, int playerId)
    {
        return new ScrServerCmdResult("", EServerCmdResultType.OK);
    }
}