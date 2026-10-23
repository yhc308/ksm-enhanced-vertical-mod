#pragma once
#include "ksmaudio/ksmaudio.hpp"
namespace Vertical::OptionsPresentation {
inline RenderTexture& Backdrop(){static RenderTexture value;return value;}
inline std::shared_ptr<ksmaudio::Stream>& Music(){static std::shared_ptr<ksmaudio::Stream> value;return value;}
inline void Begin(){if(!Music())Music()=std::make_shared<ksmaudio::Stream>("se/option_bgm.ogg",1.0,false,false,true);if(!Music()->isPlaying())Music()->play();}
inline void Pause(){if(Music())Music()->pause();}
inline void Reset(){Music().reset();Backdrop()=RenderTexture{};}
inline void Draw(){if(Backdrop())Backdrop().resized(Scene::Size()).draw();else Scene::Rect().draw(ColorF{.025,.05,.08});Scene::Rect().draw(ColorF{0,.7});}
}
