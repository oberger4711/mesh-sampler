#include "texture.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Texture::Texture(const std::string& file_name)
{
  int channels;
  unsigned char* raw_data = stbi_load(file_name.c_str(), &w, &h, &channels, 3);
  data.resize(w * h);
  for (int y = 0; y < h; ++y)
  {
    for (int x = 0; x < w; ++x)
    {
      const int index = y * w + x;
      data[index] = Pixel
      {
        .r = raw_data[index * 3 + 0],
        .g = raw_data[index * 3 + 1],
        .b = raw_data[index * 3 + 2],
      };
    }
  }
  stbi_image_free(raw_data);
}

Pixel Texture::operator()(const int x, const int y) const
{
  return data[x + w * y];
}

bool Texture::isInBounds(const int x, const int y) const
{
  return x >= 0 && x < w &&
    y >= 0 && y < h;
}

int Texture::width() const
{
  return w;
}

int Texture::height() const
{
  return h;
}
