// OpsTrackCallback.c
// REST API callback handler for async HTTP responses

class OpsTrackCallback : RestCallback
{
	protected ApiClient m_Client;

	void OpsTrackCallback(ApiClient client)
	{
		m_Client = client;
		
		// Register callback functions
		SetOnSuccess(OnSuccessHandler);
		SetOnError(OnErrorHandler);
	}
	
	// Called on successful request
	void OnSuccessHandler(RestCallback cb)
	{
		if (!cb)
		{
			OpsTrackLogger.Warn("OnSuccessHandler: callback is null");
			return;
		}
		
		int httpCode = cb.GetHttpCode();
		string data = cb.GetData();
		
		OpsTrackLogger.Info(string.Format("REST request succeeded. HTTP %1", httpCode));
		
		if (data && data != "")
			OpsTrackLogger.Debug(string.Format("REST response: %1", data));
	}
	
	// Called on error
	void OnErrorHandler(RestCallback cb)
	{
		if (!cb)
		{
			OpsTrackLogger.Error("OnErrorHandler: callback is null");
			TriggerBackoff();
			return;
		}

		int httpCode = cb.GetHttpCode();
		string data = cb.GetData();
		ERestResult restResult = cb.GetRestResult();

		OpsTrackLogger.Error(string.Format("REST request failed. HTTP %1, RestResult %2", httpCode, restResult));

		if (data && data != "")
		{
			OpsTrackLogger.Error(string.Format("REST error response: %1", data));
		}
		else
		{
			OpsTrackLogger.Error("REST error response: <empty>");
		}

		// Check for timeout
		if (restResult == ERestResult.EREST_ERROR_TIMEOUT)
		{
			OpsTrackLogger.Error("Request timed out");
		}

		// Only trigger backoff for server errors (5xx) or connection issues
		// Don't backoff for client errors like 400, 404 - these are data issues, not API issues
		if (httpCode >= 500 || httpCode == 0 || restResult == ERestResult.EREST_ERROR_TIMEOUT)
		{
			TriggerBackoff();
		}
		else
		{
			OpsTrackLogger.Warn(string.Format("HTTP %1 - not triggering backoff (client error)", httpCode));
		}
	}
	
	protected void TriggerBackoff()
	{
		if (m_Client)
			m_Client.Backoff();
		else
			OpsTrackLogger.Warn("Cannot trigger backoff: ApiClient reference is null");
	}
}
