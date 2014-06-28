//N11235693 Junye Zhu
//Operating System Lab 1

#include <iostream>
#include <fstream>
#include <list>
#include <string>
#include <sstream>
#include <map>
#include <algorithm>
#include <vector>
#include <iomanip>
#include <stdlib.h>

#include <cstdlib>
#include <cstring>

using namespace std;

struct key_value_pair{
  string key;
  string value1;
  int value2;
};

struct module {
  int id;
  int base;
  int defcount;
  int usercount;
  int progcount;
  vector<string> definition_list;
  vector<string> user_list; 
  vector<key_value_pair> program_context;
  map<string,bool> user_state;
};

//GLOBAL VARIABLES
int LINE_NUM=1;
int OFFSET=1;
int LINE_LENGTH_CURRENT;
int LINE_LENGTH_PRE;
vector<string> SYMBOLS;
map<string,int> SYMBOL_TABLE;
map<string,bool> SYMBOL_ERROR_STATE;      //store the state of symbol, true for got error
map<string,bool> SYMBOL_USED_STATE;       //store the state of symbol, true for used
map<string,int> SYMBOL_MODULE;            //store the module_id for each symbol 
vector<module> MODULES;
vector<string> WARNINGS;

//PARSE ERROR MESSAGE
int NUM_EXPECTED=0;
int SYM_EXPECTED=1;
int ADDR_EXPECTED=2;
int SYM_TOLONG=3;
int TO_MANY_DEF_IN_MODULE=4;
int TO_MANY_USE_IN_MODULE=5;
int TO_MANY_INSTR=6;

//MAX machine size
int MAX_MACHINE_SIZE=512;

