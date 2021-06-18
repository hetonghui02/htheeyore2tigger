#include <iostream>
#include <fstream>
#include <unistd.h>
#include "ast.hpp"
#include "parser.tab.hpp"
extern FILE * yyin;
extern FILE * yyout;

using namespace std;

inline void myassert_fail(int line, const char* fn){
    printf("error: assertion failed\n%s (line %d)\n",fn, line);
    exit(5+line%100);
}

#define assert(x) do {if(!x) myassert_fail(__LINE__, __FILE__);}  while(0);

void Compile(){
  BaseEeyoreAST* root;
  yyparse(&root);
  dynamic_cast<ProgramAST *>(root)->generator();
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    cerr << "usage: " << argv[0] << " -S -t <INPUT> -o <OUTPUT> " << endl;
    return 1;
  }

  char opt;
  char *sysy_file_path = nullptr;
	char *output_file_path = nullptr;
  while( (opt = getopt(argc, argv, "t:o:S")) != -1 ){
		if(opt == 't') sysy_file_path = optarg;
		if(opt == 'o') output_file_path = optarg;
	}

  FILE* input = fopen(sysy_file_path, "r");
  if(input == nullptr){
    cerr<<"Error, no input file!"<<endl;
    cerr << "usage: " << argv[0] << " -S -t <INPUT> -o <OUTPUT> " << endl;
    return 1;
  } 
  FILE* output = fopen(output_file_path, "w");
  if(output == nullptr){
    cerr<<"Error, no output file!"<<endl;
    cerr << "usage: " << argv[0] << " -S -t <INPUT> -o <OUTPUT> " << endl;
    return 1;
  }
  yyin = input;
  yyout = output;
  
  streambuf* coutBuf = cout.rdbuf();
  ofstream of(output_file_path);
  streambuf* fileBuf = of.rdbuf();
  cout.rdbuf(fileBuf);

  Compile();
  
  cout.rdbuf(coutBuf);

  fclose(input);
  fclose(output);
}

