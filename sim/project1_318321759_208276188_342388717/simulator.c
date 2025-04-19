#include "simulator.h"

////////////////////////////////////////////////////////////
//////// GENERAL FILE HANDLING FUNCTIONS 
///////////////////////////////////////////////////////////

FILE* open_file_to_read(char* fp)
{
    FILE* fh = fopen(fp, "r");
    if (fh == NULL) assert(fh != NULL && "-ERROR- Could not open file for reading!"); // Handle fopen fail
    return fh;
};

FILE* open_file_to_write(char* fp)
{
    FILE* fh = fopen(fp, "w");
    if (fh == NULL) assert(fh != NULL && "-ERROR- Could not open file for writing!"); // Handle fopen fail
    return fh;
};

FILE* open_file_to_append(char* fp)
{
    FILE* fh = fopen(fp, "a");
    if (fh == NULL) assert(fh != NULL && "-ERROR- Could not open file for appending!"); // Handle fopen fail
    return fh;
};

////////////////////////////////////////////////////////////
//////// SIMULATOR INITIALIZING FUNCTIONS  
///////////////////////////////////////////////////////////

// function to read main memory from memin.txt to simulator
void load_data_from_input_file_to_main_mem(simulator* simulator, FILE* fh)
{
    int i = 0;
    char line[MAX_LINE_LENGTH] = { 0 };
    char* curr_line = NULL;

    // Iterate over input file and write to the correct simulator field
    curr_line = fgets(line, MAX_LINE_LENGTH, fh);
    while (curr_line != NULL)
    {
        simulator->main_memory[i++] = (unsigned int)strtol(line, NULL, 16);
        curr_line = fgets(&(line[0]), MAX_LINE_LENGTH, fh);
        if(simulator->main_memory[i - 1] != 0) // if the value is not 0, update the max index
        {
            simulator->max_main_memory_index = i;
        }
    }
}

// function to read instruction memory from imemX.txt to each core
// need to fix and see how we handle the instructions
void load_data_from_input_file_to_core(core* core, FILE* fh)
{
    int i = 0;
    char line[MAX_LINE_LENGTH] = { 0 };
    char* curr_line = NULL;

    // Iterate over input file and write to the correct simulator field
    curr_line = fgets(line, MAX_LINE_LENGTH, fh);
    while (curr_line != NULL)
    {
        core->imem[i++] = (unsigned int)strtol(line, NULL, 16); //do we need the base 16? - we need to support instructions
        curr_line = fgets(&(line[0]), MAX_LINE_LENGTH, fh);
    }
}

// handels initilization of cores and main mem in the simulator
// to do - when we will have 4 core need to uncomment many lines!
void init_simulator(simulator* simulator, char* memin_fp)
{
    FILE* memin_fh = NULL;
    // Validate arguments
    assert(simulator != NULL && "-ERROR- You entered invalid simulator pointer when calling init_simulator function");
    assert(memin_fp != NULL && "-ERROR- You entered invalid memin file path when calling init_simulator function");

    // Open input files
    memin_fh = open_file_to_read(memin_fp);

    // Initialize simulator fields value to 0 
    simulator->PC_0 = 0;
    simulator->PC_1 = 0;
    simulator->PC_2 = 0;
    simulator->PC_3 = 0;
    simulator->run_en = false;
    simulator->clk = 0;
    simulator->transaction_timer = 0;
    simulator->bus_is_busy = false;
    simulator->sender_id = -1;
    simulator->max_main_memory_index = 0;
    simulator->dirty_block = 0;
    simulator->busrdx_en = false;
    simulator->arb_is_done = false;
    simulator->bus_is_updated = false;
    simulator->last_transaction = false;


    //bus_d init
    simulator->bus_q = (bus*)malloc(sizeof(bus));
    simulator->bus_q->addr = 0;
    simulator->bus_q->cmd = NO_COMMAND;
    simulator->bus_q->data = 0;
    simulator->bus_q->origid = 0;
    simulator->bus_q->shared = 0;

    //bus_q init
    simulator->bus_d = (bus*)malloc(sizeof(bus));
    simulator->bus_d->addr = 0;
    simulator->bus_d->cmd = NO_COMMAND;
    simulator->bus_d->data = 0;
    simulator->bus_d->origid = 0;
    simulator->bus_d->shared = 0;
  
    (simulator->main_memory) = malloc(MAIN_MEMORY_DEPTH*sizeof(unsigned int));
    memset(simulator->main_memory, 0, MAIN_MEMORY_DEPTH * sizeof(unsigned int));
    // Load data from input files to simulator struct -> instructions from memin.txt, disk content from diskin.txt, cycles to raise irq2 from irq2in.txt
    load_data_from_input_file_to_main_mem(simulator, memin_fh);
    // Close input files 
    fclose(memin_fh);
}

void init_core(char* imem_core_fp, core* core)
{
    FILE* imem_core_fh = NULL;

    // Validate arguments
    assert(core != NULL && "-ERROR- You entered invalid core pointer when calling init_core function");
    assert(imem_core_fp != NULL && "-ERROR- You entered invalid imem file path when calling init_core function");

    // Open input files
    imem_core_fh = open_file_to_read(imem_core_fp);

    // Initialize core fields value to 0
    core->PC =0;
    core->next_PC =0;
    core->halt_en = false;
    core->branch_en = false;
    core->priority = 0;
    core->stall_en = false;
    core->is_hit = false;
    core->memory_stall = false;
    core->transaction_number = 0;
    core->bus_addr = 0;
    core->bus_cmd = NO_COMMAND;
    core->bus_data=0;
    core->request_bus=0;
    core->bus_grant =false;
    core->flush_addr = 0;
    core->self_flushing = false;


    //init pipeline registers
    pipeline_regs* pipe_regs = (pipeline_regs*)malloc(sizeof(pipeline_regs));
    for (int i = 0; i < PIPE_REGS_WIDTH; i++){
        pipe_regs->FD_reg[i] = (ff*)malloc(sizeof(ff));
        pipe_regs->DE_regs[i] = (ff*)malloc(sizeof(ff));
        pipe_regs->EM_regs[i] = (ff*)malloc(sizeof(ff));
        pipe_regs->MWB_regs[i] = (ff*)malloc(sizeof(ff));
        pipe_regs->DE_regs[i]->d= -1;
        pipe_regs->DE_regs[i]->q= -1;
        pipe_regs->EM_regs[i]->d= -1;
        pipe_regs->EM_regs[i]->q= -1;
        pipe_regs->FD_reg[i]->d= -1;
        pipe_regs->FD_reg[i]->q= -1;
        pipe_regs->MWB_regs[i]->d= -1;
        pipe_regs->MWB_regs[i]->q= -1;
    }

    core->pipe_regs = pipe_regs;
    //init stats
    stats* stats0 = (stats*)malloc(sizeof(stats));
    core->stats = stats0;
    core->stats->cycles = 0;
    core->stats->decode_stall = 0;
    core->stats->instructions = 0;
    core->stats->mem_stall = 0;
    core->stats->read_hit = 0;
    core->stats->read_miss = 0;
    core->stats->write_hit = 0;
    core->stats->write_miss = 0;
    core->core_done = false;
    core->last_clk = 0;

    //init core registers
    for(int i = 0; i < NUMBER_OF_CORE_REGS; i++)
    {
        core->regs[i] = 0;
    }

    //cache init
    initialize_dsram(core);
    initialize_tsram(core);

    // Load data to core cache
    load_data_from_input_file_to_core(core ,imem_core_fh); 
    fclose(imem_core_fh);
}

// Free all memory allocated for a core
void free_core(core* core){
    for (int i = 0; i < PIPE_REGS_WIDTH; i++){
        free(core->pipe_regs->FD_reg[i]);
        free(core->pipe_regs->DE_regs[i]);
        free(core->pipe_regs->EM_regs[i]);
        free(core->pipe_regs->MWB_regs[i]);
    }

    for(int i = 0; i < CACHE_TSRAM_DEPTH; i++) 
    {  
        free(core->tsram[i]);
    }

    free(core->pipe_regs);
    free(core->stats);
    free(core);
}

////////////////////////////////////////////////////////////
////////PIPE REGISTERS HANDLING FUNCTIONS
//////////////////////////////////////////////////////////

