/*
 * Software License Agreement (BSD License)
 *
 *  Point Cloud Library (PCL) - www.pointclouds.org
 *  Copyright (c) 2010-2011, Willow Garage, Inc.
 *
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of the copyright holder(s) nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <pcl/visualization/pcl_visualizer.h>
#include <pcl/io/ply_io.h>
#include <pcl/io/vtk_lib_io.h>
#include <pcl/common/transforms.h>
#include <vtkVersion.h>
#include <vtkPLYReader.h>
#include <vtkOBJReader.h>
#include <vtkTriangle.h>
#include <vtkTriangleFilter.h>
#include <vtkPolyDataMapper.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/console/print.h>
#include <pcl/console/parse.h>
#include "texture.h"
#include "pcache.h"

enum class FileType
{
  PLY,
  PCD,
  PCACHE
};

inline double
uniform_deviate (int seed)
{
  double ran = seed * (1.0 / (RAND_MAX + 1.0));
  return ran;
}

inline void
randomPointTriangle (float a1, float a2, float a3, float b1, float b2, float b3, float c1, float c2, float c3,
                     float r1, float r2, Eigen::Vector3f& p)
{
  float r1sqr = std::sqrt (r1);
  float OneMinR1Sqr = (1 - r1sqr);
  float OneMinR2 = (1 - r2);
  a1 *= OneMinR1Sqr;
  a2 *= OneMinR1Sqr;
  a3 *= OneMinR1Sqr;
  b1 *= OneMinR2;
  b2 *= OneMinR2;
  b3 *= OneMinR2;
  c1 = r1sqr * (r2 * c1 + b1) + a1;
  c2 = r1sqr * (r2 * c2 + b2) + a2;
  c3 = r1sqr * (r2 * c3 + b3) + a3;
  p[0] = c1;
  p[1] = c2;
  p[2] = c3;
}

inline void
sampleTexture (const double** meshPoints, const double** texturePoints)
{

}

inline void
randPSurface (vtkPolyData * polydata, const Texture& texture, std::vector<double> * cumulativeAreas, double totalArea, Eigen::Vector3f& p, bool calcNormal, Eigen::Vector3f& n, bool calcColor, Eigen::Vector3f& c)
{
  float r = static_cast<float> (uniform_deviate (rand ()) * totalArea);

  std::vector<double>::iterator low = std::lower_bound (cumulativeAreas->begin (), cumulativeAreas->end (), r);
  vtkIdType el = vtkIdType (low - cumulativeAreas->begin ());

  double A[3], B[3], C[3];
  vtkIdType npts = 0;
  vtkIdType *ptIds = NULL;
  polydata->GetCellPoints (el, npts, ptIds);
  polydata->GetPoint (ptIds[0], A);
  polydata->GetPoint (ptIds[1], B);
  polydata->GetPoint (ptIds[2], C);
  if (calcNormal)
  {
    // OBJ: Vertices are stored in a counter-clockwise order by default
    Eigen::Vector3f v1 = Eigen::Vector3f (A[0], A[1], A[2]) - Eigen::Vector3f (C[0], C[1], C[2]);
    Eigen::Vector3f v2 = Eigen::Vector3f (B[0], B[1], B[2]) - Eigen::Vector3f (C[0], C[1], C[2]);
    n = v1.cross (v2);
    n.normalize ();
  }
  float r1 = static_cast<float> (uniform_deviate (rand ()));
  float r2 = static_cast<float> (uniform_deviate (rand ()));
  randomPointTriangle (float (A[0]), float (A[1]), float (A[2]),
                       float (B[0]), float (B[1]), float (B[2]),
                       float (C[0]), float (C[1]), float (C[2]), r1, r2, p);

  if (calcColor)
  {
    // Old implementation uses vertex colors.
    /*
    vtkUnsignedCharArray *const colors = vtkUnsignedCharArray::SafeDownCast (polydata->GetPointData ()->GetScalars ());
    if (colors && colors->GetNumberOfComponents () == 3)
    {
      double cA[3], cB[3], cC[3];
      colors->GetTuple (ptIds[0], cA);
      colors->GetTuple (ptIds[1], cB);
      colors->GetTuple (ptIds[2], cC);

      randomPointTriangle (float (cA[0]), float (cA[1]), float (cA[2]),
                           float (cB[0]), float (cB[1]), float (cB[2]),
                           float (cC[0]), float (cC[1]), float (cC[2]), r1, r2, c);

    }
    else
    {
      static bool printed_once = false;
      if (!printed_once)
        PCL_WARN ("Mesh has no vertex colors, or vertex colors are not RGB!");
      printed_once = true;
    }
    */
    // New implementation samples texture.
    double tA[2], tB[2], tC[2];
    auto* const textureCoords = polydata->GetPointData()->GetTCoords();
    textureCoords->GetTuple(ptIds[0], tA);
    textureCoords->GetTuple(ptIds[1], tB);
    textureCoords->GetTuple(ptIds[2], tC);
    Eigen::Vector3f tFloat;
    randomPointTriangle (float (tA[0]), float (tA[1]), 0.f,
                         float (tB[0]), float (tB[1]), 0.f,
                         float (tC[0]), float (tC[1]), 0.f, r1, r2, tFloat);
    // Scale.
    tFloat[1] = 1 - tFloat[1];
    //tFloat[0] = 1 - tFloat[0];
    tFloat = tFloat.cwiseProduct(Eigen::Vector3f(texture.width(), texture.height(), 0));
    //std::cout << tFloat[0] << " " << tFloat[1] << std::endl;
    // Round.
    Eigen::Vector2i t = (tFloat + Eigen::Vector3f(0.5f, 0.5f, 0.f)).cast<int>().topRows<2>();
    //std::cout << t[0] << " " << t[1] << std::endl;
    // Sample pixel.
    if (texture.isInBounds(t[0], t[1]))
    {
      const auto px = texture(t[0], t[1]);
      //std::cout << px.r << " " << px.g << " " << px.b << std::endl;
      c[0] = px.r;
      c[1] = px.g;
      c[2] = px.b;
    }
    else
    {
      std::cerr << "Point (" << t[0] << ", " << t[1] << ") out of bounds!" << std::endl;
    }
  }
}

