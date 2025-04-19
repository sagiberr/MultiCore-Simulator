#include<stdio.h>
#include<string.h>
#include<stdbool.h>
#include<stdlib.h>
#include<stdint.h>
#include<assert.h>

#define NUMBER_OF_INPUT_ARGS (28) // 28 comand line args
#define MAX_LINE_LENGTH (500) // FIXME - check it
#define NUMBER_OF_CORE_REGS (16) // 16 regs for each core
#define INST_MEMORY_DEPTH (1024) // 32 b* 1024 rows
#define CACHE_DSRAM_DEPTH (256) // 32b * 256 rows
#define CACHE_NUM_OF_BLOCKS (64) // 256/4 rows = 64 blocks (4 rows = 16B each)
#define CACHE_TSRAM_DEPTH (64) // 14b * 64 rows
#define CACHE_BLOCK_SIZE (4) // 4 rows * 32b
#define CACHE_TAG_WIDTH (12) // 0-11 bits in TSRAM row
#define MAIN_MEMORY_DEPTH (1048576) // 2^20 words = 1MB
#define MAIN_MEMORY_DELAY (16) // 16 clock cycles for main mem delay
#define PIPE_REGS_WIDTH (6) // width of pipe-register
#define NUM_CORES (4) //4 cores


// bus struct - will hold every cycle the bus fields
typedef struct bus {
    unsigned int origid : 3;
    unsigned int cmd : 2;
    unsigned int addr : 20;
    unsigned int data : 32;
    unsigned int shared : 1;
} bus;

// Simulator data & state
// handles the PC of 4 cores, manages memory
typedef struct simulator {
    unsigned int PC_0;
    unsigned int PC_1;
    unsigned int PC_2;
    unsigned int PC_3;
    unsigned int* main_memory;  // main memory, shared to accsess between the cores
    int max_main_memory_index; // the max index of the main memory
    bool         run_en;
    long clk; 
    bus* bus_d;
    bus* bus_q;
    int  transaction_timer;
    bool bus_is_busy;
    int sender_id;
    int dirty_block;
    bool busrdx_en;
    bool arb_is_done;
    bool bus_is_updated;
    bool last_transaction;
    //FIXME - add statistics 
} simulator;

typedef struct stats {
    int cycles;
    int instructions;
    int read_hit;
    int write_hit;
    int read_miss;
    int write_miss;
    int decode_stall;
    int mem_stall;
} stats;

//Metadata for each block in the cache
typedef struct {
    unsigned int mesi : 2; // 2 bits for MESI state machine (0 - invalid, 3 - modified == dirty)
    unsigned int tag : 12;      // 12 bits for tag
} TSRAM_Line;

// PIPELINE STRUCTS
struct ff
{
  int d; // ff input, sampled in any rising edge
  int q; // ff output, sampled in any rising edge
} typedef ff;


// PIPE-REGS enum
typedef enum _PIPE_REGS_NAMES{
    //FD regs
    INST = 0,
    INST_PC = 5,
    //DE regs
    OPCODE = 0,
    R_RS = 1,
    R_RT = 2,
    IMM = 3,
    RD = 4,
    //EM regs
    ALU_OUT = 1,
    //MWB regs
    MD = 2,
} _PIPE_REGS_NAMES;


struct pipeline_regs
{
    ff* FD_reg[PIPE_REGS_WIDTH]; //includes the instruction that read from IMEM (fetch, decode), PC
    ff* DE_regs[PIPE_REGS_WIDTH]; //includes: opcode,R[rs],R[rt],IMM,rd - for WB stage, PC
    ff* EM_regs[PIPE_REGS_WIDTH]; //inculdes: opcode,ALU output, R[rt],rd, PC 
    ff* MWB_regs[PIPE_REGS_WIDTH]; //includes: opcode, ALU, MD, rd, PC
} typedef pipeline_regs;


