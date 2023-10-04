#include <stb_image_write.h>
// make internal stbi_write_png_to_mem available everywhere
STBIWDEF unsigned char *stbi_write_png_to_mem(const unsigned char *pixels, int stride_bytes, int x, int y, int n, int *out_len);
