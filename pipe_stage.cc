#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <math.h>
#include <string.h>
#include <iomanip>
#include <inttypes.h>
#include <tuple>
#include <list>
#include <vector>
#include <bitset>
#include <string>
#include "pipe_stage.h"
#include "sim_proc.h"
using namespace std;

ROB_FIFO::ROB_FIFO(proc_params parami){
    SP_scaler_width = parami.width;
    ISqueue_size = parami.iq_size;
    ROB_size = parami.rob_size;
    
    Head = Tail = parami.rob_size - 1;
    full = false;

    rob_comp = new ROB_component[ROB_size];
    for(unsigned int i=0; i < ROB_size; i++) {
        rob_comp[i].v_avail = false;
        rob_comp[i].value = 0;
        rob_comp[i].I_order = -1;
        rob_comp[i].dst = -1;
        rob_comp[i].Ready = false;
    }

}

int ROB_FIFO::find_next(int pos){
    int Next;
    Next = (pos + 1) % ROB_size;
    return Next;
}

void ROB_FIFO::rob_retire(){
    Head = find_next(Head);
    full = false;
}

bool ROB_FIFO::Rob_empty(){
    if(full){
        return false;
    }else {
        int f_full = Tail;
        for(unsigned long int i=0; i < (SP_scaler_width-1); i++){
            f_full = find_next(f_full);
            if(Head == f_full){
                return false;
            }
        }
        return true;
    }
}

void ROB_FIFO::RN_rob_assign(int order, int dst, bool rdy, int src1, int src2, int op){
    rob_comp[Tail].I_order = order;
    rob_comp[Tail].dst = dst;
    rob_comp[Tail].Ready = rdy;
    rob_comp[Tail].src1 = src1;
    rob_comp[Tail].src2 = src2;
    rob_comp[Tail].op = op;

    Tail = find_next(Tail);
    full = (Tail == Head);
}

Issue_Queue::Issue_Queue(proc_params parami){
    SP_scaler_width = parami.width;
    ISqueue_size = parami.iq_size;
    ROB_size = parami.rob_size;
    
    largest_order = 1;

    isq_comp = new ISQ_component[ISqueue_size];
    for(unsigned long int i=0; i < ISqueue_size; i++) {
        isq_comp[i].I_order = -1;
        isq_comp[i].v = false;
        isq_comp[i].rasie_hand = false;
        isq_comp[i].op = -1;
        isq_comp[i].dst = -1;
        isq_comp[i].rs1_tag = -1;
        isq_comp[i].rs1_rdy = false;
        isq_comp[i].rs2_tag = -1;
        isq_comp[i].rs2_rdy = false;
    }
}

int Issue_Queue::empty_isq(){
    for(unsigned long int i=0;i<ISqueue_size;i++){
        if(isq_comp[i].v == false)
            return (int) i;
    }
    return -1;
}

bool Issue_Queue::iq_avail_for_next(){
    int count = 0;
    for(unsigned long int i=0;i<ISqueue_size;i++){
        if(isq_comp[i].v == false){
            count++;
        }
        
    }
    if(count >= SP_scaler_width){
        return true;
    }else{
        return false;
    }

}



int Issue_Queue::find_largest_order(){
    int max = -1;
    for(unsigned long int i=0;i<ISqueue_size;i++){
        if(isq_comp[i].v){
            if(isq_comp[i].I_order > max){
                max = isq_comp[i].I_order;
            }
        }
    }
    return max;
}


int Issue_Queue::find_raise_hand(){
    int min = largest_order;
    int raise_pos = -1;
    for(unsigned long int i=0;i<ISqueue_size;i++){
        if(isq_comp[i].v && isq_comp[i].rasie_hand){
            if((isq_comp[i].I_order != -1) && (isq_comp[i].I_order < min)){
                min = isq_comp[i].I_order;
                raise_pos = i;
            } 
        }
    }
    if(raise_pos == -1){
        return -1;
    }else{
        return raise_pos;
    }
}

