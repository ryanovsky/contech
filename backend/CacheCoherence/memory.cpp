#include "memory.hpp"

Memory::Memory(Time *t){
  timer = t;
}

void Memory::flush(){
  timer->time ++;
}

void Memory::load(){
  timer->time ++;
}
