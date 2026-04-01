include "map_builder.lua"
include "trajectory_builder.lua"

options = {
    map_builder = MAP_BUILDER,
    trajectory_builder = TRAJECTORY_BUILDER,
    range_data_inserter = {
        insert_free_space = true,
        hit_probability = 0.65,
        miss_probability = 0.35,
    },
    map_resolution = 0.05
}

MAP_BUILDER.use_trajectory_builder_2d = true
MAP_BUILDER.use_trajectory_builder_3d = false
MAP_BUILDER.num_background_threads = 5

return options