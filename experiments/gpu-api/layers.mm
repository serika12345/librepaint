// SPDX-License-Identifier: GPL-2.0-or-later
// Standalone normal, premultiplied RGBA8 layer composition experiment.
#define main single_api_experiment_main
#include "benchmark.mm"
#undef main

struct LayerData {
 int size,count;std::vector<uint8_t> data,cache,result;std::vector<float> working;const uint8_t *top=nullptr;
 LayerData(int s,int n):size(s),count(n),data(size_t(s)*s*n*4),cache(size_t(s)*s*4),result(cache.size()),working(size_t(s)*s*4) {
  for(int l=0;l<n;++l)for(int y=0;y<s;++y)for(int x=0;x<s;++x){auto* p=&data[(size_t(l)*s*s+size_t(y)*s+x)*4];int a=32+l*19%96;p[0]=((x*3+l*23)%256)*a/255;p[1]=((y*5+l*41)%256)*a/255;p[2]=((x+y+l*37)%256)*a/255;p[3]=a;}
  top=data.data();compose(cache,0,s,1,false);compose(result,0,s,0,false);
 }
 void update(int width,int frame){for(int y=0;y<width;++y)for(int x=0;x<width;++x){auto* p=&data[(size_t(y)*size+x)*4];p[0]=((x*3+frame*7)%256)*p[3]/255;p[2]=((x+y+frame*11)%256)*p[3]/255;}}
 void compose(std::vector<uint8_t>& out,int start,int width,int first,bool cached){
  std::fill_n(working.begin(),size_t(width)*width*4,0.f);
  if(cached)for(int y=0;y<width;++y)for(int x=0;x<width*4;++x)working[size_t(y)*width*4+x]=cache[(size_t(start+y)*size+start)*4+x]*(1.f/255.f);
  for(int l=cached?0:count-1;l>=first;--l){float inv=1-data[size_t(l)*size*size*4+3]*(1.f/255.f);
   for(int y=0;y<width;++y){const auto* pixels=(l==0?top:data.data()+size_t(l)*size*size*4)+(size_t(start+y)*size+start)*4;auto* row=working.data()+size_t(y)*width*4;
    for(int x=0;x<width*4;++x)row[x]=pixels[x]*(1.f/255.f)+row[x]*inv;
   }
  }
  for(int y=0;y<width;++y)for(int x=0;x<width*4;++x)out[(size_t(start+y)*size+start)*4+x]=uint8_t(working[size_t(y)*width*4+x]*255+.5f);
 }
};

