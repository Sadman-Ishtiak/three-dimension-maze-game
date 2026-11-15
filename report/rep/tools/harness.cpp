#include "core.cpp"
#include <cstdio>
static void dump(const char*fn, const vec3&p,double yaw){
  vector<Uint32> b((size_t)SCREEN_WIDTH*SCREEN_HEIGHT);
  render(b,vec3(p.x(),p.y(),EYE_HEIGHT),yaw);
  double temp=temperature(p);
  FILE*f=fopen(fn,"wb");fprintf(f,"P6\n%d %d\n255\n",SCREEN_WIDTH,SCREEN_HEIGHT);
  for(auto px:b){unsigned char c[3]={(unsigned char)(px>>16),(unsigned char)(px>>8),(unsigned char)px};fwrite(c,1,3,f);}
  fclose(f); printf("%s temp=%+.3f\n",fn,temp);
}
int main(){
  for(int z=0;z<MAZE_H;z++)for(int x=0;x<MAZE_W;x++){if(MAP[z][x]=='S'){start_x=x;start_z=z;}if(MAP[z][x]=='E'){exit_x=x;exit_z=z;}}
  build_exit_dist(); printf("max_dist=%d\n",max_dist);
  dump("v1.ppm",vec3(1.5,1.5,0.5),0.0);                 // spawn, facing +x
  dump("v2.ppm",vec3(5.5,3.5,0.5),PI/2*0+1.57);         // mid maze
  dump("v3.ppm",vec3(6.5,11.5,0.5),-PI/2);              // facing up the corridor toward exit column
  dump("v4.ppm",vec3(4.5,9.5,0.5),0.0);                 // looking at exit tile in corridor (x=3..5,y=9)
  dump("v5.ppm",vec3(9.5,13.5,0.5),PI);                 // far / cold area
  // collision tests
  printf("collides spawn=%d inside wall=%d near wall(1.5,1.2)=%d (1.5,1.3)=%d\n",collides(vec3(1.5,1.5,.5)),collides(vec3(0.5,0.5,.5)),collides(vec3(1.5,1.2,.5)),collides(vec3(1.5,1.3,.5)));
  printf("temp start=%+.3f exit=%+.3f\n",temperature(vec3(1.5,1.5,.5)),temperature(vec3(6.5,9.5,.5)));
  // timing
  vector<Uint32> b((size_t)SCREEN_WIDTH*SCREEN_HEIGHT);
  auto t0=chrono::steady_clock::now(); for(int i=0;i<30;i++) render(b,vec3(5.5,3.5,.5),i*0.1);
  double ms=chrono::duration<double,milli>(chrono::steady_clock::now()-t0).count()/30; printf("render ms/frame=%.2f\n",ms);
}