// function to update core registers - used in the end of each cycle
void core_regs_update(core* core){

        for (int j = 0; j < PIPE_REGS_WIDTH; j++)
        {
            // if memory stall - bypass all values in MWB. Fetch, Decode and Execute are not updated.
            if (core->memory_stall){
                core->pipe_regs->MWB_regs[j]->q = core->pipe_regs->MWB_regs[j]->d;
                continue;
            }

            // if stall mode - bypass all values in DE, EM, MWB. Fetch and Decode are not updated.
            if (core->stall_en){
            core->pipe_regs->DE_regs[j]->q = core->pipe_regs->DE_regs[j]->d; 
            core->pipe_regs->EM_regs[j]->q = core->pipe_regs->EM_regs[j]->d;
            core->pipe_regs->MWB_regs[j]->q = core->pipe_regs->MWB_regs[j]->d;
            continue;
            }
            //fixme update all sections to byp
            
            // regular mode - update all values in DE, EM, MWB
            core->pipe_regs->FD_reg[j]->q = core->pipe_regs->FD_reg[j]->d;
            core->pipe_regs->DE_regs[j]->q = core->pipe_regs->DE_regs[j]->d;
            core->pipe_regs->EM_regs[j]->q = core->pipe_regs->EM_regs[j]->d;
            core->pipe_regs->MWB_regs[j]->q = core->pipe_regs->MWB_regs[j]->d;
        }

    return;
    }

void bus_regs_update(simulator* sim){
    bus* bus_q = sim->bus_q;
    bus* bus_d = sim->bus_d;

    bus_q->addr = bus_d->addr;
    bus_q->data = bus_d->data;
    bus_q->cmd = bus_d->cmd;
    bus_q->origid = bus_d->origid;
    bus_q->shared = bus_d->shared;
    return;
}

// function to check if the pipeline is empty at the next rising edge - used in the end of each cycle
bool pipeline_is_empty(core* core){

 return ((core->pipe_regs->FD_reg[INST_PC]->d == -1) &&
        (core->pipe_regs->FD_reg[INST_PC]->q == -1) &&
        (core->pipe_regs->DE_regs[INST_PC]->q == -1)&&
        (core->pipe_regs->EM_regs[INST_PC]->q == -1)&&
        (core->pipe_regs->MWB_regs[INST_PC]->q != -1)); // the last stage is not empty
}


////////////////////////////////////////////////////////////
//////// WRITE OUTPUT FILES FUNCTIONS  
///////////////////////////////////////////////////////////

void write_output_file_memout(simulator* simulator, char* memout_fp)
{
    FILE* memout_fh = open_file_to_write(memout_fp);
    for (int i = 0; i <= simulator->max_main_memory_index; i++)
    {
        fprintf(memout_fh, "%08X\n", simulator->main_memory[i]);
    }
    fclose(memout_fh);
    return;
}

//general for a core, need to send correct core regout file
void write_output_file_regout_core(core* core, char* regout_fp)
{
    FILE* regout_fh = open_file_to_write(regout_fp);
    for (int i = 2; i < NUMBER_OF_CORE_REGS; i++) {
        fprintf(regout_fh, "%08X\n", core->regs[i]);
    }
    fclose(regout_fh);
    return;
}

// todo - yaron FIXME
void write_output_file_stats(core* core, char* stats_fp){
    FILE* stats_fh = open_file_to_write(stats_fp);

    fprintf(stats_fh, "cycles %ld\n", core->last_clk + 1); // cycles
    fprintf(stats_fh, "instructions %d\n", core->stats->instructions - 1); // instructions
    fprintf(stats_fh, "read_hit %d\n", core->stats->read_hit); // read_hit
    fprintf(stats_fh, "write_hit %d\n", core->stats->write_hit); // write_hit
    fprintf(stats_fh, "read_miss %d\n", core->stats->read_miss); // read_miss
    fprintf(stats_fh, "write_miss %d\n", core->stats->write_miss); // write_miss
    fprintf(stats_fh, "decode_stall %d\n", core->stats->decode_stall); // decode_stall
    fprintf(stats_fh, "mem_stall %d\n", core->stats->mem_stall); // mem_stall

    fclose(stats_fh);
    return;
}


// trace file line format: CYCLE FETCH DECODE EXEC MEM WB R2 R3 R4 R5 R6 R7 R8 R9 R10 R11 R12 R13 R14 R15
void write_output_file_core_trace(core* core, FILE* trace_fh, long clk, bool is_empty)
{
    bool print_en = false;
    const char str[] = "---";
    char fetch[12], decode[12], exec[12], mem[12], wb[12];

    if(core->pipe_regs->FD_reg[INST_PC]->d == -1){ //Empty pipeline - used in the drain process (HALT)
        strcpy(fetch,str);}
    else{
        sprintf(fetch, "%03X", core->pipe_regs->FD_reg[INST_PC]->d);
        print_en = true;}

    if(core->pipe_regs->FD_reg[INST_PC]->q == -1){ //Empty pipeline - used in the drain process (HALT)
        strcpy(decode,str);}
    else{
        sprintf(decode, "%03X", core->pipe_regs->FD_reg[INST_PC]->q);
        print_en = true;}

    if(core->pipe_regs->DE_regs[INST_PC]->q == -1){
        strcpy(exec,str);}
    else{
        sprintf(exec, "%03X", core->pipe_regs->DE_regs[INST_PC]->q);
        print_en = true;}

    if(core->pipe_regs->EM_regs[INST_PC]->q == -1){
        strcpy(mem,str);}
    else{
        sprintf(mem, "%03X",core->pipe_regs->EM_regs[INST_PC]->q);
        print_en = true;}
    
    if(core->pipe_regs->MWB_regs[INST_PC]->q == -1){
        strcpy(wb,str);}
    else{
        sprintf(wb, "%03X", core->pipe_regs->MWB_regs[INST_PC]->q); 
        print_en = true;}

    if (!print_en) return;

    fprintf(trace_fh, "%ld %s %s %s %s %s %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X \n",
    clk, 
    fetch,
    decode, 
    exec,
    mem,
    wb,
    core->regs[2],
    core->regs[3],
    core->regs[4],
    core->regs[5],
    core->regs[6],
    core->regs[7],
    core->regs[8],
    core->regs[9],
    core->regs[10],
    core->regs[11],
    core->regs[12],
    core->regs[13],
    core->regs[14],
    core->regs[15]);
}

// need to write every cycle the bus changes its state
// writing format: CYCLE bus_origid bus_cmd bus_addr bus_data bus_shared
void write_output_file_bustrace(simulator* sim, char* bus_trace_fp)
{
    FILE* bus_trace_fh = open_file_to_append(bus_trace_fp);

    fprintf(bus_trace_fh, "%d %01X %01X %05X %08X %01X\n", // fix me - format for cycle
        //cycle, // need to be cycle. get from simulator? each core?
        sim->clk,
        sim->bus_q->origid,
        sim->bus_q->cmd,
        sim->bus_q->addr,
        sim->bus_q->data,
        sim->bus_q->shared);
    fclose(bus_trace_fh);
}

// to do!
// writing format: dsram for each core
void write_output_file_dsram(core* core, char* dsram_fp)
{
    FILE* ds_ram_fh = open_file_to_write(dsram_fp);
    for(int i = 0; i < CACHE_DSRAM_DEPTH; i++) {
        fprintf(ds_ram_fh, "%08X\n",core->dsram[i]);
    }
    fclose(ds_ram_fh);
}

// to do!
// writing format: tsram for each core
void write_output_file_tsram(core* core, char* tsram_fp)
{
    FILE* ts_ram_fh = open_file_to_write(tsram_fp);
    for(int i = 0; i < CACHE_TSRAM_DEPTH; i++) {
        fprintf(ts_ram_fh, "%08X\n",(core->tsram[i]->tag | core->tsram[i]->mesi<<12)); // fix me - write all tsram data (insted tsram_fp)
    }
    fclose(ts_ram_fh);
}

////////////////////////////////////////////////////////////
//////// SIMULATOR FUNCTIONS
///////////////////////////////////////////////////////////

void inc_clk(simulator* sim)
{
    sim->clk = sim->clk + 1;
}

