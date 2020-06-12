#include <Omega_h_file.hpp>
#include <Omega_h_library.hpp>
#include <Omega_h_mesh.hpp>
#include <Omega_h_comm.hpp>
#include <Omega_h_build.hpp>
#include <algorithm>
#include <vector>
#include <unordered_set>
#include <iostream>
#include <chrono>

int main(int argc,char **argv)
{
  auto lib = Omega_h::Library(&argc, &argv);
  Omega_h::Mesh mesh(&lib);
  Omega_h::binary::read("24k.osh", lib.world(), &mesh);
  // auto world = lib.world();
  // auto mesh = Omega_h::build_box(world, OMEGA_H_SIMPLEX, 1., 1., 0, 2, 2, 0);
  // Omega_h::vtk::write_parallel("fusion.vtk", &mesh, false);

  const int dim = mesh.dim();
  const int numCells = mesh.nelems();
  const int numVertices = mesh.nverts();
  const int numCorners = 3;

  // Get the coordinates of vertices
  Omega_h::HostRead<Omega_h::Real> vertexCoords(mesh.coords());
  assert(vertexCoords.size() == dim*numVertices);
  vertexCoords.data(); //This is the actual array

  // Get the vertices of each cell
  Omega_h::HostRead<Omega_h::LO> cell(mesh.ask_elem_verts());
  assert(cell.size() == numCorners*numCells);
  cell.data();
  for (int i = 0; i < cell.size(); i++)
  {
    if ((i+1)%3 != 0) 
    {
      printf("%d ", cell[i]+8);
    }
    else if ((i+1)%3 == 0) 
    {
      printf("%d \n", cell[i]+8);
    }
  }

  auto start = std::chrono::high_resolution_clock::now(); 
  
  // Get the edges for each face
  Omega_h::Read<Omega_h::LO> face2edges(mesh.get_adj(2, 1).ab2b);
  assert(face2edges.size() == 3*numCells);
  std::vector<int> boundary_edge;
  // If the edge appeared twice, then it is shared by two faces and therefore not a boundary edge
  for (int i = 0; i < face2edges.size(); i++)
  {
    bool duplicate = false;
    for (int j = 0; j < face2edges.size(); j++)
    {
      if (i != j && face2edges.get(i) == face2edges.get(j))
      {
        duplicate = true;
        break;
      }
    }
    if (duplicate == false)
    {
      std::cout << face2edges.get(i) << std::endl;
      boundary_edge.push_back(face2edges.get(i));
    }
  }

  auto stop = std::chrono::high_resolution_clock::now(); 
  auto duration = std::chrono::duration_cast<std::chrono::seconds>(stop - start); 
  std::cout << duration.count() << " seconds" << std::endl;  
  
  // /* Get the edges for each face. 
  // face2edge have the edges for each of the face */
  // Omega_h::Read<Omega_h::LO> face2edges = mesh.get_adj(2, 1).ab2b;
  // /* Get the faces for each edge
  // edge2face first have one of the faces made up by each of the edges and then the second face for shared edges */
  // Omega_h::Read<Omega_h::LO> edge2face = mesh.ask_up(1, 2).ab2b;

  // /* To get the boundary edges, first look for faces for the shared edges. 
  // These faces exist after the total_number_of_edges-1 index. Get the edges for those face. 
  // Check whether those edges make up the faces for the shared edges. 
  // This allows us to get the index for the shared edges */
  // bool boundary_edge_bool[mesh.nedges()] = {false};
  // for (int i = 0; i < mesh.nelems(); i++)
  // {
  //   int edge1 = face2edges.get(3*i);
  //   int edge2 = face2edges.get(3*i+1);
  //   int edge3 = face2edges.get(3*i+2);
    
  // }
  
  // for (int i = 0; i < edge2face.size(); i++)
  // {
  //   if ((i+1)%3 != 0) 
  //   {
  //     printf("%d ", edge2face.get(i));
  //   }
  //   else if ((i+1)%3 == 0) 
  //   {
  //     printf("%d \n", edge2face.get(i));
  //   }
  // }

  start = std::chrono::high_resolution_clock::now(); 

  // Get the vertices for each edge
  Omega_h::Read<Omega_h::LO> edge2verts = mesh.get_adj(1, 0).ab2b;
  assert(edge2verts.size() == 2*mesh.nedges());
  // By using a set, the vertices for all the boundary edges would not repeat
  // Can be replaced with an unordered_set
  std::unordered_set<int> boundary_vert;
  for (unsigned int i = 0; i < boundary_edge.size(); i++)
  {
    // Insert the two vertices for each edge
    boundary_vert.insert(edge2verts.get(2*boundary_edge[i])+numCells);
    boundary_vert.insert(edge2verts.get(2*boundary_edge[i]+1)+numCells);
  }

  stop = std::chrono::high_resolution_clock::now(); 
  duration = std::chrono::duration_cast<std::chrono::seconds>(stop - start); 
  std::cout << duration.count() << " seconds" << std::endl;  

  // Make the starting index of edges after number of cells and vertices with a lambda function
  for_each(boundary_edge.begin(), boundary_edge.end(), [&numCells, &numVertices](int &n){ n=n+numCells+numVertices; });

  return 0;
}
