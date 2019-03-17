#pragma once

#include <iostream>
#include <fstream>

void savePCACHEFileASCII(const std::string& fileName, const pcl::PointCloud<pcl::PointNormal>& cloud)
{
  std::cerr << "savePCACHEFileASCII() not implemented for PointNormal." << std::endl;
}

void savePCACHEFileASCII(const std::string& fileName, const pcl::PointCloud<pcl::PointXYZ>& cloud)
{
  std::cerr << "savePCACHEFileASCII() not implemented for PointXYZ." << std::endl;
}

template<typename PointT>
void savePCACHEFileASCII(const std::string& fileName, const pcl::PointCloud<PointT>& cloud)
{
  ofstream file;
  file.open(fileName);
  const auto endl = "\n";
  //const auto endl = "\r\n"; // Windows line ending
  // Write header.
  file
    << "pcache" << endl
    << "format ascii 1.0" << endl
    << "comment Export with mesh-sampler" << endl
    << "elements " << cloud.size() << endl
    << "property float position.x" << endl
    << "property float position.y" << endl
    << "property float position.z" << endl
    << "property float color.r" << endl
    << "property float color.g" << endl
    << "property float color.b" << endl
    << "property float color.a" << endl
    << "end_header" << endl;
  for (size_t i = 0; i < cloud.size(); i++)
  {
    const auto& p = cloud.points[i];
    const uint32_t rgb = *reinterpret_cast<const int*>(&p.rgb);
    const float r = ((rgb >> 16) & 0x0000ff) / 255.f;
    const float g = ((rgb >> 8)  & 0x0000ff) / 255.f;
    const float b = ((rgb)       & 0x0000ff) / 255.f;
    file << p.x << " " << p.y << " " << p.z << " " << r << " " << g << " " << b << " " << 1 << endl;
  }
  file.close();

}