// function to run the simulator - each while iteration is a clock cycle
void run_simulator(simulator* simulator, core* core0,core* core1, core* core2,core* core3,char* memout_fp, char* core0_trace_fp, char* core1_trace_fp, char* core2_trace_fp, char* core3_trace_fp, char* bus_trace_fp)
{
    // Validate arguments
    assert(simulator != NULL && "-ERROR- You entered invalid simulator pointer when calling run_simulator function");
    assert(memout_fp != NULL && "-ERROR- You entered invalid memout file path when calling run_simulator function");
    assert(core0_trace_fp != NULL && "-ERROR- You entered invalid core0 trace path when calling run_simulator function");
    assert(core1_trace_fp != NULL && "-ERROR- You entered invalid core1 trace path when calling run_simulator function");
    assert(core2_trace_fp != NULL && "-ERROR- You entered invalid core2 trace file path when calling run_simulator function");
    assert(core3_trace_fp != NULL && "-ERROR- You entered invalid core3 trace file path when calling run_simulator function");
    assert(bus_trace_fp != NULL && "-ERROR- You entered invalid bus trace trace file path when calling run_simulator function");

    // assert run_en to start simulation  
    simulator->run_en = true;
    bool branch_en = false; // Branch done and PC changed accordingly

    // core reg trace files handling 
    FILE* core0_trace_fh = open_file_to_write(core0_trace_fp);
    FILE* core1_trace_fh = open_file_to_write(core1_trace_fp);
    FILE* core2_trace_fh = open_file_to_write(core2_trace_fp);
    FILE* core3_trace_fh = open_file_to_write(core3_trace_fp);

    // Initialize bus trace file 
    FILE* bus_trace_fh = open_file_to_write(bus_trace_fp); // reset bus trace file
    fclose(bus_trace_fh);
    // Run fetch-decode-execute loop until HALT and pipeline is empty
    bool pipeline0_is_empty = false;
    bool pipeline1_is_empty = false;
    bool pipeline2_is_empty = false;
    bool pipeline3_is_empty = false;

    // variables for MESI
    core *cores[NUM_CORES];
    core *priority_core = NULL;
    int bus_requests[4];  // if we got a request lw/sw
    int priority_cores[4]; // shows core priority, 0 is first priority

    initialize_bus_requests(bus_requests);
    cores[0] = core0;
    cores[1] = core1;
    cores[2] = core2;
    cores[3] = core3;
    intialize_priority_cores(core0, core1, core2, core3, priority_cores);
    int check = 0;
    // MAIN LOOP
    while (simulator->run_en){

        //FETCH STAGE
        pipe_fetch(core0);
        pipe_fetch(core1);
        pipe_fetch(core2);
        pipe_fetch(core3);

        //DECODE STAGE
        pipe_decode(core0);
        pipe_decode(core1);
        pipe_decode(core2);
        pipe_decode(core3);
  
        //EXECUTE STAGE
        pipe_execute(core0);
        pipe_execute(core1);
        pipe_execute(core2);
        pipe_execute(core3);

        //printf("DEBUG: clk is:%d,mem stalls are: %d %d %d %d\n",simulator->clk,core0->memory_stall,core1->memory_stall,core2->memory_stall,core3->memory_stall);
        //MEMORY STAGE - understand if we need the bus, or read/write data from cahce
        pipe_memory_hit_or_bus_request(core0, simulator);
        pipe_memory_hit_or_bus_request(core1, simulator);
        pipe_memory_hit_or_bus_request(core2, simulator);
        pipe_memory_hit_or_bus_request(core3, simulator);

        //Transaction completed (the previous one vas number 0) - bus is free to use
        if (simulator->transaction_timer == -1)
        {
            simulator->bus_is_busy = false;
            simulator->last_transaction = true;
            simulator->transaction_timer = 0; // Assume that the number doesnt change
            if (priority_core->self_flushing)
            {
                priority_core->self_flushing = false;
                priority_core->flush_addr = 0;
                simulator->arb_is_done = true; // Current core will be the next who get the bus after self flushing
            }
        }
        

        //Start a new transaction over the bus - set timers and send the requested data
        //If timer > 3 BUS CMD = NO COMMAND
        if (simulator->bus_is_updated)
        {
            execute_transaction(priority_core,cores, simulator);
            simulator->bus_is_busy = true; //initiate bus transaction
            priority_core->bus_grant =true;
            simulator->bus_is_updated = false;
            simulator->bus_d->cmd= NO_COMMAND; //invalidate bus command
        }

        //SEND TRANSACTION to the bus
        if ((simulator->bus_is_busy) && (simulator->transaction_timer <= 3)) //sending condition
        {   
            send_transaction(simulator,cores,priority_core); // send transaction to bus

        }//end if (bus is busy)


        //BUSTRACE - trace requesting commands or flush commands
        if(((simulator->bus_is_busy) && (simulator->bus_q->cmd != NO_COMMAND))||(simulator->last_transaction))
        {
            write_output_file_bustrace(simulator, bus_trace_fp);
            simulator->last_transaction = false;
        }

        if (simulator->bus_is_busy)
        {
            simulator->transaction_timer-- ; //if trans number is 0, set it -1
        }


        // SNOOPING STAGE - for all cores
        pipe_memory_bus_snooping(cores, core0, simulator->bus_q,simulator);
        pipe_memory_bus_snooping(cores, core1, simulator->bus_q,simulator);
        pipe_memory_bus_snooping(cores, core2, simulator->bus_q,simulator);
        pipe_memory_bus_snooping(cores, core3, simulator->bus_q,simulator);
     

        //HANDLE BUS REQUESTS
        bus_requests[0] = core0->request_bus;
        bus_requests[1] = core1->request_bus;
        bus_requests[2] = core2->request_bus;
        bus_requests[3] = core3->request_bus;


        if (simulator->arb_is_done)
        {
            update_bus(simulator->bus_d, priority_core); // update bus with the core that recieved the bus
            simulator->arb_is_done =false;
            simulator->bus_is_updated = true;
        }
        

        //EXECUTE NEW BUS TRANSACTION
        if((bus_requests[0] || bus_requests[1] || bus_requests[2] || bus_requests[3]) && (!simulator->bus_is_busy) && (!simulator->bus_is_updated)){
            priority_core = round_robin_arbitration(cores, priority_cores,simulator, bus_requests);
            simulator->arb_is_done = true;
        }//end if (bus is busy or) 

        //WRITE BACK STAGE
        write_output_file_core_trace(core0 , core0_trace_fh, simulator->clk, pipeline0_is_empty);
        write_output_file_core_trace(core1 , core1_trace_fh, simulator->clk, pipeline1_is_empty);
        write_output_file_core_trace(core2 , core2_trace_fh, simulator->clk, pipeline2_is_empty);
        write_output_file_core_trace(core3 , core3_trace_fh, simulator->clk,pipeline3_is_empty);


        //UPDATE COMMON USAGE REGISTERS
        pipe_wb(core0);
        pipe_wb(core1);
        pipe_wb(core2);
        pipe_wb(core3);


        //RESPONCE TO EACH CORE
        pipeline0_is_empty = pipeline_is_empty(core0);
        pipeline1_is_empty = pipeline_is_empty(core1);
        pipeline2_is_empty = pipeline_is_empty(core2);
        pipeline3_is_empty = pipeline_is_empty(core3);

        // if pipeline is empty and halt is enabled - set core_done to true
        if (pipeline0_is_empty && core0->halt_en)
        {
            core0->last_clk = simulator->clk;
            core0->core_done = true;
        }
        if (pipeline1_is_empty && core1->halt_en) // add for all rest of cores
        {
            core1->last_clk = simulator->clk;
            core1->core_done = true;
        }

        if (pipeline2_is_empty && core2->halt_en) // add for all rest of cores
        {
            core2->last_clk = simulator->clk;
            core2->core_done = true;
        }

        if (pipeline3_is_empty && core3->halt_en) // add for all rest of cores
        {
            core3->last_clk = simulator->clk;
            core3->core_done = true;
        }

        // RUN EN VALIDATION
        simulator->run_en = (!core0->core_done)||(!core1->core_done)||(!core2->core_done)||(!core3->core_done);

        //UPDATE REGISTERS
        core_regs_update(core0);
        core_regs_update(core1);
        core_regs_update(core2);
        core_regs_update(core3);
        bus_regs_update(simulator);


        //INCREMENT CLOCK
        inc_clk(simulator);                
    } // End of while(simulator->run_en)

    // When the simulator stops running (after a halt instruction), write the rest of the output files (that aren't written during the simulator run)
    write_output_file_memout(simulator, memout_fp);
    fclose(core0_trace_fh);
    fclose(core1_trace_fh);
    fclose(core2_trace_fh);
    fclose(core3_trace_fh);
}

