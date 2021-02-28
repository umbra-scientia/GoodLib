#pragma once
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
typedef void (*ChannelRecvCallback)(Channel*, const void* data, u32 length);
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
	Channel(User* user, std::string app);
	~Channel();
	void SetRecvCallback(ChannelRecvCallback callback);
	void SetSendCallback(ChannelSendCallback callback);
	void SendImmediate(const void* data, u32 length, PacketCallback onConfirm = nullptr);
	f32 GetLatency();

	User* user;
	std::string app;
	ChannelRecvCallback recvCallback;
	ChannelSendCallback sendCallback;
	u32 next_lseq = 0, rseq = 0;
	bool rseqs[history_len];
	std::unordered_map<u32, PacketStatus> statuses;
	f32 rate = 0;
};
