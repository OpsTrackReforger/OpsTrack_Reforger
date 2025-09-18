class ApiClient
{
    protected RestContext m_Context;
	protected ref OpsTrackCallback m_Callback;

    void ApiClient(string baseUrl)
    {
        RestApi restApi = GetGame().GetRestApi();
        m_Context = restApi.GetContext(baseUrl);
        m_Context.SetHeaders("Content-Type,application/json");
		m_Callback = new OpsTrackCallback();
    }

    protected void Post(string endpoint, string json)
    {
        m_Context.POST(m_Callback, endpoint, json);
        Print("[OpsTrack] POST " + endpoint + " => " + json);
    }
}
