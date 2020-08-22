#ifndef FRAME_STATS_HPP
#define FRAME_STATS_HPP


struct FrameStats {
    int     frame_id    = 0;
    int     fps         = 0;
    float   frame_time  = 0;
    float   time        = 0;
};

extern FrameStats gFrameStats;

#endif
