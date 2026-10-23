#pragma once
#include "Common/FsUtils.hpp"
#include "VerticalModConfig.hpp"

namespace Vertical {
// Three connected UV panels. Each wall is one projective plane, not a radial fan.
class ProjectiveWall {
    struct alignas(16) Parameters {std::array<Float4,12> clip{},world{};};
    MeshData m_source,m_data{12,6};
    DynamicMesh m_mesh{m_data};
    VertexShader m_shader;
    mutable ConstantBuffer<Parameters> m_parameters;
    std::array<Float3,12> m_local{},m_world{},m_screen{};
    std::array<Float2,12> m_uv{};
    Mat4x4 m_transform;
    BasicCamera3D m_camera;
    Vec2 m_center{},m_shift{};
    double m_lo=.42,m_hi=.58,m_v=.26,m_radius=58.5,m_weight=1;
    double m_f=1,m_sLeft=1,m_sRight=1,m_dv=0,m_alpha=1,m_requested=0,m_margin=0;
    double m_lowerResponse=0,m_outerOffset=0,m_lastControl=0;
    bool m_valid=false,m_ready=false,m_gpu=false,m_disabled=false,m_cached=false;
    std::array<Float3,4> m_lastActual{},m_lastReference{},m_lastCorners{};
    String m_profile=U"wall";
    static Float4 Clip(Float3 p,const Mat4x4& matrix) {
        DirectX::XMFLOAT4 a{p.x,p.y,p.z,1},b;
        DirectX::XMStoreFloat4(&b,DirectX::XMVector4Transform(DirectX::XMLoadFloat4(&a),matrix.value));
        return {b.x,b.y,b.z,b.w};
    }
    static Float3 Unclip(Float4 p,const Mat4x4& inverse) {
        DirectX::XMFLOAT4 a{p.x,p.y,p.z,p.w},b;
        DirectX::XMStoreFloat4(&b,DirectX::XMVector4Transform(DirectX::XMLoadFloat4(&a),inverse.value));
        return {b.x/b.w,b.y/b.w,b.z/b.w};
    }
    Float3 Source(Vec2 uv) const {
        const auto a=m_source.vertices[0].pos,b=m_source.vertices[1].pos,c=m_source.vertices[2].pos;
        return a+(b-a)*static_cast<float>(uv.x)+(c-a)*static_cast<float>(uv.y);
    }
    double Q(Vec2 uv) const {
        const double r=uv.x<m_lo ? (m_lo-uv.x)/m_lo : uv.x>m_hi ? (uv.x-m_hi)/(1-m_hi) : 0;
        return 1+(1/m_f-1)*r;
    }
    // The extra depth factor supplies the correct perspective UV interpolation.
    Float3 Mapped(Vec2 uv,double sLeft,double sRight) const {
        const double r=uv.x<m_lo ? (m_lo-uv.x)/m_lo : uv.x>m_hi ? (uv.x-m_hi)/(1-m_hi) : 0;
        const double q=Q(uv),inner=uv.x<m_lo ? m_lo : m_hi;
        const double s=uv.x<m_lo ? sLeft : sRight;
        Vec2 mapped=uv;
        if(r>0) {
            mapped.x=inner+(uv.x-inner)*s/q;
            mapped.y=m_v+(uv.y-m_v+r*m_dv/m_f)/q;
        }
        const auto world=m_transform.transformPoint(Source(mapped));
        const auto eye=Float3{m_camera.getEyePosition()};
        return eye+(world-eye)*static_cast<float>(q);
    }
    Float4 DisplayClip(Vec2 uv,double sLeft,double sRight) const {
        auto p=Clip(Mapped(uv,sLeft,sRight),m_camera.getViewProj());
        const auto size=m_camera.getSceneSize();
        p.x+=static_cast<float>(2*m_shift.x/size.x)*p.w;
        p.y-=static_cast<float>(2*m_shift.y/size.y)*p.w;
        return p;
    }
    Vec2 Pixel(Float4 p) const {
        const auto size=m_camera.getSceneSize();
        return {(p.x/p.w*.5+.5)*size.x,(.5-p.y/p.w*.5)*size.y};
    }
    bool Initialize(Float3 reference) {
        const auto size=m_camera.getSceneSize();
        const auto left=m_camera.worldToScreenPoint(m_transform.transformPoint(Source({0,0})));
        const auto right=m_camera.worldToScreenPoint(m_transform.transformPoint(Source({1,0})));
        m_radius=58.5*size.y/1280.0;
        m_lo=.5-m_radius/Max(static_cast<double>((right-left).length()),1.0);
        m_hi=1-m_lo;
        m_center=Vec2{reference.xy()}-Vec2{0,m_radius};
        const auto top=m_camera.worldToScreenPoint(m_transform.transformPoint(Source({.5,0})));
        const auto bottom=m_camera.worldToScreenPoint(m_transform.transformPoint(Source({.5,1})));
        m_v=Clamp((m_center.y-top.y)/(bottom.y-top.y),0.0,1.0);
        if(m_lo<=.05 || m_lo>=.48) return false;
        const std::array<double,4> u{0,m_lo,m_hi,1};
        for(int panel=0;panel<3;++panel) {
            const std::array<Vec2,4> uv{Vec2{u[panel],0},Vec2{u[panel+1],0},Vec2{u[panel],1},Vec2{u[panel+1],1}};
            for(int k=0;k<4;++k) {
                const int i=panel*4+k;m_uv[i]=Float2{uv[k]};m_local[i]=Source(uv[k]);
                auto& v=m_data.vertices[i];v.pos=m_local[i];v.tex=m_uv[i];v.normal=m_source.vertices[0].normal;
                if(m_gpu) v.normal.x=static_cast<float>(i);
            }
            m_data.indices[2*panel]={static_cast<uint32>(panel*4),static_cast<uint32>(panel*4+2),static_cast<uint32>(panel*4+1)};
            m_data.indices[2*panel+1]={static_cast<uint32>(panel*4+1),static_cast<uint32>(panel*4+2),static_cast<uint32>(panel*4+3)};
        }
        for(auto& tri:m_data.indices) if((m_local[tri.i1]-m_local[tri.i0]).cross(m_local[tri.i2]-m_local[tri.i0]).dot(m_source.vertices[0].normal)<0) std::swap(tri.i1,tri.i2);
        m_mesh.fill(m_data.indices);if(m_gpu)m_mesh.fill(m_data.vertices);
        m_ready=true;return true;
    }
    double Cover(bool left) const {
        // Solve only the extension actually needed by the rotated wall's two end points.
        // Large authored spins retain their original orientation rather than auto-fitting.
        double s=0;
        const auto size=m_camera.getSceneSize();
        const double target=(left ? -2.0*size.x/720 : size.x+2.0*size.x/720);
        const double ndc=target/size.x*2-1;
        for(double v:{0.,1.}) {
            const Vec2 uv{left ? 0.0 : 1.0,v};
            const auto a=DisplayClip(uv,left?0:1,left?1:0);
            const auto c=DisplayClip(uv,1,1);const auto b=c-a;
            const double divisor=b.x-ndc*b.w;
            if(Abs(divisor)<1e-8)return 1;
            const double root=(ndc*a.w-a.x)/divisor;
            if(!std::isfinite(root)||root<0||root*m_f>3.0)return 1;
            s=Max(s,root);
        }
        return Max(s,1e-4);
    }
    void Apply() {
        m_sLeft=Cover(true);m_sRight=Cover(false);
        m_valid=true;m_margin=1e20;
        const auto inverse=m_transform.inverse();
        for(int i=0;i<12;++i) {
            const auto clip=DisplayClip(Vec2{m_uv[i]},m_sLeft,m_sRight);
            if(!std::isfinite(clip.x)||!std::isfinite(clip.y)||!std::isfinite(clip.z)||!std::isfinite(clip.w)||clip.w<=1e-6) {m_valid=false;return;}
            const auto world=Unclip(clip,m_camera.getInvViewProj());
            m_world[i]=world;m_screen[i]=m_camera.worldToScreenPoint(world);
            m_parameters->clip[i]=clip;m_parameters->world[i]={world.x,world.y,world.z,1};
            if(!m_gpu)m_data.vertices[i].pos=inverse.transformPoint(world);
            const auto base=m_camera.worldToScreenPoint(m_transform.transformPoint(m_local[i]));
            if(i==0||i==2)m_margin=Min(m_margin,-static_cast<double>(m_screen[i].x));
            if(i==9||i==11)m_margin=Min(m_margin,static_cast<double>(m_screen[i].x-m_camera.getSceneSize().x));
            if(!std::isfinite(base.z)||!std::isfinite(m_screen[i].z)){m_valid=false;return;}
        }
        if(minAreaRatio()<.20) {m_valid=false;return;}
        if(!m_gpu)m_mesh.fill(m_data.vertices);
    }
public:
    using GearQuad=std::array<Float3,4>;
    explicit ProjectiveWall(const MeshData& source,bool enabled,bool forceCPU=false):m_source(source) {
        const auto args=System::GetCommandLineArgs();m_disabled=!enabled||args.contains(U"--vertical-preview-warp-off");
        if(!m_disabled&&!forceCPU&&!args.contains(U"--vertical-preview-warp-cpu")) {
            m_shader=HLSL{FsUtils::GetResourcePath(U"mods/vertical/shaders/wall_projective.hlsl"),U"VS"};m_gpu=static_cast<bool>(m_shader);
        }
    }
    void setProfile(FilePathView file,const std::array<int,8>& strengths=EffectiveDepthConfig().depthIntensities) {
        size_t index=7;m_profile=U"custom";
        for(size_t i=0;i<7;++i) {
            if(FileSystem::FullPath(file)!=FileSystem::FullPath(U"imgs/bg/{}.gif"_fmt(kDepthImageNames[i])))continue;
            index=i;m_profile=kDepthImageNames[i];break;
        }
        m_weight=std::clamp(strengths[index],0,200)/100.0;
    }
    void warmUp(const BasicCamera3D& camera,const Mat4x4& transform,Float3 reference,const Texture& texture) {
        if(m_disabled)return;m_camera=camera;m_transform=transform;
        if(!Initialize(camera.worldToScreenPoint(reference)))return;
        m_shift={0,0};m_f=1;m_dv=0;Apply();
        if(!m_gpu||!m_valid)return;
        const RenderTexture scratch{Size{1,1},TextureFormat::R8G8B8A8_Unorm,HasDepth::Yes};
        const ScopedRenderTarget3D target{scratch};Graphics3D::SetCameraTransform(camera);draw(transform,texture);Graphics3D::Flush();
    }
    void updateGear(const BasicCamera3D& camera,const Mat4x4& transform,const GearQuad& actual,const GearQuad& reference,double lowerControl=std::numeric_limits<double>::quiet_NaN()) {
        if(m_disabled||m_weight==0){m_valid=false;return;}
        for(int i=0;i<4;++i) if(!std::isfinite(actual[i].x)||!std::isfinite(actual[i].y)||!std::isfinite(actual[i].z)||!std::isfinite(reference[i].x)||!std::isfinite(reference[i].y)||!std::isfinite(reference[i].z)||actual[i].z<=0||reference[i].z<=0){m_valid=false;return;}
        m_camera=camera;m_transform=transform;
        const Vec2 upper=(Vec2{actual[0].xy()}+Vec2{actual[1].xy()})*.5;
        const Vec2 neutral=(Vec2{reference[0].xy()}+Vec2{reference[1].xy()})*.5;
        if(!m_ready&&!Initialize(Float3{static_cast<float>(neutral.x),static_cast<float>(neutral.y),reference[0].z})){m_valid=false;return;}
        std::array<Float3,4> corners{};bool same=m_cached && (!std::isfinite(lowerControl) || Abs(lowerControl-m_lastControl)<1e-12);
        for(int i=0;i<4;++i){corners[i]=camera.worldToScreenPoint(transform.transformPoint(m_source.vertices[i].pos));same=same&&(actual[i]-m_lastActual[i]).lengthSq()<1e-10&&(reference[i]-m_lastReference[i]).lengthSq()<1e-10&&(corners[i]-m_lastCorners[i]).lengthSq()<1e-10;}
        if(same)return;m_cached=true;if(std::isfinite(lowerControl))m_lastControl=lowerControl;m_lastActual=actual;m_lastReference=reference;m_lastCorners=corners;
        const auto baseCenter=camera.worldToScreenPoint(transform.transformPoint(Source({.5,m_v})));
        m_shift=upper-Vec2{0,m_radius}-Vec2{baseCenter.xy()};m_requested=upper.y-neutral.y;
        const double width=(Vec2{reference[3].xy()}-Vec2{reference[2].xy()}).length();
        const double topWidth=(Vec2{reference[1].xy()}-Vec2{reference[0].xy()}).length();
        if(width<1e-6||topWidth<1e-6){m_valid=false;return;}
        const double ratio=(Vec2{actual[3].xy()}-Vec2{actual[2].xy()}).length()/width;
        const double topRatio=(Vec2{actual[1].xy()}-Vec2{actual[0].xy()}).length()/topWidth;
        // The two sides of lower zoom are separate artistic responses, not
        // inverse uniform scales. Preserve the inner edge and move the outer wall.
        m_lowerResponse=std::isfinite(lowerControl) ? Clamp(lowerControl,-1.0,1.0) : Clamp((ratio-1)/.4,-1.0,1.0);
        const double magnitude=Abs(m_lowerResponse);
        const double targetThickness=1-(m_lowerResponse<0 ? .45 : .68)*magnitude;
        m_f=Max(.25,1+m_weight*(targetThickness-1));
        m_outerOffset=m_weight*(m_lowerResponse<0 ? 220.0 : 80.0)*magnitude*camera.getSceneSize().y/1280;
        const Vec2 lower=(Vec2{actual[2].xy()}+Vec2{actual[3].xy()})*.5;
        const Vec2 lowRef=(Vec2{reference[2].xy()}+Vec2{reference[3].xy()})*.5;
        const double height=Max(static_cast<double>((corners[2]-corners[0]).length()),1.0);
        m_dv=(m_weight*((lower.y-lowRef.y)-(upper.y-neutral.y))+m_outerOffset)/height;
        // Far width affects the wall's pitch cue while the protected center stays rigid.
        m_dv+=m_weight*(Clamp(topRatio,.60,1.40)-1)*.10;
        m_alpha=1;Apply();
        while(!m_valid&&m_alpha>.03125) {
            m_alpha*=.5;m_shift*=.5;m_dv*=.5;m_f=1+(m_f-1)*.5;Apply();
        }
    }
    bool draw(const Mat4x4& transform,const Texture& texture,const ColorF& color=Palette::White) const {
        if(!m_valid||m_disabled||m_weight==0)return false;
        // Projective depth is interpolation data, not gameplay occlusion geometry.
        // Keep the wall behind the subsequently drawn highway and effects.
        const ScopedRenderStates3D depth{DepthStencilState{false,false}};
        if(m_gpu){const ScopedCustomShader3D shader{m_shader};Graphics3D::SetVSConstantBuffer(4,m_parameters);m_mesh.draw(transform,texture,color);}
        else m_mesh.draw(transform,texture,color);
        return true;
    }
    void drawDebug() const {
        if(!m_valid)return;
        for(const auto t:m_data.indices)Triangle{Vec2{m_screen[t.i0].xy()},Vec2{m_screen[t.i1].xy()},Vec2{m_screen[t.i2].xy()}}.drawFrame(1,ColorF{0,1,1,.7});
        const auto center=Pixel(DisplayClip({.5,m_v},m_sLeft,m_sRight));Circle{center,m_radius}.drawFrame(2,Palette::Hotpink);
        for(int panel=0;panel<3;++panel) for(double u:{0.,.25,.5,.75,1.}) {
            const double a=panel==0?0:panel==1?m_lo:m_hi,b=panel==0?m_lo:panel==1?m_hi:1;
            Line{Pixel(DisplayClip({a+(b-a)*u,0},m_sLeft,m_sRight)),Pixel(DisplayClip({a+(b-a)*u,1},m_sLeft,m_sRight))}.draw(1,ColorF{1,.8,0,.8});
        }
    }
    double lineError() const {
        if(!m_ready)return 0;
        double error=0;
        for(int panel=0;panel<3;++panel) for(double v:{.3125,.45,.75}) {
            const double a=panel==0?0:panel==1?m_lo:m_hi,b=panel==0?m_lo:panel==1?m_hi:1;
            const Vec2 start=Pixel(DisplayClip({a,v},m_sLeft,m_sRight)),end=Pixel(DisplayClip({b,v+.05},m_sLeft,m_sRight));
            for(double t:{.25,.5,.75}){const auto p=Pixel(DisplayClip({a+(b-a)*t,v+.05*t},m_sLeft,m_sRight));error=Max(error,Abs((end.x-start.x)*(p.y-start.y)-(end.y-start.y)*(p.x-start.x))/Max((end-start).length(),1e-6));}
        }
        return error;
    }
    double minAreaRatio() const {
        if(!m_ready)return 1;
        double ratio=1;
        const auto area=[](Vec2 a,Vec2 b,Vec2 c){return (b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x);};
        for(const auto t:m_data.indices){const auto base=[&](uint32 i){return Vec2{m_camera.worldToScreenPoint(m_transform.transformPoint(m_local[i])).xy()};};const double original=area(base(t.i0),base(t.i1),base(t.i2));if(Abs(original)>1e-6)ratio=Min(ratio,area(Vec2{m_screen[t.i0].xy()},Vec2{m_screen[t.i1].xy()},Vec2{m_screen[t.i2].xy()})/original);}
        return ratio;
    }
    double sharedEdgeError() const {double e=0;for(auto pair:{std::pair{1,4},std::pair{3,6},std::pair{5,8},std::pair{7,10}})e=Max(e,static_cast<double>((m_screen[pair.first]-m_screen[pair.second]).length()));return e;}
    double uvError() const {double e=0;for(int i=0;i<12;++i)e=Max(e,static_cast<double>((m_uv[i]-m_data.vertices[i].tex).length()));return e;}
    double protectedShapeError() const {
        double e=0;
        for(double u:{m_lo,.5,m_hi})for(double v:{m_v-.08,m_v,m_v+.08}) {
            const Vec2 base=m_camera.worldToScreenPoint(m_transform.transformPoint(Source({u,v}))).xy();
            const Vec2 actual=Pixel(DisplayClip({u,v},m_sLeft,m_sRight));
            e=Max(e,(actual-base-m_shift).length());
        }
        return e;
    }
    double projectionError(const BasicCamera3D& camera,const Mat4x4& transform) const {
        double e=0;
        if(!m_gpu)for(int i=0;i<12;++i)e=Max(e,static_cast<double>((camera.worldToScreenPoint(transform.transformPoint(m_data.vertices[i].pos))-m_screen[i]).length()));
        return e;
    }
    double thickness() const{return m_f;}
    bool valid() const{return m_valid;}
    bool gpu() const{return m_gpu;}
    double margin() const{return m_margin;}
    void writeReport(FilePathView path) const {
        TextWriter out{path};out.writeln(U"valid,gpu,requested_pixels,applied_pixels,min_area_ratio,vertices,triangles,alpha,projective,middle_max_move,wall_thickness,straight_error,uv_error,shared_edge_error,cover_margin,profile,lower_response,outer_offset_pixels");
        out.writeln(U"{},{},{},{},{},12,6,{},true,0,{},{},{},{},{},{},{},{}"_fmt(m_valid,m_gpu,m_requested,m_shift.y,minAreaRatio(),m_alpha,m_f,lineError(),uvError(),sharedEdgeError(),m_margin,m_profile,m_lowerResponse,m_outerOffset));
    }
};
}
