#pragma once

#ifdef _MSV_VER

#define PRIuSIZE "Iu"
#define PRIxSIZE "Ix"

#else // ifdef _MSV_VER

#define PRIuSIZE "zu"
#define PRIxSIZE "zx"

#endif // ifdef _MSV_VER
