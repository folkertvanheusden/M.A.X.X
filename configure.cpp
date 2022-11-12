#include <string>
#include <utility>
#include <vector>

#include <ArduinoJson.h>
#include <LittleFS.h>

#include "configure.h"
#include "wifi.h"


static const std::string name = "M.A.X.X - by vanheusden.com";


configure_wifi::configure_wifi()
{
	load_configured_ap_list();
}

configure_wifi::~configure_wifi()
{
}

bool configure_wifi::is_configured() const
{
	return configured_ap_list.empty() == false;
}

std::vector<std::pair<std::string, std::string> > configure_wifi::get_targets() const
{
	return configured_ap_list;
}

bool configure_wifi::load_configured_ap_list()
{
	File dataFile = LittleFS.open(CFG_FILE, "r");

	if (!dataFile)
		return false;

	size_t size = dataFile.size();

	if (size > 1024) {  // this should not happen
		dataFile.close();

		return false;
	}

	char buffer[1024];

	dataFile.read(reinterpret_cast<uint8_t *>(buffer), size);
	buffer[1023] = 0x00;

	dataFile.close();

	StaticJsonDocument<1024> json;  // TODO: what is the maximum ssid & password length?

	auto error = deserializeJson(json, buffer);

	if (error)  // this should not happen
		return { };

	configured_ap_list.clear();

	JsonArray array = json.as<JsonArray>();

	for(auto element : array)
		configured_ap_list.push_back({ element["ssid"], element["password"] });

	return true;
}

bool configure_wifi::save_configured_ap_list()
{
	StaticJsonDocument<1024> json_doc;

	for(auto & element : configured_ap_list) {
		JsonObject ar_elem  = json_doc.createNestedObject();

		ar_elem["ssid"]     = element.first;
		ar_elem["password"] = element.second;
	}

	File dataFile = LittleFS.open(CFG_FILE, "w");

	if (!dataFile)
		return false;

	serializeJson(json_doc, dataFile);

	dataFile.close();

	return true;
}

bool configure_wifi::delete_ssid_from_ap_list(const std::string & ssid)
{
	const size_t size = configured_ap_list.size();

	for(size_t i = 0; i<size; i++) {
		if (configured_ap_list.at(i).first == ssid) {
			configured_ap_list.erase(configured_ap_list.begin() + i);

			return true;
		}
	}

	return false;
}

bool configure_wifi::add_ssid_to_ap_list(const std::string & ssid, const std::string & password)
{
	const size_t size = configured_ap_list.size();

	for(size_t i = 0; i<size; i++) {
		if (configured_ap_list.at(i).first == ssid)
			return false;
	}

	configured_ap_list.push_back({ ssid, password });

	std::sort(configured_ap_list.begin(), configured_ap_list.end(), [](auto & a, auto & b) { return strcmp(a.first.c_str(), b.first.c_str()) > 0; });

	return true;
}

void configure_wifi::request_wifi_status(AsyncWebServerRequest * client)
{
	StaticJsonDocument<1024> json_doc;

	json_doc["hostname"]      = WiFi.getHostname();

#if defined(ESP32)
	json_doc["free-heap"]     = ESP.getMinFreeHeap();
	json_doc["heap-size"]     = ESP.getHeapSize();
#else
	uint32_t free = 0;
	uint16_t max  = 0;
	uint8_t  frag = 0;
	ESP.getHeapStats(&free, &max, &frag);

	json_doc["heap-size"]     = max;
	json_doc["free-heap"]     = free;
	json_doc["fragmentation"] = frag;
#endif

	AsyncResponseStream *response = client->beginResponseStream("application/json");
	response->addHeader("Server", name.c_str());
	serializeJson(json_doc, *response);
	client->send(response);
}

void configure_wifi::basic_response(AsyncWebServerRequest * client, const int code, const std::string & mime_type, const std::string & text)
{
	AsyncWebServerResponse *response = client->beginResponse(code, mime_type.c_str(), text.c_str());

	response->addHeader("Server", name.c_str());

	client->send(response);
}

