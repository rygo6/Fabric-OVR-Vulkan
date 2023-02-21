C:/VulkanSDK/1.3.236.0/Bin/glslc.exe shader_base.vert -o vert.spv
C:/VulkanSDK/1.3.236.0/Bin/glslc.exe shader_comp.vert -o vert_comp.spv
C:/VulkanSDK/1.3.236.0/Bin/glslc.exe shader_base.frag -o frag.spv
C:/VulkanSDK/1.3.236.0/Bin/glslc.exe shader_base_crasher.frag -o frag_crasher.spv
copy C:\Developer\Fabric-OVR-Vulkan\shaders\* C:\Developer\Fabric-OVR-Vulkan\cmake-build-debug\shaders\
pause