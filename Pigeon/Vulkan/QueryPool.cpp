#include "QueryPool.h"
#include "Device.h"
#include "../BetterAssert.h"
#include "Include.h"
#include "CommandPool.h"

using namespace std;

namespace Vulkan {

    QueryPool::QueryPool(Device& d, Type type_, int number_of_queries_)
		:device(d), type(type_), number_of_queries(number_of_queries_)
    {

        VkQueryPoolCreateInfo create_info {VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};

        assert_(type == Type::Timer);
        create_info.queryType = VK_QUERY_TYPE_TIMESTAMP;

        create_info.queryCount = number_of_queries;

		assert__(vkCreateQueryPool(device.get().get_device_handle_(), &create_info, nullptr, &handle) == VK_SUCCESS,
			"vkCreateQueryPool error");
    }

    QueryPool::~QueryPool()
    {
        vkDestroyQueryPool(device.get().get_device_handle_(), handle, nullptr);
        handle = nullptr;
    }

    vector<double> QueryPool::get_time_values()
    {
        vector<uint64_t> int_data;
        int_data.resize(number_of_queries);

        assert__(vkGetQueryPoolResults(device.get().get_device_handle_(), handle, 0,
        number_of_queries, number_of_queries*8, int_data.data(), 8, VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT) == VK_SUCCESS,
			"vkGetQueryPoolResults error");

        vector<double> float_data;
        for(uint64_t x : int_data) {
            float_data.push_back(static_cast<double>(x) * device.get().get_timer_multiplier());
        }
        return float_data;
    }

}