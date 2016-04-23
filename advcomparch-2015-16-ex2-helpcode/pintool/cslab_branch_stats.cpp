#include "pin.H"

#include <iostream>
#include <fstream>

/* ===================================================================== */
/* Commandline Switches                                                  */
/* ===================================================================== */
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE,    "pintool",
    "o", "cslab_branch_stats.out", "specify output file name");
/* ===================================================================== */

/* ===================================================================== */
/* Global Variables                                                      */
/* ===================================================================== */
struct branch_stats_s {
    UINT64 total,
           conditional[2], // [0] -> taken, [1] -> not taken
           unconditional,
           call,
           ret;
} branch_stats;

UINT64 total_instructions;
std::ofstream outFile;

/* ===================================================================== */

INT32 Usage()
{
    cerr << "This tool gathers statistics about branches of the application.\n\n";
    cerr << KNOB_BASE::StringKnobSummary();
    cerr << endl;
    return -1;
}

/* ===================================================================== */

VOID count_instruction()
{
    total_instructions++;
}

VOID call_instruction()
{
    branch_stats.call++;
    branch_stats.total++;
}

VOID ret_instruction()
{
    branch_stats.ret++;
    branch_stats.total++;
}

VOID conditional_instruction(BOOL taken)
{
    branch_stats.conditional[taken]++;
    branch_stats.total++;
}

VOID unconditional_instruction()
{
    branch_stats.unconditional++;
    branch_stats.total++;
}

VOID Instruction(INS ins, void * v)
{
    if (INS_Category(ins) == XED_CATEGORY_COND_BR)
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)conditional_instruction,
                       IARG_BRANCH_TAKEN, IARG_END);
	else if (INS_Category(ins) == XED_CATEGORY_UNCOND_BR)
             INS_InsertCall(ins, IPOINT_BEFORE,
             (AFUNPTR)unconditional_instruction, IARG_END);
    else if (INS_IsCall(ins))
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)call_instruction, IARG_END);
    else if (INS_IsRet(ins))
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)ret_instruction, IARG_END);

    // Count each and every instruction
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)count_instruction, IARG_END);
}

/* ===================================================================== */

VOID Fini(int code, VOID * v)
{
    // Report total instructions and total cycles
    outFile << "Total Instructions: " << total_instructions << "\n";
    outFile << "\n";

    outFile << "Branch statistics:\n";
    outFile << "  Total-Branches: " << branch_stats.total << "\n";
    outFile << "  Conditional-Taken-Branches: " << branch_stats.conditional[0] << "\n";
    outFile << "  Conditional-NotTaken-Branches: " << branch_stats.conditional[1] << "\n";
    outFile << "  Unconditional-Branches: " << branch_stats.unconditional << "\n";
    outFile << "  Calls: " << branch_stats.call << "\n";
    outFile << "  Returns: " << branch_stats.ret << "\n";

    outFile.close();
}

VOID roi_begin()
{
    INS_AddInstrumentFunction(Instruction, 0);
}

VOID roi_end()
{
    // We need to manually call Fini here because it is not called by PIN
    // if PIN_Detach() is encountered.
    Fini(0, 0);
    PIN_Detach();
}

VOID Routine(RTN rtn, void *v)
{
    RTN_Open(rtn);

    if (RTN_Name(rtn) == "__parsec_roi_begin")
        RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)roi_begin, IARG_END);
    if (RTN_Name(rtn) == "__parsec_roi_end")
        RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)roi_end, IARG_END);

    RTN_Close(rtn);
}

/* ===================================================================== */

int main(int argc, char *argv[])
{
    PIN_InitSymbols();

    if(PIN_Init(argc,argv))
        return Usage();

    // Open output file
    outFile.open(KnobOutputFile.Value().c_str());

    // Instrument function calls in order to catch __parsec_roi_{begin,end}
    RTN_AddInstrumentFunction(Routine, 0);

    // Called when the instrumented application finishes its execution
    PIN_AddFiniFunction(Fini, 0);

    // Never returns
    PIN_StartProgram();
    
    return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
