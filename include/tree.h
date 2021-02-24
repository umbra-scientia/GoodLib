#pragma once
#include "ref.h"
#include <string>
#include <vector>
#include <set>

typedef unsigned long long TreeTag;

struct TreeData : public RCObj {
	static TreeTag GetTagID(std::string);
	static std::string GetTagName(TreeTag);

	TreeTag type;
	std::set<Ref<TreeData>> flags;
	std::vector<Ref<TreeData>> sequence;
};
