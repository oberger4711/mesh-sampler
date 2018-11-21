#pragma once

#include <vector>
#include <string>

struct Pixel
{
  int r;
  int g;
  int b;
};

class Texture
{
public:
  Texture(const std::string& file_name);
  inline Pixel operator()(const int x, const int y) const;
  bool isInBounds(const int x, const int y) const;
  int width() const;
  int height() const;

private:
  int w;
  int h;
  std::vector<Pixel> data;
};