////////////////////////////////////////////////////////////
//////// PIPELINE  FUNCTIONS PER STAGES
///////////////////////////////////////////////////////////

void pipe_fetch(core* core){

    // if halt - invalidate instruction in fetch
    if (core->halt_en)
    {
        core->pipe_regs->FD_reg[INST_PC]->d = -1; //Invalidate instruction in fetch
        return;
    }
    
    // if branch - go to branch pc
    if(core->branch_en){ // if brunch go to branch pc
        core->PC = core->next_PC;
        core->branch_en = false; //branch is not needed
    }

    // if stall - do nothing, count stats
    if (core->stall_en || core->memory_stall){
        return;
    }

    // regular mode - fetch instruction, push to FD register, increment PC
    core->pipe_regs->FD_reg[INST]->d = core->imem[core->PC]; //fetch inst from IMEM;
    core->pipe_regs->FD_reg[INST_PC]->d = core->PC; //load PC to FD registe
    core->PC = core->PC + 1; //increment PC - not branch
    core->stats->instructions++;
    return;
}

void pipe_decode(core* core){

    // initialize variables
    int instruction =  core->pipe_regs->FD_reg[INST]->q;
    int imm,rd,rt,rs,opcode;
    core->stall_cycles = 0;

    if (core->memory_stall && !core->stall_en)
    {
        return;
    }

    if(!core->memory_stall && core->stall_en){ // counting decode stall only if not memory stall
        core->stats->decode_stall++;
    }

    //BYPASS - Drain process (after halt) or filling process (before first valid instrucation is fetched)
    if(core->pipe_regs->FD_reg[INST_PC]->q == -1) { //for pipeline fill-drain processes BYPASS all instructions with PC = -1
        core->pipe_regs->DE_regs[INST_PC]->d = -1;
        return; 
    }
   
    
    // if halt - invalidate both instructions in decode and execution stages
    if (core->halt_en)
    {
        core->pipe_regs->FD_reg[INST_PC]->q = -1; //Invalidate instruction in decode
        core->pipe_regs->DE_regs[INST_PC]->d = -1; //BYP bubble to the next stage
        return;
    }
    
    // DECODE THE INSTRUCTION
    core->pipe_regs->DE_regs[INST]->d = instruction; 

    //Isolate each instruction field using masking
    imm = instruction & 0xFFF;
    instruction = instruction >> 12;
    rt = instruction & 0xF;
    instruction = instruction >> 4;
    rs = instruction & 0xF;
    instruction = instruction >> 4;
    rd = instruction & 0xF;
    instruction = instruction >> 4;
    opcode = instruction & 0xFF;

    //HALT RESOLUTION
    // if opcode is HALT - set halt_en, let the instruction pass to the next stage
    if (opcode == HALT)
    {
        core->halt_en = true;
    }

    //if one of the registers is 1, it means that the value is in imm
    if (rs == 1 || rt == 1) 
    {
       core->regs[1] = imm; // load imm to R[1] - used in execute stage
    }
    
    // check if destenation register is in use in the next stages - RAW hazard
    int check_MWB_reg = core->pipe_regs->MWB_regs[RD]->q;
    int check_EM_reg = core->pipe_regs->EM_regs[RD]->q;
    int check_DE_reg = core->pipe_regs->DE_regs[RD]->q;
    int is_sw_in_exec = (core->pipe_regs->EM_regs[OPCODE]->q == SW);
    int is_sw_in_mem = (core->pipe_regs->DE_regs[OPCODE]->q == SW);
    int is_sw_in_wb = (core->pipe_regs->MWB_regs[OPCODE]->q == SW);


    //STALL RESOLUTION
    if (!core->halt_en)
    {
        core->stall_en = false;
        if ( opcode==SW && (((check_MWB_reg == rd) && (!is_sw_in_wb))
        ||((check_EM_reg == rd) && (!is_sw_in_mem))
        ||((check_DE_reg == rd) && (!is_sw_in_exec))))
        {
            //core->stall_cycles = 3;
            core->stall_en = true;
        }
        
        //printf("DEBUG: rt,rs values are:%d, %d\n",rt,rs);
        if(((check_MWB_reg == rt) && (rt!=1) && (rt!=0)) || ((check_MWB_reg == rs) && (rs!=1) && (rs!=0)) ){
            //core->stall_cycles = 1;
            core->stall_en = true;
        }
        // check if a stall is needed for EM -> 2 stalls
        else if(((check_EM_reg == rt) && (rt!=1) && (rt!=0)) || ((check_EM_reg == rs) && (rs!=1)&& (rs!=0))){
            //core->stall_cycles = 2;
            core->stall_en = true;
        }
        // check if a stall is needed for DE -> 3 stalls
        else if(((check_DE_reg == rt) && (rt!=1)) || ((check_DE_reg == rs) && (rs!=1)) ){
            //core->stall_cycles = 3;
            core->stall_en = true;
        }

        if((core->stall_en)){
            core->pipe_regs->DE_regs[INST_PC]->d = -1; //Invalidate current instruction and push bubble to the next stage
            core->pipe_regs->DE_regs[RD]->d = -1; //Invalidate RD of the next stage
            return; 
        }

    }   

    //BRANCH RESOLUTION
    core->branch_en = (opcode == BEQ) ||  (opcode == BNE) ||  (opcode == BLT) ||  (opcode == BGT)
    ||  (opcode == BLE) ||  (opcode == BGE) || (opcode == JAL);

    if (core->branch_en)
    {
        if (!rd)
        {
            core->regs[rd] = 0;
        }

        if (rd == 1)
        {
            core->regs[rd] = imm;
        }

        switch (opcode)
        {
        case BEQ: {

            if (core->regs[rs] == core->regs[rt])
            {
                core->next_PC = core->regs[rd]; //FIXME - check if rd or R[rd]
            }
            else{
                core->branch_en = false;
            }
            break;} // if (R[rs] == R[rt]) pc = R[rd]
        case BNE: {
            if (core->regs[rs] != core->regs[rt])
            {
                core->next_PC = core->regs[rd];
            } 
            else{
                core->branch_en = false;
            }
            break;} // if (R[rs] != R[rt]) pc = R[rd]

        case BLT: {
            if (core->regs[rs] < core->regs[rt])
            {
                core->next_PC = core->regs[rd];
            }
            else{
                core->branch_en = false;
            }
            break;} // if (R[rs] < R[rt]) pc = R[rd]

        case BGT: {
            if (core->regs[rs] > core->regs[rt])
            {
                core->next_PC = core->regs[rd];
            }
            else{
                core->branch_en = false;
            }
            break;} // if (R[rs] > R[rt]) pc = R[rd]

        case BLE: {
            if (core->regs[rs] <= core->regs[rt])
            {
                core->next_PC = core->regs[rd];
            }
            else{
                core->branch_en = false;
            }
            break;} // if (R[rs] <= R[rt]) pc = R[rd]

        case BGE: {
            if (core->regs[rs] >= core->regs[rt])
            {
                core->next_PC = core->regs[rd];
            }
            else{
                core->branch_en = false;
            }
            break;

        // Jump and link command:
        case JAL: {
            core->regs[rd] = core->PC + 1; // R[rd] = next instruction address
            core->next_PC = core->regs[rs]; //  pc = R[rs]
            break;
        }
        
        default:
            break;}

    } //end of switch (opcode) 
    }// end if (is_branch)

    //Load the registers values into next pipestage, imm and opcode
    core->pipe_regs->DE_regs[OPCODE]->d = opcode;

    if (rs == 1){
        core->pipe_regs->DE_regs[R_RS]->d = imm;
    }
    else if(rs == 0){
        core->pipe_regs->DE_regs[R_RS]->d = 0;
    }
    else {
        core->pipe_regs->DE_regs[R_RS]->d = core->regs[rs];
    }
     if (rt == 1){
        core->pipe_regs->DE_regs[R_RT]->d = imm;
    } 
    else if(rt == 0){
        core->pipe_regs->DE_regs[R_RT]->d = 0;
    }
    else {
        core->pipe_regs->DE_regs[R_RT]->d = core->regs[rt];
    }

    core->pipe_regs->DE_regs[IMM]->d = imm; 
    core->pipe_regs->DE_regs[RD]->d = rd;
    core->pipe_regs->DE_regs[INST_PC]->d = core->pipe_regs->FD_reg[INST_PC]->q;
    return;
}
    
