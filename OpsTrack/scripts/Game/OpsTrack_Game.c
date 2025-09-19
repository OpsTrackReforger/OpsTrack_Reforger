//modded class ArmaReforgerScripted
//{
//	override bool OnGameStart()
//	{
//		bool result = super.OnGameStart(); //Run base OnGameStart
//		
//		//Initialise Opstrack settings
//		if(RplSession.Mode() == RplMode.Dedicated) //Runs only on a server instance.
//		{
//			OpsTrackManager.Init();
//			Print("[OpsTrack] Manager initialized on server!");
//		}
//				
//		
//		//pass through the result from super.
//		return result;
//	}
//}