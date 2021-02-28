// Copyright © 2019, Mykos Hudson-Crisp. All rights reserved.
#pragma once

extern "C" {
    void c25519(void* output, const void* secret, const void* source);
    void X25519(void* output, const void* secret, const void* source);
    void X25519base(void* output, const void* secret);
};
