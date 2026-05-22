# RTSpatialJoin
3D spatial join powered by NVIDIA ray-tracing cores.

# Run code
./main ../models/temp_plain_model_off/https___cdn.humanatlas.io_digital-objects_ref-organ_brain-male_v1.4_assets_3d-allen-m-brain/ ../models/temp_plain_model_off/https___cdn.humanatlas.io_digital-objects_ref-organ_brain-male_v1.4_assets_3d-allen-m-brain/Allen_amygdalohippocampal_area_L.off 1024 0


# takeaway
1. test whether only edges of mesh1 intersected with BVH of mesh2, running time does not change, which means reducing the rays will not impact the running time as all the ray-mesh intersections are running in parallel. 

So first, we can build IAS and GAS on background meshes then test edges of the query mesh with the accelerating structures of the background objects; second, we will merge all the edges of background objects together to test them with the GAS of the query mesh in parallel. 

Write the paper for sure. 


buffer1 is the background models. 
buffer2 is query model


./mainPaper ../models/allen-m-brain/ ../models/allen-m-brain/Allen_white_matter_of_forebrain_L.off 1024 0
./cpu_baseline ../models/allen-m-brain/ ../models/allen-m-brain/Allen_white_matter_of_forebrain_L.off 1024 0