void configure_wifi::request_wifi_scan(AsyncWebServerRequest * client)
{
	if (!has_triggered_scan) {
		scan_access_points_start();

		has_triggered_scan = true;
	}

	if (scan_access_points_wait()) {
		auto aps_visible = scan_access_points_get();

		StaticJsonDocument<2048> json_doc;

		for(auto & ap : aps_visible) {
			JsonObject entry = json_doc.createNestedObject();

			entry["ssid"]           = ap.first;
			entry["rssi"]           = std::get<0>(ap.second);
			entry["encryptionType"] = std::get<1>(ap.second);
			entry["channel"]        = std::get<2>(ap.second);
		}

		if (aps_visible.empty())
			basic_response(client, 200, "application/json", "[]");
		else {
			AsyncResponseStream *response = client->beginResponseStream("application/json");
			response->addHeader("Server", name.c_str());

			serializeJson(json_doc, *response);

			client->send(response);
		}

		has_triggered_scan = false;
	}
	else {
		basic_response(client, 200, "application/json", "{\"status\":\"scanning\"}");
	}
}

void configure_wifi::request_configured_ap_list(AsyncWebServerRequest * client)
{
	StaticJsonDocument<1024> json_doc;

	for(size_t i=0; i<configured_ap_list.size(); i++) {
		JsonObject entry = json_doc.createNestedObject();

		entry["id"]     = i;
		entry["apName"] = configured_ap_list.at(i).first;
		entry["apPass"] = configured_ap_list.at(i).second.empty() ? false : true;
	}

	if (configured_ap_list.empty() == true)
		basic_response(client, 200, "application/json", "[]");
	else {
		AsyncResponseStream *response = client->beginResponseStream("application/json");
		response->addHeader("Server", name.c_str());
		serializeJson(json_doc, *response);
		client->send(response);
	}
}

void configure_wifi::request_add_ap(AsyncWebServerRequest * client, const JsonVariant & json_buffer)
{
	if (add_ssid_to_ap_list(json_buffer["apName"].as<std::string>(), json_buffer["apPass"].as<std::string>())) {
		if (save_configured_ap_list()) {
			basic_response(client, 200, "application/json", "{\"message\":\"New AP added\"}");

			return;
		}
	}

	basic_response(client, 500, "application/json", "{\"message\":\"New AP not added due to some error\"}");
}

void configure_wifi::request_del_ap(AsyncWebServerRequest * client, const JsonVariant & json_buffer)
{
	if (delete_ssid_from_ap_list(configured_ap_list.at(json_buffer["id"].as<int>()).first)) {
		if (save_configured_ap_list()) {
			basic_response(client, 200, "application/json", "{\"message\":\"AP deleted\"}");

			return;
		}
	}

	basic_response(client, 500, "application/json", "{\"message\":\"AP not deleted due to some error\"}");
}

void configure_wifi::request_stop(AsyncWebServerRequest * client)
{
	basic_response(client, 200, "application/json", "{\"message\":\"Activating new configuration...\"}");
}

bool configure_wifi::configure_aps()
{
	bool       rc     = false;

#if defined(ESP32)
	auto       prev_sleep_mode = WiFi.getSleep();

	WiFi.setSleep(false);
#endif

	AsyncWebServer server(80);

	server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
			request->redirect("/gui.html");
		});

	server.on("/api/wifi/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
			request_wifi_status(request);
		});

	server.on("/api/wifi/scan", HTTP_GET, [this](AsyncWebServerRequest *request){
			request_wifi_scan(request);
		});

	server.on("/api/wifi/configlist", HTTP_GET, [this](AsyncWebServerRequest *request){
			request_configured_ap_list(request);
		});

	server.addHandler(new AsyncCallbackJsonWebHandler("/api/wifi/add", [this](AsyncWebServerRequest *request, JsonVariant &json) {
					request_add_ap(request, json);
				}));

	server.addHandler(new AsyncCallbackJsonWebHandler("/api/wifi/id", [this](AsyncWebServerRequest *request, JsonVariant &json) {
					request_del_ap(request, json);
				}));

	server.on("/api/wifi/softAp/stop", HTTP_POST, [this, &rc](AsyncWebServerRequest *request) {
			printf("Request to switch to run mode\r\n");

			request_stop(request);

			rc = true;
		});

	server.serveStatic("/", LittleFS, "/");

	server.begin();

	while(rc == false)
		delay(100);

#if defined(ESP32)
	WiFi.setSleep(prev_sleep_mode);
#endif

	return rc;
}