struct LayerGPU {
 WG &r;LayerData &d;int layerCount;bool partial;WGPUTexture input=nullptr;WGPUComputePipeline pipeline=nullptr;WGPUBindGroup group=nullptr;
 LayerGPU(WG &renderer,LayerData &data,bool cached,bool dirty):r(renderer),d(data),layerCount(cached?2:data.count),partial(dirty){
  WGPUTextureDescriptor td{};td.size={uint32_t(d.size),uint32_t(d.size),uint32_t(layerCount)};td.dimension=WGPUTextureDimension_2D;td.format=WGPUTextureFormat_RGBA8Unorm;td.sampleCount=td.mipLevelCount=1;td.usage=WGPUTextureUsage_TextureBinding|WGPUTextureUsage_CopyDst;input=wgpuDeviceCreateTexture(r.device,&td);
  for(int l=0;l<layerCount;++l){WGPUTexelCopyTextureInfo dest{};dest.texture=input;dest.origin.z=l;WGPUTexelCopyBufferLayout layout{};layout.bytesPerRow=d.size*4;layout.rowsPerImage=d.size;WGPUExtent3D extent{uint32_t(d.size),uint32_t(d.size),1};const uint8_t* bytes=cached&&l==1?d.cache.data():d.data.data()+size_t(l)*d.size*d.size*4;wgpuQueueWriteTexture(r.queue,&dest,bytes,d.result.size(),&layout,&extent);}
  td.size.depthOrArrayLayers=1;td.usage=WGPUTextureUsage_StorageBinding|WGPUTextureUsage_TextureBinding|WGPUTextureUsage_CopySrc;WGPUTexture output=wgpuDeviceCreateTexture(r.device,&td);
  WGPUTextureView outputView=wgpuTextureCreateView(output,nullptr);WGPUBindGroupLayout renderLayout=wgpuRenderPipelineGetBindGroupLayout(r.pipeline,0);WGPUSamplerDescriptor sd{};sd.addressModeU=sd.addressModeV=sd.addressModeW=WGPUAddressMode_ClampToEdge;sd.magFilter=sd.minFilter=WGPUFilterMode_Nearest;sd.mipmapFilter=WGPUMipmapFilterMode_Nearest;sd.maxAnisotropy=1;WGPUSampler sampler=wgpuDeviceCreateSampler(r.device,&sd);
  WGPUBindGroupEntry renderEntries[2]{};renderEntries[0].binding=0;renderEntries[0].textureView=outputView;renderEntries[1].binding=1;renderEntries[1].sampler=sampler;WGPUBindGroupDescriptor renderDesc{};renderDesc.layout=renderLayout;renderDesc.entryCount=2;renderDesc.entries=renderEntries;WGPUBindGroup newGroup=wgpuDeviceCreateBindGroup(r.device,&renderDesc);wgpuBindGroupRelease(r.group);r.group=newGroup;wgpuTextureRelease(r.source);r.source=output;
  wgpuBindGroupLayoutRelease(renderLayout);wgpuSamplerRelease(sampler);
  const char *code=R"WGSL(
@group(0) @binding(0) var layers:texture_2d_array<f32>;
@group(0) @binding(1) var result:texture_storage_2d<rgba8unorm,write>;
@compute @workgroup_size(8,8) fn compose(@builtin(global_invocation_id) id:vec3u) {
 let xy=vec2i(id.xy);if(any(id.xy>=textureDimensions(result))){return;}
 var c=vec4f(0.);var l=i32(textureNumLayers(layers));
 loop{if(l==0){break;}l-=1;let s=textureLoad(layers,xy,l,0);c=s+c*(1.-s.a);}
 textureStore(result,xy,c);
})WGSL";
  WGPUShaderSourceWGSL wgsl{};wgsl.chain.sType=WGPUSType_ShaderSourceWGSL;wgsl.code={code,WGPU_STRLEN};WGPUShaderModuleDescriptor sm{};sm.nextInChain=&wgsl.chain;WGPUShaderModule shader=wgpuDeviceCreateShaderModule(r.device,&sm);WGPUComputePipelineDescriptor pd{};pd.compute.module=shader;pd.compute.entryPoint={"compose",WGPU_STRLEN};pipeline=wgpuDeviceCreateComputePipeline(r.device,&pd);require(pipeline,"layer compute pipeline");wgpuShaderModuleRelease(shader);
  WGPUTextureViewDescriptor vd{};vd.dimension=WGPUTextureViewDimension_2DArray;vd.arrayLayerCount=layerCount;vd.mipLevelCount=1;vd.aspect=WGPUTextureAspect_All;WGPUTextureView inputView=wgpuTextureCreateView(input,&vd);WGPUBindGroupLayout layout=wgpuComputePipelineGetBindGroupLayout(pipeline,0);WGPUBindGroupEntry entries[2]{};entries[0].binding=0;entries[0].textureView=inputView;entries[1].binding=1;entries[1].textureView=outputView;WGPUBindGroupDescriptor gd{};gd.layout=layout;gd.entryCount=2;gd.entries=entries;group=wgpuDeviceCreateBindGroup(r.device,&gd);wgpuBindGroupLayoutRelease(layout);wgpuTextureViewRelease(inputView);wgpuTextureViewRelease(outputView);
  WGPUCommandEncoder encoder=wgpuDeviceCreateCommandEncoder(r.device,nullptr);encodeCompose(encoder,d.size);WGPUCommandBuffer cb=wgpuCommandEncoderFinish(encoder,nullptr);wgpuQueueSubmit(r.queue,1,&cb);wgpuCommandBufferRelease(cb);wgpuCommandEncoderRelease(encoder);r.wait();
 }
 ~LayerGPU(){r.wait();wgpuBindGroupRelease(group);wgpuComputePipelineRelease(pipeline);wgpuTextureRelease(input);}
 void encodeCompose(WGPUCommandEncoder e,int width){WGPUComputePassEncoder pass=wgpuCommandEncoderBeginComputePass(e,nullptr);wgpuComputePassEncoderSetPipeline(pass,pipeline);wgpuComputePassEncoderSetBindGroup(pass,0,group,0,nullptr);wgpuComputePassEncoderDispatchWorkgroups(pass,(width+7)/8,(width+7)/8,1);wgpuComputePassEncoderEnd(pass);wgpuComputePassEncoderRelease(pass);}
 double frame(int index,int width){
  double begin=now();int slot=index%3;r.waitMapped(slot);auto* mapped=static_cast<uint8_t*>(wgpuBufferGetMappedRange(r.staging[slot],0,r.o.updateBytes()));require(mapped,"layer staging map");for(int y=0;y<width;++y)memcpy(mapped+size_t(y)*width*4,d.top+size_t(y)*d.size*4,width*4);r.mappedReady[slot]=false;wgpuBufferUnmap(r.staging[slot]);
  WGPUCommandEncoder e=wgpuDeviceCreateCommandEncoder(r.device,nullptr);WGPUTexelCopyBufferInfo src{};src.buffer=r.staging[slot];src.layout.bytesPerRow=width*4;src.layout.rowsPerImage=width;WGPUTexelCopyTextureInfo dst{};dst.texture=input;WGPUExtent3D extent{uint32_t(width),uint32_t(width),1};wgpuCommandEncoderCopyBufferToTexture(e,&src,&dst,&extent);
  encodeCompose(e,partial?width:d.size);WGPUCommandBuffer cb=wgpuCommandEncoderFinish(e,nullptr);wgpuQueueSubmit(r.queue,1,&cb);wgpuCommandBufferRelease(cb);wgpuCommandEncoderRelease(e);r.remap(slot);r.wakeProgress();return now()-begin;
 }
};

