#include "fbr_texture.h"

void createTextureImage(const FbrAppState* pAppState) {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load("textures/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels) {
        printf( "%s - failed to load texture image!\n", __FUNCTION__ );
    }


}
