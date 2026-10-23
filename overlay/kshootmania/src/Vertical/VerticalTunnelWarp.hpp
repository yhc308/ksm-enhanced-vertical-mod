#pragma once
#include "Common/FsUtils.hpp"

namespace Vertical
{
    // Display-only piecewise affine tunnel. No chart values or UVs are rewritten.
    class TunnelWarp
    {
        struct alignas(16) Parameters { std::array<Float4,49> shifts{}; };
        MeshData m_data{49,80};
        DynamicMesh m_mesh{m_data};
        VertexShader m_shader;
        mutable ConstantBuffer<Parameters> m_parameters;
        MeshData m_source;
        std::array<Float3,49> m_screen{};
        std::array<Float3,49> m_local{};
        std::array<double,49> m_weights{};
        std::array<Float3,4> m_corners{};
        Vec2 m_center{};
        std::array<int,16> m_outerEdge{};
        std::array<double,16> m_outerT{};
        std::array<Float2,49> m_sourceUV{};
        double m_thickness=1;

        double m_delta=0, m_alpha=1;
        std::array<Vec2,49> m_moves{};
        bool m_projective=false, m_quadCached=false;
        std::array<Float3,4> m_lastActual{},m_lastReference{};
        bool m_valid=false, m_cached=false, m_gpu=false;
        bool m_disabled=false;
        double m_requested=0;
        static double Area(Vec2 a,Vec2 b,Vec2 c) { return (b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x); }
        static bool Finite(Float3 p) { return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z); }

        Float3 LocalAt(const BasicCamera3D& camera, const Mat4x4& transform, Vec2 screen) const
        {
            const auto a=transform.transformPoint(m_source.vertices[0].pos);
            const auto b=transform.transformPoint(m_source.vertices[1].pos);
            const auto c=transform.transformPoint(m_source.vertices[2].pos);
            const Vec3 normal=(Vec3{b}-Vec3{a}).cross(Vec3{c}-Vec3{a});
            const Vec3 eye=camera.getEyePosition();
            const Vec3 ray=Vec3{camera.screenToWorldPoint(Float2{screen},camera.worldToScreenPoint(a).z)}-eye;
            const double denominator=normal.dot(ray);
            if(Abs(denominator)<1e-8) return {NAN,NAN,NAN};
            return transform.inverse().transformPoint(eye+ray*(normal.dot(Vec3{a}-eye)/denominator));
        }

