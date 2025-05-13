// ======================================================================== //
// Copyright 2018-2019 Ingo Wald                                            //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "SpatialJoinRenderer2.h"
#include <boost/filesystem.hpp>
#include <iostream>
#include <fstream>

namespace fs = boost::filesystem;


/*! \namespace osc - Optix Siggraph Course */
namespace osc {
  
  /*! main entry point to this example - initially optix, print hello
    world, then exit */
  extern "C" int main(int ac, char **av)
  {
    try {
      
      if (ac < 2) {

        std::cout << "Please provide the folder for all models!" << std::endl;
        return 0;
      }
      else if (ac < 3) {
        std::cout << "Please provide the path of the query model!" << std::endl;
      }
      else if (ac < 4) {
        std::cout << "Please provide the launch dimension!" << std::endl;
        return 0;
      }
      else if (ac < 5) {
        std::cout << "Please provide the face threshold!" << std::endl;
        return 0;
      }
      
      // A set of meshes for spatial join
      std::string organ_path = std::string(av[1]);
      std::string query_path = std::string(av[2]);
      const int launchDim = std::atoi(av[3]);
      const int face_threshold = std::atoi(av[4]);
      
      // model1.addCube(vec3f(0.f,0.f,0.f),vec3f(2.f,2.f,2.f));
      // model2.addCube(vec3f(0.f,0.f,0.f),vec3f(1.f,1.f,1.f));
      
      // Create a renderer for spatial join
      auto t1 = std::chrono::high_resolution_clock::now();
      SpatialJoinRenderer *sjrenderer = new SpatialJoinRenderer();
      auto t2 = std::chrono::high_resolution_clock::now();

      // Output file
      std::ofstream outFile("RToutput.txt"); 

      std::chrono::duration<double> duration;

      duration = t2 - t1;
      std::cout << "spatial join renderer set up: " << duration.count() << " seconds" << std::endl; 
      outFile << "spatial join renderer set up: " << duration.count() << " seconds" << std::endl; 
      

      std::vector<TriangleMesh> models;
      for (fs::directory_entry& AS : fs::directory_iterator(organ_path)) 
      {

          std::string file_path = AS.path().string();
          TriangleMesh model1;
          model1.addTriangleMesh(file_path);
          
          // filter out meshes with faces less than face_threshold;
          if (model1.index.size() < face_threshold) continue;

          models.push_back(model1);
      }

      std::cout << models.size() << std::endl;
      TriangleMesh query_model;
      query_model.addTriangleMesh(query_path);
      
      sjrenderer->render(models, launchDim);
      sjrenderer->query(query_model);

      // int num_edges = 3*(model1.index.size() + model2.index.size());
      // bool h_result[num_edges];
      // sjrenderer->downloadResult(h_result);
      // Create a wrapper for get result
      // sjrenderer->getResult();
      outFile.close();
    

    } catch (std::runtime_error& e) {
      std::cout << GDT_TERMINAL_RED << "FATAL ERROR: " << e.what()
                << GDT_TERMINAL_DEFAULT << std::endl;
      exit(1);
    }
    
    return 0;
  }
  
} // ::osc


/*
remaining: 
1. LaunchParams.h
2. write back result, devicePrograms.cu
3. move the project to separate folder, rewrite CMakeLists.txt for compilation
*/

