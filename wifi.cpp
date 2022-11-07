#include <map>
#include <string>
#include <vector>
#include <utility>

#include "wifi.h"


bool debug = false;

bool set_hostname(const std::string & hostname)
{
	WiFi.hostname(hostname.c_str());

	return true;
}

void start_wifi(const std::optional<std::string> & listen_ssid)
{
	WiFi.persistent(false);

	if (listen_ssid.has_value()) {
		WiFi.mode(WIFI_AP_STA);

		WiFi.softAP(listen_ssid.value().c_str());
	}
	else {
		WiFi.mode(WIFI_STA);
	}
}

void enable_wifi_debug()
{
	debug = true;
}

void scan_access_points_start()
{
	WiFi.scanNetworks(true);
}

void scan_access_points_wait()
{
	for(;;) {
		int rc = WiFi.scanComplete();

		if (rc == -1)
			scan_access_points_start();

		if (rc >= 0)
			break;

		delay(100);
	}
}

std::map<std::string, std::tuple<int, uint8_t, int> > scan_access_points_get()
{
	std::map<std::string, std::tuple<int, uint8_t, int> > out;

	int n_networks = WiFi.scanComplete();

	for(int i=0; i<n_networks; i++) {
		out.insert({ WiFi.SSID(i).c_str(), { WiFi.RSSI(i), WiFi.encryptionType(i), WiFi.channel(i) } });

		if (debug)
			printf("[%2d] %s: %d\r\n", i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i));
	}

	WiFi.scanDelete();

	return out;
}

std::map<std::string, std::tuple<int, uint8_t, int> > scan_access_points()
{
	scan_access_points_start();

	scan_access_points_wait();

	return scan_access_points_get();
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

	printf("wifi status: %d\r\n", status);

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

bool try_connect(const std::vector<std::pair<std::string, std::string> > & targets,
	 	 const std::map<std::string, std::tuple<int, uint8_t, int> > & scan_results,
		 const int timeout,
		 std::function<bool(const int, const int, const std::string &)> progress_indicator)
{
	// scan through selected & scan-results and order by signal strength
	std::vector<std::tuple<std::string, std::string, int> > use;

	for(auto & target : targets) {
		auto scan_result = scan_results.find(target.first);

		if (scan_result != scan_results.end())
			use.push_back({ target.first, target.second, std::get<0>(scan_result->second) });
	}

	std::sort(use.begin(), use.end(), [](auto & a, auto & b) { return std::get<2>(b) < std::get<2>(a); });

	size_t nr               = 0;

	int    waiting_nr       = 0;

	bool   connecting_state = 0;

        bool   ok               = false;

	while(nr < targets.size()) {
		bool sleep = false;

		if (connecting_state == false) {
			if (debug)
				printf("Connecting to %s\r\n", std::get<0>(use.at(nr)).c_str());

			if (connect_to_access_point(std::get<0>(use.at(nr)), std::get<1>(use.at(nr)))) {
				connecting_state = true;

				waiting_nr       = 0;
			}
			else {
				nr++;
			}
		}
		else {
			auto status = check_wifi_connection_status();

			if (status == CS_CONNECTED) {
				if (debug)
					printf("Connected\r\n");

				progress_indicator(100, 100, std::get<0>(use.at(nr)));

				ok = true;

				break;
			}

			if (waiting_nr >= timeout) {
				if (debug)
					printf("timeout\r\n");

				nr++;

				connecting_state = false;

				continue;
			}

			if (status == CS_FAILURE) {
				if (debug)
					printf("Connection failed\r\n");

				nr++;

				wifi_disconnect();

				connecting_state = false;
			}
			else {
				sleep = true; // no error, just waiting
			}
		}

		if (nr < targets.size() && progress_indicator(nr * timeout + waiting_nr, targets.size() * timeout, std::get<0>(use.at(nr))) == false)
			break;

		if (sleep)
			delay(100);

		waiting_nr++;
	}
	
	if (!ok)
		progress_indicator(0, 100, "");

	return ok;
}

bool wifi_disconnect()
{
	WiFi.disconnect();

	return true;
}
