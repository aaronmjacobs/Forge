#include "Renderer/Frame.h"

#include "Graphics/DebugUtils.h"

#if FORGE_WITH_MIDI
#  include "Platform/Midi.h"
#endif // FORGE_WITH_MIDI

// static
std::vector<vk::DescriptorSetLayoutBinding> FrameDescriptorSet::getBindings()
{
   return
   {
      vk::DescriptorSetLayoutBinding()
         .setBinding(0)
         .setDescriptorType(vk::DescriptorType::eUniformBuffer)
         .setDescriptorCount(1)
         .setStageFlags(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
   };
}

Frame::Frame(const GraphicsContext& graphicsContext, DynamicDescriptorPool& dynamicDescriptorPool)
   : GraphicsResource(graphicsContext)
   , uniformBuffer(graphicsContext)
   , descriptorSet(graphicsContext, dynamicDescriptorPool)
{
   NAME_CHILD(uniformBuffer, "");
   NAME_CHILD(descriptorSet, "");

   updateDescriptorSets();
}

void Frame::update(uint32_t frameNumber, float time)
{
   FrameUniformData frameUniformData;

#if FORGE_WITH_MIDI
   const Kontroller::State& midiState = Midi::getState();

   frameUniformData.midiA = glm::vec4(midiState.groups[0].slider, midiState.groups[1].slider, midiState.groups[2].slider, midiState.groups[3].slider);
   frameUniformData.midiB = glm::vec4(midiState.groups[4].slider, midiState.groups[5].slider, midiState.groups[6].slider, midiState.groups[7].slider);
#endif // FORGE_WITH_MIDI

   frameUniformData.number = frameNumber;
   frameUniformData.time = time;

   uniformBuffer.update(frameUniformData);
}

void Frame::updateDescriptorSets()
{
   for (uint32_t frameIndex = 0; frameIndex < GraphicsContext::kMaxFramesInFlight; ++frameIndex)
   {
      vk::DescriptorBufferInfo viewBufferInfo = getDescriptorBufferInfo(frameIndex);

      vk::WriteDescriptorSet viewDescriptorWrite = vk::WriteDescriptorSet()
         .setDstSet(descriptorSet.getSet(frameIndex))
         .setDstBinding(0)
         .setDstArrayElement(0)
         .setDescriptorType(vk::DescriptorType::eUniformBuffer)
         .setDescriptorCount(1)
         .setPBufferInfo(&viewBufferInfo);

      device.updateDescriptorSets({ viewDescriptorWrite }, {});
   }
}

