#include <map>
#include <string>


typedef enum {
	CS_IDLE,  // not connected yet
	CS_CONNECTED,
	CS_FAILURE  // not connected any more or can't connect
} connect_status_t;


// helper functions
bool                       connect_to_access_point     (const std::string ssid, const std::string password);

connect_status_t           check_wifi_connection_status();

std::optional<std::string> select_best_access_point    (const std::map<std::string, int> & list);

//// main functions

/// call this before invoking any other wifi function
bool                       set_hostname                (const std::string & hostname);

// ssid, signal-strength (in dB) pairs
std::map<std::string, int> scan_access_points          ();

// targets: ssid, password
// timeout in number of 100ms intervals: must be at least the number of ssid/password-pairs in 'targets'
// callback: interval_nr, max_nr_set, can return false to abort
bool                       try_connect                 (const std::vector<std::pair<std::string, std::string> > & targets,
							const std::map<std::string, int> & scan_results,
							const int timeout,
							std::function<bool(int, int)> & progress_indicator);

bool                       wifi_disconnect             ();
