#include "http.h"
#include <sstream>
#include <string>
#include <vector>
#include <map>

using namespace std;

struct HTTP_STATUS_CODE_t {
	HTTP_STATUS_CODE_t() {
		table[100] = "Continue";
		table[200] = "OK";
		table[304] = "Not Modified";
		table[403] = "Forbidden";
		table[404] = "File Not Found";
		table[500] = "Internal Server Error";
		// FIXME add these strings
	}
	string operator[](unsigned short x) {
		return table[x];
	}
	map<unsigned short, string> table;
};
HTTP_STATUS_CODE_t HTTP_STATUS_CODE;

void HTTPListener::OnConnect(Ref<TCPConnection> con, Ref<tcp_address_t> from_hint) {
	Ref<HTTPRequest> req = new HTTPRequest();
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
			auto res = OnRequest(req, con);
			if (res) {
				if (!res->status.size()) {
					res->status = HTTP_STATUS_CODE[res->code & 0xFFFF];
				}
				stringstream ss;
				ss << "HTTP/1.1 " << res->code << res->status << "\n";
				for (auto i: res->headers) {
					for (auto j: i.second) {
						ss << i.first << ": " << j << "\n";
					}
				}
				ss << "\n";
				auto head = ss.str();
				con->Write(head.data(), head.size());
				con->Write(res->body.data(), res->body.size());
			} else {
				con->Write("HTTP/1.1 500 ERROR\n\n", 20);
			}
			return;
		}
	}
}

Ref<HTTPResponse> HTTPListener::OnRequest(Ref<HTTPRequest> req, Ref<TCPConnection> con) {
	return nullptr;
}
