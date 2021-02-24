#include "types.h"
#include "udp.h"
#include <bitset>
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
/// status is 1 if confirmed, 0 if probably failed, or -1 if definitely failed
/// deliveryProbability is between 0 and 1
typedef void (*PacketCallback)(Channel*, u32 id, int status, f32 deliveryProbability);
/// datalen contains the length of data. packets cannot be larger than datalen.
/// callback should copy the data to be sent into `data`, then set datalen to the length of that data. setting datalen
/// to 0 will skip sending anything.
typedef void (*ChannelSendCallback)(Channel*, u32 id, void* data, u32* datalen, PacketCallback* onConfirm);

struct PacketStatus {
	PacketCallback onConfirm;
	u64 timestamp;
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
	void SetSendCallback(ChannelSendCallback callback);
	void SendImmediate(const void* data, u32 length, PacketCallback onConfirm = nullptr);
	f32 GetLatency();

	User* user;
	std::string app;
	std::unordered_set<CallbackData, CallbackDataHash> callbacks;
	ChannelSendCallback sendCallback;
	u32 next_lseq = 0, rseq = 0;
	bool rseqs[history_len];
	std::unordered_map<u32, PacketStatus> statuses;
	f32 rate = 0;
};