void Issue_Queue::EXfu_bypass_IQ(int bypass_tag){
    for(unsigned long int i=0;i<ISqueue_size;i++){
        if((isq_comp[i].v) && (!isq_comp[i].rasie_hand)){
            if(!isq_comp[i].rs1_rdy){
                isq_comp[i].rs1_rdy = is_bypassing(isq_comp[i].rs1_tag, bypass_tag);
            }
            if(!isq_comp[i].rs2_rdy){
                isq_comp[i].rs2_rdy = is_bypassing(isq_comp[i].rs2_tag, bypass_tag);
            }
            if(isq_comp[i].rs1_rdy && isq_comp[i].rs2_rdy){
                isq_comp[i].rasie_hand = true;
            }
        }
    }
}

bool Issue_Queue::is_bypassing(int a, int bypass){
    if(a == bypass){
        return true;
    }else{
        return false;
    }
}



Execute_FU::Execute_FU(proc_params parami){
    SP_scaler_width = parami.width;
    ISqueue_size = parami.iq_size;
    ROB_size = parami.rob_size;

    ex_fu_comp = new EX_FU_component[5*SP_scaler_width];
    for(unsigned int i=0; i < 5*SP_scaler_width; i++) {
        ex_fu_comp[i].I_order = -1;
        ex_fu_comp[i].dst = -1;
        ex_fu_comp[i].cycle_left = -1;
    }
}

int Execute_FU::empty_fu(){
    for(unsigned long int i=0;i<5*SP_scaler_width;i++){
        int cycle = ex_fu_comp[i].cycle_left;
        if((cycle == 0) || (cycle == -1)){
            return i;
        }
    }
    return -1;
}

void Execute_FU::IS_to_EXfu(int I_order, int oprand, int dsti){
    
    int pos = empty_fu();

    if(oprand == 2){
        ex_fu_comp[pos].cycle_left = 5;
    }else if(oprand == 1){
        ex_fu_comp[pos].cycle_left = 2;
    }else if(oprand == 0){
        ex_fu_comp[pos].cycle_left = 1;
    }

    ex_fu_comp[pos].I_order = I_order;
    ex_fu_comp[pos].dst = dsti;
    
}



OOO_cpu::OOO_cpu(proc_params parami){
    SP_scaler_width = parami.width;
    ISqueue_size = parami.iq_size;
    ROB_size = parami.rob_size;
    ARF_size = 67;
    cycle = 0;
    cpu_complete = false;

    total_cycle = -1;
    D_IC = -1;
    ipc = 0;

    DCode = new stage[SP_scaler_width];
    Rname = new stage[SP_scaler_width];
    RGread = new stage[SP_scaler_width];
    DSpactch = new stage[SP_scaler_width];
    for(unsigned int i=0; i < SP_scaler_width; i++) {
        DCode[i].I_order = -1;
        DCode[i].oprand = -1;
        DCode[i].rs1 = -1;
        DCode[i].rs2 = -1;
        DCode[i].dst = -1;
        DCode[i].using_rob1 = false;
        DCode[i].using_rob2 = false;


        Rname[i].I_order = -1;
        Rname[i].oprand = -1;
        Rname[i].rs1 = -1;
        Rname[i].rs2 = -1;
        Rname[i].dst = -1;
        Rname[i].using_rob1 = false;
        Rname[i].using_rob2 = false;

        RGread[i].I_order = -1;
        RGread[i].oprand = -1;
        RGread[i].rs1 = -1;
        RGread[i].rs2 = -1;
        RGread[i].dst = -1;
        RGread[i].using_rob1 = false;
        RGread[i].using_rob2 = false;

        DSpactch[i].I_order = -1;
        DSpactch[i].oprand = -1;
        DSpactch[i].rs1 = -1;
        DSpactch[i].rs2 = -1;
        DSpactch[i].dst = -1;
        DSpactch[i].using_rob1 = false;
        DSpactch[i].using_rob2 = false;
    } 
    Rob = ROB_FIFO(parami);
    RMT = new RMT_component[ARF_size];
    for(int i=0; i < ARF_size; i++) {
        RMT[i].v = false;
        RMT[i].ROB_tag = -1;
    }
    WrBack = new WB_component[5*SP_scaler_width];
    for(unsigned long int i=0; i < 5*SP_scaler_width; i++) {
        WrBack[i].I_order = -1;
        WrBack[i].ROB_tag = -1;
        WrBack[i].v_avail = false;
    }
    IsQueue = Issue_Queue(parami);
    ExFu = Execute_FU(parami);

    Emp_stage = (Empty_stage){ true };

    one_circulation = 2*SP_scaler_width + ROB_size;
    result = new stage_info[one_circulation];
    for(unsigned long int i=0; i < one_circulation; i++) {
        result[i].FE.start = -1;
        result[i].FE.period = -1;
        result[i].DE.start = -1;
        result[i].DE.period = -1;
        result[i].RN.start = -1;
        result[i].RN.period = -1;
        result[i].RR.start = -1;
        result[i].RR.period = -1;
        result[i].DI.start = -1;
        result[i].DI.period = -1;
        result[i].IS.start = -1;
        result[i].IS.period = -1;
        result[i].EX.start = -1;
        result[i].EX.period = -1;
        result[i].WB.start = -1;
        result[i].WB.period = -1;
        result[i].RT.start = -1;
        result[i].RT.period = -1;
    }

}

