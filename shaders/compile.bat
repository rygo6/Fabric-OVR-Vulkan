C:/VulkanSDK/1.3.236.0/Bin/glslc.exe shader_base.vert -o vert.spv
C:/VulkanSDK/1.3.236.0/Bin/glslc.exe shader_comp.vert -o vert_comp.spv
C:/VulkanSDK/1.3.236.0/Bin/glslc.exe shader_base.frag -o frag.spv
C:/VulkanSDK/1.3.236.0/Bin/glslc.exe shader_crasher.frag -o frag_crasher.spv

C:/VulkanSDK/1.3.236.0/Bin/glslc.exe node_comp.vert -o node_comp_vert.spv
C:/VulkanSDK/1.3.236.0/Bin/glslc.exe node_comp.frag -o node_comp_frag.spv
C:/VulkanSDK/1.3.236.0/Bin/glslc.exe node_comp.tese -o node_comp_tess.spv
C:/VulkanSDK/1.3.236.0/Bin/glslc.exe node_comp.tesc -o node_comp_tesc.spv

copy C:\Developer\Fabric-OVR-Vulkan\shaders\* C:\Developer\Fabric-OVR-Vulkan\cmake-build-debug\shaders\
pause