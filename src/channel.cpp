#include "channel.h"
#include <random>
#include <memory.h>
#include <thread>

using namespace std;

namespace channel {
	struct SendData {
		void* data;
		u32 length;
		udp_address_t address;
	};

	vector<SendData> sendbuf;
	thread* thread_handle = nullptr;

	void init() {
		if (!thread_handle) {
			thread_handle = new thread([] {
				while (true) {
					this_thread::sleep_for(1s);

					for (auto&& send : sendbuf) {
						UDP::Send(send.data, send.length, send.address);
						free(send.data);
					}
				}
			});
		}
	}
}

using namespace channel;

void udpcb(void* userdata, void* data, int length, udp_address_t from_hint) {
	auto channel = (Channel*)userdata;

	if (memcmp(channel->user->address.addr->sa_data, from_hint.addr->sa_data, 14) != 0) { return; }

	auto header = ((u32*)data)[0];
	auto send_id = 0; // TODO: write the thing here
	auto recv_id = 0; // TODO: write the thing here

	channel->recv_id = (channel->recv_id > send_id) ? (channel->recv_id) : (send_id);
	channel->recv_ids.insert(send_id); // TODO: clean up recv_ids eventually?
	auto&& status = channel->statuses.at(recv_id);
	status->confirmed = true;
	status->onConfirm(channel, status);

	recv_id--;
	for (size_t i = 0; i < 12; i++) {
		auto flag = (header >> i) & 1;
		if (flag) {
			auto&& status = channel->statuses.at(recv_id - i);
			status->confirmed = true;
			status->onConfirm(channel, status);
		}
	}

	for (auto [callback, userdata] : channel->callbacks) {
		callback(channel, &((char*)data)[16], length - 16);
	}
}

Channel::Channel(User* user, std::string app) {
	this->user = user;
	this->app = app;
	next_send_id = 0;
	recv_id = 0;
	UDP::Listen(udpcb, this);
}

Channel::~Channel() {
	UDP::UnListen(udpcb, this);
}

void Channel::Recv(ChannelCallback callback, void* userdata) {
	callbacks.insert({ callback, userdata });
}

void Channel::Send(const void* data, u32 length, PacketStatus* handler) {
	auto send_id = next_send_id++;

	auto tosend = (char*)malloc(length + 16);
	auto header = &((u32*)tosend)[0];
	// 10 bits for send_id, 10 for latest recv_id, and 12 for prev recv_ids
	*header = (send_id << 22) | (recv_id & 0x3FF << 12);
	for (auto&& prev_recv_id : recv_ids) {
		*header |= (1 << (recv_id - prev_recv_id - 1)) & 0xFFF;
	}
	// 12 random bytes
	for (size_t i = 1; i < 4; i++) {
		((i32*)tosend)[i] = 0;
	}
	// rest of data
	memcpy(&tosend[16], data, length);

	sendbuf.push_back(SendData { tosend, length, user->address });
	statuses.insert({ send_id++, handler });
}

f32 Channel::GetLatency() {
	// TODO: implement
	return 0;
}