        bool Build(const BasicCamera3D& camera,const Mat4x4& transform,Vec2 center,const std::array<Float3,4>& corners)
        {
            // Billboard ordering: TL, TR, BL, BR. Ring ordering is clockwise on screen.
            const std::array<Vec2,4> boundary{Vec2{corners[0].xy()},Vec2{corners[1].xy()},Vec2{corners[3].xy()},Vec2{corners[2].xy()}};
            const double radius=58.5*camera.getSceneSize().y/1280.0/Cos(22.5_deg);
            std::array<double,16> angles{};
            for(int i=0;i<8;++i) angles[i]=i*Math::TwoPi/8;
            for(int i=0;i<4;++i) {
                const auto v=boundary[i]-center;
                angles[8+i]=Atan2(v.y,v.x);
                if(angles[8+i]<0) angles[8+i]+=Math::TwoPi;
            }
            // Two additional controls on each side edge, sharing the same UV surface.
            for(int side=0;side<2;++side) for(int k=0;k<2;++k) {
                const int edge=side==0 ? 1 : 3;
                const Vec2 v=boundary[edge]+(boundary[(edge+1)%4]-boundary[edge])*(k==0 ? .25 : .75)-center;
                angles[12+side*2+k]=Atan2(v.y,v.x);
                if(angles[12+side*2+k]<0) angles[12+side*2+k]+=Math::TwoPi;
            }
            std::sort(angles.begin(),angles.end());
            std::array<Vec2,49> points{}; points[0]=center;
            m_weights.fill(0); m_weights[0]=1;
            for(int i=0;i<16;++i) {
                const Vec2 direction{Cos(angles[i]),Sin(angles[i])};
                double distance=1e20;
                for(int edge=0;edge<4;++edge) {
                    const Vec2 a=boundary[edge], segment=boundary[(edge+1)%4]-a, relative=a-center;
                    const double cross=direction.x*segment.y-direction.y*segment.x;
                    if(Abs(cross)<1e-8) continue;
                    const double t=(relative.x*segment.y-relative.y*segment.x)/cross;
                    const double u=(relative.x*direction.y-relative.y*direction.x)/cross;
                    if(t>0 && u>=-1e-6 && u<=1+1e-6 && t<distance) {distance=t;m_outerEdge[i]=edge;m_outerT[i]=Clamp(u,0.0,1.0);}
                }
                if(!std::isfinite(distance)||distance>1e10||distance<=radius+1) return false;
                points[1+i]=center+direction*radius; m_weights[1+i]=1;
                points[17+i]=center+direction*((radius+distance)*.5); m_weights[17+i]=.5;
                points[33+i]=center+direction*distance;
            }
            const auto left=m_source.vertices[0].pos.x, right=m_source.vertices[1].pos.x;
            const auto top=m_source.vertices[0].pos.y, bottom=m_source.vertices[2].pos.y;
            for(int i=0;i<49;++i) {
                const auto local=LocalAt(camera,transform,points[i]);
                if(!Finite(local)) return false;
                m_local[i]=local;
                m_screen[i]=camera.worldToScreenPoint(transform.transformPoint(local));
                auto& vertex=m_data.vertices[i];
                vertex.pos=local;
                vertex.normal=m_source.vertices[0].normal;
                vertex.normal.x=static_cast<float>(i);
                vertex.tex={static_cast<float>((local.x-left)/(right-left)),static_cast<float>((local.y-top)/(bottom-top))};
            }
            for(uint32 i=0;i<16;++i) {
                const uint32 j=(i+1)%16;
                m_data.indices[5*i]={0,1+i,1+j};
                m_data.indices[5*i+1]={1+i,17+i,17+j};
                m_data.indices[5*i+2]={1+i,17+j,1+j};
                m_data.indices[5*i+3]={17+i,33+i,33+j};
                m_data.indices[5*i+4]={17+i,33+j,17+j};
            }
            for(auto& tri:m_data.indices) {
                const auto cross=(m_local[tri.i1]-m_local[tri.i0]).cross(m_local[tri.i2]-m_local[tri.i0]);
                if(cross.dot(m_source.vertices[0].normal)<0) std::swap(tri.i1,tri.i2);
            }
            for(int i=0;i<49;++i) m_sourceUV[i]=m_data.vertices[i].tex;
            m_mesh.fill(m_data.indices);
            if(m_gpu) m_mesh.fill(m_data.vertices);
            return true;
        }
    public:
        explicit TunnelWarp(const MeshData& source,bool enabled,bool forceCPU=false):m_source(source)
        {
            const auto args=System::GetCommandLineArgs();
            m_disabled=!enabled||args.contains(U"--vertical-preview-warp-off");
            if(!m_disabled && !forceCPU && !args.contains(U"--vertical-preview-warp-cpu")) {
                m_shader=HLSL{FsUtils::GetResourcePath(U"mods/vertical/shaders/tunnel_warp.hlsl"),U"VS"};
                m_gpu=static_cast<bool>(m_shader);
            }
        }
        void warmUp(const BasicCamera3D& camera,const Mat4x4& transform,Float3 reference,const Texture& texture)
        {
            const auto p=camera.worldToScreenPoint(reference);
            update(camera,transform,camera.screenToWorldPoint(Float2{p.x,p.y+1},p.z),reference);
            if(!m_gpu) return;
            // Touch the pipeline before music starts; never allocate this during play.
            const RenderTexture scratch{Size{1,1},TextureFormat::R8G8B8A8_Unorm,HasDepth::Yes};
            const ScopedRenderTarget3D target{scratch};
            Graphics3D::SetCameraTransform(camera);
            draw(transform,texture);
            Graphics3D::Flush();
        }
        void update(const BasicCamera3D& camera,const Mat4x4& transform,Float3 farWorld,Float3 referenceWorld)
        {
            if(m_disabled) {m_valid=false;return;}
            const auto farPoint=camera.worldToScreenPoint(farWorld), reference=camera.worldToScreenPoint(referenceWorld);
            if(!Finite(farPoint)||!Finite(reference)||farPoint.z<=0||reference.z<=0) {m_valid=false;return;}
            const bool changed=prepare(camera,transform,reference);
            if(!m_valid) return;
            m_requested=farPoint.y-reference.y;
            setMoves(camera,transform,Vec2{farPoint.xy()}-Vec2{0,58.5*camera.getSceneSize().y/1280.0}-m_center,nullptr,nullptr,changed);
        }
        using GearQuad=std::array<Float3,4>;
        void updateGear(const BasicCamera3D& camera,const Mat4x4& transform,const GearQuad& actual,const GearQuad& reference)
        {
            if(m_disabled) {m_valid=false;return;}
            for(int i=0;i<4;++i) if(!Finite(actual[i])||!Finite(reference[i])||actual[i].z<=0||reference[i].z<=0) {m_valid=false;return;}
            const Vec2 upper=(Vec2{actual[0].xy()}+Vec2{actual[1].xy()})*.5;
            const Vec2 neutral=(Vec2{reference[0].xy()}+Vec2{reference[1].xy()})*.5;
            const bool changed=prepare(camera,transform,Float3{static_cast<float>(neutral.x),static_cast<float>(neutral.y),reference[0].z});
            if(!m_valid) return;
            bool same=m_quadCached && !changed;
            for(int i=0;i<4;++i) same=same && (actual[i]-m_lastActual[i]).lengthSq()<1e-10 && (reference[i]-m_lastReference[i]).lengthSq()<1e-10;
            if(same) return;
            m_lastActual=actual;m_lastReference=reference;m_quadCached=true;
            m_requested=upper.y-neutral.y;
            setMoves(camera,transform,upper-Vec2{0,58.5*camera.getSceneSize().y/1280.0}-m_center,&actual,&reference,changed);
        }
    private:
        bool prepare(const BasicCamera3D& camera,const Mat4x4& transform,Float3 reference) {
            const Vec2 center=Vec2{reference.xy()}-Vec2{0,58.5*camera.getSceneSize().y/1280.0};
            std::array<Float3,4> corners{};
            for(int i=0;i<4;++i) corners[i]=camera.worldToScreenPoint(transform.transformPoint(m_source.vertices[i].pos));
            bool changed=!m_cached || (center-m_center).lengthSq()>1e-10;
            for(int i=0;i<4;++i) changed=changed || (corners[i]-m_corners[i]).lengthSq()>1e-10;
            if(changed) {
                if(!m_cached) {m_valid=Build(camera,transform,center,corners);m_cached=m_valid;}
                else {
                    m_valid=true;
                    for(int i=0;i<49;++i) {
                        m_screen[i]=camera.worldToScreenPoint(transform.transformPoint(m_local[i]));
                        m_valid=m_valid && Finite(m_screen[i]) && m_screen[i].z>0;
                    }
                }
                m_corners=corners;
                if(m_valid) m_center=m_screen[0].xy();
            }
            return changed;
        }
        static bool Solve(const GearQuad& actual,const GearQuad& reference,Size size,std::array<double,8>& h)
        {
            double a[8][9]{};
            for(int i=0;i<4;++i) {
                const double x=reference[i].x/size.x,y=reference[i].y/size.y,u=actual[i].x/size.x,v=actual[i].y/size.y;
                const int r=2*i;
                a[r][0]=x;a[r][1]=y;a[r][2]=1;a[r][6]=-u*x;a[r][7]=-u*y;a[r][8]=u;
                a[r+1][3]=x;a[r+1][4]=y;a[r+1][5]=1;a[r+1][6]=-v*x;a[r+1][7]=-v*y;a[r+1][8]=v;
            }
            for(int col=0;col<8;++col) {
                int pivot=col;for(int row=col+1;row<8;++row) if(Abs(a[row][col])>Abs(a[pivot][col])) pivot=row;
                if(Abs(a[pivot][col])<1e-12) return false;
                for(int k=col;k<=8;++k) std::swap(a[col][k],a[pivot][k]);
                const double d=a[col][col];for(int k=col;k<=8;++k) a[col][k]/=d;
                for(int row=0;row<8;++row) if(row!=col) {
                    const double f=a[row][col];for(int k=col;k<=8;++k) a[row][k]-=f*a[col][k];
                }
            }
            for(int i=0;i<8;++i) {h[i]=a[i][8];if(!std::isfinite(h[i])) return false;}
            return true;
        }
        void setMoves(const BasicCamera3D& camera,const Mat4x4& transform,Vec2 centerMove,const GearQuad* actual,const GearQuad* reference,bool changed)
        {
            std::array<Vec2,49> moves{};
            for(int i=0;i<49;++i) moves[i]=centerMove*m_weights[i];
            std::array<double,8> h{};
            m_thickness=1;
            m_projective=actual && Solve(*actual,*reference,camera.getSceneSize(),h);
            if(m_projective) {
                // One tunnel: the far/near gear rows control the outer boundary.
                // Intermediate vertices are derived from endpoints, never moved independently.
                const Vec2 ru=(Vec2{(*reference)[0].xy()}+Vec2{(*reference)[1].xy()})*.5;
                const Vec2 rl=(Vec2{(*reference)[2].xy()}+Vec2{(*reference)[3].xy()})*.5;
                const Vec2 au=(Vec2{(*actual)[0].xy()}+Vec2{(*actual)[1].xy()})*.5;
                const Vec2 al=(Vec2{(*actual)[2].xy()}+Vec2{(*actual)[3].xy()})*.5;
                const double rw0=(Vec2{(*reference)[1].xy()}-Vec2{(*reference)[0].xy()}).length();
                const double rw1=(Vec2{(*reference)[3].xy()}-Vec2{(*reference)[2].xy()}).length();
                const double aw0=(Vec2{(*actual)[1].xy()}-Vec2{(*actual)[0].xy()}).length();
                const double aw1=(Vec2{(*actual)[3].xy()}-Vec2{(*actual)[2].xy()}).length();
                if(Abs(rl.y-ru.y)<1e-6 || rw0<1e-6 || rw1<1e-6) m_projective=false;
                else {
                m_thickness=Clamp(aw1/rw1,.60,1.0);
                const bool shrinking=aw1/rw1<1.0-1e-6;
                for(int i=0;i<16;++i) {
                    const Vec2 p=m_screen[33+i].xy();
                    const double t=Clamp((p.y-ru.y)/(rl.y-ru.y),0.0,1.0);
                    const Vec2 origin=ru+(rl-ru)*t;
                    const Vec2 shift=(au-ru)*(1-t)+(al-rl)*t;
                    const double scale=(aw0/rw0)*(1-t)+(aw1/rw1)*t;
                    moves[33+i]=shift+Vec2{(p.x-origin.x)*(scale-1),0};
                    if(shrinking) {
                        // Keep the cover frame. Only redistribute UV-bearing side controls.
                        const int edge=m_outerEdge[i];
                        if(m_outerT[i]<1e-5 || m_outerT[i]>1-1e-5) {moves[33+i]={0,0};continue;}
                        const std::array<int,4> order{0,1,3,2};
                        const Vec2 a=m_corners[order[edge]].xy();
                        const Vec2 line=Vec2{m_corners[order[(edge+1)%4]].xy()}-a;
                        const double lengthSq=line.lengthSq();
                        double u=m_outerT[i];
                        if(edge==1 || edge==3) {
                            const double axis=(m_center-a).dot(line)/lengthSq;
                            const double follow=centerMove.dot(line)/lengthSq;
                            u=axis+(u-axis)*m_thickness+follow;
                        } else u+=(shift.dot(line)/lengthSq);
                        moves[33+i]=a+line*Clamp(u,0.0,1.0)-p;
                    }
                }
                }
            }
            for(int i=0;i<16;++i) {
                // A source-space midpoint is not a screen midpoint under perspective.
                // Use its projected fraction so the whole ray stays straight at any roll.
                const Vec2 a=m_screen[1+i].xy(),b=m_screen[33+i].xy(),m=m_screen[17+i].xy();
                const double t=Clamp((m-a).dot(b-a)/(b-a).lengthSq(),0.0,1.0);
                moves[17+i]=moves[1+i]*(1-t)+moves[33+i]*t;
            }
            // Apply one shared safety factor: this preserves rigid center shape
            // and every straight inner/middle/outer ray even near extreme zooms.
            m_alpha=1;
            for(const auto tri:m_data.indices) {
                const auto a=tri.i0,b=tri.i1,c=tri.i2;
                const Vec2 p=m_screen[a].xy(),q=m_screen[b].xy(),r=m_screen[c].xy();
                const double area=Area(p,q,r);if(Abs(area)<1e-5) continue;
                const auto da=moves[a],db=moves[b],dc=moves[c];
                const double second=Area(da,db,dc)/area;
                const double first=(Area(p+da,q+db,r+dc)-area)/area-second;
                const auto root=[&](double v){if(v>0 && v<=1) m_alpha=Min(m_alpha,Max(0.0,v-1e-7));};
                if(Abs(second)<1e-12) {if(first<0) root(-.8/first);}
                else {
                    const double disc=first*first-3.2*second;
                    if(disc>=0) {root((-first-Sqrt(disc))/(2*second));root((-first+Sqrt(disc))/(2*second));}
                }
            }
            for(auto& move:moves) move*=m_alpha;
            bool same=!changed;
            for(int i=0;i<49;++i) same=same && (moves[i]-m_moves[i]).lengthSq()<1e-12;
            if(same) return;
            m_moves=moves;
            m_delta=m_requested*m_alpha;
            const auto size=camera.getSceneSize();
            const auto inverse=transform.inverse();
            for(int i=0;i<49;++i) {
                const Vec2 shift=moves[i];
                m_parameters->shifts[i]={static_cast<float>(2*shift.x/size.x),static_cast<float>(-2*shift.y/size.y),0,0};
                if(!m_gpu) {
                    const auto p=m_screen[i];
                    m_data.vertices[i].pos=inverse.transformPoint(camera.screenToWorldPoint(Float2{static_cast<float>(p.x+shift.x),static_cast<float>(p.y+shift.y)},p.z));
                    m_data.vertices[i].normal=m_source.vertices[0].normal;
                }
            }
            if(!m_gpu) m_mesh.fill(m_data.vertices);
        }
    public:

