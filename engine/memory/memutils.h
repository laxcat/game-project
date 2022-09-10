#pragma once

#define ALIGN_SIZE (8)
#define ALIGN(V) (((V) + ALIGN_SIZE - 1) / ALIGN_SIZE * ALIGN_SIZE)
