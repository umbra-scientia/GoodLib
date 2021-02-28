#include "channel.h"
#include <cassert>
#include <random>
#include <stdlib.h>
#include <string.h>
#include <thread>

using namespace std;
using namespace std::chrono;

namespace channel {
	const u32 PACKET_SIZE = 1337;
	const u32 HEADER_SIZE = 16;

	unordered_set<Channel*> channels;
	thread* thread_handle = nullptr;

	f32 ewma_step(f32 x, f32 new_x, f32 weight) { return x * weight + new_x * (1 - weight); }

	void createHeader(Channel* channel, void* data) {
		auto lseq = channel->next_lseq;
		auto rseq = channel->rseq;

		auto header = &((u32*)data)[0];
		// 10 bits for lseq, 10 for latest rseq, and 12 for prev rseqs
		*header = (lseq << 22) | (rseq & 0x3FF << 12);
		for (size_t i = 1; i <= 12; i++) {
			*header |= channel->rseqs[(rseq - i + Channel::history_len) % Channel::history_len] << i;
		}
		// TODO: 12 bytes of crypto stuff
		for (size_t i = 1; i < 4; i++) {
			((i32*)data)[i] = 0;
		}
	}

	void init() {
		if (thread_handle) { return; }

		thread_handle = new thread([] {
			while (true) {
				// TODO: make dynamic
				this_thread::sleep_for(100ms);

				for (auto channel : channels) {
					auto lseq = channel->next_lseq;
					u32 datalen = PACKET_SIZE - HEADER_SIZE;
					u8 data[PACKET_SIZE];
					PacketCallback onConfirm = nullptr;
					if (channel->sendCallback) {
						channel->sendCallback(channel, lseq, &data[HEADER_SIZE], &datalen, &onConfirm);
					}
					if (!datalen) { continue; }

					createHeader(channel, data);
					auto timestamp = UDP::Send(data, datalen, channel->user->address);
					channel->statuses.insert({ lseq, PacketStatus { onConfirm, timestamp } });
					channel->next_lseq++;
				}
			}
		});
	}
}

using namespace channel;

void udpcb(void* userdata, void* data, int length, u64 timestamp, udp_address_t* from_hint) {
	auto channel = (Channel*)userdata;

	// TODO: use crypto stuff in header instead of this
	if (memcmp(channel->user->address.addr->sa_data, from_hint->addr->sa_data, 14) != 0) { return; }

	auto header = ((u32*)data)[0];
	auto rseq = (channel->rseq & 0xFFFFFC00) | (header >> 22);
	if (rseq - channel->rseq >= 512) { rseq -= 0x200; }
	auto lseq = (channel->next_lseq & 0xFFFFFC00) | ((header >> 12) & 0x3FF);
	if (lseq > channel->next_lseq) { lseq -= 0x200; }

	auto&& status = channel->statuses.at(lseq);

	auto new_rate = 1000.0 / (timestamp - status.timestamp);
	channel->rate = ewma_step(channel->rate, new_rate, 0.9);

	channel->rseqs[rseq % Channel::history_len] = true;
	if (rseq > channel->rseq) {
		for (u32 i = channel->rseq + 1; i < rseq; i++) {
			channel->rseqs[i % Channel::history_len] = false;
		}
		channel->rseq = rseq;
	}

	status.onConfirm(channel, lseq, 1, 1);
	channel->statuses.erase(lseq);

	for (size_t i = 1; i <= 12; i++) {
		auto flag = (header >> (i - 1)) & 1;
		if (flag) {
			auto lseq2 = lseq - i;
			auto&& status = channel->statuses.at(lseq2);
			status.onConfirm(channel, lseq2, 1, 1);
			channel->statuses.erase(lseq2);
		}
	}

	if (channel->recvCallback) {
		channel->recvCallback(channel, &((char*)data)[HEADER_SIZE], length - HEADER_SIZE);
	}
}

Channel::Channel(User* user, std::string app) {
	this->user = user;
	this->app = app;
	recvCallback = nullptr;
	sendCallback = nullptr;
	*rseqs = {};
	channels.insert(this);
	UDP::Listen(udpcb, this);
}

Channel::~Channel() {
	UDP::UnListen(udpcb, this);
	channels.erase(this);
}

void Channel::SetRecvCallback(ChannelRecvCallback callback) {
	recvCallback = callback;
}

void Channel::SetSendCallback(ChannelSendCallback callback) {
	sendCallback = callback;
}

void Channel::SendImmediate(const void* data, u32 length, PacketCallback onConfirm) {
	assert(length < PACKET_SIZE - HEADER_SIZE);

	u8 tosend[PACKET_SIZE];
	createHeader(this, tosend);
	memcpy(&tosend[HEADER_SIZE], data, length);

	auto timestamp = UDP::Send(data, length, user->address);
	statuses.insert({ next_lseq, PacketStatus { onConfirm, timestamp } });
	next_lseq++;
}

f32 Channel::GetLatency() {
	// TODO: implement
	return 0;
}
