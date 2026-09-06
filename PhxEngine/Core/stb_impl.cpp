// Both stb_image and stb_image_resize2 are single-header libraries: each
// needs exactly one translation unit defining its *_IMPLEMENTATION macro
// before including the header, to get the actual function bodies. Every
// other file just includes the headers normally.
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize2.h>