void
uniform_sampling (vtkSmartPointer<vtkPolyData> polydata, const Texture& texture, size_t n_samples, bool calc_normal, bool calc_color, pcl::PointCloud<pcl::PointXYZRGBNormal> & cloud_out)
{
  polydata->BuildCells ();
  vtkSmartPointer<vtkCellArray> cells = polydata->GetPolys ();

  double p1[3], p2[3], p3[3], totalArea = 0;
  std::vector<double> cumulativeAreas (cells->GetNumberOfCells (), 0);
  size_t i = 0;
  vtkIdType npts = 0, *ptIds = NULL;
  for (cells->InitTraversal (); cells->GetNextCell (npts, ptIds); i++)
  {
    polydata->GetPoint (ptIds[0], p1);
    polydata->GetPoint (ptIds[1], p2);
    polydata->GetPoint (ptIds[2], p3);
    totalArea += vtkTriangle::TriangleArea (p1, p2, p3);
    cumulativeAreas[i] = totalArea;
  }

  cloud_out.points.resize (n_samples);
  cloud_out.width = static_cast<pcl::uint32_t> (n_samples);
  cloud_out.height = 1;

  for (i = 0; i < n_samples; i++)
  {
    Eigen::Vector3f p;
    Eigen::Vector3f n;
    Eigen::Vector3f c;
    randPSurface (polydata, texture, &cumulativeAreas, totalArea, p, calc_normal, n, calc_color, c);
    cloud_out.points[i].x = p[0];
    cloud_out.points[i].y = p[1];
    cloud_out.points[i].z = p[2];
    if (calc_normal)
    {
      cloud_out.points[i].normal_x = n[0];
      cloud_out.points[i].normal_y = n[1];
      cloud_out.points[i].normal_z = n[2];
    }
    if (calc_color)
    {
      cloud_out.points[i].r = static_cast<uint8_t>(c[0]);
      cloud_out.points[i].g = static_cast<uint8_t>(c[1]);
      cloud_out.points[i].b = static_cast<uint8_t>(c[2]);
    }
  }
}

