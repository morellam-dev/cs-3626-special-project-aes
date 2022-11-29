/// AES.CPP
///
/// @brief Implements aes.h

#include "aes.h"

#include <algorithm>
#include <bit>      // for std::bit_cast
#include <numeric>  // std::bit_xor
#include <ranges>

#include "galois.h"
#include "rijndael.h"

namespace ranges = std::ranges;

auto aes::SubBytes(std::span<byte, 16> key, bool inverse) -> void {
  for (uint8_t &b : key) {
    b = rijndael::SubByte(b, inverse);
  }
}

auto aes::ShiftRows(std::span<byte, 16> key, bool inverse) -> void {
  for (int i = 0; i < 4; i++) {
    auto row = key.begin() + 4 * i;
    auto middle = row + (!inverse ? i : 4 - i);
    std::rotate(row, middle, row + 4);
  }
}

auto aes::MixColumns(std::span<byte, 16> key, bool inverse) -> void {
  // import lookup tables
  using namespace galois;
  for (int j = 0; j < 4; j++) {
    // get column vector
    const std::array<byte, 4> a = {key[j], key[4 + j], key[8 + j], key[12 + j]};
    if (!inverse) {
      uint8_t temp = a[0] ^ a[1] ^ a[2] ^ a[3];
      key[j] = a[0] ^ temp ^ x2[a[0] ^ a[1]];
      key[j + 4] = a[1] ^ temp ^ x2[a[1] ^ a[2]];
      key[j + 8] = a[2] ^ temp ^ x2[a[2] ^ a[3]];
      key[j + 12] = a[3] ^ temp ^ x2[a[3] ^ a[0]];
    } else {
      key[j] = xE[a[0]] ^ x9[a[3]] ^ xD[a[2]] ^ xB[a[1]];
      key[j + 4] = xE[a[1]] ^ x9[a[0]] ^ xD[a[3]] ^ xB[a[2]];
      key[j + 8] = xE[a[2]] ^ x9[a[1]] ^ xD[a[0]] ^ xB[a[3]];
      key[j + 12] = xE[a[3]] ^ x9[a[2]] ^ xD[a[1]] ^ xB[a[0]];
    }
  }
}

const std::array<aes::byte, 16> RconValues = {
    0xFF, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1B, 0x36};

auto aes::KeyExpansion(const std::span<byte, 16> key)
    -> std::array<std::array<byte, 16>, 11> {
  std::array<std::array<byte, 4>, 44> words;
  // Case 1: words[i] = k[i];
  // Case 2: words[i] = words[i-4] ^ SubWord(RotWord(words[i-1])) ^ Rcon[i/4]
  // Case 3: words[i] = words[i-4] ^ words[i-1]
  for (int i = 0; i < 44; i++) {
    if (i < 4) {
      ranges::copy(key.subspan(4 * i, 4), words[i].begin());
    } else if (i % 4 == 0) {
      std::array<byte, 4> t = words[i - 1];
      // Rotate left by 1
      ranges::rotate(t, t.begin() + 1);
      // Sub word
      ranges::transform(t, t.begin(),
                        [](byte b) { return rijndael::SubByte(b); });
      // Sub rcon
      t[0] ^= RconValues[i / 4];
      // words[i] = words[i-4] ^ t
      ranges::transform(words[i - 4], t, words[i].begin(), std::bit_xor{});
    } else {
      ranges::transform(words[i - 4], words[i - 1], words[i].begin(), std::bit_xor{});
    }
  }
  return std::bit_cast<std::array<std::array<byte, 16>, 11>>(words);
}