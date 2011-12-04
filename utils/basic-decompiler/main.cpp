#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <sstream>
#include <vector>

#define INDENT "    "

typedef struct
{
    std::string in, out;
} Instruction;

std::map<std::string, Instruction> instructions =
{
    // r: register
    // R: input-only register
    // i: immediate value
    // a: addition ("%a0" is "+ %i0" if positive, and "- %i0" otherwise)
    // o: offset (like "32($sp)" )
    // f: function
    // l: loc_
    // x: special, for jr (=> goto <reg> or return (v1 << 32) | v0 if it's $ra)
    {"li", {"r0 i0", "%r0 = %i0"}},
    {"lui", {"r0 i0", "%r0 = %i00000"}},
    {"move", {"r0 R1", "%r0 = %R1"}},
    {"nop", {"", ""}},

    {"addiu", {"r0 R1 a0", "%r0 = %R1 %a0"}},
    {"addu", {"r0 R1 R2", "%r0 = %R1 + %R2"}},
    {"subu", {"r0 R1 R2", "%r0 = %R1 - %R2"}},

    {"and", {"r0 R1 R2", "%r0 = %R1 & %R2"}},
    {"andi", {"r0 R1 i0", "%r0 = %R1 & %i0"}},
    {"nor", {"r0 R1 R2", "%r0 = ~(%R1 | %R2)"}},
    {"or", {"r0 R1 R2", "%r0 = %R1 | %R2"}},
    {"ori", {"r0 R1 i0", "%r0 = %R1 | %i0"}},
    {"xori", {"r0 R1 i0", "%r0 = %R1 ^ %i0"}},

    {"beq", {"R0 R1 l0", "%R0 == %R1"}},
    {"beql", {"R0 R1 l0", "%R0 == %R1"}},
    {"beqz", {"R0 l0", "%R0 == 0"}},
    {"beqzl", {"R0 l0", "%R0 == 0"}},
    {"bne", {"R0 R1 l0", "%R0 != %R1"}},
    {"bnel", {"R0 R1 l0", "%R0 != %R1"}},
    {"bnez", {"R0 l0", "%R0 != 0"}},
    {"bnezl", {"R0 l0", "%R0 != 0"}},
    {"bgez", {"R0 l0", "%R0 >= 0"}},
    {"bgezl", {"R0 l0", "%R0 >= 0"}},
    {"blez", {"R0 l0", "%R0 <= 0"}},
    {"blezl", {"R0 l0", "%R0 <= 0"}},
    {"bltz", {"R0 l0", "%R0 < 0"}},
    {"bltzl", {"R0 l0", "%R0 < 0"}},

    {"jal", {"f0", "v0, v1 = %f0"}},
    {"jr", {"x0", "%x0"}},
    {"j", {"l0", "%l0"}},
    {"jalr", {"f0", "v0, v1 = %f0"}},

    {"lb", {"r0 o0", "%r0 = *(char*)(%o0)"}},
    {"lbu", {"r0 o0", "%r0 = *(unsigned char*)(%o0)"}},
    {"lh", {"r0 o0", "%r0 = *(short*)(%o0)"}},
    {"lhu", {"r0 o0", "%r0 = *(unsigned short*)(%o0)"}},
    {"lw", {"r0 o0", "%r0 = *(int*)(%o0)"}},
    {"sb", {"R0 o0", "*(char*)(%o0) = %R0"}},
    {"sh", {"R0 o0", "*(short*)(%o0) = %R0"}},
    {"sw", {"R0 o0", "*(int*)(%o0) = %R0"}},

    {"seb", {"r0 R1", "%r0 = (char)%R1"}},
    {"seh", {"r0 R1", "%r0 = (short)%R1"}},

    {"sll", {"r0 R1 i0", "%r0 = %R1 << %i0"}},
    {"sllv", {"r0 R1 R2", "%r0 = %R1 << %R2"}},
    {"srl", {"r0 R1 i0", "%r0 = (unsigned int)%R1 >> %i0"}},
    {"srlv", {"r0 R1 R2", "%r0 = (unsigned int)%R1 >> %R2"}},
    {"sra", {"r0 R1 i0", "%r0 = %R1 >> %i0"}},
    {"srav", {"r0 R1 R2", "%r0 = %R1 >> %R2"}},

    {"slt", {"r0 R1 R2", "%r0 = %R1 < %R2"}},
    {"sltu", {"r0 R1 R2", "%r0 = (unsigned int)%R1 < (unsigned int)%R2"}},
    {"slti", {"r0 R1 i0", "%r0 = %R1 < %i0"}},
    {"sltiu", {"r0 R1 i0", "%r0 = (unsigned int)%R1 < (unsigned int)%i0"}},

    // special instructions, not using the format because they're too complicated
    {"ext", {"r0 R1 i0 i1", "NONE"}},
    {"ins", {"r0 R1 i0 i1", "NONE"}},
    {"movz", {"r0 R1 R2", "NONE"}},
    {"movn", {"r0 R1 R2", "NONE"}},
};

