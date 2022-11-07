#include <string>
#include <utility>
#include <vector>

#include <ArduinoJson.h>
#include <LittleFS.h>

#include "configure.h"
#include "wifi.h"


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
	File dataFile = LittleFS.open("wifi-aps.json", "r");

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
	DynamicJsonDocument json(1024);

	for(auto & element : configured_ap_list) {
		DynamicJsonDocument ar_elem(128);

		ar_elem["ssid"]     = element.first;
		ar_elem["password"] = element.second;

		json.add(ar_elem);
	}

	File dataFile = LittleFS.open("wifi-aps.json", "w");

	if (!dataFile)
		return false;

	serializeJson(json, dataFile);

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

void configure_wifi::http_header(WiFiClient & client, const int code, const String & mime_type)
{
	client.print("HTTP/1.0 ");
	client.print(code);
	client.print("\r\n");

	client.print("Server: M.A.X.X - by vanheusden.com\r\n");

	client.print("Content-Type: " + mime_type + "\r\n");

	client.print("\r\n");
}

void configure_wifi::request_wifi_status(WiFiClient & client)
{
	http_header(client, 200, "application/json");

	DynamicJsonDocument json_doc(1024);

	json_doc["hostname"]      = WiFi.getHostname();

	uint32_t free = 0;
	uint16_t max  = 0;
	uint8_t  frag = 0;
	ESP.getHeapStats(&free, &max, &frag);

	json_doc["getHeapSize"]   = max;
	json_doc["freeHeap"]      = free;
	json_doc["fragmentation"] = frag;

	serializeJson(json_doc, client);
}

void configure_wifi::request_wifi_scan(WiFiClient & client)
{
	auto aps_visible = scan_access_points();

	DynamicJsonDocument json_doc(1024);

	for(auto & ap : aps_visible) {
		JsonObject entry = json_doc.createNestedObject();

		entry["ssid"]           = ap.first;
		entry["rssi"]           = std::get<0>(ap.second);
		entry["encryptionType"] = std::get<1>(ap.second);
		entry["channel"]        = std::get<2>(ap.second);
	}

	http_header(client, 200, "application/json");

	serializeJson(json_doc, client);
}

void configure_wifi::request_configured_ap_list(WiFiClient & client)
{
	DynamicJsonDocument json_doc(1024);

	for(size_t i=0; i<configured_ap_list.size(); i++) {
		JsonObject entry = json_doc.createNestedObject();

		entry["id"]     = i;
		entry["apName"] = configured_ap_list.at(i).first;
		entry["apPass"] = configured_ap_list.at(i).second.empty() ? false : true;
	}

	http_header(client, 200, "application/json");

	if (configured_ap_list.empty() == true)
		client.write("[]");
	else
		serializeJson(json_doc, client);
}

void configure_wifi::request_add_app(WiFiClient & client, const String & json_string)
{
	DynamicJsonDocument json_buffer(256);
	deserializeJson(json_buffer, json_string.c_str());

	if (add_ssid_to_ap_list(json_buffer["apName"].as<std::string>(), json_buffer["apPass"].as<std::string>())) {
		if (save_configured_ap_list()) {
			http_header(client, 200, "application/json");

			client.write("{\"message\":\"New AP added\"}");

			return;
		}
	}

	http_header(client, 500, "application/json");

	client.write("{\"message\":\"New AP not added due to some error\"}");
}

void configure_wifi::request_del_app(WiFiClient & client, const String & json_string)
{
	DynamicJsonDocument json_buffer(256);
	deserializeJson(json_buffer, json_string.c_str());

	if (delete_ssid_from_ap_list(configured_ap_list.at(json_buffer["id"].as<int>()).first)) {
		if (save_configured_ap_list()) {
			http_header(client, 200, "application/json");

			client.write("{\"message\":\"AP deleted\"}");

			return;
		}
	}

	http_header(client, 500, "application/json");

	client.write("{\"message\":\"New AP not deleted due to some error\"}");
}

void configure_wifi::request_some_file(WiFiClient & client, const String & url)
{
	Serial.print(F("Serve static file: "));
	Serial.println(url);

	String mime_type = "text/html";

	int dot          = url.lastIndexOf('.');
	
	if (dot != -1) {
		String ext = url.substring(dot);

		if (ext == ".js")
			mime_type = "text/javascript";
		else if (ext == ".css")
			mime_type = "text/css";
	}

	// TODO: path sanity checks
	File dataFile = LittleFS.open(url, "r");

	if (!dataFile) {
		Serial.println(F("404"));

		http_header(client, 404, "text/html");
		client.write("???");

		return;
	}

	http_header(client, 200, mime_type);

	client.write(dataFile);

	dataFile.close();
}

void configure_wifi::request_stop(WiFiClient & client)
{
	http_header(client, 200, "application/json");

	client.write("{ \"message\":\"Activating new configuration...\" }");
}

bool configure_wifi::configure_aps()
{
	bool       rc     = false;

	WiFiServer server(80);

	server.begin();

	for(;;) {
		WiFiClient client = server.available();

		if (!client) {  // server.accept is not blocking
			delay(10);

			continue;
		}

		// ignore request headers & retrieve rest data
		String json_string;
		String request;
		bool   header      = true;

		while (client.available() || header) {
			String line = client.readStringUntil('\r');

			if (header && request.isEmpty()) {
				request = line;

				client.setTimeout(100);
			}
			else if (line.length() == 1 && line[0] == '\n' && header)
				header = false;
			else if (!header) {
				json_string += line;
			}

			yield();
		}

		String cmd;
		String url;

		int    space = request.indexOf(' ');
		if (space != -1) {
			cmd = request.substring(0, space);

			url = request.substring(space + 1);

			space = url.indexOf(' ');

			if (space != -1)
				url = url.substring(0, space);
		}

		Serial.println(url);

		if (url == "/api/wifi/status") {
			request_wifi_status(client);
		}
		else if (url == "/api/wifi/scan") {
			request_wifi_scan(client);
		}
		else if (url == "/api/wifi/configlist") {
			request_configured_ap_list(client);
		}
		else if (url == "/api/wifi/add" && cmd == "POST") {
			request_add_app(client, json_string);
		}
		else if (url == "/api/wifi/id" && cmd == "DELETE") {
			request_del_app(client, json_string);
		}
		else if (url == "/api/wifi/softAp/stop") {
			printf("Request to switch to run mode\r\n");

			request_stop(client);

			client.stop();

			delay(1000);

			rc = true;

			break;
		}
		else if (url == "/" && cmd == "GET")
			request_some_file(client, "gui.html");
		else {
			request_some_file(client, url);
		}

		client.stop();
	}

	return rc;
}
