#include <string>
#include <utility>
#include <vector>

#include <AsyncJson.h>

#if defined(ESP32)
#include <AsyncTCP.h>
#else
#include <ESPAsyncTCP.h>
#endif

#include <ESPAsyncWebServer.h>

#include "wifi.h"


#define CFG_FILE "/wifi-aps.json"

class configure_wifi
{
private:
	std::vector<std::pair<std::string, std::string> > configured_ap_list;

	bool has_triggered_scan        { false };

	bool load_configured_ap_list();
	bool save_configured_ap_list();

	bool add_ssid_to_ap_list       (const std::string & ssid, const std::string & password);
	bool delete_ssid_from_ap_list  (const std::string & ssid);

	void basic_response            (AsyncWebServerRequest *client, const int code, const std::string & mime_type, const std::string & text);

	void request_add_ap            (AsyncWebServerRequest *client, const JsonVariant & json_string);
	void request_configured_ap_list(AsyncWebServerRequest *client);
	void request_del_ap            (AsyncWebServerRequest *client, const JsonVariant & json_string);
	void request_stop              (AsyncWebServerRequest *client);
	void request_wifi_scan         (AsyncWebServerRequest *client);
	void request_wifi_status       (AsyncWebServerRequest *client);

public:
	configure_wifi();
	~configure_wifi();

	bool is_configured() const;

	std::vector<std::pair<std::string, std::string> > get_targets() const;

	bool configure_aps(const int timeout);
};
