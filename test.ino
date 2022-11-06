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

	start_wifi("test123");

	if (!LittleFS.begin())
		printf("LittleFS.begin() failed\r\n");

	for(;;) {
		// anything configured?
		auto targets = load_configured_ap_list();

		// no, start softap
		if (targets.size() == 0)
			configure_aps();

		// see what we can see
		auto available_access_points = scan_access_points();
		// try to connect
		if (try_connect(targets, available_access_points, 300, progress_indicator))
			break;

		// could not connect, (re-)configure
		configure_aps();
	}
}

void loop() {
}
