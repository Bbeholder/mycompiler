int func(int n) {
  if (n <= 50) {
    return n;
  }
  else {
    return n;
  }
}

int main() {
  int i;

  if (func(0) == 1 || func(50) == 1 && func(100) == 0)
    i = 0;
  else
    i = 1;
  
  if (func(50) == 1 && func(40) == 1 || func(1) == 1 )
    i = 0;
  else
    i = 1;

  return 0;
}
