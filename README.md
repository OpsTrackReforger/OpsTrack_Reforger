**Support**
Join the discord: https://discord.gg/w9Gf3twcUn
Or create an issue

**Description**
Opstrack_Reforger is the ArmA reforger mod that pushes data to the OpsTrack_API: https://github.com/OpsTrackReforger/OpsTrack_API

**Getting started**
- Find Opstrack in reforger Workshop and add the mod id to your servers mod list.
- Install and run OpsTrack_API: https://github.com/OpsTrackReforger/OpsTrack_API (See install guide on the repo)
- In reforger server profiles/OpsTrackSettings.json set the following:
  - ApiBaseUrl - example "ApiBaseUrl": "http://192.168.1.10:5050",
  - ApiKey - The same key you set in your environment variable on startup of the api
  - enable events - Enable the events you would like to send to the api.
  - MaxRetries - How many times should the mod attempt to send a request to the api before terminating the request?


