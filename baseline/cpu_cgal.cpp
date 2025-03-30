#include "mymesh.h"
#include <chrono>
#include <vector>

int main(int ac, char **av) {

    std::string off_file_path1 = std::string(av[1]);
    std::string off_file_path2 = std::string(av[2]);

    Mymesh model1(off_file_path1);
    Mymesh model2(off_file_path2);

    auto t1 = std::chrono::high_resolution_clock::now();
    model1.create_aabb_tree();
    model2.create_aabb_tree();
    auto t2 = std::chrono::high_resolution_clock::now();

    auto aabbtree1 = model1.get_aabb_tree();
    auto aabbtree2 = model2.get_aabb_tree();

    auto t3 = std::chrono::high_resolution_clock::now();
    bool is_collided = aabbtree1->do_intersect(*aabbtree2);
    auto t4 = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> duration = t2 - t1;
    std::cout << "CGAL building spatial indexing: " << duration.count() << " seconds" << std::endl;
    
    duration = t4 - t3;
    std::cout << "CGAL compute spatial ntersection: " << duration.count() << " seconds" << std::endl;
    

    std::cout << "\n*******************************************************\n" << std::endl;
    if (is_collided) std::cout << "Result: two models are collided. \n" << std::endl;
    else std::cout << "Result: two models are not collided. \n" << std::endl;

    return 0;
}