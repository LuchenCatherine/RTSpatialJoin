#include "cpu_cgal2.h"

int main(int ac, char **av) {


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

    std::ofstream outFile("CGALoutput.txt");

    std::vector<Mymesh> models;
    
    for (fs::directory_entry& AS : fs::directory_iterator(organ_path)) 
    {
        std::string file_path = AS.path().string();
        Mymesh model;
        if (model.load_from_off(file_path))
            models.push_back(model);
    }

    for (auto& model: models) model.create_aabb_tree();

    Mymesh query_model(query_path);
    query_model.create_aabb_tree();
    auto aabbtree1 = query_model.get_aabb_tree();
    

    std::cout << "CGAL result: ";

    auto t1 = std::chrono::high_resolution_clock::now();
    for (auto& model: models) {
        auto aabbtree2 = model.get_aabb_tree();
        bool intersected = aabbtree1->do_intersect(*aabbtree2);
        std::cout << intersected << " ";
    }
    std::cout << std::endl;

    auto t2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration;
        
    duration = t2 - t1;
    std::cout << "#cgal spatial query takes " << duration.count() << " seconds with " << models.size() << " background models" << std::endl; 
    
    outFile.close();

    return 0;
}