// EXECUTE INSTRUCTION
void pipe_execute(core* core){

    int invalid_write_attempt = 0;
    int rs = core->pipe_regs->DE_regs[R_RS]->q;
    int rt = core->pipe_regs->DE_regs[R_RT]->q;
    int opcode = core->pipe_regs->DE_regs[OPCODE]->q;

    if (core->memory_stall)
    {
        core->stats->mem_stall += 1;
        return;
    }

    //Drain process or filling process or stall process - BYPASS
    if (core->pipe_regs->DE_regs[INST_PC]->q == -1)
    {
       core->pipe_regs->EM_regs[INST_PC]->d = -1; //Invalidate instruction
       core->pipe_regs->EM_regs[RD]->d = -1; //Invalidate RD
       return;
    }
    
    // Choose operation mode based on opcode
    switch (opcode) {

    // if opcode is HALT - let the instruction pass to the next stage    
    case HALT: {
        //BYP INST AND INST_PC
        core->pipe_regs->EM_regs[INST_PC]->d = core->pipe_regs->DE_regs[INST_PC]->q;
        core->pipe_regs->EM_regs[OPCODE]->d = core->pipe_regs->DE_regs[OPCODE]->q;
    }
        // Register and logic commands:
    case ADD: {
        core->pipe_regs->EM_regs[ALU_OUT]->d = rs + rt; 
        break;
    } // R[rd] = R[rs] + R[rt]

    case SUB: {
        core->pipe_regs->EM_regs[ALU_OUT]->d = rs - rt; break;
    } // R[rd] = R[rs] - R[rt]

    case MUL: {
        core->pipe_regs->EM_regs[ALU_OUT]->d = rs * rt; break;
    } // R[rd] = R[rs] * R[rt]

    case AND: {
        core->pipe_regs->EM_regs[ALU_OUT]->d = rs & rt; break;
    } // R[rd] = R[rs] AND R[rt]

    case OR: {
        core->pipe_regs->EM_regs[ALU_OUT]->d = rs | rt; break;
    } // R[rd] = R[rs] OR R[rt]

    case XOR: {
        core->pipe_regs->EM_regs[ALU_OUT]->d = rs ^ rt; break;
    } // R[rd] = R[rs] XOR R[rt]

    case SLL: {
        core->pipe_regs->EM_regs[ALU_OUT]->d = rs << rt;break;
    } // R[rd] = R[rs] SLL R[rt]

    case SRA: {
        if (rt < 0) {
            core->pipe_regs->EM_regs[ALU_OUT]->d = (rs >> rt) | ~(~(0x0) >> rt);
        }
        else {
            core->pipe_regs->EM_regs[ALU_OUT]->d = (rs >> rt);
        }
        break;
    } // R[rd] = R[rs] SRA R[rt]

    case SRL: {
        core->pipe_regs->EM_regs[ALU_OUT]->d = rs >> rt;
        break;
    } // R[rd] = R[rs] SRL R[rt]
    case LW:{
        core->pipe_regs->EM_regs[ALU_OUT]->d = rs + rt;
        break;
    }
    case SW:{
        core->pipe_regs->EM_regs[ALU_OUT]->d = rs + rt;
        break;
    }
    default:
        break;
    } // End of switch(opcode)

    //  ALU result should be BYP to write-back stage
    core->pipe_regs->EM_regs[RD]->d = core->pipe_regs->DE_regs[RD]->q;
    // R[rt] used as data to store
    core->pipe_regs->EM_regs[R_RT]->d = core->pipe_regs->DE_regs[R_RT]->q;
    // Bypassing opcode to mem stage
    core->pipe_regs->EM_regs[OPCODE]->d = opcode;
    core->pipe_regs->EM_regs[INST_PC]->d = core->pipe_regs->DE_regs[INST_PC]->q;
    core->pipe_regs->EM_regs[IMM]->d =core->pipe_regs->DE_regs[IMM]->q;
;
    return;
}

void pipe_memory_hit_or_bus_request(core* core, simulator* sim){
    int opcode = core->pipe_regs->EM_regs[OPCODE]->q;
    int address = core->pipe_regs->EM_regs[ALU_OUT]->q;
    int imm = core->pipe_regs->EM_regs[IMM]->q;
    int block_index = (address >> 2) & 0x3F; //cache block index
    int dest_reg = core->pipe_regs->EM_regs[RD]->q;
    int data = (dest_reg == 1) ? imm:core->regs[dest_reg];
    core->is_hit = 0; // Flag to indicate if the data is in the cache
    int return_data = 0; 
    
    //if the pipe is stalled, bypass
    if ((core->pipe_regs->EM_regs[INST_PC]->q == -1) || core->memory_stall)
    {
        core->pipe_regs->MWB_regs[INST_PC]->d = -1; //Invalidate instruction
        core->pipe_regs->MWB_regs[RD]->d = -1; //Invalidate next RD
        return;
    }
    
    ///check if hit or miss
    //printf(" before the hit is %d\n",core->is_hit);
    core->is_hit = is_hit(core,address);
    //printf(" after the hit is %d\n",core->is_hit);
    // for hit we write ot read from cache
    if((opcode == LW) || (opcode == SW)){
        
        if (core->is_hit){
            //printf("DEBUG: HIT! core id: %d, clk is: %d, command is %d, address is %d \n", core->id, sim->clk,opcode,address);
            switch (opcode){
            case LW:{ // reading from cache
                get_data_from_cache(core,address, sim,&return_data);
                core->pipe_regs->MWB_regs[MD]->d = return_data; //load data from cache

                // Bypassing ALU output, opcode and rd to write-back stage - used as data to write
                core->pipe_regs->MWB_regs[OPCODE]->d = opcode;
                core->pipe_regs->MWB_regs[ALU_OUT]->d = address; //ALU_OUT is used as address in this stage
                core->pipe_regs->MWB_regs[RD]->d = core->pipe_regs->EM_regs[RD]->q;
                core->pipe_regs->MWB_regs[INST_PC]->d = core->pipe_regs->EM_regs[INST_PC]->q;
                
                core->stats->read_hit++;     
                return;
                break;}

            case SW: { // if exc or mod -> writing to cache.  
                //printf("DEBUG: WRITING TO THE CACHE core id: %d, clk is: %d, data is %d, address is %d \n", core->id, sim->clk,data,address);
                //SHARED case - initiate BUSRDX transaction
                if (core->tsram[block_index]->mesi == EXCLUSIVE)
                {
                    //printf("DEBUG:EXC -> MOD\n");
                    core->tsram[block_index]->mesi = MODIFIED;
                }
                if (core->tsram[block_index]->mesi == MODIFIED)
                {
                    write_data_to_cache(core,address,data,sim);
                    core->stats->write_hit++;
                    core->pipe_regs->MWB_regs[OPCODE]->d = opcode;
                    core->pipe_regs->MWB_regs[ALU_OUT]->d = address; //ALU_OUT is used as address in this stage
                    core->pipe_regs->MWB_regs[RD]->d = core->pipe_regs->EM_regs[RD]->q;//RD is used as data to write
                    core->pipe_regs->MWB_regs[INST_PC]->d = core->pipe_regs->EM_regs[INST_PC]->q;//RD is used as data to write
                    //printf("DEBUG: WRITE HIT! core id: %d, clk is: %d, command is %d, address is %d \n", core->id, sim->clk,opcode,address);
                    return;
                }
               //Initiate bus transaction with BUSRDX
                core->request_bus = 1;
                core->bus_addr = address;
                core->bus_cmd = BUSRDX;
                core->memory_stall = true; // set memory stall flag
                core->pipe_regs->MWB_regs[INST_PC]->d = -1;
                core->pipe_regs->MWB_regs[RD]->d = -1;
                return;
                break;
                }

            default:
                break;
            }// end of switch(opcode) 
        } // end of if (core->is_hit)

        // MISS - produce request to BUS and stall the pipeline
        else{
            //printf("DEBUG: MISS! core id: %d, clk is: %d, command is %d, address is %d \n", core->id, sim->clk,opcode,address);
            if (opcode == LW){
                core->stats->read_miss++;
            }
            else{
                core->stats->write_miss++;
            }

            //Self flushing case 
            if (core->tsram[block_index]->mesi == MODIFIED)
            {
                int pattern = 0x0FFF & (core->tsram[block_index]->tag);
                pattern = pattern << 8;
                core->flush_addr = pattern | ((0X0FF & block_index) << 2);
                //printf("DEBUG: flushing tag is %d \n",core->tsram[block_index]->tag);
                //printf("DEBUG:self flushing, upcoming data is: %d,flusing data is: %d, flushing address is: %d,upcoming address is: %d\n",data,core->dsram[block_index],core->flush_addr,address);
                core->self_flushing = true;
            }

            //Initiate bus transaction with BUSRD or BUSRDX
            core->request_bus = 1;
            core->bus_addr = address;
            core->bus_cmd =  (opcode == SW) ? BUSRDX:BUSRD ;
            core->memory_stall = true; // set memory stall flag
            core->pipe_regs->MWB_regs[INST_PC]->d = -1;
            core->pipe_regs->MWB_regs[RD]->d = -1;
            return;
        }
    }// end of if((opcode == LW) || (opcode == SW))

    //normal pipes update, no memory handling (SW/LW)
    core->pipe_regs->MWB_regs[OPCODE]->d = opcode;
    core->pipe_regs->MWB_regs[ALU_OUT]->d = address; //ALU_OUT is used as address in this stage
    core->pipe_regs->MWB_regs[RD]->d = core->pipe_regs->EM_regs[RD]->q;
    core->pipe_regs->MWB_regs[INST_PC]->d = core->pipe_regs->EM_regs[INST_PC]->q;

    return;
    } 

