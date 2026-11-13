// Plays many games: magician (first, book + solver) vs a spectator.
// Spectator styles: 0 = random, 1 = "smart" (wins/blocks if it can, else random),
//                   2 = strongest (perfect resistance once past the opening; random before ply 9).
// Usage: sim BOOKDIR NGAMES STYLE SEED
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "solver.c"
#define HS (1<<20)
static char hk[HS][16]; static signed char hv[HS]; static int hn;
static unsigned hsh(const char*s){unsigned h=2166136261u; while(*s){h^=(unsigned char)*s++; h*=16777619u;} return h&(HS-1);}
static void put(const char*s,int v){unsigned i=hsh(s); while(hk[i][0]||hv[i]){ if(!strcmp(hk[i],s)) return; i=(i+1)&(HS-1);} strcpy(hk[i],s); hv[i]=v+1; hn++;}
static int get(const char*s){unsigned i=hsh(s); while(hk[i][0]||hv[i]){ if(!strcmp(hk[i],s)) return hv[i]-1; i=(i+1)&(HS-1);} return -1;}
int main(int argc,char**argv){
  char path[256], seq[64], line[128];
  put("",3);
  for(int c=1;c<=7;c++){ snprintf(path,sizeof path,"%s/b4%d.txt",argv[1],c); FILE*f=fopen(path,"r"); if(!f){perror(path);return 1;}
    while(fgets(line,sizeof line,f)){ int b,s; if(sscanf(line,"%63s %d %d",seq,&b,&s)==3) put(seq,b);} fclose(f);}
  int N=atoi(argv[2]), style=atoi(argv[3]); srand(atoi(argv[4]));
  int wins=0, draws=0, losses=0, offbook=0; long totlen=0;
  for(int gi=0;gi<N;gi++){
    c4_reset(); int n=0; seq[0]=0; int result=0;
    while(1){
      int c;
      if(g.moves%2==0){ c=get(seq); if(c<0){ if(g.moves<=12) offbook++; c=c4_best_limited(0);} }
      else {
        c=-1;
        if(style>=1){ for(int x=0;x<7;x++) if(c4_can_play(x)&&is_winning_move(&g,x)) c=x;
          if(c<0){ Pos o=g; o.cur^=o.mask; u64 t=winning_spots(o.cur,o.mask)&possible(&g); for(int x=0;x<7;x++) if(t&column_mask(x)) c=x; } }
        if(c<0 && style==2 && g.moves>=9) c=c4_best_limited(0);
        while(c<0||!c4_can_play(c)) c=rand()%7;
      }
      int w=is_winning_move(&g,c); c4_play(c); seq[n++]='1'+c; seq[n]=0;
      if(w){ result = (g.moves%2==1)?1:-1; break; }
      if(g.moves==42){ result=0; break; }
    }
    if(result>0) wins++; else if(result==0) draws++; else { losses++; printf("LOSS %s\n",seq); }
    totlen+=n;
  }
  printf("book=%d style=%d games=%d wins=%d draws=%d losses=%d offbook(<=ply12)=%d avg_len=%.1f\n",hn,style,N,wins,draws,losses,offbook,(double)totlen/N);
}
