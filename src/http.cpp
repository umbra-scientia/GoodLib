#include "http.h"
#include <string>
#include <vector>
#include <map>

using namespace std;

void HTTPListener::OnConnect(Ref<TCPConnection> con, Ref<tcp_address_t> from_hint) {
	string cmd, path, proto, key, value;
	map<string, vector<string>> headers;
	int state = 0;
	while (con->Valid()) {
		char c = 0;
		if (!con->Read(&c, 1)) continue;
		printf("Request: state=%d, cmd=%s, path=%s, proto=%s, headers.size()=%d\n", state, cmd.c_str(), path.c_str(), proto.c_str(), headers.size());
		if (state == 0) {
			if (isspace(c)) state = 1;
			else cmd += tolower(c);
			continue;
		}
		if (state == 1 && !isspace(c)) state = 2;
		if (state == 2) {
			if (isspace(c)) state = 3;
			else path += c;
			continue;
		}
		if (state == 3 && !isspace(c)) state = 4;
		if (state == 4) {
			if (c == '\n') state = 6;
			else proto += c;
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
			} else if (!isspace(c)) {
				key += tolower(c);
			}
			continue;
		}
		if (state == 7) {
			if (c == '\n') {
				headers[key].push_back(value);
				key = "";
				value = "";
				state = 6;
			} else if (c != '\r') {
				value += c;
			}
			continue;
		}
		if (state == 8) {
			printf("*** DUNNIT ***\nRequest:\nstate=%d\ncmd=%s\npath=%s\nproto=%s\nheaders.size()=%d\n", state, cmd.c_str(), path.c_str(), proto.c_str(), headers.size());
			for(auto i: headers) {
				if (!i.second.size()) {
					printf("headers[\"%s\"]=[]\n", i.first.c_str());
					continue;
				}
				printf("headers[\"%s\"]=[\n", i.first.c_str());
				for(auto j: i.second) {
					printf("\t%s\n", j.c_str());
				}
				printf("]\n", i.first.c_str());
			}
			printf("\n");
		}
	}
}