typedef struct core {
    unsigned int PC; // should we have PC in core or in simulator only?
    unsigned int regs[NUMBER_OF_CORE_REGS]; // general purpuse 
    unsigned int imem[INST_MEMORY_DEPTH]; //instructions memory
    unsigned int dsram[CACHE_DSRAM_DEPTH]; //Cache data memory
    TSRAM_Line* tsram[CACHE_TSRAM_DEPTH];//Cache TSRAM of 64 lines
    pipeline_regs* pipe_regs; //
    bool halt_en; // Halt enable - set at decode stage
    bool branch_en; // Branch enable - set at decode stage
    unsigned int priority; // round robin - priority 0-3
    int request_bus;
    stats* stats;
    int wb_sel; // WB stage MUX (0-ALU,1-MD)
    bool stall_en;
    bool core_done;
    int next_PC; // Next PC to be fetched
    int stall_cycles; // Stall cycles counter
    int bus_cmd; // Bus command
    int bus_addr; // Bus address
    int bus_data; // Bus data
    bool bus_grant;
    int id; // Core ID
    bool is_hit; // Flag to indicate if the data is in the cache
    bool memory_stall; // Memory stall flag
    int transaction_number; // Transaction number (0-3)
    long last_clk; // Last clock cycle
    int flush_addr;
    bool self_flushing;

} core;

typedef enum _cmd_line_arg {
    CMD_LINE_ARG_IMEM0 = 1,
    CMD_LINE_ARG_IMEM1 = 2,
    CMD_LINE_ARG_IMEM2 = 3,
    CMD_LINE_ARG_IMEM3 = 4,
    CMD_LINE_ARG_MEMIN = 5,
    CMD_LINE_ARG_MEMOUT = 6,
    CMD_LINE_ARG_REGOUT0 = 7,
    CMD_LINE_ARG_REGOUT1 = 8,
    CMD_LINE_ARG_REGOUT2 = 9,
    CMD_LINE_ARG_REGOUT3 = 10,
    CMD_LINE_ARG_CORE0TRACE = 11,
    CMD_LINE_ARG_CORE1TRACE = 12,
    CMD_LINE_ARG_CORE2TRACE = 13,
    CMD_LINE_ARG_CORE3TRACE = 14,
    CMD_LINE_ARG_BUSTRACE = 15,
    CMD_LINE_ARG_DSRAM0 = 16,
    CMD_LINE_ARG_DSRAM1 = 17,
    CMD_LINE_ARG_DSRAM2 = 18,
    CMD_LINE_ARG_DSRAM3 = 19,
    CMD_LINE_ARG_TSRAM0 = 20,
    CMD_LINE_ARG_TSRAM1 = 21,
    CMD_LINE_ARG_TSRAM2 = 22,
    CMD_LINE_ARG_TSRAM3 = 23,
    CMD_LINE_ARG_STATS0 = 24,
    CMD_LINE_ARG_STATS1 = 25,
    CMD_LINE_ARG_STATS2 = 26,
    CMD_LINE_ARG_STATS3 = 27,
} cmd_line_arg;

// MESI enum
typedef enum _MESI_STATUS{
    MODIFIED = 3,
    EXCLUSIVE = 2,
    SHARED = 1,
    INVALID = 0,
} MESI_STATUS;

typedef enum _MESI_COMAND{
    BUSRD = 1,
    BUSRDX = 2,
    FLUSH = 3,
    NO_COMMAND = 0,
} _MESI_COMAND;



// Map opcode name to number 
typedef enum _opcode {
    ADD = 0,
    SUB = 1,
    AND = 2,
    OR = 3,
    XOR = 4,
    MUL = 5,
    SLL = 6,
    SRA = 7,
    SRL = 8,
    BEQ = 9,
    BNE = 10,
    BLT = 11,
    BGT = 12,
    BLE = 13,
    BGE = 14,
    JAL = 15,
    LW = 16,
    SW = 17,
    HALT = 20,
} opcode;

