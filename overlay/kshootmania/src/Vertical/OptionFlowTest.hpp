#pragma once
#include "VerticalDisplay.hpp"
namespace Vertical::OptionProbe {
    inline int stage=0;
    inline bool passed=true;
    inline bool active() {static const bool value=System::GetCommandLineArgs().contains(U"--vertical-options-test");return value;}
    inline Stopwatch timer{StartImmediately::Yes};
    inline void record(StringView name,bool ok) {
        static TextWriter writer{U"vertical-options-flow-test.csv"};
        if(stage==0) writer.writeln(U"check,status");
        passed=passed&&ok;writer.writeln(U"{},{}"_fmt(name,ok?U"PASS":U"FAIL"));

    }
    inline void advance(int next) {stage=next;timer.restart();}
    inline bool done() {return stage==13;}
    inline void update() {
        if(!active()) return;
        if(stage==18 && timer.sF()<.30)KeyConfig::RequestVirtualDown(kButtonStart);
        if((stage==7||stage==9||stage==11)&&IsDisplayChangePending()&&timer.sF()>.8) {
            KeyConfig::RequestVirtualDown(stage==9 ? kButtonBack : kButtonStart);timer.restart();
        }
        if(timer.sF()>20) {record(U"timeout",false);record(U"overall",false);stage=13;}
    }
}
