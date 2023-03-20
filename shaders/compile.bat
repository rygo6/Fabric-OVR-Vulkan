C:/VulkanSDK/1.3.236.0/Bin/glslc.exe shader_base.vert -o vert.spv
C:/VulkanSDK/1.3.236.0/Bin/glslc.exe shader_base.frag -o frag.spv
C:/VulkanSDK/1.3.236.0/Bin/glslc.exe shader_crasher.frag -o frag_crasher.spv

C:/VulkanSDK/1.3.236.0/Bin/glslc.exe node.vert -o node_vert.spv
C:/VulkanSDK/1.3.236.0/Bin/glslc.exe node.frag -o node_frag.spv
C:/VulkanSDK/1.3.236.0/Bin/glslc.exe node.tese -o node_tese.spv
C:/VulkanSDK/1.3.236.0/Bin/glslc.exe node.tesc -o node_tesc.spv

copy C:\Developer\Fabric-OVR-Vulkan\shaders\* C:\Developer\Fabric-OVR-Vulkan\cmake-build-debug\shaders\
pause