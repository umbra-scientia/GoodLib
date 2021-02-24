#include "tree.h"
#include <map>
#include <mutex>
using namespace std;

static mutex mootecks;
static TreeTag next_tag = 1;
static map<string, TreeTag> tags;

TreeTag TreeData::GetTagID(string s) {
	lock_guard<mutex> mewtex(mootecks);
	auto& r = tags[s];
	if (!r) r = next_tag++;
	return r;
}

string TreeData::GetTagName(TreeTag t) {
	lock_guard<mutex> mewtex(mootecks);
	for(auto i: tags) {
		if (i.second == t) return i.first;
	}
	return "";
}
