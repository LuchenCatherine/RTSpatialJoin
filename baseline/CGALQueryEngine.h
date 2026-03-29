#include "mymesh.h"

class CGALQueryEngine {
    public:
        CGALQueryEngine(){};
        
        void load_query_model(std::string& query_path);
        void load_rendered_model(std::string& organ_path, int face_threshold);
        
        void render();
        void query();
    
    private:
        std::vector<Mymesh> models;
        Mymesh query_model;
};