        bool draw(const Mat4x4& transform,const Texture& texture) const
        {
            if(!m_valid) return false;
            bool moving=false;for(const auto& v:m_moves) moving=moving||v.lengthSq()>1e-12;
            if(!moving) return false;
            if(m_gpu) {
                const ScopedCustomShader3D shader{m_shader};
                Graphics3D::SetVSConstantBuffer(4,m_parameters);
                m_mesh.draw(transform,texture);
            } else m_mesh.draw(transform,texture);
            return true;
        }
        void drawDebug() const
        {
            if(!m_valid) return;
            const auto point=[&](uint32 i) { return Vec2{m_screen[i].xy()}+m_moves[i]; };
            for(const auto tri:m_data.indices) Triangle{point(tri.i0),point(tri.i1),point(tri.i2)}.drawFrame(1,ColorF{0,1,1,.7});
            for(int i=0;i<16;++i) Line{point(1+i),point(1+(i+1)%16)}.draw(2,Palette::Hotpink);
        }
        void writeReport(FilePathView path) const {
            TextWriter writer{path};
            writer.writeln(U"valid,gpu,requested_pixels,applied_pixels,min_area_ratio,vertices,triangles,alpha,projective,middle_max_move,wall_thickness,straight_error,uv_error");
            double middle=0;for(int i=17;i<33;++i) middle=Max(middle,m_moves[i].length());
            writer.writeln(U"{},{},{},{},{},49,80,{},{},{},{},{},{}"_fmt(m_valid,m_gpu,m_requested,m_delta,minAreaRatio(),m_alpha,m_projective,middle,m_thickness,straightError(),uvError()));
        }
        double thickness() const {return m_thickness;}
        double sideThicknessRatio() const {
            double result=1;
            for(int edge:{1,3}) {
                int a=-1,b=-1;
                for(int i=0;i<16;++i) if(m_outerEdge[i]==edge) {
                    if(Abs(m_outerT[i]-.25)<1e-4) a=i;
                    if(Abs(m_outerT[i]-.75)<1e-4) b=i;
                }
                if(a>=0 && b>=0) {
                    const Vec2 pa=m_screen[33+a].xy(),pb=m_screen[33+b].xy();
                    result=Min(result,((pb+m_moves[33+b])-(pa+m_moves[33+a])).length()/(pb-pa).length());
                }
            }
            return result;
        }
        double uvError() const {double error=0;for(int i=0;i<49;++i) error=Max(error,static_cast<double>((m_sourceUV[i]-m_data.vertices[i].tex).length()));return error;}
        double straightError() const {
            double result=0;
            for(int i=0;i<16;++i) {
                const Vec2 a=Vec2{m_screen[1+i].xy()}+m_moves[1+i];
                const Vec2 b=Vec2{m_screen[33+i].xy()}+m_moves[33+i];
                const Vec2 m=Vec2{m_screen[17+i].xy()}+m_moves[17+i];
                result=Max(result,Abs(Area(a,b,m))/Max((b-a).length(),1e-6));
            }
            return result;
        }
        double middleMove() const {double value=0;for(int i=17;i<33;++i) value=Max(value,m_moves[i].length());return value;}
        double protectedShapeError() const {double value=0;for(int i=1;i<17;++i) value=Max(value,(m_moves[i]-m_moves[0]).length());return value;}
        double boundaryMove() const {double value=0;for(int i=33;i<49;++i) value=Max(value,m_moves[i].length());return value;}
        bool projective() const {return m_projective;}
        double delta() const {return m_delta;}
        double requested() const {return m_requested;}
        bool valid() const {return m_valid;}
        bool gpu() const {return m_gpu;}
        double projectionError(const BasicCamera3D& camera,const Mat4x4& transform) const
        {
            if(!m_valid) return 0;
            double result=0;
            for(int i=0;i<49;++i) {
                const auto expected=Vec2{m_screen[i].xy()}+m_moves[i];
                const auto actual=camera.worldToScreenPoint(transform.transformPoint(m_gpu ? m_local[i] : m_data.vertices[i].pos));
                const auto displayed=Vec2{actual.xy()}+(m_gpu ? m_moves[i] : Vec2{0,0});
                result=Max(result,(displayed-expected).length());
            }
            return result;
        }
        double minAreaRatio() const
        {
            double result=1;
            for(const auto t:m_data.indices) {
                const Vec2 a=m_screen[t.i0].xy(),b=m_screen[t.i1].xy(),c=m_screen[t.i2].xy();
                const double area=Area(a,b,c); if(Abs(area)<1e-5) continue;
                result=Min(result,Area(a+m_moves[t.i0],b+m_moves[t.i1],c+m_moves[t.i2])/area);
            }
            return result;
        }
    };
}
