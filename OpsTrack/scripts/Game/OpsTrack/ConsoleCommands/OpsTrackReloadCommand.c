// OpsTrackReloadCommand.c
class OpsTrackReloadCommand : ScrServerCommand
{
    // Navnet på kommandoen (bruges med #opstrack_reload i RCON/console/chat)
    override string GetKeyword()
    {
        return "opstrack_reload";
    }

    // Skal køre på serveren
    override bool IsServerSide()
    {
        return true;
    }

    // Krævet RCON‑permission (tilpas hvis din enum hedder anderledes)
    override int RequiredRCONPermission()
    {
        return ERCONPermissions.PERMISSIONS_ADMIN;
    }

    // Krævet spillerrolle for at køre via chat
    override int RequiredChatPermission()
    {
        return EPlayerRole.ADMINISTRATOR;
    }

    // Krævet af API’et – bruges hvis en kommando returnerer PENDING
    override ref ScrServerCmdResult OnUpdate()
    {
        return new ScrServerCmdResult("No update required", EServerCmdResultType.OK);
    }

    // Eksekvering via RCON/console
    override ref ScrServerCmdResult OnRCONExecution(array<string> argv)
    {
        OpsTrackManager.Get().Reload();
        return new ScrServerCmdResult("[OpsTrack] Settings reloaded via RCON", EServerCmdResultType.OK);
    }

    // Eksekvering via chat (på serveren)
    override ref ScrServerCmdResult OnChatServerExecution(array<string> argv, int playerId)
    {
        if (!GetGame().GetPlayerManager().HasPlayerRole(playerId, EPlayerRole.ADMINISTRATOR))
            return new ScrServerCmdResult("You are not an admin!", EServerCmdResultType.MISSING_PERMISSION);

        OpsTrackManager.Get().Reload();
        return new ScrServerCmdResult("[OpsTrack] Settings reloaded!", EServerCmdResultType.OK);
    }

    // Eksekvering på klienten (før serveren kaldes) – kan være tom
    override ref ScrServerCmdResult OnChatClientExecution(array<string> argv, int playerId)
    {
        return new ScrServerCmdResult("", EServerCmdResultType.OK);
    }
}