void invalidate_other_blocks(core* cores[],core* core,int address){
    int block_index = (address >> 2) & 0x3F; // find the index from the address 

    for(int i = 0; i < NUM_CORES; i++){
        if (core->id != i && is_hit(cores[i], address)){ // invalidate other blocks
            cores[i]->tsram[block_index]->mesi = INVALID;
        }
    }
    return;
}

void shared_other_blocks(core* cores[],core* core,int address){
    int block_index = (address >> 2) & 0x3F; // find the index from the address 

    for(int i = 0; i < NUM_CORES; i++){
        if ((core->id != i) && (is_hit(cores[i], address)) && (cores[i]->tsram[block_index]->mesi == EXCLUSIVE)){ // invalidate other blocks
            cores[i]->tsram[block_index]->mesi = SHARED;
        }
    }
    return;
}


void pipe_memory_bus_snooping(core* cores[],core* core,bus* bus,simulator* sim){

    //CORE INFORMATION
    int requested_opcode = core->pipe_regs->EM_regs[OPCODE]->q; // Doesnt change since core request
    int requested_address = core->bus_addr;  // Doesnt change since core request
    int dest_reg = core->pipe_regs->EM_regs[RD]->q;
    int write_data = core->regs[dest_reg];
    int requested_block_index = (requested_address >> 2) & 0x3F;
    int requested_tag = requested_address >> 8;     
    int requested_block_offset = requested_address & 0x3; 
    int requested_data = 0;     // Will be updated later
    //BUS INFORMATION
    int upcoming_command = bus->cmd; 
    int upcoming_address = bus->addr; // Doesnt change since core request
    int upcoming_block_index = (upcoming_address >> 2) & 0x3F; 
    int upcoming_tag = upcoming_address >> 8;
    int upcoming_block_offset = upcoming_address & 0x3;
    int upcoming_data = bus->data;

    // if the core is the sender of the transaction, do nothing FIXME - optimize
    if ((bus->origid == core->id) || (upcoming_command != FLUSH) || (!core->bus_grant) || (core->self_flushing))
    {
        return;
    }

    // flush transaction - response to read request from another core or from main memory
    if (requested_block_index == upcoming_block_index)
    {    
        //printf("RECEIVING TRANSACTION: Sender id is:%d, the address is:%d, the data is:%d, trans number is:%d\n",bus->origid,upcoming_address,upcoming_data,core->transaction_number);
        core->dsram[4*upcoming_block_index + core->transaction_number] = upcoming_data; // update data in cache line

        //LAST CYCLE FOR CURRENT TRANSACTION
        if (core->transaction_number == 3)
        {
            //forward data to WB pipe-stage if it is the last transaction in current block
            core->pipe_regs->MWB_regs[OPCODE]->d = requested_opcode; // bypass opcode
            core->pipe_regs->MWB_regs[RD]->d = core->pipe_regs->EM_regs[RD]->q; // bypass rd
            core->pipe_regs->MWB_regs[INST_PC]->d = core->pipe_regs->EM_regs[INST_PC]->q; // bypass inst pc
            // SW case - write data to cache
            if (requested_opcode == SW)
            {
                core->dsram[4*requested_block_index + requested_block_offset] = write_data; // update data in cache
            }// end if (requested_opcode == SW)

            //SW case - just push the requested data to the next pipe-stage
            if (requested_opcode == LW)
            {   
                requested_data = core->dsram[4*requested_block_index + requested_block_offset]; // get data from cache
                core->pipe_regs->MWB_regs[MD]->d = requested_data; // update data in pipeline
            } //end if (requested_opcode == LW)


            //CLEAN THE CORE 
            //printf("updated data in cache,data is %d\n",core->dsram[4*requested_block_index + requested_block_offset]);
            core->memory_stall = false; // remove memory stall flag
            core->request_bus = 0; // remove bus request flag
            core->transaction_number = 0; // reset transaction number
            core->bus_cmd = NO_COMMAND;
            core->bus_grant = false;
            return;
        }//end if (transaction_number == 3)
        core->transaction_number++; // increment transaction number (between 0-3)
    }//end if (upcoming_command == FLUSH)
    return;
}

void invalidate_block(core* core,int tag, int block_index){
    if (core->tsram[block_index]->tag == tag)
    {
        core->tsram[block_index]->mesi = INVALID;
    }
}


void pipe_wb(core* core){
    int rd = core->pipe_regs->MWB_regs[RD]->q;
    int opcode = core->pipe_regs->MWB_regs[OPCODE]->q;
    bool invalid_write_attempt;

    //printf("inside pipe wb rd: %d, opcode: %d\n", rd, opcode); for debug
    //Bubble termination
    if ((core->pipe_regs->MWB_regs[INST_PC]->q == -1))
    {
        return; //no operation needed
    }

    //Protect $zero and $imm registers from writing 
    invalid_write_attempt = ( rd == 0 || rd == 1) &&
         (opcode == ADD || opcode == SUB || opcode == MUL || opcode == AND || opcode == OR || opcode == XOR ||
             opcode == SLL || opcode == SRA || opcode == SRL || opcode == JAL || opcode == LW || opcode == SW);

    if (invalid_write_attempt) return;

    //MEM case
    if (opcode == LW)
    {
        //printf("Writeback for LW case, data is: %d\n",core->pipe_regs->MWB_regs[MD]->q);
        //printf("-----------------------\n");
        core->regs[rd] = core->pipe_regs->MWB_regs[MD]->q;
        return;
    }

    if (opcode == SW)
    {
        return;
    }
    
    //ALU operation case
    core->regs[rd] = core->pipe_regs->MWB_regs[ALU_OUT]->q;
    return;
}

/////////////////////////////////////////////
// CORE CACHE FUCNTIONS
//////////////////////////////////////////////


void initialize_dsram(core* core)
{
    for(int i = 0; i < CACHE_DSRAM_DEPTH; i++) 
    {
     core->dsram[i] = 0; // Set all 32b-words in DSRAM to 0
    }
}


