#pragma once

#define _64KB 65536

#define ALIGN_TO(size, alignment) (size + (alignment - 1) & ~(alignment-1))
