#ifndef pipe_stage_H
#define pipe_stage_H

#include <cstdio>
#include <iostream>
#include <cstdlib>
#include "sim_proc.h"
using namespace std;

typedef struct {
  int I_order;    // i1 - i2 - i3 ...
  int oprand;     // arithmatic -> add multi branch
  int rs1;
  int rs2;
  int dst;
}input_component;

typedef struct {
  int I_order;    // i1 - i2 - i3 ...
  int oprand;     // arithmatic -> add multi branch
  int rs1;
  int rs2;
  int dst;
  bool using_rob1;
  bool using_rob2;
}stage;

typedef struct {
  int   I_order;
  int   ROB_tag;
  bool  v_avail;  // value_available
}WB_component;

typedef struct {
  int   ROB_tag;
  bool  v;     // v = false -> ARF, v = true -> Rob
}RMT_component;

typedef struct {
  unsigned long int value;
  bool  v_avail;  // value_available
  int   I_order;
  int   dst;
  bool  Ready;
  int src1;
  int src2;
  int op;
}ROB_component;

class ROB_FIFO{
  public:
    unsigned long int SP_scaler_width;
    unsigned long int ISqueue_size;
    unsigned long int ROB_size;
    ROB_component*    rob_comp;
    int   Head;
    int   Tail;
    bool  full;

    ROB_FIFO(){};
    ROB_FIFO(proc_params parami);
    
    void rob_retire();
    int find_next(int);
    bool Rob_empty();
    void RN_rob_assign(int, int, bool, int, int, int);
};

typedef struct {
  int   I_order;
  bool  v;
  bool  rasie_hand;
  int   dst;
  int   op;
  int   rs1_tag;
  bool  rs1_rdy;
  int   rs2_tag;
  bool  rs2_rdy;
}ISQ_component;

class Issue_Queue{
  public:
    unsigned long int SP_scaler_width;
    unsigned long int ISqueue_size;
    unsigned long int ROB_size;
    
    unsigned long int largest_order;

    ISQ_component*    isq_comp;
    
    Issue_Queue(){};
    Issue_Queue(proc_params parami);

    int empty_isq();
    bool iq_avail_for_next();
    int find_largest_order();
    int find_raise_hand();
    void EXfu_bypass_IQ(int);
    bool is_bypassing(int, int);
    
    
};

typedef struct {
  int   I_order;
  int   cycle_left;
  int   dst;
}EX_FU_component;


class Execute_FU{
  public:
    unsigned long int SP_scaler_width;
    unsigned long int ISqueue_size;
    unsigned long int ROB_size;
    EX_FU_component*    ex_fu_comp;
    
    Execute_FU(){};
    Execute_FU(proc_params parami);

    int empty_fu();
    void IS_to_EXfu(int, int, int);
   
};

typedef struct {
  bool FE;
  bool DE;
  bool RN;
  bool RR;
  bool DI;
  bool IS;
  bool EX;
  bool WB;
  bool RT;
}Empty_stage;

typedef struct {
  int start;
  int period;
}stage_cycle;


typedef struct {
  stage_cycle FE;
  stage_cycle DE;
  stage_cycle RN;
  stage_cycle RR;
  stage_cycle DI;
  stage_cycle IS;
  stage_cycle EX;
  stage_cycle WB;
  stage_cycle RT;
}stage_info;

class OOO_cpu{
  
  public:
    unsigned long int SP_scaler_width;
    unsigned long int ISqueue_size;
    unsigned long int ROB_size;
    unsigned long int one_circulation;
    int ARF_size;
    uint64_t cycle;
    bool cpu_complete;

    unsigned long int total_cycle;
    unsigned long int D_IC;
    float ipc;

    stage* DCode;
    stage* Rname;
    stage* RGread;
    stage* DSpactch;
    ROB_FIFO Rob;
    RMT_component* RMT;
    WB_component* WrBack;
    Issue_Queue IsQueue;
    Execute_FU ExFu;

    stage_info* result;

    Empty_stage Emp_stage;

    OOO_cpu(){};
    OOO_cpu(proc_params parami);


    void Retire();
    void Writeback();
    void Execute();
    void Issue();
    void Dispatch();
    void RegRead();
    void Rename();
    void Decode();
    void Fetch(input_component*);
    bool Advance_Cycle();
    bool value_bypass(int);
    bool RR_val_is_rdy_check(int);
    void print_result(int, int, int, int, int, int);
    void reset(int);




    void parami_print();

};

#endif


