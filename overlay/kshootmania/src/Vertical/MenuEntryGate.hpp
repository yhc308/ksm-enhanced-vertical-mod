#pragma once
namespace Vertical {
class MenuEntryGate {
    bool m_armed=false;
public:
    // Ignore the opening press (including virtual Down) and held mouse/key
    // repeats until all activation inputs are released after the entry fade.
    bool accept(double elapsed,bool held,bool down) {
        if(m_armed)return true;
        if(elapsed>=.15 && !held && !down)m_armed=true;
        return false;
    }
};
}
