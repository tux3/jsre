#ifndef HASH_HPP
#define HASH_HPP

#include <sodium/crypto_generichash.h>

class GenericHash
{
public:
    constexpr static const size_t hashsize = 12;

public:
    GenericHash();
    void update(const void* data, size_t size);
    void final(uint8_t hash[hashsize]);

private:
    crypto_generichash_state state;
};

#endif // HASH_HPP
