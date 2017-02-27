#pragma once

const int c_block_size		= 16384;
const int c_min_word_length = 3;

#define USE_JUDY_ARRAY
#ifdef USE_JUDY_ARRAY
extern "C" {
#include <Judy.h>
}
#define JUDY_MAX_INDEX_LEN 2048
#endif