void OOO_cpu::Retire(){
//cout<<"# ===  RT Simulator Command =========\n";
    for(unsigned long int k=0;k<ROB_size;k++){
        int order = Rob.rob_comp[k].I_order % one_circulation;
        if((Rob.rob_comp[k].I_order != -1) && (Rob.rob_comp[k].Ready)){
            if(result[order].RT.start <= -1){
                result[order].RT.start = result[order].WB.start + result[order].WB.period;
                result[order].RT.period = 1;
            }else{
                result[order].RT.period++;
            }
        }

    }
    bool RT_empty = false;
    for(unsigned long int k=0;k<SP_scaler_width;k++){
        
        if(!RT_empty){
            if(Rob.rob_comp[Rob.Head].Ready){
                
                int order = Rob.rob_comp[Rob.Head].I_order;
                int pr_pos = order % one_circulation;
                int op = Rob.rob_comp[Rob.Head].op;
                int src1 = Rob.rob_comp[Rob.Head].src1;
                int src2 = Rob.rob_comp[Rob.Head].src2;
                int dst = Rob.rob_comp[Rob.Head].dst;

                if(Rob.rob_comp[Rob.Head].dst != -1){
                    int rmt_pos = Rob.rob_comp[Rob.Head].dst;
                    
                    if((RMT[rmt_pos].ROB_tag == Rob.Head) && (RMT[rmt_pos].v)){
                        RMT[rmt_pos].ROB_tag = -1;
                        RMT[rmt_pos].v = false;
                    }
                }

                Rob.rob_retire();
                if(Rob.Head == Rob.Tail){
                    cpu_complete = true;
                    RT_empty = true;
                }else{
                    cpu_complete = false;
                    RT_empty = false;
                }

                if( cpu_complete ){
                    total_cycle = cycle+1;
                    D_IC = (unsigned long int)order + 1;
                }

                print_result(pr_pos, order, op, src1, src2, dst);
                
            }
        }
    } 

}


void OOO_cpu::Writeback(){
    //cout<<"# ===WB Simulator Command =========\n";
    for(unsigned long int k=0;k<5*SP_scaler_width;k++){
        int order = WrBack[k].I_order % one_circulation;
        if(WrBack[k].I_order != -1 && WrBack[k].v_avail){
            if(result[order].WB.start == -1){
                result[order].WB.start = cycle;
                result[order].WB.period = 1;
            }
        }

    }

    for(unsigned long int k=0;k<5*SP_scaler_width;k++){
        if(WrBack[k].v_avail){
            Rob.rob_comp[WrBack[k].ROB_tag].Ready = true;
        }
    }

}

void OOO_cpu::Execute(){
    //cout<<"# === EX Simulator Command =========\n";
    for(unsigned long int k=0;k<5*SP_scaler_width;k++){
        int order = ExFu.ex_fu_comp[k].I_order % one_circulation;
        if((ExFu.ex_fu_comp[k].I_order != -1)){
            if(result[order].EX.start == -1 && ExFu.ex_fu_comp[k].cycle_left != -1){
                result[order].EX.start = cycle;
                result[order].EX.period = ExFu.ex_fu_comp[k].cycle_left;
            }
        }

    }

    for(unsigned long int k=0;k<5*SP_scaler_width;k++){
        
        if(ExFu.ex_fu_comp[k].cycle_left >= 0){
            ExFu.ex_fu_comp[k].cycle_left--;
        }
    }

    for(unsigned long int k=0;k<5*SP_scaler_width;k++){
        if(ExFu.ex_fu_comp[k].cycle_left == 0){
            IsQueue.EXfu_bypass_IQ(ExFu.ex_fu_comp[k].dst);

            WrBack[k].v_avail = true;
            WrBack[k].ROB_tag = ExFu.ex_fu_comp[k].dst;
            WrBack[k].I_order = ExFu.ex_fu_comp[k].I_order;
        }else{
            WrBack[k].v_avail = false;
        }
    }

}

