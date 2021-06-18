#include "ast.hpp"
#include <iostream>

int string2num(const string s){
    return atoi(s.c_str());
}
string num2string(int num){
    string ans = to_string(num);
    return ans;
}
bool ifGlobal(string s){
    if(globalVar.find(s)!=globalVar.end()){
        return true;
    }else  return false;
}

void ProgramAST::generator(){
    for(auto i : globalDecl){
        if(dynamic_cast<DeclAST* >(i)->isArray){
            Entry tmp("v"+num2string(vn++), true, dynamic_cast<DeclAST* >(i)->num);
            globalVar.insert(pair<string,Entry>(dynamic_cast<DeclAST* >(i)->varName,tmp));
        }else {
            Entry tmp("v"+num2string(vn++), false);
            globalVar.insert(pair<string,Entry>(dynamic_cast<DeclAST* >(i)->varName,tmp));
        }
    }
    for(auto i : globalInit){
        if(dynamic_cast<InitAST* >(i)->isArray){
            ;
        }else {
            EntryIT it = globalVar.find(dynamic_cast<InitAST* >(i)->leftName);
            it->second.val = dynamic_cast<InitAST* >(i)->rightNum;
        }
    }
    for(auto i : globalVar){
        if(i.second.isArray==false){
            tiggerCode.push_back(i.second.tiggerName + " = " + num2string(i.second.val));
        }else{
            tiggerCode.push_back(i.second.tiggerName + " = malloc " + num2string(i.second.arrNum));
        }
    }
    for(auto i : funDefs) i->generator();
    for(auto &i: tiggerCode)
		cout << i << endl;
	cout << endl;
}
void DeclAST::generator(){
    if(!funcName){
        printf("Error: no ptr funcName!\n");
        return ;
    }
    if(isArray){
        if(isTemp){
            funcName->tNum+=num/4;
        }else{
            funcName->TNum+=num/4;
        }
        Entry tmp("",true,num);
        funcName->tempVar.insert(pair<string,Entry>(varName,tmp));
    }else{
        if(isTemp){
            funcName->tNum++;
        }else{
            funcName->TNum++;
        }
        Entry tmp("",false);
        funcName->tempVar.insert(pair<string,Entry>(varName,tmp));
    }
}
void InitAST::generator(){
    //处理的是函数内部的初始化
    ;
}
void FunDefAST::generator(){
    for(auto i:dynamic_cast<StmtsAST*>(StmtsASTptr)->stmts){
        if(i->type==Decl) i->generator();
    }
    int totalVar = tNum + TNum;
    int i = 0;
    for(EntryIT it = tempVar.begin();it!=tempVar.end();++it){
        if(it->second.isArray==0){
            it->second.tiggerName = num2string(i); //内存地址
            i++;
            if(i>totalVar) printf("Error: stack overflow!\n");
        }else{
            it->second.tiggerName = num2string(i); //内存地址
            i+=(it->second.arrNum)/4;
            if(i>totalVar) printf("Error: stack overflow!\n");
        }
    }
    head->generator();
    DebugPrint();
    for(auto i:dynamic_cast<StmtsAST*>(StmtsASTptr)->stmts){
        if(i->type!=Decl) i->generator();
    }
    end->generator();
}
void FunDefAST::DebugPrint(){
    if(Debug){
        printf("************ function:%s ************\n", \
        dynamic_cast<FunHeadAST*>(head)->funName.c_str());
        int k = 1;
        for(EntryIT it = tempVar.begin();it!=tempVar.end();++it){
            printf("%d----\tEeyore: %s,  memory in tigger: %s\t----\n",k++, \
            it->first.c_str(), it->second.tiggerName.c_str());
        }
        printf("*************************************\n");
    }
}
void FunHeadAST::generator(){
    tiggerCode.push_back(funName+" ["+num2string(paraNum)+"]"+ \
    " ["+num2string(funcName->tNum+funcName->TNum + 8)+"]"); //留8个用于load store a0-a7
    if(paraNum>0){
        for(int i=0;i<paraNum;++i){
            Entry tmp("a"+num2string(i),false);
            funcName->tempVar.insert(pair<string,Entry>("p"+num2string(i),tmp));
        }
    }
}
void StmtsAST::generator(){
    for(auto i:stmts) i->generator();
}
void FunEndAST::generator(){
    tiggerCode.push_back("end "+funName);
}



