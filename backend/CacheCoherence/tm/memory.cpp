#include "memory.hpp"

Memory::Memory(Time *t){
  timer = t;
}

void Memory::flush(){
  timer->time += 1;
}

void Memory::load(){
  timer->time += 1;
}
