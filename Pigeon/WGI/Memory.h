#pragma once

namespace WGI {

	enum class MemoryTypePerference {
		Must,
		Prefered,
		PreferedNot,
		DontCare,
		MustNot
	};

	struct MemoryCriteria {
		MemoryTypePerference device_local = MemoryTypePerference::DontCare;
		MemoryTypePerference host_visible = MemoryTypePerference::Must;
		MemoryTypePerference host_coherent = MemoryTypePerference::Prefered;

		// Non-cached memory is best for sequential writes on CPU and reads on GPU
		// Cached memory is best for writes on GPU and reads on CPU
		MemoryTypePerference host_cached = MemoryTypePerference::PreferedNot;

		// from VkMemoryRequirements
		uint32_t memory_type_bits = 0;
		uint32_t alignment = 1;
		uint32_t size = 0;
	};


	struct MemoryRegion {
		unsigned int start = 0;
		unsigned int size = 0;

		MemoryRegion() {}

		MemoryRegion(unsigned int start_, unsigned int size_)
			: start(start_), size(size_) {}
	};
}