void BinaryAST::generator(){
    bool flag2isNum = false;
    if(firstVal->isNum) {
        tiggerCode.push_back("\tt5 = "+firstVal->s);
    }else{
        if(ifGlobal(firstVal->s)){
            EntryIT it = globalVar.find(firstVal->s);
            if(it->second.isArray==0)tiggerCode.push_back("\tload  "+it->second.tiggerName+"  t5");
            else{tiggerCode.push_back("\tloadaddr  "+it->second.tiggerName+"  t5");}
        } else {
            EntryIT it = funcName->tempVar.find(firstVal->s);
            string memory = it->second.tiggerName;
            if(memory[0]!='a'){
                if(it->second.isArray==0)tiggerCode.push_back("\tload  "+memory+"  t5");
                else{tiggerCode.push_back("\tloadaddr  "+memory+"  t5");}
            }  
            else    tiggerCode.push_back("\tt5 = "+memory);
        }
    }

    if(secVal->isNum) {
        flag2isNum = true;
    }else{
        if(ifGlobal(secVal->s)){
            EntryIT it = globalVar.find(secVal->s);
            if(it->second.isArray==0) tiggerCode.push_back("\tload  "+it->second.tiggerName+"  t6");
            else {tiggerCode.push_back("\tloadaddr  "+it->second.tiggerName+"  t6");}
        } else {
            EntryIT it = funcName->tempVar.find(secVal->s);
            string memory = it->second.tiggerName;
            if(memory[0]!='a'){
                if(it->second.isArray==0) tiggerCode.push_back("\tload  "+memory+"  t6");
                else {tiggerCode.push_back("\tloadaddr  "+memory+"  t6");}
            }
            else    tiggerCode.push_back("\tt6 = "+memory);
        }
    }

    if(ifGlobal(leftName)){
        if(flag2isNum==false)   tiggerCode.push_back("\tt4 = t5 "+op+" t6");
        else    tiggerCode.push_back("\tt4 = t5 "+op+" "+secVal->s);

        tiggerCode.push_back("\tloadaddr  "+(globalVar.find(leftName))->second.tiggerName+"  t3");
        tiggerCode.push_back("\tt3[0] = t4");
    }else{
        if(flag2isNum==false)   tiggerCode.push_back("\tt4 = t5 "+op+" t6");
        else    tiggerCode.push_back("\tt4 = t5 "+op+" "+secVal->s);

        string memoryLeft = (funcName->tempVar.find(leftName))->second.tiggerName;
        if(memoryLeft[0]!='a') tiggerCode.push_back("\tstore t4 "+ memoryLeft);
        else tiggerCode.push_back("\t"+memoryLeft+" = t4");
    }
    
}


void UnaryAST::generator(){
    if(firstVal->isNum) {
        tiggerCode.push_back("\tt5 = "+firstVal->s);
    }else{
        if(ifGlobal(firstVal->s)){
            EntryIT it = globalVar.find(firstVal->s);
            if(it->second.isArray==0)tiggerCode.push_back("\tload  "+it->second.tiggerName+"  t5");
            else{tiggerCode.push_back("\tloadaddr  "+it->second.tiggerName+"  t5");}
        } else {
            EntryIT it = funcName->tempVar.find(firstVal->s);
            string memory = it->second.tiggerName;
            if(memory[0]!='a'){
                if(it->second.isArray==0) tiggerCode.push_back("\tload  "+memory+"  t5");
                else {tiggerCode.push_back("\tloadaddr  "+memory+"  t5");}
            }
            else tiggerCode.push_back("\tt5 = "+memory);
        }
    }
    tiggerCode.push_back("\tt4 = "+op+" t5 ");
    if(ifGlobal(leftName)){
        tiggerCode.push_back("\tloadaddr  "+(globalVar.find(leftName))->second.tiggerName+"  t3");
        tiggerCode.push_back("\tt3[0] = t4");
    }else{
        string memory = (funcName->tempVar.find(leftName))->second.tiggerName;
        if(memory[0]!='a') tiggerCode.push_back("\tstore t4 "+ memory );
        else tiggerCode.push_back("\t"+memory+" = t4");
    }
}


