// Opening-book generator: walks the magician's strategy tree.
// Magician (first player) plays the best move; every spectator reply is explored.
// Usage: gen MAXPLY PREFIX   (PREFIX = moves already played, e.g. "43")
// Output lines: <move sequence> <best column 0-6> <score>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "solver.c"

#define SET (1u<<22)
static u64 skey[SET]; static signed char sval[SET]; static signed char sscore[SET];
static int lookup(u64 k,int*sc){ u32 i=(u32)(k*0x9E3779B97F4A7C15ULL>>42)&(SET-1); while(skey[i]){ if(skey[i]==k){*sc=sscore[i];return sval[i];} i=(i+1)&(SET-1);} return -1;}
static void store(u64 k,int v,int sc){ u32 i=(u32)(k*0x9E3779B97F4A7C15ULL>>42)&(SET-1); while(skey[i]&&skey[i]!=k) i=(i+1)&(SET-1); skey[i]=k; sval[i]=v; sscore[i]=sc;}

static int maxply; static char seq[64]; static long count;
static void rec(Pos P,int n){
  // magician to move
  if(P.moves>maxply) return;
  int sc, b=lookup(key(&P),&sc);
  if(b<0){ g=P; b=c4_best(); sc=c4_score(); store(key(&P),b,sc); }
  seq[n]=0; printf("%s %d %d\n",seq,b,sc); count++;
  if(is_winning_move(&P,b)) return;
  play_bit(&P,col_move(&P,b)); seq[n]='1'+b;
  if(P.moves>=W*H) return;
  for(int c=0;c<W;c++){
    if(!can_play(&P,c)) continue;
    if(is_winning_move(&P,c)) continue; // spectator win: cannot happen on a won line
    Pos Q=P; play_bit(&Q,col_move(&Q,c)); seq[n+1]='1'+c;
    if(Q.moves>=W*H) continue;
    rec(Q,n+2);
  }
  if(P.moves<=7){ fprintf(stderr,"done %.*s count=%ld\n",n+1,seq,count); fflush(stdout);} 
}
int main(int argc,char**argv){
  maxply=atoi(argv[1]); const char*pre=argc>2?argv[2]:"";
  Pos P={0,0,0}; int n=0;
  for(const char*p=pre;*p;p++){ play_bit(&P,col_move(&P,*p-'1')); seq[n++]=*p; }
  rec(P,n);
  fprintf(stderr,"total %ld\n",count);
}
