#pragma once


#define _KB(x) ((size_t) (x) << 10)
#define _MB(x) ((size_t) (x) << 20)
#define _GB(x) ((size_t) (x) << 30)
#define _64KB _KB(64)
#define _1MB _MB(1)
#define _2MB _MB(2)
#define _4MB _MB(4)
#define _8MB _MB(8)
#define _16MB _MB(16)
#define _32MB _MB(32)
#define _64MB _MB(64)
#define _128MB _MB(128)
#define _256MB _MB(256)

#define BYTE_TO_MB(x) ((size_t) (x) / _MB(1))
#define BYTE_TO_GB(x) ((size_t) (x) / _GB(1))