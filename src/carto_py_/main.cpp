#include <cartographer/common/configuration_file_resolver.h>
#include <cartographer/io/probability_grid_points_processor.h>
#include <cartographer/mapping/map_builder.h>
#include <cartographer/sensor/imu_data.h>
#include <Eigen/Core>
#include <nanobind/nanobind.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/string.h>

using namespace cartographer;

class SLAM {
private:
    double map_resolution_;
    int trajectory_id_;

    std::unique_ptr<mapping::ProbabilityGridRangeDataInserter2D> range_data_inserter_;
    std::unique_ptr<mapping::MapBuilderInterface> map_builder_;
    std::unique_ptr<mapping::ProbabilityGrid> probability_grid_;
    std::set<mapping::TrajectoryBuilderInterface::SensorId> sensors_;
    std::unique_ptr<common::LuaParameterDictionary> parameter_dictionary_;

    std::string lidar_sensor_name_;
    std::string imu_sensor_name_;

    std::mutex mtx_trajectory_builder_;

    Eigen::Vector3f last_pose;

    std::set<mapping::TrajectoryBuilderInterface::SensorId> sensors_ids_;

    bool start_ = false;

public:
    SLAM(const std::string& lidar_name, const std::string& imu_name) : lidar_sensor_name_(lidar_name), imu_sensor_name_(imu_name) {
        initialize();
    }

    void initialize() 
    {
        // INITIALIZATION PROCESS
        auto file_resolver = std::make_unique<common::ConfigurationFileResolver>(std::vector<std::string>{"cfg/"});
        const std::string code = file_resolver->GetFileContentOrDie("my_config.lua");
        parameter_dictionary_ = std::make_unique<common::LuaParameterDictionary>(code, std::move(file_resolver));

        auto range_data_inserter_options = mapping::CreateProbabilityGridRangeDataInserterOptions2D(
            parameter_dictionary_->GetDictionary("range_data_inserter").get()
        );
        range_data_inserter_ = std::make_unique<mapping::ProbabilityGridRangeDataInserter2D>(range_data_inserter_options);

        map_resolution_ = parameter_dictionary_->GetDouble("map_resolution");

        mapping::ValueConversionTables conversion_tables;
        probability_grid_ = std::make_unique<mapping::ProbabilityGrid>(io::CreateProbabilityGrid(map_resolution_, &conversion_tables));

        auto opts_map_builder = mapping::CreateMapBuilderOptions(parameter_dictionary_->GetDictionary("map_builder").get());
        map_builder_ = mapping::CreateMapBuilder(opts_map_builder);

        // SENSORS
        mapping::TrajectoryBuilderInterface::SensorId imu_sensor;
        imu_sensor.type = mapping::TrajectoryBuilderInterface::SensorId::SensorType::IMU,
        imu_sensor.id = imu_sensor_name_;
        sensors_.insert(imu_sensor);

        mapping::TrajectoryBuilderInterface::SensorId lidar_sensor;
        lidar_sensor.type = mapping::TrajectoryBuilderInterface::SensorId::SensorType::RANGE;
        lidar_sensor.id = lidar_sensor_name_;
        sensors_.insert(lidar_sensor);

        // TRAJECTORY BUILDER
        auto opts_trajectory = mapping::CreateTrajectoryBuilderOptions(parameter_dictionary_->GetDictionary("trajectory_builder").get());
        trajectory_id_ = map_builder_->AddTrajectoryBuilder(
            sensors_,
            opts_trajectory,
            [this](const int trajectory_id, 
                const common::Time& time, 
                const transform::Rigid3d& local_pose,
                sensor::RangeData range_data_in_local, 
                const std::unique_ptr<const mapping::TrajectoryBuilderInterface::InsertionResult> res
            ) {
                onSlamResultCallback(trajectory_id, time, local_pose, range_data_in_local);
            }
        );
    }