void OOO_cpu::Issue(){
    //cout<<"# === IssueSimulator Command =========\n";
    for(unsigned long int k=0;k<ISqueue_size;k++){
        int order = IsQueue.isq_comp[k].I_order % one_circulation;
        if((IsQueue.isq_comp[k].I_order >= 0) && (IsQueue.isq_comp[k].v)){
            if(result[order].IS.start == -1){
                result[order].IS.start = cycle;
                result[order].IS.period = 1;
            }else{
                result[order].IS.period++;
            }
        }
    }

    for(unsigned long int k=0;k<SP_scaler_width;k++){
        int iq_to_ex_pos = IsQueue.find_raise_hand();

        if(iq_to_ex_pos != -1){
            int order  = IsQueue.isq_comp[iq_to_ex_pos].I_order;
            int optype = IsQueue.isq_comp[iq_to_ex_pos].op;
            int desti  = IsQueue.isq_comp[iq_to_ex_pos].dst;
            
            int fu_pos = ExFu.empty_fu();                           //// attention
            if(fu_pos != -1){
                ExFu.IS_to_EXfu(order, optype, desti);
                IsQueue.isq_comp[iq_to_ex_pos].v           =  false;
                IsQueue.isq_comp[iq_to_ex_pos].rasie_hand  =  false;
                IsQueue.isq_comp[iq_to_ex_pos].I_order     =  -1;
               

            }
        }
    }

    if(IsQueue.iq_avail_for_next()){     // may have error
        Emp_stage.IS = true;
    }else{
        Emp_stage.IS = false;
    }


    

}

void OOO_cpu::Dispatch(){
//cout<<"# === DI Simulator Command =========\n";
    for(unsigned long int k=0;k<SP_scaler_width;k++){
        int order = DSpactch[k].I_order % one_circulation;
        if(DSpactch[k].I_order != -1){
            if(result[order].DI.start == -1){
                result[order].DI.start = cycle;
                result[order].DI.period = 1;
            }else{
                result[order].DI.period++;
            }
        }

    }

    for(unsigned long int k=0;k<SP_scaler_width;k++){
        if(DSpactch[k].I_order != -1){
            if(DSpactch[k].using_rob1){
                DSpactch[k].using_rob1 = !RR_val_is_rdy_check(DSpactch[k].rs1);   // if check == true, rob are replaced by falue, meaning using rob == false
            }
            
            if(DSpactch[k].using_rob2){
                DSpactch[k].using_rob2 = !RR_val_is_rdy_check(DSpactch[k].rs2);
            } 
        }
            
    }
    
    if( Emp_stage.IS ){
        Emp_stage.DI = true;
        for(unsigned long int k=0;k<SP_scaler_width;k++){
            if(DSpactch[k].I_order != -1){
                int isq_pos = IsQueue.empty_isq();
                if(!DSpactch[k].using_rob1 && !DSpactch[k].using_rob2){
                    IsQueue.isq_comp[isq_pos].rasie_hand = true;
                }else{
                    IsQueue.isq_comp[isq_pos].rasie_hand = false;
                }

                IsQueue.isq_comp[isq_pos].v         =  true;
                IsQueue.isq_comp[isq_pos].I_order   =  DSpactch[k].I_order;
                IsQueue.isq_comp[isq_pos].op        =  DSpactch[k].oprand;
                IsQueue.isq_comp[isq_pos].dst       =  DSpactch[k].dst;
                IsQueue.isq_comp[isq_pos].rs1_tag   =  DSpactch[k].rs1;
                IsQueue.isq_comp[isq_pos].rs1_rdy   =  !DSpactch[k].using_rob1;
                IsQueue.isq_comp[isq_pos].rs2_tag   =  DSpactch[k].rs2;
                IsQueue.isq_comp[isq_pos].rs2_rdy   =  !DSpactch[k].using_rob2;
                
                IsQueue.largest_order++;
            }
            
        }
    }else{
        bool empty = true;
        for(unsigned long int k=0;k<SP_scaler_width;k++){
            if(DSpactch[k].I_order != -1){
                empty = false;
            } 
        }
        if(empty){
            Emp_stage.DI = true;
        }else{
            Emp_stage.DI = false;
        }
    }

    
}

