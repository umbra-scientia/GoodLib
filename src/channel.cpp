#include "../include/goodlib/channel.h"
#include <cstdlib>
#include <random>
#include <thread>

using namespace std;
using namespace std::chrono;

namespace channel {
	struct SendData {
		Channel* channel;
		u32 lseq;
		void* data;
		u32 length;
	};

	vector<SendData> sendbuf;
	thread* thread_handle = nullptr;

	f32 ewma_step(f32 x, f32 new_x, f32 weight) { return x * weight + new_x * (1 - weight); }

	void init() {
		if (thread_handle) { return; }

		thread_handle = new thread([] {
			while (true) {
				// TODO: make dynamic
				this_thread::sleep_for(100ms);

				for (auto&& send : sendbuf) {
					send.channel->statuses.at(send.lseq)->timestamp =
						UDP::Send(send.data, send.length, send.channel->user->address);
					free(send.data);
				}
			}
		});
	}
}

using namespace channel;

void udpcb(void* userdata, void* data, int length, u64 timestamp, udp_address_t from_hint) {
	auto channel = (Channel*)userdata;

	// TODO: use crypto stuff in header instead of this
	if (memcmp(channel->user->address.addr->sa_data, from_hint.addr->sa_data, 14) != 0) { return; }

	auto header = ((u32*)data)[0];
	auto rseq = (channel->rseq & 0xFFFFFC00) | (header >> 22);
	if (rseq - channel->rseq >= 512) { rseq -= 0x200; }
	auto lseq = (channel->next_lseq & 0xFFFFFC00) | ((header >> 12) & 0x3FF);
	if (lseq > channel->next_lseq) { lseq -= 0x200; }

	auto new_rate = 1000.0 / (timestamp - channel->statuses.at(lseq)->timestamp);
	channel->rate = ewma_step(channel->rate, new_rate, 0.9);

	channel->rseqs[rseq % Channel::history_len] = true;
	if (rseq > channel->rseq) {
		for (u32 i = channel->rseq + 1; i < rseq; i++) {
			channel->rseqs[i % Channel::history_len] = false;
		}
		channel->rseq = rseq;
	}

	auto&& status = channel->statuses.at(lseq);
	status->confirmed = true;
	status->onConfirm(channel, status);
	for (size_t i = 1; i <= 12; i++) {
		auto flag = (header >> (i - 1)) & 1;
		if (flag) {
			auto&& status = channel->statuses.at(lseq - i);
			if (status->confirmed) { continue; }
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
	*rseqs = {};
	UDP::Listen(udpcb, this);
}

Channel::~Channel() {
	UDP::UnListen(udpcb, this);
}

void Channel::Recv(ChannelCallback callback, void* userdata) {
	callbacks.insert({ callback, userdata });
}

void Channel::Send(const void* data, u32 length, PacketStatus* handler) {
	auto lseq = next_lseq++;

	auto tosend = (char*)malloc(length + 16);
	auto header = &((u32*)tosend)[0];
	// 10 bits for lseq, 10 for latest rseq, and 12 for prev rseqs
	*header = (lseq << 22) | (rseq & 0x3FF << 12);
	for (size_t i = 1; i <= 12; i++) {
		*header |= rseqs[(rseq - i + history_len) % history_len] << i;
	}
	// TODO: 12 bytes of crypto stuff
	for (size_t i = 1; i < 4; i++) {
		((i32*)tosend)[i] = 0;
	}
	// rest of data
	memcpy(&tosend[16], data, length);

	sendbuf.push_back(SendData { this, lseq, tosend, length });
	statuses.insert({ lseq++, handler });
}

f32 Channel::GetLatency() {
	// TODO: implement
	return 0;
}