void AssignAST::generator(){
    switch (AssignType)
    {
    case 1:
        if(val2->isNum)  {
            if(ifGlobal(val1)){
                tiggerCode.push_back("\tt4 = "+val2->s);
                tiggerCode.push_back("\tloadaddr  "+(globalVar.find(val1))->second.tiggerName+"  t3");
                tiggerCode.push_back("\tt3[0] = t4");
            }else{
                string memory = (funcName->tempVar.find(val1))->second.tiggerName;
                tiggerCode.push_back("\tt5 = "+val2->s);
                if(memory[0]!='a') tiggerCode.push_back("\tstore t5 "+memory);
                else tiggerCode.push_back("\t"+memory+" = t5");
            }
        }else{
            if(ifGlobal(val1)){
                if(ifGlobal(val2->s)){
                    EntryIT it = globalVar.find(val2->s);
                    if(it->second.isArray==0)tiggerCode.push_back("\tload  "+ it->second.tiggerName+"  t4");
                    else {tiggerCode.push_back("\tloadaddr  "+ it->second.tiggerName+"  t4");}
                }else{
                    EntryIT it = funcName->tempVar.find(val2->s);
                    string memory = it->second.tiggerName;
                    if(memory[0]!='a'){
                        if(it->second.isArray==0)tiggerCode.push_back("\tload  "+memory+"  t4");
                        else {tiggerCode.push_back("\tloadaddr  "+memory+"  t4");}
                    }
                    else{tiggerCode.push_back("\tt4 = "+memory);}
                }
                tiggerCode.push_back("\tloadaddr  "+(globalVar.find(val1))->second.tiggerName+"  t3");
                tiggerCode.push_back("\tt3[0] = t4");
            }else{
                if(ifGlobal(val2->s)){
                    EntryIT it = globalVar.find(val2->s);
                    if(it->second.isArray==0)tiggerCode.push_back("\tload  "+ it->second.tiggerName+"  t4");
                    else {tiggerCode.push_back("\tloadaddr  "+ it->second.tiggerName+"  t4");}
                }else{
                    EntryIT it = (funcName->tempVar.find(val2->s));
                    string memory = it->second.tiggerName;
                    if(memory[0]!='a'){
                        if(it->second.isArray==0)tiggerCode.push_back("\tload  "+memory+"  t4");
                        else{tiggerCode.push_back("\tloadaddr  "+memory+"  t4");}
                    }
                    else {tiggerCode.push_back("\tt4 = "+memory);}
                }
                string memoryLeft = (funcName->tempVar.find(val1))->second.tiggerName;
                if(memoryLeft[0]!='a') tiggerCode.push_back("\tstore t4 "+memoryLeft);
                else {tiggerCode.push_back("\t"+memoryLeft+" = t4");}
            }
        }
        break;


    case 2:
        if(ifGlobal(val1)){
            tiggerCode.push_back("\tloadaddr "+(globalVar.find(val1))->second.tiggerName+"  t1");
        }else{
            string memory1 = (funcName->tempVar.find(val1))->second.tiggerName;
            if(memory1[0]!='a') tiggerCode.push_back("\tloadaddr "+memory1+" t1");
            else{
                tiggerCode.push_back("\tt1 = "+memory1);
            }
        }
        if(ifGlobal(val2->s)){
            tiggerCode.push_back("\tload "+(globalVar.find(val2->s))->second.tiggerName+"  t2");
        }else{
            if(val2->isNum){
                tiggerCode.push_back("\tt2 = "+val2->s);
            }else{
            string memory2 = (funcName->tempVar.find(val2->s))->second.tiggerName;
            if(memory2[0]!='a') tiggerCode.push_back("\tload "+memory2+" t2");
            else tiggerCode.push_back("\tt2 = "+memory2);
            }
        }
        tiggerCode.push_back("\tt4 = t1 + t2");
        if(val3->isNum){
            tiggerCode.push_back("\tt3 = "+val3->s);
            tiggerCode.push_back("\tt4[0] = t3");
            break;
        }
        if(ifGlobal(val3->s)){
            EntryIT it = globalVar.find(val3->s);
            if(it->second.isArray==0) tiggerCode.push_back("\tload "+ it->second.tiggerName+"  t3");
            else {tiggerCode.push_back("\tloadaddr "+ it->second.tiggerName+"  t3");}
        }else{
            if(val3->isNum){
                tiggerCode.push_back("\tt3 = "+val3->s);
            }else{
            EntryIT it = (funcName->tempVar.find(val3->s));
            string memory3 = it->second.tiggerName;
            if(memory3[0]!='a'){
                if(it->second.isArray==0)tiggerCode.push_back("\tload "+memory3+" t3");
                else{tiggerCode.push_back("\tloadaddr "+memory3+" t3");}
            }
            else tiggerCode.push_back("\tt3 = "+memory3);
            }
        }
        tiggerCode.push_back("\tt4[0] = t3");
        break;


    case 3:
        if(ifGlobal(s2)){
            tiggerCode.push_back("\tloadaddr "+(globalVar.find(s2))->second.tiggerName+"  t2");
        }else{
            string memory2 = (funcName->tempVar.find(s2))->second.tiggerName;
            if(memory2[0]!='a') tiggerCode.push_back("\tloadaddr "+memory2+" t2");
            else tiggerCode.push_back("\tt2 = "+memory2);
        }
        if(ifGlobal(val3->s)){
            tiggerCode.push_back("\tload "+(globalVar.find(val3->s))->second.tiggerName+"  t3");
        }else{
            if(val3->isNum){tiggerCode.push_back("\tt3 = "+val3->s);}
            else{
            string memory3 = (funcName->tempVar.find(val3->s))->second.tiggerName;
            if(memory3[0]!='a') tiggerCode.push_back("\tload "+memory3+" t3");
            else tiggerCode.push_back("\tt3 = "+memory3);
            }
        }
        tiggerCode.push_back("\tt4 = t2 + t3");
        if(ifGlobal(val1)){
            tiggerCode.push_back("\tloadaddr "+(globalVar.find(val1))->second.tiggerName+"  t1");
            tiggerCode.push_back("\tt1[0] = t4[0]");
        }else{
            string memory1 = (funcName->tempVar.find(val1))->second.tiggerName;
            if(memory1[0]!='a'){
                tiggerCode.push_back("\tt1 = t4[0]");
                tiggerCode.push_back("\tstore t1 "+memory1);
            } else{
                tiggerCode.push_back("\t"+memory1+" = t4[0]");
            } 
        }
        break;
    default:
        break;
    }
}


