//OpsTrack_BaseGameMode
modded class SCR_BaseGameMode
{	
//	ref OpsTrackCallback m_JoinCallBack;
//	ref OpsTrackCallback m_LeaveCallback;
	ref ConnectionEventSender m_ConnectionEvents;
	
	
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner); //Run code from super class
		
		if(Replication.IsServer()) //Using IsServer because we only want it to happen when it is a server.
		{
			OpsTrackSettings settings = OpsTrackManager.Get().GetSettings();
			
			m_ConnectionEvents = new ConnectionEventSender();
			Print("[OpsTrack] Mod is running on the server!");
		}
	}
	
	override void OnPlayerRegistered(int playerId)
	{
		super.OnPlayerRegistered(playerId);
		
		if(m_ConnectionEvents)
		{
			m_ConnectionEvents.SendJoin(playerId);
		}
	}
	
	override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
	
		
		if(m_ConnectionEvents)
		{
			m_ConnectionEvents.SendLeave(playerId);
		}
		
		super.OnPlayerDisconnected(playerId, cause, timeout); //Runn as last thing, so it doesnt dispose the data above methods need.
	}
	
//	
//    void SendJoinEvent(int playerId, int attempt = 0)
//    {
//
//		BackendApi backendApi = GetGame().GetBackendApi();
//		RestApi restApi = GetGame().GetRestApi();
//		
//        string uniqueId = backendApi.GetPlayerIdentityId(playerId);
//		string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
//
//		if(uniqueId == "" && attempt < 20)
//		{
//			GetGame().GetCallqueue().CallLater(SendJoinEvent, 100, false, playerId, attempt + 1);
//			return;
//		}
//		
//        	RestContext ctx = restApi.GetContext("http://91.226.144.240:5050");
//        	ctx.SetHeaders("Content-Type,application/json");
//		
//			string json = string.Format("{\"gameIdentity\":\"%1\",\"name\":\"%2\"}", uniqueId, playerName);
//		
//			m_JoinCallBack = new OpsTrackCallback();
//        	ctx.POST(m_JoinCallBack, "/player/join", json);
//		
//			Print(json);
//        	Print(string.Format("[OpsTrack] Player connected: %1 (%2)", uniqueId, playerName));
//    }

//	override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
//    {
//
//		BackendApi backendApi = GetGame().GetBackendApi();
//		RestApi restApi = GetGame().GetRestApi();
//	
//        string uniqueId = backendApi.GetPlayerIdentityId(playerId);
//		string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
//		
//		//IMPORTANT
//        super.OnPlayerDisconnected(playerId, cause, timeout); //Needs to be call after storing the data we need for the post, else it gets disposed 
//		//IMPORTANT
//		
//        RestContext ctx = restApi.GetContext("http://91.226.144.240:5050");
//        ctx.SetHeaders("Content-Type,application/json");
//
//        string json = string.Format("{\"gameIdentity\":\"%1\",\"name\":\"%2\"}", uniqueId, playerName);
//		
//		
//		m_LeaveCallback = new OpsTrackCallback();
//        ctx.POST(m_LeaveCallback, "/player/leave", json);
//		Print(json);
//        Print(string.Format("[OpsTrack] Player disconnected: %1 (%2)", uniqueId, playerName));
//		
//    }


}