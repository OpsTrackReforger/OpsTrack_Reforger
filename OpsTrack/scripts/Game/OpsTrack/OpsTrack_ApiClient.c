class ApiClient
{
    protected RestContext m_Context;
    protected ref OpsTrackCallback m_Callback;
    protected ref OpsTrackSettings settings;

    void ApiClient()
    {
        settings = OpsTrackManager.Get().GetSettings();
        RestApi restApi = GetGame().GetRestApi();
        m_Context = restApi.GetContext(settings.ApiBaseUrl);
		m_Context.SetHeaders("Content-Type,application/json,X-Api-Key," + settings.ApiKey);

        m_Callback = new OpsTrackCallback();
    }
	

    protected void Post(string endpoint, string json)
    {
        m_Context.POST(m_Callback, endpoint, json);
        OpsTrackLogger.Debug("POST request sent to endpoint '" + endpoint + "' with payload: " + json);
    }
}