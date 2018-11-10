#include "hash.hpp"

#include <sodium/randombytes.h>

static uint64_t hashkey = ((uint64_t)randombytes_random()<<32) + randombytes_random();

GenericHash::GenericHash()
{
    crypto_generichash_init(&state, (const uint8_t*)&hashkey, sizeof(hashkey), hashsize);
}

void GenericHash::update(const void *data, size_t size)
{
    crypto_generichash_update(&state, (const uint8_t*)data, size);
}

void GenericHash::final(uint8_t* hash)
{
    crypto_generichash_final(&state, hash, hashsize);
}