// File handling functions
FILE* open_file_to_read(char* fp);
FILE* open_file_to_write(char* fp);
FILE* open_file_to_append(char* fp);

// Load data from input files to simulator and cores
void load_data_from_input_file_to_main_mem(simulator* simulator, FILE* fh);
void load_data_from_input_file_to_core(core* core, FILE* fh);

// Initialize simulation: 
// 1. Validate input files
// 2. Handle input files: load memory
void init_simulator(simulator* simulator, char* memin_fp);
void init_core(char* imem_core_fp, core* core);
void create_files(char* argv[]);
void free_core(core* core);

// Run simulation:
// 2. Run fetch-decode-execute loop
// 3. Write output files 
void run_simulator(simulator* simulator, core* core0,core* core1, core* core2,core* core3, char* memout_fp, char* core0_trace_fp, char* core1_trace_fp, char* core2_trace_fp, char* core3_trace_fp, char* bus_trace_fp);

// Write output files functions (hwregtrace.txt is not written in a function, but in the main fetch-decode-execute loop)
void write_output_file_memout(simulator* simulator, char* memout_fp);
void write_output_file_regout_core(core* core, char* regout_fp);
void write_output_file_core_trace(core* core, FILE* trace_fh, long clk, bool pipeline_is_empty);
void write_output_file_bustrace(simulator* sim, char* bus_trace_fp);
void write_output_file_dsram(core* core, char* dsram_fp);
void write_output_file_tsram(core* core, char* tsram_fp); 
void write_output_file_stats(core* core, char* stats_fp); 

// Clock increment function
void inc_clk(simulator* simulator);

// Pipeline functions
void pipe_fetch(core* core);
void pipe_decode(core* core);
void pipe_execute(core* core);
void pipe_memory_hit_or_bus_request(core* core, simulator* sim);
void pipe_memory_miss_handling(core* core, bus* bus);
void pipe_wb(core* core);

//Core level functions
void core_regs_update(core* core); 


//Cache functions
void get_data_from_cache(core*core, int address, simulator* sim, int* return_data); 
void write_data_to_cache(core* core,int address, int data, simulator* sim); //FIXME - Sagi
void write_block_to_memory(int block_index,core* core, simulator* sim) ; //FIXME - Sagi
void initialize_tsram(core* core);
void initialize_dsram(core* core);
int get_data_from_memory(core*core,int address, simulator* sim); //FIXME - Sagi
void update_bus(bus* bus_d, core* core); 
bool is_hit (core* core, int address);

//MESI functions
core *cores[NUM_CORES];
void initialize_cores(core* core0,core* core1,core* core2, core* core3);
void initialize_bus_requests(int bus_requests[]);
void check_bus_requests(int bus_requests[], core* core0, core* core1, core* core2, core* core3, bus* bus );
void intialize_priority_cores(core* core0, core* core1, core* core2, core* core3, int prioriry_cores[]);
core* round_robin_arbitration(core* cores[], int prioriry_cores[],simulator* sim, int bus_requests[]);
void execute_transaction(core* ex_core, core* cores[], simulator* sim);
//void handle_busRd(core* requesting_core, core* cores[],simulator* sim);
void invalidate_block(core* core,int tag, int block_index);
void send_transaction(simulator* sim,core* cores[],core* requesting_core);
void pipe_memory_bus_snooping(core* cores[], core* core,bus* bus,simulator* sim);

//need to add the functions between
void handle_busRDX(core* requesting_core, core* cores[], simulator* sim);
void handle_flush(core* core, simulator* sim);
int cache_has_data(core* other_core,int address);
int cache_state(core* other_core,int address);
void cache_insert(core* requesting_core,core* other_core,int address);
void load_block_from_memory(core* requesting_core,int address,simulator* sim);
int memory_read(simulator* sim,int address);
bool pipeline_is_empty(core* core);
void invalidate_other_blocks(core* cores[], core* core,int address);
void shared_other_blocks(core* cores[],core* core,int address);
void bus_regs_update(simulator* sim);






