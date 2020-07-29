#pragma once

enum Phase
{
    Calibrating,
    DrivingNormal,
    DrivingSpin,
    Pause,
    End,
    Failure
};

bool isDrivingPhase(Phase p) { return p == DrivingNormal || p == DrivingSpin; }
