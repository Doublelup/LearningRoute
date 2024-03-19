#include"lab0.h"


uint64_t time(){
    return io_read(AM_TIMER_UPTIME).us;
}

int main(){
    ioe_init();
    graph_init();
    background_init();
    ball_init();
    keycode code=KEY(NONE);
    uint64_t frame_new=time()/10000;
    uint64_t frame_old=frame_new;
    while(1){
        background();
        ball_paint();
        code=read_keyboard();
        if(code==KEY(ESCAPE))break;
        ball(code);
        ball_paint();
        while(frame_old==frame_new){
            frame_new=time()/10000;
        }
        frame_old=frame_new;
    }
    halt(0);
    return 0;
}
