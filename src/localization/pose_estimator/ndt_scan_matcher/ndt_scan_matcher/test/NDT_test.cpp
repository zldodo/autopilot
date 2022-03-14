#include "ndt_scan_matcher/ndt_scan_matcher_core.hpp"

#include <gtest/gtest.h>
#include <rclcpp/rclcpp.hpp>

TEST(test_ndt_scan_matcher, getNDT)
{
    std::shared_ptr<NDTBase> ndt_ptr = 
    getNDT<PointSource, PointTarget>(NDTImplementType::PCL_GENERIC);
    bool res;
    if(ndt_ptr)
    {
        res = true;
    }
    else
    {
        res = false;
    }
    EXPECT_TRUE(res);

}
TEST(test_ndt_scan_matcher, initNDTScanMatcher)
{
    size_t counts = 10;
    bool res =true;
    for(size_t i=0;i<counts;i++)
    {
        std::cout<<"start to initNDTScanMatcher"<<std::endl;
        // std::shared_ptr<NDTScanMatcher> ndt_scan_matcher;
        auto ndt_scan_matcher = std::make_shared<NDTScanMatcher>();
        if(!ndt_scan_matcher)
        {
            res = false;
            std::cout<<"initNDTScanMatcher failed at " << i << "th round"<<std::endl;
            break;
        }
    }
    EXPECT_TRUE(res);
}



// int main(int argc, char** argv) {

    


// }