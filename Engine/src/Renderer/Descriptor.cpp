#include "Descriptor.h"

#include <ranges>

// *************** Descriptor Set Layout Builder *********************

DescriptorSetLayout::Builder &DescriptorSetLayout::Builder::AddBinding(
	uint32_t binding,
	VkDescriptorType descriptorType,
	VkShaderStageFlags stageFlags,
	uint32_t count)
{
	assert(!m_Bindings.contains(binding) && "Binding already in use");

	VkDescriptorSetLayoutBinding layoutBinding{};
	layoutBinding.binding = binding;
	layoutBinding.descriptorType = descriptorType;
	layoutBinding.descriptorCount = count;
	layoutBinding.stageFlags = stageFlags;
	m_Bindings[binding] = layoutBinding;
	return *this;
}

std::unique_ptr<DescriptorSetLayout> DescriptorSetLayout::Builder::Build() const
{
	return std::make_unique<DescriptorSetLayout>(*m_Device, m_Bindings);
}

// *************** Descriptor Set Layout *********************

DescriptorSetLayout::DescriptorSetLayout(Device &device, const std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding>& bindings)
	: m_Device{device}, m_Bindings{bindings}
{
	std::vector<VkDescriptorSetLayoutBinding> layoutBindings{};
	for (auto val : bindings | std::views::values)
	{
		layoutBindings.push_back(val);
	}

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
	descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
	descriptorSetLayoutInfo.pBindings = layoutBindings.data();

	if (vkCreateDescriptorSetLayout(
			device.GetLogicalDevice(),
			&descriptorSetLayoutInfo,
			nullptr,
			&m_DescriptorSetLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

DescriptorSetLayout::~DescriptorSetLayout()
{
	vkDestroyDescriptorSetLayout(m_Device.GetLogicalDevice(), m_DescriptorSetLayout, nullptr);
}

// *************** Descriptor Pool Builder *********************

DescriptorPool::Builder &DescriptorPool::Builder::AddPoolSize(const VkDescriptorType descriptorType, const uint32_t count)
{
	m_PoolSizes.push_back({descriptorType, count});
	return *this;
}

DescriptorPool::Builder &DescriptorPool::Builder::SetPoolFlags(
    const VkDescriptorPoolCreateFlags flags)
{
	m_PoolFlags = flags;
	return *this;
}
DescriptorPool::Builder &DescriptorPool::Builder::SetMaxSets(uint32_t count)
{
	m_MaxSets = count;
	return *this;
}

std::unique_ptr<DescriptorPool> DescriptorPool::Builder::Build() const
{
	return std::make_unique<DescriptorPool>(m_Device, m_MaxSets, m_PoolFlags, m_PoolSizes);
}

// *************** Descriptor Pool *********************

DescriptorPool::DescriptorPool(
    const std::shared_ptr<Device>& device,
    const uint32_t maxSets,
    const VkDescriptorPoolCreateFlags poolFlags,
    const std::vector<VkDescriptorPoolSize> &poolSizes)
    : m_Device{device}
{
	VkDescriptorPoolCreateInfo descriptorPoolInfo{};
	descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	descriptorPoolInfo.pPoolSizes = poolSizes.data();
	descriptorPoolInfo.maxSets = maxSets;
	descriptorPoolInfo.flags = poolFlags;

	if (vkCreateDescriptorPool(m_Device->GetLogicalDevice(), &descriptorPoolInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

DescriptorPool::~DescriptorPool()
{
  vkDestroyDescriptorPool(m_Device->GetLogicalDevice(), m_DescriptorPool, nullptr);
}

bool DescriptorPool::AllocateDescriptor(
    const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet &descriptor) const
{
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_DescriptorPool;
	allocInfo.pSetLayouts = &descriptorSetLayout;
	allocInfo.descriptorSetCount = 1;

	// Might want to create a "DescriptorPoolManager" class that handles this case, and builds
	// a new pool whenever an old pool fills up. But this is beyond our current scope
	if (vkAllocateDescriptorSets(m_Device->GetLogicalDevice(), &allocInfo, &descriptor) != VK_SUCCESS)
	{
		return false;
	}
	return true;
}

void DescriptorPool::FreeDescriptors(const std::vector<VkDescriptorSet>& descriptors) const
{
	vkFreeDescriptorSets(
	  m_Device->GetLogicalDevice(),
	  m_DescriptorPool,
	  static_cast<uint32_t>(descriptors.size()),
	  descriptors.data());
}

void DescriptorPool::ResetPool() const
{
	vkResetDescriptorPool(m_Device->GetLogicalDevice(), m_DescriptorPool, 0);
}

// *************** Descriptor Writer *********************

DescriptorWriter::DescriptorWriter(DescriptorSetLayout &setLayout, DescriptorPool &pool)
    : m_SetLayout{setLayout}, m_Pool{pool} {}

DescriptorWriter &DescriptorWriter::WriteBuffer(
    const uint32_t binding, const VkDescriptorBufferInfo *bufferInfo)
{
	assert(m_SetLayout.m_Bindings.count(binding) == 1 && "Layout does not contain specified binding");

	const auto &bindingDescription = m_SetLayout.m_Bindings[binding];

	assert(
	  bindingDescription.descriptorCount == 1 &&
	  "Binding single descriptor info, but binding expects multiple");

	VkWriteDescriptorSet write{};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.descriptorType = bindingDescription.descriptorType;
	write.dstBinding = binding;
	write.pBufferInfo = bufferInfo;
	write.descriptorCount = 1;

	m_Writes.push_back(write);
	return *this;
}

DescriptorWriter& DescriptorWriter::WriteImage(const uint32_t binding, const VkDescriptorImageInfo *imageInfo)
{
	assert(m_SetLayout.m_Bindings.count(binding) == 1 && "Layout does not contain specified binding");

	const auto &bindingDescription = m_SetLayout.m_Bindings[binding];

	assert(
	  bindingDescription.descriptorCount == 1 &&
	  "Binding single descriptor info, but binding expects multiple");

	VkWriteDescriptorSet write{};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.descriptorType = bindingDescription.descriptorType;
	write.dstBinding = binding;
	write.pImageInfo = imageInfo;
	write.descriptorCount = 1;

	m_Writes.push_back(write);
	return *this;
}

bool DescriptorWriter::Build(VkDescriptorSet &set)
{
	if (m_Pool.AllocateDescriptor(m_SetLayout.GetDescriptorSetLayout(), set) == false)
	{
		return false;
	}
	Overwrite(set);
	return true;
}

void DescriptorWriter::Overwrite(const VkDescriptorSet& set)
{
	for (auto &write : m_Writes)
	{
		write.dstSet = set;
	}
	vkUpdateDescriptorSets(
		m_Pool.m_Device->GetLogicalDevice(),
		m_Writes.size(),
		m_Writes.data(),
		0,
		nullptr);
}