void IfAST::generator(){
    bool flag1isNum = false; bool flag2isNum = false;
    if(val1->isNum) {flag1isNum=true;}
    else{
        if(ifGlobal(val1->s)){
            EntryIT it = globalVar.find(val1->s);
            if(it->second.isArray==0) tiggerCode.push_back("\tload  "+ it->second.tiggerName+"  t5");
            else{tiggerCode.push_back("\tloadaddr  "+ it->second.tiggerName+"  t5");}
        } else {
            EntryIT it = (funcName->tempVar.find(val1->s));
            string memory = it->second.tiggerName;
            if(memory[0]!='a'){
                if(it->second.isArray==0) tiggerCode.push_back("\tload  "+memory+"  t5");
                else tiggerCode.push_back("\tloadaddr  "+memory+"  t5");
            } 
            else   tiggerCode.push_back("\tt5 = "+memory);
        }
    }

    if(val2->isNum) {flag2isNum=true;}
    else{
        if(ifGlobal(val2->s)){
            EntryIT it = globalVar.find(val2->s);
            if(it->second.isArray==0) tiggerCode.push_back("\tload  "+it->second.tiggerName+"  t6");
            else{tiggerCode.push_back("\tloadaddr  "+it->second.tiggerName+"  t6");}
        } else {
            EntryIT it = (funcName->tempVar.find(val2->s));
            string memory = it->second.tiggerName;
            if(memory[0]!='a'){
                if(it->second.isArray==0) tiggerCode.push_back("\tload  "+memory+"  t6");
                else tiggerCode.push_back("\tloadaddr  "+memory+"  t6");
            }
            else   tiggerCode.push_back("\tt6 = "+memory);
        }
    }

    if(flag1isNum==true && flag2isNum==true){
        tiggerCode.push_back("\tt5 = "+val1->s);
        tiggerCode.push_back("\tt6 = "+val2->s);
        tiggerCode.push_back("\tif  t5 "+ op +" t6  goto "+label);
        return ;
    }
    if(flag1isNum==false && flag2isNum==true){
        tiggerCode.push_back("\tt6 = "+val2->s);
        tiggerCode.push_back("\tif  t5 "+ op +" t6  goto "+label);
        return ;
    }
    if(flag1isNum==true && flag2isNum==false){
        tiggerCode.push_back("\tt5 = "+val1->s);
        tiggerCode.push_back("\tif  t5 "+ op +" t6  goto "+label);
        return ;
    }
    if(flag1isNum==false && flag2isNum==false){
        tiggerCode.push_back("\tif  t5 "+ op +" t6  goto "+label);
        return ;
    }
}
void GotoAST::generator(){
    tiggerCode.push_back("\tgoto "+label);
}
void CallAST::generator(){
    int stackV = funcName->TNum+funcName->tNum+8;
    if(isAssign){
        tiggerCode.push_back("\tcall  "+funName);
        if(ifGlobal(leftName)){
            tiggerCode.push_back("\tloadaddr  "+(globalVar.find(leftName))->second.tiggerName+"  t5");
            tiggerCode.push_back("\tt5[0] = a0");
        } else {
            string memory = (funcName->tempVar.find(leftName))->second.tiggerName;
            if(memory[0]!='a') tiggerCode.push_back("\tstore  a0  "+memory);
            else tiggerCode.push_back("\t"+memory+" = a0");
        }
    }else{
        tiggerCode.push_back("\tcall  "+funName);
    }
    for(int i=0;i<nowParam;++i){
        string an = "a"+num2string(i);
        tiggerCode.push_back("\tload  "+num2string(stackV-i-1)+" "+an);
    }
    nowParam = 0;

}
void RetAST::generator(){
    if(isVoid)  tiggerCode.push_back("\treturn");
    else{
        if(dynamic_cast<FunHeadAST*>(funcName->head)->funName=="f_main"){
            tiggerCode.push_back("\ta0 = 10\n\tcall f_putch");
        }
        if(retVal->isNum == 0){
            if(ifGlobal(retVal->s)){
                EntryIT it = globalVar.find(retVal->s);
                if(it->second.isArray==0)tiggerCode.push_back("\tload  "+ it->second.tiggerName+"  a0");
                else{tiggerCode.push_back("\tloadaddr  "+ it->second.tiggerName+"  a0");}
                if(dynamic_cast<FunHeadAST*>(funcName->head)->funName=="f_main"){
                }
                tiggerCode.push_back("\treturn");
            } else {
                EntryIT it = (funcName->tempVar.find(retVal->s));
                string memory = it->second.tiggerName;
                if(memory[0]!='a'){
                    if(it->second.isArray==0) tiggerCode.push_back("\tload  "+memory+"  a0");
                    else {tiggerCode.push_back("\tloadaddr  "+memory+"  a0");}
                } 
                else {tiggerCode.push_back("\ta0 = "+memory);}
                if(dynamic_cast<FunHeadAST*>(funcName->head)->funName=="f_main"){
                }
                tiggerCode.push_back("\treturn");
            }
        } else{
            tiggerCode.push_back("\ta0 = "+retVal->s);
            if(dynamic_cast<FunHeadAST*>(funcName->head)->funName=="f_main"){
            }
            tiggerCode.push_back("\treturn");
        }
    }
}
void LabelAST::generator(){
    tiggerCode.push_back(Label+":");
}
void ParamAST::generator(){
    if(nowParam>8){
        printf("Error: too many params!\n");
        return ;
    }

    int stackV = funcName->TNum+funcName->tNum+8;
    if(nowParam==0){
        for(int i = 0; i<8; ++i){
            string an = "a"+num2string(i);
            tiggerCode.push_back("\tstore  "+an+" "+num2string(stackV-i-1));
        }
    }

    if(p->isNum){
        tiggerCode.push_back("\ta"+num2string(nowParam)+" = "+p->s);
    }
    else{
        if(ifGlobal(p->s)){
            EntryIT it = globalVar.find(p->s);
            if(it->second.isArray==0)tiggerCode.push_back("\tload  "+ it->second.tiggerName+"  t5");
            else {tiggerCode.push_back("\tloadaddr  "+ it->second.tiggerName+"  t5");}
            tiggerCode.push_back("\ta"+ num2string(nowParam) + " = t5");
        } else {
            EntryIT it = (funcName->tempVar.find(p->s));
            string memory = it->second.tiggerName;
            if(memory[0]!='a'){
                if(it->second.isArray==0)tiggerCode.push_back("\tload  "+memory+"  t5");
                else tiggerCode.push_back("\tloadaddr  "+memory+"  t5");
            }
            else {
                tiggerCode.push_back("\tload  "+num2string(stackV-(memory[1]-'0')-1)+" t5");
            }
            tiggerCode.push_back("\ta"+ num2string(nowParam) + " = t5");
        }
    }
    nowParam++;
}
void RightValueAST::generator(){
}