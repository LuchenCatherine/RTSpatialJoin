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

#include "SpatialJoinRenderer.h"
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

        std::cout << "Please provide the paths for two models!" << std::endl;
        return 0;
      }
      else if (ac < 3) {
        std::cout << "Please provide the launch dimension!" << std::endl;
        return 0;
      }
      


      
      std::string body_path = std::string(av[1]);
      const int launchDim = std::atoi(av[2]);
      const int face_threshold = std::atoi(av[3]);
      
      // model1.addCube(vec3f(0.f,0.f,0.f),vec3f(2.f,2.f,2.f));
      // model2.addCube(vec3f(0.f,0.f,0.f),vec3f(1.f,1.f,1.f));
      
      // model1 and model2 are arguments passed to renderer. 
      auto t1 = std::chrono::high_resolution_clock::now();
      SpatialJoinRenderer *sjrenderer = new SpatialJoinRenderer();
      auto t2 = std::chrono::high_resolution_clock::now();

      // output file
      std::ofstream outFile("RToutput.txt"); 

      std::chrono::duration<double> duration;

      duration = t2 - t1;
      std::cout << "spatial join renderer set up: " << duration.count() << " seconds" << std::endl; 
      outFile << "spatial join renderer set up: " << duration.count() << " seconds" << std::endl; 
      

      for (fs::directory_entry& organ_path : fs::directory_iterator(body_path)) 
      {
        std::string organ_name = organ_path.path().filename().string();
        // std::cout << organ_name << std::endl;   
        for (fs::directory_entry& AS : fs::directory_iterator(organ_path)) 
        {
            std::string file_path = AS.path().string();
            // self-self intersection
            auto t3 = std::chrono::high_resolution_clock::now();
            TriangleMesh model1;
            TriangleMesh model2;
            model1.addTriangleMesh(file_path);
            model2.addTriangleMesh(file_path);
            auto t33 = std::chrono::high_resolution_clock::now();

            if (model1.index.size() < face_threshold || model2.index.size() < face_threshold) continue;

            outFile << "Model1: " << file_path << std::endl;
            outFile << "Model2: " << file_path << std::endl;
            outFile << "Model1 size: " << model1.index.size() << std::endl;
            outFile << "Model2 size: " << model2.index.size() << std::endl;

            duration = t33 - t3;
            std::cout << "Model loading time: " << duration.count() << " seconds" << std::endl;
            outFile << "Model loading time: " << duration.count() << " seconds" << std::endl;

            auto t4 = std::chrono::high_resolution_clock::now();
            sjrenderer->render(model1, model2, launchDim, outFile);
            auto t5 = std::chrono::high_resolution_clock::now();

            int num_edges = 3*(model1.index.size() + model2.index.size());
            bool h_result[num_edges];
            sjrenderer->downloadResult(h_result);
            bool is_collided = false;
            for (int i = 0; i < num_edges; i++) {
              is_collided |= h_result[i];
            }
            auto t6 = std::chrono::high_resolution_clock::now();

            duration = t5 - t4;
            std::cout << "spatial join rendering: " << duration.count() << " seconds" << std::endl; 
            duration = t6 - t5;

            outFile << "RT download result: " << duration.count() << " seconds" << std::endl;
            std::cout << "compute results: " << duration.count() << " seconds" << std::endl; 

            std::cout << "\n*******************************************************\n" << std::endl;
            if (is_collided) {
              std::cout << "Result: two models are collided. \n" << std::endl;
              outFile << "Result: two models are collided. \n" << std::endl;

            }
            else {
              std::cout << "Result: two models are not collided. \n" << std::endl;
              outFile << "Result: two models are collided. \n" << std::endl;
            }

        }
      }

      outFile.close();
      // sjrenderer->render(model1, model2, launchDim);
      // auto t3 = std::chrono::high_resolution_clock::now();
      // sjrenderer->render(model2, model1, 16);
      // auto t33 = std::chrono::high_resolution_clock::now();
      
      
      
      // duration = t1 - t0;
      // std::cout << "model loading time: " << duration.count() << " seconds" << std::endl; 

      // duration = t2 - t1;
      // std::cout << "spatial join renderer set up: " << duration.count() << " seconds" << std::endl; 

      // duration = t3 - t2;
      // std::cout << "spatial join rendering (launch dim 1024): " << duration.count() << " seconds" << std::endl; 

      // duration = t33 - t3;
      // std::cout << "spatial join rendering (launch dim 16): " << duration.count() << " seconds" << std::endl; 

      // // the size of h_result is the total number of edges.

      // int num_edges = 3*(model1.index.size() + model2.index.size());
      // bool h_result[num_edges];
      

      // auto t4 = std::chrono::high_resolution_clock::now();
      // // download results
      // sjrenderer->downloadResult(h_result);
      
      // bool is_collided = false;
      // for (int i = 0; i < num_edges; i++) {
      //   is_collided |= h_result[i];
      // }
      // auto t5 = std::chrono::high_resolution_clock::now();
      
      // duration = t5 - t4;
      // std::cout << "compute result: " << duration.count() << " seconds" << std::endl; 
      
      // std::cout << "\n*******************************************************\n" << std::endl;
      // if (is_collided) std::cout << "Result: two models are collided. \n" << std::endl;
      // else std::cout << "Result: two models are not collided. \n" << std::endl;


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

