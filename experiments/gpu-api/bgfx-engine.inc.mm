// SPDX-License-Identifier: GPL-2.0-or-later
// Included by the standalone harness; raw Metal shaders use bgfx's pinned v12 container.
#include <bgfx/bgfx.h>
extern "C" int bgfxExperimentWait(uint32_t,double*,double*,double*,double*,double*);
extern "C" void* bgfxExperimentTexture(uint16_t);
extern "C" int bgfxExperimentPrivateSource(uint16_t);
extern "C" void bgfxExperimentReset();
struct BG final:Engine {
 bgfx::ProgramHandle program=BGFX_INVALID_HANDLE;bgfx::UniformHandle sampler=BGFX_INVALID_HANDLE;bgfx::TextureHandle source=BGFX_INVALID_HANDLE;
 std::array<bgfx::TextureHandle,3> targets;std::array<bgfx::FrameBufferHandle,3> fbos;
 CAMetalLayer* layer=nullptr;uint32_t targetFrame=0,completedFrame=0;bool threaded=false,recording=false;
 struct WaitInfo {double gpu=0,ready=0,wait=0,render=0,acquire=0;};WaitInfo latest;std::vector<WaitInfo> infos;
 static const bgfx::Memory* shader(char type,const std::string& code,bool texture=false){
  std::vector<uint8_t> bytes;auto append=[&](auto value){auto *p=reinterpret_cast<const uint8_t*>(&value);bytes.insert(bytes.end(),p,p+sizeof(value));};
  bytes.insert(bytes.end(),{uint8_t(type),'S','H',12});for(int i=0;i<4;++i)append(uint32_t(0));append(uint16_t(texture?1:0));
  if(texture){std::string name="s_tex";append(uint8_t(name.size()));bytes.insert(bytes.end(),name.begin(),name.end());append(uint8_t(0x10));append(uint8_t(1));append(uint16_t(0));append(uint16_t(1));append(uint8_t(0));append(uint8_t(2));append(uint16_t(0));}
  append(uint32_t(code.size()));bytes.insert(bytes.end(),code.begin(),code.end());append(uint8_t(0));return bgfx::copy(bytes.data(),uint32_t(bytes.size()));
 }
 BG(Options options):Engine(options){
  targets.fill(BGFX_INVALID_HANDLE);fbos.fill(BGFX_INVALID_HANDLE);threaded=o.api=="bgfx-threaded";
  if(!threaded)bgfx::renderFrame();
  bgfx::Init init;init.type=bgfx::RendererType::Metal;init.fallback=false;init.debug=false;init.profile=false;
  init.swapChain.width=o.surface()?o.size:0;init.swapChain.height=o.surface()?o.size:0;init.swapChain.formatColor=bgfx::TextureFormat::RGBA8;init.swapChain.formatDepthStencil=bgfx::TextureFormat::Count;init.swapChain.maxFrameLatency=3;
  if(o.surface()){
   layer=[CAMetalLayer layer];layer.contentsScale=1;layer.drawableSize=CGSizeMake(o.size,o.size);window.contentView.wantsLayer=YES;window.contentView.layer=layer;
   init.swapChain.nwh=(__bridge void*)layer;init.swapChain.formatColor=bgfx::TextureFormat::BGRA8;
  }
  init.reset=o.present=="fifo"?BGFX_RESET_VSYNC:BGFX_RESET_NONE;
  if(o.strategy=="flip-after-render")init.reset|=BGFX_RESET_FLIP_AFTER_RENDER;
  require(bgfx::init(init),"bgfx init");require(bgfx::getRendererType()==bgfx::RendererType::Metal,"bgfx fallback occurred");renderer="bgfx Metal / "+std::string(MTLCreateSystemDefaultDevice().name.UTF8String);
  const std::string vertex="#include <metal_stdlib>\nusing namespace metal;struct V{float4 p [[position]];float2 uv;};vertex V xlatMtlMain(uint i [[vertex_id]]){float2 p=float2((i<<1)&2,i&2);return {float4(p*2-1,0,1),float2(p.x,1-p.y)};}";
  const std::string fragment=o.texture()?"#include <metal_stdlib>\nusing namespace metal;struct V{float4 p [[position]];float2 uv;};fragment float4 xlatMtlMain(V v [[stage_in]],texture2d<float> s_tex [[texture(0)]]){constexpr sampler s(coord::normalized,address::clamp_to_edge,filter::nearest);return s_tex.sample(s,v.uv);}":"#include <metal_stdlib>\nusing namespace metal;fragment float4 xlatMtlMain(){return float4(.25,.5,.75,1);}";
  auto vs=bgfx::createShader(shader('V',vertex));auto fs=bgfx::createShader(shader('F',fragment,o.texture()));require(bgfx::isValid(vs)&&bgfx::isValid(fs),"bgfx shader container");program=bgfx::createProgram(vs,fs,true);require(bgfx::isValid(program),"bgfx program");
  const uint64_t flags=BGFX_SAMPLER_U_CLAMP|BGFX_SAMPLER_V_CLAMP|BGFX_SAMPLER_MIN_POINT|BGFX_SAMPLER_MAG_POINT|BGFX_SAMPLER_MIP_POINT;
  if(o.texture()){sampler=bgfx::createUniform("s_tex",bgfx::UniformType::Sampler);source=bgfx::createTexture2D(o.size,o.size,false,1,bgfx::TextureFormat::RGBA8,flags,o.upload()?nullptr:bgfx::copy(pixels.data(),uint32_t(pixels.size())));}
  if(!o.surface())for(int i=0;i<3;++i){targets[i]=bgfx::createTexture2D(o.size,o.size,false,1,bgfx::TextureFormat::RGBA8,BGFX_TEXTURE_RT|flags|(o.strategy=="rt-private"?BGFX_TEXTURE_RT_WRITE_ONLY:0));fbos[i]=bgfx::createFrameBuffer(1,&targets[i],false);require(bgfx::isValid(fbos[i]),"bgfx target");}
  bgfx::setViewMode(0,bgfx::ViewMode::Sequential);bgfx::setViewClear(0,BGFX_CLEAR_COLOR,0x202020ff);gpuTiming=o.timestamps;
  frame(0);wait();
  if(o.strategy=="source-private"){require(o.work=="texture","private source control uses static texture");require(bgfxExperimentPrivateSource(source.idx),"bgfx private source allocation");}
 }
 ~BG(){recording=false;wait();for(auto f:fbos)if(bgfx::isValid(f))bgfx::destroy(f);for(auto t:targets)if(bgfx::isValid(t))bgfx::destroy(t);if(bgfx::isValid(source))bgfx::destroy(source);if(bgfx::isValid(sampler))bgfx::destroy(sampler);bgfx::destroy(program);bgfx::shutdown();bgfxExperimentReset();}
 Sample frame(int index)override{
  Sample s;double t=now();if(o.upload()){auto mem=o.strategy=="upload-ref"?bgfx::makeRef(pixels.data(),uint32_t(pixels.size())):bgfx::copy(pixels.data(),uint32_t(pixels.size()));bgfx::updateTexture2D(source,0,0,0,0,o.size,o.size,mem,o.size*4);}s.upload=now()-t;
  t=now();bgfx::setViewFrameBuffer(0,o.surface()?bgfx::FrameBufferHandle BGFX_INVALID_HANDLE:fbos[index%3]);bgfx::setViewRect(0,0,0,o.size,o.size);
  if(o.work=="clear")bgfx::touch(0);
  else{
   int count=o.strategy=="instanced"?1:o.draws;
   for(int i=0;i<count;++i){bgfx::setVertexCount(3);if(o.strategy=="instanced")bgfx::setInstanceCount(o.draws);if(o.texture())bgfx::setTexture(0,sampler,source);if(o.work=="draws")bgfx::setScissor(0,0,1,1);bgfx::setState(BGFX_STATE_WRITE_RGB|BGFX_STATE_WRITE_A);bgfx::submit(0,program);}
  }
  s.encode=now()-t;t=now();targetFrame=bgfx::frame(o.surface()?BGFX_FRAME_NONE:BGFX_FRAME_FLUSH);s.submit=now()-t;
  if(gpuTiming){t=now();wait();s.wait=now()-t;if(latest.gpu>0)s.gpu=latest.gpu;}
  return s;
 }
 void wait()override{
  if(targetFrame==completedFrame)return;
  require(bgfxExperimentWait(targetFrame,&latest.gpu,&latest.ready,&latest.wait,&latest.render,&latest.acquire),"bgfx observed GPU completion failed");completedFrame=targetFrame;if(recording)infos.push_back(latest);
 }
 void beginMeasurement()override{infos.clear();recording=true;}
 void endMeasurement()override{recording=false;}
 void reportExtra()override{
  if(o.work!="clear")require(bgfx::getStats()->numPrims[bgfx::Topology::TriList]==uint32_t(o.draws),"bgfx rendered triangle count mismatch");
  std::cout<<",\"bgfx_threaded\":"<<(threaded?"true":"false")<<",\"completion_observation\":\"observed Metal frame number, CPU commit publication and waitUntilCompleted\",\"bgfx_metrics_ms\":{";
  bool first=true;for(int field=0;field<5;++field){std::vector<double> values;for(auto s:infos)values.push_back(field==0?s.ready:field==1?s.wait:field==2?s.render:field==3?s.acquire:s.gpu);if(!values.empty())distribution(field==0?"render_ready_wait":field==1?"gpu_completion_wait":field==2?"render_wall":field==3?"drawable_acquire":"gpu_command_buffer",values,first);}std::cout<<'}';
  if(!o.surface()){id<MTLTexture> target=(__bridge id<MTLTexture>)bgfxExperimentTexture(targets[0].idx);std::cout<<",\"bgfx_target_storage_mode\":"<<int(target.storageMode);}
  if(o.texture()){id<MTLTexture> texture=(__bridge id<MTLTexture>)bgfxExperimentTexture(source.idx);std::cout<<",\"bgfx_source_storage_mode\":"<<int(texture.storageMode);}
  std::cout<<",\"bgfx_num_draw_calls\":"<<bgfx::getStats()->numDraw<<",\"bgfx_num_triangles\":"<<bgfx::getStats()->numPrims[bgfx::Topology::TriList];if(layer)std::cout<<",\"actual_display_sync\":"<<(layer.displaySyncEnabled?"true":"false");
 }
 std::array<int,4> pixel()override{
  id<MTLTexture> texture=(__bridge id<MTLTexture>)bgfxExperimentTexture(targets[0].idx);require(texture!=nil,"bgfx native texture");
  id<MTLBuffer> output=[texture.device newBufferWithLength:256 options:MTLResourceStorageModeShared];id<MTLCommandQueue> q=[texture.device newCommandQueue];id<MTLCommandBuffer> cb=[q commandBuffer];id<MTLBlitCommandEncoder> b=[cb blitCommandEncoder];[b copyFromTexture:texture sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0,0,0) sourceSize:MTLSizeMake(1,1,1) toBuffer:output destinationOffset:0 destinationBytesPerRow:256 destinationBytesPerImage:256];[b endEncoding];[cb commit];[cb waitUntilCompleted];require(cb.status!=MTLCommandBufferStatusError,"bgfx readback error");auto p=static_cast<const uint8_t*>(output.contents);return {p[0],p[1],p[2],p[3]};
 }
};