using namespace pcl;
using namespace pcl::io;
using namespace pcl::console;

const int default_number_samples = 100000;
const float default_leaf_size = 0.01f;

void
printHelp (int, char **argv)
{
  print_error ("Syntax is: %s input.obj texture.{png, jpeg, ...} output.{ply, pcd, pcache} <options>\n", argv[0]);
  print_info ("  where options are:\n");
  print_info ("                     -n_samples X      = number of samples (default: ");
  print_value ("%d", default_number_samples);
  print_info (")\n");
  print_info (
              "                     -leaf_size X  = the XYZ leaf size for the VoxelGrid -- for data reduction (default: ");
  print_value ("%f", default_leaf_size);
  print_info (" m)\n");
  print_info ("                     -write_normals = flag to write normals to the output\n");
  print_info ("                     -write_colors  = flag to write colors to the output\n");
  print_info (
              "                     -vis_result = flag to stop visualizing the generated\n");
}

template<typename PointT> void
saveFile(const std::string& fileName, const pcl::PointCloud<PointT>& cloud, const FileType fileType)
{
  switch (fileType)
  {
    case FileType::PLY:
      pcl::io::savePLYFileASCII(fileName, cloud);
      break;
    case FileType::PCD:
      pcl::io::savePCDFileASCII(fileName, cloud);
      break;
    case FileType::PCACHE:
      savePCACHEFileASCII(fileName, cloud);
      break;
  }
}

