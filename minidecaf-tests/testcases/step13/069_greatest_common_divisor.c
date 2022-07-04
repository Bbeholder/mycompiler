int fun(int m,int n){
	int rem;			
	while(n > 0){
		rem = m % n;
		m = n;
		n = rem;
	}
	return m;				
}
int main(){
	int n;
	int m;
	int num;
	m=91;
	n=39;
	num=fun(m,n);
	return num; 
}