std::string getArg(char type, std::string arg)
{
    if (type == 'r' || type == 'R')
    {
        arg = arg.substr(1); // remove the '$'
        if (arg == "zr")
            arg = "0";
    }
    else if (type == 'a')
    {
        if (arg[0] == '-')
            arg = "- " + arg.substr(1);
        else
            arg = "+ " + arg;
    }
    else if (type == 'o')
    {
        std::string off = getArg('a', arg.substr(0, arg.find_first_of('(')));
        std::string reg = getArg('r', arg.substr(arg.find_first_of('(') + 1).substr(0, 3));
        arg = reg + " " + off;
    }
    else if (type == 'f')
    {
        arg += "(...)";
    }
    else if (type == 'x')
    {
        std::string reg = getArg('r', arg);
        if (reg == "ra")
            arg = "return (v1 << 32) | v0";
        else
            arg = reg;
    }
    return arg;
}

std::vector<std::string> getArgs(std::string instr)
{
    std::vector<std::string> argList;
    std::string args = instr.substr(instr.find_first_of(' '));
    size_t argPos = args.find_first_not_of(" \t");
    if (argPos != std::string::npos)
        args = args.substr(argPos);

    std::istringstream argStream(args);
    for (;;)
    {
        std::string curArg;
        argStream >> curArg;
        if (curArg == "")
            break;
        curArg = curArg.substr(0, curArg.find_first_of(','));
        argList.push_back(curArg);
    }
    return argList;
}

std::string getInstr(std::string line)
{
    return line.substr(33);
}

std::string getInstrType(std::string line)
{
    std::string instr = getInstr(line);
    return instr.substr(0, instr.find_first_of(' '));
}

bool isBranch(std::string line)
{
    return (getInstrType(line)[0] == 'b');
}

bool isJump(std::string line)
{
    return (getInstrType(line)[0] == 'j');
}

bool needsGoto(std::string line)
{
    return isBranch(line) || getInstrType(line) == "j";
}

bool isLikely(std::string line)
{
    std::string type = getInstrType(line);
    return type[type.size() - 1] == 'l';
}

Instruction *getFormat(std::string instr)
{
    std::string insType = instr.substr(0, instr.find_first_of(' '));
    if (instructions.count(insType) > 0 && instructions[insType].out != "NONE")
        return &instructions[insType];
    return NULL;
}

std::vector<std::string> getArgsWithType(std::string instr, char type)
{
    Instruction *ins = getFormat(instr);
    std::vector<std::string> argList;
    if (ins != NULL)
    {   
        std::vector<std::string> args = getArgs(instr);
        std::istringstream skelStream(ins->in);
        std::string output(ins->out);
            
        for (unsigned int i = 0; i < args.size(); i++)
        {   
            std::string curSkelArg;
            std::string curArg = args[i];
            skelStream >> curSkelArg;
            if (curSkelArg == "")
                break;

            curArg = curArg.substr(0, curArg.find_first_of(','));
            if (curSkelArg[0] == type)
                argList.push_back(curArg);
        }
    }
    return argList;
}

std::string getJumpDest(std::string line)
{
    std::vector<std::string> args;
    std::string instr = getInstr(line);

    args = getArgsWithType(instr, 'x');
    if (!args.empty())
        return args[0];

    args = getArgsWithType(instr, 'l');
    if (!args.empty())
        return args[0];

    args = getArgsWithType(instr, 'f');
    if (!args.empty())
        return args[0];

    return "";
}

std::string getCInstr(std::string line)
{
    std::string instr = getInstr(line);
    Instruction *ins = getFormat(instr);
    std::vector<std::string> args = getArgs(instr);
    if (ins != NULL)
    {
        std::istringstream skelStream(ins->in);
        std::string output(ins->out);

        for (unsigned int i = 0; i < args.size(); i++)
        {
            std::string curSkelArg;
            std::string curArg = args[i];
            skelStream >> curSkelArg;
            if (curSkelArg == "")
                break;

            curArg = curArg.substr(0, curArg.find_first_of(','));
            curArg = getArg(curSkelArg[0], curArg);

            size_t outArgPos = output.find("%" + curSkelArg);
            if (outArgPos != std::string::npos)
                output.replace(outArgPos, 3, curArg);
        }
        return output;
    }
    else if (getInstrType(line) == "ext")
    {
        std::string r1 = getArg('r', args[0]);
        std::string r2 = getArg('r', args[1]);
        int pos = stoi(args[2]);
        int size = stoi(args[3]);
        int mask = ~(0xFFFFFFFF << size);
        char maskStr[11], posStr[3];
        snprintf(maskStr, 11, "0x%08X", mask);
        snprintf(posStr, 3, "%d", pos);
        return r1 + " = (" + r2 + " >> " + posStr + ") & " + maskStr;
    }
    else if (getInstrType(line) == "ins")
    {
        std::string r1 = getArg('r', args[0]);
        std::string r2 = getArg('r', args[1]);
        int pos = stoi(args[2]);
        int size = stoi(args[3]);
        int mask = ~(0xFFFFFFFF << size) << pos;
        char mask2[11];
        snprintf(mask2, 11, "0x%08X", ~mask);
        if (r2 == "zr")
            return r1 + " &= " + mask2;
        else
        {
            char mask1[11], posStr[3];
            snprintf(mask1, 11, "0x%08X", mask);
            snprintf(posStr, 3, "%d", pos);
            return r1 + " = (" + r1 + " & " + mask2 + ") | ((" + r2 + " << " + posStr + ") & " + mask1 + ")";
        }
    }
    else
        return "asm(\"" + instr + "\")";
}

