#pragma once

#include "../Ref.h"

struct VkQueryPool_T;

namespace Vulkan {

	class Device;

	class QueryPool {
	public:
		enum class Type {
			Timer
		};
	
	private:

		std::reference_wrapper<Device> device;
		VkQueryPool_T* handle = nullptr;
		Type type;
		unsigned int number_of_queries;


	public:
		QueryPool(Device&, Type, int number_of_queries);
		~QueryPool();

		std::vector<double> get_time_values(); 

		VkQueryPool_T* get_handle_() const {
			return handle;
		}

		unsigned int get_number_of_queries() const { return number_of_queries; }

		QueryPool(const QueryPool&) = delete;
		QueryPool& operator=(const QueryPool&) = delete;
		QueryPool(QueryPool&&) = delete;
		QueryPool& operator=(QueryPool&&) = delete;
	};
}
