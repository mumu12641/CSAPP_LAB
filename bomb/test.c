#include<stdio.h>

// int func4(int x ,int y,int z){
//     int result = x;
//     result = result - y;
//     int tmp = result;
//     result += (tmp>>31);
//     result >>= 1;

//     long a = result + y;

//     if (a <= z){
//         result = 0;
//         if(a >= z){
//             return result;
//         }else{
//             y = a + 1;
//             func4(x,y,z);
//         }
//      }else{
//             x = a -1;
//         func4(x,y,z);

//     }
//     return result;

// }
int func4(int a, int b, int c){
  int val = a;
  val -= b;
  val += val >> 31;
  val >>= 1;
  long tmp = b + val;
  printf("a = %d b = %d  val = %d tmp = %d,    \n",a,b,val,tmp);
  if(tmp > c) {
    a = tmp - 1;
    func4(a, b, c);
  }else {
    val = 0;
    if(tmp < c) {
        b = tmp + 1;
        func4(a, b, c);
    }else {
        return val;
    } 
  }
  return val;
}
int main(){
    printf("%d",func4(14,0,9));
    return 0;
}