/* ---[ */
int
main (int argc, char **argv)
{
  print_info ("Convert a textured CAD model to a point cloud using uniform sampling. For more information, use: %s -h\n",
              argv[0]);

  if (argc < 4)
  {
    printHelp (argc, argv);
    return (-1);
  }

  // Parse command line arguments
  int SAMPLE_POINTS_ = default_number_samples;
  parse_argument (argc, argv, "-n_samples", SAMPLE_POINTS_);
  float leaf_size = default_leaf_size;
  parse_argument (argc, argv, "-leaf_size", leaf_size);
  bool vis_result = find_switch (argc, argv, "-vis_result");
  const bool write_normals = find_switch (argc, argv, "-write_normals");
  const bool write_colors = find_switch (argc, argv, "-write_colors");
  std::string texture_file_name(argv[2]);

  // Parse the command line arguments for .ply files
  std::vector<int> ply_file_indices = parse_file_extension_argument (argc, argv, ".ply");
  std::vector<int> pcd_file_indices = parse_file_extension_argument (argc, argv, ".pcd");
  std::vector<int> pcache_file_indices = parse_file_extension_argument (argc, argv, ".pcache");
  if (ply_file_indices.size () != 1 && pcd_file_indices.size () != 1 && pcache_file_indices.size() != 1)
  {
    print_error ("Need a single PLY, PCD or PCACHE file as output to continue.\n");
    return (-1);
  }
  std::vector<int> obj_file_indices = parse_file_extension_argument (argc, argv, ".obj");
  if (obj_file_indices.size () != 1)
  {
    print_error ("Need a single input OBJ file to continue.\n");
    return (-1);
  }

  vtkSmartPointer<vtkPolyData> polydata1 = vtkSmartPointer<vtkPolyData>::New ();
  if (obj_file_indices.size () == 1)
  {
    vtkSmartPointer<vtkOBJReader> readerQuery = vtkSmartPointer<vtkOBJReader>::New ();
    readerQuery->SetFileName (argv[obj_file_indices[0]]);
    readerQuery->Update ();
    polydata1 = readerQuery->GetOutput ();
  }

  //make sure that the polygons are triangles!
  vtkSmartPointer<vtkTriangleFilter> triangleFilter = vtkSmartPointer<vtkTriangleFilter>::New ();
#if VTK_MAJOR_VERSION < 6
  triangleFilter->SetInput (polydata1);
#else
  triangleFilter->SetInputData (polydata1);
#endif
  triangleFilter->Update ();

  vtkSmartPointer<vtkPolyDataMapper> triangleMapper = vtkSmartPointer<vtkPolyDataMapper>::New ();
  triangleMapper->SetInputConnection (triangleFilter->GetOutputPort ());
  triangleMapper->Update ();
  polydata1 = triangleMapper->GetInput ();

  bool INTER_VIS = false;

  if (INTER_VIS)
  {
    visualization::PCLVisualizer vis;
    vis.addModelFromPolyData (polydata1, "mesh1", 0);
    vis.setRepresentationToSurfaceForAllActors ();
    vis.spin ();
  }

  // Load texture.
  Texture texture(texture_file_name);

  pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr cloud_1 (new pcl::PointCloud<pcl::PointXYZRGBNormal>);
  uniform_sampling (polydata1, texture, SAMPLE_POINTS_, write_normals, write_colors, *cloud_1);

  if (INTER_VIS)
  {
    visualization::PCLVisualizer vis_sampled;
    vis_sampled.addPointCloud<pcl::PointXYZRGBNormal> (cloud_1);
    if (write_normals)
      vis_sampled.addPointCloudNormals<pcl::PointXYZRGBNormal> (cloud_1, 1, 0.02f, "cloud_normals");
    vis_sampled.spin ();
  }

  // Voxelgrid
  VoxelGrid<PointXYZRGBNormal> grid_;
  grid_.setInputCloud (cloud_1);
  grid_.setLeafSize (leaf_size, leaf_size, leaf_size);

  pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr voxel_cloud (new pcl::PointCloud<pcl::PointXYZRGBNormal>);
  grid_.filter (*voxel_cloud);

  if (vis_result)
  {
    visualization::PCLVisualizer vis3 ("VOXELIZED SAMPLES CLOUD");
    vis3.addPointCloud<pcl::PointXYZRGBNormal> (voxel_cloud);
    if (write_normals)
      vis3.addPointCloudNormals<pcl::PointXYZRGBNormal> (voxel_cloud, 1, 0.02f, "cloud_normals");
    vis3.spin ();
  }

  FileType fileType;
  std::string fileName;
  if (!ply_file_indices.empty())
  {
    fileType = FileType::PLY;
    fileName = argv[ply_file_indices[0]];
  }
  else if (!pcd_file_indices.empty())
  {
    fileType = FileType::PCD;
    fileName = argv[pcd_file_indices[0]];
  }
  else if (!pcache_file_indices.empty())
  {
    fileType = FileType::PCACHE;
    fileName = argv[pcache_file_indices[0]];
  }

  if (write_normals && write_colors)
  {
    saveFile (fileName, *voxel_cloud, fileType);
  }
  else if (write_normals)
  {
    pcl::PointCloud<pcl::PointNormal>::Ptr cloud_xyzn (new pcl::PointCloud<pcl::PointNormal>);
    // Strip uninitialized colors from cloud:
    pcl::copyPointCloud (*voxel_cloud, *cloud_xyzn);
    saveFile (fileName, *cloud_xyzn, fileType);
  }
  else if (write_colors)
  {
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud_xyzrgb (new pcl::PointCloud<pcl::PointXYZRGB>);
    // Strip uninitialized normals from cloud:
    pcl::copyPointCloud (*voxel_cloud, *cloud_xyzrgb);
    saveFile (fileName, *cloud_xyzrgb, fileType);
  }
  else // !write_normals && !write_colors
  {
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_xyz (new pcl::PointCloud<pcl::PointXYZ>);
    // Strip uninitialized normals and colors from cloud:
    pcl::copyPointCloud (*voxel_cloud, *cloud_xyz);
    saveFile (fileName, *cloud_xyz, fileType);
  }
}