void OOO_cpu::RegRead(){
   // cout<<"# === RD Simulator Command =========\n";
    for(unsigned long int k=0;k<SP_scaler_width;k++){
        int order = RGread[k].I_order % one_circulation;
        if(RGread[k].I_order != -1){
            if(result[order].RR.start == -1){
                result[order].RR.start = cycle;
                result[order].RR.period = 1;
            }else{
                result[order].RR.period++;
            }
        }

    }

    for(unsigned long int k=0;k<SP_scaler_width;k++){
        if(RGread[k].using_rob1){
            RGread[k].using_rob1 = !RR_val_is_rdy_check(RGread[k].rs1);   // if check == true, rob are replaced by falue, meaning using rob == false
        }
        
        if(RGread[k].using_rob2){
            RGread[k].using_rob2 = !RR_val_is_rdy_check(RGread[k].rs2);
        }
    }

    
    if( Emp_stage.DI ){
        Emp_stage.RR = true;
        for(unsigned long int k=0;k<SP_scaler_width;k++)
            DSpactch[k] = RGread[k];
    }else{
        bool empty = true;
        for(unsigned long int k=0;k<SP_scaler_width;k++){
            if(RGread[k].I_order != -1){
                empty = false;
            } 
        }
        if(empty){
            Emp_stage.RR = true;
        }else{
            Emp_stage.RR = false;
        }
              // may have problem
      
    }
}

void OOO_cpu::Rename(){

    for(unsigned long int k=0;k<SP_scaler_width;k++){
        int order = Rname[k].I_order % one_circulation;
        if(Rname[k].I_order != -1){
            if(result[order].RN.start == -1){
                result[order].RN.start = cycle;
                result[order].RN.period = 1;
            }else{
                result[order].RN.period++;
            }
        }

    }

    if( Emp_stage.RR ){
        if(Rob.Rob_empty()){
            Emp_stage.RN = true;
            for(unsigned long int k=0;k<SP_scaler_width;k++){
                int src1 = Rname[k].rs1;
                int src2 = Rname[k].rs2;
                int op = Rname[k].oprand;
                int dst = Rname[k].dst;
                if(Rname[k].I_order != -1){
                    if(Rname[k].rs1 != -1){
                        if(RMT[Rname[k].rs1].v){
                            Rname[k].rs1 = RMT[Rname[k].rs1].ROB_tag;
                            Rname[k].using_rob1 = true;
                        }
                    }
                    if(Rname[k].rs2 != -1){
                        if(RMT[Rname[k].rs2].v){
                            Rname[k].rs2 = RMT[Rname[k].rs2].ROB_tag;
                            Rname[k].using_rob2 = true;
                        }
                    }

                    if(dst != -1){
                        RMT[dst].v = true;
                        RMT[dst].ROB_tag = Rob.Tail;
                    }
                    
                    
                    Rname[k].dst = Rob.Tail;
                    Rob.RN_rob_assign(Rname[k].I_order, dst, false, src1, src2, op);


                }
                
                RGread[k] = Rname[k];
            }
        }else{
            Emp_stage.RN = false;
            for(unsigned long int k=0;k<SP_scaler_width;k++){
                RGread[k].I_order = -1;
                RGread[k].oprand = -1;
                RGread[k].rs1 = -1;
                RGread[k].rs2 = -1;
                RGread[k].dst = -1;
            }   
        }
    }else{
        Emp_stage.RN = false;
    }


    
}