    void run() {
        start_ = true;

        std::thread t_mapping([this]() {
            while (start_) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }

            map_builder_->FinishTrajectory(trajectory_id_);    
        });
    }

    void stop() {
        start_ = false;
    }

    void onSlamResultCallback(
            const int trajectory_id, 
            const common::Time& time, 
            const transform::Rigid3d& local_pose,
            sensor::RangeData range_data_in_local
    ) {
        last_pose.x() = local_pose.translation().x();
        last_pose.y() = local_pose.translation().y();
        // last_pose.z() = local_pose.translation().z();
    }

    void addIMUData(
        double la_x, double la_y, double la_z, 
        double av_x, double av_y, double av_z
    ) {
        auto timestamp = GetCurrentCartographerTime();
        auto trajectory_builder = map_builder_->GetTrajectoryBuilder(trajectory_id_);
        
        sensor::ImuData data = {
            timestamp,
            Eigen::Vector3d(la_x, la_y, la_z),
            Eigen::Vector3d(av_x, av_y, av_z),
        };

        {
            std::lock_guard lock(mtx_trajectory_builder_);
            trajectory_builder->AddSensorData(imu_sensor_name_, data);
        }
    }

    void addLidarData(
        const std::vector<float> distances,
        const std::vector<float> angles
    ) {
        assert(distances.size() == angles.size());

        auto timestamp = GetCurrentCartographerTime();
        auto trajectory_builder = map_builder_->GetTrajectoryBuilder(trajectory_id_);

        sensor::TimedPointCloud ranges;

        auto points_batch = std::make_unique<io::PointsBatch>();
        points_batch->origin << last_pose.x(), last_pose.y(), 0.0;

        for (int point = 0; point < distances.size(); point++) {
            sensor::TimedRangefinderPoint p;

            float dist = distances[point];
            float ang = angles[point];

            p.position.x() = last_pose.x() + cosf(ang) * dist;
            p.position.y() = last_pose.y() + sinf(ang) * dist;
            p.position.z() = 0.0f;

            p.time = 0.0f;

            points_batch->points.push_back({ p.position });
            ranges.push_back(std::move(p));
        }

        sensor::TimedPointCloudData point_cloud {
            timestamp,
            last_pose,
            ranges,
            {}
        };

        {
            std::lock_guard lock(mtx_trajectory_builder_);
            trajectory_builder->AddSensorData(lidar_sensor_name_, point_cloud);
        }

        range_data_inserter_->Insert({
            points_batch->origin,
            sensor::PointCloud(points_batch->points),
            {}
        }, probability_grid_.get());
    }

    void doFinalOptimization() {
        map_builder_->pose_graph()->RunFinalOptimization();
    }

    common::Time GetCurrentCartographerTime() {
        auto now = std::chrono::system_clock::now();
        auto since_epoch = now.time_since_epoch();
        auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(since_epoch).count();

        int64_t ticks_from_unix_epoch = microseconds * 10;
        constexpr int64_t kUtsTicksOffset = cartographer::common::kUtsEpochOffsetFromUnixEpochInSeconds * 10000000LL;
        int64_t cartographer_ticks = ticks_from_unix_epoch + kUtsTicksOffset;

        return cartographer::common::FromUniversal(cartographer_ticks);
    }

    std::unique_ptr<io::Image> createImage() {
        std::cout << "creating image" << std::endl;

        Eigen::Array2i off{0, 0};
        std::unique_ptr<io::Image> image = io::DrawProbabilityGrid(*probability_grid_, &off);

        if (image != nullptr) {
            image->Rotate90DegreesClockwise(); // why?
        }

        return image;
    }

    void writePNG() {
        auto img = createImage();

        std::cout << "created image" << std::endl;

        io::FileWriterFactory fwf = [](const std::string& name) {
            std::cout << "creating image factory" << std::endl;

            std::string file_path = "./";
            return std::make_unique<io::StreamFileWriter>(file_path + name);
        };

        img->WritePng(fwf("map.png").get());

        std::cout << "wrote" << std::endl;
    }
};

using namespace nanobind::literals;

NB_MODULE(carto_py_, m) {
    nanobind::class_<SLAM>(m, "Carto")
        .def(nanobind::init<const std::string&, const std::string&>())
        .def("run", &SLAM::run)
        .def("stop", &SLAM::stop)
        .def("add_imu_data", &SLAM::addIMUData, 
            "lin_acc_x"_a, "lin_acc_y"_a, "lin_acc_z"_a,
            "ang_vel_x"_a, "ang_vel_y"_a, "ang_vel_z"_a
        )
        .def("add_lidar_data", &SLAM::addLidarData,
            "distances"_a,
            "angles"_a
        )
        .def("do_final_optimizations", &SLAM::doFinalOptimization)
        .def("write_png", &SLAM::writePNG);
}

// int main() {
//     auto slam = new SLAM(
//         "lidar",
//         "imu"
//     );

//     float distances[360];
//     float angles[360];

//     for (int i = 0; i < 360; i++) {
//         float ang = M_PI / 180.0 * i;

//         angles[i] = ang;
//         distances[i] = 2.0f + abs(sinf(ang / 2)) * 5.0f;
//     }

//     std::cout << "created angles arr" << std::endl;

//     for (int i = 0; i < 30; i++) {
//         slam->addLidarData(
//             slam->GetCurrentCartographerTime(),
//             distances,
//             angles,
//             360
//         );

//         slam->addIMUData(
//             slam->GetCurrentCartographerTime(),
//             0.0,
//             0.0,
//             0.0,
//             0.0,
//             0.0,
//             0.0
//         );

//         std::this_thread::sleep_for(std::chrono::milliseconds(50));
//     }

//     std::cout << "writing png";
//     slam->writePNG();
//     std::cout << "wrote";
 
//     slam->doFinalOptimization();

//     slam->stop();
//     t_mapping.join();

//     return 0;
// }
