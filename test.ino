#include <string>

#include <LittleFS.h>

#include "configure.h"
#include "wifi.h"


bool progress_indicator(const int nr, const int mx, const std::string & which)
{
	printf("%3.2f%%: %s\r\n", nr * 100. / mx, which.c_str());

	return true;
}

void setup() {
	Serial.begin(115200);

	set_hostname("test123");

//	enable_wifi_debug();

	start_wifi("test123");  // enable wifi with AP (empty string for no wifi)

	if (!LittleFS.begin())
		printf("LittleFS.begin() failed\r\n");

	for(;;) {
		printf("load_configured_ap_list\r\n");
		// anything configured?
		auto targets = load_configured_ap_list();

		// no, start softap
		if (targets.size() == 0) {
			printf("configure_aps\r\n");

			configure_aps();

			ESP.reset();
		}

		// see what we can see
		printf("scan\r\n");
		auto available_access_points = scan_access_points();
		// try to connect
		printf("connect\r\n");
		if (try_connect(targets, available_access_points, 300, progress_indicator))
			break;

		// could not connect, (re-)configure
		printf("configure_aps\r\n");
		configure_aps();

		ESP.reset();
	}
}

void loop() {
}
