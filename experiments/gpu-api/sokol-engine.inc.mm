// SPDX-License-Identifier: GPL-2.0-or-later
#define SOKOL_IMPL
#define SOKOL_METAL
#include <sokol_gfx.h>
struct SG final:Engine {
 id<MTLDevice> device;id<MTLCommandBuffer> last;CAMetalLayer* layer=nullptr;
 sg_image source{};sg_view sourceView{};std::array<sg_image,3> targets{};std::array<sg_view,3> views{};sg_shader shader{};sg_pipeline pipeline{};
 sg_buffer staging{};
 bool privateSource()const{return o.strategy=="source-private"||o.strategy=="upload-staging";}
 void copySource(){sg_write_buffer_desc w{};w.src.data={pixels.data(),pixels.size()};w.dst.buffer=staging;sg_write_buffer_transient(&w);sg_copy_buffer_to_image_desc c{};c.src.buffer=staging;c.src.bytes_per_row=o.size*4;c.src.bytes_per_slice=pixels.size();c.dst.image=source;c.size={o.size,o.size,1};sg_copy_buffer_to_image(&c);}
 SG(Options opt):Engine(opt){
  device=MTLCreateSystemDefaultDevice();require(device!=nil,"sokol Metal device");renderer="sokol_gfx Metal / "+std::string(device.name.UTF8String);
  if(o.surface()){layer=[CAMetalLayer layer];layer.device=device;layer.pixelFormat=MTLPixelFormatBGRA8Unorm;layer.framebufferOnly=NO;layer.contentsScale=1;layer.drawableSize=CGSizeMake(o.size,o.size);layer.displaySyncEnabled=o.present=="fifo";window.contentView.wantsLayer=YES;window.contentView.layer=layer;}
  sg_desc desc{};desc.environment.metal.device=(__bridge const void*)device;desc.environment.defaults.color_format=o.surface()?SG_PIXELFORMAT_BGRA8:SG_PIXELFORMAT_RGBA8;desc.environment.defaults.depth_format=SG_PIXELFORMAT_NONE;desc.environment.defaults.sample_count=1;
  desc.logger.func=[](const char*,uint32_t level,uint32_t,const char* message,uint32_t,const char*,void*){if(message)std::cerr<<"sokol: "<<message<<'\n';if(level==0)std::abort();};sg_setup(&desc);require(sg_isvalid(),"sokol setup");
  if(o.texture()){
   sg_image_desc d{};d.width=o.size;d.height=o.size;d.pixel_format=SG_PIXELFORMAT_RGBA8;d.usage.write_transient=o.upload()&&!privateSource();d.usage.copy_dst=privateSource();if(!o.upload()&&!privateSource())d.data.mip_levels[0]={pixels.data(),pixels.size()};source=sg_make_image(&d);require(sg_query_image_state(source)==SG_RESOURCESTATE_VALID,"sokol source");sg_view_desc v{};v.texture.image=source;sourceView=sg_make_view(&v);require(sg_query_view_state(sourceView)==SG_RESOURCESTATE_VALID,"sokol texture view");
   if(privateSource()){sg_buffer_desc b{};b.size=pixels.size();b.usage.staging_buffer=true;b.usage.write_transient=true;b.usage.copy_src=true;staging=sg_make_buffer(&b);require(sg_query_buffer_state(staging)==SG_RESOURCESTATE_VALID,"sokol staging");if(!o.upload()){copySource();last=_sg.mtl.cmd_buffer;sg_commit();wait();}}
  }
  if(!o.surface())for(int i=0;i<3;++i){sg_image_desc d{};d.width=o.size;d.height=o.size;d.pixel_format=SG_PIXELFORMAT_RGBA8;d.usage.color_attachment=true;targets[i]=sg_make_image(&d);sg_view_desc v{};v.color_attachment.image=targets[i];views[i]=sg_make_view(&v);require(sg_query_view_state(views[i])==SG_RESOURCESTATE_VALID,"sokol target");}
  const char* vertex="#include <metal_stdlib>\nusing namespace metal;struct V{float4 p [[position]];float2 uv;};vertex V vertexMain(uint i [[vertex_id]]){float2 p=float2((i<<1)&2,i&2);return {float4(p*2-1,0,1),float2(p.x,1-p.y)};}";
  const char* fragment=o.texture()?"#include <metal_stdlib>\nusing namespace metal;struct V{float4 p [[position]];float2 uv;};fragment float4 fragmentMain(V v [[stage_in]],texture2d<float> t [[texture(0)]]){constexpr sampler s(coord::normalized,address::clamp_to_edge,filter::nearest);return t.sample(s,v.uv);}":"#include <metal_stdlib>\nusing namespace metal;fragment float4 fragmentMain(){return float4(.25,.5,.75,1);}";
  sg_shader_desc sd{};sd.vertex_func.source=vertex;sd.vertex_func.entry="vertexMain";sd.fragment_func.source=fragment;sd.fragment_func.entry="fragmentMain";
  if(o.texture()){sd.views[0].texture.stage=SG_SHADERSTAGE_FRAGMENT;sd.views[0].texture.image_type=SG_IMAGETYPE_2D;sd.views[0].texture.sample_type=SG_IMAGESAMPLETYPE_FLOAT;sd.views[0].texture.msl_texture_n=0;}
  shader=sg_make_shader(&sd);require(sg_query_shader_state(shader)==SG_RESOURCESTATE_VALID,"sokol shader");sg_pipeline_desc pd{};pd.shader=shader;pd.depth.pixel_format=SG_PIXELFORMAT_NONE;pd.colors[0].pixel_format=desc.environment.defaults.color_format;pd.sample_count=1;pipeline=sg_make_pipeline(&pd);require(sg_query_pipeline_state(pipeline)==SG_RESOURCESTATE_VALID,"sokol pipeline");gpuTiming=o.timestamps;
 }
 ~SG(){wait();sg_shutdown();last=nil;}
 Sample frame(int index)override{
  Sample s;double t=now();if(o.upload()){if(privateSource())copySource();else{sg_write_image_desc d{};d.src.data={pixels.data(),pixels.size()};d.dst.image=source;sg_write_image_transient(&d);}}s.upload=now()-t;
  id<CAMetalDrawable> drawable=nil;if(layer){t=now();drawable=[layer nextDrawable];require(drawable!=nil,"sokol drawable");s.acquire=now()-t;}
  t=now();sg_pass pass{};pass.action.colors[0].load_action=SG_LOADACTION_CLEAR;pass.action.colors[0].store_action=SG_STOREACTION_STORE;pass.action.colors[0].clear_value={.125f,.125f,.125f,1.f};
  if(drawable){pass.swapchain.width=o.size;pass.swapchain.height=o.size;pass.swapchain.sample_count=1;pass.swapchain.color_format=SG_PIXELFORMAT_BGRA8;pass.swapchain.depth_format=SG_PIXELFORMAT_NONE;pass.swapchain.metal.current_drawable=(__bridge const void*)drawable;}else pass.attachments.colors[0]=views[index%3];
  sg_begin_pass(&pass);if(o.work!="clear"){sg_apply_pipeline(pipeline);sg_bindings b{};if(o.texture())b.views[0]=sourceView;sg_apply_bindings(&b);if(o.work=="draws")sg_apply_scissor_rect(0,0,1,1,true);if(o.strategy=="instanced")sg_draw(0,3,o.draws);else for(int i=0;i<o.draws;++i)sg_draw(0,3,1);}sg_end_pass();s.encode=now()-t;
  // Hold the exact submitted buffer, without adding a marker submission.
  last=_sg.mtl.cmd_buffer;require(last!=nil,"sokol frame command buffer");t=now();sg_commit();s.submit=now()-t;
  if(gpuTiming){t=now();wait();s.wait=now()-t;if(last.GPUEndTime>last.GPUStartTime)s.gpu=(last.GPUEndTime-last.GPUStartTime)*1e9;}
  return s;
 }
 void wait()override{if(last){[last waitUntilCompleted];require(last.status!=MTLCommandBufferStatusError,"sokol command failed");}}
 void reportExtra()override{std::cout<<",\"sokol_inflight_limit\":"<<SG_NUM_INFLIGHT_FRAMES<<",\"completion_observation\":\"exact internal Metal buffer held before sg_commit, waitUntilCompleted\"";if(o.texture()){auto info=sg_mtl_query_image_info(source);id<MTLTexture> texture=(__bridge id<MTLTexture>)info.tex[info.active_slot];std::cout<<",\"sokol_source_storage_mode\":"<<int(texture.storageMode);}if(layer)std::cout<<",\"actual_display_sync\":"<<(layer.displaySyncEnabled?"true":"false");}
 std::array<int,4> pixel()override{
  auto info=sg_mtl_query_image_info(targets[0]);id<MTLTexture> texture=(__bridge id<MTLTexture>)info.tex[info.active_slot];require(texture!=nil,"sokol native target");id<MTLBuffer> output=[device newBufferWithLength:256 options:MTLResourceStorageModeShared];id<MTLCommandQueue> q=[device newCommandQueue];id<MTLCommandBuffer> cb=[q commandBuffer];id<MTLBlitCommandEncoder> b=[cb blitCommandEncoder];[b copyFromTexture:texture sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0,0,0) sourceSize:MTLSizeMake(1,1,1) toBuffer:output destinationOffset:0 destinationBytesPerRow:256 destinationBytesPerImage:256];[b endEncoding];[cb commit];[cb waitUntilCompleted];require(cb.status!=MTLCommandBufferStatusError,"sokol readback error");auto p=static_cast<const uint8_t*>(output.contents);return {p[0],p[1],p[2],p[3]};
 }
};
