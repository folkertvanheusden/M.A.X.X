#include <map>
#include <string>

#include <ESP8266WiFi.h>

#include "wifi.h"


bool set_hostname(const std::string & hostname)
{
	WiFi.hostname(hostname.c_str());

	return false;
}

// returns a list of SSIDs + signal strengths
std::map<std::string, int> scan_access_points()
{
	std::map<std::string, int> out;

	int n_networks = WiFi.scanNetworks();

	for(int i=0; i<n_networks; i++)
		out.insert({ WiFi.SSID(i).c_str(), WiFi.RSSI(i) });

	return out;
}

bool connect_to_access_point(const std::string ssid, const std::string password)
{
	auto status = WiFi.begin(ssid.c_str(), password.c_str());

	// also WL_DISCONNECTED?
	return status != WL_NO_SHIELD && status != WL_NO_SSID_AVAIL && status != WL_CONNECT_FAILED && status != WL_CONNECTION_LOST;
}

connect_status_t check_wifi_connection_status()
{
	auto status = WiFi.status();

	if (status == WL_CONNECTED)
		return CS_CONNECTED;

	if (status == WL_IDLE_STATUS || status == WL_DISCONNECTED)
		return CS_IDLE;

	return CS_FAILURE;
}

std::optional<std::string> select_best_access_point(const std::map<std::string, int> & list)
{
	std::string best_ssid;
	int         best_ss   = -32767;

	for(auto & entry : list) {
		if (entry.second > best_ss) {
			best_ssid = entry.first;
			best_ss   = entry.second;
		}
	}

	if (best_ssid.empty())
		return { };

	return best_ssid;
}

// timeout in number of 100ms intervals
// callback: interval_nr, max_nr_set, can return false to abort
bool try_connect(const std::string ssid, const std::string password, const int timeout, std::function<bool(int, int)> & progress_indicator)
{
	if (!connect_to_access_point(ssid, password))
		return false;

	int nr = 0;

	for(;;) {
		auto status = check_wifi_connection_status();

		if (status == CS_CONNECTED)
			return true;

		if (status == CS_FAILURE) {
			if (!connect_to_access_point(ssid, password))
				break;

			nr = 0;
		}

		if (progress_indicator(nr, timeout) == false)
			break;

		nr++;

		delay(100);
	}

	WiFi.disconnect();

	return false;
}

bool wifi_disconnect()
{
	WiFi.disconnect();

	return true;
}
