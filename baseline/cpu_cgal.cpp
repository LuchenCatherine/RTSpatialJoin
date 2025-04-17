#include "mymesh.h"
#include <chrono>
#include <vector>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;


bool compute_3d_spatial_intersection(std::string& off_file_path1, std::string& off_file_path2, int face_threshold, std::ofstream& outFile) 
{

     auto t0 = std::chrono::high_resolution_clock::now();
    Mymesh model1(off_file_path1);
    Mymesh model2(off_file_path2);

    if (model1.size_faces < face_threshold || model2.size_faces < face_threshold) return false;

    std::cout << off_file_path1 << std::endl;
    outFile << "Model1: " << off_file_path1 << std::endl;
    outFile << "Model2: " << off_file_path2 << std::endl;
    std::cout << "Model1 size: " << model1.size_faces << std::endl;
    std::cout << "Model2 size: " << model2.size_faces << std::endl;

    outFile << "Model1 size: " << model1.size_faces << std::endl;
    outFile << "Model2 size: " << model2.size_faces << std::endl;


    auto t1 = std::chrono::high_resolution_clock::now();
    model1.create_aabb_tree();
    model2.create_aabb_tree();
    auto t2 = std::chrono::high_resolution_clock::now();

    auto aabbtree1 = model1.get_aabb_tree();
    auto aabbtree2 = model2.get_aabb_tree();


    auto t3 = std::chrono::high_resolution_clock::now();
    bool is_collided = aabbtree1->do_intersect(*aabbtree2);
    auto t4 = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> duration;
    duration = t1 - t0;
    std::cout << "Model loading time: " << duration.count() << " seconds" << std::endl;
    outFile << "Model loading time: " << duration.count() << " seconds" << std::endl;
    
    duration = t2 - t1;
    std::cout << "CGAL building spatial indexing: " << duration.count() << " seconds" << std::endl;
    outFile << "CGAL building spatial indexing: " << duration.count() << " seconds" << std::endl;
    
    duration = t4 - t3;
    std::cout << "CGAL compute spatial intersection: " << duration.count() << " seconds" << std::endl;
    outFile << "CGAL compute spatial intersection: " << duration.count() << " seconds" << std::endl;
    

    std::cout << "\n*******************************************************\n" << std::endl;
    if (is_collided) {
        std::cout << "Result: two models are collided. \n" << std::endl;
        outFile << "Result: two models are collided. \n" << std::endl;
    }
    else {
        std::cout << "Result: two models are not collided. \n" << std::endl;
        outFile << "Result: two models are not collided. \n" << std::endl;
    }

    return true;

}



int main(int ac, char **av) {

    std::string body_path = std::string(av[1]);
    int face_threshold = std::stoi(av[2]);

    std::ofstream outFile("CGALoutput.txt");

    int count = 0;
    for (fs::directory_entry& organ_path : fs::directory_iterator(body_path)) 
    {
        std::string organ_name = organ_path.path().filename().string();
        // std::cout << organ_name << std::endl;   
        for (fs::directory_entry& AS : fs::directory_iterator(organ_path)) 
        {
            std::string file_path = AS.path().string();
            // self-self intersection
            if (compute_3d_spatial_intersection(file_path, file_path, face_threshold, outFile))
                count++;
        }
    }

    std::cout << count << " meshes that faces exceeds " << face_threshold << std::endl;
    outFile.close();

    return 0;
}