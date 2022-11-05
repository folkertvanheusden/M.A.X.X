#include <map>
#include <string>
#include <vector>
#include <utility>

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

bool try_connect(const std::vector<std::pair<std::string, std::string> > & targets, const std::map<std::string, int> & scan_results, const int timeout, std::function<bool(int, int, std::string)> & progress_indicator)
{
	// scan through selected & scan-results and order by signal strength
	std::vector<std::tuple<std::string, std::string, int> > use;

	for(auto & target : targets) {
		auto scan_result = scan_results.find(target.first);

		if (scan_result != scan_results.end())
			use.push_back({ target.first, target.second, scan_result->second });
	}

	std::sort(use.begin(), use.end(), [](auto & a, auto & b) { return std::get<2>(b) < std::get<2>(a); });

	int  nr               = 0;

	bool connecting_state = 0;

        bool ok               = false;

	while(nr < timeout) {
		bool sleep = false;

		if (connecting_state == false) {
			if (connect_to_access_point(std::get<0>(use.at(nr)), std::get<1>(use.at(nr))))
				connecting_state = true;
			else
				nr++;
		}
		else {
			auto status = check_wifi_connection_status();

			if (status == CS_CONNECTED) {
				ok = true;

				break;
			}

			if (status == CS_FAILURE) {
				nr++;

				wifi_disconnect();

				connecting_state = false;
			}
			else {
				sleep = true; // no error, just waiting
			}
		}

		if (progress_indicator(nr, timeout, std::get<0>(use.at(nr))) == false)
			break;

		if (sleep)
			delay(100);
	}

	return ok;
}

bool wifi_disconnect()
{
	WiFi.disconnect();

	return true;
}