int main(int argc, char** argv) {
  if (argc != 2){
    cout << "Wrong Number of arguments, exiting ..." << endl;
    exit(0);
  }

  string file_name=argv[1];
	ifstream infile;
  infile.open (file_name.c_str());
	if (!infile.is_open()){
    cout << "Error Opening File, Please make sure correct input file name typed" << endl;
    exit(0);
  }

  //claim functions here
  bool is_symbol(const string &s);
  bool is_type(const string &s);
  bool is_instruction(const string &s);
  bool is_num(const string &s);
  string tokenize(string &s, int &start, int &end);
  void __parseerror(int errorcode);

	string line,token;
  string symbol,type;
  int instr;
  int defcount, usercount, progcount;
	int base = 0;
	int status = 1;
  int module_id=1;
  LINE_NUM=0;
  LINE_LENGTH_CURRENT=0;
  LINE_LENGTH_PRE=0;
 
  //***********  PASS ONE START  *********
	while (!infile.eof()){
    getline(infile, line);
    LINE_NUM++;

    LINE_LENGTH_PRE=LINE_LENGTH_CURRENT;
    LINE_LENGTH_CURRENT=line.length();
    if (line.length() == 0) continue;

    int start=0;
    int end=0;
    OFFSET=1;
    while (end < line.length()) {
      OFFSET+=(end-start);
      start=end; 
      token=tokenize(line,start,end);
      if (token == "") continue;

      if (status==1){
        module m;
        m.id=module_id++;
        MODULES.push_back(m);
      }

      if (status==8){
        key_value_pair p = {type,token,-1};
        MODULES.back().program_context.push_back(p);
      }

      switch(status){
        case 1:                   //Got defcount
          if (is_num(token)){
            defcount=atoi(token.c_str());
            MODULES.back().defcount=defcount;
          }
          else{
            __parseerror(NUM_EXPECTED);
            exit(1);
          }
          if (defcount == 0)         //Pass to Definition List
            status=4;             
          else if (defcount < 17)                 //Skip Definition List Part, Switch to User List
            status=2;             
          else{
            __parseerror(TO_MANY_DEF_IN_MODULE);               //More than 16 definition in Def List
            exit(1);
          }
          break;

        case 2:                   //Definition List Part, got symbol name
          if (token.length()>16){
            __parseerror(SYM_TOLONG);
            exit(1);
          }
          if (is_symbol(token)){
            symbol=token;
            status=3;
          }
          else{
            __parseerror(SYM_EXPECTED);
            exit(1);
          }
          break;

        case 3:                   //Definition List Part, got symbol address
          if (is_num(token)){
            //RULE 2
            if (find(SYMBOLS.begin(), SYMBOLS.end(), symbol) != SYMBOLS.end()){
              SYMBOL_ERROR_STATE[symbol]=true;
            }else{
              SYMBOLS.push_back(symbol);
              SYMBOL_TABLE[symbol]=base+atoi(token.c_str());
              SYMBOL_ERROR_STATE[symbol]=false;
              SYMBOL_USED_STATE[symbol]=false;
              SYMBOL_MODULE[symbol]=MODULES.back().id;
              MODULES.back().definition_list.push_back(symbol);
            }
            defcount--;
          }else{
            __parseerror(NUM_EXPECTED);               //More than 16 definition in Def List
            exit(1);
          }
          if(defcount>0)
            status=2;
          else if(defcount==0)                 
            status=4;
          break;

        case 4:                   //Got usercount        
          if (is_num(token)){
            usercount=atoi(token.c_str());
            MODULES.back().usercount=usercount;
          }
          else{
            __parseerror(NUM_EXPECTED);
            exit(1);
          }
          if (usercount==0)        
            status=6;             //Pass to User List
          else if (usercount<17) 
            status=5;           //Pass to Program Context
          else{
            __parseerror(TO_MANY_USE_IN_MODULE);               //More than 16 definition in Def List
            exit(1);
          }
          break;

        case 5:                   //User List Part, got symbol name
          if (token.length()>16){
            __parseerror(SYM_TOLONG);
            exit(1);
          }
          if (is_symbol(token)){
            usercount--;
            MODULES.back().user_list.push_back(token);
          }
          else{
            __parseerror(SYM_EXPECTED);
            exit(1);
          }
          if(usercount==0) 
            status=6;
          break;

        case 6:                   //Got progcount 
          if (is_num(token)){
            progcount=atoi(token.c_str());
            MODULES.back().progcount=progcount;
            MODULES.back().base=base;

            //RULE 5
            for (vector<string>::const_iterator i=MODULES.back().definition_list.begin(); i!=MODULES.back().definition_list.end(); ++i){
              if ((SYMBOL_TABLE[*i]-base)>=progcount){
                cout << "Warning: Module " << MODULES.back().id << ": " << (*i) << " to big " << SYMBOL_TABLE[*i] << " (max=" << (progcount-1) <<") assume zero relative" << endl;
                SYMBOL_TABLE[*i]=0;
              }
            }

            base+=progcount;
            if (base > MAX_MACHINE_SIZE){
              __parseerror(TO_MANY_INSTR);
              exit(1);
            }

          }
          else{
            __parseerror(NUM_EXPECTED);
            exit(1);
          }
          if (progcount>0)
            status=7;             //Pass to Program Text Part
          else if (progcount==0)
            status=1;             //Skip Program Text, Switch to Def List Part
          break;

        case 7:                   //Program Text Part, got type
          if (is_type(token)){
            type=token;
            status=8;
          }
          else{
            __parseerror(ADDR_EXPECTED);
            exit(1);
          }
          break;

        case 8:                  //Program Text Part, got instruction 
          status=7;
          progcount--;
          if (progcount==0)
            status=1;
          break;

        default: printf("errorround1");
      }
    }
  }
  infile.close();
  
  LINE_NUM--;
  OFFSET=LINE_LENGTH_PRE+1;
  if (status == 2 || status ==5 ){
    __parseerror(SYM_EXPECTED);
    exit(1);
  }else if (status == 3 || status == 4 || status == 6){
    __parseerror(NUM_EXPECTED);
    exit(1);
  }else if (status == 7 || status == 8){
    __parseerror(ADDR_EXPECTED);
    exit(1);
  }

  //print symbol table
  cout << "Symbol Table" << endl;
  for (vector<string>::const_iterator i=SYMBOLS.begin(); i!=SYMBOLS.end(); ++i){
    cout << *i << "=" << SYMBOL_TABLE[*i];
    if (SYMBOL_ERROR_STATE[symbol] == true){
      cout << " Error: This variable is multiple times defined; first value used";
    }
    cout << endl;
  }

  cout << endl;

  //***********  PASS TWO START  *********
  int line_count=0;
  cout << "Memory Map\n";
  int opcode,operand;
  for (vector<module>::iterator i=MODULES.begin();i!=MODULES.end();++i){
    //Process MODULES[i]
    module_id=(*i).id;
    base=(*i).base;
    defcount=(*i).defcount;
    usercount=(*i).usercount;
    progcount=(*i).progcount;

    for (vector<string>::const_iterator j=(*i).user_list.begin(); j!=(*i).user_list.end(); ++j){
      //initialize user state table
      (*i).user_state[*j]=false;
    }

    for (vector<key_value_pair>::iterator j=(*i).program_context.begin();j!=(*i).program_context.end();++j){
      type=(*j).key;
      token=(*j).value1;

      cout << setw(3) << setfill('0') << line_count++ << ": ";
      if (type=="I"){
        if (token.length() <= 4){
          (*j).value2=atoi(token.c_str());
          cout << setw(4) << setfill('0') << (*j).value2 << endl;

        //RULE 10
        }else{
          (*j).value2=9999;
          cout << setw(4) << setfill('0') << (*j).value2 << " Error: Illegal immediate value; treated as 9999" << endl;
        }

      }else{

        if (token.length() <=4){
          instr=atoi(token.c_str());
          opcode=instr/1000;
          operand=instr%1000;

        //RULE 11
        }else{
          (*j).value2=9999;
          cout << setw(4) << setfill('0') << (*j).value2 << " Error: Illegal opcode; treated as 9999" << endl;
          continue;
        }

        if (type=="A"){

          //RULE 8
          if (operand>=MAX_MACHINE_SIZE){
            (*j).value2=1000*opcode+0;
            cout << setw(4) << setfill('0') << (*j).value2 << " Error: Absolute address exceeds machine size; zero used" << endl;
          }else{
            (*j).value2=instr;
            cout << setw(4) << setfill('0') << (*j).value2 << endl;
          }

        }else if (type=="R"){

          //RULE 8
          if (operand>=MAX_MACHINE_SIZE){
            (*j).value2=1000*opcode+base;
            cout << setw(4) << setfill('0') << (*j).value2 << " Error: Absolute address exceeds machine size; zero used" << endl;

          //RULE 9
          }else if (operand>=progcount){
            (*j).value2=1000*opcode+base;
            cout << setw(4) << setfill('0') << (*j).value2 << " Error: Relative address exceeds module size; zero used" << endl;

          }else{
            operand+=base;
            (*j).value2=1000*opcode+operand;
            cout << setw(4) << setfill('0') << (*j).value2 << endl;
          }

        }else if (type=="E"){

          //RULE 6
          if (operand >= (*i).user_list.size()){
            (*j).value2=instr;
            cout << setw(4) << setfill('0') << (*j).value2 << " Error: External address exceeds length of uselist; treated as immediate" << endl;

          }else{
            symbol= (*i).user_list[operand];
            (*i).user_state[symbol]=true;

            //RULE 3
            if (find(SYMBOLS.begin(), SYMBOLS.end(), symbol) != SYMBOLS.end()){
              operand=SYMBOL_TABLE[symbol];
              SYMBOL_USED_STATE[symbol]=true;
              (*j).value2=1000*opcode+operand;
              cout << setw(4) << setfill('0') << (*j).value2 << endl;
            }else{
              (*j).value2=1000*opcode+0;
              cout << setw(4) << setfill('0') << (*j).value2 << " Error: " << symbol <<" is not defined; zero used" << endl;
            }
          }
        }
      }
    }
    //RULE 7
    for (vector<string>::const_iterator j=(*i).user_list.begin(); j!=(*i).user_list.end(); ++j){
      if ((*i).user_state[*j]==false){
        cout << "Warning: Module " << module_id << ": " << (*j) << " appeared in the uselist but was not actually used" << endl;
      }
    }
  }

  cout << endl;
  //RULE 4
  for (vector<string>::const_iterator i=SYMBOLS.begin(); i!=SYMBOLS.end(); ++i){
    if (SYMBOL_USED_STATE[*i]==false){
      cout << "Warning: Module " << SYMBOL_MODULE[*i] << ": " << (*i) << " was defined but never used" << endl;
    }
  }
  cout << endl;

  return 1;
}

