// SPDX-License-Identifier: GPL-2.0-or-later
// Standalone macOS experiment. No LibrePaint or Qt headers/libraries.
#define GL_SILENCE_DEPRECATION
#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>
#import <Metal/Metal.h>
#include <OpenGL/gl3.h>
#include <webgpu/wgpu.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <sys/resource.h>
using Clock=std::chrono::steady_clock;
static double now(){return std::chrono::duration<double,std::nano>(Clock::now().time_since_epoch()).count();}
static double cpu(){rusage u{};getrusage(RUSAGE_SELF,&u);return (u.ru_utime.tv_sec+u.ru_stime.tv_sec)*1e9+(u.ru_utime.tv_usec+u.ru_stime.tv_usec)*1e3;}
static void require(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
static std::string jsonString(const std::string &s){std::string r="\"";for(char c:s){if(c=='\\'||c=='\"')r+='\\';if(c=='\n')r+="\\n";else r+=c;}return r+'\"';}
struct Options {
 std::string api="gl",work="triangle",strategy="direct",present="offscreen",waitMode="block",poolMode="batch";
 int size=32,draws=1,batch=1,frames=500,warmup=100,pollUs=50,updateSize=0;bool validation=true,timestamps=false;
 int updateWidth()const{return updateSize?updateSize:size;}
 size_t updateBytes()const{return size_t(updateWidth())*updateWidth()*4;}
 bool texture()const{return work=="texture"||work=="upload";}
 bool upload()const{return work=="upload";}
 bool surface()const{return present!="offscreen";}
};
struct Sample {double upload=0,acquire=0,encode=0,submit=0,present=0,wait=0,gpu=-1;};
static NSWindow *window=nullptr;
static void events(){if(!window)return;NSEvent *event;while((event=[NSApp nextEventMatchingMask:NSEventMaskAny untilDate:[NSDate distantPast] inMode:NSDefaultRunLoopMode dequeue:YES]))[NSApp sendEvent:event];}
static void makeWindow(int size){
 [NSApplication sharedApplication];[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
 window=[[NSWindow alloc] initWithContentRect:NSMakeRect(0,0,size,size) styleMask:NSWindowStyleMaskTitled backing:NSBackingStoreBuffered defer:NO];
 [window setTitle:@"Standalone GPU API measurement"];[window setLevel:NSFloatingWindowLevel];[window center];[window makeKeyAndOrderFront:nil];[NSApp activateIgnoringOtherApps:YES];
 for(int i=0;i<10;++i){events();[NSThread sleepForTimeInterval:.01];}
}
struct Engine {
 Options o;std::vector<uint8_t> pixels,updatePixels;std::string renderer;bool gpuTiming=false;double timestampPeriod=1;
 Engine(Options options):o(options),pixels(size_t(o.size)*o.size*4),updatePixels(o.updateWidth()==o.size?0:o.updateBytes()){for(int y=0;y<o.size;++y)for(int x=0;x<o.size;++x){size_t p=(size_t(y)*o.size+x)*4;pixels[p]=x%256;pixels[p+1]=y%256;pixels[p+2]=64;pixels[p+3]=255;}if(!updatePixels.empty())for(int y=0;y<o.updateWidth();++y)memcpy(updatePixels.data()+size_t(y)*o.updateWidth()*4,pixels.data()+size_t(y)*o.size*4,o.updateWidth()*4);}
 const uint8_t* uploadData(){if(o.updateWidth()==o.size)return pixels.data();updatePixels[0]=pixels[0];return updatePixels.data();}
 virtual ~Engine()=default;
 virtual void beginMeasurement(){}
 virtual void endMeasurement(){}
 virtual void reportExtra(){}
 virtual Sample frame(int index)=0;
 virtual void wait()=0;
 virtual std::array<int,4> pixel()=0;
};
struct GL final:Engine {
 NSOpenGLContext *context=nullptr;GLuint program=0,vao=0,source=0;
 std::array<GLuint,3> textures{},fbos{},pbos{};GLuint query=0;int swapInterval=0;bool timerSupported=false;
 GL(Options opt):Engine(opt){
 NSOpenGLPixelFormatAttribute attrs[]={NSOpenGLPFAOpenGLProfile,NSOpenGLProfileVersion4_1Core,NSOpenGLPFAAccelerated,NSOpenGLPFAColorSize,24,NSOpenGLPFAAlphaSize,8,NSOpenGLPFADoubleBuffer,0};
 NSOpenGLPixelFormat *format=[[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];require(format!=nil,"OpenGL pixel format");
 context=[[NSOpenGLContext alloc] initWithFormat:format shareContext:nil];require(context!=nil,"OpenGL context");
 if(o.surface()){window.contentView.wantsBestResolutionOpenGLSurface=NO;[context setView:window.contentView];[context update];}
 [context makeCurrentContext];GLint interval=o.present=="fifo"?1:0;[context setValues:&interval forParameter:NSOpenGLContextParameterSwapInterval];[context getValues:&swapInterval forParameter:NSOpenGLContextParameterSwapInterval];
 renderer=reinterpret_cast<const char*>(glGetString(GL_RENDERER));
 GLint count=0;glGetIntegerv(GL_NUM_EXTENSIONS,&count);for(int i=0;i<count;++i){std::string e=reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS,i));timerSupported|=e=="GL_ARB_timer_query"||e=="GL_EXT_timer_query";}
 GLint bits=0;glGetQueryiv(GL_TIME_ELAPSED,GL_QUERY_COUNTER_BITS,&bits);GLenum timerError=glGetError();timerSupported=timerError==GL_NO_ERROR&&bits>0;gpuTiming=o.timestamps&&timerSupported; if(gpuTiming)glGenQueries(1,&query);
 const char *vs="#version 410 core\nout vec2 uv;void main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);gl_Position=vec4(p*2.-1.,0.,1.);uv=vec2(p.x,1.-p.y);}";
 const char *fs=o.texture()?"#version 410 core\nin vec2 uv;uniform sampler2D image;out vec4 color;void main(){color=texture(image,uv);}":"#version 410 core\nout vec4 color;void main(){color=vec4(.25,.5,.75,1.);}";
 auto compile=[](GLenum type,const char *code){GLuint shader=glCreateShader(type);glShaderSource(shader,1,&code,nullptr);glCompileShader(shader);GLint ok=0;glGetShaderiv(shader,GL_COMPILE_STATUS,&ok);if(!ok){char log[4096];glGetShaderInfoLog(shader,sizeof(log),nullptr,log);throw std::runtime_error(log);}return shader;};
 GLuint v=compile(GL_VERTEX_SHADER,vs),f=compile(GL_FRAGMENT_SHADER,fs);program=glCreateProgram();glAttachShader(program,v);glAttachShader(program,f);glLinkProgram(program);GLint linked;glGetProgramiv(program,GL_LINK_STATUS,&linked);require(linked,"GL link");glDeleteShader(v);glDeleteShader(f);glUseProgram(program);
 glGenVertexArrays(1,&vao);glBindVertexArray(vao);glDisable(GL_DITHER);glDisable(GL_BLEND);glDisable(GL_DEPTH_TEST);
 glGenTextures(1,&source);glBindTexture(GL_TEXTURE_2D,source);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,o.size,o.size,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
 if(!o.surface())for(int i=0;i<3;++i){glGenTextures(1,&textures[i]);glBindTexture(GL_TEXTURE_2D,textures[i]);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,o.size,o.size,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);glGenFramebuffers(1,&fbos[i]);glBindFramebuffer(GL_FRAMEBUFFER,fbos[i]);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,textures[i],0);require(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE,"GL FBO");}
 glBindTexture(GL_TEXTURE_2D,source);glUniform1i(glGetUniformLocation(program,"image"),0);glFinish();require(glGetError()==GL_NO_ERROR,"GL initialize error");
 if(o.strategy=="pbo"){glGenBuffers(3,pbos.data());for(auto b:pbos){glBindBuffer(GL_PIXEL_UNPACK_BUFFER,b);glBufferData(GL_PIXEL_UNPACK_BUFFER,o.updateBytes(),nullptr,GL_STREAM_DRAW);}glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);}
 }
 ~GL(){[context clearDrawable];[NSOpenGLContext clearCurrentContext];}
 Sample frame(int index)override{
 Sample s;double t=now();if(o.upload()){glBindTexture(GL_TEXTURE_2D,source);const void* data=uploadData();if(o.strategy=="pbo"){glBindBuffer(GL_PIXEL_UNPACK_BUFFER,pbos[index%3]);void* mapped=glMapBufferRange(GL_PIXEL_UNPACK_BUFFER,0,o.updateBytes(),GL_MAP_WRITE_BIT|GL_MAP_INVALIDATE_BUFFER_BIT);require(mapped,"GL PBO map");memcpy(mapped,data,o.updateBytes());require(glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER),"GL PBO unmap");data=nullptr;}glTexSubImage2D(GL_TEXTURE_2D,0,0,0,o.updateWidth(),o.updateWidth(),GL_RGBA,GL_UNSIGNED_BYTE,data);if(o.strategy=="pbo")glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);}s.upload=now()-t;
 t=now();glBindFramebuffer(GL_FRAMEBUFFER,o.surface()?0:fbos[index%3]);glViewport(0,0,o.size,o.size);glDisable(GL_SCISSOR_TEST);
 if(gpuTiming)glBeginQuery(GL_TIME_ELAPSED,query);
 glClearColor(.125,.125,.125,1.);glClear(GL_COLOR_BUFFER_BIT);
 if(o.work!="clear"){
  if(o.work=="draws"){glEnable(GL_SCISSOR_TEST);glScissor(0,0,1,1);}
  glUseProgram(program);glBindVertexArray(vao);glBindTexture(GL_TEXTURE_2D,source);
  if(o.strategy=="instanced")glDrawArraysInstanced(GL_TRIANGLES,0,3,o.draws);else for(int i=0;i<o.draws;++i)glDrawArrays(GL_TRIANGLES,0,3);
 }
 if(gpuTiming)glEndQuery(GL_TIME_ELAPSED);s.encode=now()-t;
 t=now();glFlush();s.submit=now()-t;
 if(o.surface()){t=now();[context flushBuffer];s.present=now()-t;}
 if(gpuTiming){t=now();glFinish();s.wait=now()-t;GLuint64 duration=0;glGetQueryObjectui64v(query,GL_QUERY_RESULT,&duration);if(duration>0&&duration<1000000000)s.gpu=double(duration);require(glGetError()==GL_NO_ERROR,"GL timer query error");}
 return s;
 }
 void wait()override{glFinish();}
 std::array<int,4> pixel()override{uint8_t p[4];glReadBuffer(o.surface()?GL_FRONT:GL_COLOR_ATTACHMENT0);glReadPixels(0,o.work=="draws"?0:o.size-1,1,1,GL_RGBA,GL_UNSIGNED_BYTE,p);require(glGetError()==GL_NO_ERROR,"GL readback error");return {p[0],p[1],p[2],p[3]};}
};
struct WG final:Engine {
 WGPUInstance instance=nullptr;WGPUAdapter adapter=nullptr;WGPUDevice device=nullptr;WGPUQueue queue=nullptr;WGPUSurface surface=nullptr;
 WGPURenderPipeline pipeline=nullptr;WGPUBindGroup group=nullptr;WGPUTexture source=nullptr;std::array<WGPUTexture,3> targets{};std::array<WGPUTextureView,3> views{};
 WGPURenderBundle bundle=nullptr;WGPUQuerySet queries=nullptr;WGPUBuffer resolve=nullptr,read=nullptr,pixelRead=nullptr;
 CAMetalLayer *layer=nullptr;bool capture=false;WGPUTextureFormat format=WGPUTextureFormat_RGBA8Unorm;
 std::array<WGPUBuffer,3> staging{};std::array<std::atomic<bool>,3> mappedReady{};
 std::mutex progressMutex;std::condition_variable progressCV;std::thread progressThread;bool stopping=false,kick=false;uint64_t progressGeneration=0;
 std::atomic<uint64_t> pollCalls{0};uint64_t remapRequests=0;double remapWait=0;
 uint64_t measuredPollCalls=0,measuredRemapRequests=0;double measuredRemapWait=0;
 std::array<double,4> uploadPhases{},measuredUploadPhases{};
 bool callbacks()const{return o.waitMode=="callback"||o.waitMode=="callback-idle";}
 bool staged()const{return o.strategy=="staging-write"||o.strategy=="staging-map"||o.strategy=="staging-fresh";}
 bool fresh()const{return o.strategy=="staging-fresh";}
 bool mapped()const{return o.strategy=="staging-map";}
 bool poll(){bool empty=wgpuDevicePoll(device,false,nullptr);++pollCalls;return empty;}
 void startProgress(){if(!callbacks())return;progressThread=std::thread([this]{if(o.waitMode=="callback-idle"){std::unique_lock<std::mutex> lock(progressMutex);while(!stopping){progressCV.wait(lock,[&]{return stopping||kick;});if(stopping)return;uint64_t generation=progressGeneration;lock.unlock();bool empty=poll();lock.lock();if(empty&&generation==progressGeneration)kick=false;if(kick)progressCV.wait_for(lock,std::chrono::microseconds(o.pollUs),[&]{return stopping||generation!=progressGeneration;});}return;}while(true){{std::unique_lock<std::mutex> lock(progressMutex);progressCV.wait_for(lock,std::chrono::microseconds(o.pollUs),[&]{return stopping||kick;});if(stopping)return;kick=false;}poll();}});}
 void wakeProgress(){if(!callbacks())return;{std::lock_guard<std::mutex> lock(progressMutex);kick=true;++progressGeneration;}progressCV.notify_all();}
 void waitMapped(int slot){double start=now();if(callbacks()){std::unique_lock<std::mutex> lock(progressMutex);require(progressCV.wait_for(lock,std::chrono::seconds(30),[&]{return mappedReady[slot].load();}),"wgpu staging map timeout");}else{while(!mappedReady[slot].load()){poll();require(now()-start<30e9,"wgpu staging map timeout");if(o.waitMode=="sleep")std::this_thread::sleep_for(std::chrono::microseconds(o.pollUs));}}remapWait+=now()-start;}
 void remap(int slot){WGPUBufferMapCallbackInfo cb{};cb.mode=WGPUCallbackMode_AllowSpontaneous;cb.userdata1=this;cb.userdata2=reinterpret_cast<void*>(uintptr_t(slot));cb.callback=[](WGPUMapAsyncStatus status,WGPUStringView,void* p,void* s){if(status!=WGPUMapAsyncStatus_Success)std::abort();auto* self=static_cast<WG*>(p);{std::lock_guard<std::mutex> lock(self->progressMutex);self->mappedReady[uintptr_t(s)].store(true);}self->progressCV.notify_all();};wgpuBufferMapAsync(staging[slot],WGPUMapMode_Write,0,o.updateBytes(),cb);++remapRequests;wakeProgress();}
 WG(Options opt):Engine(opt){
 WGPUInstanceExtras extras{};extras.chain.sType=static_cast<WGPUSType>(WGPUSType_InstanceExtras);extras.backends=WGPUInstanceBackend_Metal;extras.flags=o.validation?WGPUInstanceFlag_Validation:0;
 WGPUInstanceDescriptor id{};id.nextInChain=&extras.chain;instance=wgpuCreateInstance(&id);require(instance,"wgpu instance");
 if(o.surface()){
  layer=[CAMetalLayer layer];layer.contentsScale=1;layer.drawableSize=CGSizeMake(o.size,o.size);window.contentView.wantsLayer=YES;window.contentView.layer=layer;
  WGPUSurfaceSourceMetalLayer src{};src.chain.sType=WGPUSType_SurfaceSourceMetalLayer;src.layer=(__bridge void*)layer;WGPUSurfaceDescriptor sd{};sd.nextInChain=&src.chain;surface=wgpuInstanceCreateSurface(instance,&sd);require(surface,"wgpu surface");
 }
 WGPURequestAdapterOptions ao{};ao.backendType=WGPUBackendType_Metal;ao.compatibleSurface=surface;
 WGPURequestAdapterCallbackInfo ac{};ac.mode=WGPUCallbackMode_AllowSpontaneous;ac.userdata1=this;ac.callback=[](WGPURequestAdapterStatus status,WGPUAdapter a,WGPUStringView,void *p,void*){require(status==WGPURequestAdapterStatus_Success,"wgpu adapter");static_cast<WG*>(p)->adapter=a;};wgpuInstanceRequestAdapter(instance,&ao,ac);require(adapter,"wgpu adapter callback");
 gpuTiming=o.timestamps&&wgpuAdapterHasFeature(adapter,WGPUFeatureName_TimestampQuery);
 WGPUFeatureName feature=WGPUFeatureName_TimestampQuery;WGPUDeviceDescriptor dd{};dd.requiredFeatureCount=gpuTiming?1:0;dd.requiredFeatures=gpuTiming?&feature:nullptr;
 dd.uncapturedErrorCallbackInfo.callback=[](const WGPUDevice*,WGPUErrorType,WGPUStringView message,void*,void*){std::cerr<<"wgpu error: "<<std::string(message.data,message.length==WGPU_STRLEN?strlen(message.data):message.length)<<'\n';std::abort();};
 WGPURequestDeviceCallbackInfo dc{};dc.mode=WGPUCallbackMode_AllowSpontaneous;dc.userdata1=this;dc.callback=[](WGPURequestDeviceStatus status,WGPUDevice d,WGPUStringView,void *p,void*){require(status==WGPURequestDeviceStatus_Success,"wgpu device");static_cast<WG*>(p)->device=d;};wgpuAdapterRequestDevice(adapter,&dd,dc);require(device,"wgpu device callback");queue=wgpuDeviceGetQueue(device);
 WGPUAdapterInfo info{};wgpuAdapterGetInfo(adapter,&info);renderer.assign(info.device.data,info.device.length==WGPU_STRLEN?strlen(info.device.data):info.device.length);wgpuAdapterInfoFreeMembers(info);timestampPeriod=wgpuQueueGetTimestampPeriod(queue);
 if(surface){
  WGPUSurfaceCapabilities caps{};require(wgpuSurfaceGetCapabilities(surface,adapter,&caps)==WGPUStatus_Success,"surface capabilities");
  auto mode=o.present=="fifo"?WGPUPresentMode_Fifo:WGPUPresentMode_Immediate;bool supported=false;for(size_t i=0;i<caps.presentModeCount;++i)supported|=caps.presentModes[i]==mode;require(supported,"presentation mode unsupported");
  format=caps.formats[0];for(size_t i=0;i<caps.formatCount;++i)if(caps.formats[i]==WGPUTextureFormat_BGRA8Unorm)format=caps.formats[i];
  require(caps.usages&WGPUTextureUsage_CopySrc,"surface copy-source unavailable");WGPUSurfaceConfiguration c{};c.device=device;c.format=format;c.width=o.size;c.height=o.size;c.usage=WGPUTextureUsage_RenderAttachment|WGPUTextureUsage_CopySrc;c.presentMode=mode;c.alphaMode=caps.alphaModes[0];wgpuSurfaceConfigure(surface,&c);wgpuSurfaceCapabilitiesFreeMembers(caps);
 }
 const char *code=o.texture()?
 "@group(0) @binding(0) var img:texture_2d<f32>;@group(0) @binding(1) var smp:sampler;struct V{@builtin(position) p:vec4f,@location(0) uv:vec2f};@vertex fn vertexMain(@builtin(vertex_index) i:u32)->V{let p=vec2f(f32((i<<1u)&2u),f32(i&2u));var v:V;v.p=vec4f(p*2.-1.,0.,1.);v.uv=vec2f(p.x,1.-p.y);return v;}@fragment fn fragmentMain(v:V)->@location(0) vec4f{return textureSample(img,smp,v.uv);}":
 "@vertex fn vertexMain(@builtin(vertex_index) i:u32)->@builtin(position) vec4f{let p=vec2f(f32((i<<1u)&2u),f32(i&2u));return vec4f(p*2.-1.,0.,1.);}@fragment fn fragmentMain()->@location(0) vec4f{return vec4f(.25,.5,.75,1.);}";
 WGPUShaderSourceWGSL wgsl{};wgsl.chain.sType=WGPUSType_ShaderSourceWGSL;wgsl.code={code,WGPU_STRLEN};WGPUShaderModuleDescriptor sm{};sm.nextInChain=&wgsl.chain;WGPUShaderModule shader=wgpuDeviceCreateShaderModule(device,&sm);
 WGPUBindGroupLayout layout=nullptr;
 if(o.texture()){
  WGPUBindGroupLayoutEntry entries[2]{};entries[0].binding=0;entries[0].visibility=WGPUShaderStage_Fragment;entries[0].texture.sampleType=WGPUTextureSampleType_Float;entries[0].texture.viewDimension=WGPUTextureViewDimension_2D;entries[1].binding=1;entries[1].visibility=WGPUShaderStage_Fragment;entries[1].sampler.type=WGPUSamplerBindingType_Filtering;
  WGPUBindGroupLayoutDescriptor l{};l.entryCount=2;l.entries=entries;layout=wgpuDeviceCreateBindGroupLayout(device,&l);
  WGPUTextureDescriptor t{};t.size={uint32_t(o.size),uint32_t(o.size),1};t.format=WGPUTextureFormat_RGBA8Unorm;t.dimension=WGPUTextureDimension_2D;t.mipLevelCount=1;t.sampleCount=1;t.usage=WGPUTextureUsage_TextureBinding|WGPUTextureUsage_CopyDst;source=wgpuDeviceCreateTexture(device,&t);
  write(true);WGPUTextureView view=wgpuTextureCreateView(source,nullptr);WGPUSamplerDescriptor sd{};sd.addressModeU=sd.addressModeV=sd.addressModeW=WGPUAddressMode_ClampToEdge;sd.magFilter=sd.minFilter=WGPUFilterMode_Nearest;sd.mipmapFilter=WGPUMipmapFilterMode_Nearest;sd.maxAnisotropy=1;WGPUSampler sampler=wgpuDeviceCreateSampler(device,&sd);
  WGPUBindGroupEntry e[2]{};e[0].binding=0;e[0].textureView=view;e[1].binding=1;e[1].sampler=sampler;WGPUBindGroupDescriptor g{};g.layout=layout;g.entryCount=2;g.entries=e;group=wgpuDeviceCreateBindGroup(device,&g);wgpuTextureViewRelease(view);wgpuSamplerRelease(sampler);
 }
 WGPUPipelineLayoutDescriptor ld{};ld.bindGroupLayoutCount=layout?1:0;ld.bindGroupLayouts=layout?&layout:nullptr;WGPUPipelineLayout pl=wgpuDeviceCreatePipelineLayout(device,&ld);
 WGPUColorTargetState ct{};ct.format=format;ct.writeMask=WGPUColorWriteMask_All;WGPUFragmentState fs{};fs.module=shader;fs.entryPoint={"fragmentMain",WGPU_STRLEN};fs.targetCount=1;fs.targets=&ct;
 WGPURenderPipelineDescriptor pd{};pd.layout=pl;pd.vertex.module=shader;pd.vertex.entryPoint={"vertexMain",WGPU_STRLEN};pd.fragment=&fs;pd.primitive.topology=WGPUPrimitiveTopology_TriangleList;pd.multisample.count=1;pd.multisample.mask=~0u;pipeline=wgpuDeviceCreateRenderPipeline(device,&pd);require(pipeline,"wgpu pipeline");wgpuShaderModuleRelease(shader);wgpuPipelineLayoutRelease(pl);if(layout)wgpuBindGroupLayoutRelease(layout);
 if(!surface)for(int i=0;i<3;++i){WGPUTextureDescriptor t{};t.size={uint32_t(o.size),uint32_t(o.size),1};t.format=format;t.dimension=WGPUTextureDimension_2D;t.mipLevelCount=1;t.sampleCount=1;t.usage=WGPUTextureUsage_RenderAttachment|WGPUTextureUsage_CopySrc;targets[i]=wgpuDeviceCreateTexture(device,&t);views[i]=wgpuTextureCreateView(targets[i],nullptr);}
 if(o.strategy=="bundle"&&o.work!="clear"){
  WGPURenderBundleEncoderDescriptor d{};d.colorFormatCount=1;d.colorFormats=&format;d.sampleCount=1;WGPURenderBundleEncoder e=wgpuDeviceCreateRenderBundleEncoder(device,&d);wgpuRenderBundleEncoderSetPipeline(e,pipeline);if(group)wgpuRenderBundleEncoderSetBindGroup(e,0,group,0,nullptr);for(int i=0;i<o.draws;++i)wgpuRenderBundleEncoderDraw(e,3,1,0,0);bundle=wgpuRenderBundleEncoderFinish(e,nullptr);wgpuRenderBundleEncoderRelease(e);
 }
 WGPUBufferDescriptor bd{};bd.size=256;bd.usage=WGPUBufferUsage_CopyDst|WGPUBufferUsage_MapRead;pixelRead=wgpuDeviceCreateBuffer(device,&bd);
 if(gpuTiming){WGPUQuerySetDescriptor q{};q.type=WGPUQueryType_Timestamp;q.count=2;queries=wgpuDeviceCreateQuerySet(device,&q);bd.size=16;bd.usage=WGPUBufferUsage_QueryResolve|WGPUBufferUsage_CopySrc;resolve=wgpuDeviceCreateBuffer(device,&bd);bd.usage=WGPUBufferUsage_MapRead|WGPUBufferUsage_CopyDst;read=wgpuDeviceCreateBuffer(device,&bd);}
 if(staged()&&!fresh()){WGPUBufferDescriptor s{};s.size=o.updateBytes();s.usage=WGPUBufferUsage_CopySrc|(mapped()?WGPUBufferUsage_MapWrite:WGPUBufferUsage_CopyDst);s.mappedAtCreation=mapped();for(int i=0;i<3;++i){staging[i]=wgpuDeviceCreateBuffer(device,&s);require(staging[i],"wgpu staging buffer");mappedReady[i].store(mapped());}}
 startProgress();wait();
 }
 ~WG(){wait();if(mapped())for(int i=0;i<3;++i)waitMapped(i);if(progressThread.joinable()){{std::lock_guard<std::mutex> lock(progressMutex);stopping=true;}progressCV.notify_all();progressThread.join();}for(auto b:staging)if(b)wgpuBufferRelease(b);if(surface){wgpuSurfaceUnconfigure(surface);wgpuSurfaceRelease(surface);}if(bundle)wgpuRenderBundleRelease(bundle);if(group)wgpuBindGroupRelease(group);if(source)wgpuTextureRelease(source);for(auto v:views)if(v)wgpuTextureViewRelease(v);for(auto t:targets)if(t)wgpuTextureRelease(t);if(queries)wgpuQuerySetRelease(queries);if(resolve)wgpuBufferRelease(resolve);if(read)wgpuBufferRelease(read);wgpuBufferRelease(pixelRead);wgpuRenderPipelineRelease(pipeline);wgpuQueueRelease(queue);wgpuDeviceRelease(device);wgpuAdapterRelease(adapter);wgpuInstanceRelease(instance);}
 void write(bool initial=false){int width=initial?o.size:o.updateWidth();WGPUTexelCopyTextureInfo dst{};dst.texture=source;WGPUTexelCopyBufferLayout l{};l.bytesPerRow=width*4;l.rowsPerImage=width;WGPUExtent3D extent{uint32_t(width),uint32_t(width),1};wgpuQueueWriteTexture(queue,&dst,initial?pixels.data():uploadData(),size_t(width)*width*4,&l,&extent);}
 void map(WGPUBuffer buffer,size_t bytes){struct Ready{WG* self;std::atomic<bool> ready{false};};Ready ready{this};WGPUBufferMapCallbackInfo cb{};cb.mode=WGPUCallbackMode_AllowSpontaneous;cb.userdata1=&ready;cb.callback=[](WGPUMapAsyncStatus status,WGPUStringView,void* p,void*){if(status!=WGPUMapAsyncStatus_Success)std::abort();auto* r=static_cast<Ready*>(p);std::lock_guard<std::mutex> lock(r->self->progressMutex);r->ready=true;r->self->progressCV.notify_all();};wgpuBufferMapAsync(buffer,WGPUMapMode_Read,0,bytes,cb);if(callbacks()){wakeProgress();std::unique_lock<std::mutex> lock(progressMutex);require(progressCV.wait_for(lock,std::chrono::seconds(30),[&]{return ready.ready.load();}),"wgpu readback map timeout");}else{double deadline=now()+30e9;while(!ready.ready.load()){poll();require(now()<deadline,"wgpu readback map timeout");}}}
 Sample frame(int index)override{
 int slot=index%3;Sample s;double t=now();if(o.upload()){if(staged()){if(mapped()||fresh()){if(fresh()){WGPUBufferDescriptor d{};d.size=o.updateBytes();d.usage=WGPUBufferUsage_CopySrc|WGPUBufferUsage_MapWrite;d.mappedAtCreation=true;double begin=now();staging[slot]=wgpuDeviceCreateBuffer(device,&d);uploadPhases[0]+=now()-begin;require(staging[slot],"wgpu fresh staging allocation");}else waitMapped(slot);void* ptr=wgpuBufferGetMappedRange(staging[slot],0,o.updateBytes());require(ptr,"wgpu mapped staging");double begin=now();memcpy(ptr,uploadData(),o.updateBytes());uploadPhases[1]+=now()-begin;mappedReady[slot].store(false);begin=now();wgpuBufferUnmap(staging[slot]);uploadPhases[2]+=now()-begin;}else{double begin=now();wgpuQueueWriteBuffer(queue,staging[slot],0,uploadData(),o.updateBytes());uploadPhases[3]+=now()-begin;}}else{double begin=now();write();uploadPhases[3]+=now()-begin;}}s.upload=now()-t;
 WGPUSurfaceTexture acquired{};WGPUTexture target=targets[index%3];WGPUTextureView view=views[index%3];
 if(surface){t=now();wgpuSurfaceGetCurrentTexture(surface,&acquired);s.acquire=now()-t;require(acquired.texture,"wgpu drawable");target=acquired.texture;view=wgpuTextureCreateView(target,nullptr);}
 t=now();WGPUCommandEncoder e=wgpuDeviceCreateCommandEncoder(device,nullptr);if(o.upload()&&staged()){WGPUTexelCopyBufferInfo src{};src.buffer=staging[slot];src.layout.bytesPerRow=o.updateWidth()*4;src.layout.rowsPerImage=o.updateWidth();WGPUTexelCopyTextureInfo dst{};dst.texture=source;WGPUExtent3D size{uint32_t(o.updateWidth()),uint32_t(o.updateWidth()),1};wgpuCommandEncoderCopyBufferToTexture(e,&src,&dst,&size);}WGPURenderPassColorAttachment a{};a.view=view;a.depthSlice=WGPU_DEPTH_SLICE_UNDEFINED;a.loadOp=WGPULoadOp_Clear;a.storeOp=WGPUStoreOp_Store;a.clearValue={.125,.125,.125,1.};WGPURenderPassDescriptor pd{};pd.colorAttachmentCount=1;pd.colorAttachments=&a;
 WGPURenderPassTimestampWrites ts{};if(gpuTiming){ts.querySet=queries;ts.beginningOfPassWriteIndex=0;ts.endOfPassWriteIndex=1;pd.timestampWrites=&ts;}
 WGPURenderPassEncoder p=wgpuCommandEncoderBeginRenderPass(e,&pd);
 if(o.work!="clear"){
  if(o.work=="draws")wgpuRenderPassEncoderSetScissorRect(p,0,0,1,1);
  if(bundle)wgpuRenderPassEncoderExecuteBundles(p,1,&bundle);
  else{wgpuRenderPassEncoderSetPipeline(p,pipeline);if(group)wgpuRenderPassEncoderSetBindGroup(p,0,group,0,nullptr);if(o.strategy=="instanced")wgpuRenderPassEncoderDraw(p,3,o.draws,0,0);else for(int i=0;i<o.draws;++i)wgpuRenderPassEncoderDraw(p,3,1,0,0);}
 }
 wgpuRenderPassEncoderEnd(p);wgpuRenderPassEncoderRelease(p);
 if(gpuTiming){wgpuCommandEncoderResolveQuerySet(e,queries,0,2,resolve,0);wgpuCommandEncoderCopyBufferToBuffer(e,resolve,0,read,0,16);}
 if(capture){WGPUTexelCopyTextureInfo src{};src.texture=target;WGPUTexelCopyBufferInfo dst{};dst.buffer=pixelRead;dst.layout.bytesPerRow=256;dst.layout.rowsPerImage=1;WGPUExtent3D size{1,1,1};wgpuCommandEncoderCopyTextureToBuffer(e,&src,&dst,&size);}
 WGPUCommandBuffer command=wgpuCommandEncoderFinish(e,nullptr);s.encode=now()-t;t=now();wgpuQueueSubmit(queue,1,&command);wgpuCommandBufferRelease(command);wgpuCommandEncoderRelease(e);if(o.upload()&&mapped())remap(slot);if(o.upload()&&fresh()){wgpuBufferRelease(staging[slot]);staging[slot]=nullptr;}if(callbacks())wakeProgress();else poll();s.submit=now()-t;
 if(surface){t=now();require(wgpuSurfacePresent(surface)==WGPUStatus_Success,"wgpu present");wgpuTextureViewRelease(view);wgpuTextureRelease(target);s.present=now()-t;}
 if(gpuTiming){t=now();wait();s.wait=now()-t;map(read,16);const auto *values=static_cast<const uint64_t*>(wgpuBufferGetConstMappedRange(read,0,16));if(values[1]>values[0]&&(values[1]-values[0])*timestampPeriod<1e9)s.gpu=(values[1]-values[0])*timestampPeriod;wgpuBufferUnmap(read);}
 return s;
 }
 void wait()override{if(!device)return;if(callbacks()){struct Done{WG* self;bool ready=false;};Done done{this};WGPUQueueWorkDoneCallbackInfo cb{};cb.mode=WGPUCallbackMode_AllowSpontaneous;cb.userdata1=&done;cb.callback=[](WGPUQueueWorkDoneStatus status,void* p,void*){if(status!=WGPUQueueWorkDoneStatus_Success)std::abort();auto* d=static_cast<Done*>(p);std::lock_guard<std::mutex> lock(d->self->progressMutex);d->ready=true;d->self->progressCV.notify_all();};wgpuQueueOnSubmittedWorkDone(queue,cb);wakeProgress();std::unique_lock<std::mutex> lock(progressMutex);require(progressCV.wait_for(lock,std::chrono::seconds(30),[&]{return done.ready;}),"wgpu completion callback timeout");}else if(o.waitMode=="spin"||o.waitMode=="sleep"){double deadline=now()+30e9;while(!wgpuDevicePoll(device,false,nullptr)){++pollCalls;require(now()<deadline,"wgpu poll timeout");if(o.waitMode=="sleep")std::this_thread::sleep_for(std::chrono::microseconds(o.pollUs));}++pollCalls;}else wgpuDevicePoll(device,true,nullptr);}
 void beginMeasurement()override{pollCalls=0;remapRequests=0;remapWait=0;uploadPhases.fill(0);}
 void endMeasurement()override{measuredPollCalls=pollCalls;measuredRemapRequests=remapRequests;measuredRemapWait=remapWait;measuredUploadPhases=uploadPhases;}
 void reportExtra()override{std::cout<<",\"wgpu_poll_calls\":"<<measuredPollCalls<<",\"wgpu_remap_requests\":"<<measuredRemapRequests<<",\"wgpu_remap_wait_ms\":"<<measuredRemapWait/1e6<<",\"wgpu_upload_phase_ms\":["<<measuredUploadPhases[0]/1e6<<','<<measuredUploadPhases[1]/1e6<<','<<measuredUploadPhases[2]/1e6<<','<<measuredUploadPhases[3]/1e6<<']';}
 std::array<int,4> pixel()override{capture=true;frame(0);capture=false;wait();map(pixelRead,256);const auto *p=static_cast<const uint8_t*>(wgpuBufferGetConstMappedRange(pixelRead,0,256));std::array<int,4> result{p[0],p[1],p[2],p[3]};if(format==WGPUTextureFormat_BGRA8Unorm)std::swap(result[0],result[2]);wgpuBufferUnmap(pixelRead);return result;}
};
struct NativeMetal final:Engine {
 id<MTLDevice> device;id<MTLCommandQueue> queue;id<MTLRenderPipelineState> pipeline;id<MTLTexture> source;std::array<id<MTLTexture>,3> targets;std::array<id<MTLBuffer>,3> staging;id<MTLCommandBuffer> last;CAMetalLayer *layer=nullptr;
 NativeMetal(Options opt):Engine(opt){
 device=MTLCreateSystemDefaultDevice();require(device!=nil,"Metal device");renderer=device.name.UTF8String;queue=[device newCommandQueue];
 MTLPixelFormat format=MTLPixelFormatRGBA8Unorm;
 if(o.surface()){layer=[CAMetalLayer layer];layer.device=device;layer.pixelFormat=MTLPixelFormatBGRA8Unorm;layer.framebufferOnly=NO;layer.contentsScale=1;layer.drawableSize=CGSizeMake(o.size,o.size);layer.displaySyncEnabled=o.present=="fifo";window.contentView.wantsLayer=YES;window.contentView.layer=layer;format=layer.pixelFormat;}
 NSString *code=o.texture()?@"#include <metal_stdlib>\nusing namespace metal;struct V{float4 p [[position]];float2 uv;};vertex V vertexMain(uint i [[vertex_id]]){float2 p=float2((i<<1)&2,i&2);return {float4(p*2-1,0,1),float2(p.x,1-p.y)};}fragment float4 fragmentMain(V v [[stage_in]],texture2d<float> t [[texture(0)]]){constexpr sampler s(coord::normalized,address::clamp_to_edge,filter::nearest);return t.sample(s,v.uv);}":@"#include <metal_stdlib>\nusing namespace metal;vertex float4 vertexMain(uint i [[vertex_id]]){float2 p=float2((i<<1)&2,i&2);return float4(p*2-1,0,1);}fragment float4 fragmentMain(){return float4(.25,.5,.75,1);}";
 NSError *error=nil;id<MTLLibrary> library=[device newLibraryWithSource:code options:nil error:&error];if(!library)throw std::runtime_error(error.localizedDescription.UTF8String);
 MTLRenderPipelineDescriptor *pd=[MTLRenderPipelineDescriptor new];pd.vertexFunction=[library newFunctionWithName:@"vertexMain"];pd.fragmentFunction=[library newFunctionWithName:@"fragmentMain"];pd.colorAttachments[0].pixelFormat=format;pipeline=[device newRenderPipelineStateWithDescriptor:pd error:&error];require(pipeline!=nil,"Metal pipeline");
 MTLTextureDescriptor *td=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:format width:o.size height:o.size mipmapped:NO];td.usage=MTLTextureUsageRenderTarget;td.storageMode=MTLStorageModePrivate;for(int i=0;i<3;++i){targets[i]=[device newTextureWithDescriptor:td];staging[i]=[device newBufferWithLength:pixels.size() options:MTLResourceStorageModeShared];}
 if(o.texture()){
  td.pixelFormat=MTLPixelFormatRGBA8Unorm;td.usage=MTLTextureUsageShaderRead;source=[device newTextureWithDescriptor:td];memcpy(staging[0].contents,pixels.data(),pixels.size());id<MTLCommandBuffer> cb=[queue commandBuffer];id<MTLBlitCommandEncoder> b=[cb blitCommandEncoder];copy(b,staging[0],true);[b endEncoding];[cb commit];[cb waitUntilCompleted];
 }
 gpuTiming=o.timestamps;
 }
 void copy(id<MTLBlitCommandEncoder> b,id<MTLBuffer> buffer,bool initial=false){int width=initial?o.size:o.updateWidth();[b copyFromBuffer:buffer sourceOffset:0 sourceBytesPerRow:width*4 sourceBytesPerImage:size_t(width)*width*4 sourceSize:MTLSizeMake(width,width,1) toTexture:source destinationSlice:0 destinationLevel:0 destinationOrigin:MTLOriginMake(0,0,0)];}
 Sample frame(int index)override{
 Sample s;double t=now();if(o.upload())memcpy(staging[index%3].contents,uploadData(),o.updateBytes());s.upload=now()-t;
 id<CAMetalDrawable> drawable=nil;id<MTLTexture> target=targets[index%3];if(layer){t=now();drawable=[layer nextDrawable];require(drawable!=nil,"Metal drawable");target=drawable.texture;s.acquire=now()-t;}
 t=now();id<MTLCommandBuffer> cb=[queue commandBuffer];if(o.upload()){id<MTLBlitCommandEncoder> b=[cb blitCommandEncoder];copy(b,staging[index%3]);[b endEncoding];}
 MTLRenderPassDescriptor *pd=[MTLRenderPassDescriptor renderPassDescriptor];pd.colorAttachments[0].texture=target;pd.colorAttachments[0].loadAction=MTLLoadActionClear;pd.colorAttachments[0].storeAction=MTLStoreActionStore;pd.colorAttachments[0].clearColor=MTLClearColorMake(.125,.125,.125,1);
 id<MTLRenderCommandEncoder> e=[cb renderCommandEncoderWithDescriptor:pd];if(o.work!="clear"){
  [e setRenderPipelineState:pipeline];if(source)[e setFragmentTexture:source atIndex:0];if(o.work=="draws")[e setScissorRect:MTLScissorRect{0,0,1,1}];
  if(o.strategy=="instanced")[e drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3 instanceCount:o.draws];else for(int i=0;i<o.draws;++i)[e drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
 }
 [e endEncoding];s.encode=now()-t;
 if(drawable&&o.strategy!="split-present"){t=now();[cb presentDrawable:drawable];s.present=now()-t;}
 t=now();[cb commit];last=cb;s.submit=now()-t;
 if(drawable&&o.strategy=="split-present"){t=now();id<MTLCommandBuffer> present=[queue commandBuffer];[present presentDrawable:drawable];[present commit];s.present=now()-t;}
 if(gpuTiming){t=now();wait();s.wait=now()-t;if(cb.GPUEndTime>cb.GPUStartTime)s.gpu=(cb.GPUEndTime-cb.GPUStartTime)*1e9;}
 return s;
 }
 void wait()override{if(last){[last waitUntilCompleted];require(last.status!=MTLCommandBufferStatusError,"Metal command error");if(o.poolMode=="frame")last=nil;}}
 std::array<int,4> pixel()override{
 id<MTLTexture> target=targets[0];if(o.surface())return {-1,-1,-1,-1};
 id<MTLBuffer> output=[device newBufferWithLength:256 options:MTLResourceStorageModeShared];id<MTLCommandBuffer> cb=[queue commandBuffer];id<MTLBlitCommandEncoder> b=[cb blitCommandEncoder];[b copyFromTexture:target sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0,0,0) sourceSize:MTLSizeMake(1,1,1) toBuffer:output destinationOffset:0 destinationBytesPerRow:256 destinationBytesPerImage:256];[b endEncoding];[cb commit];[cb waitUntilCompleted];const auto *p=static_cast<const uint8_t*>(output.contents);return {p[0],p[1],p[2],p[3]};
 }
};
static void distribution(const char*,std::vector<double>,bool&);
#ifdef LIBREPAINT_EXPERIMENT_BGFX
#include "bgfx-engine.inc.mm"
#endif
#ifdef LIBREPAINT_EXPERIMENT_SOKOL
#include "sokol-engine.inc.mm"
#endif
static void distribution(const char *key,std::vector<double> values,bool &first){
 if(!first)std::cout<<',';first=false;std::sort(values.begin(),values.end());double median=values[values.size()/2];if(values.size()%2==0)median=(median+values[values.size()/2-1])*.5;
 std::cout<<jsonString(key)<<":{\"median\":"<<median/1e6<<",\"p95\":"<<values[size_t(std::ceil(values.size()*.95))-1]/1e6<<",\"min\":"<<values[0]/1e6<<",\"max\":"<<values.back()/1e6<<'}';
}
int main(int argc,char **argv){@autoreleasepool{try{
 Options o;for(int i=1;i<argc;i+=2){require(i+1<argc,"option value missing");std::string key=argv[i],v=argv[i+1];if(key=="--api")o.api=v;else if(key=="--work")o.work=v;else if(key=="--strategy")o.strategy=v;else if(key=="--present")o.present=v;else if(key=="--wait")o.waitMode=v;else if(key=="--pool")o.poolMode=v;else if(key=="--poll-us")o.pollUs=std::stoi(v);else if(key=="--update-size")o.updateSize=std::stoi(v);else if(key=="--size")o.size=std::stoi(v);else if(key=="--draws")o.draws=std::stoi(v);else if(key=="--batch")o.batch=std::stoi(v);else if(key=="--frames")o.frames=std::stoi(v);else if(key=="--warmup")o.warmup=std::stoi(v);else if(key=="--validation")o.validation=std::stoi(v);else if(key=="--timestamps")o.timestamps=std::stoi(v);else throw std::runtime_error("unknown option "+key);}
 require(o.updateWidth()<=o.size&&o.updateWidth()>0&&o.pollUs>0,"invalid update size/poll interval");if(o.updateSize)require(o.api=="gl"||o.api=="wgpu"||o.api=="metal","partial update requires supported API");require(o.batch>=1&&o.batch<=3&&o.frames>=o.batch,"invalid batch/frames");require(o.strategy!="bundle"||o.api=="wgpu","bundle is wgpu only");if(o.timestamps)require(o.batch==1,"timestamp probe uses serial execution");if(o.surface())makeWindow(o.size);
 std::unique_ptr<Engine> engine;if(o.api=="gl")engine=std::make_unique<GL>(o);else if(o.api=="wgpu")engine=std::make_unique<WG>(o);else if(o.api=="metal")engine=std::make_unique<NativeMetal>(o);
#ifdef LIBREPAINT_EXPERIMENT_BGFX
 else if(o.api=="bgfx-single"||o.api=="bgfx-threaded")engine=std::make_unique<BG>(o);
#endif

#ifdef LIBREPAINT_EXPERIMENT_SOKOL
 else if(o.api=="sokol")engine=std::make_unique<SG>(o);
#endif
 else throw std::runtime_error("unknown API");
 if(o.upload()&&!o.surface()){engine->pixels[0]=127;engine->frame(0);engine->wait();auto changed=engine->pixel();require(changed[0]==127,"upload did not change rendered pixel");engine->pixels[0]=0;engine->frame(0);engine->wait();}
 for(int i=0;i<o.warmup;++i){@autoreleasepool{engine->frame(i);if((i+1)%o.batch==0)engine->wait();events();}}engine->wait();
 engine->beginMeasurement();std::vector<Sample> samples;std::vector<double> complete;double cpuBegin=cpu(),elapsedBegin=now();
 for(int i=0;i<o.frames;i+=o.batch){@autoreleasepool{double start=now();int count=std::min(o.batch,o.frames-i);for(int j=0;j<count;++j){if(o.poolMode=="frame"){@autoreleasepool{samples.push_back(engine->frame(i+j));}}else samples.push_back(engine->frame(i+j));}double waitStart=now();engine->wait();samples.back().wait+=now()-waitStart;complete.push_back((now()-start)/count);events();}}
 double elapsed=now()-elapsedBegin,processCpu=cpu()-cpuBegin;engine->endMeasurement();
 // Readback outside measurement. All source resources, shaders and pipelines are reused.
 engine->frame(0);engine->wait();auto p=o.surface()?std::array<int,4>{-1,-1,-1,-1}:engine->pixel();std::array<int,4> expected=o.work=="clear"?std::array<int,4>{32,32,32,255}:o.texture()?std::array<int,4>{0,0,64,255}:std::array<int,4>{64,128,191,255};
 if(!o.surface())for(int c=0;c<4;++c)require(std::abs(p[c]-expected[c])<=1,"rendered pixel mismatch");
 std::cout<<"{\"api\":"<<jsonString(o.api)<<",\"renderer\":"<<jsonString(engine->renderer)<<",\"work\":"<<jsonString(o.work)<<",\"strategy\":"<<jsonString(o.strategy)<<",\"present\":"<<jsonString(o.present)<<",\"wait_mode\":"<<jsonString(o.api=="wgpu"?o.waitMode:"native")<<",\"pool_mode\":"<<jsonString(o.poolMode)<<",\"size\":"<<o.size<<",\"update_size\":"<<o.updateWidth()<<",\"transferred_bytes_per_frame\":"<<(o.upload()?o.updateBytes():0)<<",\"poll_us\":"<<o.pollUs<<",\"draws\":"<<o.draws<<",\"batch\":"<<o.batch<<",\"frames\":"<<o.frames<<",\"warmup\":"<<o.warmup<<",\"validation\":"<<(o.validation?"true":"false")<<",\"timestamp_requested\":"<<(o.timestamps?"true":"false")<<",\"gpu_timestamp_supported\":"<<(engine->gpuTiming?"true":"false")<<",\"timestamp_period_ns\":"<<engine->timestampPeriod<<",\"gpu_scope\":"<<jsonString((o.api=="metal"||o.api=="sokol"||o.api.find("bgfx")==0)?"command_buffer_including_upload":"render_pass_excluding_upload")<<",\"elapsed_ms\":"<<elapsed/1e6<<",\"process_cpu_ms\":"<<processCpu/1e6<<",\"fps\":"<<o.frames*1e9/elapsed<<",\"upload_change_verified\":"<<((o.upload()&&!o.surface())?"true":"false")<<",\"pixel_verified\":"<<(o.surface()?"false":"true")<<",\"pixel\":["<<p[0]<<','<<p[1]<<','<<p[2]<<','<<p[3]<<']';
 if(auto *gl=dynamic_cast<GL*>(engine.get()))std::cout<<",\"actual_swap_interval\":"<<gl->swapInterval;
 if(auto *wg=dynamic_cast<WG*>(engine.get());wg&&wg->layer)std::cout<<",\"actual_display_sync\":"<<(wg->layer.displaySyncEnabled?"true":"false");
 if(auto *m=dynamic_cast<NativeMetal*>(engine.get());m&&m->layer)std::cout<<",\"actual_display_sync\":"<<(m->layer.displaySyncEnabled?"true":"false");
 engine->reportExtra();
 std::cout<<",\"invalid_gpu_timestamp_samples\":"<<std::count_if(samples.begin(),samples.end(),[&](const Sample &s){return engine->gpuTiming&&s.gpu<0;})<<",\"metrics_ms\":{";bool first=true;distribution("batch_complete_per_frame",complete,first);
 for(auto name:{"upload","acquire","encode","submit","present","wait","gpu"}){std::vector<double> v;for(const auto &s:samples){double x=0;if(std::string(name)=="upload")x=s.upload;else if(std::string(name)=="acquire")x=s.acquire;else if(std::string(name)=="encode")x=s.encode;else if(std::string(name)=="submit")x=s.submit;else if(std::string(name)=="present")x=s.present;else if(std::string(name)=="wait")x=s.wait;else x=s.gpu;if(x>=0)v.push_back(x);}if(!v.empty())distribution(name,v,first);}
 std::cout<<"},\"samples_ns\":[";for(size_t i=0;i<samples.size();++i){if(i)std::cout<<',';auto s=samples[i];std::cout<<'['<<s.upload<<','<<s.acquire<<','<<s.encode<<','<<s.submit<<','<<s.present<<','<<s.wait<<','<<s.gpu<<']';}std::cout<<"]}\n";
 engine.reset();if(window){[window orderOut:nil];window=nil;}return 0;
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}}
