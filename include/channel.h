#include "types.h"
#include "udp.h"
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

struct User {
	udp_address_t address;
};

struct Channel;
struct PacketStatus;
typedef void (*ChannelCallback)(Channel*, const void* data, u32 length);
typedef void (*PacketCallback)(Channel*, const PacketStatus*);

struct PacketStatus {
	bool confirmed, failed;
	PacketCallback onConfirm, onFail;
};

struct Channel {
	static const u16 history_len = 4096;

	struct CallbackData {
		ChannelCallback callback;
		void* userdata;

		bool operator==(const CallbackData& rhs) const {
			return callback == rhs.callback && userdata == rhs.userdata;
		}
	};
	struct CallbackDataHash {
		std::size_t operator()(const Channel::CallbackData& v) const {
			auto hash = std::hash<ChannelCallback>()(v.callback);
			hash ^= std::hash<void*>()(v.userdata) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
			return hash;
		}
	};

	Channel(User* user, std::string app);
	~Channel();
	void Recv(ChannelCallback callback, void* userdata);
	void Send(const void* data, u32 length, PacketStatus* handler);
	f32 GetLatency();

	User* user;
	std::string app;
	std::unordered_set<CallbackData, CallbackDataHash> callbacks;
	u32 next_lseq = 0, rseq = 0;
	bool rseqs[history_len];
	std::unordered_map<u32, PacketStatus*> statuses;
};
