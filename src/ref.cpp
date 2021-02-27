#include "ref.h"
RCObj::RCObj() { usage = 0; }
RCObj::~RCObj() {}
void RCObj::Reserve() { usage++; }
void RCObj::Release() { if (--usage <= 0) delete this; }
bool RCObj::Valid() { return true; }