std::string getCInstrWithRef(std::string line, std::string &dataRef)
{
    std::string out = getCInstr(line) + ";";
    if (dataRef != "") {
        out += " // " + dataRef;
        dataRef = "";
    }
    return out;
}

void showInstr(std::string line, std::string &dataRef)
{
    std::string type = getInstrType(line);
    if (type != "nop")
    {  
        if (type == "movz")
        {  
            std::vector<std::string> args = getArgs(getInstr(line));
            std::cout << INDENT << "if (" << getArg('r', args[2]) << " == 0)" << std::endl;
            std::cout << INDENT << INDENT << getArg('r', args[0]) << " = " << getArg('r', args[1]) << ";" << std::endl;
        }
        else if (type == "movn")
        {  
            std::vector<std::string> args = getArgs(getInstr(line));
            std::cout << INDENT << "if (" << getArg('r', args[2]) << " != 0)" << std::endl;
            std::cout << INDENT << INDENT << getArg('r', args[0]) << " = " << getArg('r', args[1]) << ";" << std::endl;
        }
        else
            std::cout << INDENT << getCInstrWithRef(line, dataRef) << std::endl;
    }
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " FILENAME ENDADDR" << std::endl;
        return 1;
    }
    std::ifstream f(argv[1]);
    if (!f) {
        std::cout << "Couldn't open file!" << std::endl;
        return 1;
    }
    std::string addr(argv[2]);
    std::string line;
    std::string dataRef("");
    bool inText = false;
    bool inDelaySlot = false;
    bool condition = false;
    bool stop = false;
    bool firstFunc = true;
    bool printGoto = false;
    bool likely = false;
    std::string oldInstr;
    std::string jumpDest;
    while (getline(f, line) && !stop)
    {
        if (line.find(addr) != std::string::npos)
            stop = true;

        if (!inText && line.find("Section .text") != std::string::npos)
            inText = true;
        else
        {
            if (line.find("Section ") != std::string::npos && line.find("data") != std::string::npos)
                inText = false;
            else if (line.find("Subroutine ") != std::string::npos)
            {
                if (!firstFunc)
                    std:: cout << "}" << std::endl << std::endl;
                std::string func = line.substr(13);
                func = func.substr(0, func.find_first_of(' '));
                std::cout << func << "(...)" << std::endl;
                std::cout << "{" << std::endl;
                firstFunc = false;
            }
            else if (line.find("Data ref") != std::string::npos)
            {
                dataRef = line.substr(2);
            }
            else if (line.find("loc_") == 0)
            {
                std::cout << std::endl << line.substr(0, 15) << std::endl;
            }
            else if (line.find("\t0x") == 0 || line.find("    0x") == 0 || line.find("        0x") == 0)
            {
                if (isBranch(line) || isJump(line))
                {
                    inDelaySlot = true;
                    condition = isBranch(line);
                    printGoto = needsGoto(line);
                    likely = isLikely(line);
                    oldInstr = getCInstr(line);
                    jumpDest = getJumpDest(line);
                }
                else if (inDelaySlot)
                {
                    if (condition)
                    {
                        if (likely)
                        {
                            std::cout << INDENT << "if (" << oldInstr << ")" << std::endl;
                            std::cout << INDENT << "{" << std::endl;
                            std::cout << INDENT;
                            showInstr(line, dataRef);
                            std::cout << INDENT << INDENT << "goto " << jumpDest << ";" << std::endl;
                            std::cout << INDENT << "}" << std::endl;
                        }
                        else
                        {
                            std::vector<std::string> args = getArgsWithType(line.substr(33), 'r');
                            bool regModified = false;
                            if (args.empty())
                                regModified = true;
                            else
                                for (unsigned int i = 0; i < args.size(); i++)
                                    if (oldInstr.find(args[i]) != std::string::npos)
                                        regModified = true;

                            if (regModified)
                            {
                                std::cout << INDENT << "cond = (" << oldInstr << ");" << std::endl;
                                showInstr(line, dataRef);
                                std::cout << INDENT << "if (cond)" << std::endl;
                            }
                            else {
                                showInstr(line, dataRef);
                                std::cout << INDENT << "if (" << oldInstr << ")" << std::endl;
                            }
                            std::cout << INDENT << INDENT << "goto " << jumpDest << ";" << std::endl;
                        }
                    }
                    else
                    {
                        showInstr(line, dataRef);

                        if (printGoto)
                            std::cout << INDENT << "goto " << jumpDest << ";" << std::endl;
                        else
                            std::cout << INDENT << oldInstr << ";" << std::endl;
                    }
                    inDelaySlot = false;
                }
                else
                    showInstr(line, dataRef);
            }
        }
    }
    if (!firstFunc)
        std::cout << "}" << std::endl << std::endl;
    return 0;
}

