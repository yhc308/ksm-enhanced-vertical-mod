#pragma once
#include "TimingFeedback.hpp"
#include "Judgment/InputTimingStats.hpp"
#include "Judgment/DetailedJudgmentStats.hpp"
#include <deque>

namespace MusicGame {
// A live draft belongs to one settings screen. Normal games never receive it.
struct JudgeSettingValues {
    FastSlowMode mode=FastSlowMode::kHide;
    TimingHudSettings hud;
    int32 input=0,visual=0,laser=0;
    bool operator==(const JudgeSettingValues& v) const {
        return mode==v.mode && hud.judgeColor==v.hud.judgeColor && hud.position==v.hud.position
            && hud.display==v.hud.display && hud.color==v.hud.color
            && input==v.input && visual==v.visual && laser==v.laser;
    }
};
struct JudgeSettingSession {
    static constexpr double kLoopSeconds=16.0,kRecentSeconds=10.0,kGuardSeconds=.2;
    JudgeSettingValues values;
    bool menu=false,focused=true,rebase=false;
    uint64 loops=0,measurementRevision=0;
    double time=0;
    Judgment::DetailedJudgmentStats counts;
    Judgment::InputTimingStats cumulative;
    struct Sample {double time,ms;};
    std::deque<Sample> recent;
    double recentSum=0;
    Optional<double> last,pendingLast,recentDisplay;
    bool enabled() const {return !menu && focused;}
    void clearRecent(){recent.clear();recentSum=0;pendingLast.reset();}
    void clearMeasurements(){clearRecent();counts={};cumulative={};++measurementRevision;}
    void advance(double delta){
        if(focused)time+=Max(0.0,delta);
        if(!menu)while(!recent.empty() && recent.front().time<=time-kRecentSeconds){recentSum-=recent.front().ms;recent.pop_front();}
        pendingLast.reset();
    }
    void chip(Judgment::JudgmentResult result,Optional<double> ms){
        if(!enabled())return;
        counts.add(result,ms);cumulative.add(result,ms);
        if(!ms || !std::isfinite(*ms))return;
        if(!pendingLast || Abs(*ms)>Abs(*pendingLast))pendingLast=*ms;
        if(result==Judgment::JudgmentResult::kCritical || IsTimingNear(result)){
            recent.push_back({time,*ms});recentSum+=*ms;
        }
    }
    void tick(Judgment::JudgmentResult result){if(enabled())counts.add(result);}
    void finishFrame(){if(pendingLast){last=pendingLast;recentDisplay=recent.empty()?none:Optional<double>{recentSum/recent.size()};}}
    Optional<double> recentMean() const {return recentDisplay;}
    bool itemEnabled(int32 index) const {return index==0 || index>=5 || values.mode==FastSlowMode::kNew;}
    int32 navigate(int32 index,int32 delta) const {
        constexpr std::array<int32,10> order{1,2,3,4,0,5,6,7,8,9};
        int32 position=0;for(int32 i=0;i<10;++i)if(order[i]==index)position=i;
        const int32 direction=Sign(delta);
        for(int32 n=0;n<Abs(delta);++n){int32 next=position+direction;
            while(next>=0 && next<10 && !itemEnabled(order[next]))next+=direction;
            if(next<0 || next>=10)break;position=next;
        }return order[position];
    }

    void change(int32 index,int32 delta){
        if(!itemEnabled(index) || !delta)return;
        const auto edit=[delta](auto& value,int32 maximum){using T=std::decay_t<decltype(value)>;value=static_cast<T>(Clamp(static_cast<int32>(value)+delta,0,maximum));};
        const auto before=values;
        if(index==0)edit(values.mode,2);
        if(index==1)edit(values.hud.judgeColor,1);
        if(index==2)edit(values.hud.color,3);
        if(index==3)values.hud.display=TimingDisplayChoice(TimingDisplayCursor(values.hud.display)+delta);
        if(index==4)edit(values.hud.position,3);
        if(index>=5 && index<=7){auto& value=index==5?values.input:index==6?values.visual:values.laser;value=Clamp(value+delta,-10000,10000);}
        if(before.input!=values.input || before.visual!=values.visual || before.laser!=values.laser){clearMeasurements();rebase=true;}
    }
};
}
