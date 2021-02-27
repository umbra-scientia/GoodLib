#include "http.h"
#include <string>
#include <vector>
#include <map>

using namespace std;

void HTTPListener::OnConnect(Ref<TCPConnection> con, Ref<tcp_address_t> from_hint) {
	Ref<HTTPRequest> req = new HTTPRequest();
	req->con = con;
	string key, value;
	int state = 0;
	while (con->Valid()) {
		char c = 0;
		if (!con->Read(&c, 1)) continue;
		//printf("Request: state=%d, cmd=%s, path=%s, proto=%s, headers.size()=%d\n", state, cmd.c_str(), path.c_str(), proto.c_str(), headers.size());
		if (state == 0) {
			if (isspace(c)) state = 1;
			else req->cmd += c;
			continue;
		}
		if (state == 1 && !isspace(c)) state = 2;
		if (state == 2) {
			if (isspace(c)) state = 3;
			else req->path += c;
			continue;
		}
		if (state == 3 && !isspace(c)) state = 4;
		if (state == 4) {
			if (c == '\n') state = 6;
			else req->proto += c;
			continue;
		}
		if (state == 5) {
			if (c == '\n') state = 6;
			continue;
		}
		if (state == 6) {
			if (c == '\n') {
				state = 8;
			} else if (c == ':') {
				state = 7;
				continue;
			} else if (!isspace(c)) {
				key += tolower(c);
				continue;
			}
		}
		if (state == 7) {
			if (c == '\n') {
				req->headers[key].push_back(value);
				key = "";
				value = "";
				state = 6;
			} else if (c != '\r') {
				value += c;
			}
			continue;
		}
		if (state == 8) {
			// FIXME: download POST data.
			if (!OnRequest(req)) {
				con->Write("HTTP/1.1 400 ERROR\n\n", 20);
			}
			return;
		}
	}
}

bool HTTPListener::OnRequest(Ref<HTTPRequest> req) {
	return false;
}
