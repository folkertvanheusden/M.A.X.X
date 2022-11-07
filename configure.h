#include <string>
#include <utility>
#include <vector>

#include "wifi.h"


#define CFG_FILE "/wifi-aps.json"

class configure_wifi
{
private:
	std::vector<std::pair<std::string, std::string> > configured_ap_list;

	bool load_configured_ap_list();
	bool save_configured_ap_list();

	bool add_ssid_to_ap_list     (const std::string & ssid, const std::string & password);
	bool delete_ssid_from_ap_list(const std::string & ssid);

	void http_header(WiFiClient & client, const int code, const String & mime_type);

	void request_add_app           (WiFiClient & client, const String & json_string);
	void request_configured_ap_list(WiFiClient & client);
	void request_del_app           (WiFiClient & client, const String & json_string);
	void request_some_file         (WiFiClient & client, const String & url);
	void request_stop              (WiFiClient & client);
	void request_wifi_scan         (WiFiClient & client);
	void request_wifi_status       (WiFiClient & client);

public:
	configure_wifi();
	~configure_wifi();

	bool is_configured() const;

	std::vector<std::pair<std::string, std::string> > get_targets() const;

	bool configure_aps();
};