// Function to initialize the cache
void initialize_tsram(core* core)
{
    for(int i = 0; i < CACHE_TSRAM_DEPTH; i++) 
    {  
        core->tsram[i] = (TSRAM_Line*)malloc(sizeof(TSRAM_Line)); // Allocate memory for each TSRAM_Line
        if (core->tsram[i] == NULL) {
            fprintf(stderr, "Memory allocation failed for TSRAM line %d\n", i);
            exit(1); // Exit if memory allocation fails
        }
        core->tsram[i]->mesi = 0; // Set MESI state to 'Invalid' (2'b0)
        core->tsram[i]->tag = 0;  // Set tag to 0
    }
}


//block size =4 -> offset = 2 bits [1:0]
//cache size = 64 blocks -> index = 6 bits [7:2]
// tag = 12 bits [19:8]

bool is_hit(core* core,int address){
    int block_index = (address >> 2) & 0x3F; // find the index from the address 
    int tag = address >> 8;       // Extract tag bits from the address
    // Check if the tag matches and the MESI state is valid
    bool invalid_block = (core->tsram[block_index]->mesi == INVALID);
    // HIT- Exclusive/Shared/Modified state
    if((core->tsram[block_index]->tag == tag) && !invalid_block) // tags are match and the block is valid 
    { 
        return 1; // Return true to indicate a hit
    }
    return 0;
}

void get_data_from_cache(core*core, int address, simulator* sim,int* return_data) //read from cache
{
    int block_offset = address & 0x3; // masking 2 bits [1:0]
    int block_index = (address >> 2) & 0x3F; // find the index from the address 
    int tag = address >> 8;       // Extract tag bits from the address

    int line_dsram = 4*block_index + block_offset; // the line of the word in the dsram
    *return_data = core->dsram[line_dsram]; // Return the word that was just loaded
}


void write_data_to_cache(core* core,int address, int data, simulator* sim)
{
    int block_offset = address & 0x3; // masking 2 bits [1:0]
    int block_index = (address >> 2) & 0x3F; // find the index from the address 
    int tag = address >> 8;       // Extract tag bits from the address
    int line_dsram = 4*block_index + block_offset; // the line of the word in the dsram
    //printf("DEBUG: Writing data to cache: %d, index is:%d\n", data, block_index);
    core->dsram[line_dsram] = data;          // Write data to DSRAM
}


void initialize_bus_requests(int bus_requests[]) {
    for (int i = 0; i < NUM_CORES; i++) {
        bus_requests[i] = 0;
    }
}


void check_bus_requests(int bus_requests[], core* core0, core* core1, core* core2, core* core3, bus* bus ) 
{
    //if core need the bus: request bus=1
    if(core0->request_bus)
        bus_requests[0]= 1;
    if(core1->request_bus)
         bus_requests[1]= 1;   
    if(core2->request_bus)
         bus_requests[2]= 1; 
    if(core3->request_bus)
         bus_requests[3]= 1;
}


void send_transaction(simulator* sim,core* cores[],core* requesting_core){
    int sender = sim->sender_id;
    int command = FLUSH;
    int address = sim->bus_q->addr;
    int address_without_lsb = address & 0xFFFFFFFC; //mask the last 2 bits
    int block_index = (address >> 2) & 0x3F; // Block index calculation
    int trans_num = 3 - sim->transaction_timer;

    //Update bus due to sender
    sim->bus_d->cmd = command;
    sim->bus_d->origid = sender;
    sim->bus_d->addr = address_without_lsb + trans_num;
    
    
    if (sender == 4) // 4 for main memory
    {
        sim->bus_d->data = sim->main_memory[address_without_lsb + trans_num];
        sim->bus_d->shared = false;
    }

    if (sim->dirty_block) //other core is flushing -> update the main memory by the way
    {
        sim->bus_d->data = cores[sender]->dsram[4*block_index + trans_num];
        sim->main_memory[address_without_lsb + trans_num] = sim->bus_d->data;
        if(address_without_lsb + trans_num > sim->max_main_memory_index){
            sim->max_main_memory_index = address_without_lsb + trans_num;
        }
        sim->bus_d->shared = 1; //FIXME, correct it
    }

    if (requesting_core->self_flushing) // self-flushing -> update the main memory
    {
        sim->bus_d->data = cores[sender]->dsram[4*block_index + trans_num];
        sim->main_memory[address_without_lsb + trans_num] = sim->bus_d->data;
        if(address_without_lsb + trans_num > sim->max_main_memory_index){
            sim->max_main_memory_index = address_without_lsb + trans_num;
        }
    }

    if (sim->transaction_timer == 0) // last cycle of transaction
    {

        sim->dirty_block = 0;
        sim->busrdx_en = false;

    }//end if timer =0
    return;
}


void intialize_priority_cores(core* core0, core* core1, core* core2, core* core3, int priority_cores[])
{
    //intialize the round robin array
    // core->priority = index of the core in the array = priority for BUS
    core0->priority = 0;
    core1->priority = 1;
    core2->priority = 2;
    core3->priority = 3;
    priority_cores[core0->priority] = 0;
    priority_cores[core1->priority] = 1;
    priority_cores[core2->priority] = 2;
    priority_cores[core3->priority] = 3;
    
}

core* round_robin_arbitration(core* cores[], int priority_cores[], simulator* sim, int bus_requests[])
{
    int i;
    int core_id;
    bool transaction_done = false;
    bool exit_loop = false;

    //printf("priority cores: %d %d %d %d\n",priority_cores[0], priority_cores[1], priority_cores[2] ,priority_cores[3]);

    for(i=0;i<NUM_CORES;i++)
    {
        core_id = priority_cores[i];
        //printf("bus requests is: %d \n", bus_requests[core_id]);
        if (bus_requests[core_id]) { // Check if this core needs the bus
            for (int j = i; j < NUM_CORES - 1; j++) {
                    priority_cores[j] = priority_cores[j + 1];
                }
                priority_cores[NUM_CORES - 1] = core_id;
                exit_loop = true;
        }//end if (bus_requests[core_id])
        if(exit_loop){
                break;
        }
    }//end for loop
    bus_requests[core_id] = 0; // Reset the bus request  // FIXME - give another name for bus_requests!!!!
    //cores[core_id]->bus_grant = true; // CORE GOT THE BUS
    return cores[core_id]; //return the core that requested the bus
}

 
void update_bus(bus* bus_d, core* core) {
    //printf("updating bus, curr addr = %d, next addr = %d\n",bus->addr,core->bus_addr);
    bus_d->addr = core->bus_addr;
    bus_d->cmd = core->bus_cmd;
    bus_d->data = core->bus_data;
    bus_d->origid = core->id;
    if (core->self_flushing)
    {
        bus_d->addr = core->flush_addr;
        bus_d->cmd = FLUSH;
        bus_d->data = core->dsram[(core->flush_addr>>2) & 0x3F];
        bus_d->origid = core->id;
        return;
    }
    return;
}


