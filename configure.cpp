#include <string>
#include <utility>
#include <vector>

#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>

#include "wifi.h"


std::vector<std::pair<std::string, std::string> > load_configured_ap_list()
{
	File dataFile = LittleFS.open("wifi-aps.json", "r");

	if (!dataFile)
		return { };

	size_t size = dataFile.size();

	if (size > 1024)  // this should not happen
		return { };

	char buffer[1024];

	dataFile.read(reinterpret_cast<uint8_t *>(buffer), size);
	buffer[1023] = 0x00;

	dataFile.close();

	StaticJsonDocument<1024> json;  // TODO: what is the maximum ssid & password length?

	auto error = deserializeJson(json, buffer);

	if (error)  // this should not happen
		return { };

	std::vector<std::pair<std::string, std::string> > out;

	JsonArray array = json.as<JsonArray>();

	for(auto element : array)
		out.push_back({ element["ssid"], element["password"] });

	return out;
}

bool save_configured_ap_list(const std::vector<std::pair<std::string, std::string> > & list)
{
	DynamicJsonDocument json(1024);

	for(auto & element : list) {
		DynamicJsonDocument ar_elem(128);

		ar_elem["ssid"]     = element.first;
		ar_elem["password"] = element.second;

		json.add(ar_elem);
	}

	String output;
	serializeJson(json, output);

	File dataFile = LittleFS.open("wifi-aps.json", "w");

	if (!dataFile)
		return false;

	dataFile.write(output.c_str(), output.length());

	dataFile.close();

	return true;
}

bool delete_ssid_from_ap_list(std::vector<std::pair<std::string, std::string> > & list, const std::string & ssid)
{
	const size_t size = list.size();

	for(size_t i = 0; i<size; i++) {
		if (list.at(i).first == ssid) {
			list.erase(list.begin() + i);

			return true;
		}
	}

	return false;
}

bool add_ssid_to_ap_list(std::vector<std::pair<std::string, std::string> > & list, const std::string & ssid, const std::string & password)
{
	const size_t size = list.size();

	for(size_t i = 0; i<size; i++) {
		if (list.at(i).first == ssid)
			return false;
	}

	list.push_back({ ssid, password });

	std::sort(list.begin(), list.end(), [](auto & a, auto & b) { return strcmp(a.first.c_str(), b.first.c_str()) > 0; });

	return true;
}

void request_wifi_status(WiFiClient & client)
{
	client.write("HTTP/1.0 200\r\n");
	client.write("Server: M.A.X.X - by vanheusden.com\r\n");
	client.print("Content-Type: application/json\r\n");
	client.write("\r\n");
	client.write("{}");
}

void request_wifi_scan(WiFiClient & client)
{
	auto aps_visible = scan_access_points();

	DynamicJsonDocument jsonDoc(1024);

	for(auto & ap : aps_visible) {
		JsonObject entry = jsonDoc.createNestedObject();

		entry["ssid"]           = ap.first;
		entry["rssi"]           = std::get<0>(ap.second);
		entry["encryptionType"] = std::get<1>(ap.second);
		entry["channel"]        = std::get<2>(ap.second);
	}

	client.write("HTTP/1.0 200\r\n");
	client.write("Server: M.A.X.X - by vanheusden.com\r\n");
	client.print("Content-Type: application/json\r\n");
	client.write("\r\n");

	serializeJson(jsonDoc, client);
}

void request_configured_ap_list(WiFiClient & client)
{
	auto aps_configured = load_configured_ap_list();

	DynamicJsonDocument jsonDoc(1024);

	for(size_t i=0; i<aps_configured.size(); i++) {
		JsonObject entry = jsonDoc.createNestedObject();

		entry["id"]     = i;
		entry["apName"] = aps_configured.at(i).first;
		entry["apPass"] = aps_configured.at(i).second.empty() ? false : true;
	}

	client.write("HTTP/1.0 200\r\n");
	client.write("Server: M.A.X.X - by vanheusden.com\r\n");
	client.print("Content-Type: application/json\r\n");
	client.write("\r\n");

	if (aps_configured.size() == 0)
		client.write("[]");
	else
		serializeJson(jsonDoc, client);
}

void request_add_app(WiFiClient & client, const String & json_string)
{
	DynamicJsonDocument json_buffer(256);
	deserializeJson(json_buffer, json_string.c_str());

	auto aps = load_configured_ap_list();

	if (add_ssid_to_ap_list(aps, json_buffer["apName"].as<std::string>(), json_buffer["apPass"].as<std::string>())) {
		if (save_configured_ap_list(aps)) {
			client.write("HTTP/1.0 200\r\n");
			client.write("Server: M.A.X.X - by vanheusden.com\r\n");
			client.print("Content-Type: application/json\r\n");
			client.write("\r\n");

			client.write("{\"message\":\"New AP added\"}");

			return;
		}
	}

	client.write("HTTP/1.0 500\r\n");
	client.write("Server: M.A.X.X - by vanheusden.com\r\n");
	client.print("Content-Type: application/json\r\n");
	client.write("\r\n");

	client.write("{\"message\":\"New AP not added due to some error\"}");
}

void request_del_app(WiFiClient & client, const String & json_string)
{
	DynamicJsonDocument json_buffer(256);
	deserializeJson(json_buffer, json_string.c_str());

	auto aps = load_configured_ap_list();

	if (delete_ssid_from_ap_list(aps, aps.at(json_buffer["id"].as<int>()).first)) {
		if (save_configured_ap_list(aps)) {
			client.write("HTTP/1.0 200\r\n");
			client.write("Server: M.A.X.X - by vanheusden.com\r\n");
			client.print("Content-Type: application/json\r\n");
			client.write("\r\n");

			client.write("{\"message\":\"AP deleted\"}");

			return;
		}
	}

	client.write("HTTP/1.0 500\r\n");
	client.write("Server: M.A.X.X - by vanheusden.com\r\n");
	client.print("Content-Type: application/json\r\n");
	client.write("\r\n");

	client.write("{\"message\":\"New AP not deleted due to some error\"}");
}

void request_some_file(WiFiClient & client, const String & url)
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

		client.write("HTTP/1.0 404 NOT FOUND\r\n\r\n");

		return;
	}

	client.write("HTTP/1.0 200\r\n");
	client.write("Server: M.A.X.X - by vanheusden.com\r\n");
	client.print("Content-Type: " + mime_type + "\r\n");
	client.write("\r\n");

	size_t size = dataFile.size();

	for(size_t i=0; i<size; i++)
		client.write(dataFile.read());

	dataFile.close();
}

void request_stop(WiFiClient & client)
{
	client.write("HTTP/1.0 200\r\n");
	client.write("Server: M.A.X.X - by vanheusden.com\r\n");
	client.print("Content-Type: application/json\r\n");
	client.write("\r\n");
	client.write("{ \"message\":\"Activating new configuration...\" }");
}

bool configure_aps()
{
	bool       rc     = false;

	WiFiServer server(80);

	server.begin();

	auto aps = load_configured_ap_list();

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