static std::vector<uint8_t> renderedImage(Engine &engine){
 engine.frame(0);engine.wait();std::vector<uint8_t> pixels(size_t(engine.o.size)*engine.o.size*4);
 if(auto* gl=dynamic_cast<GL*>(&engine)){std::vector<uint8_t> reversed(pixels.size());glReadPixels(0,0,engine.o.size,engine.o.size,GL_RGBA,GL_UNSIGNED_BYTE,reversed.data());require(glGetError()==GL_NO_ERROR,"layer GL readback");for(int y=0;y<engine.o.size;++y)memcpy(pixels.data()+size_t(y)*engine.o.size*4,reversed.data()+size_t(engine.o.size-1-y)*engine.o.size*4,engine.o.size*4);}
 else {auto& r=static_cast<WG&>(engine);WGPUBufferDescriptor bd{};bd.size=pixels.size();bd.usage=WGPUBufferUsage_MapRead|WGPUBufferUsage_CopyDst;WGPUBuffer buffer=wgpuDeviceCreateBuffer(r.device,&bd);WGPUCommandEncoder e=wgpuDeviceCreateCommandEncoder(r.device,nullptr);WGPUTexelCopyTextureInfo src{};src.texture=r.targets[0];WGPUTexelCopyBufferInfo dst{};dst.buffer=buffer;dst.layout.bytesPerRow=engine.o.size*4;dst.layout.rowsPerImage=engine.o.size;WGPUExtent3D extent{uint32_t(engine.o.size),uint32_t(engine.o.size),1};wgpuCommandEncoderCopyTextureToBuffer(e,&src,&dst,&extent);WGPUCommandBuffer cb=wgpuCommandEncoderFinish(e,nullptr);wgpuQueueSubmit(r.queue,1,&cb);wgpuCommandBufferRelease(cb);wgpuCommandEncoderRelease(e);r.map(buffer,pixels.size());memcpy(pixels.data(),wgpuBufferGetConstMappedRange(buffer,0,pixels.size()),pixels.size());wgpuBufferUnmap(buffer);wgpuBufferRelease(buffer);}
 return pixels;
}

