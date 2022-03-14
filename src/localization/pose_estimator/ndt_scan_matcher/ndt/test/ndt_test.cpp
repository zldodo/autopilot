
#include<ndt/omp.hpp>
#include<ndt/pcl_generic.hpp>
#include<ndt/pcl_modified.hpp>
// #include<pclomp/ndt_omp.h>
#include <gtest/gtest.h>



using PointSource = pcl::PointXYZ;
using PointTarget = pcl::PointXYZ;

typedef NormalDistributionsTransformBase<PointSource, PointTarget> NDTBase;
typedef NormalDistributionsTransformPCLGeneric<PointSource, PointTarget> NDTPCLGeneric;
typedef NormalDistributionsTransformPCLModified<PointSource, PointTarget> NDTPCLModified;
typedef NormalDistributionsTransformOMP<PointSource, PointTarget> NDTOMP;
// typedef pclomp::NormalDistributionsTransform<PointSource, PointTarget> NDTOrin;

// TEST(NDTTest, initialize_NDTBase)
// {
//     std::shared_ptr<NDTBase> ndt_base_ptr(new NDTBase);
//     ASSERT_TRUE(ndt_base_ptr!=nullptr);
// }
TEST(NDTTest, initialize_NDTOMP)
{
    std::shared_ptr<NDTBase> ndt_base_ptr(new NDTOMP);
    // std::shared_ptr<NDTOrin> ndt_base_ptr(new NDTOrin);
    // pclomp::NormalDistributionsTransform<pcl::PointXYZ, pcl::PointXYZ>::Ptr ndt_omp(new pclomp::NormalDistributionsTransform<pcl::PointXYZ, pcl::PointXYZ>());
    EXPECT_TRUE(ndt_base_ptr);
}
// TEST(NDTTest, initialize_NDTPCLGeneric)
// {
//     std::shared_ptr<NDTBase> ndt_base_ptr(new NDTPCLGeneric);
//     EXPECT_TRUE(ndt_base_ptr!=nullptr);
// }
// TEST(NDTTest, initialize_NDTPCLModified)
// {
//     std::shared_ptr<NDTBase> ndt_base_ptr(new NDTPCLModified);
//     EXPECT_TRUE(ndt_base_ptr!=nullptr);
// }

int main(int argc, char * argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}