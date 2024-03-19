#ifndef GRAPH
#define GRAPH
#include<am.h>
#include<amdev.h>
#include<klib-macros.h>
#include<keyboard.h>

#define NUMW 20
#define NUMH 20

struct  screen{
    bool state;
    int width;
    int height;
}screen;

#define rgb(red,green,blue) ((red)*0x10000+(green)*0x100+(blue))
uint32_t color1=rgb(240,127,153),color2=rgb(125,249,252),ballcolor=rgb(81,79,245);

void graph_init(){
    AM_GPU_CONFIG_T info;
    ioe_read(AM_GPU_CONFIG,&info);
    screen.state=info.present;
    screen.width=info.width;
    screen.height=info.height;
    printf("width:%d,height:%d\n",screen.width,screen.height);
    return;
}

void print(int x,int y,int w,int h,uint32_t color){
    uint32_t pixels[w*h];
    for(int i=0;i<w*h;i++){
        pixels[i]=color;
    }
    io_write(AM_GPU_FBDRAW,x,y,pixels,w,h,1);
    return;
}

struct{
    int width;
    int height;
    int w;
    int h;
    uint32_t color1,color2;
}background_info;

void background_init(){
    background_info.w=screen.width/NUMW;
    background_info.h=screen.height/NUMH;
    background_info.width=NUMW*background_info.w;
    background_info.height=NUMH*background_info.h;
    background_info.color1=color1;
    background_info.color2=color2;
    printf("%d\t%d\t%d\t%d\n",background_info.w,background_info.h,background_info.color1,background_info.color2);
    return;
}

void background(){
    int i=0,j=0;
    for(i=0;i<NUMW;i++){
        for(j=0;j<NUMH;j++){
            if((i&1)==(j&1))print(i*background_info.w,j*background_info.h,background_info.w,background_info.h,background_info.color1);
            else print(i*background_info.w,j*background_info.h,background_info.w,background_info.h,background_info.color2);
        }
    }
    return;
}

struct{
    int x;
    int y;
    int width;
    int height;
    int v_x;
    int v_y;
    uint32_t color;
}ball_info;

void ball_init(){
    ball_info.x=0;
    ball_info.y=0;
    ball_info.width=background_info.height/(NUMW+1);
    ball_info.height=ball_info.width;
    ball_info.v_x=0;
    ball_info.v_y=0;
    ball_info.color=ballcolor;
    printf("%d %d\n",ball_info.width,ball_info.height);
    return;
}

#define MINIV 1

void ball(keycode code){
    if(code==KEY(W)){
        ball_info.v_y-=MINIV;
    }
    else if(code==KEY(S)){
        ball_info.v_y+=MINIV;
    }
    else if(code==KEY(A)){
        ball_info.v_x-=MINIV;
    }
    else if(code==KEY(D)){
        ball_info.v_x+=MINIV;
    }
    ball_info.x+=ball_info.v_x;
    ball_info.y+=ball_info.v_y;
    if(ball_info.x>background_info.width-ball_info.width){
        ball_info.x=2*background_info.width-ball_info.x-2*ball_info.width;
        ball_info.v_x=-ball_info.v_x;
    }
    if(ball_info.x<0){
        ball_info.x=-ball_info.x;
        ball_info.v_x=-ball_info.v_x;
    }
    if(ball_info.y>background_info.height-ball_info.height){
        ball_info.y=2*background_info.height-ball_info.y-2*ball_info.height;
        ball_info.v_y=-ball_info.v_y;
    }
    if(ball_info.y<0){
        ball_info.y=-ball_info.y;
        ball_info.v_y=-ball_info.v_y;
    }
    return;
}

void ball_paint(){
	print(ball_info.x,ball_info.y,ball_info.width,ball_info.height,ball_info.color);
	return;
}

#endif
