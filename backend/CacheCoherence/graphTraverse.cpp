#include "graphTraverse.hpp"
#include <stdio.h>

using namespace contech;

GraphTraverse::GraphTraverse(char* fname)
{
  assert(fname != NULL);

  ct_file* taskGraphIn  = create_ct_file_r(fname);
  if (taskGraphIn == NULL) {
    fprintf(stderr, "ERROR: Couldn't open input file\n");
    exit(1);
  }

  tg = TaskGraph::initFromFile(taskGraphIn);

  done = false;
}

GraphTraverse::~GraphTraverse()
{
  delete tg;
}

// getNextMemoryRequest
//   Populates a MemReq struct with the next memory request
//   returns 0 if no request is returned
int GraphTraverse::getNextMemoryRequest(MemReqContainer &nextReq)
{
  if (memReqQ.empty())
  {
    if(done) return 0;
    else {
      populateQueue();
      done = true;
    }
  }
  nextReq = memReqQ.front();
  memReqQ.pop_front();

  return 1;
}

//
// Scan the vector of successor tasks and find the next in sequence
//
TaskId GraphTraverse::getSequenceTask(vector<TaskId>& succ, ContextId selfId)
{
  TaskId possible_succ = 0;

  for (auto i : succ)
  {
    if (i.getContextId() == selfId)
    {
      if (possible_succ == 0 ||
          i < possible_succ)
      {
        possible_succ = i;
      }
    }
  }

  return possible_succ;
}

// populateQueue
//   Populates the memory request queue with additional requests



/** pseudo code
 *
 * while (size of map > 0){
 *   1)if(a task is finished, or the map does not have a current task, put one in the map getSequence task
 *   2)If current task is sync create, join or barrier(then skip), if was type barrier then pause
 *   3) if task is paused check predecessors are complete, if so -> unpause
 *   2)loop through map, if !paused send one request at a time
 * }
 *
 */
//initialize map to hold values for first task

int GraphTraverse::populateQueue()
{
  int addedMemOps = 0;
  ContextId ctid;
  bool barrier_complete;
  SeqId sqid;
  TaskId taskId;
  Task* currentTask;
  for(int i = 0; i < tg->getNumberOfContexts(); i++){
    ctid = (uint32_t)i;
    sqid = (uint32_t)0;
    ctid_current_state* tempState = new ctid_current_state;
    taskId = TaskId(ctid,sqid);
    currentTask = tg->getTaskById(taskId);
    tempState->currentTask = currentTask;
    tempState->currentBBCol = currentTask->getBasicBlockActions();
    tempState->currentBB = tempState->currentBBCol.begin();
    tempState->terminated = false;
    contechState[ctid] = tempState;
  }
  assert(contechState.size() == tg->getNumberOfContexts());
  ctid_current_state* tempState;
  while(contechState.size() > 0){ //1 while loop
    for (auto begin = contechState.begin(), end = contechState.end(); begin != end; ++begin){ //2 for loop
      tempState = (begin->second);

      //Task* t = tempState->currentTask;
      if(tempState->terminated){
        contechState.erase(begin);
        continue;
      }

      //check if task is not type basic block
      if(tempState->currentTask->getType() == task_type_create ||
          currentTask->getType() == task_type_join ||
          (currentTask->getType() == task_type_join &&
           currentTask->getSyncType() != sync_type_lock)){ // 3 if statement
        //skip these tasks
        taskId = getSequenceTask(
            tempState->currentTask->getSuccessorTasks(),
            tempState->currentTask->getContextId());
        currentTask = tg->getTaskById(taskId);
        tempState->currentTask = currentTask;
        if(currentTask->getTaskId() == 0){
          tempState->terminated = true;
          contechState.erase(begin);
          continue;
        }
        tempState->currentBBCol = currentTask->getBasicBlockActions();
        tempState->currentBB = tempState->currentBBCol.begin();
        tempState->terminated = false;
      }// close 3 if statement
      //if the task is blocked check if can be unblocked
      else if(tempState->currentTask->getType() == task_type_barrier){ // 3 else if
        barrier_complete = true;
        auto pred = tempState->currentTask->getPredecessorTasks();
        for (auto it = pred.begin(), et = pred.end(); it != et; ++it){ //4 for loop
          //it and et are iterators to a vector
          if(tempState->currentTask->getTaskId() < *it){
            barrier_complete = barrier_complete && false;
          }
        }
        if(barrier_complete) { // can move on to the next task
          taskId = getSequenceTask(
              tempState->currentTask->getSuccessorTasks(),
              tempState->currentTask->getContextId());
          currentTask = tg->getTaskById(taskId);
          tempState->currentTask = currentTask;
          if(currentTask->getTaskId() == 0){
            tempState->terminated = true;
            contechState.erase(begin);
            continue;
          }
          tempState->currentBBCol = currentTask->getBasicBlockActions();
          tempState->currentBB = tempState->currentBBCol.begin();
          tempState->terminated = false;
        }
      }
      auto f = tempState->currentBB;
      if (tempState->currentBB == tempState->currentBBCol.end()) {//at the end of the basic block collection. Need a new task
        taskId = getSequenceTask(
            tempState->currentTask->getSuccessorTasks(),
            tempState->currentTask->getContextId());
        currentTask = tg->getTaskById(taskId);

        if(currentTask->getTaskId() == 0) {
          tempState->terminated = true;
          contechState.erase(begin);
          continue;
        }
        tempState->currentTask = currentTask;
        tempState->currentBBCol = currentTask->getBasicBlockActions();
        tempState->currentBB = tempState->currentBBCol.begin();
        contechState[ctid] = tempState;
      }
      else if(tempState->currentTask->getType() != task_type_barrier){ // send one memory request from basic block if not type barrier
        BasicBlockAction tbb = *f;
        // Push MemOps onto queue
        assert(tempState->currentTask->getType() == task_type_basic_blocks);
        auto memOps = f.getMemoryActions();
        MemReqContainer tReq;
        tReq.mav.clear();
        tReq.bbid = tbb.basic_block_id;
        tReq.locked = false;
        tReq.ctid = (unsigned int) tempState->currentTask->getContextId();
        assert(tReq.ctid >= 0 && tReq.ctid < tg->getNumberOfContexts());
        uint32_t pushedOps = 0;
        for (auto iReq = memOps.begin(), eReq = memOps.end(); iReq != eReq; ++iReq)
        {
          MemoryAction ma = *iReq;
          if (ma.type == action_type_mem_read || action_type_mem_write)
          {
          }
          else if (ma.type == action_type_memcpy)
          {
            tReq.mav.push_back(ma);
            ++iReq;
            pushedOps++;
            ma = *iReq;
            if (ma.type == action_type_memcpy)
            {
              tReq.mav.push_back(ma);
              ++iReq;
              pushedOps++;
              ma = *iReq;
            }
          }
          else if (ma.type == action_type_malloc)
          {
            tReq.mav.push_back(ma);
            ++iReq;
            pushedOps++;
            ma = *iReq;
          }
          pushedOps++;
          tReq.mav.push_back(ma);
          if(tempState->currentTask->getType() == task_type_sync){
            tReq.locked = true;
            assert(pushedOps == 1);
            assert(tempState->currentTask->getSyncType() == sync_type_lock);
          }
        }
        assert(pushedOps == tReq.mav.size());

        if (pushedOps != 0)
        {
          memReqQ.push_back(tReq);
          addedMemOps += pushedOps;
        }
        ++(tempState->currentBB);
      }//close else if
    }
  }
  return addedMemOps;
}
