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

	Serial.setDebugOutput(true);

	set_hostname("test123");

	enable_wifi_debug();

	scan_access_points_start();

	if (!LittleFS.begin())
		printf("LittleFS.begin() failed\r\n");

	configure_wifi cw;

	if (cw.is_configured() == false) {
		start_wifi("test123");  // enable wifi with AP (empty string for no AP)

		cw.configure_aps();
	}
	else {
		start_wifi("");
	}

	// see what we can see
	printf("scan\r\n");
	auto available_access_points = scan_access_points();

	// try to connect
	printf("connect\r\n");

	auto state = try_connect_init(cw.get_targets(), available_access_points, 300, progress_indicator);

	connect_status_t cs = CS_IDLE;

	for(;;) {
		cs = try_connect_tick(state);

		if (cs != CS_IDLE)
			break;

		delay(100);
	}

	// could not connect, restart esp
	// you could also re-run the portal
	if (cs == CS_FAILURE) {
#if defined(ESP32)
		ESP.restart();
#else
		ESP.reset();
#endif
	}

	// connected!
}

void loop() {
}
