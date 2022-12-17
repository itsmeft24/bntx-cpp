#pragma once

constexpr inline size_t div_round_up(size_t x, size_t d) {
    return (x + d - 1) / d;
}

constexpr inline size_t round_up(size_t x, size_t n) {
    return ((x + n - 1) / n) * n;
}

extern "C" void swizzle_block_linear(size_t width, size_t height, size_t depth, unsigned char* source, size_t source_len, unsigned char* destination, size_t destination_len, size_t block_height, size_t bytes_per_pixel);

extern "C" void deswizzle_block_linear(size_t width, size_t height, size_t depth, unsigned char* source, size_t source_len, unsigned char* destination, size_t destination_len, size_t block_height, size_t bytes_per_pixel);

extern "C" size_t swizzled_mip_size(size_t width, size_t height, size_t depth, size_t block_height, size_t bytes_per_pixel);

extern "C" size_t deswizzled_mip_size(size_t width, size_t height, size_t depth, size_t bytes_per_pixel);

extern "C" size_t block_height_mip0(size_t height);

extern "C" size_t mip_block_height(size_t mip_height, size_t _block_height_mip0);