void execute_transaction(core* requesting_core, core* cores[],simulator* sim) {
    //Assume that bus is updated
    int address = sim->bus_q->addr;  
    int block_index = (address >> 2) & 0x3F; // Block index calculation
    int tag = address >> 8; // Extract tag bits from the address
    int requested_command = requesting_core->bus_cmd;
    bool shared_state_flag = false; // update requesting core according to it
    int sender_bit = 0; // 0 - main mem, 1 - modified dirty core 
    
    // Self-flushing case
    if (requesting_core->self_flushing){
        sim->sender_id = requesting_core->id; // Set the sender ID
        sim->transaction_timer = 3; // Set the transaction timer for non-memory case
        requesting_core->tsram[block_index]->mesi = INVALID;
        return;
    }

    //BUSRD case - requsting core trying to read something
    if (requested_command == BUSRD)
    {
        //Iterate over all cores and update them according to requesting core current state and current buscmd
       for (int i = 0; i < NUM_CORES; i++) {

        // Skip the requesting core and invalid block
        if ((cores[i]->id == requesting_core->id)&&(cores[i]->tsram[block_index]->mesi == INVALID)){
            continue;
        }

        // Exlusive block is found
        if (is_hit(cores[i], address) && (cores[i]->tsram[block_index]->mesi == EXCLUSIVE))
        {
            cores[i]->tsram[block_index]->mesi = SHARED;
            shared_state_flag = true;
            continue;
        }

        // Shared block is found
        if (is_hit(cores[i], address) && (cores[i]->tsram[block_index]->mesi == SHARED))
        {
            shared_state_flag = true;
            continue;
        }
        
        // Modified block is found
        if (is_hit(cores[i], address) && (cores[i]->tsram[block_index]->mesi == MODIFIED)) {
            cores[i]->tsram[block_index]->mesi = SHARED; // Set the updated state for sending core
            requesting_core->tsram[block_index]->mesi = SHARED; // Set the updated state for receiving core
            requesting_core->tsram[block_index]->tag = tag; // Set the updated tag for receiving core
            sim->sender_id = i; // Set the sender ID
            sim->transaction_timer = 3; // Set the transaction timer for non-memory case
            sim->dirty_block = 1;
            return;
        }
    }// end of for

    // If there is no modified block in the cache -> read from the main memory
    requesting_core->tsram[block_index]->mesi = (shared_state_flag) ? SHARED:EXCLUSIVE; // Set the updated state for receiving core
    requesting_core->tsram[block_index]->tag = tag; // Set the updated tag for receiving core
    sim->sender_id = 4; // Set the sender ID to main memory
    sim->transaction_timer = 18; // Set the transaction timer
    }// end of BUSRD


    //////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////


    //BUSRDX case - requsting core trying to write something
    if (requested_command == BUSRDX)
    {
        //BUSRDX flag initialization - used in shared bit validation when the data comes from some core
        sim->busrdx_en = true;

        //Iterate over all cores and update them according to requesting core current state and current buscmd
       for (int i = 0; i < NUM_CORES; i++) {

        // Skip the requesting core and invalid block
        if ((cores[i]->id == requesting_core->id) && (cores[i]->tsram[block_index]->mesi == INVALID)){
            continue;
        }

        // Block is found in other core -> invalidate and totaly terminate it!
        if (is_hit(cores[i], address) && (cores[i]->tsram[block_index]->mesi != MODIFIED))
        {
            cores[i]->tsram[block_index]->mesi = INVALID;
            continue;
        }
        
        // Modified block is found
        if (is_hit(cores[i], address) && (cores[i]->tsram[block_index]->mesi == MODIFIED)) {
            cores[i]->tsram[block_index]->mesi = INVALID; // Set the updated state for sending block
            requesting_core->tsram[block_index]->mesi = MODIFIED; // Set the updated state for receiving core
            requesting_core->tsram[block_index]->tag = tag; // Set the updated tag for receiving core
            sim->sender_id = i; // Set the sender ID
            sim->transaction_timer = 3; // Set the transaction timer for non-memory case
            sim->dirty_block = 1;
            return;
        }
    }// end of for

    // If there is no modified block in the cache -> read from the main memory
    requesting_core->tsram[block_index]->mesi = MODIFIED; // Set the updated state for receiving core
    requesting_core->tsram[block_index]->tag = tag; // Set the updated tag for receiving core
    sim->sender_id = 4; // Set the sender ID to main memory
    sim->transaction_timer = 18; // Set the transaction timer
    return;
    } // end of if(BUSRDX)
    }

void create_files(char* argv[]){
        FILE* file1 = open_file_to_append("imem0.txt");
        fclose(file1);
        FILE* file2 = open_file_to_append("imem1.txt");
        fclose(file2);
        FILE* file3 = open_file_to_append("imem2.txt");
        fclose(file3);
        FILE* file4 = open_file_to_append("imem3.txt");
        fclose(file4);
        FILE* file5 = open_file_to_append("memin.txt");
        fclose(file5);
        argv[1] = "imem0.txt";
        argv[2] = "imem1.txt";
        argv[3] = "imem2.txt";
        argv[4] = "imem3.txt";
        argv[5] = "memin.txt";
        argv[6] = "memout.txt";
        argv[7] = "regout0.txt";
        argv[8] = "regout1.txt";
        argv[9] = "regout2.txt";
        argv[10] = "regout3.txt";
        argv[11] = "core0trace.txt";
        argv[12] = "core1trace.txt";
        argv[13] = "core2trace.txt";
        argv[14] = "core3trace.txt";
        argv[15] = "bustrace.txt";
        argv[16] = "dsram0.txt";
        argv[17] = "dsram1.txt";
        argv[18] = "dsram2.txt";
        argv[19] = "dsram3.txt";
        argv[20] = "tsram0.txt";
        argv[21] = "tsram1.txt";
        argv[22] = "tsram2.txt";
        argv[23] = "tsram3.txt";
        argv[24] = "stats0.txt";
        argv[25] = "stats1.txt";
        argv[26] = "stats2.txt";
        argv[27] = "stats3.txt";
}

////////////////////////////////////////////////////////////
//////// MAIN  FUNCTION
///////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    simulator* sim = (simulator*)malloc(sizeof(simulator));

    // Check if the number of input arguments is valid 
    if(argc == 1)
    {
        create_files(argv);
    }
    else if(NUMBER_OF_INPUT_ARGS != argc){
        printf("Wrong input, you gave %d arguments instead of %d", argc, NUMBER_OF_INPUT_ARGS);
        return 1;
    }

    //maybe need malloc per core?
    core* core0 = (core*)malloc(sizeof(core));
    core* core1 = (core*)malloc(sizeof(core));
    core* core2 = (core*)malloc(sizeof(core));
    core* core3 = (core*)malloc(sizeof(core));
    
    // Initialize simulation, open input files & load data to memory and disk   
    init_simulator(sim, argv[CMD_LINE_ARG_MEMIN]);

    init_core(argv[CMD_LINE_ARG_IMEM0] , core0);
    core0->id = 0;
    init_core(argv[CMD_LINE_ARG_IMEM1] , core1);
    core1->id = 1;
    init_core(argv[CMD_LINE_ARG_IMEM2] , core2);
    core2->id = 2;
    init_core(argv[CMD_LINE_ARG_IMEM3] , core3);
    core3->id = 3;

    // Run simulation, handle instructions & write output files 
    // todo - fix input files
    run_simulator(sim, core0, core1, core2, core3, argv[CMD_LINE_ARG_MEMOUT], argv[CMD_LINE_ARG_CORE0TRACE], argv[CMD_LINE_ARG_CORE1TRACE], argv[CMD_LINE_ARG_CORE2TRACE],
        argv[CMD_LINE_ARG_CORE3TRACE], argv[CMD_LINE_ARG_BUSTRACE]);

    // writing output files after run
    // regout per core writing
    write_output_file_regout_core(core0, argv[CMD_LINE_ARG_REGOUT0]);
    write_output_file_regout_core(core1, argv[CMD_LINE_ARG_REGOUT1]);
    write_output_file_regout_core(core2, argv[CMD_LINE_ARG_REGOUT2]);
    write_output_file_regout_core(core3, argv[CMD_LINE_ARG_REGOUT3]);

    // stats per core writing
    write_output_file_stats(core0, argv[CMD_LINE_ARG_STATS0]);
    write_output_file_stats(core1, argv[CMD_LINE_ARG_STATS1]);
    write_output_file_stats(core2, argv[CMD_LINE_ARG_STATS2]);
    write_output_file_stats(core3, argv[CMD_LINE_ARG_STATS3]);

    //core dsram ouput files
    write_output_file_dsram(core0, argv[CMD_LINE_ARG_DSRAM0]);
    write_output_file_dsram(core1, argv[CMD_LINE_ARG_DSRAM1]);
    write_output_file_dsram(core2, argv[CMD_LINE_ARG_DSRAM2]);
    write_output_file_dsram(core3, argv[CMD_LINE_ARG_DSRAM3]);

    //core tsram ouput files
    write_output_file_tsram(core0, argv[CMD_LINE_ARG_TSRAM0]);
    write_output_file_tsram(core1, argv[CMD_LINE_ARG_TSRAM1]);
    write_output_file_tsram(core2, argv[CMD_LINE_ARG_TSRAM2]);
    write_output_file_tsram(core3, argv[CMD_LINE_ARG_TSRAM3]);

    // implement functions to write regout, dsram, tsram, and stats files at the end of simulator ru
    free_core(core0);
    free_core(core1);    
    free_core(core2);
    free_core(core3);
    free(sim->bus_d);
    free(sim->bus_q);
    free(sim->main_memory);
    //free sim mem
    free(sim);
    return 0;
}