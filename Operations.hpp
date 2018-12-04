#pragma once
#include <vector>
#include <string>
#include <map>
#include "Signal.hpp"
#include "BasicDevices.hpp"
using namespace std;
namespace SynthFramework {
  namespace Polymer {
    class LogicDesign;
    /*
    Operations are what we start with before any synthesis - these are then converted
    to LUTs or special function blocks, and then optimised
    All operations take buses as inputs and outputs
    */
    enum OperationType {
      OPER_B_ADD, //addition
      OPER_T_ADD_CIN, //addition with explicit CIN, internal use only
      OPER_B_SUB, //subtraction
      OPER_B_MUL, //multiplication (signedness determined by operands)
      OPER_B_DIV, //division
      OPER_B_MOD, //modulo
      OPER_B_BWOR, //bitwise operations
      OPER_B_BWAND,
      OPER_B_BWXOR,
      OPER_B_LOR, //logical operations
      OPER_B_LAND,
      OPER_B_EQ, //equals
      OPER_B_NEQ, //not equals
      OPER_B_LT, //less than
      OPER_B_LTE, //less than/equal to
      OPER_B_GT, //greater than
      OPER_B_GTE, //greater than/equal to
      OPER_B_LS, //left shift
      OPER_B_RS, //right shift
      OPER_U_BWNOT, //bitwise not
      OPER_U_LNOT, //logical not
      OPER_U_MINUS, //unary minus
      OPER_U_REG, //explicit register
      OPER_U_SREG, //explicit register for shifting (not counted for latency purposes)
      OPER_U_SEREG, //explicit register for shifting with enable (not counted for latency purposes)
      OPER_T_COND, //ternary conditional (cond/true/false)
      OPER_CONNECT, //connect two signals together
    };

    class Operation {
    public:
      Operation();
      Operation(OperationType _type, vector<Bus*> ins, Bus *out);
      string name; //name to be used as a unique identifier
      OperationType type;
      vector<Bus*> inputs;
      Bus* output;
      void Synthesise(LogicDesign* topLevel); //Convert to LUTs/device-specific blocks

      //Synthesis helper functions

      //Generate LUTs for a bitwise function
      void GenerateBitwiseLUTs(LUTDeviceType lutType, LogicDesign* topLevel);
      //Generate LUTs for an add or subtract operation, with optional explicit CIN
      void GenerateAdderLUTs(bool isSubtract, LogicDesign* topLevel, Signal *cin = nullptr);
      //Get the maximum size of all input buses
      int GetMaxInputSize();
      //Return the signal corresponding to bit j of input i, handling zero/sign extension correctly
      Signal *GetInputSignal(int i, int j, LogicDesign* topLevel);
      //Return the signal corresponding to bit j of the output, returning nullptr if j is out of range
      Signal *GetOutputSignal(int j, LogicDesign* topLevel);
      //Generate a tree of gates (usually OR, AND can also be used) to convert a value to a boolean
      Signal *ConvertToBoolean(Bus *bus, LogicDesign* topLevel, LUTDeviceType opType = Device_OR2);
      //Generate LUTs for a logical function
      void GenerateLogicalLUTs(LUTDeviceType lutType, LogicDesign* topLevel);
      //Generate LUTs for a conditional operation
      void GenerateConditionalLUTs(LogicDesign* topLevel);
      //Generate LUTs for a comparison operation
      //LT = true : less than, otherwise greater than
      //EQ = true : include equality (ie >= or <=)
      void GenerateComparisonLUTs(bool lt, bool eq, LogicDesign* topLevel);
      //Generate LUTs for an equality or inequality comparison
      void GenerateEqualityLUTs(bool inv, LogicDesign* topLevel);
      //Generate LUTs for a fixed shift
      void GenerateFixedShiftLUTs(int amount, bool isRightShift, LogicDesign* topLevel);
      //Generate LUTs for a barrel shifter shift
      void GenerateBarrelShiftLUTs(bool isRightShift, LogicDesign* topLevel);
      //Generate LUTs for a multiplication by constant
      void GenerateMultiplyByConstLUTs(LogicDesign* topLevel);
      //Generate LUTs for a non-constant multiply
      void GenerateMultiplyLUTs(LogicDesign* topLevel);
      //Generate LUTs for division using the non-restoring algorithm
      void GenerateDivisionLUTs(LogicDesign* topLevel);
    };

    struct OperationTypeInfo {
      string name; //Operation name
      int nOperands; //Number of operands
    };
    //Look up an operation by name, setting type if found
    bool GetOperationByName(string name, OperationType &type);

    extern map<OperationType, OperationTypeInfo> OperationInfo;
  }
}