//Help Funtions
bool is_symbol(const string &s){
  int i=1;
  if (('a'<=s.at(0) && s.at(0)<='z') || ('A'<=s.at(0) && s.at(0)<='Z')){
    for (;i<s.length();i++){
      if (('a'<=s.at(i) && s.at(i)<='z') || ('A'<=s.at(i) && s.at(i)<='Z') || ('0'<=s.at(i) && s.at(i)<='9'))
        continue; 
      else
        break;
    }
    if (i==s.length()) return true;
  }
  return false;
}

bool is_type(const string &s){
  if (s == "I" || s == "A" || s == "E" || s == "R"){
    return true;
  }
  return false;
}

bool is_num(const string &s){
  istringstream sin(s);
  double t;
  char p;
  if (!(sin >> t))
      return false;
  if (sin >> p)
      return false;
  else
      return true;
}

string tokenize(string &s, int &start, int &end){
  int len=s.length();

  //skip the delimeter
  while (start<len && (s.at(start)=='\t' || s.at(start)==' ')){
    //skip the tabs
    if (s.at(start)=='\t'){
      while (start<len && s.at(start)=='\t'){
        start++;
      }
      //treat tabs as one character
      OFFSET++;
    }
    
    //skip the space
    while (start<len && s.at(start)==' '){
      start++;
      OFFSET++;
    }
  }
  end=start;

  //get the token
  while (end<len && s.at(end)!='\t' && s.at(end)!=' ')
    end++;

  return s.substr(start,end-start);
}

void __parseerror(int errcode) {
	static string errstr[] = {
		"NUM_EXPECTED",          // Number Expected
		"SYM_EXPECTED",          // Symbol Expected
		"ADDR_EXPECTED",         // Addressing Expected
		"SYM_TOLONG",            // Symbol Name is to long
		"TO_MANY_DEF_IN_MODULE", // More than 16 symbols in Def List
		"TO_MANY_USE_IN_MODULE", // More than 16 symbols in User List
		"TO_MANY_INSTR", // More than 16 symbols in User List
	};
	printf("Parse Error line %d offset %d: %s\n",LINE_NUM, OFFSET, errstr[errcode].c_str());
}