int main(int argc,char** argv){@autoreleasepool{try{
 Options o;o.size=1024;o.frames=12;o.warmup=3;o.batch=3;o.validation=false;o.updateSize=256;o.api="wgpu";int count=24;std::string mode="gpu-cache";bool precomputed=false;
 for(int i=1;i<argc;i+=2){require(i+1<argc,"missing option");std::string k=argv[i],v=argv[i+1];if(k=="--size")o.size=std::stoi(v);else if(k=="--layers")count=std::stoi(v);else if(k=="--frames")o.frames=std::stoi(v);else if(k=="--update")o.updateSize=std::stoi(v);else if(k=="--batch")o.batch=std::stoi(v);else if(k=="--mode")mode=v;else if(k=="--api")o.api=v;else if(k=="--precomputed")precomputed=std::stoi(v);else throw std::runtime_error("unknown option");}
 require(count>0&&count<=64&&o.size<=2048&&o.updateWidth()<=o.size&&o.updateWidth()%64==0&&o.batch>0&&o.batch<=3,"invalid layer options");bool gpu=mode=="gpu-all"||mode=="gpu-cache"||mode=="gpu-dirty"||mode=="gpu-cache-dirty",cached=mode=="cpu-cache"||mode=="gpu-cache"||mode=="gpu-cache-dirty",full=mode=="cpu-full";require(!gpu||o.api=="wgpu","GPU composition requires wgpu");require(gpu||mode=="cpu-full"||mode=="cpu-dirty"||mode=="cpu-cache","unknown layer mode");int changed=o.updateWidth();if(full)o.updateSize=o.size;
 o.work=gpu?"texture":"upload";o.strategy=o.api=="wgpu"?(full?"direct":"staging-map"):"direct";o.waitMode="callback-idle";
 LayerData data(o.size,count);std::unique_ptr<Engine> renderer;if(o.api=="gl")renderer=std::make_unique<GL>(o);else renderer=std::make_unique<WG>(o);std::unique_ptr<LayerGPU> compute;if(gpu)compute=std::make_unique<LayerGPU>(static_cast<WG&>(*renderer),data,cached,mode=="gpu-dirty"||mode=="gpu-cache-dirty");
 else{renderer->pixels=data.result;if(!renderer->updatePixels.empty())for(int y=0;y<changed;++y)memcpy(renderer->updatePixels.data()+size_t(y)*changed*4,data.result.data()+size_t(y)*o.size*4,changed*4);if(auto* gl=dynamic_cast<GL*>(renderer.get())){glBindTexture(GL_TEXTURE_2D,gl->source);glTexSubImage2D(GL_TEXTURE_2D,0,0,0,o.size,o.size,GL_RGBA,GL_UNSIGNED_BYTE,data.result.data());}else static_cast<WG&>(*renderer).write(true);renderer->wait();}
 std::vector<uint8_t> alternate;if(precomputed){alternate.assign(data.data.begin(),data.data.begin()+data.result.size());data.update(changed,1);std::swap_ranges(alternate.begin(),alternate.end(),data.data.begin());}
 double composeTime=0,prepareTime=0,updateTime=0;auto frame=[&](int i){double t=now();if(precomputed)data.top=i%2?alternate.data():data.data.data();else data.update(changed,i);updateTime+=now()-t;if(gpu){prepareTime+=compute->frame(i,changed);renderer->frame(i);}else{t=now();data.compose(renderer->pixels,0,full?o.size:changed,0,cached);composeTime+=now()-t;if(!renderer->updatePixels.empty())for(int y=0;y<changed;++y)memcpy(renderer->updatePixels.data()+size_t(y)*changed*4,renderer->pixels.data()+size_t(y)*o.size*4,changed*4);Sample s=renderer->frame(i);prepareTime+=s.upload;}};
 for(int i=0;i<o.warmup;++i)frame(i);renderer->wait();composeTime=prepareTime=updateTime=0;std::vector<double> durations;double begin=now(),cpuBegin=cpu();for(int i=0;i<o.frames;i+=o.batch){@autoreleasepool{double start=now();int n=std::min(o.batch,o.frames-i);for(int j=0;j<n;++j)frame(i+j);renderer->wait();durations.push_back((now()-start)/n);}}double elapsed=now()-begin,cpuTime=cpu()-cpuBegin;
 auto actual=renderedImage(*renderer);std::vector<uint8_t> expected(data.result.size());data.compose(expected,0,o.size,0,false);int maxError=0;size_t bad=0;for(size_t i=0;i<actual.size();++i){int error=std::abs(int(actual[i])-int(expected[i]));maxError=std::max(maxError,error);bad+=error>2;}require(bad==0,"layer image differs from CPU source-over reference");
 std::cout<<"{\"api\":"<<jsonString(o.api)<<",\"mode\":"<<jsonString(mode)<<",\"precomputed\":"<<(precomputed?"true":"false")<<",\"size\":"<<o.size<<",\"layers\":"<<count<<",\"gpu_layers\":"<<(gpu?compute->layerCount:0)<<",\"update\":"<<changed<<",\"uploaded_bytes_per_frame\":"<<size_t(full?o.size:changed)*(full?o.size:changed)*4<<",\"frames\":"<<o.frames<<",\"batch\":"<<o.batch<<",\"elapsed_ms\":"<<elapsed/1e6<<",\"process_cpu_ms\":"<<cpuTime/1e6<<",\"cpu_compose_ms\":"<<composeTime/1e6<<",\"prepare_ms\":"<<prepareTime/1e6<<",\"input_update_ms\":"<<updateTime/1e6<<",\"image_verified\":true,\"max_channel_error\":"<<maxError<<",\"metrics_ms\":{";bool first=true;distribution("complete",durations,first);std::cout<<"}}\n";
 compute.reset();renderer.reset();return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}}
