#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "sim_proc.h"
#include "pipe_stage.h"

/*  argc holds the number of command line arguments
    argv[] holds the commands themselves

    Example:-
    sim 256 32 4 gcc_trace.txt
    argc = 5
    argv[0] = "sim"
    argv[1] = "256"
    argv[2] = "32"
    ... and so on
*/
int main (int argc, char* argv[])
{
    FILE *FP;               // File handler
    char *trace_file;       // Variable that holds trace file name;
    proc_params params;       // look at sim_bp.h header file for the the definition of struct proc_params
    int op_type, dest, src1, src2;  // Variables are read from trace file
    uint64_t pc; // Variable holds the pc read from input file
    
    if (argc != 5)
    {
        printf("Error: Wrong number of inputs:%d\n", argc-1);
        exit(EXIT_FAILURE);
    }
    
    params.rob_size     = strtoul(argv[1], NULL, 10);
    params.iq_size      = strtoul(argv[2], NULL, 10);
    params.width        = strtoul(argv[3], NULL, 10);
    trace_file          = argv[4];
    /*printf("rob_size:%lu "
            "iq_size:%lu "
            "width:%lu "
            "tracefile:%s\n", params.rob_size, params.iq_size, params.width, trace_file);
    */
    // Open trace_file in read mode
    FP = fopen(trace_file, "r");
    if(FP == NULL)
    {
        // Throw error and exit if fopen() failed
        printf("Error: Unable to open file %s\n", trace_file);
        exit(EXIT_FAILURE);
    }
    
    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    // The following loop just tests reading the trace and echoing it back to the screen.
    //
    // Replace this loop with the "do { } while (Advance_Cycle());" loop indicated in the Project 3 spec.
    // Note: fscanf() calls -- to obtain a fetch bundle worth of instructions from the trace -- should be
    // inside the Fetch() function.
    //
    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    
    OOO_cpu* cpu = new OOO_cpu(params);

    input_component* input_comp;
    input_comp = new input_component[params.width];
    int i_order = 0;
    uint64_t final = 0;
    //int CII = 0;
    //int a = 0;
    while(final == 0){
        for(unsigned long int k=0; k < params.width ; k++)
        {
            if(fscanf(FP, "%lx %d %d %d %d", &pc, &op_type, &dest, &src1, &src2) != EOF)
            {
                input_comp[k].I_order = i_order;
                input_comp[k].oprand = op_type;
                input_comp[k].rs1 = src1;
                input_comp[k].rs2 = src2;
                input_comp[k].dst = dest;
                
                final = 0;
                i_order++;
            }else
            {
                input_comp[k].I_order = -1;
                input_comp[k].oprand = op_type;
                input_comp[k].rs1 = src1;
                input_comp[k].rs2 = src2;
                input_comp[k].dst = dest;
                final = 1;
            }

        } // end for k

        do{
            

            /*if(CII == a){
                scanf ("%d",&a);
            }*/

            cpu->Retire();
            cpu->Writeback();
            cpu->Execute();
            cpu->Issue();
            cpu->Dispatch();
            cpu->RegRead();
            cpu->Rename();
            cpu->Decode();
            cpu->Fetch(input_comp);
            
            //CII++;

        }while( cpu->Advance_Cycle() );

    }
    
    do{
        for(unsigned long int k=0; k < params.width ; k++){
            input_comp[k].I_order = -1;
            input_comp[k].oprand = op_type;
            input_comp[k].rs1 = src1;
            input_comp[k].rs2 = src2;
            input_comp[k].dst = dest;
        }

        cpu->Retire();
        cpu->Writeback();
        cpu->Execute();
        cpu->Issue();
        cpu->Dispatch();
        cpu->RegRead();
        cpu->Rename();
        cpu->Decode();
        cpu->Fetch(input_comp);
        cpu->Advance_Cycle();
    }while( !cpu->cpu_complete);
    
    
    cout<<"# === Simulator Command =========\n";
    cout<<"# "<<argv[0]<<" "<<argv[1]<<" "<<argv[2]<<" "<<argv[3]<<" "<<argv[4]<<endl;
    cpu->parami_print();

    return 0;
}
