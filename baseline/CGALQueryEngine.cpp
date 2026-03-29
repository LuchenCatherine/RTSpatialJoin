#include "CGALQueryEngine.h"

void CGALQueryEngine::load_query_model(std::string& query_path)
{
    query_model.load_from_off(query_path);
    query_model.create_aabb_tree();
}


void CGALQueryEngine::load_rendered_model(std::string& organ_path, int face_threshold)
{
    for (fs::directory_entry& AS : fs::directory_iterator(organ_path)) 
    {
        std::string file_path = AS.path().string();
        Mymesh model;
        if (model.load_from_off(file_path))
        {
            if (model.size_faces >= face_threshold) models.push_back(model);

        }
    }
}
        
void CGALQueryEngine::render()
{
    for (auto& model: models) model.create_aabb_tree();
}


void CGALQueryEngine::query()
{
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

}