void OOO_cpu::Decode(){

    for(unsigned long int k=0;k<SP_scaler_width;k++){
        int order = DCode[k].I_order % one_circulation;
        if(DCode[k].I_order != -1){
            if(result[order].DE.start == -1){
                result[order].DE.start = cycle;
                result[order].DE.period = 1;
            }else{
                result[order].DE.period++;
            }
        }

    }

    if( Emp_stage.RN ){
        Emp_stage.DE = true;
        for(unsigned long int k=0;k<SP_scaler_width;k++)
            Rname[k] = DCode[k];
    }else{
        Emp_stage.DE = false;
    }
}

void OOO_cpu::Fetch(input_component* input_comp){


    if( Emp_stage.DE ){
        for(unsigned long int k=0;k<SP_scaler_width;k++){
        int order = input_comp[k].I_order % one_circulation;
        if(input_comp[k].I_order != -1){
            if(result[order].FE.start == -1){
                result[order].FE.start = cycle;
                result[order].FE.period = 1;
            }
        }

        }
        Emp_stage.FE = true;
        for(unsigned long int k=0;k<SP_scaler_width;k++){
            DCode[k].I_order    =  input_comp[k].I_order;
            DCode[k].oprand     =  input_comp[k].oprand;
            DCode[k].rs1        =  input_comp[k].rs1;
            DCode[k].rs2        =  input_comp[k].rs2;
            DCode[k].dst        =  input_comp[k].dst;
            DCode[k].using_rob1 =  false;
            DCode[k].using_rob2 =  false;
        }
    }else{  
        Emp_stage.FE = false;
    }
}

bool OOO_cpu::Advance_Cycle(){

    cycle++;

    if(!Emp_stage.FE)   
        return true;
    else
        return false;
}

bool OOO_cpu::value_bypass(int rs_tag){
    for(unsigned long int i=0; i < 5*SP_scaler_width; i++) {
        if(WrBack[i].v_avail && WrBack[i].ROB_tag == rs_tag){    // may need to add something to check whether it is valid or not
            return true;
        }
    }
    return false;
}

bool OOO_cpu::RR_val_is_rdy_check(int rs_tag){
    if(Rob.rob_comp[rs_tag].Ready){
        return true;
    }else if(value_bypass(rs_tag)){
        return true;
    }else{
        return false;
    }
}

void OOO_cpu::print_result(int pos, int order, int op, int src1, int src2, int dst){
    stage_info print = result[pos];
    printf("%d fu{%d} src{%d,%d} dst{%d} ",order,op,src1,src2,dst);
    printf("FE{%d,%d} DE{%d,%d} RN{%d,%d} ",print.FE.start,print.FE.period,print.DE.start,print.DE.period,print.RN.start,print.RN.period);
    printf("RR{%d,%d} DI{%d,%d} IS{%d,%d} ",print.RR.start,print.RR.period,print.DI.start,print.DI.period,print.IS.start,print.IS.period);
    printf("EX{%d,%d} WB{%d,%d} RT{%d,%d}",print.EX.start,print.EX.period,print.WB.start,print.WB.period,print.RT.start,print.RT.period);
    printf("\n");
    reset(pos);
}

void OOO_cpu::reset(int order){
    result[order].FE.start = -1;
    result[order].FE.period = -1;
    result[order].DE.start = -1;
    result[order].DE.period = -1;
    result[order].RN.start = -1;
    result[order].RN.period = -1;
    result[order].RR.start = -1;
    result[order].RR.period = -1;
    result[order].DI.start = -1;
    result[order].DI.period = -1;
    result[order].IS.start = -1;
    result[order].IS.period = -1;
    result[order].EX.start = -1;
    result[order].EX.period = -1;
    result[order].WB.start = -1;
    result[order].WB.period = -1;
    result[order].RT.start = -1;
    result[order].RT.period = -1;
}



void OOO_cpu::parami_print(){
    ipc = float( D_IC )/float( total_cycle );
    cout<<"# === Processor Configuration ==="<<endl;
    cout<<"# ROB_SIZE = "<<ROB_size<<endl;
    cout<<"# IQ_SIZE  = "<<ISqueue_size<<endl;
    cout<<"# WIDTH    = "<<SP_scaler_width<<endl;
    cout<<"# === Simulation Results ========\n";
    cout<<"# Dynamic Instruction Count    = "<<D_IC<<endl;
    cout<<"# Cycles                       = "<<total_cycle<<endl;
    cout<<"# Instructions Per Cycle (IPC) = "<<fixed<<setprecision(2)<<ipc<<endl;
}