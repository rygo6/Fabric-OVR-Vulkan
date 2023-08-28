C:/VulkanSDK/1.3.261.0/Bin/glslc.exe shader_base.vert -o vert.spv
C:/VulkanSDK/1.3.261.0/Bin/glslc.exe shader_base.frag -o frag.spv
C:/VulkanSDK/1.3.261.0/Bin/glslc.exe shader_crasher.frag -o frag_crasher.spv
C:/VulkanSDK/1.3.261.0/Bin/glslc.exe node_tess.vert -o node_vert_tess.spv
C:/VulkanSDK/1.3.261.0/Bin/glslc.exe node_tess.frag -o node_frag_tess.spv
C:/VulkanSDK/1.3.261.0/Bin/glslc.exe node_tess.tese -o node_tese_tess.spv
C:/VulkanSDK/1.3.261.0/Bin/glslc.exe node_tess.tesc -o node_tesc_tess.spv
C:/VulkanSDK/1.3.261.0/Bin/glslc.exe composite.comp -o composite_comp.spv
C:/VulkanSDK/1.3.261.0/Bin/glslc.exe composite_depthoffset.comp -o composite_depthoffset_comp.spv
C:/VulkanSDK/1.3.261.0/Bin/glslc.exe node_mesh.task --target-spv=spv1.4 -o node_mesh_task.spv
C:/VulkanSDK/1.3.261.0/Bin/glslc.exe node_mesh.mesh --target-spv=spv1.4 -o node_mesh_mesh.spv
copy C:\Developer\Fabric-OVR-Vulkan\shaders\* C:\Developer\Fabric-OVR-Vulkan\cmake-build-debug\shaders\
pause