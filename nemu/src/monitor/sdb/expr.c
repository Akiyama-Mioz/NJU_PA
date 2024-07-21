/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

enum {
  TK_NOTYPE = 256, TK_EQ=255,TK_NUM=254,

  /* TODO: Add more token types */

};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"\\-", '-'},         // minus
  {"\\*", '*'},         // multiply
  {"\\/", '/'},         // divide
  {"\\(", '('},         // left bracket
  {"\\)", ')'},         // right bracket
  {"\\b[0-9]+\\b", TK_NUM},// number
  {"==", TK_EQ},        // equal
};

#define NR_REGEX ARRLEN(rules)//计算数组长度

static regex_t re[NR_REGEX] = {};//储存编译后的正则表达式类型

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);//regcomp函数将字符串形式的正则表达式编译成regex_t类型储存在re[i]，然后给regexec使用
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[500] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;//正则表达式库中定义的一个结构体类型,用来存放匹配的字符串开始和结束的信息

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s ",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */
        
        switch (rules[i].token_type) {
          case TK_NOTYPE : break;
//          case '+' : tokens[nr_token].type=rules[i].token_type;nr_token++;break;
//          case '-' : tokens[nr_token].type=rules[i].token_type;nr_token++;break;
//          case '*' : tokens[nr_token].type=rules[i].token_type;nr_token++;break;
//          case '/' : tokens[nr_token].type=rules[i].token_type;nr_token++;break;
//          case '(' : tokens[nr_token].type=rules[i].token_type;nr_token++;break;
//          case ')' : tokens[nr_token].type=rules[i].token_type;nr_token++;break;
//          case TK_NUM : tokens[nr_token].type=TK_NUM;tokens[nr_token].str[0]=e[position];break;
//          case TK_EQ : tokens[nr_token].type=e[position];break;
          default : {
              tokens[nr_token].type=rules[i].token_type;
              strncpy(tokens[nr_token].str,substr_start,substr_len);
              tokens[nr_token].str[substr_len] = '\0';//确保字符串结束符
              nr_token++;
              break;
          }
        }
        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

bool check_parentheses(int p,int q){
  if(p =='(' && q ==')'){
     return true;
  }
  return false;
}

int main_operator(){
  int parentheses = 0;//设计一个括号标记符，用于追踪括号
  int mmop=0;
  int op = 0;
  for(int i=0;i<nr_token;i++){
    if(tokens[i].type == '('){
        ++parentheses;
        continue;
    } 
    if(tokens[i].type == ')'){
        --parentheses;
        continue;
    } 
    if(parentheses==0){
      switch(tokens[i].type){
        case '+' : op=i;break;
        case '-' : op=i;break;
      }
    }
    if(parentheses==0 && op==mmop){
      switch(tokens[i].type){
        case '*' : op=i;break;
        case '/' : op=i;break;
      }
      mmop=op;
    }

  }
  return op;
}

uint32_t eval(int p,int q){
  int sum = 0;
  if(p > q){
    return 0;
  }
  else if(p == q){
    printf("val = %d\n",atoi(tokens[p].str));
    return atoi(tokens[p].str);
  }
  else if(check_parentheses(0,nr_token) == true){
    return eval(p+1,nr_token-1);
  }
  else{
    int op = main_operator();
    int val1 = eval(p,op-1);
    printf("val1 = %d\n",val1);
    int val2 = eval(op+1,q);
    printf("val2 = %d\n",val2);
    switch (tokens[op].type){
      case '+':{sum = val1+val2;break;}
      case '-':{sum = val1-val2;break;}
      case '*':{sum = val1*val2;break;}
      case '/':{sum = val1/val2;break;}
      default: printf("Invalid operator in expression!");assert(0);
    }
  }
  return sum;
}

word_t expr(char *e, bool *success) {
  int sum;
  if (!make_token(e)) {
    *success = false;
    return 0;
  }
  sum = eval(0,nr_token-1);
  /* TODO: Insert codes to evaluate the expression. */
  return sum;
}
